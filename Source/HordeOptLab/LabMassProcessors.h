#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "LabMassProcessors.generated.h"

/**
 * M1 Mass 코어의 프로세서. M0 액터(ALabMinion, ALabOrb)의 이동·피해·사망 로직을 그대로 옮긴다. plan.md "M1".
 * 미니언 태그(FLabMinionTag)가 있는 엔티티만 다루므로 M0 실행에서는 대상이 없어 일하지 않는다.
 * 순서: 접근(Tasks 그룹, 엔진 조향 앞) → 엔진 조향·이동 → 범위 피해 → 저격 → 상태 타이머.
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
