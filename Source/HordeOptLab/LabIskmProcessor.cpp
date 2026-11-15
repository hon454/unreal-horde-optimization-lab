#include "LabIskmProcessor.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabUnitAssets.h"
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
	/** M6 수정 B: 그리는 끝 밖의 피격·사망 개체(LOD Low, 중요도 Off = 3.0)의 범위인가 */
	bool LodSignificanceIsFarHit(float LODSignificance) { return LODSignificance >= float(EMassLOD::Off); }

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

	/**
	 * v2 W9 ISKM 번쩍임: 이 범위의 컴포넌트마다 처음 한 번 트랙 풀의 크기(트랙 번호의 끝)를 Capacity로 늘려 둔다. Capacity개를 할당한 뒤 모두 해제한다.
	 * 왜: 렌더러는 트랙 수(UniqueAnimationCount = 트랙 풀의 Datas.Num(), AnimSequenceTransformProviderData.cpp:1341-1344)가 바뀔 때마다 그 컴포넌트의 뼈대 변환 버퍼를
	 * 다시 할당하고(SkinningSceneExtension.cpp:1336-1352), 옛 자리의 지금·이전 슬롯을 새 자리로 복사한다(:1857-1866, CopyPreviousTransforms.usf).
	 * 그런데 옛 자리를 먼저 Free하고 새 자리를 Allocate해서(:1346-1351) FSpanAllocator가 해제 대기 구간을 합쳐(SpanAllocator.h:27-30) 옛 자리와 겹치는 앞쪽 자리를 줄 수 있다.
	 * 겹친 채로 같은 UAV를 병렬 복사하면(셰이더 주석은 "allocate-before-free guarantees no overlap"이라 가정한다) 이전 슬롯이 깨져 그 프레임에 컴포넌트의 모든 인스턴스가 틀린 속도를 받는다.
	 * 트랙 번호의 끝을 미리 늘려 두면 해제된 번호를 다시 쓰므로(AnimTrackPool.h AllocateTrack은 가장 낮은 빈 번호) 트랙 수가 바뀌지 않아 재할당이 일어나지 않는다.
	 * 비용: 버퍼는 Capacity × 뼈 수 × 2슬롯을 차지한다. GPU 평가는 활성 트랙만 한다(AnimSequenceTransformProvider.cpp:1456).
	 */
	void LabReserveTracks(FMassInstancedSkinnedMeshInfo& Info, float LODSignificance, int32 Capacity, TArray<TWeakObjectPtr<UAnimSequenceTransformProviderDataInstance>>& Reserved)
	{
		FMassLODInstancedSkinnedMeshSignificanceRange* Range = Info.GetLODSignificanceRange(LODSignificance);
		if (!Range || !Range->InstancedSkinnedMeshSharedDataPtr)
		{
			return;
		}
		for (const FInstancedSkinnedMeshComponentSharedDataKey& Key : Range->SkinnedMeshComponentRefs)
		{
			FMassInstancedSkinnedMeshComponentSharedData* Shared = Range->InstancedSkinnedMeshSharedDataPtr->Find(Key);
			UInstancedSkinnedMeshComponent* Component = Shared ? Shared->GetMutableInstancedSkinnedMeshComponent() : nullptr;
			UAnimSequenceTransformProviderDataInstance* Provider = Component ? Cast<UAnimSequenceTransformProviderDataInstance>(Component->GetTransformProvider()) : nullptr;
			if (!Provider || Reserved.Contains(Provider))
			{
				continue;
			}
			Reserved.Add(Provider);
			const int32 Before = int32(Provider->GetUniqueAnimationCount());
			TArray<int32> Temp;
			while (int32(Provider->GetUniqueAnimationCount()) < Capacity)
			{
				Temp.Add(Provider->AllocateTrack());
			}
			for (const int32 TrackIndex : Temp)
			{
				Provider->DeallocateTrack(TrackIndex);
			}
			UE_LOG(LogLab, Log, TEXT("lab_iskm_reserve component=%s tracks_before=%d capacity=%d tracks_after=%d"), *Component->GetName(), Before, Capacity, int32(Provider->GetUniqueAnimationCount()));
		}
	}

