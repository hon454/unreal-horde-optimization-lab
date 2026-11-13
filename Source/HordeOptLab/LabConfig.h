#pragma once

#include "CoreMinimal.h"

/** 미니언 표현 변형(M0 E0). 실행 인자 -LabVariant=로 고른다. plan.md "M0". */
enum class ELabVariant : uint8
{
	Full,   // ACharacter + 스켈레탈 메시 + 애니메이션
	Static, // 같은 ACharacter + UStaticMeshComponent, 애니메이션 없음
	Budget, // Full + USkeletalMeshComponentBudgeted + Significance 거리 단계 + a.Budget.BudgetMs 2
	Uro,    // Full + URO만
	Pawn,   // Full에서 CharacterMovement 틱을 끄고 위치를 직접 이동
	Mass,   // M1부터의 기본. 단계가 표현을 정하므로 M0에서는 Full과 같다
	MassSt, // M1 E1 (b): mass.FullyParallel 0(Mass 처리를 게임 스레드 하나에서)
	MassAv, // M1 E1 (c): 엔진 회피(UMassObstacleAvoidanceTrait + UMassNavigationObstacleTrait) 켬
	// M1 E1 (d) 피해 적용 방식: 비치명 피해를 초당 1천(lo)·1만(hi) 건 무작위 개체에 넣는다(체력 최소 1, 사망 없음).
	MassDirLo, // 직접 깎기, 초당 1천
	MassDirHi, // 직접 깎기, 초당 1만
	MassEvtLo, // 피해 이벤트 엔티티로 모아 깎기, 초당 1천
	MassEvtHi, // 피해 이벤트 엔티티로 모아 깎기, 초당 1만
	// M2 E2 하위 실험(-LabStage=M2와 함께). 대표는 Mass(모든 거리를 Bone VAT로 그림)
	VatVert,    // E2 (b): Vertex 모드 VAT(텍스처 메모리 비교)
	VatCull,    // E2 (c)(1): 200m 밖은 그리지 않는다(거리만, 확정값 "200m~ 없음")
	VatCullVis, // E2 (c)(2): 화면 밖일 때만 200m 밖을 그리지 않는다(BaseLODDistance 200m, VisibleLODDistance 맵 끝)
	VatDie0,    // E2 (d) 첫 설계(쓰지 않음): 범위 피해·저격을 꺼서 개체가 오브 주위에 쌓였다. 사망 비용이 아니라 밀집을 쟀다
	VatDie1k,   // E2 (d) 첫 설계(쓰지 않음): VatDie0 + 초당 1천 명 피격→사망
	Kill0,      // E2 (d): 평소 시나리오(범위 피해·저격으로 초당 약 10명 사망). Mass와 같고 같은 묶음에서 잴 이름만 다르다
	Kill1k,     // E2 (d): 평소 시나리오 + 초당 1천 명을 무작위로 피격→사망(인스턴스 제거·재생성 비용)
	// M3 E3 계단(-LabStage=M3와 함께). 대표는 Mass(하이드레이션만)
	M3Uro,       // E3 (2): 액터 메시에 URO
	M3BudgetDist,// E3 (3a): URO 대신 Animation Budget Allocator, 중요도는 거리 단계(M0 budget과 같은 25/50/100/250m)
	M3BudgetSig, // E3 (3b): Budget Allocator, 중요도는 Mass LODSignificance 변환값(1 - LODSignificance / 3)
	M3NoActor,   // 진단: M3 단계 그대로 두되 0~30m도 VAT 인스턴스(액터 없음). 액터가 다른 프로세서 비용에 주는 영향을 가른다
	M3LowRes,    // E3 (5): 0~15m 액터, 15~30m 저품질 액터(URO, 가장 낮은 메시 LOD, 그림자 끔), 30~200m VAT 인스턴스
	M3ColQuery,  // E3 (6): 액터 캡슐을 QueryOnly로, 오버랩·물리 볼륨 갱신 끔(트레이스로 맞히는 게임의 절충)
	M3NoCol,     // E3 (6): 액터 캡슐 콜리전 끔(판정을 Mass가 갖는 퍼핏 구조)
	M3Hyd15,     // E3 (7): 하이드레이션 거리 15m(대표 30m). 나머지 밴드·상한은 그대로
	M3Hyd50,     // E3 (7): 하이드레이션 거리 50m
	// M3b(plan.md "M3b"). 별칭 m0full = Full, m0static = Static, m1mass·m2mass·m3mass = Mass(묶음 1 라벨이 단계별로 갈리게 이름만 다르다)
	StaticPawn,  // M0 코드 정비 5: static(정적 메시, 애니 없음) + pawn(CharacterMovement 틱 끔). 컨테이너 전환만의 값을 보는 대조군
	M3QOld,      // 진단: 정비 전 구조. VAT·ActorSync가 로직 프래그먼트와 태그를 읽고, 저격이 쿼리 밖에서 쓰고, 로직 뒤 순서 선언 없음(코드 정비 1·2 A/B)
	M3SyncAll,   // 진단: ActorSync가 액터 태그 필터 없이 전 엔티티를 순회(코드 정비 4 A/B)
	M3Vis0,      // E3b (8): 화면 밖 High 밴드 0m(카메라 뒤는 액터로 만들지 않음), 화면 안 30m. 대조군
	M3Vis15,     // E3b (8): 화면 밖 High 밴드 15m, 화면 안 30m(MetaHuman Crowd 비율). 결정 1로 M3 기본값이 되어 m3mass와 같다. run-m3b.ps1이 쓰는 이름이라 남긴다
	M3Vis30,     // E3b (8) 기준점: 결정 1 전의 M3 대표값(화면 밖 Base 30m, 화면 안 Visible 30m)
	M3Hys20,     // E3b (8): 30/30m에 히스테리시스 20%(엔진 기본 10)
	M3Lock0,     // 진단: 정비 코드 그대로, mass.commands.LockObserversDuringFlushing만 0(ini의 1을 이 변형에서만 덮는다). m3mass가 M3 묶음보다 0.7ms 느린 원인 가르기
	M3Sig,       // E3 (3c): Budget Allocator, 중요도는 화면 크기(바운드 반경 ÷ 카메라 거리). 비교 상대는 (3a) m3budgetdist. 새 스탯 LargeActorSkips
	M3Order,     // A/B: VatISM·ActorSync에 ExecuteAfter(LabLogic)를 선언한 쪽. 기본(m3mass)은 선언 없음(옛 m3noorder 동작). 20k P1 m3noorder 15.52 vs m3mass 16.12ms
	// M4 E4(-LabStage=M4와 함께, plan.md "M4"). 대표는 Mass(별칭 m4mass): Simulation LOD 80/200m, 로직 틱 Medium 0.5초 / Low 1.0초
	M4Off,       // E4 (a) 끔: Trait과 LOD 태그는 그대로, 틱 간격만 전부 0(매 프레임). m3mass → m4off는 태그로 아키타입이 늘어난 비용, m4off → m4mass는 틱 간격의 효과
	M4M25L50,    // E4 (b): Medium 0.25초 / Low 0.5초
	M4M25L100,   // E4 (b): Medium 0.25초 / Low 1.0초
	M4M50L50,    // E4 (b): Medium 0.5초 / Low 0.5초
	M4OffMove,   // E4 (d): 200m 밖을 Simulation LOD의 Off로 두어 엔진 이동·조향 넷에서 빼고(넷 모두 FMassOffLODTag를 None으로 요구한다) ULabOffMoveProcessor가 틱이 온 프레임에 흐른 시간만큼 오브로 직진시킨다. Docs/Worklog/M4.md "E4 (d) Low 밴드 이동"
	M4Avoid,     // E4 (c): 대표 + 엔진 회피 Trait. 회피 프로세서는 High 태그만 처리하므로(MassAvoidanceProcessors.cpp:301-303) 80m 안만 비켜 간다
	// M5 E5(-LabStage=M5와 함께, plan.md "M5"). M5 단계의 Simulation LOD는 모두 m4offmove 구성(200m 밖 Off + Lab 이동 프로세서)이다.
	// 별칭 m5mass = M5Iskm. m4offmove는 M5 단계에서도 지난 묶음과 같은 구성(VAT, 엔티티 풀링 없음)이다.
	M4OffPool,   // 기준점: m4offmove + 엔티티 풀링(원거리 표현은 VAT)
	M5Iskm,      // M5 대표: 원거리 표현 ISKM(BlendTime 0, AnimationMinScreenSize -1, 그림자 켬) + 엔티티 풀링
	M5Destroy,   // E5 (b) 누수 진단: m5iskm에서 엔티티 풀링 끔(사망 개체를 파괴하고 새로 만든다)
	M5Blend,     // 참고: m5iskm + BlendTime 0.25초
	M5MinSS,     // 참고: m5iskm + AnimationMinScreenSize 0.1(엔진 r.Skinning.DefaultAnimationMinScreenSize 기본값)
	M5Bounds,    // 참고: m5iskm + ISKM 컴포넌트 고정 바운드(SetPrimitiveBoundsOverride). 프레임 끝 인스턴스별 CalcBounds 반복이 SendAllEndOfFrameUpdates 증가분의 원인인지 가른다(Docs/Worklog/M5.md "트레이스 검토")
	// Q2(-LabStage=M5와 함께, plan.md "품질 계획" Q2). 이름은 Q5에서 단계 이름과 함께 바뀔 수 있다.
	Q2Crowd,     // m5bounds + Niagara 배경 무리(-LabCrowdFx 기본 1). 표현·풀링·Budget은 m5bounds와 같다
	// Q4(-LabStage=M5와 함께, plan.md "품질 계획" Q4, Docs/Worklog/Q4.md). 사다리 8단계 = 최종 구성 후보
	Q4Final,     // q2crowd + Q4에서 고른 액터 상한·전환 거리(FLabQuality::Q4FinalActorCap·Q4FinalHydrateM). 나머지는 q2crowd와 같다
};

