#include "LabOrb.h"
#include "LabConfig.h"
#include "LabCrowdSpawner.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ALabOrb::ALabOrb()
{
	PrimaryActorTick.bCanEverTick = true;
	Sphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	RootComponent = Sphere;
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Sphere->SetStaticMesh(SphereMesh.Object);
	Sphere->SetRelativeScale3D(FVector(2.f));
}

void ALabOrb::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Spawner)
	{
		return;
	}

	const FVector Center = GetActorLocation();
	const float KillRadiusSq = FMath::Square(FLabScenario::KillRadius);
	for (ALabMinion* Minion : Spawner->GetMinions())
	{
		if (Minion->IsAlive() && FVector::DistSquared2D(Minion->GetActorLocation(), Center) <= KillRadiusSq)
		{
			Minion->Kill();
			++KillCount;
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
