#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassObserverProcessor.h"
#include "LabConfig.h"
#include "LabOverlap.h"
#include "LabSeparation.h"
#include "MassCommonFragments.h"   // FTransformFragment
#include "MassMovementFragments.h" // FMassDesiredMovementFragment
#include "MassRepresentationFragments.h" // FMassRepresentationLODFragment, EMassRepresentationType(v2 W7 전환 프로세서)
#include "LabMassProcessors.generated.h"

/**
 * M1 Mass 코어의 프로세서. M0 액터(ALabMinion, ALabOrb)의 이동·피해·사망 로직을 그대로 옮긴다. plan.md "M1".
 * 미니언 태그(FLabMinionTag)가 있는 엔티티만 다루므로 M0 실행에서는 대상이 없어 일하지 않는다.
 * 순서: 접근(Tasks 그룹, 엔진 조향 앞) → 엔진 조향·이동 → 그룹 LabLogic(범위 피해 → 저격 → 상태 타이머 → 피해 발생·적용) → 표현(VAT·ActorSync. 순서 선언은 변형 m3order에서만 하고 기본은 솔버가 정한다).
 * M3b 정비(Docs/Worklog/M3b.md "코드 정비"): 로직은 상태 전환 때 FLabAnimStateFragment를 쓰고 표현은 그것만 읽는다.
 * 엔진 조향은 Tasks 뒤에 돌아야 하므로 접근만 Tasks 그룹에 남긴다. 그룹 이름 의존성은 솔버가 자식 노드 전부로 펼친다(MassProcessorDependencySolver.cpp:680-719).
 * M4(Docs/Worklog/M4.md): 접근·범위 피해 둘은 M4부터 청크 필터 ShouldTickChunkThisFrame를 걸어 80m 밖 청크를 틱 간격마다만 돈다.
 * 범위 피해는 엔진 누적 델타(FMassSimulationVariableTickFragment::DeltaTime)로 건너뛴 시간을 보상한다. 상태 타이머는 필터 없이 매 프레임 돈다(LabMassProcessors.cpp의 상태 프로세서 주석).
 * 저격은 오브의 이벤트성 질의라 필터를 걸지 않는다. M3까지의 쿼리와 계산은 그대로다.
 */

/** Lab 로직 프로세서 그룹. 표현 프로세서가 ExecuteAfter로 가리킨다. */
inline const FName LabLogicGroupName(TEXT("LabLogic"));

/*
 * 표현 프로세서(VatISM·ISKM·ActorSync)는 LabLogic 뒤라는 선언을 하지 않는다. 로직과 표현 사이에 선후가 없다. 로직이 FLabAnimStateFragment를 쓰고 표현이 읽는 겹침은
 * 청크 단위 접근 충돌일 뿐 프로세서의 선후를 만들지 않는다(MassProcessingQueueTypes.cpp:203-254, engine-notes.md 13절). 표현이 같은 프레임 로직보다 먼저 돌면
 * 상태 변화가 한 프레임 늦게 보인다(Docs/Guides/code-tour.md "순서가 정해져 있지 않은 곳"). 20k P1에서 선언이 0.4~0.7ms 느렸다(Docs/Worklog/M3b.md "코드 A/B").
 * 선언을 넣은 A/B 변형(m3order)과 정비 전 쿼리 구조 변형(m3qold)은 v2 W10에서 지웠다.
 */

/** 살아 있는 개체의 이동 목표를 오브로 둔다. 실제 이동은 엔진 조향(UMassSteerToMoveTargetProcessor)과 이동 프로세서가 한다. */
UCLASS()
class ULabApproachProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabApproachProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
	/**
	 * M4: 청크 필터 없는 같은 쿼리. 매 프레임 청크의 개체 수만 읽어 LOD별 걷는 개체 수를 CSV 스탯 Lab/SimBand*로 남긴다(돈 개체 수는 Lab/SimTick*).
	 * -LabSimLodLog를 주면 "이 프레임에 돈 개체 ÷ 전체"를 1초마다 로그에도 남긴다(Development). M3까지는 순회하지 않는다.
	 */
	FMassEntityQuery CountQuery;
	int64 DiagTicked[5] = {};
	int64 DiagTotal[5] = {};
	int64 DiagTickedChunks[5] = {};
	int64 DiagTotalChunks[5] = {};
	int32 DiagFrames = 0;
	double DiagSince = 0.;
};

