#include "LabMassProcessors.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabMassTypes.h"
#include "LabSnipe.h"
#include "LabUnitAssets.h"
#include "LabVatProcessor.h"
#include "MassCommands.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "Movement/MassMovementProcessors.h" // ULabSeparationProcessor의 순서(UMassApplyMovementProcessor 뒤)
#include "MassNavigationFragments.h"
#include "MassRepresentationFragments.h" // ULabRepSwitchCountProcessor, ULabRepHoldProcessor
#include "MassRepresentationTypes.h"
#include "MassLODFragments.h" // FMassViewerInfoFragment(ULabRepHoldProcessor)
#include "MassSimulationLOD.h"
#include "HordeOptLab.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/World.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

namespace
{
	/**
	 * M4: 이 쿼리를 Simulation LOD의 가변 틱에 맡긴다. 청크 프래그먼트가 "이 프레임에 돈다"고 한 청크만 순회한다(MassSimulationLOD.h:68-72).
	 * M3까지는 부르지 않아 쿼리가 그대로다.
	 */
	void LabUseVariableTick(FMassEntityQuery& Query)
	{
		Query.AddChunkRequirement<FMassSimulationVariableTickChunkFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
		Query.SetChunkFilter(&FMassSimulationVariableTickChunkFragment::ShouldTickChunkThisFrame);
	}

	/**
	 * 가변 틱 청크 필터를 쓰는 프로세서는 그 값을 쓰는 UMassSimulationLODProcessor(LOD 그룹) 뒤에 돌아야 한다(M4부터).
	 * 순서를 적지 않으면 두 프로세서의 순서가 프레임마다 바뀔 수 있고, "이 프레임에 돈다"는 값은 LOD 프로세서의 한 번과 다음 번 사이에만 참이라
	 * 그 사이에 읽지 못하면 한 주기를 통째로 건너뛴다(lab_chunk 진단 로그, Docs/Worklog/M4.md "사망 전환 지연").
	 * 엔진에서 같은 필터를 쓰는 프로세서도 이렇게 적는다(MassStateTreeProcessors.cpp:165, MassAvoidanceProcessors.cpp:288).
	 */
	void LabRunAfterSimLod(FMassProcessorExecutionOrder& Order)
	{
		if (FLabConfig::Get().UsesSimLod())
		{
			Order.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::LOD);
		}
	}

	/** 청크의 Simulation LOD(0 High ~ 3 Off, 아직 정해지지 않았으면 4). 진단 배열의 색인으로 쓴다 */
	int32 LabChunkLod(const FMassExecutionContext& Context)
	{
		return FMath::Clamp(int32(FMassSimulationVariableTickChunkFragment::GetChunkLOD(Context)), 0, 4);
	}

	/** 피격·사망에 들어간 개체는 멈춘다. 엔진 이동 프로세서는 코드 구동 태그가 있는 엔티티만 옮긴다(MassMovementProcessors.cpp:88). */
	void StopMoving(FMassExecutionContext& Context, FMassEntityHandle Entity)
	{
		Context.Defer().RemoveTag<FMassCodeDrivenMovementTag>(Entity);
	}
}

// 접근 -----------------------------------------------------------------------

ULabApproachProcessor::ULabApproachProcessor()
	: EntityQuery(*this)
	, CountQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks; // 엔진 조향은 Tasks 뒤에 돈다(MassSteeringProcessors.cpp:62)
	LabRunAfterSimLod(ExecutionOrder);
}

void ULabApproachProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadOnly); // 오브 위치를 읽는다
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FLabUnitTypeParams>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabHitTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::None);
	if (FLabConfig::Get().UsesSimLod())
	{
		CountQuery = EntityQuery; // 필터를 걸기 전의 같은 쿼리(진단용, 읽기만 한다)
		LabUseVariableTick(EntityQuery);
		CountQuery.AddChunkRequirement<FMassSimulationVariableTickChunkFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	}
	else
	{
		CountQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All); // 쓰지 않지만 등록된 쿼리는 요구 조건이 있어야 한다
	}
}

void ULabApproachProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const ULabCrowdSubsystem* Crowd = LabCrowdRead(Context);
	if (!Crowd)
	{
		return;
	}
	const FVector Orb = Crowd->OrbCenter;
	const bool bSimLod = FLabConfig::Get().UsesSimLod();
	int32 Ticked[5] = {};
	int32 TickedChunks[5] = {};
	EntityQuery.ForEachEntityChunk(Context, [Orb, bSimLod, &Ticked, &TickedChunks](FMassExecutionContext& Context)
	{
		if (bSimLod)
		{
			const int32 Lod = LabChunkLod(Context);
			Ticked[Lod] += Context.GetNumEntities();
			++TickedChunks[Lod];
		}
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassMoveTargetFragment> MoveTargets = Context.GetMutableFragmentView<FMassMoveTargetFragment>();
		const FLabUnitTypeParams& Params = Context.GetConstSharedFragment<FLabUnitTypeParams>();
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			// M0 액터와 같이 오브 중심으로 곧게 간다. 도착 의도는 Move라 중심에 가까워져도 감속하지 않는다.
			FVector ToOrb = Orb - Transforms[It].GetTransform().GetLocation();
			ToOrb.Z = 0.;
			const float Distance = ToOrb.Length();
			FMassMoveTargetFragment& Target = MoveTargets[It];
			Target.Center = Orb;
			Target.Forward = Distance > KINDA_SMALL_NUMBER ? ToOrb / Distance : FVector::ForwardVector;
			Target.DistanceToGoal = Distance;
			Target.DesiredSpeed = FMassInt16Real(Params.MoveSpeed);
			Target.IntentAtGoal = EMassMovementAction::Move;
		}
	});

	if (!bSimLod)
	{
		return;
	}
	// M4: 이 프레임에 접근 로직이 돈 개체 수(LOD별). 걷는 개체만 센다(피격·사망은 접근 쿼리 밖).
	CSV_CUSTOM_STAT(Lab, SimTickHigh, Ticked[EMassLOD::High], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SimTickMedium, Ticked[EMassLOD::Medium], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SimTickLow, Ticked[EMassLOD::Low], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SimTickOff, Ticked[EMassLOD::Off], ECsvCustomStatOp::Set); // 200m 밖이 Low 대신 Off다

	// 밴드별 걷는 개체 수(필터 없는 같은 쿼리, 청크의 개체 수만 읽는다). 실제 틱 주기 = 평균 프레임 시간 × SimBand ÷ SimTick.
	int32 Band[5] = {};
	int32 BandChunks[5] = {};
	CountQuery.ForEachEntityChunk(Context, [&Band, &BandChunks](FMassExecutionContext& Context)
	{
		const int32 Lod = LabChunkLod(Context);
		Band[Lod] += Context.GetNumEntities();
		++BandChunks[Lod];
	});
	CSV_CUSTOM_STAT(Lab, SimBandHigh, Band[EMassLOD::High], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SimBandMedium, Band[EMassLOD::Medium], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SimBandLow, Band[EMassLOD::Low], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SimBandOff, Band[EMassLOD::Off], ECsvCustomStatOp::Set);

	static const bool bLog = FParse::Param(FCommandLine::Get(), TEXT("LabSimLodLog"));
	if (!bLog)
	{
		return;
	}
	for (int32 Lod = 0; Lod < 5; ++Lod)
	{
		DiagTicked[Lod] += Ticked[Lod];
		DiagTickedChunks[Lod] += TickedChunks[Lod];
		DiagTotal[Lod] += Band[Lod];
		DiagTotalChunks[Lod] += BandChunks[Lod];
	}
	++DiagFrames;
	const double Now = Crowd->GetWorld()->GetTimeSeconds();
	if (DiagSince <= 0.)
	{
		DiagSince = Now;
	}
	else if (Now - DiagSince >= 1.)
	{
		// 개체: 1초 동안의 프레임 평균(돈 수 / 전체). 간격: 평균 프레임 시간 × 전체 ÷ 돈 수 = 개체 하나가 실제로 도는 주기(초).
		const double FrameSec = (Now - DiagSince) / DiagFrames;
		auto Interval = [&](int32 Lod) { return DiagTicked[Lod] > 0 ? FrameSec * double(DiagTotal[Lod]) / double(DiagTicked[Lod]) : 0.; };
		UE_LOG(LogLab, Log, TEXT("lab_simlod t=%.1f frames=%d high=%lld/%lld med=%lld/%lld low=%lld/%lld other=%lld/%lld chunks_high=%lld/%lld chunks_med=%lld/%lld chunks_low=%lld/%lld interval_high=%.3f interval_med=%.3f interval_low=%.3f"),
			Now, DiagFrames,
			DiagTicked[0] / DiagFrames, DiagTotal[0] / DiagFrames, DiagTicked[1] / DiagFrames, DiagTotal[1] / DiagFrames, DiagTicked[2] / DiagFrames, DiagTotal[2] / DiagFrames,
			(DiagTicked[3] + DiagTicked[4]) / DiagFrames, (DiagTotal[3] + DiagTotal[4]) / DiagFrames,
			DiagTickedChunks[0] / DiagFrames, DiagTotalChunks[0] / DiagFrames, DiagTickedChunks[1] / DiagFrames, DiagTotalChunks[1] / DiagFrames, DiagTickedChunks[2] / DiagFrames, DiagTotalChunks[2] / DiagFrames,
			Interval(0), Interval(1), Interval(2));
		FMemory::Memzero(DiagTicked); FMemory::Memzero(DiagTotal); FMemory::Memzero(DiagTickedChunks); FMemory::Memzero(DiagTotalChunks);
		DiagFrames = 0;
		DiagSince = Now;
	}
}

