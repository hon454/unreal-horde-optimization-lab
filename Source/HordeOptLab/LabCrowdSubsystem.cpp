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
		const float Bands[EMassLOD::Max] = { 0.f, HydrateCm, 8000.f, 20000.f };
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
	// 생성 직후 초기화(생성 컨텍스트 안, 게임 스레드). 처리 중 접근이 아니라 쿼리 밖 접근을 그대로 둔다(Docs/Worklog/M3b.md 표).
	for (int32 i = 0; i < Entities.Num(); ++i)
	{
		const FVector Facing = (OrbCenter - Locations[i]).GetSafeNormal2D();
		EntityManager.GetFragmentDataChecked<FTransformFragment>(Entities[i]).SetTransform(FTransform(Facing.Rotation(), Locations[i]));
		EntityManager.GetFragmentDataChecked<FLabMinionFragment>(Entities[i]).Serial = Serials[i];
		EntityManager.GetFragmentDataChecked<FLabHealthFragment>(Entities[i]).Health = FLabScenario::MinionHealth;
		EntityManager.GetFragmentDataChecked<FLabAnimStateFragment>(Entities[i]).Set(ELabMinionState::Walk, -LabWalkPhase(Serials[i])); // 재생 시각 = Now + 위상
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
	// ISM 컴포넌트는 표현 프로세서가 필요할 때 만든다. 처음 한 개만 고치면 뒤에 생긴 컴포넌트가 빠져서(2026-11-08 그림자 캐시 수정이
	// 듣지 않았다), 매 틱 컴포넌트를 훑어 아직 설정되지 않은 것만 고친다(비주얼라이저 하나, 컴포넌트 몇 개).
	for (TActorIterator<AMassVisualizer> It(GetWorld()); It; ++It)
	{
		TArray<UInstancedStaticMeshComponent*> Components;
		It->GetComponents(Components);
		for (UInstancedStaticMeshComponent* Component : Components)
		{
			const bool bWantAlways = false; // 그림자 캐시 남음은 VAT 메시 바운드로 고쳤다(Scripts/bake-vat.py BOUNDS_MARGIN). Always는 매 프레임 무효화라 비용이 커서 끈다
			if (!Component->bAffectDynamicIndirectLighting && (!bWantAlways || Component->ShadowCacheInvalidationBehavior == EShadowCacheInvalidationBehavior::Always))
			{
				continue; // 이미 고친 컴포넌트
			}
			LabExcludeFromLumen(*Component);
			UE_LOG(LogLab, Log, TEXT("lab_ism_setup %s instances=%d"), *Component->GetName(), Component->GetInstanceCount());
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
