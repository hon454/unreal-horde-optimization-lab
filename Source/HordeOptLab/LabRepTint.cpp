#include "LabRepTint.h"

#if !UE_BUILD_TEST
#include "HordeOptLab.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace LabRepTint
{
	bool IsOn()
	{
		static const bool bOn = FParse::Param(FCommandLine::Get(), TEXT("LabRepTint"));
		return bOn;
	}

	FLinearColor Color(ERep Rep)
	{
		// 선형 RGB(V2-w14-plan.md 10.2절). 액터 빨강과 ISM/ISKM 파랑은 posting.md "밴드 색 디버그 뷰"의 색과 같은 뜻이다
		switch (Rep)
		{
		case ERep::Actor:        return FLinearColor(0.9f, 0.1f, 0.1f);
		case ERep::IskmShadow:   return FLinearColor(0.1f, 0.3f, 0.9f);
		case ERep::IskmNoShadow: return FLinearColor(0.2f, 0.8f, 0.9f);
		case ERep::Vat:          return FLinearColor(0.1f, 0.8f, 0.2f);
		case ERep::Crowd:        return FLinearColor(0.95f, 0.8f, 0.1f);
		case ERep::FarHit:       return FLinearColor(0.9f, 0.2f, 0.8f);
		}
		return FLinearColor::White;
	}

	const TCHAR* Name(ERep Rep)
	{
		switch (Rep)
		{
		case ERep::Actor:        return TEXT("actor");
		case ERep::IskmShadow:   return TEXT("iskm_shadow");
		case ERep::IskmNoShadow: return TEXT("iskm_noshadow");
		case ERep::Vat:          return TEXT("vat");
		case ERep::Crowd:        return TEXT("crowd");
		case ERep::FarHit:       return TEXT("far_hit");
		}
		return TEXT("?");
	}

	FString ColorTable()
	{
		FString Out;
		for (ERep Rep : { ERep::Actor, ERep::IskmShadow, ERep::IskmNoShadow, ERep::Vat, ERep::Crowd, ERep::FarHit })
		{
			const FLinearColor C = Color(Rep);
			Out += FString::Printf(TEXT("%s%s=(%.2f,%.2f,%.2f)"), Out.IsEmpty() ? TEXT("") : TEXT(" "), Name(Rep), C.R, C.G, C.B);
		}
		return Out;
	}

	UMaterialInstanceDynamic* MakeMid(UMaterialInterface* Parent, ERep Rep, UObject* Outer)
	{
		// 파라미터가 있는지 먼저 본다. MID의 SetVectorParameterValue는 없는 이름에 조용하다(머티리얼이 그 값을 읽지 않는다)
		FLinearColor Default;
		if (!Parent || !Parent->GetVectorParameterValue(FHashedMaterialParameterInfo(ParamName), Default))
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Parent, Outer);
		Mid->SetVectorParameterValue(ParamName, Color(Rep));
		return Mid;
	}

	int32 TintComponent(UMeshComponent& Component, ERep Rep)
	{
		static TSet<FName> Logged; // 게임 스레드(액터 BeginPlay)에서만 부른다
		int32 Tinted = 0;
		const int32 Num = Component.GetNumMaterials();
		for (int32 Slot = 0; Slot < Num; ++Slot)
		{
			UMaterialInterface* Parent = Component.GetMaterial(Slot);
			UMaterialInstanceDynamic* Mid = MakeMid(Parent, Rep, &Component);
			if (Mid)
			{
				Component.SetMaterial(Slot, Mid);
				++Tinted;
			}
			const FName Key(FString::Printf(TEXT("%s/%d/%s"), Name(Rep), Slot, *GetNameSafe(Parent)));
			if (!Logged.Contains(Key))
			{
				Logged.Add(Key);
				FLinearColor Now = FLinearColor::Black;
				const bool bRead = Mid && Mid->GetVectorParameterValue(FHashedMaterialParameterInfo(ParamName), Now);
				UE_LOG(LogLab, Log, TEXT("lab_reptint rep=%s owner=%s slot=%d/%d parent=%s tinted=%d unit_tint=%s"), Name(Rep), *GetNameSafe(Component.GetOwner() ? Component.GetOwner()->GetClass() : nullptr),
					Slot, Num, *GetNameSafe(Parent), Mid ? 1 : 0, bRead ? *FString::Printf(TEXT("(%.2f,%.2f,%.2f)"), Now.R, Now.G, Now.B) : TEXT("-"));
			}
		}
		return Tinted;
	}
}
#endif
