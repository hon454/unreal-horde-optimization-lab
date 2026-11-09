#include "LabMassProcessors.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabMassTypes.h"
#include "LabSnipe.h"
#include "LabVatProcessor.h"
#include "MassCommands.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "Engine/World.h"

namespace
{
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
}

void ULabApproachProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowd(Context);
	if (!Crowd)
	{
		return;
	}
	const FVector Orb = Crowd->OrbCenter;
	EntityQuery.ForEachEntityChunk(Context, [Orb](FMassExecutionContext& Context)
	{
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
}

void ULabOrbContactProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowd(Context);
	if (!Crowd)
	{
		return;
	}
	{
		FScopeLock Lock(&Crowd->ResultLock);
		Crowd->bHasNearest = false;
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
	const float Now = Crowd->GetWorld()->GetTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FLabHealthFragment> Healths = Context.GetMutableFragmentView<FLabHealthFragment>();
		const TArrayView<FLabMinionFragment> Minions = Context.GetMutableFragmentView<FLabMinionFragment>();
		const TArrayView<FLabAnimStateFragment> AnimStates = Context.GetMutableFragmentView<FLabAnimStateFragment>();
		const float DeltaSeconds = Context.GetDeltaTimeSeconds();

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

	// M0 ALabOrb::Snipe와 같은 선택: 화면 안의 산 개체를 거리 구간별로 나눠 구간마다 해시가 가장 작은 개체(LabSnipe.h).
	TLabSnipePicker<FMassEntityHandle> Picker;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
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
			Picker.Offer(Band, HashCombineFast(::GetTypeHash(Minions[It].Serial), ::GetTypeHash(SnipeTick)), Context.GetEntity(It), Location);
		}
	});

	const float HitDuration = Crowd->HitDuration;
	const float Now = Crowd->GetWorld()->GetTimeSeconds();
	const bool bOld = LabUseOldQueries();
	FScopeLock Lock(&Crowd->ResultLock);
	Picker.ForEach([&](const TLabSnipePicker<FMassEntityHandle>::FPick& Pick)
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
}

void ULabMinionStateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowd(Context);
	if (!Crowd)
	{
		return;
	}
	const float DeathDuration = Crowd->DeathDuration;
	const float Now = Crowd->GetWorld()->GetTimeSeconds();
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
			Minion.StateTimeLeft -= DeltaSeconds;
			if (Minion.StateTimeLeft > 0.f)
			{
				continue;
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
				AnimStates[It].Set(ELabMinionState::Dead, Now);
				Context.Defer().RemoveTag<FLabHitTag>(Entity);
				Context.Defer().AddTag<FLabDeadTag>(Entity);
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
