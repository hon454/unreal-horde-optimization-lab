#include "LabIskmProcessor.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabMassTypes.h"
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
	int32 TrackWrites = 0;   // 이 프로세서가 기존 트랙에 직접 쓴 수(상태 전환, 풀에서 다시 나옴)
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

			Representation.PrevTransform = Transform;
			Representation.PrevLODSignificance = LODSignificance;

			// 표현 축(measurement.md): 거리 밴드별로 그린 개체 수. VAT 프로세서와 같은 스탯 이름(Lab/IsmBand*)이라 요약 스크립트가 그대로 읽는다
			const float Dist = FVector::Dist2D(Transform.GetLocation(), ViewLocation);
			++BandCounts[Dist < 3000.f ? 0 : Dist < 8000.f ? 1 : Dist < 20000.f ? 2 : 3];
		}
	});

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
