#pragma once

#include "CoreMinimal.h"

/** 미니언 표현 변형(M0 E0). 실행 인자 -LabVariant=로 고른다. plan.md "M0". */
enum class ELabVariant : uint8
{
	Full,   // ACharacter + 스켈레탈 메시 + 애니메이션
	Static, // 같은 ACharacter + UStaticMeshComponent, 애니메이션 없음
	Budget, // Full + USkeletalMeshComponentBudgeted + Significance 거리 단계 + a.Budget.BudgetMs 2
	Uro,    // Full + URO만
};

/**
 * 시나리오 고정값 초안. M0 8번에서 실측해 사용자 확정을 받은 뒤 measurement.md "확정값"에 적고 여기를 맞춘다.
 * 단위는 cm, 초.
 */
struct FLabScenario
{
	static constexpr float MapRadius = 25000.f;      // R 250m
	static constexpr float RespawnRingWidth = 1000.f; // 재생성 링 [R-10m, R]
	static constexpr float KillRadius = 300.f;       // 오브 소멸 반경 3m
	static constexpr float MoveSpeed = 300.f;        // 이동 속도 3m/s
	static constexpr float SnipeInterval = 2.f;      // 저격 간격(0.5초 10발은 화면에서 너무 잦았다, 2026-11-06 사용자 확인)
	static constexpr int32 SnipeCount = 5;           // 저격당 대상 수 K
	static constexpr float SnipeMinDistance = 8000.f; // 저격 대상 최소 거리(초안 밴드 80m 밖)

	// 카메라 프리셋. plan.md "프리셋의 역할"
	static constexpr float OrbitRadius = 6000.f;     // P1 궤도 반경 60m
	static constexpr float OrbitHeight = 2000.f;     // P1 높이 20m
	static constexpr float OrbitPeriod = 60.f;       // P1 1rpm, 측정 60초에 한 바퀴
	static constexpr float TopDownHeight = 30000.f;  // P2 탑뷰. M0 8번에서 모든 개체가 ISM 밴드에 들도록 다시 정한다
	static constexpr float GroundDistance = 2500.f;  // P3 오브에서 25m
	static constexpr float GroundEyeHeight = 170.f;  // P3 눈높이

	// budget 변형의 Significance 거리 단계(25/50/100/250m). plan.md "M0"
	static constexpr float BudgetTiers[4] = { 2500.f, 5000.f, 10000.f, 25000.f };
	static constexpr float BudgetMs = 2.f;
};

/** 실행 인자. 프로세스 시작 시 한 번 읽는다. */
struct FLabConfig
{
	int32 N = 500;
	int32 Preset = 1;          // 1 궤도, 2 탑뷰, 3 지상
	int32 Seed = 1;
	ELabVariant Variant = ELabVariant::Full;
	bool bBench = false;       // 벤치마크 러너(M0 7번)
	FString Label;
	float DurationSec = 0.f;   // 0보다 크면 그 시간 뒤 종료(개발용 -Small 실행)
	float ShotAtSec = 0.f;     // 0보다 크면 그 시각에 스크린샷 한 장(Saved/Screenshots/)

	static const FLabConfig& Get();
	static const TCHAR* VariantName(ELabVariant V);
	FString ToLogString() const;
};
