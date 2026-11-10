#include "LabUnitAssets.h"
#include "LabConfig.h"
#include "Misc/ConfigCacheIni.h"

bool FLabUnitTypes::bLargeSubstituted = false;

int32 FLabUnitTypes::Num()
{
	return FLabConfig::Get().Types;
}

namespace
{
	const TCHAR* LabUnitAssetsSection = TEXT("LabUnitAssets");

	/** ini 키 <Prefix><Key>가 있으면 그 값, 없으면 Default. 빈 값은 "없음"으로 둔다(대형 키를 비워 두면 대체 경로로 간다). */
	FString LabReadPath(const TCHAR* Prefix, const TCHAR* Key, const TCHAR* Default)
	{
		FString Value;
		if (GConfig && GConfig->GetString(LabUnitAssetsSection, *FString::Printf(TEXT("%s%s"), Prefix, Key), Value, GGameIni))
		{
			return Value.TrimStartAndEnd();
		}
		return Default;
	}

	FLabUnitAssetPaths LabLoadPaths(int32 UnitType)
	{
		// 소형 기본값은 2종 전 코드가 쓰던 경로 그대로다(LabCrowdSpawner.cpp LoadAssets, LabMinion.cpp ALabMinionMassActor::BeginPlay, 커밋 7f0236f 기준).
		const bool bSmall = UnitType == LabUnitSmall;
		const TCHAR* Prefix = bSmall ? TEXT("Small") : TEXT("Large");
		const TCHAR* Root = TEXT("/Game/ParagonMinions/Characters/Minions/Down_Minions");
		FLabUnitAssetPaths P;
		P.SkeletalMesh = LabReadPath(Prefix, TEXT("SkeletalMesh"), bSmall ? *FString::Printf(TEXT("%s/Meshes/Minion_Lane_Melee_Dawn.Minion_Lane_Melee_Dawn"), Root) : TEXT(""));
		P.AnimClass = LabReadPath(Prefix, TEXT("AnimClass"), bSmall ? TEXT("/Game/Lab/Anim/ABP_LabMinion.ABP_LabMinion_C") : TEXT(""));
		P.Walk = LabReadPath(Prefix, TEXT("Walk"), bSmall ? *FString::Printf(TEXT("%s/Animations/Melee/NonCombat_JogFwd.NonCombat_JogFwd"), Root) : TEXT(""));
		P.Hit = LabReadPath(Prefix, TEXT("Hit"), bSmall ? *FString::Printf(TEXT("%s/Animations/Melee/HitReact_Front.HitReact_Front"), Root) : TEXT(""));
		P.Death = LabReadPath(Prefix, TEXT("Death"), bSmall ? *FString::Printf(TEXT("%s/Animations/Melee/Death_A.Death_A"), Root) : TEXT(""));
		P.StaticMesh = LabReadPath(Prefix, TEXT("StaticMesh"), bSmall ? TEXT("/Game/Lab/Derived/SM_LabMinion.SM_LabMinion") : TEXT(""));
		P.VatMesh = LabReadPath(Prefix, TEXT("VatMesh"), bSmall ? TEXT("/Game/Lab/VAT/Baked/SM_LabMinionVAT") : TEXT(""));
		P.VatData = LabReadPath(Prefix, TEXT("VatData"), bSmall ? TEXT("/Game/Lab/VAT/Baked/DA_LabMinionVAT") : TEXT(""));
		P.ActorMaterial = LabReadPath(Prefix, TEXT("ActorMaterial"), bSmall ? TEXT("/Game/Lab/VAT/Baked/M_LabMinionActor.M_LabMinionActor") : TEXT(""));
		P.BoundRadius = bSmall ? 40.f : 60.f; // 소형 40은 M1 트레이트의 값(LabMassTraits.cpp). 대형 60은 임시값(확정 전, 메시 바운드로 잰 뒤 ini에 적는다)
		if (GConfig)
		{
			GConfig->GetFloat(LabUnitAssetsSection, *FString::Printf(TEXT("%sBoundRadius"), Prefix), P.BoundRadius, GGameIni);
			if (!bSmall)
			{
				GConfig->GetFloat(TEXT("Lab"), TEXT("LargeScale"), P.Scale, GGameIni); // 소형은 늘 1
				P.Scale = FMath::Max(P.Scale, 0.1f);
			}
		}
		return P;
	}
}

const FLabUnitAssetPaths& FLabUnitAssetPaths::Get(int32 UnitType)
{
	static const FLabUnitAssetPaths Paths[LabUnitTypeMax] = { LabLoadPaths(LabUnitSmall), LabLoadPaths(LabUnitLarge) };
	return Paths[FMath::Clamp(UnitType, 0, LabUnitTypeMax - 1)];
}

FString FLabUnitAssetPaths::WithMode(const FString& Base, const TCHAR* Mode)
{
	if (Base.IsEmpty())
	{
		return FString();
	}
	// "/Game/Lab/VAT/Baked/SM_LabMinionVAT" + "Bone" → "/Game/Lab/VAT/Baked/SM_LabMinionVAT_Bone.SM_LabMinionVAT_Bone"
	FString Name;
	Base.Split(TEXT("/"), nullptr, &Name, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	return FString::Printf(TEXT("%s_%s.%s_%s"), *Base, Mode, *Name, Mode);
}
