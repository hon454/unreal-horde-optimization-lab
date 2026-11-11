#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabVatProcessor.h"
#include "LabActorSyncProcessor.h"
#include "LabMassProcessors.h"
#include "LabIskmProcessor.h"
#include "MassUpdateISMProcessor.h"
#include "MassNavigationProcessors.h"
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
		// M5 ISKM 변형(plan.md "M5" 결정 2): VAT 프로세서 자리에 ULabIskmProcessor 하나. ISM 컴포넌트가 없으니 엔진 ISM 갱신도 끈 채로 둔다.
		// m4offpool·m4offmove는 M5 단계에서도 VAT 그대로다. 엔진 ISKM 갱신(UMassUpdateInstancedSkinnedMeshProcessor)은 Config/DefaultMass.ini에서 꺼 둔 채다.
		const bool bIskm = FLabConfig::Get().UsesIskm();
		LabSetAutoRegister(GetMutableDefault<UMassUpdateISMProcessor>(), !bVat);
		LabSetAutoRegister(GetMutableDefault<ULabVatISMProcessor>(), bVat && !bIskm);
		LabSetAutoRegister(GetMutableDefault<ULabIskmProcessor>(), bIskm);
		LabSetAutoRegister(GetMutableDefault<ULabActorSyncProcessor>(), FLabConfig::Get().Stage >= ELabStage::M3); // M3 액터 동기화
		// M4: 새 엔티티(재생성 포함)는 첫 프레임에 Off 태그다(MassLODTrait.cpp:128-133). 그 프레임에 UMassOffLODNavigationProcessor가
		// 위치를 MoveTarget.Center(오브)로 옮기므로(MassNavigationProcessors.cpp:35-37,61) M4부터 등록하지 않는다. M3까지는 Off 태그가 없어 대상이 없었다.
		const bool bSimLod = FLabConfig::Get().UsesSimLod();
		LabSetAutoRegister(GetMutableDefault<UMassOffLODNavigationProcessor>(), !bSimLod);
		// E4 (d) bOffMove(m4offmove·m4offpool과 M5 단계): Off 밴드(200m 밖) 이동 프로세서. 다른 변형에서는 등록하지 않아 프로세서 구성이 그대로다.
		const bool bOffMove = bSimLod && FLabConfig::Get().SimLod().bOffMove;
		LabSetAutoRegister(GetMutableDefault<ULabOffMoveProcessor>(), bOffMove);
		UE_LOG(LogLab, Log, TEXT("lab_ism_processor %s simlod=%d offmove=%d"), bIskm ? TEXT("iskm") : bVat ? TEXT("vat") : TEXT("engine"), bSimLod ? 1 : 0, bOffMove ? 1 : 0);
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FLabGameModule, HordeOptLab, "HordeOptLab");

DEFINE_LOG_CATEGORY(LogLab);
