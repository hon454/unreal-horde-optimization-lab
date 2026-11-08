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

	/**
	 * 클립을 이 시각(초)부터 재생한다. AnimBP의 시퀀스 플레이어 Start Position 핀이 이 값에 묶여 있고(Scripts/bind-anim-start.md),
	 * 시퀀스 플레이어는 초기화될 때 Start Position부터 재생한다(Engine/Private/Animation/AnimNode_SequencePlayer.cpp:61).
	 * 상태가 바뀌는 보통의 경우는 0, M3에서 VAT 인스턴스가 액터로 바뀌는 순간에는 VAT가 재생하던 시각을 넣는다.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Lab")
	float StartTime = 0.f;
};
