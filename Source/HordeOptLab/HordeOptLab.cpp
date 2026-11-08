#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabVatProcessor.h"
#include "MassUpdateISMProcessor.h"
#include "Modules/ModuleManager.h"

/**
 * 단계에 따라 ISM 갱신 프로세서를 고른다(plan.md "M2", 2026-11-08 사용자 결정).
 * M1까지는 엔진 UMassUpdateISMProcessor 그대로, M2부터는 VAT 커스텀 데이터를 같이 쓰는 ULabVatISMProcessor만 돈다.
 * 자동 등록 여부(bAutoRegisterWithProcessingPhases)는 CDO 값이고, Mass가 월드를 시작하며 프로세서 목록을 만들기 전에 바꿔야 해서 게임 모듈 시작 때 한다.
 * 엔진의 SetShouldAutoRegisterWithGlobalList(MassProcessor.h:180-185)는 에디터 빌드에서 값을 Config/DefaultMass.ini에 써 버리므로
 * (MassProcessor.cpp:255-270 UpdateSinglePropertyInConfigFile) 리플렉션으로 CDO 값만 바꾼다. 패키지에서는 두 방법이 같다.
 */
namespace
{
	void LabSetAutoRegister(UMassProcessor* CDO, bool bAutoRegister)
	{
		const FBoolProperty* Prop = CastField<FBoolProperty>(UMassProcessor::StaticClass()->FindPropertyByName(TEXT("bAutoRegisterWithProcessingPhases")));
		check(Prop && CDO->HasAnyFlags(RF_ClassDefaultObject));
		Prop->SetPropertyValue_InContainer(CDO, bAutoRegister);
	}
}

class FLabGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		const bool bVat = FLabConfig::Get().Stage >= ELabStage::M2;
		LabSetAutoRegister(GetMutableDefault<UMassUpdateISMProcessor>(), !bVat);
		LabSetAutoRegister(GetMutableDefault<ULabVatISMProcessor>(), bVat);
		UE_LOG(LogLab, Log, TEXT("lab_ism_processor %s"), bVat ? TEXT("vat") : TEXT("engine"));
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FLabGameModule, HordeOptLab, "HordeOptLab");

DEFINE_LOG_CATEGORY(LogLab);
