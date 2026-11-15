#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LabMinionAnimInstance.h"
#include "MassActorPoolableInterface.h"
#include "MassEntityHandle.h"
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
	UPROPERTY() TObjectPtr<class UStaticMesh> StaticMesh; // 사다리 단계 2·3의 정지 메시 인스턴스(Scripts/make-static-minion.py)
	/**
	 * M0 액터의 슬롯 0에 넣을 머티리얼. 대형만 채운다(M3 하이드레이션 액터와 같은 M_LabMinionLargeActor, 짙은 회색).
	 * 소형은 비어 있어 Paragon 원본 머티리얼 그대로다. M0는 사망 진행도를 머티리얼에 넘기지 않으므로 사망 색은 나오지 않는다(소형과 같다).
	 */
	UPROPERTY() TObjectPtr<class UMaterialInterface> ActorMaterial;
};

/**
 * 미니언 표현을 Lumen 장면에서 뺀다(모든 단계 공통). 소프트웨어 Lumen은 "동적 간접광에 영향"이 켜져 있고
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
 * AI 컨트롤러 없이 CharacterMovement가 직접 돈다(bRunPhysicsWithNoController).
 * v2 W6부터 개체끼리 캡슐로 막는다(겹침 방지, 캡슐 반지름 = 몸통 반지름, InitMinion). -LabSeparate=0이면 W5까지처럼 서로 무시한다.
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
	/** 사망 중(겹친 쌍에서 뺀다, Mass의 FLabDeadTag와 같은 뜻). 피격 중인 개체는 센다 */
	bool IsDead() const { return State == ELabMinionState::Dead; }
	uint32 GetSerial() const { return Serial; }
	/** 종류(ELabUnitType, LabUnitAssets.h). M0는 InitMinion이 일련번호로 정하고, M3 하이드레이션 액터는 클래스(…Large)가 정한다. 저격 E안이 읽는다 */
	int32 GetUnitType() const { return UnitType; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void SetupVisual(const FLabMinionAssets& Assets);
	void EnterState(ELabMinionState NewState);

	UPROPERTY() TObjectPtr<ALabCrowdSpawner> Spawner;

	const FLabMinionAssets* AssetsRef = nullptr;
	uint32 Serial = 0;
	int32 UnitType = 0; // ELabUnitType
	ELabMinionState State = ELabMinionState::Walk;
	float Health = 0.f;
	float StateTimeLeft = 0.f;
	bool bSingleNode = false;
	bool bManualMove = false; // ALabMinionPawn(하이드레이션 액터의 부모): CharacterMovement 틱 없이 위치를 직접 옮긴다
};

/**
 * CharacterMovement의 틱을 끄고 위치를 직접 옮기는 액터(ACharacter는 이 컴포넌트를 필수로 만든다). 캡슐·스켈레탈 메시·AnimBP는 ALabMinion과 같다.
 * v1의 pawn 변형(액터 쪽 천장)이었고, v2 W10부터는 하이드레이션 액터의 부모로만 남는다.
 */
UCLASS()
class ALabMinionPawn : public ALabMinion
{
	GENERATED_BODY()
public:
	ALabMinionPawn(const FObjectInitializer& ObjectInitializer);
};

/**
 * 하이드레이션 액터(사다리 단계 2부터). Mass가 가까운 엔티티에 붙였다 떼는 표현일 뿐이고 로직은 엔티티에 있다(plan.md "M3").
 * 부품은 캡슐, 스켈레탈 메시, ABP_LabMinion, CharacterMovement 틱 끔이다(ALabMinionPawn).
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

	// 동기화 프로세서가 쓰는 상태. 엔티티가 바뀌면(새로 붙음, 풀에서 다시 씀) 애니를 처음부터 맞춘다. M3b부터 일련번호 대신 엔티티 핸들로 판정한다(표현이 로직 프래그먼트를 읽지 않게).
	FMassEntityHandle BoundEntity;
	ELabMinionState AppliedState = ELabMinionState::Walk;
	float AppliedDeadAmount = 0.f;

	/**
	 * 전환 팝핑 대책: 새로 붙은 액터는 애니를 맞춘 뒤 메시가 포즈를 계산할 때까지 숨겨 두고,
	 * 그동안 엔진이 VAT 인스턴스를 계속 그린다(bWaitForActorVisualReadiness, ULabActorManagement가 이 값을 돌려준다).
	 */
	bool bPoseReady = false;
	int32 PendingReadyFrames = 0;
	int32 ClearVelocityFrames = 0; // 보이기 시작한 뒤 이전 프레임 변환을 지울 남은 프레임 수
	/**
	 * 숨기는 방법: 액터 숨김(SetActorHiddenInGame)은 렌더 프록시를 장면에서 빼서, 다시 보일 때 지난번 보였던 위치·뼈 변환이 이전 프레임으로 쓰여
	 * 모션 블러가 미니언을 번진 잔상으로 그렸다(연속 캡처, 모션 블러를 끄면 사라짐). 그래서 프록시는 장면에 둔 채
	 * 메인 패스·깊이 패스·그림자에서만 뺀다. 그동안에도 위치와 뼈가 렌더러에 전달돼 보이는 첫 프레임의 속도가 맞다.
	 */
	void HideUntilPoseReady();
	void ShowPoseReady();
};

/**
 * E3 (3c): 메시가 USkeletalMeshComponentBudgeted인 하이드레이션 액터(화면 크기 애니 예산, 사다리 단계 4부터). 중요도는 동기화 프로세서가 화면 크기로 넘긴다.
 * 풀에 들어가면 중요도 0(숨긴 액터가 예산을 쓰지 않게, engine-notes.md 4절).
 */
UCLASS()
class ALabMinionMassActorBudget : public ALabMinionMassActor
{
	GENERATED_BODY()
public:
	ALabMinionMassActorBudget(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void PrepareForPooling_Implementation() override;
};

/**
 * 2종 대형 하이드레이션 액터(M3b). 부품과 동작은 소형과 같고 에셋만 대형(BeginPlay가 종류로 읽는다, LabUnitAssets.h).
 * 클래스를 따로 두는 이유: 엔진 액터 풀이 클래스를 키로 쓴다(UMassActorSpawnerSubsystem::PooledActors, MassActorSpawnerSubsystem.h:321). 그래서 대형 풀이 따로 생긴다.
 */
UCLASS()
class ALabMinionMassActorLarge : public ALabMinionMassActor
{
	GENERATED_BODY()
public:
	ALabMinionMassActorLarge(const FObjectInitializer& ObjectInitializer);
};

/** 2종 대형 + 화면 크기 애니 예산. */
UCLASS()
class ALabMinionMassActorBudgetLarge : public ALabMinionMassActorBudget
{
	GENERATED_BODY()
public:
	ALabMinionMassActorBudgetLarge(const FObjectInitializer& ObjectInitializer);
};
