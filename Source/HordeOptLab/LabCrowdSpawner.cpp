#include "LabCrowdSpawner.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

namespace
{
	const TCHAR* MinionRoot = TEXT("/Game/ParagonMinions/Characters/Minions/Down_Minions");
	const float SpawnZ = 100.f; // 캡슐 반높이 위
}

ALabCrowdSpawner::ALabCrowdSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALabCrowdSpawner::LoadAssets()
{
	Assets.Mesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%s/Meshes/Minion_Lane_Melee_Dawn.Minion_Lane_Melee_Dawn"), MinionRoot));
	Assets.Walk = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("%s/Animations/Melee/NonCombat_JogFwd.NonCombat_JogFwd"), MinionRoot));
	Assets.Hit = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("%s/Animations/Melee/HitReact_Front.HitReact_Front"), MinionRoot));
	Assets.Death = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("%s/Animations/Melee/Death_A.Death_A"), MinionRoot));
	Assets.AnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Lab/Anim/ABP_LabMinion.ABP_LabMinion_C"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	// static 변형의 메시. 미니언 정적 메시 변환(M0 6번)이 끝나기 전에는 엔진 실린더로 대신한다.
	Assets.StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Lab/Meshes/SM_LabMinion.SM_LabMinion"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!Assets.StaticMesh)
	{
		Assets.StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	}
	UE_LOG(LogLab, Log, TEXT("lab_assets mesh=%d walk=%d hit=%d death=%d animbp=%d staticmesh=%s"),
		Assets.Mesh != nullptr, Assets.Walk != nullptr, Assets.Hit != nullptr, Assets.Death != nullptr,
		Assets.AnimClass != nullptr, *GetNameSafe(Assets.StaticMesh));
}

TSubclassOf<ALabMinion> ALabCrowdSpawner::MinionClass() const
{
	switch (FLabConfig::Get().Variant)
	{
	case ELabVariant::Static: return ALabMinionStatic::StaticClass();
	case ELabVariant::Budget: return ALabMinionBudget::StaticClass();
	default: return ALabMinion::StaticClass();
	}
}

void ALabCrowdSpawner::SpawnInitial()
{
	LoadAssets();
	const FLabConfig& Config = FLabConfig::Get();
	Stream.Initialize(Config.Seed);
	OrbLocation = FVector::ZeroVector;

	// 반지름을 [피해 범위, R]에서 균등하게 뽑으면 반지름당 개체 수가 일정하다(밀도 ∝ 1/r).
	Minions.Reserve(Config.N);
	for (int32 i = 0; i < Config.N; ++i)
	{
		const float Radius = Stream.FRandRange(FLabScenario::ZoneRadius, FLabScenario::MapRadius);
		const float Angle = Stream.FRandRange(0.f, UE_TWO_PI);
		SpawnAt(Radius, Angle);
	}
	UE_LOG(LogLab, Log, TEXT("lab_spawned n=%d"), Minions.Num());
}

ALabMinion* ALabCrowdSpawner::SpawnAt(float Radius, float Angle)
{
	const FVector Location(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), SpawnZ);
	const FRotator Facing = (OrbLocation - Location).GetSafeNormal2D().Rotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALabMinion* Minion = GetWorld()->SpawnActor<ALabMinion>(MinionClass(), Location, Facing, Params);
	Minion->InitMinion(this, Assets, NextSerial++);
	Minions.Add(Minion);
	return Minion;
}

void ALabCrowdSpawner::OnMinionFinished(ALabMinion* Minion)
{
	Minions.RemoveSwap(Minion);
	Minion->Destroy();

	// 재생성 위치도 같은 스트림에서 뽑는다. 사망 순서는 실행마다 다를 수 있어 위치 열만 재현된다(measurement.md "시드").
	const float Radius = Stream.FRandRange(FLabScenario::MapRadius - FLabScenario::RespawnRingWidth, FLabScenario::MapRadius);
	const float Angle = Stream.FRandRange(0.f, UE_TWO_PI);
	SpawnAt(Radius, Angle);
	++RespawnCount;
}
