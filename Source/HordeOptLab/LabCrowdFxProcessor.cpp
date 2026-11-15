#include "LabCrowdFxProcessor.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabFarStream.h"
#include "LabMassProcessors.h" // ULabFarHitIskmProcessor::HitStartOf
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
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

ULabCrowdFxGatherProcessor::ULabCrowdFxGatherProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = false; // 배경 무리(사다리 단계 7부터)에서만 게임 모듈이 켠다(HordeOptLab.cpp)
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::VisualizationProcessing); // 이번 프레임의 표현(None 여부)이 정해진 뒤
	bRequiresGameThreadExecution = true; // Niagara 컴포넌트의 배열 사용자 변수를 고친다
}

void ULabCrowdFxGatherProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadWrite); // CrowdRecsLast를 쓴다(게임 스레드 전용 프로세서)
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadOnly); // M6 수정 B: 기다리는 피격 개체의 걷기 위상(일련번호)
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
	ULabCrowdSubsystem* Crowd = LabCrowdWrite(Context);
	if (!Crowd)
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
	int32 Frozen = 0; // M6 수정 B: ISKM을 기다리는 피격·사망 개체를 멈춘 입자로 넣은 수
	const bool bFarHitIskm = FLabConfig::Get().UsesFarHitIskm();
	const float HitDuration = Crowd->HitDuration;
	const FVector OrbNow = Crowd->OrbCenter;
#if !UE_BUILD_TEST
	const FVector Orb = Crowd->OrbCenter;
	const FVector Camera = Crowd->SnipeView.Location; // 표현 LOD와 같은 기준(카메라까지 3D 거리)
	int32 Ring[2] = {};
	float MaxLag = 0.f;
	double MinCamSq = TNumericLimits<double>::Max();
	// v2 W8 개발 진단(-LabOrbDiag). 입자 식의 오브 위치는 지난 프레임까지 넣은 값이다(이번 프레임 값은 아래에서 넣는다)
	static const bool bOrbDiag = FParse::Param(FCommandLine::Get(), TEXT("LabOrbDiag"));
	const FVector2D FxOrb(Crowd->CrowdFxOrbPos.X, Crowd->CrowdFxOrbPos.Y);
	const FVector2D NowOrb(Orb.X, Orb.Y);
	const FLabSnipeView View = Crowd->SnipeView;
	TMap<FMassEntityHandle, FLabFxPrev> NextPrev;
	int32 CoverHidden = 0;
