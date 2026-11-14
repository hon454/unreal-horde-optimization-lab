#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassObserverProcessor.h"
#include "LabConfig.h"
#include "LabOverlap.h"
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

/** 진단 변형 m3qold: 정비 전 쿼리 구조(표현이 로직 프래그먼트와 태그를 읽고, 저격이 쿼리 밖에서 쓰고, 표현에 로직 뒤 순서 선언이 없다). */
inline bool LabUseOldQueries()
{
	return FLabConfig::Get().Variant == ELabVariant::M3QOld;
}

/**
 * A/B 변형 m3order만 표현 프로세서(VatISM·ActorSync)에 ExecuteAfter(LabLogic)를 선언한다.
 * 기본은 선언 없음: 로직과 표현 사이에 선후가 없다. 로직이 FLabAnimStateFragment를 쓰고 표현이 읽는 겹침은 청크 단위 접근 충돌일 뿐 프로세서의 선후를 만들지 않는다
 * (MassProcessingQueueTypes.cpp:203-254, engine-notes.md 13절). 표현이 같은 프레임 로직보다 먼저 돌면 상태 변화가 한 프레임 늦게 보인다(Docs/Guides/code-tour.md "순서가 정해져 있지 않은 곳").
 * 20k P1에서 선언이 0.4~0.7ms 느렸다(m3noorder 15.52 vs m3mass 16.12ms, Docs/Worklog/M3b.md "코드 A/B").
 */
inline bool LabExplicitLogicOrder()
{
	return FLabConfig::Get().Variant == ELabVariant::M3Order;
}

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
 * E1 (d) 비치명 피해 발생기. 초당 정해진 건수를 무작위 개체에 넣는다. 개체마다 해시(일련번호, 프레임)로 뽑아 기대 건수를 맞춘다.
 * 직접 방식은 그 자리에서 체력을 깎고(최소 1), 이벤트 방식은 피해 이벤트 엔티티를 만든다. 두 방식 모두 같은 순회를 하므로 차이는 적용 방식뿐이다.
 */
UCLASS()
class ULabDamageGenProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabDamageGenProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
	double Owed = 0.0; // 아직 넣지 못한 건수(소수점 누적)
};

/** E1 (d) 이벤트 방식: 피해 이벤트 엔티티를 모아 대상 체력에 적용하고(최소 1) 이벤트를 지운다. */
UCLASS()
class ULabDamageApplyProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabDamageApplyProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EventQuery;
	FMassEntityQuery HealthQuery; // 미니언 체력에 쓴다는 것을 의존성 해석기에 알리려고 등록만 한다
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
 * 시험 실행 Spike1(Docs/Worklog/V2.md "시험 실행"): 표현 전환 수를 방향별로 센다. 오브와 카메라가 움직이면 높은 품질 → 낮은 품질 전환과 경계에서의 왕복이 생긴다.
 * 엔진 표현 프로세서가 이번 프레임에 갱신한 청크(FMassVisualizationChunkFragment::ShouldUpdateVisualization, MassRepresentationProcessor.cpp:529)에서
 * PrevRepresentation != CurrentRepresentation인 엔티티가 이번 프레임에 전환한 것이다(:118-121. 액터 준비 대기 중에는 Prev가 고정되므로 대기가 끝난 프레임에 한 번 센다).
 * CSV 스탯(프레임마다 그 프레임의 수): Lab/SwNoneToInst(없음·입자 → 인스턴스), Lab/SwInstToActor, Lab/SwActorToInst, Lab/SwInstToNone, Lab/SwOther(없음 ↔ 액터),
 * Lab/SwRound(같은 엔티티가 RoundTripSec 안에 같은 경계를 반대로 다시 넘은 수). 궤도 인자(FLabOrbit)를 하나라도 준 실행에서만 등록한다.
 */
UCLASS()
class ULabRepSwitchCountProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabRepSwitchCountProcessor();
	static constexpr double RoundTripSec = 2.;
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
	struct FLastSwitch { double Time = 0.; uint8 From = 0; uint8 To = 0; };
	TMap<FMassEntityHandle, FLastSwitch> LastSwitch; // 전환한 엔티티만 들어간다(게임 스레드 전용)
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
