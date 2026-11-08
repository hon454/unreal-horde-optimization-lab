#include "LabCrowdSpawner.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "AnimToTextureDataAsset.h"
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
	// 그래프가 아직 비어 있는 AnimBP(출력 포즈 노드 하나뿐)는 레퍼런스 포즈만 내므로 쓰지 않고 싱글 노드로 대신한다.
	if (const UAnimBlueprintGeneratedClass* AnimBPClass = Cast<UAnimBlueprintGeneratedClass>(Assets.AnimClass.Get()))
	{
		if (AnimBPClass->GetAnimNodeProperties().Num() <= 1)
		{
			Assets.AnimClass = nullptr;
		}
	}
	// static 변형의 메시. Scripts/make-static-minion.py가 만든다. 없으면 엔진 실린더로 대신한다.
	Assets.StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Lab/Derived/SM_LabMinion.SM_LabMinion"), nullptr, LOAD_Quiet | LOAD_NoWarn);
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
	case ELabVariant::Pawn: return ALabMinionPawn::StaticClass();
	default: return ALabMinion::StaticClass();
	}
}

void ALabCrowdSpawner::SpawnInitial()
{
	LoadAssets();
	const FLabConfig& Config = FLabConfig::Get();
	Stream.Initialize(Config.Seed);
	OrbLocation = FVector::ZeroVector;

	// 반지름을 [피해 범위, R]에서 균등하게 뽑으면 반지름당 개체 수가 일정하다(밀도 ∝ 1/r). 모든 단계가 같은 순서로 뽑는다.
	TArray<FVector2D> RadiusAngles;
	RadiusAngles.Reserve(Config.N);
	for (int32 i = 0; i < Config.N; ++i)
	{
		const float Radius = Stream.FRandRange(FLabScenario::ZoneRadius, FLabScenario::MapRadius);
		const float Angle = Stream.FRandRange(0.f, UE_TWO_PI);
		RadiusAngles.Emplace(Radius, Angle);
	}

	if (Config.Stage == ELabStage::M0)
	{
		Minions.Reserve(Config.N);
		for (const FVector2D& RA : RadiusAngles)
		{
			SpawnAt(RA.X, RA.Y);
		}
	}
	else
	{
		// M1부터 개체는 Mass 엔티티다. 표현은 static 변형과 같은 정적 메시, 피격·사망 시간은 M0 애니 길이와 같다.
		Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>();
		Crowd->OrbCenter = OrbLocation;
		UStaticMesh* CrowdMesh = Assets.StaticMesh;
		if (Config.Stage >= ELabStage::M2)
		{
			// M2: VAT 메시(Scripts/bake-vat.py). 클립 프레임은 데이터 에셋에서 읽는다. 없으면 시작하지 않는다(정지 메시로 재면 M1과 같아진다).
			const TCHAR* Mode = Config.Variant == ELabVariant::VatVert ? TEXT("Vertex") : TEXT("Bone"); // E2 (b)
			UStaticMesh* VatMesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Lab/VAT/Baked/SM_LabMinionVAT_%s.SM_LabMinionVAT_%s"), Mode, Mode));
			const UAnimToTextureDataAsset* VatData = LoadObject<UAnimToTextureDataAsset>(nullptr, *FString::Printf(TEXT("/Game/Lab/VAT/Baked/DA_LabMinionVAT_%s.DA_LabMinionVAT_%s"), Mode, Mode));
			checkf(VatMesh && VatData && VatData->Animations.Num() == 3, TEXT("VAT assets missing: run Scripts/bake-vat.py"));
			FLabVatClips& Clips = Crowd->VatClips;
			FLabVatClip* Targets[3] = { &Clips.Walk, &Clips.Hit, &Clips.Death };
			for (int32 i = 0; i < 3; ++i)
			{
				Targets[i]->StartFrame = VatData->Animations[i].StartFrame;
				Targets[i]->EndFrame = VatData->Animations[i].EndFrame;
			}
			Clips.SampleRate = VatData->SampleRate;
			CrowdMesh = VatMesh;
			UE_LOG(LogLab, Log, TEXT("lab_vat mode=%s walk=%d..%d hit=%d..%d death=%d..%d rate=%.0f"), Mode,
				Clips.Walk.StartFrame, Clips.Walk.EndFrame, Clips.Hit.StartFrame, Clips.Hit.EndFrame, Clips.Death.StartFrame, Clips.Death.EndFrame, Clips.SampleRate);
		}
		Crowd->Activate(this, CrowdMesh,
			Assets.Hit ? Assets.Hit->GetPlayLength() : 0.5f, Assets.Death ? Assets.Death->GetPlayLength() : 1.f);
		SpawnEntitiesAt(RadiusAngles);
	}
	UE_LOG(LogLab, Log, TEXT("lab_spawned n=%d"), GetAliveCount());
}

int32 ALabCrowdSpawner::GetAliveCount() const
{
	return Crowd ? Crowd->GetAliveCount() : Minions.Num();
}

void ALabCrowdSpawner::SpawnEntitiesAt(TConstArrayView<FVector2D> RadiusAngles)
{
	TArray<FVector> Locations;
	TArray<uint32> Serials;
	Locations.Reserve(RadiusAngles.Num());
	Serials.Reserve(RadiusAngles.Num());
	for (const FVector2D& RA : RadiusAngles)
	{
		Locations.Emplace(RA.X * FMath::Cos(RA.Y), RA.X * FMath::Sin(RA.Y), 0.f); // 엔티티 위치는 발밑(지면)이다
		Serials.Add(NextSerial++);
	}
	Crowd->SpawnEntities(Locations, Serials);
}

void ALabCrowdSpawner::RespawnEntities(int32 Count)
{
	// M0 OnMinionFinished와 같은 스트림, 같은 링에서 뽑는다.
	TArray<FVector2D> RadiusAngles;
	for (int32 i = 0; i < Count; ++i)
	{
		const float Radius = Stream.FRandRange(FLabScenario::MapRadius - FLabScenario::RespawnRingWidth, FLabScenario::MapRadius);
		const float Angle = Stream.FRandRange(0.f, UE_TWO_PI);
		RadiusAngles.Emplace(Radius, Angle);
	}
	SpawnEntitiesAt(RadiusAngles);
	RespawnCount += Count;
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