/**
 * 근거리 품질 설정(Q4, Docs/Worklog/Q4.md). 액터 상한 = 표현 LOD High의 LODMaxCount(두 종류의 템플릿이 같은 LOD 파라미터라 한 공유 프래그먼트를 써서 합계 상한이다,
 * MassVisualizationTrait.cpp:145 GetOrCreateSharedFragment), 전환 거리 = 화면 안 High 경계(VisibleLODDistance[Medium]). 화면 밖 경계(15m, M3b 결정 1)는 그대로다.
 * High 경계에는 히스테리시스가 없다(엔진은 한 단계 위로 오를 때 그 위 밴드의 경계에만 둔다, MassLODCalculator.h:868-886).
 */
struct FLabQuality
{
	static constexpr int32 DefaultActorCap = 500;   // 확정값 LODMaxCount High(measurement.md)
	static constexpr float DefaultHydrateM = 30.f;  // 확정값 거리 밴드 0~30m
	static constexpr float MaxHydrateM = 79.f;      // 80m(Medium/Low 경계, 확정값)를 넘지 않는다
	// Q4 묶음에서 고른 값(Docs/Worklog/Q4.md "최종 구성"): 1만 P1 프레임 p95 11.98ms(13.3ms 안), 액터 평균 709개. 1만에서는 상한이 걸리지 않고 2만에서 1,000개로 막는다
	static constexpr int32 Q4FinalActorCap = 1000;
	// Q4b 표현 LOD 재편: 79m → 40m. Q4 묶음에서 대형 액터의 애니 갱신 건너뜀 비율이 40m 0.03~0.07, 50m 0.25~0.31, 79m 0.66이었다
	static constexpr float Q4FinalHydrateM = 40.f;
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
 * Q2 Niagara 배경 무리(/Game/Lab/FX/NS_LabCrowd, Docs/Worklog/Q2.md). 그리는 끝 밖을 실제 미니언과 연결되지 않은 GPU 입자로 채운다.
 * 입자는 바깥 고리에서 생겨 미니언과 같은 속도로 오브를 향해 걷고, 수명이 끝나는 안쪽 반경에서 사라진다(페이드 없음).
 */
struct FLabCrowdFx
{
	static constexpr float OuterRadius = 100000.f; // 1,000m. 바닥 반쪽 길이 1,020m(Scripts/create-bench-map.py)
	static constexpr float LargeShare = 0.1f;      // 2종의 대형 비율(일련번호 10의 배수가 대형, LabUnitAssets.h). 1종이면 0
	// 실제 미니언이 Off에서 그려지기 시작하는 카메라 거리 = Lerp(Low 경계 80m, 그리는 끝, 1 − 히스테리시스 10%)(MassLODCalculator.h:864-886, 표현 LOD 기본 히스테리시스 10%,
	// MassRepresentationFragments.h:309). 그리는 끝 200m면 188m다. 밴드 80m는 LabCrowdSubsystem.cpp의 표현 LOD 밴드와 같은 값이다.
	static constexpr float RepLowCm = 8000.f;
	static constexpr float RepHysteresis = 0.1f;
	/**
	 * Q2b 엔티티 구동 입자(-LabCrowdFxEntity, 기본 1, Docs/Worklog/Q2.md "Q2b"). 그려지지 않는 걷는 엔티티(표현 None)의 위치를 매 프레임 종류별 Niagara 배열로 넘긴다(ULabCrowdFxGatherProcessor).
	 * 원소 = (X, Y, T0, 위상): 월드 좌표(cm), 그 위치에 있던 월드 시각(초), 걷기 위상(초, = −StateStartTime). 에셋은 P = P0 + 오브 방향 × Speed × max(LabNow − T0, 0)로 외삽한다.
	 * 이름은 에셋 단위와의 약속이다. 에셋에 없으면 경고 한 번 남기고 그 값만 넣지 않는다.
	 */
	static constexpr const TCHAR* SmallRecsParam = TEXT("User.LabSmallRecs"); // Niagara Array Float4
	static constexpr const TCHAR* LargeRecsParam = TEXT("User.LabLargeRecs"); // Niagara Array Float4
	static constexpr const TCHAR* NowParam = TEXT("User.LabNow");             // float, 월드 초(매 프레임)
	static constexpr const TCHAR* OrbPosParam = TEXT("User.LabOrbPos");       // Vector(또는 Position), 오브 월드 위치(활성화 전 한 번)
};

/**
 * M4 Simulation LOD 설정. 거리는 카메라 기준이고 화면 안팎을 가리지 않는다(MassLODLogic.h:42).
 * 기본은 Off 밴드를 두지 않는다: 엔진의 Off는 위치를 이동 목표(오브)로 옮기고 이동·조향에서 뺀다(engine-notes.md 5절).
 * bOffMove(m4offmove·m4offpool과 M5 단계)만 200m 밖을 Off로 둔다. 순간 이동 프로세서는 M4부터 등록하지 않고(HordeOptLab.cpp) Lab 프로세서가 대신 옮긴다.
 */
struct FLabSimLod
{
	static constexpr float MediumDistance = 8000.f;  // High 0~80m
	static constexpr float LowDistance = 20000.f;    // Medium 80~200m, Low 200m~
	float TickMedium = 0.5f; // 초. High는 0(매 프레임)
	float TickLow = 1.0f;
	bool bAvoidance = false;
	bool bOffMove = false;   // E4 (d): Low 밴드 대신 Off 밴드(간격은 TickLow), 이동은 ULabOffMoveProcessor
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
	static constexpr float TopDownHeight = 10000.f;  // P2 탑뷰 100m. 가장 가까운 개체도 100m 떨어져 근거리 밴드(초안 30m)에 들지 않는다. 300m는 너무 멀었다

