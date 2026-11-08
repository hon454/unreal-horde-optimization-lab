#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabVatProcessor.h"
#include "MassUpdateISMProcessor.h"
#include "Modules/ModuleManager.h"

/**
 * 단계에 따라 ISM 갱신 프로세서를 고른다(plan.md "M2", 2026-11-08 사용자 결정).
 * M1까지는 엔진 UMassUpdateISMProcessor 그대로, M2부터는 VAT 커스텀 데이터를 같이 쓰는 ULabVatISMProcessor만 돈다.
 * 자동 등록 여부는 CDO에서만 바꿀 수 있고(UMassProcessor::SetShouldAutoRegisterWithGlobalList, MassProcessor.h:180-185),
 * Mass가 월드를 시작하며 프로세서 목록을 만들기 전에 바꿔야 해서 게임 모듈 시작 때 한다.
 */
class FLabGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		const bool bVat = FLabConfig::Get().Stage >= ELabStage::M2;
		GetMutableDefault<UMassUpdateISMProcessor>()->SetShouldAutoRegisterWithGlobalList(!bVat);
		GetMutableDefault<ULabVatISMProcessor>()->SetShouldAutoRegisterWithGlobalList(bVat);
		UE_LOG(LogLab, Log, TEXT("lab_ism_processor %s"), bVat ? TEXT("vat") : TEXT("engine"));
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FLabGameModule, HordeOptLab, "HordeOptLab");

DEFINE_LOG_CATEGORY(LogLab);
