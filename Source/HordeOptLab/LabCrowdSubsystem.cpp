#include "LabCrowdSubsystem.h"
#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabCrowdSpawner.h"
#include "LabMassTraits.h"
#include "LabMassTypes.h"
#include "MassCommonFragments.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "MassEntityUtils.h"
#include "MassLODTrait.h"
#include "MassMovableVisualizationTrait.h"
#include "MassSpawnerSubsystem.h"
#include "SmoothOrientation/MassSmoothOrientationTrait.h"
#include "Steering/MassSteeringTrait.h"
#include "Avoidance/MassAvoidanceTrait.h"
#include "Avoidance/MassNavigationObstacleTrait.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "MassVisualizer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/InstancedSkinnedMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "MassMovementFragments.h"
#include "MassMovementTypes.h"
#include "LabMinion.h"
#include "LabActorSyncProcessor.h"
#include "Engine/World.h"

namespace
{
	/** UMassEntityConfigAsset::AddTrait(TSubclassOf)는 에디터 전용이라(MassEntityConfigAsset.h WITH_EDITOR) 런타임 API인 FMassEntityConfig::AddTrait(Trait&)를 쓴다. */
	template<typename T>
	T* AddLabTrait(UMassEntityConfigAsset& Config)
	{
		T* Trait = NewObject<T>(&Config);
		Config.GetMutableConfig().AddTrait(*Trait);
		return Trait;
	}
}

void ULabCrowdSubsystem::Activate(ALabCrowdSpawner* InSpawner, TConstArrayView<UStaticMesh*> MeshesByType, float InHitDuration, float InDeathDuration)
{
	Spawner = InSpawner;
	HitDuration = InHitDuration;
	DeathDuration = InDeathDuration;
	EntityConfigs.Reset();
	for (int32 Type = 0; Type < MeshesByType.Num(); ++Type)
	{
		EntityConfigs.Add(BuildEntityConfig(Type, MeshesByType[Type]));
	}
	bActive = true;
}

