#include "LabCrowdSpawner.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabUnitAssets.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "AnimToTextureDataAsset.h"
#include "Engine/World.h"

namespace
{
	const float SpawnZ = 100.f; // 캡슐 반높이 위

	/**
	 * 종류별 에셋을 ini 경로(FLabUnitAssetPaths, Config/DefaultGame.ini [LabUnitAssets])로 읽는다. 없는 것은 Missing에 이름을 더한다.
	 * 그래프가 아직 비어 있는 AnimBP(출력 포즈 노드 하나뿐)는 레퍼런스 포즈만 내므로 없는 것으로 보고 싱글 노드로 대신한다.
	 */
	void LabLoadMinionAssets(int32 UnitType, FLabMinionAssets& Out, FString& Missing)
	{
		const FLabUnitAssetPaths& P = FLabUnitAssetPaths::Get(UnitType);
		auto Note = [&Missing](const TCHAR* What, const UObject* Obj) { if (!Obj) { if (!Missing.IsEmpty()) { Missing += TEXT(","); } Missing += What; } };
		auto LoadPath = [](auto* Dummy, const FString& Path) -> decltype(Dummy) { return Path.IsEmpty() ? nullptr : LoadObject<std::remove_pointer_t<decltype(Dummy)>>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn); };
		Out.Mesh = LoadPath((USkeletalMesh*)nullptr, P.SkeletalMesh);
		Out.Walk = LoadPath((UAnimSequence*)nullptr, P.Walk);
		Out.Hit = LoadPath((UAnimSequence*)nullptr, P.Hit);
		Out.Death = LoadPath((UAnimSequence*)nullptr, P.Death);
		Out.StaticMesh = LoadPath((UStaticMesh*)nullptr, P.StaticMesh);
		Out.AnimClass = P.AnimClass.IsEmpty() ? nullptr : LoadClass<UAnimInstance>(nullptr, *P.AnimClass, nullptr, LOAD_Quiet | LOAD_NoWarn);
		if (const UAnimBlueprintGeneratedClass* AnimBPClass = Cast<UAnimBlueprintGeneratedClass>(Out.AnimClass.Get()))
		{
			if (AnimBPClass->GetAnimNodeProperties().Num() <= 1)
			{
				Out.AnimClass = nullptr;
			}
		}
		Note(TEXT("mesh"), Out.Mesh); Note(TEXT("walk"), Out.Walk); Note(TEXT("hit"), Out.Hit); Note(TEXT("death"), Out.Death);
		Note(TEXT("animbp"), Out.AnimClass); Note(TEXT("staticmesh"), Out.StaticMesh);
	}
}

ALabCrowdSpawner::ALabCrowdSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALabCrowdSpawner::LoadAssets()
{
	// 소형: 경로는 ini의 Small* 키, 없으면 2종 전과 같은 기본 경로(LabUnitAssets.cpp).
	FString Missing;
	LabLoadMinionAssets(LabUnitSmall, Assets, Missing);
	// static 변형의 메시. Scripts/make-static-minion.py가 만든다. 없으면 엔진 실린더로 대신한다.
	if (!Assets.StaticMesh)
	{
		Assets.StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	}
	UE_LOG(LogLab, Log, TEXT("lab_assets mesh=%d walk=%d hit=%d death=%d animbp=%d staticmesh=%s"),
		Assets.Mesh != nullptr, Assets.Walk != nullptr, Assets.Hit != nullptr, Assets.Death != nullptr,
		Assets.AnimClass != nullptr, *GetNameSafe(Assets.StaticMesh));

	if (FLabUnitTypes::Num() == 2)
	{
		// 대형(2종): 세트가 하나라도 비면 전부 소형으로 대체하고 경고한다(스모크 전용, 측정 금지). 종류(아키타입·저격 예약·액터 풀)는 그대로 갈린다.
		// VAT(M2 이상이 읽는다)도 여기서 함께 확인해 대체 여부를 한 번에 정한다. 모드는 변형에 따른다(vatvert만 Vertex).
		FString LargeMissing;
		LabLoadMinionAssets(LabUnitLarge, LargeAssets, LargeMissing);
		const FLabUnitAssetPaths& Large = FLabUnitAssetPaths::Get(LabUnitLarge);
		const TCHAR* Mode = FLabConfig::Get().Variant == ELabVariant::VatVert ? TEXT("Vertex") : TEXT("Bone");
		const FString VatMeshPath = Large.VatMeshPath(Mode);
		const FString VatDataPath = Large.VatDataPath(Mode);
		if (VatMeshPath.IsEmpty() || !LoadObject<UStaticMesh>(nullptr, *VatMeshPath, nullptr, LOAD_Quiet | LOAD_NoWarn)) { LargeMissing += LargeMissing.IsEmpty() ? TEXT("vatmesh") : TEXT(",vatmesh"); }
		if (VatDataPath.IsEmpty() || !LoadObject<UAnimToTextureDataAsset>(nullptr, *VatDataPath, nullptr, LOAD_Quiet | LOAD_NoWarn)) { LargeMissing += LargeMissing.IsEmpty() ? TEXT("vatdata") : TEXT(",vatdata"); }
		FLabUnitTypes::bLargeSubstituted = !LargeMissing.IsEmpty();
		if (FLabUnitTypes::bLargeSubstituted)
		{
			LargeAssets = Assets;
			UE_LOG(LogLab, Warning, TEXT("lab_types_warning types=2 large_assets_missing=%s substituting=small smoke_only=1 do_not_measure=1"), *LargeMissing);
		}
		UE_LOG(LogLab, Log, TEXT("lab_types types=2 large_assets=%s large_radius=%.0f small_radius=%.0f"),
			FLabUnitTypes::bLargeSubstituted ? TEXT("substituted") : TEXT("ok"), Large.BoundRadius, FLabUnitAssetPaths::Get(LabUnitSmall).BoundRadius);
	}
}

