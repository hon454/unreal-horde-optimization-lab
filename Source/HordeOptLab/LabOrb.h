#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LabOrb.generated.h"

class ALabCrowdSpawner;
class UNiagaraComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * 맵 중앙의 목표(테슬라 코일 타워). 피해 범위(보라색 원기둥) 안의 개체는 초당 ZoneDps 피해를 받고, 저격 간격마다 먼 개체 K개를 쏜다.
 * 저격 대상은 난수가 아니라 결정적 해시(스폰 일련번호 + 저격 틱)로 고른다. plan.md "M0".
 * 겉모습은 Niagara Examples Pack의 BP_TeslaCoil 구성(타워 메시, 구체, NS_TeslaCoil, NS_Boundary_Cylinder)을 같은 배치로 옮긴 것이다.
 * BP는 GAS 상태 효과를 끌고 와서 쓰지 않는다. 이펙트는 모든 실행에서 하나씩 상시로 돈다. 저격 표현은 디버그 라인이다(Test 패키지에서는 그려지지 않는다).
 * 액터 위치는 지면에서 OrbHeight 위다(BP의 루트 높이).
 */
UCLASS()
class ALabOrb : public AActor
{
	GENERATED_BODY()
public:
	ALabOrb();

	static constexpr float OrbHeight = 200.f;

	void SetSpawner(ALabCrowdSpawner* InSpawner) { Spawner = InSpawner; }
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	int32 GetKillCount() const { return KillCount; }
	int32 GetSnipeCount() const { return SnipeCount; }

protected:
	void Snipe();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Tower;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Sphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> CoilFx;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> ZoneFx;
	UPROPERTY() TObjectPtr<ALabCrowdSpawner> Spawner;

	float SnipeAccumulator = 0.f;
	uint32 SnipeTick = 0;
	int32 KillCount = 0;
	int32 SnipeCount = 0;
};
