#pragma once

#include "CoreMinimal.h"

/**
 * 개체 종류(M3b 2종, plan.md "M3b" "바꾸는 것"). 0 소형(기본, 지금까지의 미니언), 1 대형.
 * 2종은 옵트인(-LabTypes=2)이고, 그때 일련번호가 10의 배수인 개체가 대형이다(초기 배치와 재생성 모두. 시드로 재현).
 * 기본(-LabTypes=1)은 전부 소형이라 1종 결과가 바뀌지 않는다.
 */
enum ELabUnitType : int32
{
	LabUnitSmall = 0,
	LabUnitLarge = 1,
	LabUnitTypeMax = 2,
};

/**
 * 종류별 에셋 경로와 바운드 반경. Config/DefaultGame.ini [LabUnitAssets]에서 읽는다(키 접두사 Small·Large, 한 번만).
 * 에셋 경로를 코드에 박지 않는 이유: 대형(시즈) 에셋은 에셋 세션이 옮기고 굽는다(Scripts/migrate-assets.py, bake-vat.py). 기대 경로는 ini와
 * Docs/Worklog/M3b.md "2종(코드)" 표에 있다. 소형 키가 없으면 지금까지 코드에 있던 경로를 쓴다(M0~M3와 같다).
 * VAT 메시·데이터 에셋은 모드(Bone·Vertex)가 이름 뒤에 붙으므로 기본 이름만 적고 VatMeshPath(Mode)로 만든다.
 */
struct FLabUnitAssetPaths
{
	FString SkeletalMesh;   // USkeletalMesh
	FString AnimClass;      // UAnimInstance 클래스(…_C). ULabMinionAnimInstance 자식이어야 한다
	FString Walk;           // UAnimSequence 셋(M0 액터, 피격·사망 길이)
	FString Hit;
	FString Death;
	FString StaticMesh;     // M0 static·M1 ISM 정적 메시(Scripts/make-static-minion.py)
	FString VatMesh;        // "/Game/Lab/VAT/Baked/SM_LabMinionVAT" 꼴. 실제 에셋은 SM_LabMinionVAT_Bone
	FString VatData;        // UAnimToTextureDataAsset, 같은 꼴
	FString ActorMaterial;  // 하이드레이션 액터 머티리얼(VAT 인스턴스와 같은 색). 비면 메시 기본 머티리얼
	float BoundRadius = 40.f; // FLabUnitTypeParams::BoundRadius. 대형은 에셋이 오기 전 임시값(ini LargeBoundRadius, 확정 전)

	static const FLabUnitAssetPaths& Get(int32 UnitType);
	FString VatMeshPath(const TCHAR* Mode) const { return WithMode(VatMesh, Mode); }
	FString VatDataPath(const TCHAR* Mode) const { return WithMode(VatData, Mode); }

private:
	static FString WithMode(const FString& Base, const TCHAR* Mode);
};

/** 2종 실행의 공유 상태. 스포너가 시작 때 대형 에셋을 확인하고 쓴다(게임 스레드). */
struct FLabUnitTypes
{
	/** -LabTypes 값(1 또는 2). */
	static int32 Num();
	static bool IsLarge(uint32 Serial) { return Num() == 2 && Serial % 10u == 0u; }
	static int32 TypeOf(uint32 Serial) { return IsLarge(Serial) ? LabUnitLarge : LabUnitSmall; }
	/**
	 * 대형 에셋이 하나라도 없어 소형 에셋으로 대체했다(ALabCrowdSpawner::LoadAssets가 정한다). 그 실행은 스모크 전용이고 측정하지 않는다(로그와 CSV 메타데이터 LabTypesAssets=substituted).
	 * 종류(아키타입, 저격 규칙, 액터 풀)는 그대로 둘로 갈리고 에셋만 같다.
	 */
	static bool bLargeSubstituted;
	/** 표현이 실제로 읽을 에셋의 종류. 대체 중이면 소형. */
	static int32 AssetType(int32 UnitType) { return bLargeSubstituted ? LabUnitSmall : UnitType; }
};