// E4 (d) Off 밴드 이동 ---------------------------------------------------------

ULabOffMoveProcessor::ULabOffMoveProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = false; // Simulation LOD(단계 5부터)에서만 게임 모듈이 켠다(HordeOptLab.cpp)
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement; // 범위 피해(LabLogic)는 Movement 뒤에 돈다
	LabRunAfterSimLod(ExecutionOrder);
}

void ULabOffMoveProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadOnly); // 오브 위치를 읽는다
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLabOffMoveFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassSimulationVariableTickFragment>(EMassFragmentAccess::ReadOnly); // 개발 확인용 비교에만 읽는다
	EntityQuery.AddConstSharedRequirement<FLabUnitTypeParams>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabHitTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassOffLODTag>(EMassFragmentPresence::All); // 엔진 이동·조향 넷이 돌지 않는 개체만
	LabUseVariableTick(EntityQuery);
}

void ULabOffMoveProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	CSV_SCOPED_TIMING_STAT(Lab, OffMove);
	const ULabCrowdSubsystem* Crowd = LabCrowdRead(Context);
	if (!Crowd)
	{
		return;
	}
	const FVector Orb = Crowd->OrbCenter;
	const double Now = Crowd->GetWorld()->GetTimeSeconds();
	const float FrameDelta = Context.GetDeltaTimeSeconds();
	int32 Moved = 0;
	float MaxSeconds = 0.f;
	double SumSeconds = 0.;
	double EngineSumSeconds = 0.;
#if !UE_BUILD_TEST
	// v2 W8 개발 진단(-LabOrbDiag): 한 틱의 직진 이동과, 같은 시간을 8조각으로 나눠 조각마다 그 시각의 오브(FLabOrbit::LocationAt)를 향해 옮긴 기준 자리의 거리
	static const bool bOrbDiag = FParse::Param(FCommandLine::Get(), TEXT("LabOrbDiag"));
	const FLabOrbit& Orbit = FLabConfig::Get().Orbit;
	double StepErrMax = 0., StepErrSum = 0.;
	int64 StepErrCount = 0;
#endif
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TArrayView<FTransformFragment> Transforms = Context.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FMassDesiredMovementFragment> Movements = Context.GetMutableFragmentView<FMassDesiredMovementFragment>();
		const TArrayView<FLabOffMoveFragment> OffMoves = Context.GetMutableFragmentView<FLabOffMoveFragment>();
		const TConstArrayView<FMassSimulationVariableTickFragment> Ticks = Context.GetFragmentView<FMassSimulationVariableTickFragment>();
		const float Speed = Context.GetConstSharedFragment<FLabUnitTypeParams>().MoveSpeed;
		Moved += Context.GetNumEntities();
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			// 옮겨야 할 시간 = 마지막으로 옮긴 뒤 흐른 시간. 새 엔티티(0)는 한 프레임만 옮긴다. Off에 다시 든 개체는 관찰자가 시각을 맞춰 두었다.
			double& LastMove = OffMoves[It].LastMoveTime;
			const float Seconds = LastMove > 0. ? float(Now - LastMove) : FrameDelta;
			LastMove = Now;
#if !UE_BUILD_TEST
			// 비교용: 엔진 값으로 셈했을 때의 시간(지금 − LastTickedTime + 한 프레임, MassLODTickRateController.h:157-164)
			EngineSumSeconds += Ticks[It].LastTickedTime > 0. ? (Now - Ticks[It].LastTickedTime) + FrameDelta : FrameDelta;
#endif
			MaxSeconds = FMath::Max(MaxSeconds, Seconds);
			SumSeconds += Seconds;
			FTransform& Transform = Transforms[It].GetMutableTransform();
			FVector ToOrb = Orb - Transform.GetLocation();
			ToOrb.Z = 0.;
			const double Distance = ToOrb.Length();
			if (Distance <= KINDA_SMALL_NUMBER)
			{
				Movements[It].DesiredVelocity = FVector::ZeroVector;
				continue;
			}
			// 방향은 틱마다 오브의 지금 위치(OrbCenter)로 다시 잡는다(v2 사양 4.2). 틱 사이 오차는 평균 0.3cm, 최대 1.8cm다(Docs/Worklog/V2.md "W8", 아래 -LabOrbDiag)
			const FVector Direction = ToOrb / Distance;
#if !UE_BUILD_TEST
			const FVector Start = Transform.GetLocation();
#endif
			Transform.AddToTranslation(Direction * FMath::Min(double(Speed * Seconds), Distance));
			Movements[It].DesiredVelocity = Direction * Speed; // 200m 안으로 돌아오면 엔진 조향이 이 속도에서 이어 간다
#if !UE_BUILD_TEST
			if (bOrbDiag && Seconds > FrameDelta * 2.f && Distance > Speed * Seconds)
			{
				constexpr int32 Pieces = 8;
				const double Piece = double(Seconds) / Pieces;
				FVector Ref = Start;
				for (int32 k = 0; k < Pieces; ++k)
				{
					FVector To = Orbit.LocationAt(Now - Seconds + (k + 1) * Piece) - Ref;
					To.Z = 0.;
					Ref += To.GetSafeNormal() * Speed * Piece;
				}
				const double Err = FVector::Dist2D(Transform.GetLocation(), Ref);
				StepErrMax = FMath::Max(StepErrMax, Err); StepErrSum += Err; ++StepErrCount;
			}
#endif
		}
	});
#if !UE_BUILD_TEST
	if (bOrbDiag)
	{
		static double DiagSince = 0., DiagErrMax = 0., DiagErrSum = 0.;
		static int64 DiagErrCount = 0;
		DiagErrMax = FMath::Max(DiagErrMax, StepErrMax); DiagErrSum += StepErrSum; DiagErrCount += StepErrCount;
		if (DiagSince <= 0.) { DiagSince = Now; }
		else if (Now - DiagSince >= 5.)
		{
			const ULabCrowdSubsystem* C = Crowd;
			UE_LOG(LogLab, Log, TEXT("lab_offstep t=%.1f steps=%lld step_err_mean=%.2f step_err_max=%.2f leave_n=%lld leave_lag_mean=%.3f leave_lag_max=%.3f"),
				Now, DiagErrCount, DiagErrCount > 0 ? DiagErrSum / DiagErrCount : 0., DiagErrMax,
				C->OffLeaveCount.load(), C->OffLeaveCount.load() > 0 ? C->OffLeaveLagSum.load() / double(C->OffLeaveCount.load()) : 0., C->OffLeaveLagMax.load());
			DiagSince = Now; DiagErrMax = DiagErrSum = 0.; DiagErrCount = 0;
		}
	}
