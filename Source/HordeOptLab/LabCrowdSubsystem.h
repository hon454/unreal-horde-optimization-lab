#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "LabCrowdSubsystem.generated.h"

class ALabCrowdSpawner;
class UMassEntityConfigAsset;
class UStaticMesh;

/**
 * M1부터 쓰는 Mass 군중의 공유 상태. 스포너(위치·일련번호의 주인)와 오브(이펙트), 프로세서(로직)가 이 객체로 이어진다.
 * EntityConfig는 에셋이 아니라 여기서 실행 시 만든다(ADR-0005). M0에서는 활성화하지 않는다.
 */
UCLASS()
class ULabCrowdSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	/** M1 단계에서 스포너가 부른다. EntityConfig를 만들고 템플릿을 준비한다. */
	void Activate(ALabCrowdSpawner* InSpawner, UStaticMesh* Mesh, float InHitDuration, float InDeathDuration);
	bool IsActive() const { return bActive; }

	/** 땅 위(z=0) 위치와 일련번호로 엔티티를 만든다. 진행 방향은 오브를 본다. */
	void SpawnEntities(TConstArrayView<FVector> Locations, TConstArrayView<uint32> Serials);
	/** 사망 프로세서가 지운 엔티티 수만큼 스포너에 재생성을 맡긴다(게임 스레드, 커맨드 플러시 중). */
	void RequestRespawn(int32 Count);

	int32 GetAliveCount() const { return AliveCount; }

	// 오브 상태. 오브가 쓰고 프로세서가 읽는다.
	FVector OrbCenter = FVector::ZeroVector;
	uint32 PendingSnipeTick = 0; // 0이 아니면 이번 처리에서 저격한다
	float HitDuration = 0.5f;
	float DeathDuration = 1.f;

	// 프로세서가 쓰고 오브가 읽는다. 청크 단위로 병렬 실행될 수 있어 잠금으로 모은다.
	FCriticalSection ResultLock;
	TArray<FVector> SnipeTargets;    // 이번 저격의 대상 위치(이펙트용)
	bool bHasNearest = false;        // 피해 범위 안에서 가장 가까운 개체(테슬라 코일 번개 대상)
	float NearestDistSq = 0.f;
	FVector Nearest = FVector::ZeroVector;
	int32 KillCount = 0;
	int32 SnipeCount = 0;

private:
	UPROPERTY() TObjectPtr<UMassEntityConfigAsset> EntityConfig;
	TWeakObjectPtr<ALabCrowdSpawner> Spawner;
	bool bActive = false;
	int32 AliveCount = 0;
};
