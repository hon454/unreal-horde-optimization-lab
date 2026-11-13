#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InstanceDataTypes.h" // FPrimitiveInstanceId
#include "LabMinionAnimInstance.h" // ELabMinionState
#include "LabUnitAssets.h"
#include "LabCrowdManager.generated.h"

class ALabCrowdSpawner;
class ALabOrb;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * 변형 manager의 개체(-LabStage=M1 -LabVariant=manager, Docs/Worklog/Q8.md). 틱이 없고 컴포넌트가 없는 액터다. 개체 데이터만 든다.
 * 2단계(M1 mass)의 프래그먼트와 같은 값을 멤버로 둔다: 위치·회전(FTransformFragment), 속도(FMassVelocityFragment), 바라는 속도(FMassDesiredMovementFragment),
 * 이동 목표의 방향(FMassMoveTargetFragment::Forward), 일련번호와 상태에 남은 시간(FLabMinionFragment), 체력(FLabHealthFragment), 표현 상태(FLabAnimStateFragment),
 * 종류(FLabUnitTypeParams), 지난 프레임 변환(FMassRepresentationFragment::PrevTransform). 계산은 모두 ALabCrowdManager가 한다.
 * 루트 컴포넌트가 없어 GetActorLocation은 쓰지 않는다. 위치는 Transform이다(발밑, 지면).
 */
UCLASS(NotPlaceable)
class ALabManagedMinion : public AActor
{
	GENERATED_BODY()
public:
	ALabManagedMinion();

	FTransform Transform = FTransform::Identity;
	FTransform PrevTransform = FTransform::Identity;
	FVector Velocity = FVector::ZeroVector;
	FVector DesiredVelocity = FVector::ZeroVector;
	FVector MoveForward = FVector::ForwardVector;
	float DistanceToGoal = 0.f;
	float Health = 0.f;
	float StateTimeLeft = 0.f;
	float StateStartTime = 0.f;
	uint32 Serial = 0;
	int32 UnitType = LabUnitSmall;
	ELabMinionState State = ELabMinionState::Walk;
	FPrimitiveInstanceId InstanceId;
};

/**
 * 변형 manager의 매니저. 틱 하나가 한 프레임에 모든 ALabManagedMinion을 한 번 순회해 2단계와 같은 규칙으로 계산하고 종류별 공유 ISM에 변환을 쓴다.
 * 로직은 2단계 프로세서와 엔진 프로세서의 식을 옮긴 것이다(LabCrowdManager.cpp의 각 절에 출처).
 * 그리기는 2단계와 같은 설정의 ISM 컴포넌트에 엔진 Mass가 부르는 것과 같은 호출을 한다(MassVisualizationComponent.cpp:236-262, 617-621, 652-658).
 * 바뀌는 것은 데이터를 든 곳(Mass 청크 → 액터 멤버)과 순회 방식(프로세서 여럿의 청크 순회 → 게임 스레드의 포인터 배열 순회 한 번)이다.
 */
UCLASS(NotPlaceable)
class ALabCrowdManager : public AActor
{
	GENERATED_BODY()
public:
	ALabCrowdManager();

	/** 스포너가 부른다. 종류별 ISM 컴포넌트를 만든다(MeshesByType.Num() = -LabTypes). 피격·사망 시간은 2단계와 같은 소형 애니 길이다 */
	void Activate(ALabCrowdSpawner* InSpawner, const FVector& InOrbCenter, TConstArrayView<UStaticMesh*> MeshesByType, float InHitDuration, float InDeathDuration);
	void SetOrb(ALabOrb* InOrb) { Orb = InOrb; }
	/** 땅 위(z=0) 위치와 일련번호로 개체를 만든다. ULabCrowdSubsystem::SpawnEntities와 같은 초기값이다 */
	void SpawnMinions(TConstArrayView<FVector> Locations, TConstArrayView<uint32> Serials);

	virtual void Tick(float DeltaSeconds) override;

	int32 GetAliveCount() const { return Minions.Num(); }
	/** 동등성 통계: 오브 수평 30m 안 개체 수(이번 프레임, ULabNearOrbCountProcessor와 같은 정의) */
	int32 GetNear30() const { return Near30; }

protected:
	UPROPERTY() TArray<TObjectPtr<ALabManagedMinion>> Minions;
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Isms; // 종류별. [0] 소형, [1] 대형(-LabTypes=2)
	UPROPERTY() TObjectPtr<ALabCrowdSpawner> Spawner;
	UPROPERTY() TObjectPtr<ALabOrb> Orb;
	FTransform LocalTransforms[LabUnitTypeMax]; // 메시 -90도 회전과 종류 배율(LabCrowdSubsystem.cpp BuildEntityConfig의 MeshDesc.LocalTransform과 같다)
	FVector OrbCenter = FVector::ZeroVector;
	float HitDuration = 0.5f;
	float DeathDuration = 1.f;
	int32 Near30 = 0;
};
