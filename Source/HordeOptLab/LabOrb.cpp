#include "LabOrb.h"
#include "LabConfig.h"
#include "HordeOptLab.h"
#include "LabCrowdSpawner.h"
#include "LabCrowdSubsystem.h"
#include "LabSnipe.h"
#include "LabUnitAssets.h"
#include "LabMinion.h"
#include "MassActorSubsystem.h"
#include "MassSimulationSubsystem.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

namespace
{
	// 오브 발광 배율. 구체 오버레이와 범위 원기둥 색에만 곱한다. 번개(NS_TeslaCoil)는 색이 사용자 파라미터로 열려 있지 않아 못 바꾼다.
	constexpr float OrbGlowBoost = 3.f; // 6배는 블룸으로 하얗게 날아가 보라색이 사라졌다
}

ALabOrb::ALabOrb()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// 상대 배치는 BP_TeslaCoil과 같다(Sphere z400, 시스템 둘은 루트). 구체 높이는 BeginPlay에서 궤도 높이로 옮긴다.
	Sphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(Root);
	Sphere->SetRelativeLocation(FVector(0.f, 0.f, 400.f));
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Sphere->SetStaticMesh(SphereMesh.Object);

	// 이펙트는 Niagara Examples Pack에서 옮긴 것(Scripts/migrate-assets.py "orbfx"). 없으면 구체만 남는다.
	CoilFx = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CoilFx"));
	CoilFx->SetupAttachment(Root);
	CoilFx->SetRelativeLocation(FVector(0.f, 0.f, 400.f)); // 구체 중심. 시스템의 발광(플레어)이 컴포넌트 원점에 생긴다
	ZoneFx = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZoneFx"));
	ZoneFx->SetupAttachment(Root);
}

void ALabOrb::UpdateOrbit()
{
	// Spike1: 궤도 위치는 월드 시각의 함수다(FLabOrbit). 카메라와 스포너도 같은 함수를 불러 틱 순서와 무관하게 같은 값을 본다.
	const FLabOrbit& Orbit = FLabConfig::Get().Orbit;
	if (Orbit.IsMoving() && OrbitFrame != GFrameCounter)
	{
		OrbitFrame = GFrameCounter;
		SetActorLocation(Orbit.LocationAt(GetWorld()->GetTimeSeconds()));
	}
}

void ALabOrb::BeginPlay()
{
	Super::BeginPlay();

	{
		// 구체를 궤도 높이에 띄운다. 번개(CoilFx)는 구체 중심에서 지면의 대상으로 내려온다
		const FLabOrbit& Orbit = FLabConfig::Get().Orbit;
		Sphere->SetRelativeLocation(FVector(0.f, 0.f, Orbit.SphereZ()));
		CoilFx->SetRelativeLocation(FVector(0.f, 0.f, Orbit.SphereZ()));
	}

	UMaterialInterface* Metal = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/NiagaraExamples/Gallery/Materials/M_Target_Metal.M_Target_Metal"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (Metal)
	{
		Sphere->SetMaterial(0, Metal);
	}
	// BP_TeslaCoil은 구체에 보라색 오버레이 머티리얼을 씌운다(BP 문자열의 OverlayMaterial, MI_Mesh_Overlay_TeslaCoil).
	if (UMaterialInterface* Overlay = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/NiagaraExamples/Materials/MI_Mesh_Overlay_TeslaCoil.MI_Mesh_Overlay_TeslaCoil"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		// 한낮 조명에서는 자동 노출 때문에 발광이 묻힌다. 조명 대신 이 오브의 발광만 키운다.
		// 파라미터 이름과 기본값은 MI_Mesh_Overlay_TeslaCoil에서 읽었다(Base/Secondary Color = (0.4, 0, 4), Flicker Low = 0.1).
		UMaterialInstanceDynamic* OverlayMid = UMaterialInstanceDynamic::Create(Overlay, this);
		OverlayMid->SetVectorParameterValue(TEXT("Base Color"), FLinearColor(0.25f, 0.f, 4.f) * OrbGlowBoost);
		OverlayMid->SetVectorParameterValue(TEXT("Secondary Color"), FLinearColor(0.25f, 0.f, 4.f) * OrbGlowBoost);
		OverlayMid->SetScalarParameterValue(TEXT("Flicker Low"), 0.6f);
		Sphere->SetOverlayMaterial(OverlayMid);
	}

	if (UNiagaraSystem* Coil = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/NiagaraExamples/FX_Ribbons/NS_TeslaCoil.NS_TeslaCoil"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		CoilFx->SetAsset(Coil);
		CoilFx->SetVariablePosition(TEXT("User.PositionTarget"), GetActorLocation()); // 구체에서 타워 밑동으로 번개가 내려온다
		CoilFx->Activate(true);
	}
	StrikeSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Lab/FX/NS_LabBolt.NS_LabBolt"), nullptr, LOAD_Quiet | LOAD_NoWarn);

	if (UMassSimulationSubsystem* Simulation = GetWorld()->GetSubsystem<UMassSimulationSubsystem>())
	{
		PhaseStartHandle = Simulation->GetOnProcessingPhaseStarted(EMassProcessingPhase::PrePhysics).AddUObject(this, &ALabOrb::OnMassPrePhysicsStart);
	}

	if (UNiagaraSystem* Zone = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/NiagaraExamples/FX_Misc/NS_Boundary_Cylinder.NS_Boundary_Cylinder"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		// User.Radius·User.Height·User.Color는 이 시스템의 사용자 파라미터다(에셋 문자열에서 확인).
		ZoneFx->SetAsset(Zone);
		ZoneFx->SetVariableFloat(TEXT("User.Radius"), FLabScenario::ZoneRadius);
		ZoneFx->SetVariableFloat(TEXT("User.Height"), 400.f);
		ZoneFx->SetVariableLinearColor(TEXT("User.Color"), FLinearColor(0.8f, 0.05f, 6.f) * OrbGlowBoost); // 보라. 기본값은 초록이다
		ZoneFx->Activate(true);
	}
}