#if !UE_BUILD_TEST
	/** v2 W9 개발 진단: ISKM 컴포넌트마다 "이름:트랙 수(UniqueAnimationCount)/인스턴스 수"를 잇는다. 트랙 수가 바뀐 프레임에 렌더러가 뼈대 버퍼를 다시 할당한다. */
	FString LabTrackCountString(UMassRepresentationSubsystem& RepresentationSubsystem)
	{
		FString Out;
		TArray<const UInstancedSkinnedMeshComponent*, TInlineAllocator<8>> Seen;
		FMassInstancedSkinnedMeshInfoArrayView Infos = RepresentationSubsystem.GetMutableInstancedSkinnedMeshInfos();
		for (int32 InfoIndex = 0; InfoIndex < Infos.Num(); ++InfoIndex)
		{
			FMassInstancedSkinnedMeshInfo& Info = Infos[InfoIndex];
			if (!Info.IsValid())
			{
				continue;
			}
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
					const UInstancedSkinnedMeshComponent* Component = Shared ? Shared->GetInstancedSkinnedMeshComponent() : nullptr;
					if (!Component || Seen.Contains(Component))
					{
						continue;
					}
					Seen.Add(Component);
					const UTransformProviderData* Provider = Component->GetTransformProvider();
					Out += FString::Printf(TEXT(" %s=%u/%d"), *Component->GetName(), Provider ? Provider->GetUniqueAnimationCount() : 0u, Component->GetInstanceCount());
				}
			}
		}
		return Out;
	}

	/** v2 W9 개발 진단: 엔티티의 지금 트랙(쓰기 전)이 무슨 클립을 재생하는지 읽는다. 트랙이 없으면 -1. */
	int32 LabPeekTrackSeq(FMassInstancedSkinnedMeshInfo& Info, float LODSignificance, FMassEntityHandle Entity)
	{
		FMassLODInstancedSkinnedMeshSignificanceRange* Range = Info.GetLODSignificanceRange(LODSignificance);
		if (!Range || !Range->InstancedSkinnedMeshSharedDataPtr)
		{
			return -1;
		}
		for (const FInstancedSkinnedMeshComponentSharedDataKey& Key : Range->SkinnedMeshComponentRefs)
		{
			FMassInstancedSkinnedMeshComponentSharedData* Shared = Range->InstancedSkinnedMeshSharedDataPtr->Find(Key);
			UInstancedSkinnedMeshComponent* Component = Shared ? Shared->GetMutableInstancedSkinnedMeshComponent() : nullptr;
			const int32* TrackId = Shared ? Shared->GetEntityToTrackMap().Find(Entity) : nullptr;
			const UAnimSequenceTransformProviderDataInstance* Provider = Component ? Cast<UAnimSequenceTransformProviderDataInstance>(Component->GetTransformProvider()) : nullptr;
			if (TrackId && Provider)
			{
				return Provider->GetSequenceIndex(*TrackId, 0);
			}
		}
		return -1;
	}
#endif

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
			// 범위 배열은 보호 멤버라 LOD 값마다 범위를 찾는다(Lab 설명자는 High~Max 한 범위, Q4b 원거리 그림자 끔이면 High~Low·Low~Max 두 범위다).
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
	ProcessorRequirements.AddSubsystemRequirement<UMassRepresentationSubsystem>(EMassFragmentAccess::ReadWrite); // 통계용(보이는 청크가 없는 프레임)
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadWrite); // HUD 집계와 개발 진단을 쓴다(원자 변수)
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
	ULabCrowdSubsystem* Crowd = LabCrowdWrite(Context);
	if (!Crowd)
	{
		return;
	}
	const float Now = World->GetTimeSeconds();
	const float DeathDuration = Crowd->DeathDuration;
	const float BlendTime = FLabIskmParams::BlendTime;
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
	static const bool bRangeLog = FParse::Param(FCommandLine::Get(), TEXT("LabRangeLog")); // Q4b 범위 경계 확인(lab_range_check)
	static int32 RangeLogged = 0;
	static TArray<FMassEntityHandle> RangePending; // 게임 스레드 전용 프로세서라 정적 배열로 둔다
#endif
#if !UE_BUILD_TEST
	// v2 W8 개발 진단(-LabOrbDiag): 바운드 상자(ULabCrowdSubsystem::IskmBoundsCenter ± IskmBoundsHalfXY) 밖의 ISKM 인스턴스 수(전체, 화면 안)
	static const bool bOrbDiag = FParse::Param(FCommandLine::Get(), TEXT("LabOrbDiag"));
	const bool bBoxDiag = bOrbDiag && Crowd->bIskmBoundsCenterSet;
	const float BoxHalf = ULabCrowdSubsystem::IskmBoundsHalfXY();
	const FVector BoxCenter = Crowd->IskmBoundsCenter;
	int32 OutBox = 0, OutBoxView = 0;
	double MaxCamFromBox = 0.;
