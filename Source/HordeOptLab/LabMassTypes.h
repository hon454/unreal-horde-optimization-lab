#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "MassEntityHandle.h"
#include "LabMinionAnimInstance.h" // ELabMinionState
#include "LabMassTypes.generated.h"

/**
 * M1 Mass 코어의 Fragment와 Tag. plan.md "M1".
 * 위치·속도·이동 목표는 엔진 Fragment(FTransformFragment, FMassVelocityFragment, FMassMoveTargetFragment)를 쓴다.
 * M3b 정비: 로직 데이터(FLabMinionFragment, FLabHealthFragment)는 로직 프로세서만 읽고 쓰고, 표현(VAT·ActorSync)은 FLabAnimStateFragment만 읽는다
 * (Docs/Worklog/M3b.md "코드 정비", 2026-11-09 사용자 결정 A안).
 */

/** 로직 전용. 오브 저격 대상 선택(결정적 해시)에 쓰는 스폰 일련번호와, 피격·사망 상태에 남은 시간. */
USTRUCT()
struct FLabMinionFragment : public FMassFragment
{
	GENERATED_BODY()

	uint32 Serial = 0;
	float StateTimeLeft = 0.f;
};

/** 걷기 위상(초). 모두 같은 프레임이면 군중이 한 몸처럼 발을 맞추므로 일련번호로 흩는다. VAT 인스턴스와 액터가 같은 식을 쓴다. */
inline float LabWalkPhase(uint32 Serial)
{
	return float(Serial % 997u) * 0.1031f;
}

/**
 * 표현 전용. 로직이 상태 전환 때만 쓰고 VAT·ActorSync가 읽는다. 표현은 태그(FLabHitTag·FLabDeadTag)를 보지 않고 이 값만 본다.
 * 재생 시각 = 월드 시각 − StateStartTime. Walk는 StateStartTime = −위상이라 재생 시각 = 월드 시각 + 위상(M2와 같다).
 */
USTRUCT()
struct FLabAnimStateFragment : public FMassFragment
{
	GENERATED_BODY()

	ELabMinionState State = ELabMinionState::Walk;
	float StateStartTime = 0.f;

	void Set(ELabMinionState InState, float Now) { State = InState; StateStartTime = Now; }
};

/**
 * E4 (d) 변형 m4offmove에서만 붙는다(LabMassTraits.cpp). ULabOffMoveProcessor가 이 개체를 마지막으로 옮긴 시각(월드 초).
 * 0이면 아직 옮긴 적이 없다. Off 태그가 붙을 때 ULabOffMoveEnterObserver가 그때 시각으로 맞춘다.
 */
USTRUCT()
struct FLabOffMoveFragment : public FMassFragment
{
	GENERATED_BODY()
	double LastMoveTime = 0.;
};

USTRUCT()
struct FLabHealthFragment : public FMassFragment
{
	GENERATED_BODY()

	float Health = 0.f;
};

/**
 * 개체 종류마다 하나씩 공유하는 값(const shared fragment). M3b 2종(-LabTypes=2)에서 소형·대형이 값이 달라 아키타입이 갈린다(LabUnitAssets.h).
 * 속도·체력은 같고 UnitType과 BoundRadius만 다르다. 프로세서는 청크마다 Context.GetConstSharedFragment<FLabUnitTypeParams>()로 종류를 안다(개체별 비용 없음).
 */
USTRUCT()
struct FLabUnitTypeParams : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY() int32 UnitType = 0; // ELabUnitType
	UPROPERTY() float MoveSpeed = 0.f;
	UPROPERTY() float MaxHealth = 0.f;
	UPROPERTY() float BoundRadius = 0.f;
};

/** E1 (d) 이벤트 방식의 피해 한 건. 1프레임만 살고 ULabDamageApplyProcessor가 대상에 적용한 뒤 지운다. */
USTRUCT()
struct FLabDamageEventFragment : public FMassFragment
{
	GENERATED_BODY()

	FMassEntityHandle Target;
	float Amount = 0.f;
};

/** 이 프로젝트의 미니언 엔티티. 프로세서가 엔진 엔티티와 섞이지 않게 거른다. */
USTRUCT()
struct FLabMinionTag : public FMassTag
{
	GENERATED_BODY()
};

/** 저격당해 피격 상태. StateTimeLeft가 다하면 사망으로 넘어간다(M0 액터의 피격 애니 길이와 같다). */
USTRUCT()
struct FLabHitTag : public FMassTag
{
	GENERATED_BODY()
};

/** 사망 상태. StateTimeLeft가 다하면 엔티티를 지우고 가장자리 링에서 새로 만든다(M0 액터의 사망 애니 길이와 같다). */
USTRUCT()
struct FLabDeadTag : public FMassTag
{
	GENERATED_BODY()
};

/**
 * M3 하이드레이션 액터가 붙어 있는 엔티티(M3b 정비 4번). ULabActorManagement::OnPostActorSpawn이 붙이고,
 * ULabActorSyncProcessor가 액터가 떼어진 것을 보면 뗀다. ActorSync는 이 태그가 있는 아키타입만 순회한다.
 */
USTRUCT()
struct FLabActorTag : public FMassTag
{
	GENERATED_BODY()
};

/**
 * M5 엔티티 풀링(-LabEntityPool=1, plan.md "M5" 결정 1). 사망이 끝나 풀에서 기다리는 엔티티. FLabDeadTag도 그대로 남아 로직 프로세서는 이미 거른다.
 * 상태 프로세서만 사망 태그 쿼리에 이 태그를 None으로 더 건다(풀링을 켰을 때만). 위치는 그리는 거리 밖(FLabPool::ParkLocation)에 두어 표현 LOD가 Off다.
 */
USTRUCT()
struct FLabPooledTag : public FMassTag
{
	GENERATED_BODY()
};

/**
 * M5 ISKM 변형에서만 붙는다(LabMassTraits.cpp). 이 엔티티의 ISKM 트랙에 마지막으로 SetAutoPlayData로 쓴 표현 상태(plan.md "M5" 결정 4).
 * ULabIskmProcessor는 FLabAnimStateFragment가 이 값과 다를 때만(상태 전환, 풀에서 다시 나옴) 트랙을 다시 쓴다. 평소 프레임에는 트랙을 건드리지 않는다.
 */
USTRUCT()
struct FLabIskmTrackFragment : public FMassFragment
{
	GENERATED_BODY()

	int8 AppliedState = -1; // ELabMinionState, -1이면 아직 쓴 적이 없다
	float AppliedStartTime = 0.f;
};
