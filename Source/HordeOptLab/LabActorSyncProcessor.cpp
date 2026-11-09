#include "LabActorSyncProcessor.h"
#include "LabVatProcessor.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabMassProcessors.h" // LabLogicGroupName, LabUseOldQueries
#include "MassActorSpawnerSubsystem.h"
#include "MassEntityManager.h"
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
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

namespace
{
	/** 액터 태그 필터를 쓰는가. m3qold(정비 전)와 m3syncall(필터만 없음)은 태그를 붙이지도 거르지도 않는다. */
	bool LabUseActorTag()
	{
		const ELabVariant V = FLabConfig::Get().Variant;
		return V != ELabVariant::M3QOld && V != ELabVariant::M3SyncAll;
	}
}

bool ULabActorManagement::IsActorReadyForRepresentation(const AActor& Actor) const
{
	const ALabMinionMassActor* Minion = Cast<ALabMinionMassActor>(&Actor);
	return !Minion || Minion->bPoseReady || !FLabConfig::Get().HasFix(FLabConfig::FixWait);
}

void ULabActorManagement::TeleportActor(const FTransform& Transform, AActor& Actor, FMassCommandBuffer& CommandBuffer) const
{
	// 엔진은 액터를 붙일 때와 준비가 끝날 때 엔티티 변환(발밑, z=0)으로 액터를 옮긴다(MassRepresentationProcessor.cpp:272, 428).
	// 우리 액터의 원점은 캡슐 중심이라 그대로 두면 그 프레임에 88cm 가라앉았다가 다음 프레임 동기화에서 올라온다.
	// 그 수직 이동이 모션 블러로 세로 줄무늬 잔상을 만들었다(2026-11-08 60fps 연속 캡처). 동기화 프로세서와 같은 높이로 옮긴다.
	FTransform Lifted = Transform;
	if (const ACharacter* Character = Cast<ACharacter>(&Actor); Character && FLabConfig::Get().HasFix(FLabConfig::FixLift))
	{
		Lifted.AddToTranslation(FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
	}
	Super::TeleportActor(Lifted, Actor, CommandBuffer);
}

EMassActorSpawnRequestAction ULabActorManagement::OnPostActorSpawn(const FMassActorSpawnRequestHandle& SpawnRequestHandle, FConstStructView SpawnRequest, TSharedRef<FMassEntityManager> EntityManager) const
{
	// 새로 스폰된 액터도 엔진이 보이게 만든 직후다(MassActorSpawnerSubsystem.cpp:328). 포즈를 맞출 때까지 숨긴다.
	const FMassActorSpawnRequest& Request = SpawnRequest.Get<const FMassActorSpawnRequest>();
	ALabMinionMassActor* Minion = Cast<ALabMinionMassActor>(Request.SpawnedActor.Get());
	if (Minion && FLabConfig::Get().HasFix(FLabConfig::FixWait))
	{
		Minion->HideUntilPoseReady();
	}
	if (Request.SpawnedActor && LabUseActorTag())
	{
		// 액터가 엔티티에 붙는 유일한 길목이다(새 스폰과 풀 재사용 모두, MassActorSpawnerSubsystem.cpp:255,480). 스포너 서브시스템 틱(게임 스레드)에서
		// 불리고 그때 Mass 처리가 워커에서 돌고 있을 수 있어 아키타입을 바로 바꾸지 않고 지연 명령으로 태그를 붙인다(MassEntityManager.h:1625).
		EntityManager->Defer().AddTag<FLabActorTag>(Request.MassAgent);
	}
	return Super::OnPostActorSpawn(SpawnRequestHandle, SpawnRequest, EntityManager);
}

ULabActorSyncProcessor::ULabActorSyncProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Representation);
	if (!LabUseOldQueries())
	{
		ExecutionOrder.ExecuteAfter.Add(LabLogicGroupName); // 로직이 이 프레임에 바꾼 표현 상태를 읽는다(M3b 정비 2)
	}
	bRequiresGameThreadExecution = true;
	// M3부터만 돈다. 게임 모듈이 단계를 보고 켠다(HordeOptLab.cpp). M1·M2 실행에 빈 순회 비용을 더하지 않는다.
	bAutoRegisterWithProcessingPhases = false;
}

void ULabActorSyncProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	if (LabUseOldQueries())
	{
		EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadOnly); // m3qold: 로직 프래그먼트를 읽는다
	}
	else
	{
		EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadOnly);
	}
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	if (LabUseActorTag())
	{
		EntityQuery.AddTagRequirement<FLabActorTag>(EMassFragmentPresence::All); // 액터가 붙은 아키타입만 연다(M3b 정비 4)
	}
}

void ULabActorSyncProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	CSV_SCOPED_TIMING_STAT(Lab, ActorSync); // 병목 분석용(Exclusive/GameThread/TickActors를 나눠 보려고, 2026-11-09)
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
	const bool bFixStartTime = FLabConfig::Get().HasFix(FLabConfig::FixStartTime);
	const bool bFixWait = FLabConfig::Get().HasFix(FLabConfig::FixWait);
	const bool bOld = LabUseOldQueries();
	const bool bActorTag = LabUseActorTag();
	int32 Actors = 0;
	int32 Rebinds = 0;
	int32 TagsRemoved = 0;
	int32 DbgHoles = 0, DbgOverlaps = 0, DbgSwitches = 0;
	// 진단(-LabVatTiming): 액터 이동(SetActorLocationAndRotation)만 따로 잰다(2026-11-09, ActorSync 비용 나누기)
	static const bool bTiming = FParse::Param(FCommandLine::Get(), TEXT("LabVatTiming"));
	uint64 CyMove = 0;
#if !UE_BUILD_TEST
	// E3b (8) 화질 캡처(-LabFrameLog): 화면 안의 보이는 액터 수와 그중 30m 안의 수(LabCrowdSubsystem::FrameDiag)
	static const bool bFrameLog = FParse::Param(FCommandLine::Get(), TEXT("LabFrameLog"));
	const FLabSnipeView FrameView = Crowd->SnipeView;
#endif

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
		const TConstArrayView<FMassRepresentationFragment> Representations = Context.GetFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FLabAnimStateFragment> AnimStates = bOld ? TConstArrayView<FLabAnimStateFragment>() : Context.GetFragmentView<FLabAnimStateFragment>();
		const TConstArrayView<FLabMinionFragment> Minions = bOld ? Context.GetFragmentView<FLabMinionFragment>() : TConstArrayView<FLabMinionFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> LODs = Context.GetFragmentView<FMassRepresentationLODFragment>();
		// m3qold: 상태는 아키타입 태그에서. 새 구조: 엔티티마다 표현 프래그먼트에서(아래 루프 안).
		const bool bHit = bOld && Context.DoesArchetypeHaveTag(*FLabHitTag::StaticStruct());
		const bool bDead = bOld && Context.DoesArchetypeHaveTag(*FLabDeadTag::StaticStruct());
		const ELabMinionState ChunkState = bDead ? ELabMinionState::Dead : bHit ? ELabMinionState::Hit : ELabMinionState::Walk;

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			const EMassRepresentationType Rep = Representations[It].CurrentRepresentation;
			const FMassEntityHandle Entity = Context.GetEntity(It);
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
			ALabMinionMassActor* Actor = Cast<ALabMinionMassActor>(ActorList[It].GetMutable());
			if (!Actor)
			{
				// 태그는 있는데 액터가 없다 = 엔진이 액터를 떼어 풀에 넣었다(ReleaseActorOrCancelSpawning은 static이라 훅이 없다, MassRepresentationProcessor.cpp:481).
				// 표현 종류가 아니라 액터 포인터로 판단한다: 액터를 한 프레임 더 두는 동안(bKeepActorExtraFrame) LOD가 되돌아오면 엔진이 같은 액터를 다시 써서
				// OnPostActorSpawn이 다시 불리지 않기 때문이다. 필터가 없는 변형에서는 스폰 요청 중인 엔티티도 여기로 온다.
				if (bActorTag)
				{
					Context.Defer().RemoveTag<FLabActorTag>(Entity);
					++TagsRemoved;
				}
				continue;
			}
			if (Rep != EMassRepresentationType::HighResSpawnedActor && Rep != EMassRepresentationType::LowResSpawnedActor)
			{
				continue; // 액터는 아직 붙어 있지만 이번 프레임은 VAT 인스턴스로 그린다(한 프레임 더 두기)
			}
			++Actors;
#if !UE_BUILD_TEST
			if (bFrameLog && Actor->bPoseReady)
			{
				const FVector Loc = Transforms[It].GetTransform().GetLocation();
				if (FrameView.Contains(Loc + FVector(0.f, 0.f, 90.f)))
				{
					++Crowd->FrameDiag[GFrameCounter & 1].ViewActors;
					if (FVector::Dist2D(Loc, FrameView.Location) < 3000.f) { ++Crowd->FrameDiag[GFrameCounter & 1].NearActors; }
				}
			}