/**
 * E4 (d) m4offmove·m4offpool과 M5 단계에서 등록한다(FLabSimLod::bOffMove, HordeOptLab.cpp). Simulation LOD가 Off인 걷는 개체(카메라에서 200m 밖)를 엔진 이동·조향 넷 대신 옮긴다.
 * 가변 틱이 온 프레임에만 돌고, 마지막으로 옮긴 뒤 흐른 시간(FLabOffMoveFragment) × 걷는 속도만큼 오브 쪽으로 곧게 옮긴다.
 * 엔진의 FMassSimulationVariableTickFragment(DeltaTime, LastTickedTime)는 쓰지 않는다. 청크가 돈 다음 갱신에서 엔진이 값을 덮어써 흐른 시간의 약 6%가 빠졌다(Docs/Worklog/M4.md "E4 (d) Low 밴드 이동"). 회전은 엔진의 Off 쿼리가 맞춘다(MassSmoothOrientationProcessors.cpp:36-40,120-133).
 * 200m 안으로 돌아오면 엔진 조향이 이어받는다. 그때 속도가 0에서 다시 붙지 않게 DesiredVelocity를 방향 × 속도로 적어 둔다.
 * 피격·사망 중인 개체는 쿼리 밖이라 움직이지 않는다.
 */
UCLASS()
class ULabOffMoveProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabOffMoveProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

/**
 * E4 (d) m4offmove·m4offpool과 M5 단계에서 등록한다. 개체가 Off 밴드에 들어올 때(FMassOffLODTag가 붙을 때) 마지막 이동 시각을 지금으로 맞춘다.
 * 그 프레임까지는 엔진 이동 프로세서가 옮겼으므로 ULabOffMoveProcessor는 그 뒤의 시간만 옮긴다. 엔진의 스프링 이동도 Off 태그를 이렇게 본다(MassSpringMovementProcessors.cpp:124-128).
 */
