#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LabGameMode.generated.h"

class ALabCameraRig;
class ALabCrowdSpawner;
class ALabOrb;
class UMaterialInstanceDynamic;
class UNiagaraComponent;

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
	/**
	 * 결정 6 바닥 띠(개발 전용 -LabBandFloor, 2026-11-09, backlog.md). L_Bench의 Floor 머티리얼을 M_LabBandFloor(에셋 세션 245cc35, Scripts/create-band-floor.md)의 MID로 바꾸고
	 * 띠 반경은 저격 구간 FLabSnipeBands(30/80/200m)에서, 중심(BandCenter)은 오브 위치에서 매 프레임 넣는다. P2(탑뷰)에서는 켜지 않는다(3D 거리라 200m 경계가 XY 173m).
	 * 켜짐은 CVar lab.BandFloor=1로 CSV 메타데이터 LabCVars에 남아 실수로 켜고 잰 실행을 걸러낸다. Test 빌드에는 코드가 없다.
	 */
	void SetupBandFloor();
	/**
	 * Q2 Niagara 배경 무리(-LabCrowdFx=1, Docs/Worklog/Q2.md (마)). Config/DefaultGame.ini [Lab] CrowdFxSystem의 시스템을 오브 위치에 놓고
	 * 사용자 변수(SmallRate·LargeRate·OuterRadius·Lifetime·Speed)를 넣은 뒤 활성화한다. 워밍업(268초)은 활성화 때 돈다.
	 */
	void SetupCrowdFx();
	/** Animation Budget Allocator가 실제로 도는가(CVar와 월드 할당기 둘 다). CSV 메타데이터 LabBudget */
	bool IsBudgetActive() const;
	/** Q3 lab_applied 로그: 단계와 실제로 적용된 값(Budget, 풀, 고정 바운드 컴포넌트 수, 전환 대책 마스크, 배경 무리 활성), 설정 tech= */
	FString AppliedString() const;

	UPROPERTY() TObjectPtr<ALabCrowdSpawner> Spawner;
	UPROPERTY() TObjectPtr<ALabOrb> Orb;
	UPROPERTY() TObjectPtr<ALabCameraRig> Camera;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BandFloorMID; // -LabBandFloor(개발 전용)일 때만 있다
	UPROPERTY() TObjectPtr<UNiagaraComponent> CrowdFx;             // -LabCrowdFx=1일 때만 있다
	float CrowdFxNextLog = 0.f; // 개발 빌드의 입자 수 로그(lab_crowdfx_count) 다음 시각

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
