#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LabMinion.h"
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

	FVector GetOrbLocation() const { return OrbLocation; }
	const TArray<TObjectPtr<ALabMinion>>& GetMinions() const { return Minions; }
	int32 GetRespawnCount() const { return RespawnCount; }

protected:
	ALabMinion* SpawnAt(float Radius, float Angle);
	TSubclassOf<ALabMinion> MinionClass() const;
	void LoadAssets();

	UPROPERTY() FLabMinionAssets Assets;
	UPROPERTY() TArray<TObjectPtr<ALabMinion>> Minions;

	FRandomStream Stream;
	FVector OrbLocation = FVector::ZeroVector;
	uint32 NextSerial = 0;
	int32 RespawnCount = 0;
};