UCLASS()
class ULabOffMoveEnterObserver : public UMassObserverProcessor
{
	GENERATED_BODY()
public:
	ULabOffMoveEnterObserver();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

/** 오브 피해 범위. 중심에 가까울수록 센 초당 피해를 주고, 체력이 다하면 사망 상태로 바꾼다. 테슬라 코일 번개 대상(가장 가까운 개체)도 찾는다. */
UCLASS()
class ULabOrbContactProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabOrbContactProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

/** 오브 저격. 오브가 저격 틱을 올린 처리에서, 화면 안의 산 개체를 거리 구간별로 골라 피격 상태로 바꾼다(M0과 같은 규칙, LabSnipe.h). */
UCLASS()
class ULabOrbSnipeProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabOrbSnipeProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

/**
 * Q5 동등성 통계(Docs/Worklog/Q5.md): 오브 중심에서 수평 30m(FLabEquivalence::NearOrbCm) 안의 미니언 엔티티 수를 매 프레임 센다. 걷기·피격·사망 모두 세고 풀에서 기다리는 엔티티는 뺀다.
 * M0 액터 단계는 게임 모드가 같은 정의로 액터를 센다. 청크 필터를 걸지 않아 Simulation LOD 틱 간격과 무관하게 전 개체를 본다(읽기만 한다).
 */
UCLASS()
class ULabNearOrbCountProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabNearOrbCountProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

/**
 * 표현 전환 수(시험 실행 Spike1에서 시작, v2 W7에서 본 구현으로 정리, Docs/Worklog/V2.md "W7"). 경계별·방향별로 센다. 모든 단계에서 등록한다(대상이 없으면 일하지 않는다).
 * 엔진 표현 프로세서가 이번 프레임에 갱신한 청크(FMassVisualizationChunkFragment::ShouldUpdateVisualization, MassRepresentationProcessor.cpp:529)에서
 * PrevRepresentation != CurrentRepresentation인 엔티티가 이번 프레임에 전환한 것이다(:118-121. 액터 준비 대기 중에는 Prev가 고정되므로 대기가 끝난 프레임에 한 번 센다).
 * 경계: 액터 ↔ 인스턴스(전환 거리, 단계 2~8 35m, ADR-0021), 인스턴스 ↔ 없음(200m, 최종 구성에서 없음은 입자), 인스턴스끼리 그림자 켬 ↔ 끔(표현 LOD Medium ↔ Low, 80m, Q4b).
 * CSV 스탯(프레임마다 그 프레임의 수): Lab/SwNoneToInst(없음·입자 → 인스턴스), Lab/SwInstToActor, Lab/SwActorToInst, Lab/SwInstToNone, Lab/SwOther(없음 ↔ 액터),
 * Lab/SwShadowOff(Medium → Low), Lab/SwShadowOn(Low → Medium),
 * Lab/SwRound(표현 무리의 왕복: 같은 엔티티가 RoundTripSec 안에 같은 경계를 반대로 다시 넘은 수. Spike1부터의 뜻 그대로), 그 가운데 Lab/SwRoundActor(액터 경계), Lab/SwRoundFar(200m 경계),
 * 그리고 따로 Lab/SwRoundShadow(80m 경계의 왕복), Lab/RepCapOn(이번 프레임에 액터 상한이 거리를 당겼으면 1, FMassVisualizationLODSharedFragment::bHasAdjustedDistancesFromCount).
 * 전환 기록은 ULabCrowdSubsystem::RepLastSwitch에 쓰고 ULabRepHoldProcessor가 읽는다.
 */
UCLASS()
class ULabRepSwitchCountProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabRepSwitchCountProcessor();
	static constexpr double RoundTripSec = 2.;
	/** 표현 무리: 0 액터, 1 인스턴스(ISKM·VAT ISM), 2 없음(그리는 끝 밖, 최종 구성에서는 입자) */
	static uint8 ClassOf(EMassRepresentationType Type);
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
	struct FShadowSwitch { double Time = 0.; bool bOff = false; };
	TMap<FMassEntityHandle, FShadowSwitch> ShadowLast; // 80m 경계를 넘은 엔티티만 들어간다(게임 스레드 전용)
	double NextPruneTime = 0.;
#if !UE_BUILD_TEST
	// 개발 확인 로그(lab_repsw, 10초마다): 액터 경계 왕복을 원인별로 나눈다. 화면 안팎이 두 전환 사이에 바뀐 것(Vis), 아닌 것(Dist), 그중 상한이 걸린 프레임(Cap)
	double LogSince = 0.;
	int64 LogSw[8] = {};
	int64 LogRoundActorVis = 0, LogRoundActorDist = 0, LogRoundActorCap = 0, LogRoundFar = 0, LogRoundShadow = 0, LogCapFrames = 0, LogFrames = 0;
	int64 LogRoundDistHist[6] = {}; // 액터 경계 왕복의 카메라 거리(0~10, 10~20, 20~30, 30~40, 40~50, 50m~)
#endif
};

/**
 * v2 W7 표현 전환의 최소 유지 시간(-LabRepHold=<ms>, FLabConfig::UsesRepHold일 때만 등록, Docs/v2-spec.md 4.1, Docs/Worklog/V2.md "W7").
 * 자리: 그룹 LOD, 엔진 표현 LOD 프로세서(UMassVisualizationLODProcessor) 뒤. 표현 프로세서(Representation 그룹, LOD 그룹 뒤)가 이번 프레임의 LOD로 표현을 바꾸기 전에 LOD를 고친다.
 * 규칙: 엔진이 고른 LOD가 지금 표현과 다른 무리를 가리키고, 그 엔티티가 RepHoldMs 안에 반대 방향으로 바뀐 적이 있으면(ULabCrowdSubsystem::RepLastSwitch) LOD를 지난 프레임 값으로 되돌린다.
 * 액터 상한: 되돌려 남긴 액터 때문에 LOD High 엔티티가 상한(LODMaxCount[High])을 넘으면, 이번 프레임 새로 액터가 될 엔티티를 먼 순으로 막는다.
 * 그래도 넘으면 남긴 액터를 먼 순으로 놓아 준다. 먼 정도는 화면 안이면 화면 안 경계, 밖이면 화면 밖 경계로 나눈 거리다(엔진 상한의 거리 구간과 같은 정규화, MassLODCalculator.h:534-642).
 * 엔진 Representation LOD는 액터 경계(High)에 히스테리시스가 없고 상한에서 거리를 매 프레임 다시 당긴다(engine-notes.md 19절, 40절). 이 규칙이 그 자리의 왕복을 막는다.
 * CSV 스탯: Lab/RepHeld(이번 프레임 되돌린 수), Lab/RepCapBlocked(상한 때문에 막은 새 액터 수).
 */
