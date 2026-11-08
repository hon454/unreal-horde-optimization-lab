#include "LabVatProcessor.h"
#include "LabCrowdSubsystem.h"
#include "LabMassTypes.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassLODFragments.h"
#include "MassRepresentationFragments.h"
#include "MassRepresentationSubsystem.h"
#include "MassVisualizationComponent.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Engine/World.h"

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

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
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

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FTransformFragment& TransformFragment = TransformList[EntityIt];
			const FMassRepresentationLODFragment& RepresentationLOD = RepresentationLODList[EntityIt];
			FMassRepresentationFragment& Representation = RepresentationList[EntityIt];

			if (Representation.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance)
			{
				const int32 ISMInfoIndex = Representation.StaticMeshDescHandle.ToIndex();
				if (ensureMsgf(ISMInfo.IsValidIndex(ISMInfoIndex), TEXT("Invalid handle index %u for ISMInfosView"), ISMInfoIndex))
				{
					UpdateISMTransform(Context.GetEntity(EntityIt), ISMInfo[ISMInfoIndex], TransformFragment.GetTransform(), Representation.PrevTransform,
						RepresentationLOD.LODSignificance, Representation.PrevLODSignificance);

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
					ISMInfo[ISMInfoIndex].AddBatchedCustomData<FLabVatFrameData>(Data, RepresentationLOD.LODSignificance, Representation.PrevLODSignificance);

					// 표현 축(measurement.md): 거리 밴드별로 그린 개체 수. 밴드 경계는 확정값 30/80/200m
					const float Dist = FVector::Dist2D(TransformFragment.GetTransform().GetLocation(), ViewLocation);
					++BandCounts[Dist < 3000.f ? 0 : Dist < 8000.f ? 1 : Dist < 20000.f ? 2 : 3];
				}
			}
			Representation.PrevTransform = TransformFragment.GetTransform();
			Representation.PrevLODSignificance = RepresentationLOD.LODSignificance;
		}
	});

	CSV_CUSTOM_STAT(Lab, IsmBand0, BandCounts[0], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand1, BandCounts[1], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand2, BandCounts[2], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand3, BandCounts[3], ECsvCustomStatOp::Set);
}