void ALabOrb::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMassSimulationSubsystem* Simulation = GetWorld()->GetSubsystem<UMassSimulationSubsystem>(); Simulation && PhaseStartHandle.IsValid())
	{
		Simulation->GetOnProcessingPhaseStarted(EMassProcessingPhase::PrePhysics).Remove(PhaseStartHandle);
	}
	PhaseStartHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void ALabOrb::OnMassPrePhysicsStart(float DeltaSeconds)
{
	UpdateOrbit(); // 프로세서가 읽는 OrbCenter보다 먼저 옮긴다
	ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>();
	if (!Spawner || !Crowd || !Crowd->IsActive())
	{
		return; // M0 액터 단계는 오브 틱이 직접 피해·저격을 한다
	}
	Crowd->OrbCenter = GetActorLocation();
	Crowd->SnipeView = GetSnipeView();
	// 저격 시점은 M0과 같은 누적 시간으로 정하고, 대상 선택은 이 단계의 저격 프로세서가 한다.
	SnipeAccumulator += DeltaSeconds;
	while (SnipeAccumulator >= FLabScenario::SnipeInterval)
	{
		SnipeAccumulator -= FLabScenario::SnipeInterval;
		Crowd->PendingSnipeTick.store(++SnipeTick);
	}
}

void ALabOrb::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateOrbit(); // Mass 단계 시작이 이 프레임에 아직 불리지 않았을 때(M0 액터 단계 포함)
	if (!Spawner)
	{
		return;
	}
	if (ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>(); Crowd && Crowd->IsActive())
	{
		TickMass(DeltaSeconds, Crowd);
		return;
	}
	// 단계 1 액터 군중: 오브 틱이 직접 피해·저격을 한다
	const FVector Center = GetActorLocation();
	// 피해는 중심에 가까울수록 세다: 가장자리 ZoneDpsEdge → 중심 ZoneDpsCenter.
	const float ZoneRadiusSq = FMath::Square(FLabScenario::ZoneRadius);
	const ALabMinion* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (ALabMinion* Minion : Spawner->GetMinions())
	{
		const float DistSq = FVector::DistSquared2D(Minion->GetActorLocation(), Center);
		if (Minion->IsAlive() && DistSq <= ZoneRadiusSq)
		{
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = Minion;
			}
			const float Closeness = 1.f - FMath::Sqrt(DistSq) / FLabScenario::ZoneRadius;
			const float Dps = FMath::Lerp(FLabScenario::ZoneDpsEdge, FLabScenario::ZoneDpsCenter, Closeness);
			if (Minion->ApplyDamage(Dps * DeltaSeconds))
			{
				++KillCount;
			}
		}
	}

	// 번개는 범위 안에서 가장 가까운 개체로 뻗는다. 원본 BP도 User.PositionTarget을 대상(플레이어) 위치로 갱신한다.
	// 대상이 없으면 오브 아래 지면으로 떨어진다. 파라미터 하나만 바꾸므로 비용은 개체 수와 무관하다.
	const FVector Target = Nearest ? Nearest->GetActorLocation() : Center;
	CoilFx->SetVariablePosition(TEXT("User.PositionTarget"), Target);

	TickStrikes(DeltaSeconds);

	SnipeAccumulator += DeltaSeconds;
	while (SnipeAccumulator >= FLabScenario::SnipeInterval)
	{
		SnipeAccumulator -= FLabScenario::SnipeInterval;
		Snipe();
	}
	WriteCapsuleStats(); // v2 W6: 단계 1도 Mass 단계와 같은 CSV 스탯을 남긴다
}

