#include "LabMassTraits.h"
#include "LabConfig.h"
#include "LabMassTypes.h"
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
	Params.MoveSpeed = FLabScenario::MoveSpeed;
	Params.MaxHealth = FLabScenario::MinionHealth;
	Params.BoundRadius = 40.f;
	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(Params));
}

ULabMovementTrait::ULabMovementTrait()
{
	// 시나리오 이동 속도(3m/s)로 곧게 간다. 가속은 M0 CharacterMovement 기본값(MaxAcceleration 2048)에 맞춘다.
	Movement.MaxSpeed = FLabScenario::MoveSpeed;
	Movement.MaxAcceleration = 2048.f;
	Movement.DefaultDesiredSpeed = FLabScenario::MoveSpeed;
	Movement.DefaultDesiredSpeedVariance = 0.f;
}