#endif

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const int32 Type = FMath::Clamp(Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType, 0, LabUnitTypeMax - 1);
		TArray<FVector4f>& Out = Recs[Type];
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassRepresentationFragment> Representations = Context.GetFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FLabAnimStateFragment> AnimStates = Context.GetFragmentView<FLabAnimStateFragment>();
		const TConstArrayView<FLabMinionFragment> Minions = Context.GetFragmentView<FLabMinionFragment>();
		const TConstArrayView<FLabOffMoveFragment> OffMoves = Context.GetFragmentView<FLabOffMoveFragment>(); // 없는 단계에서는 비어 있다
		// Off 태그 아키타입만 ULabOffMoveProcessor가 틱 간격마다 옮긴다. 그 밖(188~200m의 Medium 포함)은 엔진 이동이 매 프레임 옮겨 위치가 지금 값이다
		const bool bOffMoved = OffMoves.Num() > 0 && Context.DoesArchetypeHaveTag(*FMassOffLODTag::StaticStruct());
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
#if !UE_BUILD_TEST
			if (bOrbDiag && View.bValid && AnimStates[It].State != ELabMinionState::Walk && Representations[It].CurrentRepresentation == EMassRepresentationType::None)
			{
				// 그려지지 않는 피격·사망 개체(입자로도 그리지 않는다) 중 화면 안
				const FVector L = Transforms[It].GetTransform().GetLocation();
				if (View.Contains(L + FVector(0.f, 0.f, 90.f))) { ++DiagHitDeadView; }
			}
			if (bOrbDiag && View.bValid && AnimStates[It].State == ELabMinionState::Walk)
			{
				// 덮임: 화면 안 카메라 150~215m의 걷는 엔티티를 무엇이 그리는가. 입자는 카메라 175m 안에서 그리지 않는다(Scripts/create-crowd-fx.md 5-3 MinCameraDistance)
				const FVector L = Transforms[It].GetTransform().GetLocation();
				const double CamDist = FVector::Dist(L, Camera);
				if (CamDist >= 15000. && CamDist <= 21500. && View.Contains(L + FVector(0.f, 0.f, 90.f)))
				{
					if (Representations[It].CurrentRepresentation != EMassRepresentationType::None) { ++DiagCoverDrawn; }
					else if (CamDist >= 17500.) { ++DiagCoverFx; }
					else { ++DiagCoverHidden; ++CoverHidden; }
				}
				// 전환: 지난 프레임 입자였고 이번 프레임 그려진다. 메시 자리·방향과 지난 입자 자리·방향을 견준다
				if (Representations[It].CurrentRepresentation != EMassRepresentationType::None)
				{
					if (const FLabFxPrev* Prev = DiagPrev.Find(Context.GetEntity(It)))
					{
						const double Jump = (FVector2D(L.X, L.Y) - Prev->Pos).Length();
						const FVector Fwd = Transforms[It].GetTransform().GetRotation().GetForwardVector();
						const FVector2D MeshDir = FVector2D(Fwd.X, Fwd.Y).GetSafeNormal();
						const double Turn = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(MeshDir | Prev->Dir, -1., 1.)));
						DiagJumpMax = FMath::Max(DiagJumpMax, Jump); DiagJumpSum += Jump;
						DiagTurnMax = FMath::Max(DiagTurnMax, Turn); DiagTurnSum += Turn;
						++DiagJumpCount;
					}
				}
			}
#endif
			if (Representations[It].CurrentRepresentation != EMassRepresentationType::None)
			{
				++Drawn;
				continue;
			}
			const FLabAnimStateFragment& Anim = AnimStates[It];
			if (Anim.State != ELabMinionState::Walk)
			{
				++HitDead; // 멈춘 개체(위 헤더 주석). 사망 상태는 태그보다 먼저 표현 상태로 바뀌므로 태그가 아니라 상태로 거른다
				if (bFarHitIskm)
				{
					// M6 수정 B: ISKM이 나올 때까지(Off 청크 갱신, 최대 약 0.55초) 맞은 순간의 걷기 자세로 멈춘 입자로 남긴다.
					// 자리는 입자가 그리던 자리다: 마지막 이동 뒤 맞기까지 걸은 거리를 더한다(ULabFarHitIskmProcessor가 같은 값을 한 번 옮긴다). T0 = 지금이라 외삽하지 않는다.
					// 위상 = 걷기 위상 + 맞은 시각 − 지금이라 재생 시각(지금 + 위상)이 맞은 순간에 멈춘다
					const float HitStart = ULabFarHitIskmProcessor::HitStartOf(Anim, HitDuration);
					FVector L = Transforms[It].GetTransform().GetLocation();
					if (bOffMoved && OffMoves[It].LastMoveTime > 0. && OffMoves[It].LastMoveTime < HitStart)
					{
						FVector ToOrb = OrbNow - L;
						ToOrb.Z = 0.;
						const double Dist = ToOrb.Length();
						if (Dist > KINDA_SMALL_NUMBER)
						{
							L += ToOrb / Dist * FMath::Min(double(FLabScenario::MoveSpeed) * (HitStart - OffMoves[It].LastMoveTime), Dist);
						}
					}
					Out.Emplace(float(L.X), float(L.Y), Now, LabWalkPhase(Minions[It].Serial) + HitStart - Now);
					++Frozen;
				}
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
			if (bOrbDiag)
			{
				// 입자 자리(에셋의 식: P0 + normalize(LabOrbPos − P0) × Speed × max(지금 − T0, 0), Scripts/create-crowd-fx.md 5-5)와
				// 엔티티가 지금 있어야 할 자리(같은 식에 오브의 지금 위치)를 견준다. 200m 밖 이동이 다음 틱에 옮길 방향이 오브의 지금 위치다
				const FVector2D P0(Location.X, Location.Y);
				const double Dt = FMath::Max(double(Now - T0), 0.);
				const FVector2D FxDir = (FxOrb - P0).GetSafeNormal();
				const FVector2D TrueDir = (NowOrb - P0).GetSafeNormal();
				const FVector2D FxPos = P0 + FxDir * FLabScenario::MoveSpeed * Dt;
				const double CamDist = FVector::Dist(Location, Camera);
				if (CamDist >= 17500. && CamDist <= 26000.)
				{
					const double PosErr = (FxPos - (P0 + TrueDir * FLabScenario::MoveSpeed * Dt)).Length();
					const double DirErr = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FxDir | TrueDir, -1., 1.)));
					DiagPosErrMax = FMath::Max(DiagPosErrMax, PosErr); DiagPosErrSum += PosErr;
					DiagDirErrMax = FMath::Max(DiagDirErrMax, DirErr); DiagDirErrSum += DirErr;
					++DiagErrCount;
				}
				if (CamDist <= 23000.)
				{
					NextPrev.Add(Context.GetEntity(It), FLabFxPrev{ FxPos, FxDir });
				}
			}