FLabSnipeView ALabOrb::GetSnipeView() const
{
	// 화면 판정은 실제로 렌더하는 카메라로 한다. 해상도는 늘 16:9(벤치 2560x1440, 개발 1280x720)다.
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APlayerCameraManager* Cam = PC ? PC->PlayerCameraManager.Get() : nullptr;
	return Cam ? FLabSnipeView::Make(Cam->GetCameraLocation(), Cam->GetCameraRotation(), Cam->GetFOVAngle(), 16.f / 9.f) : FLabSnipeView();
}

void ALabOrb::Snipe()
{
	++SnipeTick;
	const FVector Center = GetActorLocation();
	const FLabSnipeView View = GetSnipeView();

	// 화면 안의 산 개체를 거리 구간별로 나눠, 구간마다 해시(일련번호, 저격 틱)가 가장 작은 개체를 고른다(LabSnipe.h). 2종은 예약 발만 대형.
	TLabSnipeSelector<ALabMinion*> Picker(SnipeTick);
	// v2 W6 캡슐 확인 대상: Mass 저격 프로세서의 CheckBest와 같은 규칙(액터가 붙은 화면 안의 산 개체 중 해시 최소). M0는 모든 개체가 액터다
	ALabMinion* CheckBest = nullptr;
	uint32 CheckBestHash = MAX_uint32;
	for (ALabMinion* Minion : Spawner->GetMinions())
	{
		if (!Minion->IsAlive() || !View.bValid)
		{
			continue;
		}
		const FVector Location = Minion->GetActorLocation();
		if (!View.Contains(FVector(Location.X, Location.Y, LabSnipeAimHeight)))
		{
			continue;
		}
		const uint32 Hash = LabSnipeHash(Minion->GetSerial(), SnipeTick);
		if (Hash < CheckBestHash)
		{
			CheckBestHash = Hash;
			CheckBest = Minion;
		}
		const int32 Band = FLabSnipeBands::BandOf(FVector::Dist2D(Location, Center));
		if (Band == INDEX_NONE)
		{
			continue;
		}
		Picker.Offer(Band, Hash, Minion, Location, Minion->GetUnitType() == LabUnitLarge);
	}

	// v2 W6: 저격 대상과 확인 대상에 캡슐 라인 트레이스를 쏴 센다(Mass 단계의 TraceSnipeCapsules와 같은 집계). 대상 선택과 피해는 바꾸지 않는다
	TArray<const ALabMinion*, TInlineAllocator<FLabScenario::SnipeCount>> Traced;
	Picker.ForEach([this, &Traced](const TLabSnipeSelector<ALabMinion*>::FPick& Pick)
	{
		TraceCapsule(Pick.Target, /*bSnipeTarget*/ true);
		Traced.Add(Pick.Target);
		Pick.Target->Snipe();
		++SnipeCount;
		SpawnStrikeFx(FVector(Pick.Location.X, Pick.Location.Y, LabSnipeAimHeight));
	});
	if (CheckBest && !Traced.Contains(CheckBest)) // 확인 대상이 마침 저격 대상이면 한 번만 쏜다
	{
		TraceCapsule(CheckBest, /*bSnipeTarget*/ false);
	}
}