TSubclassOf<ALabMinion> ALabCrowdSpawner::MinionClass() const
{
	switch (FLabConfig::Get().Variant)
	{
	case ELabVariant::Static: return ALabMinionStatic::StaticClass();
	case ELabVariant::Budget: return ALabMinionBudget::StaticClass();
	case ELabVariant::Pawn: return ALabMinionPawn::StaticClass();
	case ELabVariant::StaticPawn: return ALabMinionStaticPawn::StaticClass();
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
		// 종류별 ISM 메시(2종이면 둘, LabUnitAssets.h). M1은 정적 메시, M2부터 VAT 메시.
		const int32 Types = FLabUnitTypes::Num();
		UStaticMesh* CrowdMeshes[LabUnitTypeMax] = { Assets.StaticMesh, LargeAssets.StaticMesh };
		if (Config.Stage >= ELabStage::M2)
		{
			// M2: VAT 메시(Scripts/bake-vat.py). 클립 프레임은 데이터 에셋에서 읽는다. 없으면 시작하지 않는다(정지 메시로 재면 M1과 같아진다).
			const TCHAR* Mode = Config.Variant == ELabVariant::VatVert ? TEXT("Vertex") : TEXT("Bone"); // E2 (b)
			for (int32 Type = 0; Type < Types; ++Type)
			{
				const FLabUnitAssetPaths& Paths = FLabUnitAssetPaths::Get(FLabUnitTypes::AssetType(Type)); // 대체 중이면 대형도 소형 VAT
				UStaticMesh* VatMesh = LoadObject<UStaticMesh>(nullptr, *Paths.VatMeshPath(Mode));
				const UAnimToTextureDataAsset* VatData = LoadObject<UAnimToTextureDataAsset>(nullptr, *Paths.VatDataPath(Mode));
				checkf(VatMesh && VatData && VatData->Animations.Num() == 3, TEXT("VAT assets missing: run Scripts/bake-vat.py"));
				FLabVatClips& Clips = Crowd->VatClipsByType[Type];
				FLabVatClip* Targets[3] = { &Clips.Walk, &Clips.Hit, &Clips.Death };
				for (int32 i = 0; i < 3; ++i)
				{
					Targets[i]->StartFrame = VatData->Animations[i].StartFrame;
					Targets[i]->EndFrame = VatData->Animations[i].EndFrame;
				}
				Clips.SampleRate = VatData->SampleRate;
				CrowdMeshes[Type] = VatMesh;
				UE_LOG(LogLab, Log, TEXT("lab_vat type=%d mode=%s walk=%d..%d hit=%d..%d death=%d..%d rate=%.0f"), Type, Mode,
					Clips.Walk.StartFrame, Clips.Walk.EndFrame, Clips.Hit.StartFrame, Clips.Hit.EndFrame, Clips.Death.StartFrame, Clips.Death.EndFrame, Clips.SampleRate);
			}
		}
		// 피격·사망 시간(로직)은 종류와 무관하게 소형 애니 길이 하나다(행동은 같다). 대형 VAT 클립 길이가 다르면 ClipFrame이 끝 프레임에서 멈추거나 잘린다.
		Crowd->Activate(this, MakeArrayView(CrowdMeshes, Types),
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
	const uint32 Serial = NextSerial++;
	// 대형은 캡슐이 배율만큼 커지므로 캡슐 중심 높이도 같은 배율로 올린다(소형은 배율 1이라 전과 같다)
	const FVector Location(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), SpawnZ * FLabUnitAssetPaths::Get(FLabUnitTypes::TypeOf(Serial)).Scale);
	const FRotator Facing = (OrbLocation - Location).GetSafeNormal2D().Rotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALabMinion* Minion = GetWorld()->SpawnActor<ALabMinion>(MinionClass(), Location, Facing, Params);
	Minion->InitMinion(this, FLabUnitTypes::IsLarge(Serial) ? LargeAssets : Assets, Serial); // M0 2종: 대형 에셋(없으면 소형과 같다)
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
