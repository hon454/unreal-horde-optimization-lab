#include "LabMassTraits.h"
#include "LabConfig.h"
#include "LabMassTypes.h"
#include "LabUnitAssets.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "MassEntityUtils.h"
#include "Engine/World.h"

void ULabMinionTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	BuildContext.AddFragment<FTransformFragment>();
	BuildContext.AddFragment_GetRef<FAgentRadiusFragment>().Radius = 40.f; // M0 액터의 캡슐 반지름(ACharacter 기본값 34)과 비슷한 크기. 회피를 켜는 E1 (c)에서 쓴다
	BuildContext.AddFragment<FMassActorFragment>();                        // UMassVisualizationTrait가 요구한다(MassVisualizationTrait.cpp RequireFragment)
	BuildContext.AddFragment<FLabMinionFragment>();
	BuildContext.AddFragment<FLabHealthFragment>();
	BuildContext.AddFragment<FLabAnimStateFragment>(); // M3b: 표현 전용 상태
	BuildContext.AddTag<FLabMinionTag>();

	FLabUnitTypeParams Params;
	Params.UnitType = UnitType;
	Params.MoveSpeed = FLabScenario::MoveSpeed;
	Params.MaxHealth = FLabScenario::MinionHealth;
	Params.BoundRadius = FLabUnitAssetPaths::Get(UnitType).BoundRadius; // 소형 40(전과 같다), 대형은 ini LargeBoundRadius(임시값, 확정 전). FAgentRadiusFragment는 둘 다 40(회피가 꺼져 있어 쓰이지 않는다)
	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(Params));
}

ULabSimulationLODTrait::ULabSimulationLODTrait()
{
	const FLabSimLod Sim = FLabConfig::Get().SimLod();
	Params.LODDistance[EMassLOD::High] = 0.f;
	Params.LODDistance[EMassLOD::Medium] = FLabSimLod::MediumDistance;
	Params.LODDistance[EMassLOD::Low] = FLabSimLod::LowDistance;
	Params.LODDistance[EMassLOD::Off] = 1.e7f; // 맵(반경 500m) 밖. Off에 들지 않는다
	for (int32 LOD = 0; LOD < EMassLOD::Max; ++LOD)
	{
		Params.LODMaxCount[LOD] = MAX_int32;
	}
	bEnableVariableTicking = true; // LOD 태그도 함께 붙는다(MassLODTrait.cpp:129)
	VariableTickParams.TickRates[EMassLOD::High] = 0.f;
	VariableTickParams.TickRates[EMassLOD::Medium] = Sim.TickMedium;
	VariableTickParams.TickRates[EMassLOD::Low] = Sim.TickLow;
	VariableTickParams.TickRates[EMassLOD::Off] = 0.f; // 새 엔티티의 첫 프레임만 Off다. 기다리지 않고 바로 LOD를 받게 한다
}

ULabMovementTrait::ULabMovementTrait()
{
	// 시나리오 이동 속도(3m/s)로 곧게 간다. 가속은 M0 CharacterMovement 기본값(MaxAcceleration 2048)에 맞춘다.
	Movement.MaxSpeed = FLabScenario::MoveSpeed;
	Movement.MaxAcceleration = 2048.f;
	Movement.DefaultDesiredSpeed = FLabScenario::MoveSpeed;
	Movement.DefaultDesiredSpeedVariance = 0.f;
}
