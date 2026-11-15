#pragma once

#include "CoreMinimal.h"
#include "LabSeparation.h"

/**
 * 근거리 품질 설정(Q4, Docs/Worklog/Q4.md). 액터 상한 = 표현 LOD High의 LODMaxCount(두 종류의 템플릿이 같은 LOD 파라미터라 한 공유 프래그먼트를 써서 합계 상한이다,
 * MassVisualizationTrait.cpp:145 GetOrCreateSharedFragment), 전환 거리 = 화면 안 High 경계(VisibleLODDistance[Medium]). 화면 밖 경계(15m, M3b 결정 1)는 그대로다.
 * High 경계에는 히스테리시스가 없다(엔진은 한 단계 위로 오를 때 그 위 밴드의 경계에만 둔다, MassLODCalculator.h:868-886).
 */
struct FLabQuality
{
	static constexpr int32 DefaultActorCap = 500;   // 확정값 LODMaxCount High(measurement.md). 사다리 단계 2~7
	static constexpr float DefaultHydrateM = 30.f;  // 확정값 거리 밴드 0~30m. 사다리 단계 2~7
	static constexpr float MaxHydrateM = 79.f;      // 80m(Medium/Low 경계, 확정값)를 넘지 않는다
	// 사다리 단계 8(품질 올리기)의 값. Q4 묶음에서 골랐다(Docs/Worklog/Q4.md "최종 구성"): 1만에서는 상한이 걸리지 않고 2만에서 1,000개로 막는다
	static constexpr int32 FinalActorCap = 1000;
	// Q4b 표현 LOD 재편: 79m → 40m. Q4 묶음에서 대형 액터의 애니 갱신 건너뜀 비율이 40m 0.03~0.07, 50m 0.25~0.31, 79m 0.66이었다
	static constexpr float FinalHydrateM = 40.f;
	// v2 W7 표현 전환의 최소 유지 시간(ms, FLabConfig::RepHoldMs). 근거는 Docs/Worklog/V2.md "W7": 액터 경계 왕복의 간격이 대부분 1~2초라
	// 1,000ms는 이동 조건의 왕복을 거의 줄이지 못했고(개발 빌드 초당 약 4 → 3.5), 2,000ms(왕복 창과 같은 값)는 상한 500 조건의 왕복(초당 13~63)을 0으로 만들었다
	static constexpr int32 DefaultRepHoldMs = 2000;
	// v2 W8 ISKM 바운드 상자를 카메라로 옮기는 간격(m, FLabConfig::BoundsStepM). 근거는 Docs/Worklog/V2.md "W8"
	static constexpr float DefaultBoundsStepM = 10.f;
	/**
	 * v2 W11 액터 메시 고정 바운드(FLabConfig::bFixedSkelBounds). 사다리 단계 8의 기본값이다(Docs/Worklog/V2.md "W11", 묶음 V2W11Cost: 프레임 p50 −0.80ms, 소형 애니 건너뜀 0.12 → 0).
	 * 메시 에셋의 고정 바운드는 피직스 에셋으로 셈한 바운드보다 작다(개발 빌드 구 반경 소형 140 대 151cm, 죽는 소형 140 대 154cm). 그대로 두면 LOD가 거칠어지고
	 * 쓰러지는 몸이 상자 밖으로 나갈 수 있어 BoundsScale을 곱한다(SkinnedMeshComponent.cpp CalcMeshBound 끝). 1.1이면 소형 154cm로 죽는 자세의 피직스 바운드 평균과 같다.
	 */
	static constexpr float FixedSkelBoundsScale = 1.1f;
};

/**
 * Q5 단계 사이 동등성 통계(Docs/Worklog/Q5.md "동등성 통계"). 모든 단계에서 같은 뜻으로 CSV에 남긴다.
 * 오브 30m 안 개체 수 = 오브 중심에서 수평 거리 30m 안의 개체(걷기·피격·사망 모두, 풀에서 기다리는 엔티티 제외). 카메라와 무관하다.
 */
struct FLabEquivalence
{
	static constexpr float NearOrbCm = 3000.f; // 30m(확정값 거리 밴드의 첫 경계와 같은 값, 기준은 카메라가 아니라 오브)
};

/**
 * Q2 Niagara 배경 무리(/Game/Lab/FX/NS_LabCrowd, Docs/Worklog/Q2.md). 사다리 단계 7부터.
 * 엔티티 구동 이미터가 그려지지 않는 걷는 엔티티의 자리에 입자를 그리고(Q2b), 독립 이미터가 맵 밖(500~1,000m)을 채운다.
 * 독립 입자는 바깥 고리에서 생겨 미니언과 같은 속도로 오브를 향해 걷고, 수명이 끝나는 안쪽 반경(맵 반경)에서 사라진다(페이드 없음).
 */