UMassEntityConfigAsset* ULabCrowdSubsystem::BuildEntityConfig(int32 UnitType, UStaticMesh* Mesh)
{
	// EntityConfig를 실행 시 만든다(ADR-0005). 소유자가 있으면 ConfigGuid가 경로로 결정적으로 정해져 실행마다 같은 템플릿이 된다
	// (MassSpawner/Private/MassEntityConfigAsset.cpp:33, 템플릿 ID는 :137). 2종(M3b)은 종류마다 이름이 다른 Config 하나씩이라 템플릿과 아키타입이 갈린다.
	UMassEntityConfigAsset* EntityConfig = NewObject<UMassEntityConfigAsset>(this, UnitType == LabUnitLarge ? TEXT("LabMinionLargeConfig") : TEXT("LabMinionConfig"));
	ULabMinionTrait* MinionTrait = AddLabTrait<ULabMinionTrait>(*EntityConfig);
	MinionTrait->UnitType = UnitType;
	AddLabTrait<ULabMovementTrait>(*EntityConfig);
	AddLabTrait<UMassSteeringTrait>(*EntityConfig);
	AddLabTrait<UMassSmoothOrientationTrait>(*EntityConfig);
	AddLabTrait<UMassLODCollectorTrait>(*EntityConfig);
	const bool bSimLod = FLabConfig::Get().UsesSimLod();
	if (bSimLod)
	{
		// M4: 엔진 Simulation LOD(가변 틱). LOD 수집 Trait(위)이 만든 FMassViewerInfoFragment를 요구한다(MassLODTrait.cpp:123)
		AddLabTrait<ULabSimulationLODTrait>(*EntityConfig);
	}
	if (FLabConfig::Get().Variant == ELabVariant::MassAv || (bSimLod && FLabConfig::Get().SimLod().bAvoidance))
	{
		// E1 (c): 엔진 회피. 이웃을 찾으려면 장애물 격자에 등록돼야 하므로 두 Trait을 함께 붙인다. 파라미터는 엔진 기본값.
		// E4 (c) m4avoid: 같은 Trait. LOD 태그가 있어 회피 프로세서가 High(80m 안)만 처리한다(MassAvoidanceProcessors.cpp:301-303)
		AddLabTrait<UMassObstacleAvoidanceTrait>(*EntityConfig);
		AddLabTrait<UMassNavigationObstacleTrait>(*EntityConfig);
	}

	// 표현: 애니 없는 ISM 하나. 밴드 없이 모든 LOD에서 같은 ISM이다(plan.md "M1"). 거리 밴드는 M2에서 켠다.
	UMassMovableVisualizationTrait* Visual = AddLabTrait<UMassMovableVisualizationTrait>(*EntityConfig);
	FMassStaticMeshInstanceVisualizationMeshDesc& MeshDesc = Visual->StaticMeshInstanceDesc.Meshes.AddDefaulted_GetRef();
	MeshDesc.Mesh = Mesh;
	// M0 액터와 같이 메시를 -90도 돌려 진행 방향을 보게 한다(LabMinion.cpp). 대형은 균일 배율을 곱한다(소형 1, LabUnitAssets.h). 메시 원점이 발밑이라 발은 지면에 남는다.
	MeshDesc.LocalTransform = FTransform(FRotator(0.f, -90.f, 0.f), FVector::ZeroVector, FVector(FLabUnitAssetPaths::Get(UnitType).Scale));
	MeshDesc.bCastShadows = true; // 엔진 기본값은 false(MassRepresentationTypes.h:105). M0 액터와 같이 그림자를 그린다
	for (int32 LOD = 0; LOD < EMassLOD::Max; ++LOD)
	{
		Visual->Params.LODRepresentation[LOD] = EMassRepresentationType::StaticMeshInstance;
		Visual->LODParams.BaseLODDistance[LOD] = LOD == 0 ? 0.f : 1.e7f;     // 맵(반경 500m) 안에서는 LOD가 바뀌지 않는다
		Visual->LODParams.VisibleLODDistance[LOD] = LOD == 0 ? 0.f : 1.e7f;
		Visual->LODParams.LODMaxCount[LOD] = MAX_int32;
	}

	// E2 (c): 확정 밴드(30/80/200m, measurement.md "확정값")를 켜고 Off(200m 밖)를 그리지 않는다. 대표(M2)는 M1과 같이 모든 거리를 그린다.
	// M3부터는 이 밴드가 대표 설정이고, High(0~30m)는 액터다(plan.md "M3").
	const ELabVariant Variant = FLabConfig::Get().Variant;
	const bool bM3 = FLabConfig::Get().Stage >= ELabStage::M3;
	if (bM3)
	{
		const bool bBudget = Variant == ELabVariant::M3BudgetDist || Variant == ELabVariant::M3BudgetSig || FLabConfig::Get().bScreenSigBudget;
		// 종류별 액터 클래스. 엔진 풀은 클래스가 키라(UMassActorSpawnerSubsystem::PooledActors, MassActorSpawnerSubsystem.h:321) 대형은 풀도 따로 생긴다.
		if (UnitType == LabUnitLarge)
		{
			Visual->HighResTemplateActor = bBudget ? ALabMinionMassActorBudgetLarge::StaticClass() : ALabMinionMassActorLarge::StaticClass();
		}
		else
		{
			Visual->HighResTemplateActor = bBudget ? ALabMinionMassActorBudget::StaticClass() : ALabMinionMassActor::StaticClass();
		}
		Visual->Params.LODRepresentation[EMassLOD::High] = Variant == ELabVariant::M3NoActor ? EMassRepresentationType::StaticMeshInstance : EMassRepresentationType::HighResSpawnedActor;
		// 전환 팝핑 대책(2026-11-08 사용자 결정). VAT → 액터: 액터의 포즈가 준비될 때까지 VAT 인스턴스를 그린다.
		// 액터 → VAT: 액터를 한 프레임 더 남긴다(엔진 군중 기본값도 켠다, MassCrowdVisualizationTrait.cpp:22). bKeepLowResActors가 켜져 있어야 동작한다(MassRepresentationProcessor.h:27-30).
		Visual->Params.RepresentationActorManagementClass = ULabActorManagement::StaticClass();
		Visual->Params.bWaitForActorVisualReadiness = FLabConfig::Get().HasFix(FLabConfig::FixWait);
		Visual->Params.bKeepActorExtraFrame = FLabConfig::Get().HasFix(FLabConfig::FixWait);
	}
	if (bM3 || Variant == ELabVariant::VatCull || Variant == ELabVariant::VatCullVis)
	{
		// E3 (7): 하이드레이션 거리(화면 안 Visible)만 바꾼다(대표 30m). 80m·200m 밴드와 상한은 그대로(2026-11-09 사용자 결정 A)
		const float HydrateCm = Variant == ELabVariant::M3Hyd15 ? 1500.f : Variant == ELabVariant::M3Hyd50 ? 5000.f : 3000.f;
		const float Bands[EMassLOD::Max] = { 0.f, HydrateCm, 8000.f, FLabConfig::Get().FarCm() }; // Off 거리: 기본 200m, -LabFarM=으로 옮긴다
		const int32 MaxCounts[EMassLOD::Max] = { 500, 5000, MAX_int32, MAX_int32 }; // 확정값 LODMaxCount
		for (int32 LOD = 0; LOD < EMassLOD::Max; ++LOD)
		{
			Visual->LODParams.BaseLODDistance[LOD] = Bands[LOD];
			Visual->LODParams.VisibleLODDistance[LOD] = Bands[LOD];
			Visual->LODParams.LODMaxCount[LOD] = MaxCounts[LOD];
		}
		if (Variant == ELabVariant::VatCullVis)
		{
			Visual->LODParams.VisibleLODDistance[EMassLOD::Off] = 1.e7f; // 화면 안이면 맵 끝까지 그린다
		}
		Visual->Params.LODRepresentation[EMassLOD::Off] = EMassRepresentationType::None;
		// E3b (8) 표현 LOD 값(plan.md "M3b"). BaseLODDistance는 화면 밖(카메라 뒤 포함), VisibleLODDistance는 화면 안의 밴드 경계다. 히스테리시스 10%(엔진 기본, MassRepresentationFragments.h:309).
		// M3 대표(m3mass, 별칭 m3vis15)는 결정 1(2026-11-09, 20k·5k 빠른 회전 캡처)로 화면 밖 Base 15m, 화면 안 Visible 30m다. 그 전 대표(30/30m)는 m3vis30으로 잰다.
		// Base를 자기 정의로 고정한 변형(m3vis0 0m, m3vis30·m3hys20·m3hyd15·m3hyd50은 Visible과 같게)만 빼고 M3 변형은 모두 15m다. M2의 vatcull·vatcullvis는 bM3가 아니라 닿지 않는다.
		if (bM3)
		{
			const bool bBaseIsVisible = Variant == ELabVariant::M3Vis30 || Variant == ELabVariant::M3Hys20 || Variant == ELabVariant::M3Hyd15 || Variant == ELabVariant::M3Hyd50;
			Visual->LODParams.BaseLODDistance[EMassLOD::Medium] = Variant == ELabVariant::M3Vis0 ? 0.f : bBaseIsVisible ? HydrateCm : 1500.f;
			if (Variant == ELabVariant::M3Hys20) { Visual->LODParams.BufferHysteresisOnDistancePercentage = 20.f; }
		}
	}
	if (bM3 && Variant == ELabVariant::M3LowRes)
	{
		// E3 (5): 0~15m 액터, 15~30m 저품질 액터, 30~200m VAT 인스턴스, 200m 밖 Off. 액터 상한은 대표와 같은 500을 둘이 나눈다.
		const float Bands[EMassLOD::Max] = { 0.f, 1500.f, 3000.f, FLabConfig::Get().FarCm() };
		const int32 MaxCounts[EMassLOD::Max] = { 250, 250, MAX_int32, MAX_int32 };
		for (int32 LOD = 0; LOD < EMassLOD::Max; ++LOD)
		{
			Visual->LODParams.BaseLODDistance[LOD] = Bands[LOD];
			Visual->LODParams.VisibleLODDistance[LOD] = Bands[LOD];
			Visual->LODParams.LODMaxCount[LOD] = MaxCounts[LOD];
		}
		Visual->LowResTemplateActor = ALabMinionMassActorLow::StaticClass();
		Visual->Params.LODRepresentation[EMassLOD::Medium] = EMassRepresentationType::LowResSpawnedActor;
	}

	if (FLabConfig::Get().UsesIskm())
	{
		// M5(plan.md "M5" 결정 2·3·6): 정적 메시 인스턴스 자리(30~200m)를 ISKM으로 바꾼다. 엔진 표현 경로(LOD, 액터 전환)는 그대로고 종류마다 ISKM 하나다
		// (컴포넌트는 메시 설명자 해시마다 하나, MassVisualizationComponent.cpp:1299-1312). 정적 메시 설명자는 비워 ISM 컴포넌트를 만들지 않는다.
		const int32 AssetType = FLabUnitTypes::AssetType(UnitType);
		const FLabIskmParams IskmParams = FLabConfig::Get().IskmParams();
		FMassSkinnedMeshInstanceVisualizationMeshDesc& SkinnedDesc = Visual->SkinnedMeshInstanceDesc.Meshes.AddDefaulted_GetRef();
		SkinnedDesc.Asset = IskmMeshes.IsValidIndex(AssetType) ? IskmMeshes[AssetType].Get() : nullptr;
		SkinnedDesc.TransformProvider = IskmProviders.IsValidIndex(AssetType) ? IskmProviders[AssetType].Get() : nullptr; // ASTPD면 엔진이 컴포넌트마다 인스턴스를 만든다(:1323-1325)
		SkinnedDesc.LocalTransform = MeshDesc.LocalTransform; // VAT와 같은 -90도 회전과 종류 배율
		SkinnedDesc.bCastShadows = IskmParams.bCastShadows;
		SkinnedDesc.AnimationMinScreenSize = IskmParams.AnimationMinScreenSize; // 컴포넌트에 SetAnimationMinScreenSize로 넘어간다(:1333)
		Visual->StaticMeshInstanceDesc.Meshes.Reset();
		for (int32 LOD = 0; LOD < EMassLOD::Max; ++LOD)
		{
			if (Visual->Params.LODRepresentation[LOD] == EMassRepresentationType::StaticMeshInstance)
			{
				Visual->Params.LODRepresentation[LOD] = EMassRepresentationType::SkinnedMeshInstance;
			}
		}
		UE_LOG(LogLab, Log, TEXT("lab_iskm_config type=%d mesh=%s provider=%s shadow=%d minss=%.2f blend=%.2f"), UnitType, *GetNameSafe(SkinnedDesc.Asset.Get()),
			*GetNameSafe(SkinnedDesc.TransformProvider.Get()), SkinnedDesc.bCastShadows ? 1 : 0, SkinnedDesc.AnimationMinScreenSize, IskmParams.BlendTime);
	}

	UE_LOG(LogLab, Log, TEXT("lab_mass_config type=%d traits=%d mesh=%s actor=%s"), UnitType, EntityConfig->GetConfig().GetTraits().Num(), *GetNameSafe(Mesh), *GetNameSafe(Visual->HighResTemplateActor.Get()));
	return EntityConfig;
}