UCLASS()
class ULabRepHoldProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabRepHoldProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
	struct FCandidate { FMassRepresentationLODFragment* LOD = nullptr; uint8 Restore = 0; float Key = 0.f; };
	TArray<FCandidate> Promotions;    // 이번 프레임 새로 액터가 될 엔티티(되돌리지 않은 것)
	TArray<FCandidate> HeldDemotions; // 되돌려 액터로 남긴 엔티티. Restore는 엔진이 고른 LOD
};

/**
 * M6 수정 B(-LabFarHitIskm, FLabConfig::UsesFarHitIskm일 때만 등록, Docs/Worklog/V2.md "M6 수정의 상세 계획안"). 그리는 끝(200m) 밖에서 피격·사망한 개체를 제거될 때까지 ISKM으로 그린다.
 * 자리: 그룹 Representation, 엔진 표현 LOD 프로세서(LOD 그룹)와 ULabRepHoldProcessor 뒤, 엔진 표현 프로세서(VisualizationProcessing) 앞.
 * 규칙: 피격·사망 태그가 있고 표현 LOD가 Off인 개체의 LOD를 Low로 올린다. LOD 중요도는 엔진 값(Off = 3.0)을 그대로 두어 [Off, Max) 범위의 설명자(그림자 끔)로 간다
 * (ULabCrowdSubsystem::BuildEntityConfig). 그 범위의 컴포넌트만 바운드 상자를 맵 전체로 넓힌다(ULabCrowdSubsystem::ApplyFixedIskmBounds).
 * Off 청크는 표현을 0.5초(± 10%)마다만 다시 보므로(engine-notes.md 21·45절) ISKM이 늦게 나올 수 있다. 그동안 모으기 프로세서가 그 개체를 입자로 남긴다(멈춘 걷기 자세).
 * 따라잡기: 200m 밖 걷는 개체는 약 1초마다 옮겨지고 입자는 그 사이를 외삽한다. 맞은 개체는 멈추므로 마지막 이동 뒤 맞기까지의 거리를 한 번 옮겨 입자가 그리던 자리에 둔다.
 * 비용: 병렬 처리 큐는 직접 프래그먼트 쓰기·읽기가 겹치는 두 쿼리를 아키타입과 무관하게 서로 막는다(mass.ForceInlineProcessorExecution 기본 참, MassProcessingQueueTypes.cpp:331-340).
 *   그래서 주 쿼리는 위치를 쓰지 않고, 따라잡기는 따로 된 쿼리가 따라잡을 개체가 있는 프레임에만 한다. 주 쿼리가 위치를 쓰면 게임 스레드가 매 프레임 겹침 방지(작업자 스레드, 위치 쓰기)를
 *   기다렸다(Test 1만 단계 8에서 프레임당 약 0.7ms, Docs/Worklog/V2.md "M6 코드 비용 줄이기").
 * CSV 스탯: Lab/FarHitTracked(올린 적이 있는 산 피격·사망 개체 수), Lab/FarHitWaiting(그중 아직 한 번도 그리지 않은 수).
 */