#endif
	int32 TrackWrites = 0;  // 이 프로세서가 기존 트랙에 직접 쓴 수(상태 전환, 풀에서 다시 나옴)
	int32 TrackFirst = 0;    // 트랙이 없어 엔진이 이 프레임에 만들 수
	const uint64 Frame = GFrameCounter; // v2 W9: 컴포넌트에 (다시) 들어오는 프레임 판정(FLabIskmTrackFragment::LastFrame)
	// v2 W9: 종류별 트랙 풀 크기(FLabConfig::bIskmTrackReserve). 엔티티는 컴포넌트마다 트랙 하나를 갖으므로 종류의 개체 수에 여유(10% + 64)를 더한다. 0이면 늘리지 않는다
	int32 TrackReserve[2] = { 0, 0 };
	if (FLabConfig::Get().bIskmTrackReserve)
	{
		const int32 N = FLabConfig::Get().N;
		const int32 NumLarge = FLabUnitTypes::Num() == 2 ? (N + 9) / 10 : 0; // 일련번호 % 10 == 0이 대형(LabUnitAssets.h FLabUnitTypes::IsLarge)
		TrackReserve[0] = FMath::CeilToInt32((N - NumLarge) * 1.1f) + 64;
		TrackReserve[1] = FMath::CeilToInt32(NumLarge * 1.1f) + 64;
	}
#if !UE_BUILD_TEST
	// v2 W9 개발 진단(-LabFlashDiag): 프레임마다 컴포넌트에 들어온 개체를 옛 트랙의 클립별로 세고(lab_flash), 화면 좌표를 남긴다(lab_event iskm_enter, iskm_state)
	static const bool bFlashDiag = FParse::Param(FCommandLine::Get(), TEXT("LabFlashDiag"));
	int32 EnterBySeq[4] = { 0, 0, 0, 0 }; // [0] 트랙 없음, [1 + s] 옛 트랙이 클립 s(0 걷기, 1 피격, 2 죽기)
	int32 EnterRange = 0, StateWrites = 0;
#endif
	UMassRepresentationSubsystem* RepresentationSubsystemForStats = Context.GetMutableSubsystem<UMassRepresentationSubsystem>(); // 쿼리 밖에서 얻는다

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
#if !UE_BUILD_TEST
		const int32 ChunkType = Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType;
		const UAnimSequenceTransformProviderData* WalkProvider = bRangeLog && Crowd->IskmProviders.IsValidIndex(ChunkType) ? Cast<UAnimSequenceTransformProviderData>(Crowd->IskmProviders[ChunkType]) : nullptr;
		const float WalkLen = WalkProvider ? FMath::Max(WalkProvider->GetSequencePlayLength(0), KINDA_SMALL_NUMBER) : 1.f; // 클립 0 = 걷기
#endif

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
#if !UE_BUILD_TEST
			{
				// M6 조사 개발 진단(-LabIskmForceSeq=<클립>): 모든 ISKM 인스턴스가 이 클립을 루프로 재생한다. 거리에 따라 사라지는지 한 화면에서 본다
				static int32 ForceSeq = INDEX_NONE;
				static const bool bForceParsed = FParse::Value(FCommandLine::Get(), TEXT("-LabIskmForceSeq="), ForceSeq);
				if (bForceParsed && ForceSeq >= 0)
				{
					Data.SequenceIndex = ForceSeq;
					Data.PlayRate = 1.f;
					Data.Position = FMath::Max(T, 0.f);
					Data.LoopMode = EAnimSequenceTrackLoopMode::Loop;
				}
			}