void ULabCrowdSubsystem::SpawnEntities(TConstArrayView<FVector> Locations, TConstArrayView<uint32> Serials)
{
	check(bActive && Locations.Num() == Serials.Num());
	UWorld* World = GetWorld();
	UMassSpawnerSubsystem* SpawnerSubsystem = World->GetSubsystem<UMassSpawnerSubsystem>();
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	// 종류별로 나눠 각 템플릿으로 만든다(2종, LabUnitAssets.h). 1종이면 모두 [0]이라 전과 같은 한 번의 생성이다.
	TArray<int32> IndicesByType[LabUnitTypeMax];
	for (int32 i = 0; i < Serials.Num(); ++i)
	{
		IndicesByType[FMath::Min(FLabUnitTypes::TypeOf(Serials[i]), EntityConfigs.Num() - 1)].Add(i);
#if !UE_BUILD_TEST
		// 개발 확인(lab_pool_check): 일련번호 순서의 위치 열. 위치는 스포너 스트림에서 일련번호 순으로 뽑히므로 풀링을 켜든 끄든 같은 일련번호까지의 해시가 같아야 한다.
		SpawnCheckHash = HashCombineFast(SpawnCheckHash, HashCombineFast(::GetTypeHash(Serials[i]), HashCombineFast(::GetTypeHash(FMath::RoundToInt32(Locations[i].X)), ::GetTypeHash(FMath::RoundToInt32(Locations[i].Y)))));
		if (Serials[i] > 0 && Serials[i] % 200u == 0u)
		{
			UE_LOG(LogLab, Log, TEXT("lab_pool_check serial=%u hash=%08x x=%.0f y=%.0f pool=%d reused=%d created=%d"), Serials[i], SpawnCheckHash, Locations[i].X, Locations[i].Y,
				FLabConfig::Get().bEntityPool ? 1 : 0, PoolReused, PoolCreated);
		}
#endif
	}
	const bool bPool = FLabConfig::Get().bEntityPool;
	TArray<FMassEntityHandle> Reused;
	for (int32 Type = 0; Type < EntityConfigs.Num(); ++Type)
	{
		const TArray<int32>& Indices = IndicesByType[Type];
		if (Indices.Num() == 0)
		{
			continue;
		}
		// M5 엔티티 풀링: 같은 종류의 풀 엔티티를 먼저 다시 쓴다(종류마다 아키타입과 표현이 달라 다른 종류는 쓸 수 없다). 위치·일련번호는 새로 만들 때와 같다.
		const int32 NumReuse = bPool ? FMath::Min(EntityPool[Type].Num(), Indices.Num()) : 0;
		for (int32 k = 0; k < NumReuse; ++k)
		{
			const int32 i = Indices[k];
			const FMassEntityHandle Entity = EntityPool[Type].Pop(EAllowShrinking::No);
			ReinitPooledEntity(EntityManager, Entity, Locations[i], Serials[i]);
			Reused.Add(Entity);
		}
		if (bPool)
		{
			PoolReused += NumReuse;
			PoolCreated += Indices.Num() - NumReuse;
		}
		AliveCount += NumReuse;
		if (NumReuse == Indices.Num())
		{
			continue;
		}
		const FMassEntityTemplate& Template = EntityConfigs[Type]->GetOrCreateEntityTemplate(*World);
		TArray<FMassEntityHandle> Entities;
		TSharedPtr<FMassEntityManager::FEntityCreationContext> Creation = SpawnerSubsystem->SpawnEntities(Template, Indices.Num() - NumReuse, Entities);
		// 생성 직후 초기화(생성 컨텍스트 안, 게임 스레드). 처리 중 접근이 아니라 쿼리 밖 접근을 그대로 둔다(Docs/Worklog/M3b.md 표).
		for (int32 k = 0; k < Entities.Num(); ++k)
		{
			const int32 i = Indices[NumReuse + k];
			const FVector Facing = (OrbCenter - Locations[i]).GetSafeNormal2D();
			EntityManager.GetFragmentDataChecked<FTransformFragment>(Entities[k]).SetTransform(FTransform(Facing.Rotation(), Locations[i]));
			EntityManager.GetFragmentDataChecked<FLabMinionFragment>(Entities[k]).Serial = Serials[i];
			EntityManager.GetFragmentDataChecked<FLabHealthFragment>(Entities[k]).Health = FLabScenario::MinionHealth;
			EntityManager.GetFragmentDataChecked<FLabAnimStateFragment>(Entities[k]).Set(ELabMinionState::Walk, -LabWalkPhase(Serials[i])); // 재생 시각 = Now + 위상
		}
		Creation.Reset(); // 옵저버(엔진 초기화 프로세서)가 여기서 돈다
		AliveCount += Entities.Num();
	}
	if (Reused.Num() > 0)
	{
		// 다시 쓴 엔티티의 태그를 한 번에 바꾼다: 사망·풀 태그를 떼고(피격 태그는 사망 때 이미 떨어졌다) 엔진 이동을 다시 켠다(StopMoving의 반대).
		FMassTagBitSet ToAdd;
		ToAdd.Add<FMassCodeDrivenMovementTag>();
		FMassTagBitSet ToRemove;
		ToRemove.Add<FLabDeadTag>();
		ToRemove.Add<FLabPooledTag>();
		ToRemove.Add<FLabHitTag>();
		TArray<FMassArchetypeEntityCollection> Collections;
		UE::Mass::Utils::CreateEntityCollections(EntityManager, Reused, FMassArchetypeEntityCollection::NoDuplicates, Collections);
		EntityManager.BatchChangeTagsForEntities(Collections, ToAdd, ToRemove);
	}
}

