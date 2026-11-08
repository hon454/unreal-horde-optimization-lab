#include "LabMassProcessors.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabMassTypes.h"
#include "LabSnipe.h"
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
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
}

void ULabOrbContactProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadWrite);
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
	const float ZoneRadiusSq = FMath::Square(FLabScenario::ZoneRadius);
	const float DeathDuration = Crowd->DeathDuration;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FLabHealthFragment> Healths = Context.GetMutableFragmentView<FLabHealthFragment>();
		const TArrayView<FLabMinionFragment> Minions = Context.GetMutableFragmentView<FLabMinionFragment>();
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
	ExecutionOrder.ExecuteAfter.Add(ULabOrbContactProcessor::StaticClass()->GetFName());
}

void ULabOrbSnipeProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabHealthFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FLabMinionTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FLabHitTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FLabDeadTag>(EMassFragmentPresence::None);
}

void ULabOrbSnipeProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ULabCrowdSubsystem* Crowd = LabCrowd(Context);
	if (!Crowd || Crowd->PendingSnipeTick == 0)
	{
		return;
	}
	const uint32 SnipeTick = Crowd->PendingSnipeTick;
	Crowd->PendingSnipeTick = 0;
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
	FScopeLock Lock(&Crowd->ResultLock);
	Picker.ForEach([&](const TLabSnipePicker<FMassEntityHandle>::FPick& Pick)
	{
		EntityManager.GetFragmentDataChecked<FLabMinionFragment>(Pick.Target).StateTimeLeft = HitDuration;
		Context.Defer().AddTag<FLabHitTag>(Pick.Target);
		StopMoving(Context, Pick.Target);
		Crowd->SnipeTargets.Add(FVector(Pick.Location.X, Pick.Location.Y, LabSnipeAimHeight)); // M0과 같은 몸통 높이
		++Crowd->SnipeCount;
	});
}

// 상태 타이머 ----------------------------------------------------------------

ULabMinionStateProcessor::ULabMinionStateProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteAfter.Add(ULabOrbSnipeProcessor::StaticClass()->GetFName());
}

void ULabMinionStateProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadWrite);
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
	TArray<FMassEntityHandle> Finished;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const bool bDead = Context.DoesArchetypeHaveTag(*FLabDeadTag::StaticStruct());
		const TArrayView<FLabMinionFragment> Minions = Context.GetMutableFragmentView<FLabMinionFragment>();
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
			}
			else
			{
				Minion.StateTimeLeft = DeathDuration;
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
	ExecutionOrder.ExecuteAfter.Add(ULabMinionStateProcessor::StaticClass()->GetFName());
}

void ULabDamageGenProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FLabHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadOnly);
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
	// 개체마다 해시로 확률 Wanted / 산 개체 수에 뽑는다. 기대 건수가 Wanted다.
	const uint32 Threshold = static_cast<uint32>(FMath::Min(1.0, double(Wanted) / Crowd->GetAliveCount()) * double(MAX_uint32));
	const uint32 Frame = static_cast<uint32>(GFrameCounter);

	TArray<FMassEntityHandle> Targets;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TArrayView<FLabHealthFragment> Healths = Context.GetMutableFragmentView<FLabHealthFragment>();
		const TConstArrayView<FLabMinionFragment> Minions = Context.GetFragmentView<FLabMinionFragment>();
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			if (MurmurFinalize32(HashCombineFast(::GetTypeHash(Minions[It].Serial), ::GetTypeHash(Frame))) > Threshold) // 섞지 않은 HashCombineFast는 작은 입력에서 고르게 퍼지지 않는다
			{
				continue;
			}
			if (bEvent)
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
