#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassLODTypes.h" // UE::Mass::ProcessorGroupNames::LOD (정의는 MassLODSubsystem.cpp)
#include "LabConfig.h"
#include "LabMassProcessors.generated.h"

/**
 * M1 Mass 코어의 프로세서. M0 액터(ALabMinion, ALabOrb)의 이동·피해·사망 로직을 그대로 옮긴다. plan.md "M1".
 * 미니언 태그(FLabMinionTag)가 있는 엔티티만 다루므로 M0 실행에서는 대상이 없어 일하지 않는다.
 * 순서: 접근(Tasks 그룹, 엔진 조향 앞) → 엔진 조향·이동 → 그룹 LabLogic(범위 피해 → 저격 → 상태 타이머 → 피해 발생·적용) → 표현(VAT·ActorSync, LabLogic 뒤).
 * M3b 정비(Docs/Worklog/M3b.md "코드 정비", 2026-11-09 사용자 결정): 로직은 상태 전환 때 FLabAnimStateFragment를 쓰고 표현은 그것만 읽는다.
 * 엔진 조향은 Tasks 뒤에 돌아야 하므로 접근만 Tasks 그룹에 남긴다. 그룹 이름 의존성은 솔버가 자식 노드 전부로 펼친다(MassProcessorDependencySolver.cpp:680-719).
 * M4(Docs/Worklog/M4.md, 2026-11-10 사용자 결정): 접근·범위 피해·상태 셋은 M4부터 청크 필터 ShouldTickChunkThisFrame를 걸어 80m 밖 청크를 틱 간격마다만 돈다.
 * 범위 피해는 엔진 누적 델타(FMassSimulationVariableTickFragment::DeltaTime)로, 상태 타이머는 절대 시각(지금 − StateStartTime)으로 건너뛴 시간을 보상한다.
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
 * A/B 변형 m3order만 표현 프로세서(VatISM·ActorSync)에 ExecuteAfter(LabLogic)를 선언한다(결정 4, 2026-11-09).
 * 기본은 선언 없음: 솔버가 프래그먼트 접근(FLabAnimStateFragment 쓰기 → 읽기)으로만 순서를 정한다.
 * 20k P1에서 선언이 0.4~0.7ms 느렸다(m3noorder 15.52 vs m3mass 16.12ms, Docs/Worklog/M3b.md "코드 A/B(D안)").
 */
inline bool LabExplicitLogicOrder()
{
	return FLabConfig::Get().Variant == ELabVariant::M3Order;
}

/**
 * M4부터 이 프로세서를 엔진 LOD 그룹(표현 LOD, Simulation LOD) 뒤에 돌린다. M3까지는 아무것도 더하지 않는다.
 * 가변 틱 청크 필터를 쓰는 프로세서는 그 값을 쓰는 UMassSimulationLODProcessor(LOD 그룹) 뒤에 돌아야 한다.
 * 순서를 적지 않으면 두 프로세서의 순서가 프레임마다 바뀔 수 있고, "이 프레임에 돈다"는 값은 LOD 프로세서의 한 번과 다음 번 사이에만 참이라
 * 그 사이에 읽지 못하면 한 주기를 통째로 건너뛴다(2026-11-10 lab_chunk 진단 로그, Docs/Worklog/M4.md "사망 전환 지연").
 * 엔진에서 같은 필터를 쓰는 프로세서도 이렇게 적는다(MassStateTreeProcessors.cpp:165, MassAvoidanceProcessors.cpp:288).
 * 실행 큐는 이름을 ExecuteInGroup과 정확히 같은 그룹으로만 푼다(MassProcessingQueue.cpp:689-692,714-731). 두 LOD 프로세서의 그룹이 정확히 LOD다.
 */
inline void LabRunAfterSimLod(FMassProcessorExecutionOrder& Order)
{
	if (FLabConfig::Get().UsesSimLod())
	{
		Order.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::LOD);
	}
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