namespace
{
	/**
	 * 풀 엔티티를 세워 두는 곳. 카메라(맵 중심 근처)에서 그리는 거리와 Simulation LOD Off 거리보다 훨씬 멀어 표현 LOD가 Off(그리지 않음)다.
	 * 그러면 엔진 표현 경로가 액터를 풀에 돌려주고 ISM·ISKM 인스턴스를 뗀다(움직이는 인스턴스는 갱신되지 않으면 지워진다, MassVisualizationComponent.cpp:812-827).
	 * ISKM 트랙은 엔티티 핸들이 키라 남아 있다가 같은 엔티티가 다시 보일 때 다시 쓰인다(:761-770). 로직 프로세서는 사망 태그로 이미 거른다.
	 */
	const FVector LabPoolParkLocation(1.0e6, 1.0e6, -1.0e5);
}

void ULabCrowdSubsystem::ReinitPooledEntity(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const FVector& Location, uint32 Serial) const
{
	// SpawnEntities의 새 엔티티 초기화와 같은 값에, 사망 동안 바뀐 이동 상태를 템플릿 기본값으로 되돌린다.
	const FVector Facing = (OrbCenter - Location).GetSafeNormal2D();
	EntityManager.GetFragmentDataChecked<FTransformFragment>(Entity).SetTransform(FTransform(Facing.Rotation(), Location));
	FLabMinionFragment& Minion = EntityManager.GetFragmentDataChecked<FLabMinionFragment>(Entity);
	Minion.Serial = Serial;
	Minion.StateTimeLeft = 0.f;
	EntityManager.GetFragmentDataChecked<FLabHealthFragment>(Entity).Health = FLabScenario::MinionHealth;
	EntityManager.GetFragmentDataChecked<FLabAnimStateFragment>(Entity).Set(ELabMinionState::Walk, -LabWalkPhase(Serial)); // 재생 시각 = Now + 위상. ISKM 트랙은 이 값이 바뀐 것을 보고 다시 쓴다
	if (FMassVelocityFragment* Velocity = EntityManager.GetFragmentDataPtr<FMassVelocityFragment>(Entity)) { Velocity->Value = FVector::ZeroVector; }
	if (FMassForceFragment* Force = EntityManager.GetFragmentDataPtr<FMassForceFragment>(Entity)) { Force->Value = FVector::ZeroVector; }
	if (FMassDesiredMovementFragment* Desired = EntityManager.GetFragmentDataPtr<FMassDesiredMovementFragment>(Entity)) { Desired->DesiredVelocity = FVector::ZeroVector; }
	if (FLabOffMoveFragment* OffMove = EntityManager.GetFragmentDataPtr<FLabOffMoveFragment>(Entity)) { OffMove->LastMoveTime = 0.; } // 새 엔티티와 같이 첫 이동은 한 프레임만(LabMassProcessors.cpp)
}

