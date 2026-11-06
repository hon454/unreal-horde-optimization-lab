#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LabOrb.generated.h"

class ALabCrowdSpawner;
class UStaticMeshComponent;

/**
 * 맵 중앙의 목표. 소멸 반경 안의 개체를 죽이고, 저격 간격마다 먼 개체 K개를 쏜다.
 * 대상은 난수가 아니라 결정적 해시(스폰 일련번호 + 저격 틱)로 고른다. plan.md "M0".
 */
UCLASS()
class ALabOrb : public AActor
{
	GENERATED_BODY()
public:
	ALabOrb();

	void SetSpawner(ALabCrowdSpawner* InSpawner) { Spawner = InSpawner; }
	virtual void Tick(float DeltaSeconds) override;

	int32 GetKillCount() const { return KillCount; }
	int32 GetSnipeCount() const { return SnipeCount; }

protected:
	void Snipe();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Sphere;
	UPROPERTY() TObjectPtr<ALabCrowdSpawner> Spawner;

	float SnipeAccumulator = 0.f;
	uint32 SnipeTick = 0;
	int32 KillCount = 0;
	int32 SnipeCount = 0;
};