#endif
			Info.AddBatchedAnimationData(Entity, Data, LODSignificance, Representation.PrevLODSignificance);
			// 커스텀 데이터 0번 = 죽기 색 정도. 액터와 같은 식(LabActorSyncProcessor.cpp)이고 ISKM 머티리얼이 PerInstanceCustomData 0번으로 읽는다(Scripts/create-iskm.py DEAD_AMOUNT_INDEX).
			const float DeadAmount = Anim.State == ELabMinionState::Dead ? FMath::Clamp(T / DeathDuration * 2.5f, 0.f, 1.f) : 0.f;
			Info.AddBatchedCustomData<float>(DeadAmount, LODSignificance, Representation.PrevLODSignificance);

			// 트랙: 표현 상태가 마지막으로 쓴 값과 다를 때만 쓴다(상태 전환, 풀에서 다시 나옴). 평소 프레임에는 엔진의 자동 재생에 맡긴다.
			// Q4b: 원거리 그림자 끔이면 종류마다 컴포넌트가 둘(LOD 중요도 [High, Low)·[Low, Max))이고 트랙 제공자도 컴포넌트마다 따로다.
			// 범위를 넘은 프레임에는 새 컴포넌트의 트랙도 다시 쓴다. 엔진은 트랙이 이미 있으면 클립 번호가 다를 때만 쓰므로(MassVisualizationComponent.cpp:763-770, 854-861)
			// 예전에 그 컴포넌트에 있을 때의 트랙(같은 걷기, 옛 시작 시각)이 남아 위상이 튄다. 트랙이 아직 없으면 엔진이 이번 프레임 애니 데이터로 만든다.
			const bool bRangeChanged = Info.GetLODSignificanceRangesNum() > 1 && Representation.PrevLODSignificance >= 0.f
				&& Info.GetLODSignificanceRange(Representation.PrevLODSignificance) != Info.GetLODSignificanceRange(LODSignificance);
			FLabIskmTrackFragment& Track = Tracks[EntityIt];
			// v2 W9: 지난 프레임에 이 범위의 컴포넌트에 그리지 않았으면 이번 프레임에 (다시) 들어온다(Off·액터에서, 풀에서 다시 나옴, 범위 경계)
			const bool bEntering = Track.LastFrame + 1 != Frame || bRangeChanged;
			Track.LastFrame = Frame;
			if (bEntering && TrackReserve[0] > 0)
			{
				// v2 W9: 컴포넌트가 처음 쓰이는 프레임에(엔진이 이 프레임 끝 EndVisualChanges에서 트랙을 만들기 전에) 트랙 풀을 늘려 둔다. 이미 늘린 컴포넌트는 건너뛴다
				// M6 수정 B: [Off, Max) 범위(그리는 끝 밖의 피격·사망)는 따로 된 컴포넌트이고(ULabCrowdSubsystem::BuildEntityConfig) 한 번에 몇 개뿐이라 64로 둔다(2초마다 저격 2발, 제거까지 약 2초)
				const int32 Capacity = LodSignificanceIsFarHit(LODSignificance) ? 64 : TrackReserve[Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType == LabUnitLarge ? 1 : 0];
				LabReserveTracks(Info, LODSignificance, Capacity, ReservedProviders);
			}
			const bool bStateChanged = Track.AppliedState != int8(Anim.State) || Track.AppliedStartTime != Anim.StateStartTime;
#if !UE_BUILD_TEST
			if (bFlashDiag && (bEntering || bStateChanged))
			{
				const int32 OldSeq = LabPeekTrackSeq(Info, LODSignificance, Entity);
				if (bEntering)
				{
					++EnterBySeq[FMath::Clamp(OldSeq + 1, 0, 3)];
					EnterRange += bRangeChanged ? 1 : 0;
					// x = 옛 트랙의 클립(-1 없음) + 10 × 범위 경계. 화면 좌표는 1280x720 기준
					LabLogScreenEvent(TEXT("iskm_enter"), Crowd->SnipeView, Transform.GetLocation(), float(OldSeq) + (bRangeChanged ? 10.f : 0.f));
				}
				else
				{
					++StateWrites;
					LabLogScreenEvent(TEXT("iskm_state"), Crowd->SnipeView, Transform.GetLocation(), float(FMath::Max(OldSeq, 0) * 10 + int32(Anim.State)));
				}
			}