UCLASS()
class ULabFarHitIskmProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabFarHitIskmProcessor();
	/** 맞은 시각(피격 시작). 사망 상태면 사망 시작 − 피격 시간 */
	static float HitStartOf(const struct FLabAnimStateFragment& Anim, float HitDuration);
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;  // 피격·사망 개체. 위치를 쓰지 않는다(아래 비용 주석)
	FMassEntityQuery CatchUpQuery; // 따라잡기(위치·Off 이동 쓰기). CatchUps가 있는 프레임에만 돈다
	struct FCatchUp { FMassEntityHandle Entity; float HitStart = 0.f; };
	TArray<FCatchUp> CatchUps; // 이번 프레임 따라잡을 개체(게임 스레드 전용, 프레임마다 비운다)
#if !UE_BUILD_TEST
	double LogSince = 0.;
	int64 LogBumped = 0, LogWaiting = 0, LogTracked = 0, LogFirst = 0, LogCatchUps = 0;
	int32 LogFrames = 0;
	double LogDelaySum = 0., LogDelayMax = 0., LogCatchUpMax = 0.;
	// 비용 진단(개발 전용, Docs/Worklog/V2.md "M6 코드 비용 줄이기"): 실행 시간, 첫 청크까지의 시간(쿼리 자원 기다림), 청크·엔티티 수의 합
	uint64 LogExecCycles = 0, LogWaitCycles = 0;
	int64 LogChunks = 0, LogEnts = 0;
	double LogExecMaxMs = 0.;
#endif
};

/**
 * v2 W4 겹침 방지, 직접 구현(W5부터 기본 켬, -LabSeparate=0이 끈다, LabSeparation.h, Docs/Worklog/V2.md "W4"). 게임 모듈이 FLabConfig::UsesSeparation()일 때만 등록한다(HordeOptLab.cpp).
 * 대상은 겹친 쌍의 수(ULabOverlapCountProcessor)와 같은 개체다: 살아 있는 미니언(걷기·피격), 사망 중인 개체와 풀 제외. Simulation LOD 밴드를 가리지 않고 매 프레임 돈다.
 * 자리: 그룹 Movement, 엔진 이동(UMassApplyMovementProcessor)과 Off 밴드 이동(ULabOffMoveProcessor) 뒤. 그 프레임에 옮겨진 위치를 고치고,
 * 범위 피해(LabLogic, Movement 뒤)와 표현이 고친 위치를 본다. 오브 피해 범위 안으로 들어가는 것은 막지 않는다.
 * 모으기와 쓰기는 한 스레드에서 차례로 하고, 이웃 찾기와 밀어낼 양의 계산은 ParallelFor로 나눈다(FLabSeparationSolver).
 * 쓰기는 모으기 때 적어 둔 프래그먼트 포인터로 한다. 한 Execute 안에서는 구조 변경이 없어 청크 메모리가 움직이지 않는다.
 */
UCLASS()
class ULabSeparationProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabSeparationProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
	FLabSeparationSolver Solver;
	TArray<FTransformFragment*> Transforms;
	TArray<FMassDesiredMovementFragment*> Movements; // 없는 개체는 nullptr
	// 개발 확인 로그(lab_sep)의 누계
	double LogSince = 0.;
	int64 LogPushed = 0;
	double LogPushSum = 0.;
	float LogPushMax = 0.f;
	int32 LogFrames = 0;
};

/** 피격·사망 상태의 남은 시간. 피격이 끝나면 사망, 사망이 끝나면 엔티티를 지우고 가장자리 링에서 새로 만든다. */
UCLASS()
class ULabMinionStateProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabMinionStateProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

/**
 * 겹친 쌍의 수(v2 W3, LabOverlap.h). LabOverlapSampleSec마다 살아 있는 미니언의 위치와 몸통 반지름(FAgentRadiusFragment)을 모아 세고
 * ULabCrowdSubsystem::OverlapPairs·OverlapDeep에 쓴다. 게임 모드가 CSV 스탯 Lab/OverlapPairs·Lab/OverlapDeep로 남긴다. 모든 Mass 단계에서 돈다.
 */
UCLASS()
class ULabOverlapCountProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabOverlapCountProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
	TArray<FLabBody> Bodies; // 표본마다 다시 채운다(할당을 되풀이하지 않는다)
	double NextSampleTime = 0.;
};
