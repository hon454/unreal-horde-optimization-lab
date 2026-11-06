#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LabMinionAnimInstance.generated.h"

UENUM(BlueprintType)
enum class ELabMinionState : uint8
{
	Walk,
	Hit,
	Dead,
};

/**
 * 미니언 AnimBP(/Game/Lab/Anim/ABP_LabMinion)의 부모 클래스. 상태 머신(걷기 루프 → 피격 → 죽기)은 그 AnimBP 그래프에 있고,
 * C++은 State만 쓴다. M0 full 변형과 M3 HighResActor가 같은 AnimBP를 쓴다.
 */
UCLASS(MinimalAPI)
class ULabMinionAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "Lab")
	ELabMinionState State = ELabMinionState::Walk;
};
