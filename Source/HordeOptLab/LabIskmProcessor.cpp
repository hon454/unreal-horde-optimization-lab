#include "LabIskmProcessor.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabMassTypes.h"
#include "LabVatProcessor.h" // LabLogScreenEvent(개발 빌드 전환 사건 로그)
#include "MassRepresentationTypes.h" // UE::Mass::Representation::IsValidActorRepresentation
#include "Animation/AnimSequenceTransformProviderData.h"
#include "Components/InstancedSkinnedMeshComponent.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassLODFragments.h"
#include "MassRepresentationFragments.h"
#include "MassRepresentationSubsystem.h"
#include "MassSkinnedMeshRepresentationTypes.h"
#include "MassUpdateSkinnedMeshProcessor.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

namespace
{
	/**
	 * 엔티티의 기존 트랙에 애니 데이터를 쓴다. 트랙이 아직 없으면(처음 ISKM으로 보이는 프레임) false를 돌려준다.
	 * 그때는 엔진이 같은 프레임 EndVisualChanges에서 트랙을 만들고 이 프레임에 쌓은 애니 데이터로 쓴다(MassVisualizationComponent.cpp:771-775, 862-866).
	 */
	bool LabWriteTrack(FMassInstancedSkinnedMeshInfo& Info, float LODSignificance, FMassEntityHandle Entity, const FAnimSequenceTrackAutoPlayData& Data)
	{
		FMassLODInstancedSkinnedMeshSignificanceRange* Range = Info.GetLODSignificanceRange(LODSignificance);
		if (!Range || !Range->InstancedSkinnedMeshSharedDataPtr)
		{
			return false;
		}
		bool bWrote = false;
		for (const FInstancedSkinnedMeshComponentSharedDataKey& Key : Range->SkinnedMeshComponentRefs)
		{
			FMassInstancedSkinnedMeshComponentSharedData* Shared = Range->InstancedSkinnedMeshSharedDataPtr->Find(Key);
			UInstancedSkinnedMeshComponent* Component = Shared ? Shared->GetMutableInstancedSkinnedMeshComponent() : nullptr;
			if (!Component)
			{
				continue;
			}
			const int32* TrackId = Shared->GetEntityToTrackMap().Find(Entity);
			UAnimSequenceTransformProviderDataInstance* Provider = Cast<UAnimSequenceTransformProviderDataInstance>(Component->GetTransformProvider());
			if (TrackId && Provider)
			{
				Provider->SetAutoPlayData(*TrackId, 0, Data);
				bWrote = true;
			}
		}
		return bWrote;
	}

#if !UE_BUILD_TEST
	/**
	 * Q1b 개발 확인(Q2 3단위): 엔티티 트랙의 엔진 쪽 재생 위치·배율·클립 번호를 읽는다. GPU로 가는 것과 같은 묶음 데이터(FAnimSequenceTrackPackedData)에서
	 * 엔진이 계산한 값이다(AnimSequenceTransformProviderData.cpp:1879-1913 GetPosition, 1816 GetPlayRate).
	 */
	bool LabReadTrack(FMassInstancedSkinnedMeshInfo& Info, float LODSignificance, FMassEntityHandle Entity, float& OutPos, float& OutRate, int32& OutSeq)
	{
		FMassLODInstancedSkinnedMeshSignificanceRange* Range = Info.GetLODSignificanceRange(LODSignificance);
		if (!Range || !Range->InstancedSkinnedMeshSharedDataPtr)
		{
			return false;
		}
		for (const FInstancedSkinnedMeshComponentSharedDataKey& Key : Range->SkinnedMeshComponentRefs)
		{
			FMassInstancedSkinnedMeshComponentSharedData* Shared = Range->InstancedSkinnedMeshSharedDataPtr->Find(Key);
			UInstancedSkinnedMeshComponent* Component = Shared ? Shared->GetMutableInstancedSkinnedMeshComponent() : nullptr;
			const int32* TrackId = Shared ? Shared->GetEntityToTrackMap().Find(Entity) : nullptr;
			const UAnimSequenceTransformProviderDataInstance* Provider = Component ? Cast<UAnimSequenceTransformProviderDataInstance>(Component->GetTransformProvider()) : nullptr;
			if (TrackId && Provider)
			{
				OutPos = Provider->GetPosition(*TrackId, 0).TargetPosition;
				OutRate = Provider->GetPlayRate(*TrackId, 0);
				OutSeq = Provider->GetSequenceIndex(*TrackId, 0);
				return true;
			}
		}
		return false;
	}
#endif

