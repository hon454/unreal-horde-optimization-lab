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
#include "MassNavigationFragments.h"
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

	/** 청크의 Simulation LOD(0 High ~ 3 Off, 아직 정해지지 않았으면 4). 진단 배열의 색인으로 쓴다 */
	int32 LabChunkLod(const FMassExecutionContext& Context)
	{
		return FMath::Clamp(int32(FMassSimulationVariableTickChunkFragment::GetChunkLOD(Context)), 0, 4);
	}

	ULabCrowdSubsystem* LabCrowd(FMassExecutionContext& Context)
	{
		UWorld* World = Context.GetWorld();
		ULabCrowdSubsystem* Crowd = World ? World->GetSubsystem<ULabCrowdSubsystem>() : nullptr;
		return Crowd && Crowd->IsActive() ? Crowd : nullptr;
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
}

void ULabApproachProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
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
	ULabCrowdSubsystem* Crowd = LabCrowd(Context);
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

	// 밴드별 걷는 개체 수(필터 없는 같은 쿼리, 청크의 개체 수만 읽는다). 실제 틱 주기 = 평균 프레임 시간 × SimBand ÷ SimTick(2026-11-10 사용자 승인, Docs/Worklog/M4.md).
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

// 범위 피해 ------------------------------------------------------------------

ULabOrbContactProcessor::ULabOrbContactProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = LabLogicGroupName;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
}

void ULabOrbContactProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
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
	ULabCrowdSubsystem* Crowd = LabCrowd(Context);
	if (!Crowd)
	{
		return;
	}
	const float Now = Crowd->GetWorld()->GetTimeSeconds();
	const bool bSimLod = FLabConfig::Get().UsesSimLod();
	const FLabSimLod Sim = FLabConfig::Get().SimLod();
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
	const ELabVariant Variant = FLabConfig::Get().Variant;
	if (Variant == ELabVariant::VatDie0 || Variant == ELabVariant::VatDie1k)
	{
		return; // E2 (d): 범위 피해로 죽지 않게 한다(사망은 발생기만 만든다)
	}
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
				Crowd->NearestHoldUntil = Now + (Lod == EMassLOD::Medium ? Sim.TickMedium : Lod == EMassLOD::Low ? Sim.TickLow : 0.f) * 1.2f; // 엔진이 간격에 ±10%를 더한다(MassLODTickRateController.h:143)
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
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabHealthFragment>(EMassFragmentAccess::ReadOnly);
	// 순회에서는 Serial만 읽지만 고른 대상의 StateTimeLeft와 표현 상태를 바꾸므로 쓰기로 선언한다(Shabel 12:32, talk-notes.md).
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FLabUnitTypeParams>(EMassFragmentPresence::All); // 2종: 청크의 종류(대형 예약 발, LabSnipe.h E안)
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabHitTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::None);
}

void ULabOrbSnipeProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowd(Context);
	if (!Crowd || Crowd->PendingSnipeTick.load() == 0)
	{
		return;
	}
	const uint32 SnipeTick = Crowd->PendingSnipeTick.exchange(0); // 오브 틱이 쓴 값을 한 번만 가져간다(std::atomic, M3b 정비)
	if (SnipeTick == 0)
	{
		return;
	}
	if (FLabConfig::Get().Variant == ELabVariant::VatDie0 || FLabConfig::Get().Variant == ELabVariant::VatDie1k)
	{
		return; // E2 (d): 저격도 끈다(사망은 발생기만 만든다)
	}
	const FVector Center = Crowd->OrbCenter;
	const FLabSnipeView View = Crowd->SnipeView;
	if (!View.bValid)
	{
		return;
	}

	// M0 ALabOrb::Snipe와 같은 선택: 화면 안의 산 개체를 거리 구간별로 나눠 구간마다 해시가 가장 작은 개체(LabSnipe.h). 2종은 예약 발만 대형(E안).
	TLabSnipeSelector<FMassEntityHandle> Picker(SnipeTick);
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const bool bLarge = Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType == LabUnitLarge; // 종류는 청크 단위(아키타입)
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
			if (Band == INDEX_NONE || !View.Contains(FVector(Location.X, Location.Y, LabSnipeAimHeight)))
			{
				continue;
			}
			Picker.Offer(Band, HashCombineFast(::GetTypeHash(Minions[It].Serial), ::GetTypeHash(SnipeTick)), Context.GetEntity(It), Location, bLarge);
		}
	});

	const float HitDuration = Crowd->HitDuration;
	const float Now = Crowd->GetWorld()->GetTimeSeconds();
	const bool bOld = LabUseOldQueries();
	FScopeLock Lock(&Crowd->ResultLock);
	Picker.ForEach([&](const TLabSnipeSelector<FMassEntityHandle>::FPick& Pick)
	{
		const FMassEntityHandle Target = Pick.Target;
		if (bOld)
		{
			// m3qold: 정비 전 쿼리 밖 쓰기. 다른 청크의 메모리를 순회 밖에서 바로 고친다(plan.md 코드 정비 1).
			EntityManager.GetFragmentDataChecked<FLabMinionFragment>(Target).StateTimeLeft = HitDuration;
			EntityManager.GetFragmentDataChecked<FLabAnimStateFragment>(Target).Set(ELabMinionState::Hit, Now);
		}
		else
		{
			// 대상은 순회가 끝난 뒤에야 정해지므로 쿼리 안에서 쓸 수 없다. 태그 추가와 같은 플러시에서 지연 명령으로 쓴다(2초에 5건).
			Context.Defer().PushCommand<FMassDeferredSetCommand>([Target, HitDuration, Now](FMassEntityManager& Manager)
			{
				if (Manager.IsEntityValid(Target))
				{
					Manager.GetFragmentDataChecked<FLabMinionFragment>(Target).StateTimeLeft = HitDuration;
					Manager.GetFragmentDataChecked<FLabAnimStateFragment>(Target).Set(ELabMinionState::Hit, Now);
				}
			});
		}
		Context.Defer().AddTag<FLabHitTag>(Target);
		StopMoving(Context, Target);
#if !UE_BUILD_TEST
		LabLogScreenEvent(Picker.bHasLarge && Pick.Target == Picker.LargeBest.Target ? TEXT("snipe_large") : TEXT("snipe_small"), View, Pick.Location); // 개발용 진단: 피격 멈춤 확인
#endif
		Crowd->SnipeTargets.Add(FVector(Pick.Location.X, Pick.Location.Y, LabSnipeAimHeight)); // M0과 같은 몸통 높이
		++Crowd->SnipeCount;
	});
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
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadWrite); // 피격 → 사망 전환
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);     // 개발용 진단 로그(제거 위치). 쿼리 밖 읽기를 없애려고 선언한다
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabHitTag>(EMassFragmentPresence::Any);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::Any);
	if (FLabConfig::Get().UsesSimLod())
	{
		LabUseVariableTick(EntityQuery);
	}
}

void ULabMinionStateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowd(Context);
	if (!Crowd)
	{
		return;
	}
	const float DeathDuration = Crowd->DeathDuration;
	const float HitDuration = Crowd->HitDuration;
	const float Now = Crowd->GetWorld()->GetTimeSeconds();
	const bool bSimLod = FLabConfig::Get().UsesSimLod();
	TArray<FMassEntityHandle> Finished;
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
				// M4부터: 절대 시각으로 판정한다(2026-11-10 사용자 결정). 청크가 늦게 돌아도 상태가 끝난 시각은 "시작 + 길이"로 정확히 안다.
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

	if (Finished.Num() > 0)
	{
		// 지연 파괴 + 재생성(plan.md "M1"). 재생성 위치와 일련번호는 M0과 같이 스포너의 시드 스트림에서 나온다.
		const int32 Count = Finished.Num();
		Context.Defer().DestroyEntities(MoveTemp(Finished));
		TWeakObjectPtr<ULabCrowdSubsystem> WeakCrowd(Crowd);
		Context.Defer().PushCommand<FMassDeferredCreateCommand>([WeakCrowd, Count](FMassEntityManager&)
		{
			if (ULabCrowdSubsystem* C = WeakCrowd.Get())
			{
				C->RequestRespawn(Count);
			}
		});
	}
}