void ULabCrowdSubsystem::RecycleAndRespawn(TConstArrayView<FMassEntityHandle> Finished)
{
	UWorld* World = GetWorld();
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	// 1. 종류별 풀 뒤에 넣는다. 재생성이 뒤에서 꺼내므로 같은 종류면 방금 죽은 엔티티가 바로 다시 쓰여 세워 두는 단계를 건너뛴다.
	const int32 OldNum[LabUnitTypeMax] = { EntityPool[0].Num(), EntityPool[1].Num() };
	int32 Count = 0;
	for (const FMassEntityHandle Entity : Finished)
	{
		if (!EntityManager.IsEntityActive(Entity))
		{
			continue;
		}
		const uint32 Serial = EntityManager.GetFragmentDataChecked<FLabMinionFragment>(Entity).Serial;
		EntityPool[FMath::Min(FLabUnitTypes::TypeOf(Serial), EntityConfigs.Num() - 1)].Add(Entity);
		++Count;
	}
	// 2. 파괴 경로와 같은 재생성(같은 스트림, 같은 일련번호 열). SpawnEntities가 풀을 먼저 쓴다.
	RequestRespawn(Count);
	// 3. 다시 쓰이지 않고 남은 이번 엔티티만 세워 두고 풀 태그를 붙인다(종류가 맞지 않아 남은 것).
	TArray<FMassEntityHandle> Parked;
	for (int32 Type = 0; Type < LabUnitTypeMax; ++Type)
	{
		for (int32 k = OldNum[Type]; k < EntityPool[Type].Num(); ++k)
		{
			const FMassEntityHandle Entity = EntityPool[Type][k];
			EntityManager.GetFragmentDataChecked<FTransformFragment>(Entity).SetTransform(FTransform(LabPoolParkLocation));
			Parked.Add(Entity);
		}
	}
	if (Parked.Num() > 0)
	{
		TArray<FMassArchetypeEntityCollection> Collections;
		UE::Mass::Utils::CreateEntityCollections(EntityManager, Parked, FMassArchetypeEntityCollection::NoDuplicates, Collections);
		FMassTagBitSet ToAdd;
		ToAdd.Add<FLabPooledTag>();
		EntityManager.BatchChangeTagsForEntities(Collections, ToAdd, FMassTagBitSet());
	}
}

