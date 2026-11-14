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
	// 몸통 반지름(v2 W3): 종류별 BoundRadius(소형 40, 대형 94)다. 엔진 회피와 겹친 쌍의 수(ULabOverlapCountProcessor)가 같은 값을 쓴다. W3 전에는 둘 다 40이었다
	BuildContext.AddFragment_GetRef<FAgentRadiusFragment>().Radius = FLabUnitAssetPaths::Get(UnitType).BoundRadius;
	BuildContext.AddFragment<FMassActorFragment>();                        // UMassVisualizationTrait가 요구한다(MassVisualizationTrait.cpp RequireFragment)
	BuildContext.AddFragment<FLabMinionFragment>();
	BuildContext.AddFragment<FLabHealthFragment>();
	BuildContext.AddFragment<FLabAnimStateFragment>(); // M3b: 표현 전용 상태
	BuildContext.AddTag<FLabMinionTag>();
	if (FLabConfig::Get().UsesSimLod() && FLabConfig::Get().SimLod().bOffMove)
	{
		BuildContext.AddFragment<FLabOffMoveFragment>(); // E4 (d) bOffMove(m4offmove·m4offpool과 M5 단계)만. 다른 변형의 아키타입은 그대로다
	}
	if (FLabConfig::Get().UsesIskm())
	{
		BuildContext.AddFragment<FLabIskmTrackFragment>(); // M5 ISKM만: 트랙에 마지막으로 쓴 상태(ULabIskmProcessor)
	}

	FLabUnitTypeParams Params;
	Params.UnitType = UnitType;
	Params.MoveSpeed = FLabScenario::MoveSpeed;
	Params.MaxHealth = FLabScenario::MinionHealth;
	Params.BoundRadius = FLabUnitAssetPaths::Get(UnitType).BoundRadius; // 소형 40(전과 같다), 대형은 ini LargeBoundRadius(확정값 94). FAgentRadiusFragment도 같은 값이다(v2 W3)
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
	if (Sim.bOffMove)
	{
		// E4 (d) m4offmove: 200m 밖을 Low 대신 Off로 둔다. 엔진이 FMassOffLODTag를 붙여 이동·조향 넷이 이 개체를 돌지 않는다(engine-notes.md 5절).
		// Low와 Off의 거리가 같으면 계산기가 Off를 먼저 고른다(MassLODCalculator.h:867-870). Off → Medium에는 히스테리시스가 없다(:873, 한 단계 차이에만 있다).
		// 새 엔티티는 청크에 들어가며 청크 수정 번호를 바꾸므로 간격을 기다리지 않고 다음 갱신에 돈다(MassLODTickRateController.h:153).
		// -LabFarM=(기본 200m)이 표현 LOD의 Off 거리와 이 거리를 함께 옮긴다. 기본값이면 지금까지와 같이 Low 거리와 같다.
		// 200m보다 크면 200m~그 거리는 Low 밴드(로직 1.0초, 이동은 엔진)가 된다. 그때 Off → Low는 한 단계라 히스테리시스가 붙어 Lerp(Low, Off, 0.9)에서 넘어간다(MassLODCalculator.h:873-880).
		// 새 엔티티도 Off에서 시작해(MassLODTrait.cpp:128-133) 같은 히스테리시스를 받는데, 표현 LOD는 PrevLOD가 Max에서 시작해 Off 거리 안이면 바로 그린다(MassRepresentationFragments.h:37).
		// 그래서 Off를 떠나는 거리(히스테리시스 점)가 그리는 거리 + 10m가 되게 Off 거리를 바깥으로 민다. 10m는 Off 청크의 LOD 재계산이 늦는 거리(최대 4.7m, Docs/Worklog/M4.md)의 두 배다.
		// 300m면 Off 거리 322m(떠나는 거리 310m), 500m면 544m(510m, 카메라에서 가장 먼 개체가 515m라 Off 밴드가 거의 빈다).
		const float FarCm = FLabConfig::Get().FarCm();
		Params.LODDistance[EMassLOD::Off] = FarCm <= FLabSimLod::LowDistance ? FLabSimLod::LowDistance
			: FLabSimLod::LowDistance + (FarCm + 1000.f - FLabSimLod::LowDistance) / (1.f - Params.BufferHysteresisOnDistancePercentage / 100.f);
		VariableTickParams.TickRates[EMassLOD::Off] = Sim.TickLow;
	}
}

ULabMovementTrait::ULabMovementTrait()
{
	// 시나리오 이동 속도(3m/s)로 곧게 간다. 가속은 M0 CharacterMovement 기본값(MaxAcceleration 2048)에 맞춘다.
	Movement.MaxSpeed = FLabScenario::MoveSpeed;
	Movement.MaxAcceleration = 2048.f;
	Movement.DefaultDesiredSpeed = FLabScenario::MoveSpeed;
	Movement.DefaultDesiredSpeedVariance = 0.f;
}
