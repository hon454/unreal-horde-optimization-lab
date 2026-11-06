#include "LabOrb.h"
#include "LabConfig.h"
#include "LabCrowdSpawner.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

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

	if (UNiagaraSystem* Coil = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/NiagaraExamples/FX_Ribbons/NS_TeslaCoil.NS_TeslaCoil"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		CoilFx->SetAsset(Coil);
		CoilFx->Activate(true);
	}
	if (UNiagaraSystem* Zone = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/NiagaraExamples/FX_Misc/NS_Boundary_Cylinder.NS_Boundary_Cylinder"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		// User.Radius·User.Height·User.Color는 이 시스템의 사용자 파라미터다(에셋 문자열에서 확인).
		ZoneFx->SetAsset(Zone);
		ZoneFx->SetVariableFloat(TEXT("User.Radius"), FLabScenario::ZoneRadius);
		ZoneFx->SetVariableFloat(TEXT("User.Height"), 400.f);
		ZoneFx->SetVariableLinearColor(TEXT("User.Color"), FLinearColor(1.5f, 0.3f, 6.f)); // 보라. 기본값은 초록이다
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
	const float ZoneRadiusSq = FMath::Square(FLabScenario::ZoneRadius);
	const float Damage = FLabScenario::ZoneDps * DeltaSeconds;
	for (ALabMinion* Minion : Spawner->GetMinions())
	{
		if (Minion->IsAlive() && FVector::DistSquared2D(Minion->GetActorLocation(), Center) <= ZoneRadiusSq)
		{
			if (Minion->ApplyDamage(Damage))
			{
				++KillCount;
			}
		}
	}

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
#if ENABLE_DRAW_DEBUG
		DrawDebugLine(GetWorld(), Center, Pick.Value->GetActorLocation(), FColor::Cyan, false, 0.2f, 0, 4.f);
#endif
	}
}
