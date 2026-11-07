#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "LabMassTypes.generated.h"

/**
 * M1 Mass 코어의 Fragment와 Tag. plan.md "M1".
 * 위치·속도·이동 목표는 엔진 Fragment(FTransformFragment, FMassVelocityFragment, FMassMoveTargetFragment)를 쓴다.
 */

/** 오브 저격 대상 선택(결정적 해시)에 쓰는 스폰 일련번호와, 피격·사망 상태에 남은 시간. */
USTRUCT()
struct FLabMinionFragment : public FMassFragment
{
	GENERATED_BODY()

	uint32 Serial = 0;
	float StateTimeLeft = 0.f;
};

USTRUCT()
struct FLabHealthFragment : public FMassFragment
{
	GENERATED_BODY()

	float Health = 0.f;
};

/** 개체 종류마다 하나씩 공유하는 값. M3b에서 대형이 들어오면 종류별로 갈린다. */
USTRUCT()
struct FLabUnitTypeParams : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY() float MoveSpeed = 0.f;
	UPROPERTY() float MaxHealth = 0.f;
	UPROPERTY() float BoundRadius = 0.f;
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
