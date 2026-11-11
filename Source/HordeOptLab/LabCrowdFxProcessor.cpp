#include "LabCrowdFxProcessor.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabMassTypes.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassLODFragments.h"           // FMassOffLODTag
#include "MassRepresentationFragments.h" // FMassRepresentationFragment
#include "MassRepresentationTypes.h"     // ProcessorGroupNames::VisualizationProcessing
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Engine/World.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

ULabCrowdFxGatherProcessor::ULabCrowdFxGatherProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = false; // -LabCrowdFx=1 + -LabCrowdFxEntity=1에서만 게임 모듈이 켠다(HordeOptLab.cpp)
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::VisualizationProcessing); // 이번 프레임의 표현(None 여부)이 정해진 뒤
	bRequiresGameThreadExecution = true; // Niagara 컴포넌트의 배열 사용자 변수를 고친다
}

void ULabCrowdFxGatherProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadOnly);
	// m4offmove·m4offpool과 M5 단계에서만 있다(LabMassTraits.cpp). 붙은 단계에서는 Off 태그와 무관하게 모든 미니언 아키타입에 있다
	EntityQuery.AddRequirement<FLabOffMoveFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	EntityQuery.AddConstSharedRequirement<FLabUnitTypeParams>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabPooledTag>(EMassFragmentPresence::None); // 풀에서 기다리는 엔티티(맵 밖 14km에 서 있다)
}

void ULabCrowdFxGatherProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	CSV_SCOPED_TIMING_STAT(Lab, CrowdFxGather); // 모으기와 Niagara에 넣기를 합친 시간
	UWorld* World = Context.GetWorld();
	ULabCrowdSubsystem* Crowd = World ? World->GetSubsystem<ULabCrowdSubsystem>() : nullptr;
	if (!Crowd || !Crowd->IsActive())
	{
		return;
	}
	UNiagaraComponent* Fx = Crowd->CrowdFx.Get();
	if (!Fx || !Fx->IsActive())
	{
		return; // 시스템을 찾지 못해 켜지 못했다(lab_crowdfx on=0)
	}
#if !UE_BUILD_TEST
	const uint64 StartCycles = FPlatformTime::Cycles64();
#endif
	static const FName SmallName(FLabCrowdFx::SmallRecsParam);
	static const FName LargeName(FLabCrowdFx::LargeRecsParam);
	static const FName NowName(FLabCrowdFx::NowParam);
	const float Now = float(World->GetTimeSeconds());
	for (TArray<FVector4f>& R : Recs)
	{
		R.Reset();
	}
	int32 HitDead = 0;
	int32 Drawn = 0;
	int32 OffRecs = 0;
#if !UE_BUILD_TEST
	const FVector Orb = Crowd->OrbCenter;
	const FVector Camera = Crowd->SnipeView.Location; // 표현 LOD와 같은 기준(카메라까지 3D 거리)
	int32 Ring[2] = {};
	float MaxLag = 0.f;
	double MinCamSq = TNumericLimits<double>::Max();
#endif

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const int32 Type = FMath::Clamp(Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType, 0, LabUnitTypeMax - 1);
		TArray<FVector4f>& Out = Recs[Type];
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassRepresentationFragment> Representations = Context.GetFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FLabAnimStateFragment> AnimStates = Context.GetFragmentView<FLabAnimStateFragment>();
		const TConstArrayView<FLabOffMoveFragment> OffMoves = Context.GetFragmentView<FLabOffMoveFragment>(); // 없는 단계에서는 비어 있다
		// Off 태그 아키타입만 ULabOffMoveProcessor가 틱 간격마다 옮긴다. 그 밖(188~200m의 Medium 포함)은 엔진 이동이 매 프레임 옮겨 위치가 지금 값이다
		const bool bOffMoved = OffMoves.Num() > 0 && Context.DoesArchetypeHaveTag(*FMassOffLODTag::StaticStruct());
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			if (Representations[It].CurrentRepresentation != EMassRepresentationType::None)
			{
				++Drawn;
				continue;
			}
			const FLabAnimStateFragment& Anim = AnimStates[It];
			if (Anim.State != ELabMinionState::Walk)
			{
				++HitDead; // 멈춘 개체(위 헤더 주석). 사망 상태는 태그보다 먼저 표현 상태로 바뀌므로 태그가 아니라 상태로 거른다
				continue;
			}
			const FVector Location = Transforms[It].GetTransform().GetLocation();
			float T0 = Now;
			if (bOffMoved && OffMoves[It].LastMoveTime > 0.)
			{
				T0 = float(OffMoves[It].LastMoveTime);
				++OffRecs;
			}
			Out.Emplace(float(Location.X), float(Location.Y), T0, -Anim.StateStartTime);