// E1 (d) 피해 적용 방식 -----------------------------------------------------

namespace
{
	/** 변형에서 초당 건수와 방식을 읽는다. 0이면 E1 (d) 모드가 아니다. */
	int32 LabDamageRate(bool& bOutEvent)
	{
		switch (FLabConfig::Get().Variant)
		{
		case ELabVariant::MassDirLo: bOutEvent = false; return 1000;
		case ELabVariant::MassDirHi: bOutEvent = false; return 10000;
		case ELabVariant::MassEvtLo: bOutEvent = true; return 1000;
		case ELabVariant::MassEvtHi: bOutEvent = true; return 10000;
		case ELabVariant::VatDie1k: bOutEvent = false; return 1000; // E2 (d) 첫 설계: 치명(피격→사망)
		case ELabVariant::Kill1k: bOutEvent = false; return 1000;   // E2 (d): 치명(피격→사망), 평소 시나리오 위에 더한다
		default: bOutEvent = false; return 0;
		}
	}
	constexpr float LabNonLethalDamage = 1.f; // 한 건의 피해. 체력 100에서 최소 1까지만 깎는다
	constexpr float LabNonLethalFloor = 1.f;
}

ULabDamageGenProcessor::ULabDamageGenProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = LabLogicGroupName;
	ExecutionOrder.ExecuteAfter.Add(ULabMinionStateProcessor::StaticClass()->GetFName());
}

void ULabDamageGenProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FLabHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadWrite); // E2 (d) 치명 모드가 피격 시간을 쓴다
	EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadWrite); // 치명 모드의 피격 전환
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabHitTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::None);
}

void ULabDamageGenProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	bool bEvent = false;
	const int32 Rate = LabDamageRate(bEvent);
	ULabCrowdSubsystem* Crowd = LabCrowd(Context);
	if (Rate == 0 || !Crowd || Crowd->GetAliveCount() <= 0)
	{
		return;
	}
	Owed += Rate * Context.GetDeltaTimeSeconds();
	const int32 Wanted = FMath::FloorToInt(Owed);
	if (Wanted <= 0)
	{
		return;
	}
	Owed -= Wanted;
	const bool bLethal = FLabConfig::Get().Variant == ELabVariant::VatDie1k || FLabConfig::Get().Variant == ELabVariant::Kill1k;
	// 개체마다 해시로 확률 Wanted / 산 개체 수에 뽑는다. 기대 건수가 Wanted다.
	// 치명 모드(E2 (d))는 피격·사망 중인 개체가 많아(초당 1천 x 약 2.9초) 걷는 개체 수로 나눠야 기대 건수가 맞는다.
	const int32 Pool = bLethal ? FMath::Max(1, EntityQuery.GetNumMatchingEntities(EntityManager)) : Crowd->GetAliveCount();
	const uint32 Threshold = static_cast<uint32>(FMath::Min(1.0, double(Wanted) / Pool) * double(MAX_uint32));
	const uint32 Frame = static_cast<uint32>(GFrameCounter);
	const float HitDuration = Crowd->HitDuration;
	const float Now = Crowd->GetWorld()->GetTimeSeconds();

	TArray<FMassEntityHandle> Targets;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TArrayView<FLabHealthFragment> Healths = Context.GetMutableFragmentView<FLabHealthFragment>();
		const TArrayView<FLabMinionFragment> Minions = Context.GetMutableFragmentView<FLabMinionFragment>();
		const TArrayView<FLabAnimStateFragment> AnimStates = Context.GetMutableFragmentView<FLabAnimStateFragment>();
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			if (MurmurFinalize32(HashCombineFast(::GetTypeHash(Minions[It].Serial), ::GetTypeHash(Frame))) > Threshold) // 섞지 않은 HashCombineFast는 작은 입력에서 고르게 퍼지지 않는다
			{
				continue;
			}
			if (bLethal)
			{
				// 저격과 같은 경로: 피격 상태로 바꾸고 멈춘다. 피격이 끝나면 상태 프로세서가 사망 → 제거 → 재생성한다.
				const FMassEntityHandle Entity = Context.GetEntity(It);
				Minions[It].StateTimeLeft = HitDuration;
				AnimStates[It].Set(ELabMinionState::Hit, Now);
				Context.Defer().AddTag<FLabHitTag>(Entity);
				StopMoving(Context, Entity);
				++Crowd->DamageHits;
			}
			else if (bEvent)
			{
				Targets.Add(Context.GetEntity(It));
			}
			else
			{
				FLabHealthFragment& Health = Healths[It];
				Health.Health = FMath::Max(LabNonLethalFloor, Health.Health - LabNonLethalDamage);
				++Crowd->DamageHits;
			}
		}
	});

	if (bEvent && Targets.Num() > 0)
	{
		// 피해 이벤트 엔티티는 처리 단계가 끝난 뒤(커맨드 플러시) 한꺼번에 만든다.
		Context.Defer().PushCommand<FMassDeferredCreateCommand>([Targets = MoveTemp(Targets)](FMassEntityManager& Manager)
		{
			static FMassArchetypeHandle Archetype;
			if (!Archetype.IsValid())
			{
				Archetype = Manager.CreateArchetype({ FLabDamageEventFragment::StaticStruct() });
			}
			TArray<FMassEntityHandle> Events;
			TSharedRef<FMassEntityManager::FEntityCreationContext> Creation = Manager.BatchCreateEntities(Archetype, Targets.Num(), Events);
			for (int32 i = 0; i < Events.Num(); ++i)
			{
				FLabDamageEventFragment& Event = Manager.GetFragmentDataChecked<FLabDamageEventFragment>(Events[i]);
				Event.Target = Targets[i];
				Event.Amount = LabNonLethalDamage;
			}
		});
	}
}

ULabDamageApplyProcessor::ULabDamageApplyProcessor()
	: EventQuery(*this)
	, HealthQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = LabLogicGroupName;
	ExecutionOrder.ExecuteAfter.Add(ULabDamageGenProcessor::StaticClass()->GetFName());
}

void ULabDamageApplyProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EventQuery.AddRequirement<FLabDamageEventFragment>(EMassFragmentAccess::ReadOnly);
	HealthQuery.AddRequirement<FLabHealthFragment>(EMassFragmentAccess::ReadWrite);
	HealthQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
}

void ULabDamageApplyProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	TArray<FMassEntityHandle> Consumed;
	EventQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FLabDamageEventFragment> Events = Context.GetFragmentView<FLabDamageEventFragment>();
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			const FLabDamageEventFragment& Event = Events[It];
			// 대상이 그사이 지워졌으면(사망·재생성) 건너뛴다.
			if (EntityManager.IsEntityValid(Event.Target))
			{
				if (FLabHealthFragment* Health = EntityManager.GetFragmentDataPtr<FLabHealthFragment>(Event.Target))
				{
					Health->Health = FMath::Max(LabNonLethalFloor, Health->Health - Event.Amount);
					if (ULabCrowdSubsystem* Crowd = LabCrowd(Context)) { ++Crowd->DamageHits; }
				}
			}
			Consumed.Add(Context.GetEntity(It));
		}
	});
	if (Consumed.Num() > 0)
	{
		Context.Defer().DestroyEntities(MoveTemp(Consumed));
	}
}