#endif
		}
	});

	// M6 수정 A(ADR-0020 결정 3): 바깥 기록을 엔티티 원소 뒤에 붙인다. 원소 형식(X, Y, T0, 위상)이 같아 에셋의 외삽 식을 그대로 쓴다.
	// 기록을 꺼낸 프레임에 같은 자리·같은 위상의 엔티티가 None 엔티티로 들어오므로 같은 이미터가 같은 자리에 그린다
	const int32 EntityRecs = Recs[LabUnitSmall].Num() + Recs[LabUnitLarge].Num();
	int32 FarRecs = 0;
	const int32 EntityRecsByType[LabUnitTypeMax] = { Recs[LabUnitSmall].Num(), Recs[LabUnitLarge].Num() }; // 개발 확인 로그는 엔티티 원소만 센다
	if (FLabConfig::Get().DrawsFarStream())
	{
		if (const FLabFarStream* Far = Crowd->GetFarStream())
		{
			for (const FLabFarRec& Rec : Far->GetRecs())
			{
				// T0는 엔티티 원소와 같은 float 월드 초다. 위상은 엔티티가 될 때와 같은 LabWalkPhase(일련번호)
				Recs[FMath::Min(FLabUnitTypes::TypeOf(Rec.Serial), LabUnitTypeMax - 1)].Emplace(Rec.X, Rec.Y, float(Rec.T0), LabWalkPhase(Rec.Serial));
			}
			FarRecs = Far->GetRecs().Num();
		}
	}

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
	// v2 W8: 외삽 목표를 매 프레임 오브의 지금 위치로 넣는다. 접근과 200m 밖 이동이 이번 프레임에 보는 OrbCenter와 같은 값이다.
	// -LabFxOrbFollow=0이면 W7까지처럼 활성화 전에 넣은 값(시작 때 오브 위치, 원점)이 남는다.
	if (Crowd->bCrowdFxOrbPos && FLabConfig::Get().bFxOrbFollow)
	{
		static const FName OrbPosName(FLabCrowdFx::OrbPosParam);
		Fx->SetVariableVec3(OrbPosName, Crowd->OrbCenter);
		Crowd->CrowdFxOrbPos = Crowd->OrbCenter;
	}
	// Lab/CrowdRecs는 전과 같이 엔티티 원소 수다. 소형·대형 스탯은 넣은 배열 크기(기록 포함), 기록 수는 Lab/CrowdFarRecs
	const int32 Total = EntityRecs;
	Crowd->CrowdRecsLast = Total;
	CSV_CUSTOM_STAT(Lab, CrowdRecs, Total, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, CrowdRecsSmall, Recs[LabUnitSmall].Num(), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, CrowdRecsLarge, Recs[LabUnitLarge].Num(), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, CrowdFarRecs, FarRecs, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, CrowdRecsFrozen, Frozen, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, CrowdRecsHitDead, HitDead, ECsvCustomStatOp::Set);

