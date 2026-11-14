#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LabMinion.h"
#include "LabOverlap.h"
#include "LabCrowdSpawner.generated.h"

/**
 * 시작 시 디스크 전체에 정상 상태 분포(반지름당 개체 수 일정, 밀도 ∝ 1/r)로 N개를 놓고,
 * 사라진 개체는 가장자리 링에서 다시 만든다. 초기 위치와 재생성 순서는 시드로 재현한다. plan.md "M0".
 */
UCLASS()
class ALabCrowdSpawner : public AActor
{
	GENERATED_BODY()
public:
	ALabCrowdSpawner();

	void SpawnInitial();
	void OnMinionFinished(ALabMinion* Minion);
	/** M1부터: 지워진 엔티티 수만큼 가장자리 링에서 새로 만든다. ULabCrowdSubsystem이 부른다. 변형 manager는 ALabCrowdManager가 부른다(같은 스트림) */
	void RespawnEntities(int32 Count);
	/** 변형 manager의 매니저. 다른 변형에서는 nullptr */
	class ALabCrowdManager* GetManager() const { return Manager; }

	/** 오브의 지금 지면 위치. Spike1 궤도(FLabOrbit)를 켜지 않으면 원점이다 */
	FVector GetOrbLocation() const;
	const TArray<TObjectPtr<ALabMinion>>& GetMinions() const { return Minions; }
	/** 단계와 무관한 개체 수(M0 액터, M1부터 엔티티). 러너의 LabAlive와 오버레이가 쓴다. */
	int32 GetAliveCount() const;
	int32 GetRespawnCount() const { return RespawnCount; }

	/**
	 * v2 W6: M0 액터의 겹친 쌍(LabOverlap.h, Mass의 ULabOverlapCountProcessor와 같은 정의). 게임 모드 틱이 매 프레임 부르고 LabOverlapSampleSec마다 센다.
	 * 몸통은 액터 위치(캡슐 중심)의 수평 원이고 반지름은 종류별 BoundRadius(소형 40, 대형 94)다. 사망 중인 개체는 뺀다
	 */
	void SampleActorOverlaps(double Now);
	int32 GetActorOverlapPairs() const { return ActorOverlapPairs; }
	int32 GetActorOverlapDeep() const { return ActorOverlapDeep; }

protected:
	ALabMinion* SpawnAt(float Radius, float Angle);
	TSubclassOf<ALabMinion> MinionClass() const;
	void LoadAssets();
	void SpawnEntitiesAt(TConstArrayView<FVector2D> RadiusAngles);

	UPROPERTY() FLabMinionAssets Assets;      // 소형(기본)
	UPROPERTY() FLabMinionAssets LargeAssets; // 2종 대형(-LabTypes=2). 대형 에셋이 하나라도 없으면 Assets와 같다(대체, 스모크 전용. LabUnitAssets.h)
	UPROPERTY() TArray<TObjectPtr<ALabMinion>> Minions;

	UPROPERTY() TObjectPtr<class ULabCrowdSubsystem> Crowd; // M1부터
	UPROPERTY() TObjectPtr<class ALabCrowdManager> Manager; // 변형 manager만(FLabConfig::UsesManager). 있으면 Crowd는 비어 있다
	FRandomStream Stream;
	uint32 NextSerial = 0;
	int32 RespawnCount = 0;
	TArray<FLabBody> OverlapBodies; // v2 W6 M0 겹친 쌍 표본(SampleActorOverlaps)
	double NextOverlapSample = 0.;
	int32 ActorOverlapPairs = 0;
	int32 ActorOverlapDeep = 0;
};
