#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "Movement/MassMovementTrait.h"
#include "MassLODTrait.h"
#include "LabMassTraits.generated.h"

/**
 * M1 미니언의 Trait. EntityConfig는 에셋이 아니라 ULabCrowdSubsystem이 실행 시 이 Trait들을 붙여 만든다(ADR-0005).
 */

/** 미니언 데이터: 위치, 반지름, 체력, 일련번호, 종류 값, 미니언 태그. 표현 Trait이 요구하는 FMassActorFragment도 여기서 더한다. */
UCLASS(meta = (DisplayName = "Lab Minion"))
class ULabMinionTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()
public:
	/** 종류(ELabUnitType). ULabCrowdSubsystem이 종류별 EntityConfig를 만들며 넣는다. FLabUnitTypeParams가 달라져 아키타입이 갈린다 */
	UPROPERTY() int32 UnitType = 0;
protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};

/** 엔진 이동 Trait. 파라미터가 protected라 하위 클래스에서 시나리오 값(FLabScenario)으로 채운다. */
UCLASS(meta = (DisplayName = "Lab Movement"))
class ULabMovementTrait : public UMassMovementTrait
{
	GENERATED_BODY()
public:
	ULabMovementTrait();
};

/**
 * M4 엔진 Simulation LOD Trait. 값이 protected라(MassLODTrait.h:47-57) 하위 클래스에서 FLabSimLod로 채운다.
 * 가변 틱 켬, 개체 수 상한 무제한, Off 밴드 없음(거리를 맵 밖에 둔다). 새 엔티티는 그래도 첫 프레임에 Off 태그로 시작하므로
 * (MassLODTrait.cpp:128-133) Off 틱 간격을 0으로 두고, 그 프레임의 위치 스냅은 게임 모듈이 UMassOffLODNavigationProcessor를 꺼서 막는다(HordeOptLab.cpp).
 */
UCLASS(meta = (DisplayName = "Lab Simulation LOD"))
class ULabSimulationLODTrait : public UMassSimulationLODTrait
{
	GENERATED_BODY()
public:
	ULabSimulationLODTrait();
};
