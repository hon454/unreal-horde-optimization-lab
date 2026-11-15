#include "HordeOptLab.h"
#include "LabConfig.h"
#include "LabVatProcessor.h"
#include "LabActorSyncProcessor.h"
#include "LabMassProcessors.h"
#include "LabIskmProcessor.h"
#include "LabCrowdFxProcessor.h"
#include "MassUpdateISMProcessor.h"
#include "MassNavigationProcessors.h"
#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"

/**
 * 사다리 단계에 따라 프로세서 등록을 켜고 끈다(-LabStage=1~8, FLabConfig).
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
		// 표현 프로세서(사다리 단계별 스위치 표, Docs/Worklog/V2.md "W10"): 단계 2·3은 엔진 UMassUpdateISMProcessor(정지 메시), 4·5는 ULabVatISMProcessor,
		// 6부터는 ULabIskmProcessor 하나(plan.md "M5" 결정 2. ISM 컴포넌트가 없으니 엔진 ISM 갱신도 끈 채로 둔다). 단계 1에는 엔티티가 없다.
		// 엔진 ISKM 갱신(UMassUpdateInstancedSkinnedMeshProcessor)은 Config/DefaultMass.ini에서 꺼 둔 채다.
		const bool bVat = FLabConfig::Get().UsesVat();
		const bool bIskm = FLabConfig::Get().UsesIskm();
		LabSetAutoRegister(GetMutableDefault<UMassUpdateISMProcessor>(), !bVat);
		LabSetAutoRegister(GetMutableDefault<ULabVatISMProcessor>(), bVat && !bIskm);
		LabSetAutoRegister(GetMutableDefault<ULabIskmProcessor>(), bIskm);
		LabSetAutoRegister(GetMutableDefault<ULabActorSyncProcessor>(), FLabConfig::Get().IsMass()); // 하이드레이션 액터 동기화(단계 2부터)
		// Simulation LOD(단계 5부터): 새 엔티티(재생성 포함)는 첫 프레임에 Off 태그다(MassLODTrait.cpp:128-133). 그 프레임에 UMassOffLODNavigationProcessor가
		// 위치를 MoveTarget.Center(오브)로 옮기므로(MassNavigationProcessors.cpp:35-37,61) 이 단계부터 등록하지 않는다. 그 앞 단계는 Off 태그가 없어 대상이 없다.
		// 200m 밖(Off 밴드)은 ULabOffMoveProcessor가 옮긴다.
		const bool bSimLod = FLabConfig::Get().UsesSimLod();
		LabSetAutoRegister(GetMutableDefault<UMassOffLODNavigationProcessor>(), !bSimLod);
		const bool bOffMove = bSimLod;
		LabSetAutoRegister(GetMutableDefault<ULabOffMoveProcessor>(), bOffMove);
		// Q2b: 배경 무리의 엔티티 구동 입자(단계 7부터, -LabCrowdFx=0이 끈다). 그 밖에는 등록하지 않아 프로세서 구성이 그대로다.
		const bool bCrowdFxEntity = FLabConfig::Get().UsesCrowdFxEntity();
		LabSetAutoRegister(GetMutableDefault<ULabCrowdFxGatherProcessor>(), bCrowdFxEntity);
		// v2 W4: 겹침 방지 직접 구현(W5부터 기본 켬, -LabSeparate=0이 끈다). 끈 실행은 프로세서 구성이 W4 전과 같다
		const bool bSeparate = FLabConfig::Get().UsesSeparation();
		LabSetAutoRegister(GetMutableDefault<ULabSeparationProcessor>(), bSeparate);
		// v2 W7: 표현 전환의 최소 유지 시간(-LabRepHold=<ms>). 끈 실행은 프로세서 구성이 W7 전과 같다
		const bool bRepHold = FLabConfig::Get().UsesRepHold();
		LabSetAutoRegister(GetMutableDefault<ULabRepHoldProcessor>(), bRepHold);
		// v2 W11: QueryOnly 바디(하이드레이션 액터 캡슐 Trace)를 가속 구조의 따로 된 칸에 넣는다(-LabQueryIsolate, engine-notes.md 38·44절).
		// 칸은 물리 씬의 가속 구조를 만들 때 생기므로(PBDRigidsEvolution.cpp CreateEmptyCollection) 첫 월드를 만들기 전인 여기서 넣는다. 끈 실행은 CVar를 건드리지 않는다
		if (FLabConfig::Get().bQueryIsolate)
		{
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("p.Chaos.AccelerationStructureIsolateQueryOnlyObjects")))
			{
				Var->Set(1, ECVF_SetByCode);
			}
			else
			{
				UE_LOG(LogLab, Warning, TEXT("lab_cvar_missing p.Chaos.AccelerationStructureIsolateQueryOnlyObjects"));
			}
		}
		// crowdfx_entity·separate·rep_hold는 켰을 때만 붙인다(다른 실행의 로그는 그대로다)
		UE_LOG(LogLab, Log, TEXT("lab_ism_processor %s simlod=%d offmove=%d%s%s%s"), bIskm ? TEXT("iskm") : bVat ? TEXT("vat") : TEXT("engine"), bSimLod ? 1 : 0, bOffMove ? 1 : 0,
			bCrowdFxEntity ? TEXT(" crowdfx_entity=1") : TEXT(""), bSeparate ? TEXT(" separate=1") : TEXT(""), bRepHold ? TEXT(" rep_hold=1") : TEXT(""));
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FLabGameModule, HordeOptLab, "HordeOptLab");

DEFINE_LOG_CATEGORY(LogLab);

namespace
{
	int32 GLabPendingExitCode = -1;
	const TCHAR* GLabPendingExitSite = nullptr;
}

void LabRequestExit(uint8 ExitCode, const TCHAR* CallSite)
{
	if (GLabPendingExitCode < 0)
	{
		GLabPendingExitCode = ExitCode;
		GLabPendingExitSite = CallSite;
	}
}

bool LabConsumePendingExit(uint8& OutExitCode, const TCHAR*& OutCallSite)
{
	if (GLabPendingExitCode < 0)
	{
		return false;
	}
	OutExitCode = uint8(GLabPendingExitCode);
	OutCallSite = GLabPendingExitSite;
	GLabPendingExitCode = -1;
	return true;
}