struct FLabCrowdFx
{
	static constexpr float OuterRadius = 100000.f; // 1,000m. 바닥 반쪽 길이 1,020m(Scripts/create-bench-map.py)
	static constexpr float LargeShare = 0.1f;      // 2종의 대형 비율(일련번호 10의 배수가 대형, LabUnitAssets.h). 1종이면 0
	/**
	 * Q2b 엔티티 구동 입자(Docs/Worklog/Q2.md "Q2b"). 그려지지 않는 걷는 엔티티(표현 None)의 위치를 매 프레임 종류별 Niagara 배열로 넘긴다(ULabCrowdFxGatherProcessor).
	 * 원소 = (X, Y, T0, 위상): 월드 좌표(cm), 그 위치에 있던 월드 시각(초), 걷기 위상(초, = −StateStartTime). 에셋은 P = P0 + 오브 방향 × Speed × max(LabNow − T0, 0)로 외삽한다.
	 * 이름은 에셋 단위와의 약속이다. 에셋에 없으면 경고 한 번 남기고 그 값만 넣지 않는다.
	 */
	static constexpr const TCHAR* SmallRecsParam = TEXT("User.LabSmallRecs"); // Niagara Array Float4
	static constexpr const TCHAR* LargeRecsParam = TEXT("User.LabLargeRecs"); // Niagara Array Float4
	static constexpr const TCHAR* NowParam = TEXT("User.LabNow");             // float, 월드 초(매 프레임)
	static constexpr const TCHAR* OrbPosParam = TEXT("User.LabOrbPos");       // Vector(또는 Position), 오브 월드 위치(활성화 전 한 번, v2 W8부터 매 프레임. FLabConfig::bFxOrbFollow)
};

/**
 * Simulation LOD 설정(사다리 단계 5부터). 거리는 카메라 기준이고 화면 안팎을 가리지 않는다(MassLODLogic.h:42).
 * 200m 밖은 Off 밴드다: 엔진이 FMassOffLODTag를 붙여 엔진 이동·조향에서 빼고 ULabOffMoveProcessor가 틱 간격마다 옮긴다(engine-notes.md 5절).
 * 엔진의 Off 순간 이동 프로세서(UMassOffLODNavigationProcessor)는 이 단계부터 등록하지 않는다(HordeOptLab.cpp).
 */
struct FLabSimLod
{
	static constexpr float MediumDistance = 8000.f;  // High 0~80m
	static constexpr float LowDistance = 20000.f;    // Medium 80~200m, Off 200m~
	static constexpr float TickMedium = 0.5f; // 초. High는 0(매 프레임)
	static constexpr float TickLow = 1.0f;    // Off 밴드의 간격
};

/**
 * 시나리오 고정값. 2026-11-07 사용자 확정(measurement.md "확정값"). 바꾸면 모든 태그를 다시 잰다.
 * 단위는 cm, 초.
 */
struct FLabScenario
{
	static constexpr float MapRadius = 50000.f;      // R 500m
	static constexpr float RespawnRingWidth = 1000.f; // 재생성 링 [R-10m, R]
	static constexpr float ZoneRadius = 800.f;       // 오브 피해 범위 8m(보라색 원기둥). 10m는 컸고 6m는 작았다
	static constexpr float ZoneDpsEdge = 100.f;      // 범위 가장자리 초당 피해. 체력 100이면 1초
	static constexpr float ZoneDpsCenter = 300.f;    // 중심 초당 피해. 안으로 갈수록 선형으로 세진다
	static constexpr float MinionHealth = 100.f;     // 공칭값. Q1부터 개체 체력은 FLabConfig::HealthFor가 정한다(MaxHealth 공유 조각에만 남아 있고 쓰이지 않는다)
	static constexpr float HealthMin = 60.f;         // Q1 체력 분산 범위(균등). 한 곳의 상수(ADR-0009, measurement.md "품질 계획에서 바뀌는 값")
	static constexpr float HealthMax = 240.f;
	static constexpr float MoveSpeed = 300.f;        // 이동 속도 3m/s
	static constexpr float SnipeInterval = 2.f;      // 저격 간격(0.5초 10발은 화면에서 너무 잦았다)
	static constexpr int32 SnipeCount = 5;           // 저격당 대상 수 K
	// 저격 대상은 화면 안에서 거리 구간별로 고른다(LabSnipe.h FLabSnipeBands. 그전에는 80m 밖 전체에서 5명)

	// 카메라 프리셋. 오브(중앙)에서 바깥을 본다. 근거리 액터부터 지평선 원거리까지 한 화면에 들어온다.
	static constexpr float OrbViewBack = 1500.f;     // P1 오브 뒤 15m. 오브가 화면 아래 가운데에 보이게 한다
	static constexpr float OrbViewHeight = 900.f;    // P1 높이 9m
	static constexpr float OrbViewPitch = -12.f;     // P1 내려다보는 각
	static constexpr float OrbViewPeriod = 60.f;     // P1 1rpm으로 제자리 회전, 측정 60초에 한 바퀴
	// v2 떠다니는 오브(Docs/v2-spec.md 2절, autonomous-decisions.md D34). 궤도의 중심은 맵의 중심이다
	static constexpr float OrbitRadius = 5730.f;     // 궤도 반지름 57.3m
	static constexpr float OrbitSpeed = 200.f;       // 궤도 속도 2m/s. 한 바퀴 180초(2π × 57.3 ÷ 2)
	static constexpr float OrbHeight = 600.f;        // 구체 높이 6m
	static constexpr float TopDownHeight = 10000.f;  // P2 탑뷰 100m. 가장 가까운 개체도 100m 떨어져 근거리 밴드(초안 30m)에 들지 않는다. 300m는 너무 멀었다

