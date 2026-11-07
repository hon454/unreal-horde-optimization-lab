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
 * BP는 GAS 상태 효과를 끌고 와서 쓰지 않는다. 타워 이펙트는 모든 실행에서 상시로 돈다. 저격은 대상마다 굵은 지그재그 번개(보라 원기둥 조각)와
 * 피격 불꽃(NS_Spark_Burst)으로 그리고, 측정 실행(-LabBench)에서는 그리지 않는다.
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

	UStaticMeshComponent* AcquireBoltSegment();

	static constexpr float StrikeDuration = 0.3f;      // 저격 번개가 남아 있는 시간
	static constexpr int32 BoltSegments = 7;           // 지그재그 조각 수
	static constexpr float BoltThickness = 35.f;       // 조각 굵기(cm). 80m 밖에서도 보이게 굵게 둔다
	static constexpr float BoltJitter = 0.04f;         // 중간 점이 옆으로 흔들리는 폭(길이 대비)
	static constexpr float BoltReshapeInterval = 0.05f; // 모양을 다시 흔드는 간격(번쩍임)

	struct FActiveStrike
	{
		FVector From = FVector::ZeroVector;
		FVector To = FVector::ZeroVector;
		float TimeLeft = 0.f;
		float ReshapeIn = 0.f;
		TArray<TObjectPtr<UStaticMeshComponent>> Segments;
	};
	void ShapeStrike(FActiveStrike& Strike);
	TArray<FActiveStrike> ActiveStrikes;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> FreeBoltSegments;
	UPROPERTY() TObjectPtr<class UStaticMesh> BoltMesh;
	UPROPERTY() TObjectPtr<class UMaterialInterface> BoltMaterial;
	UPROPERTY() TObjectPtr<class UNiagaraSystem> ImpactSystem;

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