void ALabOrb::SpawnStrikeFx(const FVector& Target)
{
	// 측정 실행을 포함해 언제나 그린다. 이펙트는 시나리오의 고정된 일부이고 모든 단계에서 같은 비용이다.
	// 저격 궤적은 구체에서 대상까지 곧은 보라 선 하나다. 디버그 라인은 Test 빌드에서 그려지지 않아서, 엔진 DynamicBeam 템플릿으로 만든
	// /Game/Lab/FX/NS_LabBolt(빔 점 2개, 리본 하나)로 그린다(Scripts/create-bolt-fx.md). 번개 표현은 화면에서 읽기 어려워 접었다.
	if (!StrikeSystem)
	{
		return;
	}
	UNiagaraComponent* Bolt = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), StrikeSystem, Sphere->GetComponentLocation(), FRotator::ZeroRotator,
		FVector(1.f), /*bAutoDestroy*/ true, /*bAutoActivate*/ false, ENCPoolMethod::AutoRelease);
	if (!Bolt)
	{
		return;
	}
	Bolt->SetVariableVec3(TEXT("User.BeamEnd"), Target);
	Bolt->SetVariableLinearColor(TEXT("User.BoltColor"), BoltColor);
	Bolt->SetVariableFloat(TEXT("User.BoltWidth"), BoltWidth);
	Bolt->Activate(true);
	ActiveStrikes.Add({ Bolt, StrikeDuration });
}

void ALabOrb::TickStrikes(float DeltaSeconds)
{
	for (int32 i = ActiveStrikes.Num() - 1; i >= 0; --i)
	{
		ActiveStrikes[i].TimeLeft -= DeltaSeconds;
		if (ActiveStrikes[i].TimeLeft <= 0.f)
		{
			// DynamicBeam 템플릿은 루프 이미터라 직접 꺼야 사라진다.
			if (UNiagaraComponent* Bolt = ActiveStrikes[i].Component.Get())
			{
				Bolt->Deactivate();
			}
			ActiveStrikes.RemoveAtSwap(i);
		}
	}
}

int32 ALabOrb::GetKillCount() const
{
	const ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>();
	return Crowd && Crowd->IsActive() ? Crowd->KillCount.load() : KillCount;
}

int32 ALabOrb::GetSnipeCount() const
{
	const ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>();
	return Crowd && Crowd->IsActive() ? Crowd->SnipeCount.load() : SnipeCount;
}

void ALabOrb::TickMass(float DeltaSeconds, ULabCrowdSubsystem* Crowd)
{
	// 오브 중심·화면 판정 카메라·저격 요청은 OnMassPrePhysicsStart가 쓴다(Mass 워커와의 경합을 피한다). 여기는 이펙트만 다룬다.
	Crowd->ExcludeVisualsFromLumen();

	TArray<FVector> Targets;
	TArray<FMassEntityHandle> TargetEntities;
	TArray<FMassEntityHandle> CheckEntities;
	FVector CoilTarget = GetActorLocation();
	{
		FScopeLock Lock(&Crowd->ResultLock);
		Targets = MoveTemp(Crowd->SnipeTargets);
		Crowd->SnipeTargets.Reset();
		TargetEntities = MoveTemp(Crowd->SnipeTargetEntities);
		Crowd->SnipeTargetEntities.Reset();
		CheckEntities = MoveTemp(Crowd->CapsuleCheckEntities);
		Crowd->CapsuleCheckEntities.Reset();
		if (Crowd->bHasNearest)
		{
			CoilTarget = Crowd->Nearest;
		}
	}
	// M0과 같이 번개는 범위 안에서 가장 가까운 개체로, 없으면 타워 밑동으로 뻗는다.
	CoilFx->SetVariablePosition(TEXT("User.PositionTarget"), CoilTarget);
	for (const FVector& Target : Targets)
	{
		SpawnStrikeFx(Target);
	}
	TickStrikes(DeltaSeconds);
	if (TargetEntities.Num() > 0 || CheckEntities.Num() > 0)
	{
		TraceSnipeCapsules(TargetEntities, CheckEntities);
	}
	WriteCapsuleStats();
}