	// 애니메이션 예산 기본값(ms). 할당기를 켜는 모든 실행이 쓰고, -LabBudgetMs= 비교도 이 값과 한다. 4ms 근거: Docs/Decisions/0016-animation-budget-value.md
	static constexpr float BudgetMs = 4.f;
};

/**
 * v2 떠다니는 오브(ADR-0017, Docs/v2-spec.md 2절): 오브가 지면 위를 원 궤도로 돈다. 기본값은 FLabScenario의 궤도 상수다.
 * 궤도는 월드 시각의 결정적 함수다. 원의 중심은 맵의 중심(원점)이고 t = 0에 (R, 0)에서 +Y로 출발한다.
 * 정지 조건(-LabOrbOrbitSpeed=0)에서는 오브가 원점에 머문다(v1과 같은 장면).
 * 탐색용 인자: -LabOrbOrbitRadius=<m>, -LabOrbOrbitSpeed=<m/s>, -LabOrbHeight=<m>.
 * 오브 액터의 위치는 늘 지면(z = 0)이다. 판정은 수평 거리라 높이는 겉모습만 바꾼다.
 */
struct FLabOrbit
{
	float RadiusCm = FLabScenario::OrbitRadius;
	float SpeedCms = FLabScenario::OrbitSpeed;
	float HeightCm = FLabScenario::OrbHeight;
	bool IsMoving() const { return RadiusCm > 0.f && SpeedCms > 0.f; }
	bool IsDefault() const { return RadiusCm == FLabScenario::OrbitRadius && SpeedCms == FLabScenario::OrbitSpeed && HeightCm == FLabScenario::OrbHeight; }
	/** 한 바퀴 시간(초). 움직이지 않으면 0 */
	float PeriodSec() const { return IsMoving() ? UE_TWO_PI * RadiusCm / SpeedCms : 0.f; }
	/** 월드 시각 T(초)의 오브 지면 위치 */
	FVector LocationAt(double T) const
	{
		if (!IsMoving())
		{
			return FVector::ZeroVector;
		}
		const double A = double(SpeedCms) / double(RadiusCm) * T;
		return FVector(RadiusCm * FMath::Cos(A), RadiusCm * FMath::Sin(A), 0.);
	}
	/** 구체 중심의 높이(cm). 0이면 v1의 타워 꼭대기 높이 400(타워 메시는 v2 W10에서 지웠다) */
	float SphereZ() const { return HeightCm > 0.f ? HeightCm : 400.f; }
	/** CSV 메타데이터 LabOrbit: "r57.3/v2/h6" 꼴(m, m/s, m) */
	FString ToString() const
	{
		return FString::Printf(TEXT("r%g/v%g/h%g"), RadiusCm / 100.f, SpeedCms / 100.f, HeightCm / 100.f);
	}
};

/** ISKM 렌더·재생 설정(plan.md "M5" 결정 5·6). 사다리 단계 6부터 */
struct FLabIskmParams
{
	static constexpr float BlendTime = 0.f;               // 클립 전환 크로스페이드(초). 0 = VAT와 같이 끊어 바꾸기
	static constexpr float AnimationMinScreenSize = -1.f; // < 0 항상 재생(MassSkinnedMeshRepresentationTypes.h:96-101)
	static constexpr bool bCastShadows = true;            // 엔진 설명자 기본값은 false(MassSkinnedMeshRepresentationTypes.h:79)
};

/**
 * 실행 인자. 프로세스 시작 시 한 번 읽는다.
 * v2 W10(Docs/Worklog/V2.md "W10"): 사다리 단계 하나(-LabStage=1~8)가 무엇을 켜는지 정하고, 빼 보기와 하위 실험은 스위치 인자로 기법 하나를 바꾼다.
 * 단계는 한 빌드 안의 실행 모드다(ADR-0004). 단계별 스위치 표는 Docs/Worklog/V2.md "W10"과 Docs/Guides/commands.md에 있다.
 */
