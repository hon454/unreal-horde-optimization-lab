#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LabOrb.generated.h"

class ALabCrowdSpawner;
struct FMassEntityHandle;
class UNiagaraComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * 떠다니는 오브(v2, FLabOrbit). 피해 범위(보라색 원기둥) 안의 개체는 중심에 가까울수록 센 피해(초당 ZoneDpsEdge~ZoneDpsCenter)를 받고, 저격 간격마다 먼 개체 K개를 쏜다.
 * 저격 대상은 화면 안의 산 개체 중 거리 구간마다 결정적 해시(스폰 일련번호 + 저격 틱)가 가장 작은 개체다(LabSnipe.h).
 * 겉모습은 Niagara Examples Pack의 BP_TeslaCoil 구성(구체, NS_TeslaCoil, NS_Boundary_Cylinder)을 옮긴 것이다. 타워 메시는 v1의 멈춘 목표용이라 v2 W10에서 지웠다.
 * BP는 GAS 상태 효과를 끌고 와서 쓰지 않는다. 이펙트는 모든 실행에서 상시로 돈다. 저격은 대상마다 구체에서 대상까지 곧은 보라 선 하나
 * (엔진 DynamicBeam 기반 NS_LabBolt)로 그린다. 측정 실행에서도 그린다(시나리오의 일부).
 * 액터 위치는 지면(z=0)이고 구체는 궤도 높이(기본 6m)에 뜬다.
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
	/**
	 * v2 W5 저격 캡슐 트레이스의 누적 수(Docs/v2-spec.md 3절 기능 3). 액터 구간의 대상(하이드레이션 액터가 붙은 엔티티)마다 구체에서 액터 캡슐 중심까지
	 * LabSnipe 채널로 한 번 쏜다. 대상은 실제 저격 대상 중 액터가 붙은 것(SnipeTraces)과 저격 틱마다 하나의 확인 대상(LabCrowdSubsystem.h CapsuleCheckEntities)이다.
	 * Hits: 처음 맞은 것이 그 대상의 캡슐. Blocked: 다른 액터의 캡슐이 먼저 맞았다. 빗나감 = Traces − Hits − Blocked.
	 * M0(v2 W6)는 모든 개체가 액터라 저격 대상 전부와 같은 규칙의 확인 대상 하나에 쏜다(ALabOrb::Snipe).
	 * 확인용이라 저격의 대상과 결과는 바꾸지 않는다. CSV 스탯 Lab/CapsuleTraces·Lab/CapsuleHits·Lab/CapsuleBlocked(누적), 메타데이터 LabCapsule*(측정 구간 증가분)
	 */
	struct FLabCapsuleTraceStats { int32 Traces = 0; int32 Hits = 0; int32 Blocked = 0; int32 SnipeTraces = 0; };
	const FLabCapsuleTraceStats& GetCapsuleTraceStats() const { return CapsuleTraceStats; }

	/** 화면 판정용 카메라(직전 프레임에 그린 카메라) */
	struct FLabSnipeView GetSnipeView() const;

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
	/** v2 W5: 저격 대상 엔티티 중 액터가 붙은 것에 캡슐 트레이스를 쏘고 센다(게임 스레드, TickMass). 위 GetCapsuleTraceStats */
	void TraceSnipeCapsules(TConstArrayView<FMassEntityHandle> SnipeTargets, TConstArrayView<FMassEntityHandle> CheckTargets);
	/** 구체에서 대상 액터의 캡슐 중심까지 LabSnipe 채널로 한 번 쏘고 센다. Mass 단계(TraceSnipeCapsules)와 M0(Snipe, v2 W6)가 같이 쓴다 */
	void TraceCapsule(const AActor* Target, bool bSnipeTarget);
	/** 누적 캡슐 트레이스 수를 CSV 스탯 Lab/CapsuleTraces·Lab/CapsuleHits·Lab/CapsuleBlocked로 쓴다(매 프레임) */
	void WriteCapsuleStats() const;
	FLabCapsuleTraceStats CapsuleTraceStats;

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
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Sphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> CoilFx;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> ZoneFx;
	UPROPERTY() TObjectPtr<ALabCrowdSpawner> Spawner;

	float SnipeAccumulator = 0.f;
	uint32 SnipeTick = 0;
	int32 KillCount = 0;
	int32 SnipeCount = 0;
};
