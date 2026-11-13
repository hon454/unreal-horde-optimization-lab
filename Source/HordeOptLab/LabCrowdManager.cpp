#include "LabCrowdManager.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSpawner.h"
#include "LabMassTypes.h" // LabWalkPhase
#include "LabMinion.h"    // LabExcludeFromLumen
#include "LabOrb.h"
#include "LabSnipe.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "MassNavigationUtils.h"                               // GetYawFromDirection, LerpAngle, ExponentialSmoothingAngle(엔진 회전 프로세서가 쓰는 함수 그대로)
#include "Steering/MassSteeringFragments.h"                    // FMassMovingSteeringParameters(엔진 기본값)
#include "SmoothOrientation/MassSmoothOrientationFragments.h"  // FMassSmoothOrientationParameters(엔진 기본값)
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

namespace
{
	/**
	 * 엔진 조향의 방향별 속도 배율. 엔진 파일 안의 static 함수라 부를 수 없어 식을 옮겼다.
	 * 출처: Engine/Plugins/AI/MassAI/Source/MassNavigation/Private/MassSteeringProcessors.cpp:26-45 (UE::MassNavigation::CalcDirectionalSpeedScale)
	 */
	double LabDirectionalSpeedScale(const FVector& ForwardDirection, const FVector& SteerDirection)
	{
		constexpr double ForwardSpeedScale = 1.;
		constexpr double BackwardSpeedScale = 0.25;
		constexpr double SideSpeedScale = 0.5;
		const FVector LeftDirection = FVector::CrossProduct(ForwardDirection, FVector::UpVector);
		const double DirX = FVector::DotProduct(LeftDirection, SteerDirection);
		const double DirY = FVector::DotProduct(ForwardDirection, SteerDirection);
		constexpr double SideA = SideSpeedScale;
		const double SideB = DirY > 0. ? ForwardSpeedScale : BackwardSpeedScale;
		const double Disc = FMath::Square(SideA) * FMath::Square(DirY) + FMath::Square(SideB) * FMath::Square(DirX);
		return Disc > SMALL_NUMBER ? SideA * SideB / FMath::Sqrt(Disc) : 0.;
	}
}

ALabManagedMinion::ALabManagedMinion()
{
	PrimaryActorTick.bCanEverTick = false; // 틱 없음. 컴포넌트도 만들지 않는다
}

ALabCrowdManager::ALabCrowdManager()
{
	PrimaryActorTick.bCanEverTick = true; // 이 변형의 틱은 이것 하나다(TG_PrePhysics)
	// 엔진의 Mass 비주얼라이저 액터와 같은 루트(AMassVisualizer, MassVisualizer.cpp:10-11)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
	RootComponent->SetMobility(EComponentMobility::Static);
}

void ALabCrowdManager::Activate(ALabCrowdSpawner* InSpawner, const FVector& InOrbCenter, TConstArrayView<UStaticMesh*> MeshesByType, float InHitDuration, float InDeathDuration)
{
	Spawner = InSpawner;
	OrbCenter = InOrbCenter;
	HitDuration = InHitDuration;
	DeathDuration = InDeathDuration;
	Isms.Reset();
	for (int32 Type = 0; Type < MeshesByType.Num(); ++Type)
	{
		// 2단계의 ISM 컴포넌트와 같은 설정. 엔진이 메시 설명자에서 컴포넌트를 만드는 줄을 그대로 따른다(MassVisualizationComponent.cpp:238-262).
		// 설명자 값은 LabCrowdSubsystem.cpp BuildEntityConfig와 같다: 그림자 켬, Movable(설명자 기본값, MassRepresentationTypes.h:115), 머티리얼 덮어쓰기 없음.
		UInstancedStaticMeshComponent* Ism = NewObject<UInstancedStaticMeshComponent>(this);
		Ism->SetStaticMesh(MeshesByType[Type]);
		Ism->SetCullDistances(0, 1000000);
		Ism->SetupAttachment(GetRootComponent());
		Ism->SetCanEverAffectNavigation(false);
		Ism->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Ism->SetCastShadow(true);
		Ism->Mobility = EComponentMobility::Movable;
		Ism->SetReceivesDecals(false);
		Ism->RegisterComponent();
		// 엔진은 갱신 때마다 아래 둘을 부른다(:571, :588). 값이 바뀌지 않으므로 한 번만 부른다. 커스텀 데이터는 M1에 없다(0)
		Ism->SetNumCustomDataFloats(0);
		Ism->SetHasPerInstancePrevTransforms(true);
		// 2단계는 오브 틱이 ISM을 Lumen 장면에서 뺀다(ULabCrowdSubsystem::ExcludeVisualsFromLumen). 같은 설정을 만들 때 넣는다
		LabExcludeFromLumen(*Ism);
		Isms.Add(Ism);
		LocalTransforms[Type] = FTransform(FRotator(0.f, -90.f, 0.f), FVector::ZeroVector, FVector(FLabUnitAssetPaths::Get(Type).Scale));
		UE_LOG(LogLab, Log, TEXT("lab_manager_ism type=%d mesh=%s shadow=%d mobility=movable prev_transforms=1"), Type, *GetNameSafe(MeshesByType[Type]), Ism->CastShadow ? 1 : 0);
	}
}