#endif
			if (bRangeChanged || bStateChanged)
			{
#if !UE_BUILD_TEST
				// Q4b 개발 확인(-LabRangeLog): 걷는 개체가 범위를 넘는 프레임에 옛 컴포넌트 트랙의 재생 위치를 읽고, 다음 프레임에 새 컴포넌트 트랙의 위치를 읽어 기대 위치(걷기 루프 시각)와 함께 남긴다
				if (bRangeLog && bRangeChanged && Anim.State == ELabMinionState::Walk && RangeLogged < 40)
				{
					float Pos = -1.f, TrackRate = 0.f;
					int32 Seq = INDEX_NONE;
					LabReadTrack(Info, Representation.PrevLODSignificance, Entity, Pos, TrackRate, Seq);
					UE_LOG(LogLab, Log, TEXT("lab_range_check phase=switch from=%.2f to=%.2f t=%.3f expected=%.3f old_track_pos=%.3f clip_len=%.3f"),
						Representation.PrevLODSignificance, LODSignificance, T, FMath::Fmod(FMath::Max(T, 0.f), WalkLen), Pos, WalkLen);
					RangePending.Add(Entity);
					++RangeLogged;
				}
#endif
				if (LabWriteTrack(Info, LODSignificance, Entity, Data)) { ++TrackWrites; } else { ++TrackFirst; }
				Track.AppliedState = int8(Anim.State);
				Track.AppliedStartTime = Anim.StateStartTime;
			}
#if !UE_BUILD_TEST
			else if (bRangeLog && RangePending.Num() > 0 && RangePending.Contains(Entity))
			{
				RangePending.Remove(Entity);
				float Pos = -1.f, TrackRate = 0.f;
				int32 Seq = INDEX_NONE;
				LabReadTrack(Info, LODSignificance, Entity, Pos, TrackRate, Seq);
				UE_LOG(LogLab, Log, TEXT("lab_range_check phase=after sig=%.2f t=%.3f expected=%.3f new_track_pos=%.3f seq=%d"),
					LODSignificance, T, FMath::Fmod(FMath::Max(T, 0.f), WalkLen), Pos, Seq);
			}
#endif
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

#if !UE_BUILD_TEST
			{
				static const bool bTmpTrace = FParse::Param(FCommandLine::Get(), TEXT("LabFarHitDiag"));
				if (bTmpTrace && Anim.State != ELabMinionState::Walk && LODSignificance >= 2.f && T < 0.3f)
				{
					float Pos = -1.f, TrackRate = 0.f;
					int32 Seq = INDEX_NONE;
					const bool bTrack = LabReadTrack(Info, LODSignificance, Entity, Pos, TrackRate, Seq);
					UE_LOG(LogLab, Log, TEXT("lab_iskm_trace frame=%llu ent=%s sig=%.2f prev_sig=%.2f state=%d t=%.3f track=%d seq=%d pos=%.3f entering=%d range_changed=%d state_changed=%d loc=%s"),
						(unsigned long long)Frame, *Entity.DebugGetDescription(), LODSignificance, Representation.PrevLODSignificance, int32(Anim.State), T, bTrack ? 1 : 0, Seq, Pos,
						bEntering ? 1 : 0, bRangeChanged ? 1 : 0, bStateChanged ? 1 : 0, *Transform.GetLocation().ToCompactString());
				}
			}
#endif
			Representation.PrevTransform = Transform;
			Representation.PrevLODSignificance = LODSignificance;
#if !UE_BUILD_TEST
			if (bBoxDiag)
			{
				const FVector P = Transform.GetLocation();
				if (FMath::Abs(P.X - BoxCenter.X) > BoxHalf || FMath::Abs(P.Y - BoxCenter.Y) > BoxHalf)
				{
					++OutBox;
					if (QualityView.bValid && QualityView.Contains(P + FVector(0.f, 0.f, 90.f))) { ++OutBoxView; }
				}
			}
#endif

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

#if !UE_BUILD_TEST
	if (bFlashDiag)
	{
		// enter_none/walk/hit/dead: 컴포넌트에 들어온 개체를 옛 트랙의 클립으로 나눈 수, range: 그중 범위 경계, state: 컴포넌트 안의 상태 전환 쓰기
		// 뒤의 이름=a/b: 컴포넌트마다 트랙 수(지난 프레임까지, 엔진 EndVisualChanges는 이 프로세서 뒤에 돈다)/인스턴스 수. 트랙 수가 바뀐 프레임에 렌더러가 뼈대 버퍼를 다시 할당한다
		UE_LOG(LogLab, Log, TEXT("lab_flash frame=%llu t=%.3f enter_none=%d enter_walk=%d enter_hit=%d enter_dead=%d range=%d state=%d%s"),
			(unsigned long long)Frame, Now, EnterBySeq[0], EnterBySeq[1], EnterBySeq[2], EnterBySeq[3], EnterRange, StateWrites,
			RepresentationSubsystemForStats ? *LabTrackCountString(*RepresentationSubsystemForStats) : TEXT(""));
	}
