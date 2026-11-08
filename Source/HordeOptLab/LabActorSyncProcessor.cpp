#include "LabActorSyncProcessor.h"
#include "LabVatProcessor.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "LabMassTypes.h"
#include "LabMinion.h"
#include "LabMinionAnimInstance.h"
#include "MassActorSubsystem.h"
#include "MassActorSpawnerSubsystem.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassRepresentationFragments.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Engine/World.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

bool ULabActorManagement::IsActorReadyForRepresentation(const AActor& Actor) const
{
	const ALabMinionMassActor* Minion = Cast<ALabMinionMassActor>(&Actor);
	return !Minion || Minion->bPoseReady;
}

EMassActorSpawnRequestAction ULabActorManagement::OnPostActorSpawn(const FMassActorSpawnRequestHandle& SpawnRequestHandle, FConstStructView SpawnRequest, TSharedRef<FMassEntityManager> EntityManager) const
{
	// 새로 스폰된 액터도 엔진이 보이게 만든 직후다(MassActorSpawnerSubsystem.cpp:328). 포즈를 맞출 때까지 숨긴다.
	if (ALabMinionMassActor* Minion = Cast<ALabMinionMassActor>(SpawnRequest.Get<const FMassActorSpawnRequest>().SpawnedActor.Get()))
	{
		Minion->HideUntilPoseReady();
	}
	return Super::OnPostActorSpawn(SpawnRequestHandle, SpawnRequest, EntityManager);
}

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
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
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
	const bool bSigBudget = FLabConfig::Get().Variant == ELabVariant::M3BudgetSig;
	int32 Actors = 0;
	int32 Rebinds = 0;
	int32 DbgHoles = 0, DbgOverlaps = 0, DbgSwitches = 0;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
		const TConstArrayView<FMassRepresentationFragment> Representations = Context.GetFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FLabMinionFragment> Minions = Context.GetFragmentView<FLabMinionFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> LODs = Context.GetFragmentView<FMassRepresentationLODFragment>();
		const bool bHit = Context.DoesArchetypeHaveTag(*FLabHitTag::StaticStruct());
		const bool bDead = Context.DoesArchetypeHaveTag(*FLabDeadTag::StaticStruct());
		const ELabMinionState State = bDead ? ELabMinionState::Dead : bHit ? ELabMinionState::Hit : ELabMinionState::Walk;

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			const EMassRepresentationType Rep = Representations[It].CurrentRepresentation;
#if !UE_BUILD_TEST
			{
				// 전환 진단(개발 빌드만): 그려지는 것이 없는 엔티티(구멍)와 둘 다 그려지는 엔티티(겹침)를 센다.
				const FMassRepresentationFragment& R = Representations[It];
				const AActor* A = ActorList[It].Get();
				const bool bActorVisible = A && !A->IsHidden();
				const bool bIsmDrawn = R.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance
					|| (R.IsWaitingForActorVisualReadiness() && R.PrevRepresentation == EMassRepresentationType::StaticMeshInstance);
				const bool bWantsDrawn = R.CurrentRepresentation != EMassRepresentationType::None;
				if (bWantsDrawn && !bActorVisible && !bIsmDrawn) { ++DbgHoles; }
				if (bActorVisible && bIsmDrawn) { ++DbgOverlaps; }
				if (R.PrevRepresentation != R.CurrentRepresentation && !R.IsWaitingForActorVisualReadiness()) { ++DbgSwitches; }
			}
#endif
			if (Rep != EMassRepresentationType::HighResSpawnedActor && Rep != EMassRepresentationType::LowResSpawnedActor)
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
				// 이 프레임에는 아직 포즈가 계산되지 않았다. 메시가 두 번 틱할 때까지 숨기고, 그동안 VAT 인스턴스가 대신 보인다.
				Actor->HideUntilPoseReady();
				Actor->PendingReadyFrames = 2;
				SkelMesh->ResetSceneVelocity();
				SkelMesh->ClearMotionVector();
				++Rebinds;
#if !UE_BUILD_TEST
				LabLogScreenEvent(TEXT("hydrate"), Crowd->SnipeView, Xf.GetLocation());
#endif
			}
			else if (!Actor->bPoseReady && Actor->BoundSerial == Minion.Serial && --Actor->PendingReadyFrames <= 0)
			{
				Actor->ShowPoseReady(); // ULabActorManagement가 엔진에 "준비됨"을 알리고, 엔진이 VAT 인스턴스를 뗀다
				// 풀에 들어가기 전의 뼈 변환이 이전 프레임으로 남지 않게 몇 프레임 동안 모션 벡터를 지운다(보조 대책).
				Actor->ClearVelocityFrames = 2;
#if !UE_BUILD_TEST
				LabLogScreenEvent(TEXT("shown"), Crowd->SnipeView, Xf.GetLocation());
#endif
			}
			else if (Anim && Actor->AppliedState != State)
			{
				Anim->StartTime = 0.f;
				Anim->State = State;
				Actor->AppliedState = State;
			}
			if (Actor->bPoseReady && Actor->ClearVelocityFrames > 0)
			{
				--Actor->ClearVelocityFrames;
				SkelMesh->ResetSceneVelocity();      // PrimitiveComponent.h:2745 "Useful to prevent motion blur when teleporting"
				SkelMesh->ClearMotionVector();       // SkinnedMeshComponent.h:430 뼈 속도(모션 블러·TAA)를 지운다
			}

			if (bSigBudget)
			{
				// E3 (3b): Mass는 0이 High, Budget은 큰 값이 중요하다(engine-notes.md 3절)
				if (USkeletalMeshComponentBudgeted* Budgeted = Cast<USkeletalMeshComponentBudgeted>(SkelMesh))
				{
					Budgeted->SetComponentSignificance(1.f - FMath::Clamp(LODs[It].LODSignificance / 3.f, 0.f, 1.f));
				}
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
	if (DbgHoles > 0 || DbgOverlaps > 0 || DbgSwitches > 0)
	{
		UE_LOG(LogLab, Log, TEXT("lab_transition frame=%llu holes=%d overlaps=%d switches=%d rebinds=%d"), (unsigned long long)GFrameCounter, DbgHoles, DbgOverlaps, DbgSwitches, Rebinds);
	}
}