	// budget 변형의 Significance 거리 단계(25/50/100/250m). plan.md "M0"
	static constexpr float BudgetTiers[4] = { 2500.f, 5000.f, 10000.f, 25000.f };
	static constexpr float BudgetMs = 2.f;
};

/**
 * 최적화 단계(마일스톤). 모든 단계가 한 빌드에 공존하고 -LabStage=로 고른다(ADR-0004).
 * 스포너·오브·카메라·러너·CSV는 모든 단계가 공유하고, 단계는 개체를 표현하고 움직이는 방식만 바꾼다.
 * 마일스톤을 끝낼 때 이전 단계를 지우지 않고 새 단계를 더한다.
 */
enum class ELabStage : uint8
{
	M0, // 액터 군중(변형은 ELabVariant)
	M1, // Mass 코어: 같은 로직을 Fragment·Processor로, 표현은 애니 없는 ISM
	M2, // Representation LOD: 같은 ISM에 VAT 애니메이션(걷기·피격·죽기). 대표 설정은 모든 거리를 그린다
	M3, // 액터 하이드레이션: 0~30m는 액터(최대 500), 30~200m VAT 인스턴스, 200m 밖 Off
	M4, // Simulation LOD: M3 표현 그대로, 로직 프로세서 둘(접근·범위 피해)이 80m 밖에서 틱 간격을 늘린다(FLabSimLod). 저격과 상태 타이머는 매 프레임 돈다
	M5, // ISKM: m4offmove 구성 위에서 30~200m 표현을 VAT ISM → ISKM으로 바꾼다(ULabIskmProcessor). 엔티티 풀링 기본 켬(-LabEntityPool)
};

/** M5 ISKM 렌더·재생 설정(plan.md "M5" 결정 5·6). 변형이 정한다. */
struct FLabIskmParams
{
	float BlendTime = 0.f;               // 클립 전환 크로스페이드(초). 대표 0 = VAT와 같이 끊어 바꾸기
	float AnimationMinScreenSize = -1.f; // < 0 항상 재생. 0이면 엔진 r.Skinning.DefaultAnimationMinScreenSize(MassSkinnedMeshRepresentationTypes.h:96-101)
	bool bCastShadows = true;            // 엔진 설명자 기본값은 false(MassSkinnedMeshRepresentationTypes.h:79)
	bool bFixedBounds = false;           // m5bounds: 컴포넌트 바운드를 고정한다(ULabCrowdSubsystem::ExcludeVisualsFromLumen)
};

/** 실행 인자. 프로세스 시작 시 한 번 읽는다. */
struct FLabConfig
{
	ELabStage Stage = ELabStage::M0;
	bool bStageValid = true;   // 아직 없는 단계를 고르면 false. 게임 모드가 종료 코드 2로 끝낸다
	bool bVariantValid = true; // 모르는 -LabVariant 이름이면 false. 게임 모드가 종료 코드 2로 끝낸다
	/**
	 * -LabHud: 상태 오버레이를 켠 채 시작(F1로도 토글). Q5부터 -LabBench와 함께 줘도 켜진다(촬영 실행 = 측정과 같은 조건 + HUD).
	 * 측정 실행은 주지 않는다. 켜졌는지는 CSV 메타데이터 LabHud로 남아 러너가 거른다(Docs/Worklog/Q5.md "HUD").
	 */
	bool bHud = false;
	int32 N = 500;
	int32 Preset = 1;          // 1 오브 시점, 2 탑뷰(P3는 2026-11-07에 뺐다)
	int32 Seed = 20261106;
	/**
	 * Q1 개체 체력 고정(-LabHealthFixed=<값>, 기본 0 = 분산). 0보다 크면 모든 개체 체력이 그 값이다(분산 전 상태 재현용 디버그 인자, 예: 100).
	 */
	float HealthFixed = 0.f;
	/** Q1 개체 체력: 시드와 스폰 일련번호의 결정적 해시로 [HealthMin, HealthMax] 균등. 초기 배치·재생성·풀 재사용·액터 단계 모두 이 함수 하나를 쓴다 */
	float HealthFor(uint32 Serial) const;
	ELabVariant Variant = ELabVariant::Full;
	bool bBench = false;       // 벤치마크 러너(M0 7번)
	FString Label;
	float DurationSec = 0.f;   // 0보다 크면 그 시간 뒤 종료(개발용 -Small 실행). -LabBench에서는 측정 구간 길이(기본 60초 대신, M5 E5 (b) 960)
	float ShotAtSec = 0.f;     // 0보다 크면 그 시각에 QHD 스크린샷 한 장(Saved/Screenshots/)
	/**
	 * M3 전환(하이드레이션) 대책 단계. 대책을 하나씩 쌓아 온 과정을 같은 장면으로 다시 보이려고 둔다(-LabHydrateFix=, 기본 4 = 전부).
	 * 0 상태만 맞춤, 1 + 시작 시각 맞추기, 2 + 준비될 때까지 VAT 유지(액터 숨김), 3 숨김 대신 화면·그림자에서만 빼기, 4 + 엔진 이동 높이 맞추기.
	 * 근거와 캡처는 Measurements/M3/shots/transition/README.md.
	 */
	int32 HydrateFix = 4;
	/**
	 * 전환 대책을 하나씩 따로 켜고 끈다(-LabHydrateFixMask=, 기본 31 = 전부). 문제 크기를 대책마다 따로 재려고 둔다.
	 * -LabHydrateFix=L을 주면 단계 L에 맞는 마스크가 된다(0→0, 1→1, 2→3, 3→7, 4→31).
	 */
	int32 HydrateFixMask = 31;
	/**
	 * 개체 종류 수(-LabTypes=, 기본 2. M4부터 기본 시나리오가 2종이다, plan.md "M4"). 2면 일련번호 10의 배수가 대형이다
	 * (소형 90 / 대형 10, plan.md "M3b" 2종). 아키타입·VAT·ISM·액터 풀이 종류별로 갈리고 저격은 예약 발에서만 대형을 쏜다(LabSnipe.h).
	 * -LabTypes=1이면 M3b까지의 1종 결과가 그대로다(LabUnitAssets.h).
	 */
	int32 Types = 2;
	/** -LabFullyParallel=0이면 mass.FullyParallel을 0으로 둔다(직렬 트레이스, run-m3b.ps1 -SerialArgs). -1은 건드리지 않음. massst 변형과 같은 경로 */
	int32 FullyParallel = -1;
	/**
	 * 그리는 거리 실험(-LabFarM=<미터>, 기본 200 = 확정값, Docs/Worklog/M4.md "그리는 거리 인자").
	 * 표현 LOD의 Off 거리(이 밖은 그리지 않는다)와 m4offmove의 Simulation LOD Off 거리를 함께 옮긴다. 30m·80m 경계, Simulation LOD의 Low 경계(200m), 저격 구간, 밴드 집계 경계는 그대로다.
	 * 200보다 작은 값은 200으로 올린다(Low 경계보다 안쪽의 Off는 다루지 않는다).
	 */
	int32 FarM = 200;
	float FarCm() const { return float(FarM) * 100.f; }
	/**
	 * M5 엔티티 풀링(-LabEntityPool=0/1, plan.md "M5" 결정 1). 사망 개체를 파괴하지 않고 풀에 두었다가 재생성 때 다시 꺼낸다(ISKM 트랙 누수 우회).
	 * 기본: M5 단계 1, 다른 단계 0. 변형 m4offpool은 어느 단계에서든 1, m4offmove·m5destroy는 0. 인자를 주면 그 값이 이긴다.
	 */
	bool bEntityPool = false;
	/**
	 * Budget Allocator, 중요도 = 화면 크기(-LabScreenSigBudget=0/1). 액터를 Budget 템플릿으로 만들고 ActorSync가 화면 크기를 중요도로 넣는다.
	 * 기본: M3·M4·M5 단계 1, 그 밖 0. 변형 m3sig는 항상 1. 비교 상대인 m3mass(와 m3base·m3colbase·m3hydbase)·m3uro·m3budgetdist·m3budgetsig·m3lowres·m3noactor와
	 * M3 하위 실험·진단 변형은 0. 인자를 주면 그 값이 이긴다. M3 이상 단계에서만 뜻이 있다.
	 */
	bool bScreenSigBudget = false;
	/**
	 * 애니메이션 예산(ms, -LabBudgetMs=<값>, 기본 FLabScenario::BudgetMs = 2). 할당기를 켜는 실행에서 a.Budget.BudgetMs에 넣는다(ALabGameMode::ApplyVariantCVars).
	 * 0 이하면 예산을 끈다. 끄는 길은 빼 보기 A8-budget(-LabScreenSigBudget=0)과 같다: bScreenSigBudget을 거짓으로 둬 액터를 Budget 템플릿으로 만들지 않는다.
	 * 변형이 스스로 정하는 예산(M0 budget, m3budgetdist, m3budgetsig)은 0 이하를 받아도 켜진 채로 남는다. 기본값과 다르면 LabOverrides에 budget_ms=<값>으로 남는다.
	 */
	float BudgetMs = 2.f;
	/**
	 * Q2 Niagara 배경 무리(-LabCrowdFx=0/1). 기본: 변형 q2crowd만 1, 그 밖 0(기존 단계·변형의 동작은 그대로). 인자를 주면 그 값이 이긴다.
	 * 단계와 기본값의 연결은 Q3·Q5에서 단계 이름과 함께 정한다(Docs/Worklog/Q2.md (마)).
	 */
	bool bCrowdFx = false;
	/**
	 * Q2b(-LabCrowdFxEntity=0/1, 기본 1). 1: 그려지지 않는 엔티티 자리에 입자를 그리고(엔티티 구동 이미터), 독립 입자는 맵 밖(500~1,000m)만 채운다.
	 * 0: Q2 3단위의 옛 방식(전부 독립 입자, 안쪽 반경 172.8m, 엔티티 배열을 넣지 않음). bCrowdFx일 때만 뜻이 있다.
	 */
	bool bCrowdFxEntity = true;
	/**
	 * Q5b(-LabCrowdFxOuter=0/1, 기본 1). 0이면 바깥 독립 이미터(Small·Large)의 스폰율을 0으로 넣어(CrowdFxTotalRate) 입자를 내지 않는다. 에셋은 고치지 않는다.
	 * 스폰율을 활성화 전에 넣으므로 워밍업도 0으로 돌아 미리 채워지는 입자가 없다. 엔티티 구동 이미터는 스폰율을 읽지 않아 그대로다.
	 * Niagara 대 ISKM 비교 조건 S8-fxent(Scripts/full-conditions.json, Docs/Worklog/Q5.md "Q5b 비교 조건"). bCrowdFx일 때만 뜻이 있다.
	 */
	bool bCrowdFxOuter = true;
	/** 개발용: 독립 입자가 사라지는 안쪽 반경(-LabCrowdFxInnerM=<미터>, 0 = 기본 CrowdFxInnerCm). 200m 경계 비교 촬영에 쓴다. Test 빌드에서는 읽지 않는다 */
	float CrowdFxInnerOverrideM = 0.f;
	/** 실제 미니언과 같은 흐름(개/초): N / 맵 반경 × 이동 속도(Docs/Worklog/Q2.md "장면 수치"). N 1만이면 60. -LabCrowdFxOuter=0이면 0 */
	float CrowdFxTotalRate() const;
	float CrowdFxSmallRate() const;
	float CrowdFxLargeRate() const;
	/**
	 * 독립 입자가 사라지는 안쪽 반경(오브 기준, cm). 수명은 이 값을 따라간다.
	 * 엔티티 구동(bCrowdFxEntity, 기본)이면 맵 반경 500m다. 그 안은 엔티티가 있고 그려지지 않는 엔티티는 엔티티 구동 이미터가 그린다.
	 * 옛 방식(-LabCrowdFxEntity=0)은 P1 카메라 앞에서 실제 미니언이 나타나는 자리다:
	 * 카메라 거리 H = Lerp(80m, 그리는 끝, 0.9)를 높이 9m로 수평 거리로 바꾸고 카메라가 오브 뒤 15m인 만큼 뺀다. 그리는 끝 200m면 172.8m(Docs/Worklog/Q2.md "200m 경계의 위치").
	 * P2에서도 같은 값을 쓴다(P2는 입자가 화면에 없고, 프리셋에 따라 입자 수가 바뀌지 않게 한다).
	 */
	float CrowdFxInnerCm() const;
	/** (바깥 반경 − 안쪽 반경) / 이동 속도. 0 이하면 켜지 않는다(그리는 끝이 1,000m 이상) */
	float CrowdFxLifetime() const;
	/** 독립 입자의 정상 상태 수 계산값 = 스폰율 × 수명. 꺼져 있으면 0. CSV 메타데이터 LabCrowdFxParticles. 엔티티 구동 입자 수는 CSV 스탯 Lab/CrowdRecs다 */
	int32 CrowdFxParticles() const;
	/** 엔티티 구동 입자를 쓰는가(bCrowdFx && bCrowdFxEntity). ULabCrowdFxGatherProcessor 등록 조건(HordeOptLab.cpp) */
	bool UsesCrowdFxEntity() const { return bCrowdFx && bCrowdFxEntity; }
	/**
	 * Q3 빼 보기 스위치(plan.md "품질 계획" 빼 보기 표, Docs/Worklog/Q3.md). 변형이 정한 구성에서 기법 하나만 뺀다. 새 인자 다섯은 0을 줄 때만 뜻이 있다(끄기 전용).
	 * 1이나 다른 값은 변형·단계의 기본 동작 그대로다. 그 단계에 없는 기법을 켜지 않는다. 나머지 네 행은 기존 인자를 쓴다
	 * (-LabScreenSigBudget=0 애니 예산, -LabHydrateFixMask=0 전환 대책, -LabEntityPool=0 엔티티 풀, -LabCrowdFx=0 배경 무리).
	 */
	bool bNoHydrate = false;     // -LabHydrate=0: 0~30m도 인스턴스(M5는 ISKM)로 그린다. 액터 0(m3noactor와 같은 경로)
	bool bNoSimLod = false;      // -LabSimLod=0: Simulation LOD Trait을 붙이지 않는다. 로직은 매 프레임, 이동은 전 개체가 엔진 이동·조향(M3 단계와 같다)
	bool bNoIskm = false;        // -LabIskm=0: M5 단계에서도 30~200m를 VAT ISM으로 그린다(m4offpool의 표현)
	bool bNoFixedBounds = false; // -LabFixedBounds=0: ISKM 컴포넌트 고정 바운드를 넣지 않는다(m5iskm의 바운드)
	bool bNoFarCull = false;     // -LabFarCull=0: 표현 LOD의 Off(그리지 않음)를 두지 않고 맵 끝까지 그린다. Simulation LOD Off 거리와 배경 무리 안쪽 반경은 그대로
	/**
	 * Q4 근거리 품질(FLabQuality). 기본은 변형이 정한다(대부분 500·30m, m3hyd15·m3hyd50은 15·50m, q4final은 Q4에서 고른 값). 인자가 이긴다.
	 * -LabActorCap=<개수>: 액터 상한(두 종류 합계). -LabHydrateM=<미터>: 화면 안에서 액터로 바뀌는 카메라 거리(1~79m). M3 이상 단계에서만 뜻이 있고 m3lowres는 자기 값을 쓴다.
	 */
	int32 ActorCap = FLabQuality::DefaultActorCap;
	float HydrateM = FLabQuality::DefaultHydrateM;
	float HydrateCm() const { return HydrateM * 100.f; }
	/**
	 * Q4b 원거리 그림자(-LabFarShadow=0|1). 거짓이면 표현 LOD Low(80m~그리는 끝) 인스턴스가 그림자를 그리지 않는다. M3 이상 단계에서만 뜻이 있다.
	 * 기본: q4final 거짓(끔), 그 밖 참(지금까지의 동작). 인자가 이긴다. 인스턴스 단위 그림자 플래그가 없어서 메시 설명자를 LOD 중요도 범위로 둘로 나눈다
	 * (High~Low 그림자 켬, Low~Max 그림자 끔, 엔진이 설명자마다 컴포넌트를 하나씩 만든다). ISKM과 VAT ISM(-LabIskm=0) 모두 같다. CSV 메타데이터 LabFarShadow.
	 */
	bool bFarShadow = true;
	/** CSV 메타데이터 LabFarShadow: "1" 켬, "0" 끔, M3 미만 단계 "-" */
	const TCHAR* FarShadowString() const { return Stage < ELabStage::M3 ? TEXT("-") : bFarShadow ? TEXT("1") : TEXT("0"); }
	/**
	 * 하이드레이션 액터(M3 이상, ALabMinionMassActor 계열)의 캡슐 콜리전. 기본은 끔이다(plan.md M3b "함께 바꾸는 설정", Q5 1b단위에서 코드에 넣었다).
	 * Off: NoCollision + 오버랩·물리 볼륨 갱신 끔(옛 m3nocol 처리). QueryOnly: 같은 두 갱신 끔 + QueryOnly(m3colquery). On: ACharacter 캡슐의 Pawn 프로필 그대로
	 * (Mass 액터 풀이 꺼낼 때 SetActorEnableCollision(true)를 부른다, MassActorSpawnerSubsystem.cpp:327).
	 * 변형 기본값: m3colquery는 QueryOnly, 라벨 m3colbase는 On(E3 (6)의 "그대로" 기준점의 뜻을 지킨다), 그 밖은 Off. -LabActorCollision=1이 켜고 =0이 끈다(인자가 이긴다).
	 * 판정(범위 피해, 저격)은 엔티티 위치의 수평 거리로 하고 캡슐을 쓰지 않는다. M0 액터(ALabMinion, CharacterMovement가 캡슐로 바닥을 찾는다)는 이 값과 무관하게 늘 켬이다.
	 */
	enum class EActorCollision : uint8 { Off, QueryOnly, On };
	EActorCollision ActorCollision = EActorCollision::Off;
	/** 하이드레이션 액터 캡슐 콜리전의 실제 적용값. "0" 끔(기본), "1" 켬(-LabActorCollision=1, m3colbase), "query" m3colquery. 액터가 없는 단계는 "-". CSV 메타데이터 LabActorCollision */
	const TCHAR* ActorCollisionString() const;
	/**
	 * 변형이 정한 기본값과 다르게 준 인자(Q5). "hydrate_m=70|actor_collision=1" 꼴(CSV 메타데이터 행이 쉼표로 나뉘어 | 로 잇는다), 없으면 빈 문자열. 사다리 단계를 그대로 고른 실행인지 가리는 데 쓴다.
	 * CSV 메타데이터 LabOverrides(빈 값이면 "-"), HUD 둘째 줄.
	 */
	FString Overrides;
	/**
	 * 사다리(plan.md "품질 계획") 단계. Id는 "1"~"8", 하위 변형 "1b"(M0 budget)·"1c"(M0 uro), 사다리 밖이면 "-". Name은 HUD에 쓰는 영어 이름.
	 * 변형과 단계만 본다. 인자로 바꾼 것은 Overrides에 따로 있다. CSV 메타데이터 LabLadder.
	 */
	struct FLadder { FString Id; FString Name; };
	FLadder Ladder() const;
	/** M3 이상 단계에서 0~30m를 액터로 바꾸는가(하이드레이션). m3noactor와 -LabHydrate=0은 거짓 */
	bool UsesHydration() const;
	/** 표현 LOD의 Off(그리는 끝 밖을 그리지 않음)가 있는가. M3 이상 단계와 M2의 vatcull·vatcullvis에서 참, -LabFarCull=0이면 거짓 */
	bool CullsFar() const;
	/**
	 * 표현 LOD의 Off 거리(cm). 지우기를 끄면 맵 반경의 두 배(1,000m)라 맵 안의 개체(카메라에서 최대 515m)는 Off에 들지 않는다.
	 * 1e7로 두지 않는 것은 풀 엔티티를 세워 두는 자리(약 14km, LabCrowdSubsystem.cpp LabPoolParkLocation)가 Off로 남아야 그리지 않기 때문이다.
	 */
	float RepOffCm() const { return bNoFarCull ? FLabScenario::MapRadius * 2.f : FarCm(); }
	/** 그리는 범위의 반경(cm, 오브 기준). 고정 바운드 크기에 쓴다. 지우기를 끄면 맵 반경 */
	float DrawExtentCm() const;
	/** Animation Budget Allocator로 도는 액터 템플릿을 쓰는가(M0 budget, M3 이상의 m3budgetdist·m3budgetsig·화면 크기 Budget) */
	bool UsesBudgetActors() const;
	/**
	 * 빼 보기 아홉 기법의 켬·끔(설정 기준). 메타데이터 키(LabTech*)와 로그 키, 값("1"/"0", 전환 대책 일부만 켜면 "m<마스크>", M4 틱 간격만이면 "tick").
	 * 기법끼리 딸린 효과(하이드레이션을 끄면 예산을 받을 액터가 없다, ISKM을 끄면 고정 바운드를 넣을 컴포넌트가 없다)는 여기서 지우지 않고
	 * 실제 적용값 메타데이터(LabBudget, LabIskmBounds)로 본다. 그래서 빼 보기 한 행은 아홉 값 중 하나만 0이다.
	 */
	struct FTech { const TCHAR* MetaKey; const TCHAR* LogKey; FString Value; };
	TArray<FTech> TechList() const;
	/** "tech=hyd:1,bud:1,..." 꼴(lab_config 로그 끝) */
	FString TechString() const;
	/** M5 단계의 ISKM 변형(m5iskm·m5destroy·m5blend·m5minss·m5bounds, 기본 변형)이면 참. m4offpool·m4offmove는 M5 단계에서도 VAT다. -LabIskm=0이면 거짓 */
	bool UsesIskm() const;
	FLabIskmParams IskmParams() const;
	enum EHydrateFix : int32
	{
		FixStartTime = 1,  // 시작 시각 맞추기(AnimBP Start Position)
		FixWait = 2,       // 준비될 때까지 VAT 유지 + 되돌아갈 때 한 프레임 더 + 그동안 액터 숨김
		FixPassHide = 4,   // 숨김을 액터 숨김 대신 메인·깊이 패스·그림자에서만 빼기(FixWait일 때만 뜻이 있다)
		FixLift = 8,       // 엔진 이동(TeleportActor)에 캡슐 반높이 더하기
		FixTintReset = 16, // 풀에 넣을 때 진홍색 지우기
	};
	bool HasFix(EHydrateFix F) const { return (HydrateFixMask & F) != 0; }

	/** M4부터 참(-LabSimLod=0이면 거짓). Simulation LOD Trait을 붙이고 로직 프로세서 둘(접근·범위 피해)이 청크 필터를 건다 */
	bool UsesSimLod() const { return Stage >= ELabStage::M4 && !bNoSimLod; }
	/** 변형에서 정한 M4 틱 간격과 회피 여부(UsesSimLod일 때만 뜻이 있다) */
	FLabSimLod SimLod() const;
	/** 로그와 CSV 메타데이터용. "80/200m tick=0/0.5/1.0 avoid=0" 꼴, M3까지는 "-" */
	FString SimLodString() const;

	static const FLabConfig& Get();
	static const TCHAR* VariantName(ELabVariant V);
	static const TCHAR* StageName(ELabStage S);
	FString ToLogString() const;
};