void ULabCrowdSubsystem::RequestRespawn(int32 Count)
{
	AliveCount -= Count;
	if (ALabCrowdSpawner* S = Spawner.Get())
	{
		S->RespawnEntities(Count);
	}
}

void ULabCrowdSubsystem::ApplyFixedIskmBounds(UInstancedSkinnedMeshComponent& Component)
{
	// 그리는 끝(히스테리시스 10%, MassRepresentationFragments.h:309) + 카메라가 오브에서 떨어진 거리 + 여유. 오브는 움직이지 않는다. 높이는 대형의 죽기 자세까지 덮는다.
	// 월드 좌표다: UpdateBounds는 CalcBounds(컴포넌트 변환)을 부르고, 그때 InverseTransformBy(GetComponentTransform().Inverse() * LocalToWorld)가 항등이라 값이 그대로 나온다(InstancedSkinnedMeshComponent.cpp:1249-1253).
	const float HalfXY = FLabConfig::Get().FarCm() * 1.1f + FLabScenario::OrbViewBack + 1000.f;
	const FBox Fixed(OrbCenter + FVector(-HalfXY, -HalfXY, -500.f), OrbCenter + FVector(HalfXY, HalfXY, 1500.f));
	if (!Component.GetPrimitiveBoundsOverride().IsValid)
	{
		// 컴포넌트는 ConstructSkinnedMeshComponents에서 늦게 생기므로 찾은 첫 틱에 넣는다. 인스턴스 갱신 때의 UpdateBounds도 이 값을 쓴다(:494-497)
		Component.SetPrimitiveBoundsOverride(Fixed);
		Component.UpdateBounds();
	}
	// 인스턴스가 들어온 뒤에도 실제 바운드가 고정값인지 컴포넌트마다 한 번 확인해 남긴다.
	if (Component.GetInstanceCount() > 0 && !IskmBoundsConfirmed.Contains(&Component) && Component.Bounds.GetBox().Equals(Fixed, 1.0))
	{
		IskmBoundsConfirmed.Add(&Component);
		UE_LOG(LogLab, Log, TEXT("lab_iskm component=%s bounds_override=1 instances=%d half_xy=%.0f identity=%d"), *Component.GetName(), Component.GetInstanceCount(), HalfXY,
			Component.GetComponentTransform().Equals(FTransform::Identity) ? 1 : 0);
	}
}

