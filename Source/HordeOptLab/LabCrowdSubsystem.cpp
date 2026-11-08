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

void ULabCrowdSubsystem::Activate(ALabCrowdSpawner* InSpawner, UStaticMesh* Mesh, float InHitDuration, float InDeathDuration)
{
	Spawner = InSpawner;
	HitDuration = InHitDuration;
	DeathDuration = InDeathDuration;

	// EntityConfig를 실행 시 만든다(ADR-0005). 소유자가 있으면 ConfigGuid가 경로로 결정적으로 정해져 실행마다 같은 템플릿이 된다
	// (MassSpawner/Private/MassEntityConfigAsset.cpp:33, 템플릿 ID는 :137).
	EntityConfig = NewObject<UMassEntityConfigAsset>(this, TEXT("LabMinionConfig"));
	AddLabTrait<ULabMinionTrait>(*EntityConfig);
	AddLabTrait<ULabMovementTrait>(*EntityConfig);
	AddLabTrait<UMassSteeringTrait>(*EntityConfig);
	AddLabTrait<UMassSmoothOrientationTrait>(*EntityConfig);
	AddLabTrait<UMassLODCollectorTrait>(*EntityConfig);
	if (FLabConfig::Get().Variant == ELabVariant::MassAv)
	{
		// E1 (c): 엔진 회피. 이웃을 찾으려면 장애물 격자에 등록돼야 하므로 두 Trait을 함께 붙인다. 파라미터는 엔진 기본값.
		AddLabTrait<UMassObstacleAvoidanceTrait>(*EntityConfig);
		AddLabTrait<UMassNavigationObstacleTrait>(*EntityConfig);
	}

	// 표현: 애니 없는 ISM 하나. 밴드 없이 모든 LOD에서 같은 ISM이다(plan.md "M1"). 거리 밴드는 M2에서 켠다.
	UMassMovableVisualizationTrait* Visual = AddLabTrait<UMassMovableVisualizationTrait>(*EntityConfig);
	FMassStaticMeshInstanceVisualizationMeshDesc& MeshDesc = Visual->StaticMeshInstanceDesc.Meshes.AddDefaulted_GetRef();
	MeshDesc.Mesh = Mesh;
	MeshDesc.LocalTransform = FTransform(FRotator(0.f, -90.f, 0.f)); // M0 액터와 같이 메시를 -90도 돌려 진행 방향을 보게 한다(LabMinion.cpp)
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
		const bool bBudget = Variant == ELabVariant::M3BudgetDist || Variant == ELabVariant::M3BudgetSig;
		Visual->HighResTemplateActor = bBudget ? ALabMinionMassActorBudget::StaticClass() : ALabMinionMassActor::StaticClass();
		Visual->Params.LODRepresentation[EMassLOD::High] = EMassRepresentationType::HighResSpawnedActor;
		// 전환 팝핑 대책(2026-11-08 사용자 결정). VAT → 액터: 액터의 포즈가 준비될 때까지 VAT 인스턴스를 그린다.
		// 액터 → VAT: 액터를 한 프레임 더 남긴다(엔진 군중 기본값도 켠다, MassCrowdVisualizationTrait.cpp:22). bKeepLowResActors가 켜져 있어야 동작한다(MassRepresentationProcessor.h:27-30).
		Visual->Params.RepresentationActorManagementClass = ULabActorManagement::StaticClass();
		Visual->Params.bWaitForActorVisualReadiness = FLabConfig::Get().HydrateFix >= 2;
		Visual->Params.bKeepActorExtraFrame = FLabConfig::Get().HydrateFix >= 2;
	}
	if (bM3 || Variant == ELabVariant::VatCull || Variant == ELabVariant::VatCullVis)
	{
		const float Bands[EMassLOD::Max] = { 0.f, 3000.f, 8000.f, 20000.f };
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
	}
	if (bM3 && Variant == ELabVariant::M3LowRes)
	{
		// E3 (5): 0~15m 액터, 15~30m 저품질 액터, 30~200m VAT 인스턴스, 200m 밖 Off. 액터 상한은 대표와 같은 500을 둘이 나눈다.
		const float Bands[EMassLOD::Max] = { 0.f, 1500.f, 3000.f, 20000.f };
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

	bActive = true;
	UE_LOG(LogLab, Log, TEXT("lab_mass_config traits=%d mesh=%s"), EntityConfig->GetConfig().GetTraits().Num(), *GetNameSafe(Mesh));
}

void ULabCrowdSubsystem::SpawnEntities(TConstArrayView<FVector> Locations, TConstArrayView<uint32> Serials)
{
	check(bActive && Locations.Num() == Serials.Num());
	UWorld* World = GetWorld();
	UMassSpawnerSubsystem* SpawnerSubsystem = World->GetSubsystem<UMassSpawnerSubsystem>();
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	const FMassEntityTemplate& Template = EntityConfig->GetOrCreateEntityTemplate(*World);

	TArray<FMassEntityHandle> Entities;
	TSharedPtr<FMassEntityManager::FEntityCreationContext> Creation = SpawnerSubsystem->SpawnEntities(Template, Locations.Num(), Entities);
	for (int32 i = 0; i < Entities.Num(); ++i)
	{
		const FVector Facing = (OrbCenter - Locations[i]).GetSafeNormal2D();
		EntityManager.GetFragmentDataChecked<FTransformFragment>(Entities[i]).SetTransform(FTransform(Facing.Rotation(), Locations[i]));
		EntityManager.GetFragmentDataChecked<FLabMinionFragment>(Entities[i]).Serial = Serials[i];
		EntityManager.GetFragmentDataChecked<FLabHealthFragment>(Entities[i]).Health = FLabScenario::MinionHealth;
	}
	Creation.Reset(); // 옵저버(엔진 초기화 프로세서)가 여기서 돈다
	AliveCount += Entities.Num();
}

void ULabCrowdSubsystem::RequestRespawn(int32 Count)
{
	AliveCount -= Count;
	if (ALabCrowdSpawner* S = Spawner.Get())
	{
		S->RespawnEntities(Count);
	}
}

void ULabCrowdSubsystem::ExcludeVisualsFromLumen()
{
	if (bLumenExcluded)
	{
		return;
	}
	for (TActorIterator<AMassVisualizer> It(GetWorld()); It; ++It)
	{
		TArray<UInstancedStaticMeshComponent*> Components;
		It->GetComponents(Components);
		for (UInstancedStaticMeshComponent* Component : Components)
		{
			LabExcludeFromLumen(*Component);
			bLumenExcluded = true;
		}
	}
}
