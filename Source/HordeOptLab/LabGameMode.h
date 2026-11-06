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
	static constexpr float WarmupSec = 30.f;
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
 * -LabBench면 벤치마크 러너로 동작한다: 렌더 CVar 고정 → 워밍업 30초 → CSV 60초(파일 이름은 -LabLabel=) → lab_summary 로그 → 종료.
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

protected:
	void ApplyVariantCVars();
	void ApplyBenchCVars();
	void TickBench();

	UPROPERTY() TObjectPtr<ALabCrowdSpawner> Spawner;
	UPROPERTY() TObjectPtr<ALabOrb> Orb;
	UPROPERTY() TObjectPtr<ALabCameraRig> Camera;

	float Elapsed = 0.f;
	bool bQuitRequested = false;
	bool bShotTaken = false;

	ELabBenchPhase BenchPhase = ELabBenchPhase::Warmup;
	float BenchPhaseTime = 0.f;
	int32 BenchFrames = 0;
	int32 RespawnsAtStart = 0;
	int32 BenchExitCode = 0;
};
