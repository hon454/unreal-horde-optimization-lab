#include "LabActorSyncProcessor.h"
#include "HordeOptLab.h"
#include "LabCrowdSubsystem.h"
#include "LabMassTypes.h"
#include "LabMinion.h"
#include "LabMinionAnimInstance.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassRepresentationFragments.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Engine/World.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

ULabActorSyncProcessor::ULabActorSyncProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Representation);
	bRequiresGameThreadExecution = true;
	// M3부터만 돈다. 게임 모듈이 단계를 보고 켠다(HordeOptLab.cpp). M1·M2 실행에 빈 순회 비용을 더하지 않는다.
	bAutoRegisterWithProcessingPhases = false;
}

void ULabActorSyncProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
}

void ULabActorSyncProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = Context.GetWorld();
	const ULabCrowdSubsystem* Crowd = World ? World->GetSubsystem<ULabCrowdSubsystem>() : nullptr;
	if (!Crowd || !Crowd->IsActive())
	{
		return;
	}
	const FLabVatClips& Clips = Crowd->VatClips;
	const float WalkLength = float(Clips.Walk.EndFrame - Clips.Walk.StartFrame + 1) / Clips.SampleRate;
	const float Now = World->GetTimeSeconds();
	const float HitDuration = Crowd->HitDuration;
	const float DeathDuration = Crowd->DeathDuration;
	int32 Actors = 0;
	int32 Rebinds = 0;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
		const TConstArrayView<FMassRepresentationFragment> Representations = Context.GetFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FLabMinionFragment> Minions = Context.GetFragmentView<FLabMinionFragment>();
		const bool bHit = Context.DoesArchetypeHaveTag(*FLabHitTag::StaticStruct());
		const bool bDead = Context.DoesArchetypeHaveTag(*FLabDeadTag::StaticStruct());
		const ELabMinionState State = bDead ? ELabMinionState::Dead : bHit ? ELabMinionState::Hit : ELabMinionState::Walk;

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			if (Representations[It].CurrentRepresentation != EMassRepresentationType::HighResSpawnedActor)
			{
				continue;
			}
			ALabMinionMassActor* Actor = Cast<ALabMinionMassActor>(ActorList[It].GetMutable());
			if (!Actor)
			{
				continue; // 스폰 요청 중(시간 분할 스폰, MassSimulationSettings DesiredActorSpawningTimeSlicePerTick)
			}
			++Actors;

			// 위치: 엔티티는 발밑(지면), 액터는 캡슐 중심이다(M0 스포너의 SpawnZ와 같은 관계).
			const FTransform& Xf = Transforms[It].GetTransform();
			const float HalfHeight = Actor->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
			Actor->SetActorLocationAndRotation(Xf.GetLocation() + FVector(0.f, 0.f, HalfHeight), Xf.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);

			const FLabMinionFragment& Minion = Minions[It];
			USkeletalMeshComponent* SkelMesh = Actor->GetMesh();
			ULabMinionAnimInstance* Anim = Cast<ULabMinionAnimInstance>(SkelMesh->GetAnimInstance());
			if (Anim && Actor->BoundSerial != Minion.Serial)
			{
				// 새로 붙은 액터: VAT 인스턴스(LabVatProcessor.cpp)와 같은 식으로 지금 재생 시각을 구해 그 시각부터 재생한다.
				float T = 0.f;
				switch (State)
				{
				case ELabMinionState::Walk: T = FMath::Fmod(Now + float(Minion.Serial % 997u) * 0.1031f, WalkLength); break;
				case ELabMinionState::Hit: T = HitDuration - Minion.StateTimeLeft; break;
				case ELabMinionState::Dead: T = DeathDuration - Minion.StateTimeLeft; break;
				}
				Anim->State = State;
				Anim->StartTime = FMath::Max(T, 0.f);
				Anim->InitializeAnimation(); // 시퀀스 플레이어가 StartTime부터 다시 시작한다(AnimNode_SequencePlayer.cpp:61)
				Actor->BoundSerial = Minion.Serial;
				Actor->AppliedState = State;
				++Rebinds;
			}
			else if (Anim && Actor->AppliedState != State)
			{
				Anim->StartTime = 0.f;
				Anim->State = State;
				Actor->AppliedState = State;
			}

			// 죽기 진홍색: VAT 머티리얼과 같은 곡선(진행도 x 2.5, 0~1로 자름, Scripts/bake-vat.py)
			const float DeadAmount = bDead ? FMath::Clamp((DeathDuration - Minion.StateTimeLeft) / DeathDuration * 2.5f, 0.f, 1.f) : 0.f;
			if (DeadAmount != Actor->AppliedDeadAmount)
			{
				SkelMesh->SetCustomPrimitiveDataFloat(0, DeadAmount);
				Actor->AppliedDeadAmount = DeadAmount;
			}
		}
	});

	CSV_CUSTOM_STAT(Lab, Actors, Actors, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, ActorRebinds, Rebinds, ECsvCustomStatOp::Set);
	if (GFrameCounter % 300 == 0)
	{
		UE_LOG(LogLab, Log, TEXT("lab_actors actors=%d rebinds=%d"), Actors, Rebinds); // 개발 실행 확인용(Test 구성은 로그가 없다)
	}
}
