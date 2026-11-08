#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LabMinionAnimInstance.h"
#include "MassActorPoolableInterface.h"
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
 * 미니언 표현을 Lumen 장면에서 뺀다(모든 단계 공통, 2026-11-07 사용자 결정). 소프트웨어 Lumen은 "동적 간접광에 영향"이 켜져 있고
 * 거리 필드가 있는 메시만 장면에 넣는다(Engine/Private/PrimitiveSceneProxy.cpp:1701-1734 UpdateVisibleInLumenScene).
 * 스켈레탈 메시는 거리 필드가 없어 원래 빠져 있으므로 정적 메시 표현(M0 static, M1부터의 ISM)도 같은 조건으로 맞춘다.
 * 두 조건 중 하나만 꺼도 결과가 같아(M1 N=2000 시험: 간접광만 끔 GPU 6.0ms, 거리 필드만 끔 5.8ms, 둘 다 켬 29.0ms) 뜻이 맞는 간접광 쪽만 끈다.
 * 미니언은 여전히 Lumen GI를 받고 그림자를 그린다.
 */
inline void LabExcludeFromLumen(UPrimitiveComponent& Component)
{
	Component.SetAffectDynamicIndirectLighting(false);
}

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

/**
 * M3 하이드레이션 액터. Mass가 0~30m 엔티티에 붙였다 떼는 표현일 뿐이고 로직은 엔티티에 있다(plan.md "M3").
 * pawn 변형과 같은 부품(캡슐, 스켈레탈 메시, ABP_LabMinion, CharacterMovement 틱 끔)이라 E0 수치와 견줄 수 있다.
 * 위치·애니 상태·죽기 색은 ULabActorSyncProcessor가 매 프레임 쓴다. 액터 자신은 틱하지 않는다.
 * 머티리얼은 VAT 인스턴스와 같은 M_LabMinionActor(Scripts/bake-vat.py)라 전환 때 색이 튀지 않는다.
 * 엔진 풀은 액터를 숨기기만 하고 컴포넌트 틱은 남겨 두므로(engine-notes.md 4절) 풀에 들어갈 때 메시 틱을 끈다.
 */
UCLASS()
class ALabMinionMassActor : public ALabMinionPawn, public IMassActorPoolableInterface
{
	GENERATED_BODY()
public:
	ALabMinionMassActor(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;

	virtual bool CanBePooled_Implementation() override { return true; }
	virtual void PrepareForPooling_Implementation() override;
	virtual void PrepareForGame_Implementation() override;

	// 동기화 프로세서가 쓰는 상태. 엔티티가 바뀌면(새로 붙음, 풀에서 다시 씀) 애니를 처음부터 맞춘다.
	uint32 BoundSerial = MAX_uint32;
	ELabMinionState AppliedState = ELabMinionState::Walk;
	float AppliedDeadAmount = 0.f;
};

/**
 * E3 (3): 메시가 USkeletalMeshComponentBudgeted인 하이드레이션 액터. 중요도는 (a) Significance Manager 거리 단계(M0 budget과 같다) 또는
 * (b) 동기화 프로세서가 넘기는 Mass LODSignificance 변환값. 풀에 들어가면 중요도 0(숨긴 액터가 예산을 쓰지 않게, engine-notes.md 4절).
 */
UCLASS()
class ALabMinionMassActorBudget : public ALabMinionMassActor
{
	GENERATED_BODY()
public:
	ALabMinionMassActorBudget(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void PrepareForPooling_Implementation() override;
	virtual void PrepareForGame_Implementation() override;
};

/** E3 (5): 저품질 액터(City Sample의 LowRes 단계에 해당). URO, 가장 낮은 메시 LOD 고정, 그림자 끔. */
UCLASS()
class ALabMinionMassActorLow : public ALabMinionMassActor
{
	GENERATED_BODY()
public:
	ALabMinionMassActorLow(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
};
