#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LabOrb.generated.h"

class ALabCrowdSpawner;
class UNiagaraComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * 맵 중앙의 목표(테슬라 코일 타워). 피해 범위(보라색 원기둥) 안의 개체는 중심에 가까울수록 센 피해(초당 ZoneDpsEdge~ZoneDpsCenter)를 받고, 저격 간격마다 먼 개체 K개를 쏜다.
 * 저격 대상은 난수가 아니라 결정적 해시(스폰 일련번호 + 저격 틱)로 고른다. plan.md "M0".
 * 겉모습은 Niagara Examples Pack의 BP_TeslaCoil 구성(타워 메시, 구체, NS_TeslaCoil, NS_Boundary_Cylinder)을 같은 배치로 옮긴 것이다.
 * BP는 GAS 상태 효과를 끌고 와서 쓰지 않는다. 타워 이펙트는 모든 실행에서 상시로 돈다. 저격은 대상마다 구체에서 대상까지 곧은 보라 선 하나
 * (엔진 DynamicBeam 기반 NS_LabBolt)로 그린다. 측정 실행에서도 그린다(시나리오의 일부).
 * 액터 위치는 지면(z=0)이다. 타워 메시(높이 90.5 x4 = 362cm)를 z360에서 뒤집어 두므로 바닥이 지면에 닿는다.
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
	void SpawnStrikeFx(const FVector& Target);
	void TickStrikes(float DeltaSeconds);

	static constexpr float StrikeDuration = 0.3f;  // 저격 궤적이 남아 있는 시간
	static constexpr float BoltWidth = 6.f;          // 궤적 굵기(빔 너비 단위)
	inline static const FLinearColor BoltColor = FLinearColor(4.f, 1.f, 24.f); // HDR 보라. 밝은 바닥에서도 보이게 1을 넘긴다

	struct FActiveStrike
	{
		TWeakObjectPtr<UNiagaraComponent> Component;
		float TimeLeft = 0.f;
	};
	TArray<FActiveStrike> ActiveStrikes;
	UPROPERTY() TObjectPtr<class UNiagaraSystem> StrikeSystem;

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