	/** ISKM 컴포넌트마다 트랙 맵 크기(Lab/IskmTracks)와 인스턴스 수(Lab/IskmInstances)를 더한다. 컴포넌트는 종류마다 하나라 몇 개뿐이다. */
	void LabCountIskm(UMassRepresentationSubsystem& RepresentationSubsystem, int32& OutTracks, int32& OutInstances)
	{
		OutTracks = 0;
		OutInstances = 0;
		TArray<const FMassInstancedSkinnedMeshComponentSharedData*, TInlineAllocator<4>> Seen;
		FMassInstancedSkinnedMeshInfoArrayView Infos = RepresentationSubsystem.GetMutableInstancedSkinnedMeshInfos();
		for (int32 InfoIndex = 0; InfoIndex < Infos.Num(); ++InfoIndex)
		{
			FMassInstancedSkinnedMeshInfo& Info = Infos[InfoIndex];
			if (!Info.IsValid())
			{
				continue;
			}
			// 범위 배열은 보호 멤버라 LOD 값마다 범위를 찾는다(Lab 설명자는 High~Max 한 범위다).
			for (int32 Lod = 0; Lod < EMassLOD::Max; ++Lod)
			{
				FMassLODInstancedSkinnedMeshSignificanceRange* Range = Info.GetLODSignificanceRange(float(Lod));
				if (!Range || !Range->InstancedSkinnedMeshSharedDataPtr)
				{
					continue;
				}
				for (const FInstancedSkinnedMeshComponentSharedDataKey& Key : Range->SkinnedMeshComponentRefs)
				{
					const FMassInstancedSkinnedMeshComponentSharedData* Shared = Range->InstancedSkinnedMeshSharedDataPtr->GetDataForKey(Key);
					if (!Shared || Seen.Contains(Shared))
					{
						continue;
					}
					Seen.Add(Shared);
					OutTracks += Shared->GetEntityToTrackMap().Num();
					if (const UInstancedSkinnedMeshComponent* Component = Shared->GetInstancedSkinnedMeshComponent())
					{
						OutInstances += Component->GetInstanceCount();
					}
				}
			}
		}
	}
}

ULabIskmProcessor::ULabIskmProcessor()
	: EntityQuery(*this)
{
	// 기본은 등록하지 않는다. 게임 모듈이 M5 ISKM 변형에서만 켠다(HordeOptLab.cpp).
	bAutoRegisterWithProcessingPhases = false;
	// 엔진 ISKM 프로세서와 같은 자리와 조건(MassUpdateSkinnedMeshProcessor.cpp:16-24). 컴포넌트와 트랙 제공자를 고치므로 게임 스레드에서 돈다.
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::VisualizationProcessing);
	bRequiresGameThreadExecution = true;
}

void ULabIskmProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	// 엔진 ISKM 프로세서의 쿼리(MassUpdateSkinnedMeshProcessor.cpp:42-54)에 Lab 표현 상태와 트랙 기록을 더한다.
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabIskmTrackFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FLabUnitTypeParams>(EMassFragmentPresence::All);
	EntityQuery.AddChunkRequirement<FMassVisualizationChunkFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.SetChunkFilter(&FMassVisualizationChunkFragment::AreAnyEntitiesVisibleInChunk);
	EntityQuery.AddSharedRequirement<FMassRepresentationSubsystemSharedFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UMassRepresentationSubsystem>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FMassStaticRepresentationTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
}

void ULabIskmProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	CSV_SCOPED_TIMING_STAT(Lab, IskmUpdate);
	UWorld* World = Context.GetWorld();
	const ULabCrowdSubsystem* Crowd = World ? World->GetSubsystem<ULabCrowdSubsystem>() : nullptr;
	if (!Crowd || !Crowd->IsActive())
	{
		return;
	}
	const float Now = World->GetTimeSeconds();
	const float DeathDuration = Crowd->DeathDuration;
	const float BlendTime = FLabConfig::Get().IskmParams().BlendTime;
	const FVector ViewLocation = Crowd->SnipeView.Location;
	int32 BandCounts[4] = { 0, 0, 0, 0 };
	// Q4 품질 지표: 화면 안에서 아직 인스턴스로 그리는 개체(액터 준비 대기 포함) 중 카메라 30m 안·전환 거리 안의 수(3D 거리, 표현 LOD와 같다)
	const FLabSnipeView QualityView = Crowd->SnipeView;
	const float Near30Sq = FMath::Square(3000.f);
	const float NearHydSq = FMath::Square(FLabConfig::Get().HydrateCm());
	const float NearMaxSq = FMath::Max(Near30Sq, NearHydSq);
	int32 ViewInst30 = 0, ViewInstHyd = 0;
#if !UE_BUILD_TEST
	static const bool bFrameLog = FParse::Param(FCommandLine::Get(), TEXT("LabFrameLog"));
	std::atomic<int32>* NearInstCounter = bFrameLog ? &Crowd->FrameDiag[GFrameCounter & 1].NearVat : nullptr;
#endif
	int32 TrackWrites = 0;  // 이 프로세서가 기존 트랙에 직접 쓴 수(상태 전환, 풀에서 다시 나옴)
	int32 TrackFirst = 0;    // 트랙이 없어 엔진이 이 프레임에 만들 수
	UMassRepresentationSubsystem* RepresentationSubsystemForStats = nullptr;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		UMassRepresentationSubsystem* RepresentationSubsystem = Context.GetSharedFragment<FMassRepresentationSubsystemSharedFragment>().RepresentationSubsystem;
		check(RepresentationSubsystem);
		RepresentationSubsystemForStats = RepresentationSubsystem;
		FMassInstancedSkinnedMeshInfoArrayView SkinnedMeshInfos = RepresentationSubsystem->GetMutableInstancedSkinnedMeshInfos();

		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassRepresentationFragment> RepresentationList = Context.GetMutableFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> RepresentationLODList = Context.GetFragmentView<FMassRepresentationLODFragment>();
		const TConstArrayView<FLabAnimStateFragment> AnimStates = Context.GetFragmentView<FLabAnimStateFragment>();
		const TArrayView<FLabIskmTrackFragment> Tracks = Context.GetMutableFragmentView<FLabIskmTrackFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FMassRepresentationFragment& Representation = RepresentationList[EntityIt];
			const bool bWaitingForActor = Representation.IsWaitingForActorVisualReadiness() && Representation.PrevRepresentation == EMassRepresentationType::SkinnedMeshInstance;
			if (Representation.CurrentRepresentation != EMassRepresentationType::SkinnedMeshInstance && !bWaitingForActor)
			{
				continue; // 액터·Off는 건드리지 않는다(PrevTransform 포함)
			}
			const int32 InfoIndex = Representation.SkinnedMeshDescHandle.ToIndex();
			if (!ensureMsgf(SkinnedMeshInfos.IsValidIndex(InfoIndex), TEXT("Invalid handle index %d for SkinnedMeshInfos"), InfoIndex))
			{
				continue;
			}
			FMassInstancedSkinnedMeshInfo& Info = SkinnedMeshInfos[InfoIndex];
			const FMassEntityHandle Entity = Context.GetEntity(EntityIt);
			const FTransform& Transform = TransformList[EntityIt].GetTransform();
			const float LODSignificance = RepresentationLODList[EntityIt].LODSignificance;
			UMassUpdateInstancedSkinnedMeshProcessor::UpdateMeshTransform(Entity, Info, Transform, Representation.PrevTransform, LODSignificance, Representation.PrevLODSignificance);
#if !UE_BUILD_TEST
			// Q3 전환 점수: 액터 → ISKM으로 돌아온 첫 프레임. VAT 프로세서의 dehydrate와 같은 조건이라 Scripts/transition-score.py --event dehydrate가 두 표현을 같은 절차로 잰다.
			if (UE::Mass::Representation::IsValidActorRepresentation(Representation.PrevRepresentation) && Representation.CurrentRepresentation == EMassRepresentationType::SkinnedMeshInstance)
			{
				LabLogScreenEvent(TEXT("dehydrate"), Crowd->SnipeView, Transform.GetLocation(), Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType == LabUnitLarge ? 1.f : 0.f);
			}
