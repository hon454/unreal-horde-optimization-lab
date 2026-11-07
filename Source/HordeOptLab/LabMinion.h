#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LabMinionAnimInstance.h"
#include "LabMinion.generated.h"

class ALabCrowdSpawner;
class UStaticMeshComponent;

/** 스포너가 한 번 불러 모든 미니언이 나눠 쓰는 에셋. */
USTRUCT()
struct FLabMinionAssets
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<class USkeletalMesh> Mesh;
	UPROPERTY() TObjectPtr<UClass> AnimClass;           // ABP_LabMinion. 없으면 싱글 노드로 대신한다
	UPROPERTY() TObjectPtr<class UAnimSequence> Walk;
	UPROPERTY() TObjectPtr<class UAnimSequence> Hit;
	UPROPERTY() TObjectPtr<class UAnimSequence> Death;
	UPROPERTY() TObjectPtr<class UStaticMesh> StaticMesh; // static 변형
};

/**
 * M0 액터 군중의 개체. 오브로 직진하고, 소멸 반경에 들거나 저격되면 죽는 애니 뒤 사라진다.
 * AI 컨트롤러 없이 CharacterMovement가 직접 돈다(bRunPhysicsWithNoController). 개체끼리는 충돌하지 않는다(회피 없음, plan.md M1 (c)).
 */
UCLASS()
class ALabMinion : public ACharacter
{
	GENERATED_BODY()
public:
	ALabMinion(const FObjectInitializer& ObjectInitializer);

	void InitMinion(ALabCrowdSpawner* InSpawner, const FLabMinionAssets& Assets, uint32 InSerial);

	/** 오브 피해 범위. 체력이 0이 되면 죽는 애니. 죽었으면 true. */
	bool ApplyDamage(float Amount);
	/** 오브 저격. 피격 애니 뒤 죽는 애니. */
	void Snipe();

	bool IsAlive() const { return State == ELabMinionState::Walk; }
	uint32 GetSerial() const { return Serial; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void SetupVisual(const FLabMinionAssets& Assets);
	void EnterState(ELabMinionState NewState);

	UPROPERTY() TObjectPtr<ALabCrowdSpawner> Spawner;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> StaticVisual;

	const FLabMinionAssets* AssetsRef = nullptr;
	uint32 Serial = 0;
	ELabMinionState State = ELabMinionState::Walk;
	float Health = 0.f;
	float StateTimeLeft = 0.f;
	bool bSingleNode = false;
	bool bManualMove = false; // pawn 변형: CharacterMovement 틱 없이 위치를 직접 옮긴다
};

/** static 변형: 스켈레탈 메시 대신 UStaticMeshComponent, 애니메이션 없음. */
UCLASS()
class ALabMinionStatic : public ALabMinion
{
	GENERATED_BODY()
public:
	ALabMinionStatic(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void SetupVisual(const FLabMinionAssets& Assets) override;
};

/**
 * pawn 변형: CharacterMovement의 틱을 끄고 위치를 직접 옮긴다(ACharacter는 이 컴포넌트를 필수로 만든다). 캡슐·스켈레탈 메시·AnimBP는 full과 같다.
 * "액터를 최대한 가볍게 만들어도 여기까지"를 보는 액터 쪽 천장이다(2026-11-07 사용자 결정, budget·uro와 같은 대조군).
 * APawn을 새로 만들지 않고 ACharacter에서 이동 틱만 꺼서 스포너·오브 코드를 그대로 쓴다.
 */
UCLASS()
class ALabMinionPawn : public ALabMinion
{
	GENERATED_BODY()
public:
	ALabMinionPawn(const FObjectInitializer& ObjectInitializer);
};

/** budget 변형: 메시가 USkeletalMeshComponentBudgeted이고 Significance Manager 거리 단계로 중요도를 받는다. */
UCLASS()
class ALabMinionBudget : public ALabMinion
{
	GENERATED_BODY()
public:
	ALabMinionBudget(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