void ULabCrowdSubsystem::ExcludeVisualsFromLumen()
{
	// ISM 컴포넌트는 표현 프로세서가 필요할 때 만든다. 처음 한 개만 고치면 뒤에 생긴 컴포넌트가 빠져서(2026-11-08 그림자 캐시 수정이
	// 듣지 않았다), 매 틱 컴포넌트를 훑어 아직 설정되지 않은 것만 고친다(비주얼라이저 하나, 컴포넌트 몇 개).
	for (TActorIterator<AMassVisualizer> It(GetWorld()); It; ++It)
	{
		const bool bFixedIskmBounds = FLabConfig::Get().UsesIskm() && FLabConfig::Get().IskmParams().bFixedBounds; // m5bounds만
	// M5: ISKM 컴포넌트도 같은 비주얼라이저 밑에 생긴다(MassVisualizationComponent.cpp:1305). 스켈레탈 메시는 원래 Lumen 장면 밖이지만(engine-notes.md) 같은 설정을 맞춘다.
		TArray<UPrimitiveComponent*> Components;
		It->GetComponents(Components);
		for (UPrimitiveComponent* Component : Components)
		{
			if (!Component->IsA<UInstancedStaticMeshComponent>() && !Component->IsA<UInstancedSkinnedMeshComponent>())
			{
				continue;
			}
			if (bFixedIskmBounds)
			{
				if (UInstancedSkinnedMeshComponent* FixedIskm = Cast<UInstancedSkinnedMeshComponent>(Component))
				{
					ApplyFixedIskmBounds(*FixedIskm);
				}
			}
			const bool bWantAlways = false; // 그림자 캐시 남음은 VAT 메시 바운드로 고쳤다(Scripts/bake-vat.py BOUNDS_MARGIN). Always는 매 프레임 무효화라 비용이 커서 끈다
			if (!Component->bAffectDynamicIndirectLighting && (!bWantAlways || Component->ShadowCacheInvalidationBehavior == EShadowCacheInvalidationBehavior::Always))
			{
				continue; // 이미 고친 컴포넌트
			}
			LabExcludeFromLumen(*Component);
			const UInstancedStaticMeshComponent* Ism = Cast<UInstancedStaticMeshComponent>(Component);
			const UInstancedSkinnedMeshComponent* Iskm = Cast<UInstancedSkinnedMeshComponent>(Component);
			UE_LOG(LogLab, Log, TEXT("lab_ism_setup %s instances=%d"), *Component->GetName(), Ism ? Ism->GetInstanceCount() : Iskm ? Iskm->GetInstanceCount() : 0);
			if (bWantAlways)
			{
				// VAT 인스턴스는 셰이더(WPO)로 모양이 바뀐다. 죽기 애니 중에는 제자리에 멈춰 위치 변화가 없어서 Virtual Shadow Map이
				// 그림자를 캐시해 쓰러지기 전 자세(칼날 끝)의 그림자가 바닥에 남았다(2026-11-08 사용자 관찰, M2 N=1000).
				// 엔진 설명대로 "시스템이 모르는 방식으로 움직이는" 프리미티브라 항상 다시 그리게 한다(Engine/Public/SceneTypes.h:222-235).
				Component->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Always;
				Component->MarkRenderStateDirty();
			}
			bLumenExcluded = true;
		}
	}
}
