#include "LabVatProcessor.h"
#include "LabCrowdSubsystem.h"
#include "LabMassTypes.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassLODFragments.h"
#include "MassRepresentationFragments.h"
#include "MassRepresentationSubsystem.h"
#include "MassVisualizationComponent.h"
#include "MassRepresentationProcessor.h"
#include "HordeOptLab.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

CSV_DEFINE_CATEGORY(Lab, true);

namespace
{
	/** 클립 안의 프레임. 루프면 감고, 한 번 도는 클립이면 끝 프레임에 멈춘다. */
	float ClipFrame(const FLabVatClip& Clip, float Seconds, float SampleRate, bool bLoop)
	{
		const float Length = float(Clip.EndFrame - Clip.StartFrame + 1);
		float Local = Seconds * SampleRate;
		Local = bLoop ? FMath::Fmod(FMath::Max(Local, 0.f), Length) : FMath::Clamp(Local, 0.f, Length - 1.f);
		return float(Clip.StartFrame) + Local;
	}
}

ULabVatISMProcessor::ULabVatISMProcessor()
{
	// 기본은 등록하지 않는다. 게임 모듈이 단계를 보고 켠다(HordeOptLab.cpp).
	bAutoRegisterWithProcessingPhases = false;
}

void ULabVatISMProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::ConfigureQueries(EntityManager);
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadOnly);
}

void ULabVatISMProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	CSV_SCOPED_TIMING_STAT(Lab, VatIsm); // 병목 분석용(2026-11-09)
	UWorld* World = Context.GetWorld();
	const ULabCrowdSubsystem* Crowd = World ? World->GetSubsystem<ULabCrowdSubsystem>() : nullptr;
	if (!Crowd || !Crowd->IsActive())
	{
		return;
	}
	const FLabVatClips& Clips = Crowd->VatClips;
	const float Now = World->GetTimeSeconds();
	const float HitDuration = Crowd->HitDuration;
	const float DeathDuration = Crowd->DeathDuration;
	const FVector ViewLocation = Crowd->SnipeView.Location;
	int32 BandCounts[4] = { 0, 0, 0, 0 };
	// 진단(-LabVatTiming): 처리 안쪽을 나눠 잰다. 액터가 있을 때만 이 프로세서가 2.3ms 느려지는 원인 찾기(2026-11-09)
	static const bool bTiming = FParse::Param(FCommandLine::Get(), TEXT("LabVatTiming"));
	uint64 CyXf = 0, CyCustom = 0, CyChunkSetup = 0;
	int32 Chunks = 0, Entities = 0, IsmEntities = 0;

	const uint64 F0 = bTiming ? FPlatformTime::Cycles64() : 0;
	uint64 CyFirstChunk = 0;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const uint64 C0 = bTiming ? FPlatformTime::Cycles64() : 0;
		if (bTiming && Chunks == 0) { CyFirstChunk = C0 - F0; } // 첫 청크 람다까지 걸린 시간(질의 준비)
		++Chunks;
		Entities += Context.GetNumEntities();
		UMassRepresentationSubsystem* RepresentationSubsystem = Context.GetSharedFragment<FMassRepresentationSubsystemSharedFragment>().RepresentationSubsystem;
		check(RepresentationSubsystem);
		FMassInstancedStaticMeshInfoArrayView ISMInfo = RepresentationSubsystem->GetMutableInstancedStaticMeshInfos();

		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassRepresentationFragment> RepresentationList = Context.GetMutableFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> RepresentationLODList = Context.GetFragmentView<FMassRepresentationLODFragment>();
		const TConstArrayView<FLabMinionFragment> Minions = Context.GetFragmentView<FLabMinionFragment>();
		const bool bHit = Context.DoesArchetypeHaveTag(*FLabHitTag::StaticStruct());
		const bool bDead = Context.DoesArchetypeHaveTag(*FLabDeadTag::StaticStruct());
		const float DeltaSeconds = Context.GetDeltaTimeSeconds();
		if (bTiming) { CyChunkSetup += FPlatformTime::Cycles64() - C0; }

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FTransformFragment& TransformFragment = TransformList[EntityIt];
			const FMassRepresentationLODFragment& RepresentationLOD = RepresentationLODList[EntityIt];
			FMassRepresentationFragment& Representation = RepresentationList[EntityIt];

			// 액터 준비를 기다리는 동안(bWaitForActorVisualReadiness) 엔진은 ISM을 떼지 않는다. 그동안도 위치와 프레임을 계속 써서 VAT 인스턴스가 멈추지 않게 한다.
			const bool bWaitingForActor = Representation.IsWaitingForActorVisualReadiness() && Representation.PrevRepresentation == EMassRepresentationType::StaticMeshInstance;
			if (Representation.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance || bWaitingForActor)
			{
				const int32 ISMInfoIndex = Representation.StaticMeshDescHandle.ToIndex();
				if (ensureMsgf(ISMInfo.IsValidIndex(ISMInfoIndex), TEXT("Invalid handle index %u for ISMInfosView"), ISMInfoIndex))
				{
					++IsmEntities;
					const uint64 X0 = bTiming ? FPlatformTime::Cycles64() : 0;
					UpdateISMTransform(Context.GetEntity(EntityIt), ISMInfo[ISMInfoIndex], TransformFragment.GetTransform(), Representation.PrevTransform,
						RepresentationLOD.LODSignificance, Representation.PrevLODSignificance);
					if (bTiming) { CyXf += FPlatformTime::Cycles64() - X0; }

					const FLabMinionFragment& Minion = Minions[EntityIt];
					FLabVatFrameData Data;
					if (bDead)
					{
						const float T = DeathDuration - Minion.StateTimeLeft;
						Data.Frame = ClipFrame(Clips.Death, T, Clips.SampleRate, false);
						Data.PrevFrame = ClipFrame(Clips.Death, T - DeltaSeconds, Clips.SampleRate, false);
					}
					else if (bHit)
					{
						const float T = HitDuration - Minion.StateTimeLeft;
						Data.Frame = ClipFrame(Clips.Hit, T, Clips.SampleRate, false);
						Data.PrevFrame = ClipFrame(Clips.Hit, T - DeltaSeconds, Clips.SampleRate, false);
					}
					else
					{
						// 걷기 위상을 일련번호로 흩는다. 모두 같은 프레임이면 군중이 한 몸처럼 발을 맞춘다.
						const float Phase = float(Minion.Serial % 997u) * 0.1031f;
						Data.Frame = ClipFrame(Clips.Walk, Now + Phase, Clips.SampleRate, true);
						Data.PrevFrame = ClipFrame(Clips.Walk, Now + Phase - DeltaSeconds, Clips.SampleRate, true);
					}
					const uint64 D0 = bTiming ? FPlatformTime::Cycles64() : 0;
					ISMInfo[ISMInfoIndex].AddBatchedCustomData<FLabVatFrameData>(Data, RepresentationLOD.LODSignificance, Representation.PrevLODSignificance);
					if (bTiming) { CyCustom += FPlatformTime::Cycles64() - D0; }

#if !UE_BUILD_TEST
					if (UE::Mass::Representation::IsValidActorRepresentation(Representation.PrevRepresentation) && Representation.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance)
					{
						LabLogScreenEvent(TEXT("dehydrate"), Crowd->SnipeView, TransformFragment.GetTransform().GetLocation());
					}
#endif
					// 표현 축(measurement.md): 거리 밴드별로 그린 개체 수. 밴드 경계는 확정값 30/80/200m
					const float Dist = FVector::Dist2D(TransformFragment.GetTransform().GetLocation(), ViewLocation);
					++BandCounts[Dist < 3000.f ? 0 : Dist < 8000.f ? 1 : Dist < 20000.f ? 2 : 3];
				}
			}
			Representation.PrevTransform = TransformFragment.GetTransform();
			Representation.PrevLODSignificance = RepresentationLOD.LODSignificance;
		}
	});

	if (bTiming)
	{
		CSV_CUSTOM_STAT(Lab, VatForEachMs, float(FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - F0)), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatFirstChunkMs, float(FPlatformTime::ToMilliseconds64(CyFirstChunk)), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatXfMs, float(FPlatformTime::ToMilliseconds64(CyXf)), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatCustomMs, float(FPlatformTime::ToMilliseconds64(CyCustom)), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatChunkSetupMs, float(FPlatformTime::ToMilliseconds64(CyChunkSetup)), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatChunks, Chunks, ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatEntities, Entities, ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatIsmEntities, IsmEntities, ECsvCustomStatOp::Set);
	}
	CSV_CUSTOM_STAT(Lab, IsmBand0, BandCounts[0], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand1, BandCounts[1], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand2, BandCounts[2], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand3, BandCounts[3], ECsvCustomStatOp::Set);
}