#endif
	CSV_CUSTOM_STAT(Lab, OffMoveCount, Moved, ECsvCustomStatOp::Set);

#if !UE_BUILD_TEST
	// 개발 확인(-LabSimLodLog): 1초마다 옮긴 개체 수(프레임 평균), 한 번에 옮긴 시간의 최대와 평균. 최대 × 걷는 속도가 한 번에 뛰는 거리의 상한이다.
	static const bool bLog = FParse::Param(FCommandLine::Get(), TEXT("LabSimLodLog"));
	if (bLog)
	{
		static double Since = 0., DiagSum = 0., DiagEngineSum = 0.;
		static int64 DiagMoved = 0;
		static int32 DiagFrames = 0;
		static float DiagMax = 0.f;
		DiagMoved += Moved; DiagSum += SumSeconds; DiagEngineSum += EngineSumSeconds; DiagMax = FMath::Max(DiagMax, MaxSeconds); ++DiagFrames;
		if (Since <= 0.)
		{
			Since = Now;
		}
		else if (Now - Since >= 1.)
		{
			UE_LOG(LogLab, Log, TEXT("lab_offmove t=%.1f frames=%d moved_per_frame=%lld max_step_s=%.3f mean_step_s=%.3f sum_s=%.1f engine_sum_s=%.1f"),
				Now, DiagFrames, DiagMoved / DiagFrames, DiagMax, DiagMoved > 0 ? DiagSum / double(DiagMoved) : 0., DiagSum, DiagEngineSum);
			Since = Now; DiagSum = 0.; DiagEngineSum = 0.; DiagMoved = 0; DiagFrames = 0; DiagMax = 0.f;
		}
	}
#endif
}

ULabOffMoveEnterObserver::ULabOffMoveEnterObserver()
	: EntityQuery(*this)
{
	ObservedTypes.Add(FMassOffLODTag::StaticStruct());
	ObservedOperations = EMassObservedOperationFlags::AddElement; // 새 엔티티는 LastMoveTime 0으로 시작해 따로 다룬다
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	// Simulation LOD(단계 5부터)에서만 관찰자로 등록한다. 등록은 CDO의 PostInitProperties에서 일어나고 그때 실행 인자는 이미 읽을 수 있다(FLabConfig::Get).
	bAutoRegisterWithObserverRegistry = FLabConfig::Get().UsesSimLod();
#if !UE_BUILD_TEST
	// 개발 확인(-LabSimLodLog): Off 태그가 떨어질 때도 불려, 그때의 카메라 거리와 마지막 이동 뒤 흐른 시간을 로그에 남긴다(lab_event offleave).
	if (FParse::Param(FCommandLine::Get(), TEXT("LabSimLodLog")) || FParse::Param(FCommandLine::Get(), TEXT("LabOrbDiag")))
	{
		ObservedOperations |= EMassObservedOperationFlags::RemoveElement;
	}
#endif
}

void ULabOffMoveEnterObserver::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadOnly); // 개발 확인 로그에만 읽는다
	EntityQuery.AddRequirement<FLabOffMoveFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly); // 개발 확인 로그에만 읽는다
}

void ULabOffMoveEnterObserver::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const UWorld* World = Context.GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	const FMassObserverExecutionContext* Observed = Context.GetAuxData().GetPtr<const FMassObserverExecutionContext>();
	const bool bRemoved = Observed && Observed->GetOperationType() == EMassObservedOperation::RemoveElement;
#if !UE_BUILD_TEST
	static const bool bLog = FParse::Param(FCommandLine::Get(), TEXT("LabSimLodLog"));
	static const bool bOrbDiag = FParse::Param(FCommandLine::Get(), TEXT("LabOrbDiag"));
	const ULabCrowdSubsystem* Crowd = bLog ? Context.GetSubsystem<ULabCrowdSubsystem>() : nullptr;
	ULabCrowdSubsystem* DiagCrowd = bOrbDiag && bRemoved ? const_cast<ULabCrowdSubsystem*>(Context.GetSubsystem<ULabCrowdSubsystem>()) : nullptr; // 원자 변수만 쓴다
#endif
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TArrayView<FLabOffMoveFragment> OffMoves = Context.GetMutableFragmentView<FLabOffMoveFragment>();
#if !UE_BUILD_TEST
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
#endif
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
#if !UE_BUILD_TEST
			if (Crowd)
			{
				// offleave의 x: 마지막으로 옮긴 뒤 흐른 초(옮긴 적이 없으면 -1). offenter의 x: 전에 옮긴 적이 있으면 1(Off에 다시 든 개체)
				const double Last = OffMoves[It].LastMoveTime;
				LabLogScreenEvent(bRemoved ? TEXT("offleave") : TEXT("offenter"), Crowd->SnipeView, Transforms[It].GetTransform().GetLocation(),
					bRemoved ? (Last > 0. ? float(Now - Last) : -1.f) : (Last > 0. ? 1.f : 0.f));
			}
			if (DiagCrowd && OffMoves[It].LastMoveTime > 0.)
			{
				const double Lag = Now - OffMoves[It].LastMoveTime;
				DiagCrowd->OffLeaveCount.store(DiagCrowd->OffLeaveCount.load() + 1);
				DiagCrowd->OffLeaveLagSum.store(DiagCrowd->OffLeaveLagSum.load() + Lag);
				DiagCrowd->OffLeaveLagMax.store(FMath::Max(DiagCrowd->OffLeaveLagMax.load(), Lag));
			}
#endif
			if (!bRemoved)
			{
				OffMoves[It].LastMoveTime = Now;
			}
		}
	});
}

// 범위 피해 ------------------------------------------------------------------

ULabOrbContactProcessor::ULabOrbContactProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = LabLogicGroupName;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	LabRunAfterSimLod(ExecutionOrder);
}

void ULabOrbContactProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadWrite); // 사망 수와 번개 대상을 쓴다(ResultLock)
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadWrite); // 사망 전환을 표현에 알린다
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabHitTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::None);
	if (FLabConfig::Get().UsesSimLod())
	{
		// M4: 건너뛴 시간은 엔진 누적 델타로 보상한다(초당 피해 × 마지막으로 돈 뒤 흐른 시간). 걷는 개체는 상태가 바뀌지 않아 델타에 다른 상태의 시간이 섞이지 않는다.
		EntityQuery.AddRequirement<FMassSimulationVariableTickFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
		LabUseVariableTick(EntityQuery);
	}
}

void ULabOrbContactProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowdWrite(Context);
	if (!Crowd)
	{
		return;
	}
	const float Now = Crowd->GetWorld()->GetTimeSeconds();
	const bool bSimLod = FLabConfig::Get().UsesSimLod();
	{
		FScopeLock Lock(&Crowd->ResultLock);
		// M4: 피해 범위의 청크가 이 프레임에 돌지 않으면(탑뷰 P2는 오브 주변이 Medium) 번개 대상이 깜박인다. 대상을 찾은 청크의 틱 간격 동안은 지난 값을 둔다.
		// High(P1의 피해 범위)는 간격이 0이라 M3까지와 같이 매 프레임 지운다.
		if (!bSimLod || Now >= Crowd->NearestHoldUntil)
		{
			Crowd->bHasNearest = false;
		}
		Crowd->NearestDistSq = TNumericLimits<float>::Max();
	}
	const FVector Center = Crowd->OrbCenter;
	const float ZoneRadiusSq = FMath::Square(FLabScenario::ZoneRadius);
	const float DeathDuration = Crowd->DeathDuration;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FLabHealthFragment> Healths = Context.GetMutableFragmentView<FLabHealthFragment>();
		const TArrayView<FLabMinionFragment> Minions = Context.GetMutableFragmentView<FLabMinionFragment>();
		const TArrayView<FLabAnimStateFragment> AnimStates = Context.GetMutableFragmentView<FLabAnimStateFragment>();
		const float FrameDelta = Context.GetDeltaTimeSeconds();
		// M4에서만 있다. 없으면(M3까지) 프레임 델타를 쓴다
		const TConstArrayView<FMassSimulationVariableTickFragment> VariableTicks = bSimLod ? Context.GetFragmentView<FMassSimulationVariableTickFragment>() : TConstArrayView<FMassSimulationVariableTickFragment>();
		const bool bAccumulated = VariableTicks.Num() > 0;

		int32 Kills = 0;
		float NearestSq = TNumericLimits<float>::Max();
		FVector Nearest = FVector::ZeroVector;
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			const FVector Location = Transforms[It].GetTransform().GetLocation();
			const float DistSq = FVector::DistSquared2D(Location, Center);
			if (DistSq > ZoneRadiusSq)
			{
				continue;
			}
			if (DistSq < NearestSq)
			{
				NearestSq = DistSq;
				Nearest = Location;
			}
			// M0 ALabOrb::Tick과 같은 피해: 가장자리 ZoneDpsEdge → 중심 ZoneDpsCenter.
			const float Closeness = 1.f - FMath::Sqrt(DistSq) / FLabScenario::ZoneRadius;
			FLabHealthFragment& Health = Healths[It];
			const float DeltaSeconds = bAccumulated ? VariableTicks[It].DeltaTime : FrameDelta;
			Health.Health -= FMath::Lerp(FLabScenario::ZoneDpsEdge, FLabScenario::ZoneDpsCenter, Closeness) * DeltaSeconds;
			if (Health.Health <= 0.f)
			{
				const FMassEntityHandle Entity = Context.GetEntity(It);
				Minions[It].StateTimeLeft = DeathDuration;
				AnimStates[It].Set(ELabMinionState::Dead, Now); // 표현은 이 값만 본다. 태그는 지연돼도 같은 프레임에 죽기 클립이 시작된다
				Context.Defer().AddTag<FLabDeadTag>(Entity);
				StopMoving(Context, Entity);
				++Kills;
			}
		}

		FScopeLock Lock(&Crowd->ResultLock);
		Crowd->KillCount += Kills;
		if (NearestSq < Crowd->NearestDistSq)
		{
			Crowd->NearestDistSq = NearestSq;
			Crowd->Nearest = Nearest;
			Crowd->bHasNearest = true;
			if (bSimLod)
			{
				const int32 Lod = LabChunkLod(Context);
				Crowd->NearestHoldUntil = Now + (Lod == EMassLOD::Medium ? FLabSimLod::TickMedium : Lod == EMassLOD::Low ? FLabSimLod::TickLow : 0.f) * 1.2f; // 엔진이 간격에 ±10%를 더한다(MassLODTickRateController.h:143)
			}
		}
	});
}

// 저격 -----------------------------------------------------------------------

ULabOrbSnipeProcessor::ULabOrbSnipeProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = LabLogicGroupName;
	ExecutionOrder.ExecuteAfter.Add(ULabOrbContactProcessor::StaticClass()->GetFName());
}

void ULabOrbSnipeProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadWrite); // 저격 틱을 가져가고 대상 위치를 쓴다(원자 변수, ResultLock)
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabHealthFragment>(EMassFragmentAccess::ReadOnly);
	// 순회에서는 Serial만 읽지만 고른 대상의 StateTimeLeft와 표현 상태를 바꾸므로 쓰기로 선언한다(Shabel 12:32, talk-notes.md).
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FLabUnitTypeParams>(EMassFragmentPresence::All); // 2종: 청크의 종류(대형 예약 발, LabSnipe.h)
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabHitTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::None);
}

void ULabOrbSnipeProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowdWrite(Context);
	if (!Crowd || Crowd->PendingSnipeTick.load() == 0)
	{
		return;
	}
	const uint32 SnipeTick = Crowd->PendingSnipeTick.exchange(0); // 오브가 단계 시작에 쓴 값을 한 번만 가져간다(std::atomic, ALabOrb::OnMassPrePhysicsStart)
	if (SnipeTick == 0)
	{
		return;
	}
	const FVector Center = Crowd->OrbCenter;
	const FLabSnipeView View = Crowd->SnipeView;
	if (!View.bValid)
	{
		return;
	}

	// M0 ALabOrb::Snipe와 같은 선택: 화면 안의 산 개체를 거리 구간별로 나눠 구간마다 해시가 가장 작은 개체(LabSnipe.h). 2종은 예약 발만 대형.
	TLabSnipeSelector<FMassEntityHandle> Picker(SnipeTick);
	// v2 W5 캡슐 확인 대상(LabCrowdSubsystem.h CapsuleCheckEntities): 액터가 붙은 화면 안의 산 개체 중 해시 최소. 대상 선택(Picker)과 따로 고르고 피해를 주지 않는다
	FMassEntityHandle CheckBest;
	uint32 CheckBestHash = MAX_uint32;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const bool bLarge = Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType == LabUnitLarge; // 종류는 청크 단위(아키타입)
		const bool bActorChunk = Context.DoesArchetypeHaveTag<FLabActorTag>(); // 하이드레이션 액터가 붙은 아키타입(ULabActorManagement::OnPostActorSpawn)
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FLabHealthFragment> Healths = Context.GetFragmentView<FLabHealthFragment>();
		const TConstArrayView<FLabMinionFragment> Minions = Context.GetFragmentView<FLabMinionFragment>();
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			// 같은 프레임 범위 피해로 죽은 개체는 태그가 아직 붙지 않았으므로 체력으로 거른다.
			if (Healths[It].Health <= 0.f)
			{
				continue;
			}
			const FVector Location = Transforms[It].GetTransform().GetLocation();
			const int32 Band = FLabSnipeBands::BandOf(FVector::Dist2D(Location, Center));
			const bool bInView = (Band != INDEX_NONE || bActorChunk) && View.Contains(FVector(Location.X, Location.Y, LabSnipeAimHeight));
			if (bActorChunk && bInView)
			{
				const uint32 Hash = LabSnipeHash(Minions[It].Serial, SnipeTick);
				if (Hash < CheckBestHash) { CheckBestHash = Hash; CheckBest = Context.GetEntity(It); }
			}
			if (Band == INDEX_NONE || !bInView)
			{
				continue;
			}
			Picker.Offer(Band, LabSnipeHash(Minions[It].Serial, SnipeTick), Context.GetEntity(It), Location, bLarge);
		}
	});

	const float HitDuration = Crowd->HitDuration;
	const float Now = Crowd->GetWorld()->GetTimeSeconds();
	FScopeLock Lock(&Crowd->ResultLock);
	Picker.ForEach([&](const TLabSnipeSelector<FMassEntityHandle>::FPick& Pick)
	{
		const FMassEntityHandle Target = Pick.Target;
		// 대상은 순회가 끝난 뒤에야 정해지므로 쿼리 안에서 쓸 수 없다. 태그 추가와 같은 플러시에서 지연 명령으로 쓴다(2초에 5건).
		Context.Defer().PushCommand<FMassDeferredSetCommand>([Target, HitDuration, Now](FMassEntityManager& Manager)
		{
			if (Manager.IsEntityValid(Target))
			{
				Manager.GetFragmentDataChecked<FLabMinionFragment>(Target).StateTimeLeft = HitDuration;
				Manager.GetFragmentDataChecked<FLabAnimStateFragment>(Target).Set(ELabMinionState::Hit, Now);
			}
		});
		Context.Defer().AddTag<FLabHitTag>(Target);
		StopMoving(Context, Target);
#if !UE_BUILD_TEST
		LabLogScreenEvent(Picker.bHasLarge && Pick.Target == Picker.LargeBest.Target ? TEXT("snipe_large") : TEXT("snipe_small"), View, Pick.Location); // 개발용 진단: 피격 멈춤 확인
#endif
		Crowd->SnipeTargets.Add(FVector(Pick.Location.X, Pick.Location.Y, LabSnipeAimHeight)); // M0과 같은 몸통 높이
		Crowd->SnipeTargetEntities.Add(Target); // v2 W5 캡슐 트레이스(ALabOrb::TraceSnipeCapsules)
		++Crowd->SnipeCount;
	});
	if (CheckBest.IsSet())
	{
		Crowd->CapsuleCheckEntities.Add(CheckBest);
	}
}

// 동등성 통계: 오브 30m 안 개체 수(Q5) ---------------------------------------

