#pragma once

#include "CoreMinimal.h"

/**
 * 표현별 색(개발 전용 -LabRepTint, CVar lab.RepTint, Docs/Worklog/V2-w14-plan.md 10절). 글의 구조 그림(거리별 표현) 정지 화면용이다. 측정에 쓰지 않는다.
 * 머티리얼 Vector 파라미터 UnitTint만 바꾼 MID를 쓴다. UnitTint는 산 개체의 기본 색에 곱하는 색이고 죽기 진홍색 lerp의 A 자리다(Scripts/bake-vat.py unit_tint_param,
 * Scripts/create-iskm.py). 그래서 사망 진홍은 색 위에 그대로 섞인다. 인스턴스 커스텀 데이터 0번(VAT 프레임, ISKM 사망 정도)과 액터의 커스텀 프리미티브 데이터 0번은 건드리지 않는다.
 * - 액터(하이드레이션): 메시 슬롯마다 MID를 넣는다(ALabMinionMassActor::BeginPlay). 단계 1 액터는 칠하지 않는다(소형은 Paragon 원본 머티리얼이라 UnitTint가 없다).
 * - VAT 인스턴스와 ISKM: 표현 설명자의 MaterialOverrides에 MID를 넣는다(ULabCrowdSubsystem::BuildEntityConfig). 엔진은 설명자마다 만드는 컴포넌트에 SetMaterial로 넣는다
 *   (MassVisualizationComponent.cpp:248-254 ISM, :1315-1321 ISKM). 설명자의 수·범위·그림자는 바꾸지 않아 컴포넌트 구성이 인자를 주지 않은 실행과 같다.
 * - 배경 무리 입자: 실행 중에 색을 바꿀 길이 없어 디버그용 사본 시스템(ini [Lab] CrowdFxTintSystem)을 읽는다(ALabGameMode::SetupCrowdFx, 결정 4). 사본은 에디터에서 만든다.
 * Test 빌드에는 이 코드가 없다(#if !UE_BUILD_TEST). 인자를 주지 않으면 아무것도 바꾸지 않는다.
 */
#if !UE_BUILD_TEST
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;

namespace LabRepTint
{
	/** 색을 받는 표현. 색은 V2-w14-plan.md 10.2절의 배정이다 */
	enum class ERep : uint8
	{
		Actor,        // 하이드레이션 액터: 빨강
		IskmShadow,   // ISKM 그림자 켬(35~80m, 단계 6·7은 35~200m 전체): 파랑
		IskmNoShadow, // ISKM 그림자 끔(80~200m, 단계 8): 하늘
		Vat,          // VAT 인스턴스(단계 4·5, ISKM을 끈 조건): 초록
		Crowd,        // 배경 무리 입자(200m 밖, 맵 밖 무리): 노랑. 코드는 쓰지 않고 사본 MI의 값이다(W14-5)
		FarHit,       // 그리는 끝 밖 피격·사망 ISKM: 자홍
	};

	/** 머티리얼 파라미터 이름. 에셋과의 약속이다 */
	inline constexpr const TCHAR* ParamName = TEXT("UnitTint");

	/** -LabRepTint를 줬는가(명령줄을 한 번 읽는다) */
	bool IsOn();
	FLinearColor Color(ERep Rep);
	const TCHAR* Name(ERep Rep);
	/** 모든 표현의 색을 "actor=(0.90,0.10,0.10) ..." 꼴로(로그) */
	FString ColorTable();
	/** Parent에 UnitTint가 있으면 UnitTint만 Color(Rep)로 바꾼 MID(Outer 밑)를 만든다. 없으면 nullptr */
	UMaterialInstanceDynamic* MakeMid(UMaterialInterface* Parent, ERep Rep, UObject* Outer);
	/** 컴포넌트의 슬롯마다 UnitTint가 있는 머티리얼을 MID로 바꾼다. 바꾼 슬롯 수를 돌려준다. 처음 보는 머티리얼만 로그를 남긴다 */
	int32 TintComponent(UMeshComponent& Component, ERep Rep);
}
#endif
