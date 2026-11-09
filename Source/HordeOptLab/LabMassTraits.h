#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "Movement/MassMovementTrait.h"
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