ULabNearOrbCountProcessor::ULabNearOrbCountProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	// 그룹과 순서를 선언하지 않는다. 읽기 전용 순회라 솔버가 트랜스폼을 쓰는 프로세서와 겹치지 않게만 둔다. 값은 이번 프레임 이동 앞이든 뒤든 같은 뜻이다
}

void ULabNearOrbCountProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadWrite); // Near30을 쓴다(원자 변수)
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabPooledTag>(EMassFragmentPresence::None); // 풀 엔티티는 맵 밖에 서 있어 어차피 세지 않지만 뜻을 분명히 한다
}

void ULabNearOrbCountProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowdWrite(Context);
	if (!Crowd)
	{
		return;
	}
	const FVector Center = Crowd->OrbCenter;
	const double RadiusSq = FMath::Square(double(FLabEquivalence::NearOrbCm));
	int32 Count = 0;
	EntityQuery.ForEachEntityChunk(Context, [Center, RadiusSq, &Count](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			if (FVector::DistSquared2D(Transforms[It].GetTransform().GetLocation(), Center) <= RadiusSq)
			{
				++Count;
			}
		}
	});
	Crowd->Near30.store(Count);
}

// 겹친 쌍의 수(v2 W3) ----------------------------------------------------------

ULabOverlapCountProcessor::ULabOverlapCountProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	// 그룹과 순서를 선언하지 않는다(ULabNearOrbCountProcessor와 같다). 읽기 전용 순회다
}

void ULabOverlapCountProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadWrite); // 겹친 쌍의 수를 쓴다(원자 변수)
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAgentRadiusFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FLabPooledTag>(EMassFragmentPresence::None);
}

void ULabOverlapCountProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowdWrite(Context);
	if (!Crowd)
	{
		return;
	}
	const double Now = Crowd->GetWorld()->GetTimeSeconds();
	if (Now < NextSampleTime)
	{
		return;
	}
	NextSampleTime = Now + LabOverlapSampleSec;
	CSV_SCOPED_TIMING_STAT(Lab, OverlapCount);
	Bodies.Reset();
	float MaxRadius = 0.f;
	EntityQuery.ForEachEntityChunk(Context, [this, &MaxRadius](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FAgentRadiusFragment> Radii = Context.GetFragmentView<FAgentRadiusFragment>();
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			const FVector Location = Transforms[It].GetTransform().GetLocation();
			FLabBody& Body = Bodies.AddDefaulted_GetRef();
			Body.X = float(Location.X);
			Body.Y = float(Location.Y);
			Body.Radius = Radii[It].Radius;
			MaxRadius = FMath::Max(MaxRadius, Body.Radius);
		}
	});
	int32 Pairs = 0;
	int32 Deep = 0;
	LabCountOverlaps(Bodies, MaxRadius, Pairs, Deep);
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	// 개발 검증(-LabOverlapCheck): 같은 표본을 모든 쌍으로 다시 세어 격자 결과와 함께 로그에 남긴다(Docs/Worklog/V2.md "W3")
	static const bool bCheck = FParse::Param(FCommandLine::Get(), TEXT("LabOverlapCheck"));
	if (bCheck)
	{
		int32 BrutePairs = 0;
		int32 BruteDeep = 0;
		for (int32 A = 0; A < Bodies.Num(); ++A)
		{
			for (int32 B = A + 1; B < Bodies.Num(); ++B)
			{
				const float DX = Bodies[A].X - Bodies[B].X;
				const float DY = Bodies[A].Y - Bodies[B].Y;
				const float Sum = Bodies[A].Radius + Bodies[B].Radius;
				const float DistSq = DX * DX + DY * DY;
				BrutePairs += DistSq < Sum * Sum ? 1 : 0;
				BruteDeep += DistSq < 0.25f * Sum * Sum ? 1 : 0;
			}
		}
		UE_LOG(LogLab, Log, TEXT("lab_overlap_check t=%.1f bodies=%d grid=%d/%d brute=%d/%d %s"), Now, Bodies.Num(), Pairs, Deep, BrutePairs, BruteDeep,
			(Pairs == BrutePairs && Deep == BruteDeep) ? TEXT("match") : TEXT("MISMATCH"));
	}
#endif
	Crowd->OverlapPairs.store(Pairs);
	Crowd->OverlapDeep.store(Deep);
}

// 겹침 방지, 직접 구현(v2 W4) ---------------------------------------------------

ULabSeparationProcessor::ULabSeparationProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = false; // FLabConfig::UsesSeparation()일 때만 게임 모듈이 켠다(기본 켬, -LabSeparate=0이 끈다, HordeOptLab.cpp)
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	// 그 프레임의 이동이 끝난 위치를 고친다. 범위 피해(LabLogic)는 Movement 그룹 뒤라 고친 위치를 본다
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(UMassApplyMovementProcessor::StaticClass()->GetFName());
	if (FLabConfig::Get().UsesSimLod())
	{
		ExecutionOrder.ExecuteAfter.Add(ULabOffMoveProcessor::StaticClass()->GetFName()); // 등록되지 않은 이름은 해결기가 빈 노드로 둔다(engine-notes.md 37절). 그래도 쓸 때만 적는다
	}
}

void ULabSeparationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadOnly); // 개발 확인 로그에 겹친 쌍의 수를 읽는다(원자 변수)
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAgentRadiusFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
	// 겹친 쌍의 수(ULabOverlapCountProcessor)와 같은 개체
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FLabPooledTag>(EMassFragmentPresence::None);
}

void ULabSeparationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	CSV_SCOPED_TIMING_STAT(Lab, Separation);
	const FLabSeparationParams& Params = FLabConfig::Get().Separation;
	Solver.Reset();
	Transforms.Reset();
	Movements.Reset();
	EntityQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext& Context)
	{
		const TArrayView<FTransformFragment> TransformList = Context.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FAgentRadiusFragment> Radii = Context.GetFragmentView<FAgentRadiusFragment>();
		const TArrayView<FMassDesiredMovementFragment> MovementList = Context.GetMutableFragmentView<FMassDesiredMovementFragment>();
		const bool bHasMovement = MovementList.Num() > 0;
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FTransformFragment& Transform = TransformList[It];
			const FVector Location = Transform.GetTransform().GetLocation();
			Solver.Add(float(Location.X), float(Location.Y), Radii[It].Radius);
			Transforms.Add(&Transform);
			Movements.Add(bHasMovement ? &MovementList[It] : nullptr);
		}
	});
	Solver.Solve(Params, /*bParallel*/ true);

	int32 Pushed = 0;
	double PushSum = 0.;
	float PushMax = 0.f;
	for (int32 I = 0; I < Transforms.Num(); ++I)
	{
		const FVector2f Push = Solver.Push[I];
		const float LenSq = Push.SizeSquared();
		if (LenSq < 1.e-4f) // 0.01cm 아래는 옮기지 않는다
		{
			continue;
		}
		const float Len = FMath::Sqrt(LenSq);
		++Pushed;
		PushSum += Len;
		PushMax = FMath::Max(PushMax, Len);
		Transforms[I]->GetMutableTransform().AddToTranslation(FVector(Push.X, Push.Y, 0.));
		if (Params.bVelocity && Movements[I])
		{
			// 밀린 방향의 반대(이웃 쪽)로 가는 성분을 뺀다. 엔진 조향이 다음 프레임부터 가속 상한으로 다시 붙인다(engine-notes.md 36절)
			FVector& Velocity = Movements[I]->DesiredVelocity;
			const double NX = Push.X / Len;
			const double NY = Push.Y / Len;
			const double Into = Velocity.X * NX + Velocity.Y * NY;
			if (Into < 0.)
			{
				Velocity.X -= Into * NX;
				Velocity.Y -= Into * NY;
			}
		}
	}
	CSV_CUSTOM_STAT(Lab, SepBodies, Transforms.Num(), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SepPushed, Pushed, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SepPushMean, Pushed > 0 ? float(PushSum / Pushed) : 0.f, ECsvCustomStatOp::Set); // 옮긴 개체 하나가 이번 프레임에 옮긴 거리(cm)

