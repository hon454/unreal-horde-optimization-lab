#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LabGameMode.generated.h"

class ALabCameraRig;
class ALabCrowdSpawner;
class ALabOrb;

/** 벤치마크 러너 시간. commands.md "벤치마크 러너 인자", measurement.md "벤치마크 조건". */
struct FLabBench
{
	static constexpr float WarmupSec = 15.f; // 2026-11-07 30초에서 줄임(measurement.md "벤치마크 조건")
	static constexpr float MeasureSec = 60.f;
	static constexpr float FlushSec = 2.f;
};

enum class ELabBenchPhase : uint8
{
	Warmup,
	Measure,
	Flush,
	Done,
};

/**
 * 맵(L_Bench, Scripts/create-bench-map.py)에는 바닥·조명·하늘만 있고, 오브·스포너·카메라는 이 게임 모드가 실행 시 만든다.
 * 플레이어 폰은 없고 카메라 프리셋으로만 본다.
 * -LabBench면 벤치마크 러너로 동작한다: 렌더 CVar 고정 → 워밍업 15초 → CSV 60초(파일 이름은 -LabLabel=) → lab_summary 로그 → 종료.
 * N이 유지되지 않았으면 종료 코드 1.
 */
UCLASS()
class ALabGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ALabGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 상태 오버레이(ALabHud)의 군중 줄. */
	FString GetHudCrowdLine() const;

protected:
	void ApplyVariantCVars();
	void ApplyBenchCVars();
	void TickBench();

	UPROPERTY() TObjectPtr<ALabCrowdSpawner> Spawner;
	UPROPERTY() TObjectPtr<ALabOrb> Orb;
	UPROPERTY() TObjectPtr<ALabCameraRig> Camera;

	float Elapsed = 0.f;
	uint64 StartFrame = 0; // 개발 실행 요약의 평균 프레임 시간용
	bool bQuitRequested = false;
	bool bShotTaken = false;

	ELabBenchPhase BenchPhase = ELabBenchPhase::Warmup;
	float BenchPhaseTime = 0.f;
	int32 BenchFrames = 0;
	int32 RespawnsAtStart = 0;
	int64 LargeActorFramesAtStart = 0; // E3 (3c) 스탯의 측정 시작 값(ULabCrowdSubsystem::LargeActorFrames·Skips)
	int64 LargeActorSkipsAtStart = 0;
	int32 BenchExitCode = 0;
};