void ALabOrb::TraceSnipeCapsules(TConstArrayView<FMassEntityHandle> SnipeTargets, TConstArrayView<FMassEntityHandle> CheckTargets)
{
	// 액터 구간의 대상인지는 엔티티에 하이드레이션 액터가 붙어 있는지로 가른다(UMassActorSubsystem, 프래그먼트 FMassActorFragment를 읽는다).
	// 대상 선택은 저격 프로세서가 이미 끝냈다. 여기서는 그 대상의 캡슐이 트레이스에 맞는지만 센다(대상·피해·사망은 바꾸지 않는다).
	// 게임 스레드에서 부른다: 액터 프래그먼트를 쓰는 프로세서(표현, ULabActorSyncProcessor)는 게임 스레드 전용이고 씬 쿼리도 게임 스레드에서 쏜다.
	const UMassActorSubsystem* Actors = GetWorld()->GetSubsystem<UMassActorSubsystem>();
	if (!Actors || !Sphere)
	{
		return;
	}
	auto TraceOne = [&](const FMassEntityHandle& Entity, bool bSnipeTarget)
	{
		// 인스턴스(VAT·ISKM)나 입자로 그리는 개체는 액터가 없어 캡슐도 없다
		if (const ALabMinionMassActor* Target = Cast<ALabMinionMassActor>(Actors->GetActorFromHandle(Entity)))
		{
			TraceCapsule(Target, bSnipeTarget);
		}
	};
	for (const FMassEntityHandle& Entity : SnipeTargets)
	{
		TraceOne(Entity, true);
	}
	for (const FMassEntityHandle& Entity : CheckTargets)
	{
		if (!SnipeTargets.Contains(Entity)) // 확인 대상이 마침 저격 대상이면 한 번만 쏜다
		{
			TraceOne(Entity, false);
		}
	}
}

void ALabOrb::TraceCapsule(const AActor* Target, bool bSnipeTarget)
{
	if (!Target || !Sphere)
	{
		return;
	}
	const FVector Start = Sphere->GetComponentLocation(); // 저격 궤적(NS_LabBolt)의 시작점
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LabSnipeCapsule), /*bTraceComplex*/ false, this);
	// 끝점은 캡슐 중심(액터 위치). 하이드레이션 액터는 ULabActorSyncProcessor가 엔티티 위치 + 반높이로 옮기고, M0 액터는 CharacterMovement가 캡슐 중심을 옮긴다.
	// 시작점이 캡슐 밖이라 맞으면 표면에서 맞는다
	FHitResult Hit;
	++CapsuleTraceStats.Traces;
	CapsuleTraceStats.SnipeTraces += bSnipeTarget ? 1 : 0;
	const bool bAny = GetWorld()->LineTraceSingleByChannel(Hit, Start, Target->GetActorLocation(), LabSnipeTraceChannel, Params);
	if (bAny)
	{
		if (Hit.GetActor() == Target) { ++CapsuleTraceStats.Hits; }
		else { ++CapsuleTraceStats.Blocked; }
	}
#if !UE_BUILD_TEST
	// 개발 확인: 막은 액터가 숨은(풀) 액터인지, 대상에서 얼마나 앞인지
	const AActor* Blocker = bAny && Hit.GetActor() != Target ? Hit.GetActor() : nullptr;
	UE_LOG(LogLab, Log, TEXT("lab_capsule snipe=%d result=%s dist_m=%.1f blocker_hidden=%d blocker_gap_m=%.1f blocker_large=%d"),
		bSnipeTarget ? 1 : 0, !bAny ? TEXT("miss") : Blocker ? TEXT("blocked") : TEXT("hit"), FVector::Dist(Start, Target->GetActorLocation()) / 100.f,
		Blocker && Blocker->IsHidden() ? 1 : 0, Blocker ? FVector::Dist2D(Blocker->GetActorLocation(), Target->GetActorLocation()) / 100.f : 0.f,
		Blocker && Cast<ALabMinion>(Blocker) && Cast<ALabMinion>(Blocker)->GetUnitType() == LabUnitLarge ? 1 : 0);
#endif
}

void ALabOrb::WriteCapsuleStats() const
{
	// 누적 값을 매 프레임 쓴다(Lab/Deaths와 같은 방식, Scripts/spike-timeline.py가 구간 증가분으로 초당 수를 낸다)
	CSV_CUSTOM_STAT(Lab, CapsuleTraces, CapsuleTraceStats.Traces, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, CapsuleHits, CapsuleTraceStats.Hits, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, CapsuleBlocked, CapsuleTraceStats.Blocked, ECsvCustomStatOp::Set);
}