#if !UE_BUILD_TEST
	// 개발 확인: 5초마다 옮긴 개체 수(프레임 평균), 옮긴 거리의 평균·최대(cm), 마지막 겹친 쌍 표본
	LogPushed += Pushed;
	LogPushSum += PushSum;
	LogPushMax = FMath::Max(LogPushMax, PushMax);
	++LogFrames;
	const UWorld* World = Context.GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.;
	if (LogSince <= 0.)
	{
		LogSince = Now;
	}
	else if (Now - LogSince >= 5.)
	{
		const ULabCrowdSubsystem* Crowd = LabCrowdRead(Context);
		UE_LOG(LogLab, Log, TEXT("lab_sep t=%.1f frames=%d bodies=%d pushed_per_frame=%lld push_mean_cm=%.2f push_max_cm=%.1f overlap=%d/%d iter=%d relax=%.2f maxstep=%.2f vel=%d skin=%.2f"),
			Now, LogFrames, Transforms.Num(), LogPushed / LogFrames, LogPushed > 0 ? LogPushSum / double(LogPushed) : 0., LogPushMax,
			Crowd ? Crowd->OverlapPairs.load() : -1, Crowd ? Crowd->OverlapDeep.load() : -1, Params.Iterations, Params.Relax, Params.MaxStepFrac, Params.bVelocity ? 1 : 0, Params.Skin);
		LogSince = Now;
		LogPushed = 0;
		LogPushSum = 0.;
		LogPushMax = 0.f;
		LogFrames = 0;
	}
#endif
}

// 표현 전환 수(Spike1에서 시작, v2 W7에서 경계별로 정리) -------------------------

uint8 ULabRepSwitchCountProcessor::ClassOf(EMassRepresentationType Type)
{
	return UE::Mass::Representation::IsValidActorRepresentation(Type) ? 0 : Type == EMassRepresentationType::None ? 2 : 1;
}

ULabRepSwitchCountProcessor::ULabRepSwitchCountProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true; // v2: 늘 돈다(v2-spec.md 4.1)
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	// ULabIskmProcessor와 같은 자리: 엔진 표현 프로세서가 이번 프레임의 Prev·Current를 쓴 뒤
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::VisualizationProcessing);
	bRequiresGameThreadExecution = true; // 전환 기록 맵(ULabCrowdSubsystem::RepLastSwitch, ShadowLast)
}

void ULabRepSwitchCountProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadWrite); // 전환 기록을 쓴다(W2 방식, engine-notes.md 35절)
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly); // 80m 경계(LOD Medium ↔ Low)와 화면 안팎
#if !UE_BUILD_TEST
	EntityQuery.AddRequirement<FMassViewerInfoFragment>(EMassFragmentAccess::ReadOnly); // 개발 확인 로그의 카메라 거리
#endif
	EntityQuery.AddSharedRequirement<FMassVisualizationLODSharedFragment>(EMassFragmentAccess::ReadOnly); // 상한이 걸렸는지
	EntityQuery.AddChunkRequirement<FMassVisualizationChunkFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabPooledTag>(EMassFragmentPresence::None);
}

void ULabRepSwitchCountProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowdWrite(Context);
	const UWorld* World = Context.GetWorld();
	if (!Crowd || !World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	TMap<FMassEntityHandle, ULabCrowdSubsystem::FLabRepSwitch>& LastSwitch = Crowd->RepLastSwitch;
	int32 NoneToInst = 0, InstToActor = 0, ActorToInst = 0, InstToNone = 0, Other = 0, ShadowOff = 0, ShadowOn = 0;
	int32 Round = 0, RoundActor = 0, RoundFar = 0, RoundShadow = 0;
	bool bCapOn = false;
	// v2 W10: 엔진 ISM 프로세서를 쓰는 사다리 단계(2·3)는 VAT 프로세서의 밴드 집계가 없어 HUD의 인스턴스 수를 여기서 센다(HUD가 보일 때만)
	const bool bHudInstances = !FLabConfig::Get().UsesVat() && Crowd->bHudCounts.load(std::memory_order_relaxed);
	int32 HudInstances = 0;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		bCapOn |= Context.GetSharedFragment<FMassVisualizationLODSharedFragment>().bHasAdjustedDistancesFromCount;
		if (bHudInstances)
		{
			for (const FMassRepresentationFragment& R : Context.GetFragmentView<FMassRepresentationFragment>())
			{
				HudInstances += R.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance ? 1 : 0;
			}
		}
		if (!Context.GetChunkFragment<FMassVisualizationChunkFragment>().ShouldUpdateVisualization())
		{
			return; // 엔진이 이번 프레임에 갱신하지 않은 청크는 Prev가 옛 값이다
		}
		const TConstArrayView<FMassRepresentationFragment> Representations = Context.GetFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> LODs = Context.GetFragmentView<FMassRepresentationLODFragment>();
#if !UE_BUILD_TEST
		const TConstArrayView<FMassViewerInfoFragment> Viewers = Context.GetFragmentView<FMassViewerInfoFragment>();
#endif
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			const FMassRepresentationFragment& R = Representations[It];
			const FMassRepresentationLODFragment& L = LODs[It];
			if (R.IsWaitingForActorVisualReadiness())
			{
				continue;
			}
			const uint8 From = ClassOf(R.PrevRepresentation);
			const uint8 To = ClassOf(R.CurrentRepresentation);
			if (From == To)
			{
				// 80m 경계: 인스턴스인 채로 LOD가 Medium(그림자 켬) ↔ Low(그림자 끔)로 바뀐 프레임(Q4b, LabCrowdSubsystem.cpp의 설명자 범위 [High, Low)·[Low, Max))
				const bool bOff = L.PrevLOD == EMassLOD::Medium && L.LOD == EMassLOD::Low;
				const bool bOn = L.PrevLOD == EMassLOD::Low && L.LOD == EMassLOD::Medium;
				if (To == 1 && (bOff || bOn))
				{
					if (bOff) { ++ShadowOff; } else { ++ShadowOn; }
					FShadowSwitch& Last = ShadowLast.FindOrAdd(Context.GetEntity(It));
					if (Last.Time > 0. && Now - Last.Time <= RoundTripSec && Last.bOff != bOff)
					{
						++RoundShadow;
					}
					Last = { Now, bOff };
				}
				continue;
			}
			if (From == 2 && To == 1) { ++NoneToInst; }
			else if (From == 1 && To == 0) { ++InstToActor; }
			else if (From == 0 && To == 1) { ++ActorToInst; }
			else if (From == 1 && To == 2) { ++InstToNone; }
			else { ++Other; }
			const bool bVisible = L.Visibility == EMassVisibility::CanBeSeen;
			ULabCrowdSubsystem::FLabRepSwitch& Last = LastSwitch.FindOrAdd(Context.GetEntity(It));
			if (Last.Time > 0. && Now - Last.Time <= RoundTripSec && Last.From == To && Last.To == From)
			{
				++Round;
				const bool bActorEdge = (From | To) == 1; // {0, 1}
				const bool bFarEdge = From + To == 3;     // {1, 2}
				RoundActor += bActorEdge ? 1 : 0;
				RoundFar += bFarEdge ? 1 : 0;
#if !UE_BUILD_TEST
				if (bActorEdge)
				{
					(Last.bVisible != bVisible ? LogRoundActorVis : LogRoundActorDist) += 1;
					LogRoundActorCap += Context.GetSharedFragment<FMassVisualizationLODSharedFragment>().bHasAdjustedDistancesFromCount ? 1 : 0;
					const float DistM = FMath::Sqrt(Viewers[It].ClosestViewerDistanceSq) / 100.f;
					LogRoundDistHist[FMath::Clamp(int32(DistM / 10.f), 0, 5)] += 1;
				}
				LogRoundFar += bFarEdge ? 1 : 0;
#endif
			}
			Last = { Now, From, To, bVisible };
		}
	});
	if (bHudInstances)
	{
		Crowd->HudInstances.store(HudInstances, std::memory_order_relaxed);
	}
	// 오래된 기록을 지운다(5초마다). 왕복 창(2초)과 최소 유지 시간 중 긴 것보다 오래된 기록은 다시 읽히지 않는다
	if (Now >= NextPruneTime)
	{
		NextPruneTime = Now + 5.;
		const double Keep = FMath::Max(RoundTripSec, FLabConfig::Get().RepHoldMs / 1000.);
		for (auto MapIt = LastSwitch.CreateIterator(); MapIt; ++MapIt)
		{
			if (Now - MapIt.Value().Time > Keep) { MapIt.RemoveCurrent(); }
		}
		for (auto MapIt = ShadowLast.CreateIterator(); MapIt; ++MapIt)
		{
			if (Now - MapIt.Value().Time > RoundTripSec) { MapIt.RemoveCurrent(); }
		}
	}
	CSV_CUSTOM_STAT(Lab, SwNoneToInst, NoneToInst, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SwInstToActor, InstToActor, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SwActorToInst, ActorToInst, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SwInstToNone, InstToNone, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SwOther, Other, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SwRound, Round, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SwShadowOff, ShadowOff, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SwShadowOn, ShadowOn, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SwRoundActor, RoundActor, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SwRoundFar, RoundFar, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, SwRoundShadow, RoundShadow, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, RepCapOn, bCapOn ? 1 : 0, ECsvCustomStatOp::Set);