#endif

			// 애니 데이터. 클립 번호 = 상태(0 걷기, 1 피격, 2 죽기, Scripts/create-iskm.py와 같은 순서). 걷기만 루프, 피격·죽기는 끝 자세에서 멈춘다(VAT ClipFrame과 같다).
			const FLabAnimStateFragment& Anim = AnimStates[EntityIt];
			const float T = Now - Anim.StateStartTime;
			FAnimSequenceTrackAutoPlayData Data;
			Data.SequenceIndex = int32(Anim.State);
			// Q1b: 사망 클립은 DeathDuration에 맞춰 끝나도록 배율로 재생한다(시즈 < 1). 위치는 클립 시각이라 배율을 곱한다.
			const float Rate = Anim.State == ELabMinionState::Dead ? Crowd->DeathRateIskm[Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType] : 1.f;
			Data.Position = FMath::Max(T, 0.f) * Rate;
			Data.PlayRate = Rate;
			Data.BlendTime = BlendTime;
			Data.LoopMode = Anim.State == ELabMinionState::Walk ? EAnimSequenceTrackLoopMode::Loop : EAnimSequenceTrackLoopMode::Clamp;
			Info.AddBatchedAnimationData(Entity, Data, LODSignificance, Representation.PrevLODSignificance);
			// 커스텀 데이터 0번 = 죽기 색 정도. 액터와 같은 식(LabActorSyncProcessor.cpp)이고 ISKM 머티리얼이 PerInstanceCustomData 0번으로 읽는다(Scripts/create-iskm.py DEAD_AMOUNT_INDEX).
			const float DeadAmount = Anim.State == ELabMinionState::Dead ? FMath::Clamp(T / DeathDuration * 2.5f, 0.f, 1.f) : 0.f;
			Info.AddBatchedCustomData<float>(DeadAmount, LODSignificance, Representation.PrevLODSignificance);

			// 트랙: 표현 상태가 마지막으로 쓴 값과 다를 때만 쓴다(상태 전환, 풀에서 다시 나옴). 평소 프레임에는 엔진의 자동 재생에 맡긴다.
			FLabIskmTrackFragment& Track = Tracks[EntityIt];
			if (Track.AppliedState != int8(Anim.State) || Track.AppliedStartTime != Anim.StateStartTime)
			{
				if (LabWriteTrack(Info, LODSignificance, Entity, Data)) { ++TrackWrites; } else { ++TrackFirst; }
				Track.AppliedState = int8(Anim.State);
				Track.AppliedStartTime = Anim.StateStartTime;
			}
#if !UE_BUILD_TEST
			{
				// Q1b 개발 확인(Q2 3단위): 죽는 동안 사망 시간의 25·50·75·97%를 지나는 프레임에 엔진 트랙의 재생 위치를 찍는다(종류·지점마다 3번까지).
				// 고쳐졌다면 위치 ≈ 지점 × 클립 길이이고, 고치기 전이었다면 시즈는 30% 지점부터 클립 끝(0.633초)에 붙어 있었다.
				static int32 Logged[LabUnitTypeMax][4] = {};
				static const float Marks[4] = { 0.25f, 0.5f, 0.75f, 0.97f };
				const float Dt = Context.GetDeltaTimeSeconds();
				const int32 Ty = Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType;
				for (int32 M = 0; M < 4 && Anim.State == ELabMinionState::Dead; ++M)
				{
					const float Mark = Marks[M] * DeathDuration;
					float Pos = 0.f, TrackRate = 0.f;
					int32 Seq = INDEX_NONE;
					if (T >= Mark && T - Dt < Mark && Logged[Ty][M] < 3 && LabReadTrack(Info, LODSignificance, Entity, Pos, TrackRate, Seq))
					{
						++Logged[Ty][M];
						const UAnimSequenceTransformProviderData* ProviderData = Crowd->IskmProviders.IsValidIndex(Ty) ? Cast<UAnimSequenceTransformProviderData>(Crowd->IskmProviders[Ty]) : nullptr;
						UE_LOG(LogLab, Log, TEXT("lab_death_check path=iskm type=%d mark=%.2f t=%.3f track_pos=%.3f track_rate=%.4f track_seq=%d clip_len=%.3f expected_pos=%.3f"),
							Ty, Marks[M], T, Pos, TrackRate, Seq, ProviderData ? ProviderData->GetSequencePlayLength(2) : -1.f, T * Rate);
					}
				}
			}