struct FLabConfig
{
	/**
	 * 사다리 단계(-LabStage=1~8, 기본 8 = 최종 구성, Docs/v2-plan.md "사다리"). 1 액터 기준선, 2 Mass 엔진 기본 경로, 3 직접 짠 분리,
	 * 4 애니메이션 표현(VAT·애니 예산·전환 대책), 5 Simulation LOD + 엔티티 풀, 6 ISKM, 7 배경 무리, 8 품질 올리기.
	 */
	int32 Step = 8;
	bool bStageValid = true;   // -LabStage가 1~8이 아니면 false(옛 이름 M0~M5 포함). 게임 모드가 종료 코드 2로 끝낸다
	bool bVariantValid = true; // -LabVariant를 주면 false(v2 W10에서 변형을 지웠다). 게임 모드가 종료 코드 2로 끝낸다
	/**
	 * -LabHud: 상태 오버레이를 켠 채 시작(F1로도 토글). Q5부터 -LabBench와 함께 줘도 켜진다(촬영 실행 = 측정과 같은 조건 + HUD).
	 * 측정 실행은 주지 않는다. 켜졌는지는 CSV 메타데이터 LabHud로 남아 러너가 거른다(Docs/Worklog/Q5.md "HUD").
	 */
	bool bHud = false;
	int32 N = 500;
	int32 Preset = 1;          // 1 오브 시점, 2 탑뷰. 둘 다 오브의 지금 위치를 따라간다(v2)
	FLabOrbit Orbit;           // v2 떠다니는 오브(기본은 FLabScenario의 궤도)
	int32 Seed = 20261106;
	/**
	 * Q1 개체 체력 고정(-LabHealthFixed=<값>, 기본 0 = 분산). 0보다 크면 모든 개체 체력이 그 값이다(분산 전 상태 재현용 디버그 인자, 예: 100).
	 */
	float HealthFixed = 0.f;
	/** Q1 개체 체력: 시드와 스폰 일련번호의 결정적 해시로 [HealthMin, HealthMax] 균등. 초기 배치·재생성·풀 재사용·액터 단계 모두 이 함수 하나를 쓴다 */
	float HealthFor(uint32 Serial) const;
	bool bBench = false;       // 벤치마크 러너(M0 7번)
	FString Label;
	float WarmupSec = -1.f;    // -LabWarmup=<초>. 0 이상이면 -LabBench 워밍업을 그 길이로 한다(기본은 FLabBench: 이동 60초, 정지 15초)
	float DurationSec = 0.f;   // 0보다 크면 그 시간 뒤 종료(개발용 -Small 실행). -LabBench에서는 측정 구간 길이(기본 60초 대신, M5 E5 (b) 960)
	float ShotAtSec = 0.f;     // 0보다 크면 그 시각에 QHD 스크린샷 한 장(Saved/Screenshots/)
	/**
	 * 전환 대책(Measurements/M3/shots/transition/README.md)을 하나씩 켜고 끈다(-LabHydrateFixMask=, 비트는 EHydrateFix). 기본은 사다리 단계 4부터 31(전부), 그 앞은 0.
	 * 빼 보기 -LabHydrateFixMask=0이 전부 끈다. CSV 메타데이터 LabHydrateFixMask, LabTechHydrateFix.
	 */
	int32 HydrateFixMask = 31;
	/**
	 * 개체 종류 수(-LabTypes=, 기본 2. M4부터 기본 시나리오가 2종이다, plan.md "M4"). 2면 일련번호 10의 배수가 대형이다
	 * (소형 90 / 대형 10, plan.md "M3b" 2종). 아키타입·VAT·ISM·액터 풀이 종류별로 갈리고 저격은 예약 발에서만 대형을 쏜다(LabSnipe.h).
	 * -LabTypes=1이면 M3b까지의 1종 결과가 그대로다(LabUnitAssets.h).
	 */
	int32 Types = 2;
	/** -LabFullyParallel=0이면 mass.FullyParallel을 0으로 둔다(직렬 트레이스). -1은 건드리지 않음 */
	int32 FullyParallel = -1;
	/**
	 * 그리는 거리 실험(-LabFarM=<미터>, 기본 200 = 확정값, Docs/Worklog/M4.md "그리는 거리 인자").
	 * 표현 LOD의 Off 거리(이 밖은 그리지 않는다)와 Simulation LOD Off 거리를 함께 옮긴다. 30m·80m 경계, Simulation LOD의 Low 경계(200m), 저격 구간, 밴드 집계 경계는 그대로다.
	 * 200보다 작은 값은 200으로 올린다(Low 경계보다 안쪽의 Off는 다루지 않는다).
	 */
	int32 FarM = 200;
	float FarCm() const { return float(FarM) * 100.f; }
	/**
	 * 엔티티 풀링(-LabEntityPool=0/1, plan.md "M5" 결정 1). 사망 개체를 파괴하지 않고 풀에 두었다가 재생성 때 다시 꺼낸다(ISKM 트랙 누수 우회).
	 * 기본: 사다리 단계 5부터 1. 인자를 주면 그 값이 이긴다.
	 */
	bool bEntityPool = false;
	/**
	 * v2 W3 엔진 회피(-LabAvoid=0/1, Docs/v2-spec.md 3절). 1이면 엔티티에 엔진 회피 Trait 둘을 붙인다
	 * (UMassObstacleAvoidanceTrait, UMassNavigationObstacleTrait, 파라미터는 엔진 기본값). 기본: 사다리 단계 2만 1(D43).
	 * Simulation LOD를 쓰는 단계에서는 엔진 회피 프로세서가 High(80m 안)만 처리한다(MassAvoidanceProcessors.cpp:301-303).
	 */
	bool bAvoid = false;
	/**
	 * 겹침 방지(-LabSeparate=0/1, Docs/v2-spec.md 3절). Mass 단계는 ULabSeparationProcessor가 겹친 개체의 위치를 직접 떼어 놓는다(v2 W4, D45).
	 * 단계 1은 액터 캡슐끼리 막기다(UsesActorBlocking, v2 W6, D47). 기본: 단계 2만 0(엔진 회피를 쓴다), 그 밖 1. CSV 메타데이터 LabTechSeparate.
	 */
	bool bSeparate = true;
	/**
	 * v2 W7 표현 전환의 최소 유지 시간(ms, -LabRepHold=<ms>, 0이면 끔, Docs/v2-spec.md 4.1, Docs/Worklog/V2.md "W7"). 기본: 사다리 단계 4부터 FLabQuality::DefaultRepHoldMs, 그 앞은 0.
	 * 한 번 표현 무리(액터·인스턴스·없음)가 바뀐 엔티티는 이 시간 안에 바로 전의 무리로 되돌아가지 않는다(ULabRepHoldProcessor). 액터 상한은 넘지 않는다. CSV 메타데이터 LabRepHold.
	 */
	int32 RepHoldMs = FLabQuality::DefaultRepHoldMs;
	bool UsesRepHold() const { return IsMass() && RepHoldMs > 0; }
	/**
	 * v2 W8 움직이는 오브용 기법(Docs/v2-spec.md 4.2, Docs/Worklog/V2.md "W8").
	 * bFxOrbFollow(-LabFxOrbFollow=0/1, 기본 1): 엔티티 구동 입자의 외삽 목표 User.LabOrbPos를 매 프레임 오브의 지금 위치로 넣는다(ULabCrowdFxGatherProcessor).
	 * 0이면 W7까지처럼 활성화 전에 한 번만 넣는다. 배경 무리를 쓸 때만 뜻이 있다. CSV 메타데이터 LabFxOrbFollow.
	 */
	bool bFxOrbFollow = true;
	/**
	 * v2 W8 ISKM 바운드 상자가 카메라를 따라간다(-LabBoundsStepM=<m>, 기본 FLabQuality::DefaultBoundsStepM). 카메라가 상자 중심에서 수평으로 이 거리 넘게 벗어나면
	 * 상자 중심을 카메라로 옮긴다(ULabCrowdSubsystem::ApplyFixedIskmBounds). 상자 반폭은 그리는 끝 × 1.1 + 이 거리 + 여유 10m다.
	 * 0이면 W7까지처럼 처음 오브 위치 둘레에 고정한다. 고정 바운드를 쓸 때만 뜻이 있다(UsesFixedBounds). CSV 메타데이터 LabBoundsStepM.
	 */
	float BoundsStepM = FLabQuality::DefaultBoundsStepM;
	/**
	 * v2 W9 ISKM 번쩍임(Docs/v2-spec.md 4.3, Docs/Worklog/V2.md "W9"). 컴포넌트마다 처음 쓰이는 프레임에 트랙 풀의 크기를 종류의 개체 수 × 1.1 + 64로 늘려 둔다(ULabIskmProcessor, LabReserveTracks).
	 * 트랙 수가 늘 때마다 렌더러가 뼈대 버퍼를 다시 할당하면서 겹친 자리로 이전 슬롯을 복사해 그 컴포넌트 전체의 속도가 한 프레임 튄다(engine-notes.md 42절).
	 * 0(-LabIskmTrackReserve=0)이면 W8까지처럼 엔진이 트랙을 하나씩 늘린다. ISKM을 쓸 때만 뜻이 있다. CSV 메타데이터 LabIskmTrackReserve.
	 */
	bool bIskmTrackReserve = true;
	/** 로그와 CSV 메타데이터 값. 그 기법을 쓰지 않는 실행은 "-" */
	FString FxOrbFollowString() const;
	FString BoundsStepString() const;
	FString IskmTrackReserveString() const;
	/** 분리 파라미터(LabSeparation.h). Test 빌드는 기본값만 쓴다. 개발 빌드는 -LabSepIter=, -LabSepRelax=, -LabSepMaxStep=, -LabSepVel=로 바꿔 본다 */
	FLabSeparationParams Separation;
	/**
	 * Animation Budget Allocator, 중요도 = 화면 크기(-LabScreenSigBudget=0/1). 액터를 Budget 템플릿으로 만들고 ActorSync가 화면 크기를 중요도로 넣는다.
	 * 기본: 사다리 단계 4부터 1. 인자를 주면 그 값이 이긴다(빼 보기 -LabScreenSigBudget=0).
	 */
	bool bScreenSigBudget = false;
	/**
	 * 애니메이션 예산(ms, -LabBudgetMs=<값>, 기본 FLabScenario::BudgetMs = 4, ADR-0016). 할당기를 켜는 실행에서 a.Budget.BudgetMs에 넣는다(ALabGameMode::ApplyBudgetCVars).
	 * 0 이하면 예산을 끈다(-LabScreenSigBudget=0과 같다). 기본값과 다르면 LabOverrides에 budget_ms=<값>으로 남는다.
	 */
	float BudgetMs = FLabScenario::BudgetMs;
	/** Q2 Niagara 배경 무리(-LabCrowdFx=0/1). 기본: 사다리 단계 7부터 1. 인자를 주면 그 값이 이긴다(빼 보기 -LabCrowdFx=0) */
	bool bCrowdFx = false;
	/**
	 * Q5b(-LabCrowdFxOuter=0/1, 기본 1). 0이면 바깥 독립 이미터(Small·Large)의 스폰율을 0으로 넣어(CrowdFxTotalRate) 입자를 내지 않는다. 에셋은 고치지 않는다.
	 * 스폰율을 활성화 전에 넣으므로 워밍업도 0으로 돌아 미리 채워지는 입자가 없다. 엔티티 구동 이미터는 스폰율을 읽지 않아 그대로다. bCrowdFx일 때만 뜻이 있다.
	 */
	bool bCrowdFxOuter = true;
	/** 실제 미니언과 같은 흐름(개/초): N / 맵 반경 × 이동 속도(Docs/Worklog/Q2.md "장면 수치"). N 1만이면 60. -LabCrowdFxOuter=0이면 0 */
	float CrowdFxTotalRate() const;
	float CrowdFxSmallRate() const;
	float CrowdFxLargeRate() const;
	/** 독립 입자가 사라지는 안쪽 반경(오브 기준, cm) = 맵 반경 500m. 그 안은 엔티티가 있고 그려지지 않는 엔티티는 엔티티 구동 이미터가 그린다(Q2b) */
	float CrowdFxInnerCm() const { return FLabScenario::MapRadius; }
	/** (바깥 반경 − 안쪽 반경) / 이동 속도 */
	float CrowdFxLifetime() const;
	/** 독립 입자의 정상 상태 수 계산값 = 스폰율 × 수명. 꺼져 있으면 0. CSV 메타데이터 LabCrowdFxParticles. 엔티티 구동 입자 수는 CSV 스탯 Lab/CrowdRecs다 */
	int32 CrowdFxParticles() const;
	/** 엔티티 구동 입자를 쓰는가. ULabCrowdFxGatherProcessor 등록 조건(HordeOptLab.cpp) */
	bool UsesCrowdFxEntity() const { return IsMass() && bCrowdFx; }
	/**
	 * Q3 빼 보기 스위치(Docs/Worklog/V2.md "W10" 빼 보기 표). 단계가 정한 구성에서 기법 하나만 뺀다. 0을 줄 때만 뜻이 있다(끄기 전용).
	 * 1이나 다른 값은 단계의 기본 동작 그대로다. 그 단계에 없는 기법을 켜지 않는다.
	 */
	bool bNoHydrate = false;     // -LabHydrate=0: 액터 구간도 인스턴스(ISKM 단계는 ISKM)로 그린다. 액터 0
	bool bNoSimLod = false;      // -LabSimLod=0: Simulation LOD Trait을 붙이지 않는다. 로직은 매 프레임, 이동은 전 개체가 엔진 이동·조향(단계 4와 같다)
	bool bNoIskm = false;        // -LabIskm=0: ISKM 단계에서도 40~200m를 VAT ISM으로 그린다(단계 5의 표현)
	bool bNoFixedBounds = false; // -LabFixedBounds=0: ISKM 컴포넌트 고정 바운드를 넣지 않는다
	bool bNoFarCull = false;     // -LabFarCull=0: 표현 LOD의 Off(그리지 않음)를 두지 않고 맵 끝까지 그린다. Simulation LOD Off 거리와 배경 무리 안쪽 반경은 그대로
	/**
	 * Q4 근거리 품질(FLabQuality). 기본: 사다리 단계 2~7은 500·30m, 단계 8은 1,000·40m. 인자가 이긴다.
	 * -LabActorCap=<개수>: 액터 상한(두 종류 합계). -LabHydrateM=<미터>: 화면 안에서 액터로 바뀌는 카메라 거리(1~79m). Mass 단계에서만 뜻이 있다.
	 */
	int32 ActorCap = FLabQuality::DefaultActorCap;
	float HydrateM = FLabQuality::DefaultHydrateM;
	float HydrateCm() const { return HydrateM * 100.f; }
	/**
	 * Q4b 원거리 그림자(-LabFarShadow=0|1). 거짓이면 표현 LOD Low(80m~그리는 끝) 인스턴스가 그림자를 그리지 않는다. Mass 단계에서만 뜻이 있다.
	 * 기본: 단계 8 거짓(끔), 그 밖 참. 인자가 이긴다. 인스턴스 단위 그림자 플래그가 없어서 메시 설명자를 LOD 중요도 범위로 둘로 나눈다
	 * (High~Low 그림자 켬, Low~Max 그림자 끔, 엔진이 설명자마다 컴포넌트를 하나씩 만든다). ISKM과 VAT ISM 모두 같다. CSV 메타데이터 LabFarShadow.
	 */
	bool bFarShadow = true;
	/** CSV 메타데이터 LabFarShadow: "1" 켬, "0" 끔, 액터 단계 "-" */
	const TCHAR* FarShadowString() const { return !IsMass() ? TEXT("-") : bFarShadow ? TEXT("1") : TEXT("0"); }
	/**
	 * 하이드레이션 액터(ALabMinionMassActor 계열)의 캡슐 콜리전(v2 W5, Docs/v2-spec.md 3절 기능 3, Docs/Worklog/V2.md "W5").
	 * Trace(기본): QueryOnly + 모든 채널 무시 + 저격 트레이스 채널(LabSnipe, LabSnipe.h)만 막음, 오버랩·물리 볼륨 갱신 끔.
	 * 캡슐끼리는 서로 무시하고 이동은 스윕 없는 텔레포트라 밀지 않는다. 오브가 액터 구간의 저격 대상에 이 채널로 라인 트레이스를 쏴 맞는지 센다(ALabOrb::TraceSnipeCapsules).
	 * Off(-LabActorCollision=0): NoCollision + 두 갱신 끔. On(=1): ACharacter 캡슐의 Pawn 프로필 그대로(Mass 액터 풀이 꺼낼 때 SetActorEnableCollision(true), MassActorSpawnerSubsystem.cpp:327).
	 * 판정(범위 피해, 저격 대상 선택)은 엔티티 위치의 수평 거리로 하고 캡슐을 쓰지 않는다. 단계 1 액터(ALabMinion)는 이 값과 무관하게 늘 켬이다
	 * (v2 W6: Pawn 프로필 + 저격 채널 막음, 메타데이터 "pawn", ALabMinion::InitMinion).
	 */
	enum class EActorCollision : uint8 { Off, On, Trace };
	EActorCollision ActorCollision = EActorCollision::Trace;
	/** 하이드레이션 액터 캡슐 콜리전의 실제 적용값. "trace" 기본, "0" 끔, "1" 켬, 단계 1 "pawn", 액터가 없으면 "-". CSV 메타데이터 LabActorCollision */
	const TCHAR* ActorCollisionString() const;
	/**
	 * v2 W11 액터 비용 후보(Docs/Worklog/V2.md "W11", engine-notes.md 44절).
	 * bFixedSkelBounds(-LabFixedSkelBounds=0/1): 하이드레이션 액터 메시의 bComponentUseFixedSkelBounds. 참이면 애니가 갱신될 때마다 피직스 에셋 바디로 바운드를 다시 셈하지 않고
	 * 메시 에셋의 고정 바운드를 쓴다(SkinnedMeshComponent.cpp CalcMeshBound). CSV 메타데이터 LabFixedSkelBounds("1"/"0", 액터가 없으면 "-").
	 * bQueryIsolate(-LabQueryIsolate=0/1): p.Chaos.AccelerationStructureIsolateQueryOnlyObjects. 참이면 QueryOnly 바디(액터 캡슐 Trace)가 가속 구조의 따로 된 칸에 들어간다.
	 * 물리 씬을 만들기 전에 넣어야 해서 게임 모듈 시작 때 넣는다(HordeOptLab.cpp). CSV 메타데이터 LabQueryIsolate(실제 CVar 값).
	 * 기본: 고정 바운드는 단계 8만 참(BoundsScale FLabQuality::FixedSkelBoundsScale을 함께 곱한다), 칸 나누기는 모든 단계에서 거짓(묶음 V2W11Cost에서 실행 간 범위 안). 인자가 이긴다.
	 */
	bool bFixedSkelBounds = false;
	bool bQueryIsolate = false;
	const TCHAR* FixedSkelBoundsString() const { return !UsesHydration() ? TEXT("-") : bFixedSkelBounds ? TEXT("1") : TEXT("0"); }
	/**
	 * 단계 기본값과 다르게 준 인자(Q5). "hydrate_m=70|actor_collision=1" 꼴(CSV 메타데이터 행이 쉼표로 나뉘어 | 로 잇는다), 없으면 빈 문자열. 사다리 단계를 그대로 고른 실행인지 가리는 데 쓴다.
	 * CSV 메타데이터 LabOverrides(빈 값이면 "-"), HUD 둘째 줄.
	 */
	FString Overrides;
	/** 사다리 단계의 번호와 HUD 이름(영어). CSV 메타데이터 LabLadder(= LabStage) */
	struct FLadder { FString Id; FString Name; };
	FLadder Ladder() const;