void ALabCrowdManager::SpawnMinions(TConstArrayView<FVector> Locations, TConstArrayView<uint32> Serials)
{
	check(Locations.Num() == Serials.Num() && Isms.Num() > 0);
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Minions.Reserve(Minions.Num() + Locations.Num());
	for (int32 i = 0; i < Locations.Num(); ++i)
	{
		ALabManagedMinion* Minion = World->SpawnActor<ALabManagedMinion>(Params);
		// ULabCrowdSubsystem::SpawnEntities의 새 엔티티 초기화와 같은 값(LabCrowdSubsystem.cpp)
		const FVector Facing = (OrbCenter - Locations[i]).GetSafeNormal2D();
		Minion->Transform = FTransform(Facing.Rotation(), Locations[i]);
		Minion->PrevTransform = Minion->Transform;
		Minion->MoveForward = Facing;
		Minion->Serial = Serials[i];
		Minion->UnitType = FMath::Min(FLabUnitTypes::TypeOf(Serials[i]), Isms.Num() - 1);
		Minion->Health = FLabConfig::Get().HealthFor(Serials[i]);
		Minion->State = ELabMinionState::Walk;
		Minion->StateStartTime = -LabWalkPhase(Serials[i]);
		// 엔진이 새 엔티티를 인스턴스로 더하는 호출과 같다(MassVisualizationComponent.cpp:652-658)
		UInstancedStaticMeshComponent* Ism = Isms[Minion->UnitType];
		const FTransform InstanceTransform = LocalTransforms[Minion->UnitType] * Minion->Transform;
		Minion->InstanceId = Ism->AddInstanceById(InstanceTransform);
		Ism->SetPreviousTransformById(Minion->InstanceId, InstanceTransform);
		Minions.Add(Minion);
	}
}

void ALabCrowdManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Spawner || !Orb || Isms.Num() == 0)
	{
		return;
	}
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("LabCrowdManager Tick");
	CSV_SCOPED_TIMING_STAT(Lab, Manager);

	// 오브 상태. 2단계는 Mass PrePhysics 단계 시작에 같은 값을 찍는다(ALabOrb::OnMassPrePhysicsStart): 오브 중심, 직전 프레임에 그린 카메라, 저격 틱.
	OrbCenter = Orb->GetActorLocation();
	const FVector Center = OrbCenter;
	const FLabSnipeView View = Orb->GetSnipeView();
	const uint32 SnipeTick = Orb->AdvanceSnipeClock(DeltaSeconds);
	const bool bSnipe = SnipeTick != 0 && View.bValid;
	const float Now = GetWorld()->GetTimeSeconds();

	// 엔진 파라미터. Trait에서 바꾸지 않은 값은 엔진 구조체의 기본값을 읽는다(조향 UMassSteeringTrait, 회전 UMassSmoothOrientationTrait).
	// 이동은 ULabMovementTrait의 값이다(MaxSpeed = 이동 속도). 이동 목표의 DesiredSpeed도 이동 속도다(ULabApproachProcessor, FLabUnitTypeParams::MoveSpeed).
	const FMassMovingSteeringParameters SteeringParams;
	const FMassSmoothOrientationParameters OrientationParams;
	const double MaxSpeed = FLabScenario::MoveSpeed;
	const double TargetSpeed = FLabScenario::MoveSpeed;
	const double SteerK = 1. / SteeringParams.ReactionTime;
	const double LookAheadDistance = FMath::Max(1.0, double(SteeringParams.LookAheadTime) * TargetSpeed);
	const double OrientationRatio = OrientationParams.Moving.MoveTargetWeight / (OrientationParams.Moving.MoveTargetWeight + OrientationParams.Moving.VelocityWeight);
	// 엔진 힘·이동·회전 프로세서는 프레임 시간을 0.1초로 자른다(MassMovementProcessors.cpp:41,100, MassSmoothOrientationProcessors.cpp:47)
	const float MoveDelta = FMath::Min(0.1f, DeltaSeconds);

	const float ZoneRadiusSq = FMath::Square(FLabScenario::ZoneRadius);
	const double NearRadiusSq = FMath::Square(double(FLabEquivalence::NearOrbCm));
	int32 Kills = 0;
	int32 NearCount = 0;
	bool bHasNearest = false;
	float NearestSq = TNumericLimits<float>::Max();
	FVector Nearest = FVector::ZeroVector;
	TLabSnipeSelector<ALabManagedMinion*> Picker(SnipeTick);
	TArray<int32, TInlineAllocator<32>> Finished; // 사망이 끝난 개체의 색인(오름차순)

	for (int32 Index = 0; Index < Minions.Num(); ++Index)
	{
		ALabManagedMinion& M = *Minions[Index];
		const bool bWalk = M.State == ELabMinionState::Walk; // 순회 시작 때의 상태. 2단계의 태그는 프레임 끝 플러시에서 바뀌므로 이 프레임의 처리는 시작 상태를 따른다
		FVector Location = M.Transform.GetLocation();

		// 1. 접근(ULabApproachProcessor): 걷는 개체의 이동 목표를 오브로 둔다
		if (bWalk)
		{
			FVector ToOrb = Center - Location;
			ToOrb.Z = 0.;
			const double Distance = ToOrb.Length();
			M.MoveForward = Distance > KINDA_SMALL_NUMBER ? ToOrb / Distance : FVector::ForwardVector;
			M.DistanceToGoal = float(Distance);
		}

		// 2. 조향(UMassSteerToMoveTargetProcessor의 Move 갈래, MassSteeringProcessors.cpp:141-237). 도착 의도가 Move라 도착 감속은 없다.
		// 엔진 조향·힘·회전은 피격·사망 개체도 돈다(코드 구동 태그를 요구하지 않는다). 위치를 옮기는 것만 걷는 개체다
		{
			const FVector CurrentForward = M.Transform.GetRotation().GetForwardVector();
			const FVector TargetSide = FVector::CrossProduct(M.MoveForward, FVector::UpVector);
			const FVector Delta = Location - Center;
			const double ForwardOffset = FVector::DotProduct(M.MoveForward, Delta);
			const double SidewaysOffset = FVector::DotProduct(TargetSide, Delta);
			const double SteerForward = FMath::Sqrt(FMath::Max(0., FMath::Square(LookAheadDistance) - FMath::Square(SidewaysOffset)));
			const FVector SteerTarget = Center + M.MoveForward * FMath::Clamp(ForwardOffset + SteerForward, 0., LookAheadDistance);
			FVector SteerDirection = SteerTarget - Location;
			SteerDirection.Z = 0.;
			const double DistanceToSteerTarget = SteerDirection.Length();
			if (DistanceToSteerTarget > KINDA_SMALL_NUMBER)
			{
				SteerDirection *= 1. / DistanceToSteerTarget;
			}
			double DesiredSpeed = TargetSpeed;
			if (SteeringParams.bAllowSpeedVariance)
			{
				DesiredSpeed *= LabDirectionalSpeedScale(CurrentForward, SteerDirection);
				double CatchupDesiredSpeed = DesiredSpeed;
				if (ForwardOffset < 0.)
				{
					CatchupDesiredSpeed = FMath::Lerp(DesiredSpeed, MaxSpeed, FMath::Min(-ForwardOffset / LookAheadDistance, 1.));
				}
				else if (ForwardOffset > 0.)
				{
					const double T = FMath::Min(ForwardOffset / LookAheadDistance, 1.);
					CatchupDesiredSpeed = FMath::Lerp(DesiredSpeed, DesiredSpeed * 0., 1. - FMath::Square(1. - T));
				}
				const double DeviantSpeed = FMath::Min(FMath::Abs(SidewaysOffset) / LookAheadDistance, 1.) * DesiredSpeed;
				DesiredSpeed = FMath::Max(CatchupDesiredSpeed, DeviantSpeed);
			}
			const FVector Force = SteerK * (SteerDirection * DesiredSpeed - M.DesiredVelocity);
			// 3. 힘 적용(UMassApplyForceProcessor, MassMovementProcessors.cpp:56-57)
			M.DesiredVelocity = (M.DesiredVelocity + Force * MoveDelta).GetClampedToMaxSize(MaxSpeed);
		}

		// 4. 이동(UMassApplyMovementProcessor, MassMovementProcessors.cpp:115-128). 2단계에서 피격·사망 개체는 코드 구동 태그가 떨어져 옮겨지지 않는다
		if (bWalk)
		{
			M.Velocity = M.DesiredVelocity;
			Location += M.Velocity * MoveDelta;
			M.Transform.SetTranslation(Location);
		}

		// 5. 회전(UMassSmoothOrientationProcessor, MassSmoothOrientationProcessors.cpp:72-112). 이동 중 가중치(이동 목표 0.4, 속도 0.6)
		{
			const double CurrentHeading = UE::MassNavigation::GetYawFromDirection(M.Transform.GetRotation().GetForwardVector());
			const double VelocityHeading = UE::MassNavigation::GetYawFromDirection(M.DesiredVelocity);
			const double MovementHeading = UE::MassNavigation::GetYawFromDirection(M.MoveForward);
			const double DesiredHeading = UE::MassNavigation::LerpAngle(VelocityHeading, MovementHeading, OrientationRatio);
			const double NewHeading = UE::MassNavigation::ExponentialSmoothingAngle(CurrentHeading, DesiredHeading, MoveDelta, OrientationParams.OrientationSmoothingTime);
			M.Transform.SetRotation(FQuat(FVector::UpVector, NewHeading));
		}

		const float DistSq = FVector::DistSquared2D(Location, Center);
		if (bWalk)
		{
			// 6. 범위 피해(ULabOrbContactProcessor): 이동 뒤의 위치로 잰다. 가장자리 ZoneDpsEdge → 중심 ZoneDpsCenter
			if (DistSq <= ZoneRadiusSq)
			{
				if (DistSq < NearestSq)
				{
					NearestSq = DistSq;
					Nearest = Location;
					bHasNearest = true;
				}
				const float Closeness = 1.f - FMath::Sqrt(DistSq) / FLabScenario::ZoneRadius;
				M.Health -= FMath::Lerp(FLabScenario::ZoneDpsEdge, FLabScenario::ZoneDpsCenter, Closeness) * DeltaSeconds;
				if (M.Health <= 0.f)
				{
					M.StateTimeLeft = DeathDuration;
					M.State = ELabMinionState::Dead;
					M.StateStartTime = Now;
					++Kills;
				}
			}
			// 7. 저격 후보(ULabOrbSnipeProcessor): 화면 안의 산 개체를 거리 구간별로. 이 프레임 범위 피해로 죽은 개체는 체력으로 거른다
			if (bSnipe && M.Health > 0.f)
			{
				const int32 Band = FLabSnipeBands::BandOf(FMath::Sqrt(DistSq));
				if (Band != INDEX_NONE && View.Contains(FVector(Location.X, Location.Y, LabSnipeAimHeight)))
				{
					Picker.Offer(Band, HashCombineFast(::GetTypeHash(M.Serial), ::GetTypeHash(SnipeTick)), &M, Location, M.UnitType == LabUnitLarge);
				}
			}
		}
		else
		{
			// 8. 상태 타이머(ULabMinionStateProcessor, Simulation LOD 없는 갈래): 피격이 끝나면 사망, 사망이 끝나면 지우고 다시 만든다
			M.StateTimeLeft -= DeltaSeconds;
			if (M.StateTimeLeft <= 0.f)
			{
				if (M.State == ELabMinionState::Dead)
				{
					Finished.Add(Index);
				}
				else
				{
					M.StateTimeLeft = DeathDuration;
					M.State = ELabMinionState::Dead;
					M.StateStartTime = Now;
				}
			}
		}

		// 9. 동등성 통계(ULabNearOrbCountProcessor): 오브 수평 30m 안. 걷기·피격·사망 모두 센다
		if (double(DistSq) <= NearRadiusSq)
		{
			++NearCount;
		}

		// 10. ISM 쓰기. 엔진이 2단계에서 개체마다 부르는 두 호출과 같다(UMassUpdateISMProcessor → MassVisualizationComponent.cpp:617-621).
		// 변환은 LocalTransform * 엔티티 변환이다(FMassLODSignificanceRange::AddBatchedTransform, :923-924). 2단계처럼 멈춘 개체도 매 프레임 쓴다
		{
			UInstancedStaticMeshComponent* Ism = Isms[M.UnitType];
			const FTransform& Local = LocalTransforms[M.UnitType];
			Ism->UpdateInstanceTransformById(M.InstanceId, Local * M.Transform);
			Ism->SetPreviousTransformById(M.InstanceId, Local * M.PrevTransform);
			M.PrevTransform = M.Transform;
		}
	}
	Near30 = NearCount;

	// 저격 적용(ULabOrbSnipeProcessor): 고른 개체를 피격 상태로 바꾼다. 타이머는 다음 프레임부터 준다(2단계의 피격 태그가 플러시에서 붙는 것과 같다)
	TArray<FVector, TInlineAllocator<FLabScenario::SnipeCount>> SnipeTargets;
	if (bSnipe)
	{
		Picker.ForEach([&](const TLabSnipeSelector<ALabManagedMinion*>::FPick& Pick)
		{
			Pick.Target->StateTimeLeft = HitDuration;
			Pick.Target->State = ELabMinionState::Hit;
			Pick.Target->StateStartTime = Now;
			SnipeTargets.Add(FVector(Pick.Location.X, Pick.Location.Y, LabSnipeAimHeight));
		});
	}
	// 이펙트와 집계는 오브가 한다(모든 단계가 같은 이펙트를 그린다)
	Orb->ShowManagerResults(bHasNearest ? Nearest : Center, SnipeTargets, Kills);

	// 사망이 끝난 개체: 인스턴스를 떼고 액터를 지운 뒤 같은 수를 가장자리 링에서 다시 만든다(2단계의 지연 파괴 + RequestRespawn과 같은 스포너 스트림)
	if (Finished.Num() > 0)
	{
		TArray<FPrimitiveInstanceId, TInlineAllocator<32>> RemovedIds[LabUnitTypeMax];
		for (int32 k = Finished.Num() - 1; k >= 0; --k)
		{
			ALabManagedMinion* Minion = Minions[Finished[k]];
			RemovedIds[Minion->UnitType].Add(Minion->InstanceId);
			Minions.RemoveAtSwap(Finished[k], EAllowShrinking::No);
			Minion->Destroy();
		}
		for (int32 Type = 0; Type < Isms.Num(); ++Type)
		{
			if (RemovedIds[Type].Num() > 0)
			{
				Isms[Type]->RemoveInstancesById(RemovedIds[Type]); // 엔진도 갱신되지 않은 인스턴스를 이 호출로 뗀다(MassVisualizationComponent.cpp:647)
			}
		}
		Spawner->RespawnEntities(Finished.Num());
	}
}