#if !UE_BUILD_TEST
	// 개발 확인: 10초마다 경계별 전환 수와 액터 경계 왕복의 원인
	const int32 Sw[8] = { NoneToInst, InstToActor, ActorToInst, InstToNone, Other, ShadowOff, ShadowOn, Round };
	for (int32 i = 0; i < 8; ++i) { LogSw[i] += Sw[i]; }
	LogRoundShadow += RoundShadow;
	LogCapFrames += bCapOn ? 1 : 0;
	++LogFrames;
	if (LogSince <= 0.)
	{
		LogSince = Now;
	}
	else if (Now - LogSince >= 10.)
	{
		const double Span = Now - LogSince;
		UE_LOG(LogLab, Log, TEXT("lab_repsw t=%.1f per_s none>inst=%.1f inst>actor=%.1f actor>inst=%.1f inst>none=%.1f other=%.1f shadow_off=%.1f shadow_on=%.1f round=%.2f round_actor_vis=%.2f round_actor_dist=%.2f round_actor_cap=%.2f round_far=%.2f round_shadow=%.2f cap_frames=%.2f round_actor_dist_m=%lld/%lld/%lld/%lld/%lld/%lld map=%d"),
			Now, LogSw[0] / Span, LogSw[1] / Span, LogSw[2] / Span, LogSw[3] / Span, LogSw[4] / Span, LogSw[5] / Span, LogSw[6] / Span, LogSw[7] / Span,
			LogRoundActorVis / Span, LogRoundActorDist / Span, LogRoundActorCap / Span, LogRoundFar / Span, LogRoundShadow / Span, double(LogCapFrames) / FMath::Max<int64>(LogFrames, 1),
			LogRoundDistHist[0], LogRoundDistHist[1], LogRoundDistHist[2], LogRoundDistHist[3], LogRoundDistHist[4], LogRoundDistHist[5], LastSwitch.Num());
		LogSince = Now;
		FMemory::Memzero(LogSw);
		FMemory::Memzero(LogRoundDistHist);
		LogRoundActorVis = LogRoundActorDist = LogRoundActorCap = LogRoundFar = LogRoundShadow = LogCapFrames = LogFrames = 0;
	}
#endif
}

// 표현 전환의 최소 유지 시간(v2 W7) ---------------------------------------------

ULabRepHoldProcessor::ULabRepHoldProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = false; // FLabConfig::UsesRepHold일 때만 게임 모듈이 켠다(HordeOptLab.cpp)
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	// 엔진 표현 LOD 프로세서(LOD 그룹) 뒤, 엔진 표현 프로세서(VisualizationProcessing) 앞. LOD 그룹 안에 두지 않아 LOD 그룹 뒤의 Lab 로직을 기다리게 하지 않는다
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::LOD);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::VisualizationProcessing);
	bRequiresGameThreadExecution = true; // 전환 기록 맵을 읽는다(ULabCrowdSubsystem::RepLastSwitch)
}

void ULabRepHoldProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassViewerInfoFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddConstSharedRequirement<FMassRepresentationParameters>();
	EntityQuery.AddConstSharedRequirement<FMassVisualizationLODParameters>();
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabPooledTag>(EMassFragmentPresence::None);
}

void ULabRepHoldProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const ULabCrowdSubsystem* Crowd = LabCrowdRead(Context);
	const UWorld* World = Context.GetWorld();
	if (!Crowd || !World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	const double HoldSec = FLabConfig::Get().RepHoldMs / 1000.;
	const TMap<FMassEntityHandle, ULabCrowdSubsystem::FLabRepSwitch>& LastSwitch = Crowd->RepLastSwitch;
	Promotions.Reset();
	HeldDemotions.Reset();
	int32 HighCount = 0; // 이번 프레임 끝의 LOD High 엔티티 수(되돌린 것 반영)
	int32 Cap = MAX_int32;
	int32 Held = 0;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const FMassRepresentationParameters& Params = Context.GetConstSharedFragment<FMassRepresentationParameters>();
		const FMassVisualizationLODParameters& LODParams = Context.GetConstSharedFragment<FMassVisualizationLODParameters>();
		Cap = FMath::Min(Cap, LODParams.LODMaxCount[EMassLOD::High]); // 두 종류가 같은 파라미터라 공유 프래그먼트 하나, 상한 하나다(engine-notes.md 19절)
		const float VisibleEdge = FMath::Max(LODParams.VisibleLODDistance[EMassLOD::Medium], 1.f);
		const float BaseEdge = FMath::Max(LODParams.BaseLODDistance[EMassLOD::Medium], 1.f);
		const TArrayView<FMassRepresentationLODFragment> LODs = Context.GetMutableFragmentView<FMassRepresentationLODFragment>();
		const TConstArrayView<FMassRepresentationFragment> Representations = Context.GetFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FMassViewerInfoFragment> Viewers = Context.GetFragmentView<FMassViewerInfoFragment>();
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FMassRepresentationLODFragment& L = LODs[It];
			const FMassRepresentationFragment& R = Representations[It];
			const bool bHigh = L.LOD == EMassLOD::High;
			HighCount += bHigh ? 1 : 0;
			const uint8 Wanted = ULabRepSwitchCountProcessor::ClassOf(Params.LODRepresentation[FMath::Min(int32(L.LOD), int32(EMassLOD::Off))]);
			const uint8 Current = ULabRepSwitchCountProcessor::ClassOf(R.CurrentRepresentation);
			if (Wanted == Current || R.IsWaitingForActorVisualReadiness() || R.ActorSpawnRequestHandle.IsValid() || R.bIsPendingDestruction)
			{
				continue; // 바뀌지 않거나 엔진이 액터 준비 중이다(그 흐름은 엔진에 맡긴다)
			}
			// 지난 프레임의 LOD는 지금 표현과 같은 무리다(표현 프로세서가 그 LOD로 표현을 정했다). 없거나 High면 Medium으로 되돌린다
			const uint8 Restore = (L.PrevLOD != EMassLOD::High && L.PrevLOD < EMassLOD::Max) ? uint8(L.PrevLOD) : uint8(EMassLOD::Medium);
			const float Edge = L.Visibility == EMassVisibility::CanBeSeen ? VisibleEdge : BaseEdge;
			const float Key = FMath::Sqrt(Viewers[It].ClosestViewerDistanceSq) / Edge;
			const ULabCrowdSubsystem::FLabRepSwitch* Last = LastSwitch.Find(Context.GetEntity(It));
			const bool bReverse = Last && Now - Last->Time < HoldSec && Last->From == Wanted && Last->To == Current;
			if (!bReverse)
			{
				if (Wanted == 0)
				{
					Promotions.Add({ &L, Restore, Key });
				}
				continue;
			}
			++Held;
			if (Current == 0)
			{
				// 액터 → 인스턴스(또는 없음)를 막는다: 이번 프레임도 High로 남긴다
				HeldDemotions.Add({ &L, uint8(L.LOD), Key });
				L.LOD = EMassLOD::High;
				++HighCount;
			}
			else
			{
				if (bHigh) { --HighCount; }
				L.LOD = EMassLOD::Type(Restore);
			}
		}
	});
	// 액터 상한: 남긴 액터 때문에 상한을 넘으면 새 액터를 먼 순으로 막고, 그래도 넘으면 남긴 액터를 먼 순으로 놓아 준다
	int32 CapBlocked = 0;
	int32 Excess = HighCount - Cap;
	if (Excess > 0)
	{
		Promotions.Sort([](const FCandidate& A, const FCandidate& B) { return A.Key > B.Key; });
		for (int32 i = 0; i < Promotions.Num() && Excess > 0; ++i, --Excess)
		{
			Promotions[i].LOD->LOD = EMassLOD::Type(Promotions[i].Restore);
			++CapBlocked;
		}
		HeldDemotions.Sort([](const FCandidate& A, const FCandidate& B) { return A.Key > B.Key; });
		for (int32 i = 0; i < HeldDemotions.Num() && Excess > 0; ++i, --Excess)
		{
			HeldDemotions[i].LOD->LOD = EMassLOD::Type(HeldDemotions[i].Restore);
			--Held;
		}
	}
	CSV_CUSTOM_STAT(Lab, RepHeld, Held, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, RepCapBlocked, CapBlocked, ECsvCustomStatOp::Set);
}