#endif

	CSV_CUSTOM_STAT(Lab, ViewInst30, ViewInst30, ECsvCustomStatOp::Set);   // Q4 (나) 대체 지표의 나머지: 화면 안 30m에서 아직 인스턴스
	CSV_CUSTOM_STAT(Lab, ViewInstHyd, ViewInstHyd, ECsvCustomStatOp::Set); // Q4: 화면 안 전환 거리 안에서 아직 인스턴스(상한이 막는 정도)
	CSV_CUSTOM_STAT(Lab, IsmBand0, BandCounts[0], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand1, BandCounts[1], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand2, BandCounts[2], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand3, BandCounts[3], ECsvCustomStatOp::Set);
	if (Crowd->bHudCounts.load(std::memory_order_relaxed)) // 독자용 HUD가 보일 때만(Docs/Worklog/Q5-hud.md)
	{
		Crowd->HudInstances.store(BandCounts[0] + BandCounts[1] + BandCounts[2] + BandCounts[3], std::memory_order_relaxed);
	}

	// plan.md "M5" 결정 8: 트랙 맵 크기와 ISKM 인스턴스 수. 엔진 EndVisualChanges가 이 프로세서 뒤에 돌므로 지난 프레임까지 반영된 값이다.
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
	if (bBoxDiag)
	{
		static double BoxSince = 0.;
		static int32 BoxFrames = 0, BoxOutMax = 0, BoxOutViewMax = 0, BoxMovesAtStart = 0;
		static int64 BoxOutSum = 0, BoxOutViewSum = 0;
		static double BoxCamMax = 0.;
		if (QualityView.bValid)
		{
			MaxCamFromBox = FMath::Max(FMath::Abs(QualityView.Location.X - BoxCenter.X), FMath::Abs(QualityView.Location.Y - BoxCenter.Y));
		}
		++BoxFrames; BoxOutSum += OutBox; BoxOutViewSum += OutBoxView;
		BoxOutMax = FMath::Max(BoxOutMax, OutBox); BoxOutViewMax = FMath::Max(BoxOutViewMax, OutBoxView);
		BoxCamMax = FMath::Max(BoxCamMax, MaxCamFromBox);
		if (BoxSince <= 0.) { BoxSince = Now; BoxMovesAtStart = Crowd->IskmBoundsMoves; }
		else if (Now - BoxSince >= 5.)
		{
			// out: 상자 밖 ISKM 인스턴스(프레임 평균, 최대), out_view: 그중 화면 안, cam_off: 카메라가 상자 중심에서 벗어난 최대 축 거리(cm), moves: 이 5초에 상자를 옮긴 횟수
			UE_LOG(LogLab, Log, TEXT("lab_iskmbox t=%.1f step_m=%g half=%.0f center=%s out_mean=%.1f out_max=%d out_view_mean=%.1f out_view_max=%d cam_off_max=%.0f moves=%d"),
				Now, FLabConfig::Get().BoundsStepM, BoxHalf, *BoxCenter.ToCompactString(), double(BoxOutSum) / BoxFrames, BoxOutMax, double(BoxOutViewSum) / BoxFrames, BoxOutViewMax,
				BoxCamMax, Crowd->IskmBoundsMoves - BoxMovesAtStart);
			BoxSince = Now; BoxFrames = 0; BoxOutSum = BoxOutViewSum = 0; BoxOutMax = BoxOutViewMax = 0; BoxCamMax = 0.; BoxMovesAtStart = Crowd->IskmBoundsMoves;
		}
	}
	if (Now - Since >= 1.)
	{
		UE_LOG(LogLab, Log, TEXT("lab_iskm_stats t=%.1f tracks=%d instances=%d track_writes=%lld track_first=%lld pooled=%d reused=%d created=%d b1=%d b2=%d"),
			Now, IskmTracks, IskmInstances, WritesSum, FirstSum, Crowd->GetPooledCount(), Crowd->PoolReused, Crowd->PoolCreated, BandCounts[1], BandCounts[2]);
		Since = Now; WritesSum = 0; FirstSum = 0;
	}
#endif
}