#endif
			// 상태와 재생 시각(VAT 인스턴스와 같은 식, LabVatProcessor.cpp)
			ELabMinionState State;
			float T;
			if (bOld)
			{
				const FLabMinionFragment& Minion = Minions[It];
				State = ChunkState;
				T = bDead ? DeathDuration - Minion.StateTimeLeft : bHit ? HitDuration - Minion.StateTimeLeft : FMath::Fmod(Now + LabWalkPhase(Minion.Serial), WalkLength);
			}
			else
			{
				const FLabAnimStateFragment& Anim = AnimStates[It];
				State = Anim.State;
				T = Now - Anim.StateStartTime;
				if (State == ELabMinionState::Walk) { T = FMath::Fmod(T, WalkLength); }
			}

			// 위치: 엔티티는 발밑(지면), 액터는 캡슐 중심이다(M0 스포너의 SpawnZ와 같은 관계).
			const FTransform& Xf = Transforms[It].GetTransform();
			const float HalfHeight = Actor->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
			const uint64 M0 = bTiming ? FPlatformTime::Cycles64() : 0;
			Actor->SetActorLocationAndRotation(Xf.GetLocation() + FVector(0.f, 0.f, HalfHeight), Xf.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
			if (bTiming) { CyMove += FPlatformTime::Cycles64() - M0; }

			USkeletalMeshComponent* SkelMesh = Actor->GetMesh();
			ULabMinionAnimInstance* Anim = Cast<ULabMinionAnimInstance>(SkelMesh->GetAnimInstance());
			if (Anim && Actor->BoundEntity != Entity)
			{
				// 새로 붙은 액터: VAT 인스턴스가 재생하던 시각(T)부터 재생한다.
				Anim->State = State;
				if (bFixStartTime)
				{
					Anim->StartTime = FMath::Max(T, 0.f);
					Anim->InitializeAnimation(); // 시퀀스 플레이어가 StartTime부터 다시 시작한다(AnimNode_SequencePlayer.cpp:61)
				}
				Actor->BoundEntity = Entity;
				Actor->AppliedState = State;
				if (bFixWait)
				{
					// 이 프레임에는 아직 포즈가 계산되지 않았다. 메시가 두 번 틱할 때까지 숨기고, 그동안 VAT 인스턴스가 대신 보인다.
					Actor->HideUntilPoseReady();
					Actor->PendingReadyFrames = 2;
					SkelMesh->ResetSceneVelocity();
					SkelMesh->ClearMotionVector();
				}
				else
				{
					Actor->bPoseReady = true;
				}
				++Rebinds;
#if !UE_BUILD_TEST
				LabLogScreenEvent(TEXT("hydrate"), Crowd->SnipeView, Xf.GetLocation());
#endif
			}
			else if (!Actor->bPoseReady && Actor->BoundEntity == Entity && --Actor->PendingReadyFrames <= 0)
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
			const float DeadAmount = State == ELabMinionState::Dead ? FMath::Clamp(T / DeathDuration * 2.5f, 0.f, 1.f) : 0.f;
			if (DeadAmount != Actor->AppliedDeadAmount)
			{
				SkelMesh->SetCustomPrimitiveDataFloat(0, DeadAmount);
				Actor->AppliedDeadAmount = DeadAmount;
			}
		}
	});

	CSV_CUSTOM_STAT(Lab, Actors, Actors, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, ActorRebinds, Rebinds, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, ActorTagsRemoved, TagsRemoved, ECsvCustomStatOp::Set); // 태그 필터 확인용: 떼어진 액터 수와 맞아야 한다
	if (bTiming)
	{
		CSV_CUSTOM_STAT(Lab, ActorMoveMs, float(FPlatformTime::ToMilliseconds64(CyMove)), ECsvCustomStatOp::Set);
	}
	if (GFrameCounter % 300 == 0)
	{
#if !UE_BUILD_TEST
		// 남는 액터 진단: 월드의 하이드레이션 액터를 상태별로 센다(보임 / 숨김(풀) / 화면 패스 꺼짐(대기)).
		int32 Total = 0, Hidden = 0, Visible = 0, Shadow = 0;
		for (TActorIterator<ALabMinionMassActor> AIt(World); AIt; ++AIt)
		{
			++Total;
			const USkeletalMeshComponent* M = AIt->GetMesh();
			if (AIt->IsHidden()) { ++Hidden; }
			else if (M->bRenderInMainPass) { ++Visible; }
			if (!AIt->IsHidden() && M->CastShadow) { ++Shadow; }
		}
		UE_LOG(LogLab, Log, TEXT("lab_actors synced=%d rebinds=%d world_total=%d hidden=%d visible=%d shadow_casting=%d"), Actors, Rebinds, Total, Hidden, Visible, Shadow);
#endif
	}
	if (DbgHoles > 0 || DbgOverlaps > 0 || DbgSwitches > 0)
	{
		UE_LOG(LogLab, Log, TEXT("lab_transition frame=%llu holes=%d overlaps=%d switches=%d rebinds=%d"), (unsigned long long)GFrameCounter, DbgHoles, DbgOverlaps, DbgSwitches, Rebinds);
	}
}