#if !UE_BUILD_TEST
			MaxLag = FMath::Max(MaxLag, Now - T0);
			MinCamSq = FMath::Min(MinCamSq, FVector::DistSquared(Location, Camera));
			const double OrbDist = FVector::Dist2D(Location, Orb);
			if (OrbDist >= 48000. && OrbDist < 49000.) { ++Ring[0]; }
			else if (OrbDist >= 49000. && OrbDist <= 50000.) { ++Ring[1]; }
#endif
		}
	});

	// 에셋에 있는 변수만 넣는다. 1종(-LabTypes=1)이면 대형 배열은 빈 배열로 넣어 지난 값을 지운다
	if (Crowd->bCrowdFxSmallRecs)
	{
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(Fx, SmallName, TConstArrayView<FVector4f>(Recs[LabUnitSmall]));
	}
	if (Crowd->bCrowdFxLargeRecs)
	{
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(Fx, LargeName, TConstArrayView<FVector4f>(Recs[LabUnitLarge]));
	}
	if (Crowd->bCrowdFxNow)
	{
		Fx->SetVariableFloat(NowName, Now);
	}
	const int32 Total = Recs[LabUnitSmall].Num() + Recs[LabUnitLarge].Num();
	Crowd->CrowdRecsLast = Total;
	CSV_CUSTOM_STAT(Lab, CrowdRecs, Total, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, CrowdRecsSmall, Recs[LabUnitSmall].Num(), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, CrowdRecsLarge, Recs[LabUnitLarge].Num(), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, CrowdRecsHitDead, HitDead, ECsvCustomStatOp::Set);

#if !UE_BUILD_TEST
	// 개발 확인(5초마다): 원소 수가 그려지지 않은 걷는 엔티티 수와 맞는지(none = recs + hitdead, none + drawn = 풀 밖 엔티티 수), 모으기 시간, 맵 가장자리 고리 밀도
	const double Ms = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - StartCycles);
	++DiagFrames;
	for (int32 Type = 0; Type < LabUnitTypeMax; ++Type)
	{
		DiagRecs[Type] += Recs[Type].Num();
	}
	DiagHitDead += HitDead;
	DiagDrawn += Drawn;
	DiagOff += OffRecs;
	DiagRing[0] += Ring[0];
	DiagRing[1] += Ring[1];
	DiagMs += Ms;
	DiagMaxMs = FMath::Max(DiagMaxMs, Ms);
	DiagMaxLag = FMath::Max(DiagMaxLag, MaxLag);
	if (Crowd->SnipeView.bValid && MinCamSq < TNumericLimits<double>::Max())
	{
		DiagMinCamM = FMath::Min(DiagMinCamM, float(FMath::Sqrt(MinCamSq) / 100.));
	}
	if (DiagSince <= 0.)
	{
		DiagSince = Now;
	}
	else if (Now - DiagSince >= 5.)
	{
		const double F = double(FMath::Max(DiagFrames, 1));
		const double RecsAll = double(DiagRecs[0] + DiagRecs[1]) / F;
		UE_LOG(LogLab, Log, TEXT("lab_crowdrecs t=%.1f frames=%d small=%.0f large=%.0f recs=%.0f hitdead=%.1f none=%.0f drawn=%.0f entities=%.0f off_recs=%.0f max_lag_s=%.3f min_cam_m=%.1f ring480=%.1f ring490=%.1f gather_ms=%.3f gather_max_ms=%.3f small_param=%d large_param=%d now_param=%d"),
			Now, DiagFrames, DiagRecs[0] / F, DiagRecs[1] / F, RecsAll, DiagHitDead / F, RecsAll + DiagHitDead / F, DiagDrawn / F, RecsAll + (DiagHitDead + DiagDrawn) / F,
			DiagOff / F, DiagMaxLag, DiagMinCamM, DiagRing[0] / F, DiagRing[1] / F, DiagMs / F, DiagMaxMs,
			Crowd->bCrowdFxSmallRecs ? 1 : 0, Crowd->bCrowdFxLargeRecs ? 1 : 0, Crowd->bCrowdFxNow ? 1 : 0);
		DiagSince = Now;
		DiagFrames = 0;
		FMemory::Memzero(DiagRecs);
		DiagHitDead = DiagDrawn = DiagOff = 0;
		DiagRing[0] = DiagRing[1] = 0;
		DiagMs = DiagMaxMs = 0.;
		DiagMaxLag = 0.f;
		DiagMinCamM = TNumericLimits<float>::Max();
	}
#endif
}