	/** 사다리 단계 1: 모든 개체가 액터(ALabMinion)다. Mass 엔티티를 만들지 않는다 */
	bool IsActorCrowd() const { return Step == 1; }
	/** 사다리 단계 2부터: 개체가 Mass 엔티티이고 표현 LOD가 액터(가까이)·인스턴스·없음을 고른다 */
	bool IsMass() const { return Step >= 2; }
	/** 인스턴스가 움직이는가(단계 4부터: 4·5와 ISKM을 끈 빼 보기는 VAT, 6부터 ISKM). 단계 2·3은 엔진 ISM 프로세서(UMassUpdateISMProcessor)와 정지 메시다. CSV 메타데이터 LabTechInstAnim */
	bool UsesVat() const { return Step >= 4; }
	/** 가까운 개체를 하이드레이션 액터로 그리는가(Mass 단계, -LabHydrate=0이면 거짓) */
	bool UsesHydration() const { return IsMass() && !bNoHydrate; }
	/** 표현 LOD의 Off(그리는 끝 밖을 그리지 않음)가 있는가. Mass 단계에서 참, -LabFarCull=0이면 거짓 */
	bool CullsFar() const { return IsMass() && !bNoFarCull; }
	/**
	 * 표현 LOD의 Off 거리(cm). 지우기를 끄면 맵 반경의 두 배(1,000m)라 맵 안의 개체(카메라에서 최대 515m)는 Off에 들지 않는다.
	 * 1e7로 두지 않는 것은 풀 엔티티를 세워 두는 자리(약 14km, LabCrowdSubsystem.cpp LabPoolParkLocation)가 Off로 남아야 그리지 않기 때문이다.
	 */
	float RepOffCm() const { return bNoFarCull ? FLabScenario::MapRadius * 2.f : FarCm(); }
	/** 그리는 범위의 반경(cm, 오브 기준). 고정 바운드 크기에 쓴다. 지우기를 끄면 맵 반경 */
	float DrawExtentCm() const { return bNoFarCull ? FLabScenario::MapRadius : FarCm(); }
	/** Animation Budget Allocator로 도는 액터 템플릿을 쓰는가(화면 크기 Budget) */
	bool UsesBudgetActors() const { return IsMass() && bScreenSigBudget; }
	/** Simulation LOD Trait을 붙이고 로직 프로세서 둘(접근·범위 피해)이 청크 필터를 거는가(단계 5부터, -LabSimLod=0이면 거짓). 200m 밖은 Off 밴드와 ULabOffMoveProcessor다 */
	bool UsesSimLod() const { return Step >= 5 && !bNoSimLod; }
	/** 40~200m를 ISKM으로 그리는가(단계 6부터, -LabIskm=0이면 거짓) */
	bool UsesIskm() const { return Step >= 6 && !bNoIskm; }
	/** ISKM 컴포넌트에 고정 바운드를 넣는가(ISKM을 쓸 때, -LabFixedBounds=0이면 거짓). 상자는 카메라를 따라간다(BoundsStepM) */
	bool UsesFixedBounds() const { return UsesIskm() && !bNoFixedBounds; }
	/** 엔티티에 엔진 회피 Trait을 붙이는가(Mass 단계, bAvoid) */
	bool UsesAvoidance() const { return IsMass() && bAvoid; }
	/** 겹친 개체를 직접 떼어 놓는가(Mass 단계, bSeparate, ULabSeparationProcessor 등록 조건) */
	bool UsesSeparation() const { return IsMass() && bSeparate; }
	/**
	 * v2 W6 액터 기준선의 겹침 방지(Docs/Worklog/V2.md "W6"). 단계 1 액터의 캡슐이 서로(Pawn 채널) 막고 CharacterMovement가 스윕 이동으로 비켜 미끄러진다.
	 * 캡슐 반지름은 몸통 반지름(소형 40, 대형 94)이다. -LabSeparate=0이면 캡슐끼리 무시한다(W5까지의 단계 1). CSV 메타데이터 LabTechSeparate는 둘 중 하나가 켜지면 1이다
	 */
	bool UsesActorBlocking() const { return IsActorCrowd() && bSeparate; }

