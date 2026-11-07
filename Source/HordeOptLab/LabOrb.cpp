#include "LabOrb.h"
#include "LabConfig.h"
#include "LabCrowdSpawner.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

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

	// 상대 배치는 BP_TeslaCoil과 같다(SM_TorchBase z360 roll180 x4, Sphere z400, 시스템 둘은 루트).
	Tower = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tower"));
	Tower->SetupAttachment(Root);
	Tower->SetRelativeLocationAndRotation(FVector(0.f, 0.f, 360.f), FRotator(0.f, 0.f, 180.f));
	Tower->SetRelativeScale3D(FVector(4.f));
	Tower->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Sphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(Root);
	Sphere->SetRelativeLocation(FVector(0.f, 0.f, 400.f));
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Sphere->SetStaticMesh(SphereMesh.Object);

	// 이펙트와 타워는 Niagara Examples Pack에서 옮긴 것(Scripts/migrate-assets.py "orbfx"). 없으면 구체만 남는다.
	CoilFx = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CoilFx"));
	CoilFx->SetupAttachment(Root);
	CoilFx->SetRelativeLocation(FVector(0.f, 0.f, 400.f)); // 구체 중심. 시스템의 발광(플레어)이 컴포넌트 원점에 생긴다
	ZoneFx = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZoneFx"));
	ZoneFx->SetupAttachment(Root);
}

void ALabOrb::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* Metal = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/NiagaraExamples/Gallery/Materials/M_Target_Metal.M_Target_Metal"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (UStaticMesh* TowerMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/NiagaraExamples/Gallery/StaticMesh/SM_TorchBase.SM_TorchBase"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		Tower->SetStaticMesh(TowerMesh);
		Tower->SetMaterial(0, Metal);
	}
	if (Metal)
	{
		Sphere->SetMaterial(0, Metal);
	}
	// BP_TeslaCoil은 구체에 보라색 오버레이 머티리얼을 씌운다(BP 문자열의 OverlayMaterial, MI_Mesh_Overlay_TeslaCoil).
	if (UMaterialInterface* Overlay = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/NiagaraExamples/Materials/MI_Mesh_Overlay_TeslaCoil.MI_Mesh_Overlay_TeslaCoil"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		// 한낮 조명에서는 자동 노출 때문에 발광이 묻힌다. 조명 대신 이 오브의 발광만 키운다(2026-11-07 사용자 의견).
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
	ImpactSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/NiagaraExamples/FX_Sparks/NS_Spark_Burst.NS_Spark_Burst"), nullptr, LOAD_Quiet | LOAD_NoWarn);

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

void ALabOrb::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Spawner)
	{
		return;
	}

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
	// 대상이 없으면 타워 밑동으로 떨어진다. 파라미터 하나만 바꾸므로 비용은 개체 수와 무관하다.
	const FVector Target = Nearest ? Nearest->GetActorLocation() : Center;
	CoilFx->SetVariablePosition(TEXT("User.PositionTarget"), Target);

	TickStrikes(DeltaSeconds);

	SnipeAccumulator += DeltaSeconds;
	while (SnipeAccumulator >= FLabScenario::SnipeInterval)
	{
		SnipeAccumulator -= FLabScenario::SnipeInterval;
		Snipe();
	}
}

void ALabOrb::Snipe()
{
	++SnipeTick;
	const FVector Center = GetActorLocation();
	const float MinDistSq = FMath::Square(FLabScenario::SnipeMinDistance);

	// 먼 산 개체 중 해시(일련번호, 저격 틱)가 가장 작은 K개.
	TArray<TPair<uint32, ALabMinion*>, TInlineAllocator<64>> Best;
	for (ALabMinion* Minion : Spawner->GetMinions())
	{
		if (!Minion->IsAlive() || FVector::DistSquared2D(Minion->GetActorLocation(), Center) < MinDistSq)
		{
			continue;
		}
		const uint32 Hash = HashCombineFast(::GetTypeHash(Minion->GetSerial()), ::GetTypeHash(SnipeTick));
		if (Best.Num() < FLabScenario::SnipeCount)
		{
			Best.Emplace(Hash, Minion);
			continue;
		}
		int32 WorstIndex = 0;
		for (int32 i = 1; i < Best.Num(); ++i)
		{
			if (Best[i].Key > Best[WorstIndex].Key) { WorstIndex = i; }
		}
		if (Hash < Best[WorstIndex].Key)
		{
			Best[WorstIndex] = TPair<uint32, ALabMinion*>(Hash, Minion);
		}
	}

	for (const TPair<uint32, ALabMinion*>& Pick : Best)
	{
		Pick.Value->Snipe();
		++SnipeCount;
		SpawnStrikeFx(Pick.Value->GetActorLocation() + FVector(0.f, 0.f, 100.f));
	}
}

void ALabOrb::SpawnStrikeFx(const FVector& Target)
{
	// 측정 실행을 포함해 언제나 그린다. 이펙트는 시나리오의 고정된 일부이고 모든 단계에서 같은 비용이다(2026-11-07 사용자 결정).
	// 저격 번개는 엔진 Niagara DynamicBeam 템플릿으로 만든 /Game/Lab/FX/NS_LabBolt다(Scripts/create-bolt-fx.md). User.BeamEnd(월드 좌표)로
	// 끝점을, User.BoltColor·User.BoltWidth로 색과 굵기를 정한다. 굵고 강해 보이도록 StrikeLayers겹을 굵기를 달리해 겹친다.
	if (!StrikeSystem)
	{
		return;
	}
	const FVector Start = Sphere->GetComponentLocation();
	for (int32 i = 0; i < StrikeLayers; ++i)
	{
		UNiagaraComponent* Bolt = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), StrikeSystem, Start, FRotator::ZeroRotator,
			FVector(1.f), /*bAutoDestroy*/ true, /*bAutoActivate*/ false, ENCPoolMethod::AutoRelease);
		if (!Bolt)
		{
			continue;
		}
		Bolt->SetVariableVec3(TEXT("User.BeamEnd"), Target);
		Bolt->SetVariableLinearColor(TEXT("User.BoltColor"), i == 0 ? FLinearColor(8.f, 4.f, 40.f) : FLinearColor(2.f, 0.2f, 14.f));
		Bolt->SetVariableFloat(TEXT("User.BoltWidth"), i == 0 ? BoltCoreWidth : BoltGlowWidth);
		Bolt->Activate(true);
		ActiveStrikes.Add({ Bolt, StrikeDuration });
	}

	if (ImpactSystem)
	{
		if (UNiagaraComponent* Impact = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactSystem, Target))
		{
			// 파라미터 이름은 NS_Spark_Burst의 사용자 파라미터(에셋 문자열에서 확인).
			Impact->SetVariableFloat(TEXT("User.Spark Color Gain"), 20.f);
			Impact->SetVariableFloat(TEXT("User.Size"), 2.f);
		}
	}
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
