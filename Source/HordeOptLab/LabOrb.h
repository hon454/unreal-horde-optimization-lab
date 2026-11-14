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
 * 저격 대상은 화면 안의 산 개체 중 거리 구간마다 결정적 해시(스폰 일련번호 + 저격 틱)가 가장 작은 개체다(LabSnipe.h).
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

	void SetSpawner(ALabCrowdSpawner* InSpawner) { Spawner = InSpawner; }
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	int32 GetKillCount() const;
	int32 GetSnipeCount() const;

	/** 화면 판정용 카메라(직전 프레임에 그린 카메라). 변형 manager의 매니저도 읽는다 */
	struct FLabSnipeView GetSnipeView() const;
	/**
	 * 변형 manager(LabCrowdManager.h): 매니저 틱이 부른다. 오브 틱은 피해·저격을 하지 않고 저격 궤적 수명만 돌린다(FLabConfig::UsesManager).
	 * AdvanceSnipeClock은 저격 시간을 누적하고 이 프레임에 쏠 저격 틱을 돌려준다(없으면 0). OnMassPrePhysicsStart와 같은 누적이다.
	 * ShowManagerResults는 번개 대상과 저격 궤적을 그리고 사망 수를 더한다(저격 수는 대상 수).
	 */
	uint32 AdvanceSnipeClock(float DeltaSeconds);
	void ShowManagerResults(const FVector& CoilTarget, TConstArrayView<FVector> SnipeTargets, int32 ZoneKills);

protected:
	void Snipe();
	/** M1부터: 피해와 저격은 프로세서(LabMassProcessors)가 하고, 오브는 저격 틱을 올리고 이펙트만 그린다. */
	void TickMass(float DeltaSeconds, class ULabCrowdSubsystem* Crowd);
	/**
	 * Mass PrePhysics 단계 시작(게임 스레드, 프로세서를 띄우기 전, MassProcessingPhaseManager.cpp:79-83)에 오브 중심·화면 판정 카메라를 찍고 저격 시간을 누적한다.
	 * 오브 틱은 같은 TG_PrePhysics에서 Mass 워커와 동시에 돌 수 있어(선후 선언 없음) 여기서 쓴다. 그래서 모든 프로세서가 같은 값, 곧 직전 프레임에 그린 카메라를 본다
	 * (카메라 캐시는 TG_PostPhysics 뒤 UpdateCameraManager에서만 바뀐다, LevelTick.cpp:1847). 저격 요청도 늘 이 프레임 처리에서 가져간다.
	 */
	void OnMassPrePhysicsStart(float DeltaSeconds);
	FDelegateHandle PhaseStartHandle;
	/** Spike1 떠다니는 오브(FLabOrbit): 프레임에 한 번 궤도 위치로 옮긴다. 궤도 속도가 0이면 아무것도 하지 않는다 */
	void UpdateOrbit();
	uint64 OrbitFrame = MAX_uint64;
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