	/**
	 * 빼 보기 기법의 켬·끔(설정 기준). 메타데이터 키(LabTech*)와 로그 키, 값("1"/"0", 전환 대책 일부만 켜면 "m<마스크>").
	 * 기법끼리 딸린 효과(하이드레이션을 끄면 예산을 받을 액터가 없다, ISKM을 끄면 고정 바운드를 넣을 컴포넌트가 없다)는 여기서 지우지 않고
	 * 실제 적용값 메타데이터(LabBudget, LabIskmBounds)로 본다.
	 */
	struct FTech { const TCHAR* MetaKey; const TCHAR* LogKey; FString Value; };
	TArray<FTech> TechList() const;
	/** "tech=anim:1,hyd:1,..." 꼴(lab_config 로그 끝) */
	FString TechString() const;
	enum EHydrateFix : int32
	{
		FixStartTime = 1,  // 시작 시각 맞추기(AnimBP Start Position)
		FixWait = 2,       // 준비될 때까지 인스턴스 유지 + 되돌아갈 때 한 프레임 더 + 그동안 액터 숨김
		FixPassHide = 4,   // 숨김을 액터 숨김 대신 메인·깊이 패스·그림자에서만 빼기(FixWait일 때만 뜻이 있다)
		FixLift = 8,       // 엔진 이동(TeleportActor)에 캡슐 반높이 더하기
		FixTintReset = 16, // 풀에 넣을 때 진홍색 지우기
	};
	bool HasFix(EHydrateFix F) const { return (HydrateFixMask & F) != 0; }

	/** 로그와 CSV 메타데이터용. "80/200m tick=0/0.50/1.00" 꼴, Simulation LOD가 없으면 "-" */
	FString SimLodString() const;

	static const FLabConfig& Get();
	FString ToLogString() const;
};