#endif

			Representation.PrevTransform = Transform;
			Representation.PrevLODSignificance = LODSignificance;

			// 표현 축(measurement.md): 거리 밴드별로 그린 개체 수. VAT 프로세서와 같은 스탯 이름(Lab/IsmBand*)이라 요약 스크립트가 그대로 읽는다
			const float Dist = FVector::Dist2D(Transform.GetLocation(), ViewLocation);
			++BandCounts[Dist < 3000.f ? 0 : Dist < 8000.f ? 1 : Dist < 20000.f ? 2 : 3];
			const float DistSq = FVector::DistSquared(Transform.GetLocation(), ViewLocation);
			if (DistSq < NearMaxSq && QualityView.bValid && QualityView.Contains(Transform.GetLocation() + FVector(0.f, 0.f, 90.f)))
			{
				if (DistSq < Near30Sq) { ++ViewInst30; }
				if (DistSq < NearHydSq) { ++ViewInstHyd; }
			}
#if !UE_BUILD_TEST
			// E3b (8) 화질 캡처(-LabFrameLog)의 near_vat를 ISKM에서도 센다(Q4, VAT 프로세서와 같은 정의: 화면 안 수평 30m에서 아직 인스턴스). M5 단계는 VAT 프로세서가 돌지 않아 0으로 찍혔다
			if (NearInstCounter && Dist < 3000.f && QualityView.Contains(Transform.GetLocation() + FVector(0.f, 0.f, 90.f))) { ++*NearInstCounter; }
#endif
		}
	});

	CSV_CUSTOM_STAT(Lab, ViewInst30, ViewInst30, ECsvCustomStatOp::Set);   // Q4 (나) 대체 지표의 나머지: 화면 안 30m에서 아직 인스턴스
	CSV_CUSTOM_STAT(Lab, ViewInstHyd, ViewInstHyd, ECsvCustomStatOp::Set); // Q4: 화면 안 전환 거리 안에서 아직 인스턴스(상한이 막는 정도)
	CSV_CUSTOM_STAT(Lab, IsmBand0, BandCounts[0], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand1, BandCounts[1], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand2, BandCounts[2], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand3, BandCounts[3], ECsvCustomStatOp::Set);

	// plan.md "M5" 결정 8: 트랙 맵 크기와 ISKM 인스턴스 수. 엔진 EndVisualChanges가 이 프로세서 뒤에 돌므로 지난 프레임까지 반영된 값이다.
	if (!RepresentationSubsystemForStats)
	{
		RepresentationSubsystemForStats = World->GetSubsystem<UMassRepresentationSubsystem>();
	}
	int32 IskmTracks = 0;
	int32 IskmInstances = 0;
	if (RepresentationSubsystemForStats)
	{
		LabCountIskm(*RepresentationSubsystemForStats, IskmTracks, IskmInstances);
	}
	CSV_CUSTOM_STAT(Lab, IskmTracks, IskmTracks, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IskmInstances, IskmInstances, ECsvCustomStatOp::Set);

#if !UE_BUILD_TEST
	// 개발 확인: 1초마다 트랙 수, 인스턴스 수, 직접 쓴 트랙 수, 엔진이 새로 만든 트랙 수, 풀 상태(lab_iskm_stats).
	static double Since = 0.;
	static int64 WritesSum = 0, FirstSum = 0;
	WritesSum += TrackWrites;
	FirstSum += TrackFirst;
	if (Now - Since >= 1.)
	{
		UE_LOG(LogLab, Log, TEXT("lab_iskm_stats t=%.1f tracks=%d instances=%d track_writes=%lld track_first=%lld pooled=%d reused=%d created=%d b1=%d b2=%d"),
			Now, IskmTracks, IskmInstances, WritesSum, FirstSum, Crowd->GetPooledCount(), Crowd->PoolReused, Crowd->PoolCreated, BandCounts[1], BandCounts[2]);
		Since = Now; WritesSum = 0; FirstSum = 0;
	}
#endif
}