// 상태 타이머 ----------------------------------------------------------------

ULabMinionStateProcessor::ULabMinionStateProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = LabLogicGroupName;
	ExecutionOrder.ExecuteAfter.Add(ULabOrbSnipeProcessor::StaticClass()->GetFName());
}

void ULabMinionStateProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadWrite); // 재생성을 맡긴다(지연 커맨드, 게임 스레드)
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadWrite); // 피격 → 사망 전환
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);     // 개발용 진단 로그(제거 위치). 쿼리 밖 읽기를 없애려고 선언한다
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabHitTag>(EMassFragmentPresence::Any);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::Any);
	if (FLabConfig::Get().bEntityPool)
	{
		EntityQuery.AddTagRequirement<FLabPooledTag>(EMassFragmentPresence::None); // M5 엔티티 풀링: 풀에서 기다리는 엔티티(사망 태그가 남아 있다)는 타이머를 돌리지 않는다
	}
	// M4에서도 청크 필터를 걸지 않는다. 피격·사망 타이머를 Medium·Low 간격으로 돌리면 피격이 끝난 시각과
	// 다음 틱 사이만큼 마지막 자세로 멈춘다(피격 0.667초, 틱은 저격 뒤 0.53초와 1.03초라 0.38초. Docs/Worklog/M4.md "사망 전환 지연").
	// 대상은 저격당한 개체(2초에 5건)와 사망 중인 개체뿐이라 매 프레임 돌려도 수백 개체다. 저격 프로세서에 필터를 걸지 않은 것과 같은 이유다:
	// 줄이는 것은 걷는 개체가 스스로 하는 갱신(이동 목표, 범위 피해)이고, 사건으로 시작된 짧은 상태는 줄이지 않는다.
}

void ULabMinionStateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowdWrite(Context);
	if (!Crowd)
	{
		return;
	}
	const float DeathDuration = Crowd->DeathDuration;
	const float HitDuration = Crowd->HitDuration;
	const float Now = Crowd->GetWorld()->GetTimeSeconds();
	const bool bSimLod = FLabConfig::Get().UsesSimLod();
	TArray<FMassEntityHandle> Finished;
	int32 FinishedByType[LabUnitTypeMax] = { 0, 0 }; // 파괴 경로의 종류별 수(M6 수정 A가 같은 종류의 바깥 기록을 꺼낸다)
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const bool bDead = Context.DoesArchetypeHaveTag(*FLabDeadTag::StaticStruct());
		const TArrayView<FLabMinionFragment> Minions = Context.GetMutableFragmentView<FLabMinionFragment>();
		const TArrayView<FLabAnimStateFragment> AnimStates = Context.GetMutableFragmentView<FLabAnimStateFragment>();
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const float DeltaSeconds = Context.GetDeltaTimeSeconds();
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FLabMinionFragment& Minion = Minions[It];
			float DeadStart = Now; // 피격이 끝나 사망이 시작된 시각
			if (bSimLod)
			{
				// M4부터: 절대 시각으로 판정한다. 청크가 늦게 돌아도 상태가 끝난 시각은 "시작 + 길이"로 정확히 안다.
				// 엔진 누적 델타는 청크를 옮긴 직후 이전 상태의 시간이 섞여(MassLODTickRateController.h:157-171) 타이머에 쓰지 않는다.
				const float EndTime = AnimStates[It].StateStartTime + (bDead ? DeathDuration : HitDuration);
				Minion.StateTimeLeft = EndTime - Now;
				if (Minion.StateTimeLeft > 0.f)
				{
					continue;
				}
				DeadStart = EndTime; // 늦게 돈 만큼 죽기 클립과 제거 시각을 앞당겨 원래 일정에 맞춘다
			}
			else
			{
				Minion.StateTimeLeft -= DeltaSeconds;
				if (Minion.StateTimeLeft > 0.f)
				{
					continue;
				}
			}
			const FMassEntityHandle Entity = Context.GetEntity(It);
			if (bDead)
			{
				Finished.Add(Entity);
				++FinishedByType[FLabUnitTypes::TypeOf(Minion.Serial)];
#if !UE_BUILD_TEST
				LabLogScreenEvent(TEXT("removed"), Crowd->SnipeView, Transforms[It].GetTransform().GetLocation()); // 개발용 진단: 사망 후 제거 위치
#endif
			}
			else
			{
				// 로직 시간과 표현 상태를 같은 자리에서 바꾼다. 표현은 태그가 아니라 AnimStates를 보므로 태그가 다음 플러시에 바뀌어도 한 프레임 튐이 없다(plan.md 코드 정비 3).
				Minion.StateTimeLeft = DeathDuration;
				AnimStates[It].Set(ELabMinionState::Dead, DeadStart);
				Context.Defer().RemoveTag<FLabHitTag>(Entity);
				Context.Defer().AddTag<FLabDeadTag>(Entity);
#if !UE_BUILD_TEST
				LabLogScreenEvent(TEXT("die"), Crowd->SnipeView, Transforms[It].GetTransform().GetLocation(), Now - DeadStart); // 개발용 진단: 피격이 끝난 시각보다 늦게 넘어간 시간
#endif
			}
		}
	});

	if (Finished.Num() > 0 && FLabConfig::Get().bEntityPool)
	{
		// M5 엔티티 풀링(plan.md "M5" 결정 1): 파괴하지 않고 풀에 넣고 재생성한다. 파괴 경로와 같은 커맨드 종류(생성)라 플러시 안에서 같은 차례에 돈다.
		// 재생성 위치와 일련번호는 파괴 경로와 같은 스포너 스트림에서 나온다(ULabCrowdSubsystem::RecycleAndRespawn).
		TWeakObjectPtr<ULabCrowdSubsystem> WeakCrowd(Crowd);
		Context.Defer().PushCommand<FMassDeferredCreateCommand>([WeakCrowd, Finished = MoveTemp(Finished)](FMassEntityManager&)
		{
			if (ULabCrowdSubsystem* C = WeakCrowd.Get())
			{
				C->RecycleAndRespawn(Finished);
			}
		});
	}
	else if (Finished.Num() > 0)
	{
		// 지연 파괴 + 재생성(plan.md "M1"). 재생성 위치와 일련번호는 M0과 같이 스포너의 시드 스트림에서 나온다.
		const int32 Small = FinishedByType[LabUnitSmall];
		const int32 Large = FinishedByType[LabUnitLarge];
		Context.Defer().DestroyEntities(MoveTemp(Finished));
		TWeakObjectPtr<ULabCrowdSubsystem> WeakCrowd(Crowd);
		Context.Defer().PushCommand<FMassDeferredCreateCommand>([WeakCrowd, Small, Large](FMassEntityManager&)
		{
			if (ULabCrowdSubsystem* C = WeakCrowd.Get())
			{
				const int32 CountByType[LabUnitTypeMax] = { Small, Large };
				C->RequestRespawn(CountByType);
			}
		});
	}
}