#if !UE_BUILD_TEST
	// 개발 확인(5초마다): 원소 수가 그려지지 않은 걷는 엔티티 수와 맞는지(none = recs + hitdead, none + drawn = 풀 밖 엔티티 수), 모으기 시간, 맵 가장자리 고리 밀도
	const double Ms = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - StartCycles);
	++DiagFrames;
	for (int32 Type = 0; Type < LabUnitTypeMax; ++Type)
	{
		DiagRecs[Type] += EntityRecsByType[Type];
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
	if (bOrbDiag)
	{
		DiagPrev = MoveTemp(NextPrev);
		DiagCoverHiddenMax = FMath::Max(DiagCoverHiddenMax, CoverHidden);
		static double OrbDiagSince = 0.;
		static int32 OrbDiagFrames = 0;
		++OrbDiagFrames;
		if (OrbDiagSince <= 0.)
		{
			OrbDiagSince = Now;
		}
		else if (Now - OrbDiagSince >= 5.)
		{
			// pos_err·dir_err: 그려지지 않은 걷는 엔티티(카메라 175~260m)의 입자 오차(cm, 도). jump·turn: 입자 → 메시 전환 순간의 자리 차이(cm)와 방향 차이(도).
			// cover_*: 화면 안 카메라 150~215m 걷는 엔티티의 프레임 평균(그림, 입자, 아무것도 안 그림), hidden_max는 한 프레임 최대
			const double F = double(FMath::Max(OrbDiagFrames, 1));
			UE_LOG(LogLab, Log, TEXT("lab_orbdiag t=%.1f fx_follow=%d fx_orb=%s orb=%s pos_err_mean=%.1f pos_err_max=%.1f dir_err_mean=%.2f dir_err_max=%.2f err_n=%lld jump_mean=%.1f jump_max=%.1f turn_mean=%.2f turn_max=%.2f switches=%lld cover_drawn=%.1f cover_fx=%.1f cover_hidden=%.2f hidden_max=%d hitdead_view=%.2f"),
				Now, FLabConfig::Get().bFxOrbFollow ? 1 : 0, *FxOrb.ToString(), *NowOrb.ToString(),
				DiagErrCount > 0 ? DiagPosErrSum / DiagErrCount : 0., DiagPosErrMax, DiagErrCount > 0 ? DiagDirErrSum / DiagErrCount : 0., DiagDirErrMax, DiagErrCount,
				DiagJumpCount > 0 ? DiagJumpSum / DiagJumpCount : 0., DiagJumpMax, DiagJumpCount > 0 ? DiagTurnSum / DiagJumpCount : 0., DiagTurnMax, DiagJumpCount,
				DiagCoverDrawn / F, DiagCoverFx / F, DiagCoverHidden / F, DiagCoverHiddenMax, DiagHitDeadView / F);
			DiagHitDeadView = 0;
			OrbDiagSince = Now;
			OrbDiagFrames = 0;
			DiagPosErrMax = DiagPosErrSum = DiagDirErrMax = DiagDirErrSum = 0.;
			DiagErrCount = 0;
			DiagJumpMax = DiagJumpSum = DiagTurnMax = DiagTurnSum = 0.;
			DiagJumpCount = 0;
			DiagCoverDrawn = DiagCoverFx = DiagCoverHidden = 0;
			DiagCoverHiddenMax = 0;
		}
	}
#endif
}
