#include "LabGameMode.h"
#include "HordeOptLab.h"
#include "LabCameraRig.h"
#include "LabConfig.h"
#include "LabCrowdManager.h"
#include "LabCrowdSpawner.h"
#include "LabCrowdSubsystem.h"
#include "LabUnitAssets.h"
#include "LabHud.h"
#include "InputCoreTypes.h"
#include "LabOrb.h"
#include "LabSnipe.h" // FLabSnipeBands(바닥 띠 반경)
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "IAnimationBudgetAllocator.h"
#include "SignificanceManager.h"
#include "RHI.h" // GRHIAdapterName, GRHIAdapterUserDriverVersion
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstance.h"           // 개발 빌드 입자 수 로그
#include "NiagaraSystemInstanceController.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraDataInterfaceArrayFloat.h"  // Q2b: 엔티티 배열 사용자 변수의 형 확인(UNiagaraDataInterfaceArrayFloat4)
#include "LabBuildVersion.h" // LAB_BUILD_COMMIT. HordeOptLab.Build.cs가 .git/HEAD에서 만든다

CSV_DECLARE_CATEGORY_EXTERN(Lab); // Q5 동등성 통계 Lab/Alive·Deaths·Near30(카테고리는 LabVatProcessor.cpp가 정의한다)

namespace
{
	/** 결정 6 바닥 띠가 켜져 있는가(개발 전용 -LabBandFloor가 1로 둔다). 측정 CSV 메타데이터 LabCVars에 찍혀 켜고 잰 실행을 걸러낸다. Test 빌드에서는 늘 0. */
	TAutoConsoleVariable<int32> CVarLabBandFloor(TEXT("lab.BandFloor"), 0, TEXT("1 if the dev-only -LabBandFloor orb-centred band floor is active (never in Test builds)"), ECVF_Default);

	void LabSetCVar(const TCHAR* Name, const TCHAR* Value)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			Var->Set(Value, ECVF_SetByCode);
		}
		else
		{
			UE_LOG(LogLab, Warning, TEXT("lab_cvar_missing %s"), Name);
		}
	}

	/**
	 * 측정에 고정하는 CVar의 실제 값(측정 방법 5번, plan.md "M3b"). 이름=값을 '|'로 잇는다(CSV 메타데이터 행은 쉼표로 나뉘므로 쉼표를 쓰지 않는다).
	 * 러너가 거는 렌더 고정값, 스케일러빌리티, 렌더 방식, Mass 병렬·옵저버 잠금이다. summarize.py가 `labcvars` 키로 읽어 build 열에 넣는다.
	 */
	FString LabFixedCVarString()
	{
		static const TCHAR* Names[] = {
			TEXT("r.VSync"), TEXT("t.MaxFPS"), TEXT("r.ScreenPercentage"), TEXT("r.DynamicRes.OperationMode"),
			TEXT("sg.ShadowQuality"), TEXT("sg.AntiAliasingQuality"), TEXT("sg.GlobalIlluminationQuality"), TEXT("sg.EffectsQuality"),
			TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.Shadow.Virtual.Enable"), TEXT("r.AntiAliasingMethod"),
			TEXT("mass.FullyParallel"), TEXT("mass.commands.LockObserversDuringFlushing"),
			TEXT("lab.BandFloor"), // 결정 6 바닥 띠(개발 전용). 1이면 그 실행은 비교에 쓰지 않는다
		};
		FString Out;
		for (const TCHAR* Name : Names)
		{
			if (const IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
			{
				if (!Out.IsEmpty()) { Out += TEXT("|"); }
				Out += FString::Printf(TEXT("%s=%s"), Name, *Var->GetString());
			}
		}
		return Out;
	}
}

bool ALabGameMode::IsBudgetActive() const
{
	// 할당기는 CVar a.Budget.Enabled == 1(bCachedEnabled)과 월드 할당기의 bEnabled가 둘 다 참일 때만 돈다(AnimationBudgetAllocator.cpp:75, 115, 135).
	const IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("a.Budget.Enabled"));
	const IAnimationBudgetAllocator* Allocator = IAnimationBudgetAllocator::Get(GetWorld());
	return Var && Var->GetInt() == 1 && Allocator && Allocator->GetEnabled();
}

FString ALabGameMode::AppliedString() const
{
	// Q3: 설정(lab_config tech=)이 아니라 실제로 적용된 값. 빼 보기 러너가 설정과 함께 대조한다.
	const ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>();
	const bool bCrowd = Crowd && Crowd->IsActive();
	return FString::Printf(TEXT("stage=%s budget=%d pool=%d iskm_bounds=%d fixmask=%d crowdfx_active=%d %s"), FLabConfig::StageName(FLabConfig::Get().Stage), IsBudgetActive() ? 1 : 0,
		FLabConfig::Get().bEntityPool ? 1 : 0, bCrowd ? Crowd->GetIskmBoundsApplied() : 0, FLabConfig::Get().HydrateFixMask, CrowdFx && CrowdFx->IsActive() ? 1 : 0, *FLabConfig::Get().TechString());
}

int32 ALabGameMode::ComputeNear30() const
{
	if (const ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>(); Crowd && Crowd->IsActive())
	{
		return Crowd->Near30.load();
	}
	if (!Spawner || !Orb)
	{
		return 0;
	}
	if (const ALabCrowdManager* Manager = Spawner->GetManager())
	{
		return Manager->GetNear30(); // 변형 manager: 매니저가 같은 정의로 센다
	}
	// M0: ULabNearOrbCountProcessor와 같은 정의. 액터 위치는 캡슐 중심이라 높이를 빼고 수평 거리로 잰다
	const FVector Center = Orb->GetActorLocation();
	const double RadiusSq = FMath::Square(double(FLabEquivalence::NearOrbCm));
	int32 Count = 0;
	for (const ALabMinion* Minion : Spawner->GetMinions())
	{
		if (Minion && FVector::DistSquared2D(Minion->GetActorLocation(), Center) <= RadiusSq)
		{
			++Count;
		}
	}
	return Count;
}

int32 ALabGameMode::GetDeathCount() const
{
	return Orb ? Orb->GetKillCount() + Orb->GetSnipeCount() : 0;
}

bool ALabGameMode::IsHudShown() const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const ALabHud* Hud = PC ? Cast<ALabHud>(PC->GetHUD()) : nullptr;
	return Hud && Hud->IsShown();
}

int32 ALabGameMode::GetHudMinions() const
{
	return Spawner ? Spawner->GetAliveCount() : 0;
}

int32 ALabGameMode::GetHudActors() const
{
	if (const ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>(); Crowd && Crowd->IsActive())
	{
		return FLabConfig::Get().Stage >= ELabStage::M3 ? Crowd->ActorCount.load() : 0;
	}
	return Spawner ? Spawner->GetMinions().Num() : 0; // M0: 모든 개체가 액터다
}

ALabGameMode::ALabGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr;
	HUDClass = ALabHud::StaticClass();
}

void ALabGameMode::StartPlay()
{
	Super::StartPlay();

	const FLabConfig& Config = FLabConfig::Get();
	UE_LOG(LogLab, Log, TEXT("%s"), *Config.ToLogString());
	if (!Config.bStageValid)
	{
		// 아직 구현하지 않은 단계. 러너가 실패로 알아보도록 종료 코드 2로 끝낸다.
		UE_LOG(LogLab, Error, TEXT("lab_stage_unknown"));
		LabRequestExit(2, TEXT("LabStage"));
		return;
	}
	if (!Config.bVariantValid)
	{
		// 모르는 변형 이름. 단계와 같이 러너가 실패로 알아보도록 종료 코드 2로 끝낸다.
		FString Variant;
		FParse::Value(FCommandLine::Get(), TEXT("-LabVariant="), Variant);
		UE_LOG(LogLab, Error, TEXT("lab_variant_unknown variant=%s"), *Variant);
		LabRequestExit(2, TEXT("LabVariant"));
		return;
	}

	ApplyVariantCVars();
	if (Config.Variant == ELabVariant::MassSt || Config.FullyParallel == 0)
	{
		// E1 (b): Mass 처리 단계를 태스크 그래프에 나누지 않는다. 런타임에 바꿔도 단계가 다시 구성된다(MassProcessingPhaseManager.cpp:576).
		// M3b 직렬 트레이스는 -LabFullyParallel=0으로 같은 경로를 쓴다(run-m3b.ps1 -SerialArgs).
		IConsoleManager::Get().FindConsoleVariable(TEXT("mass.FullyParallel"))->Set(0, ECVF_SetByCode);
	}
	if (Config.Variant == ELabVariant::M3Lock0)
	{
		// 진단 D안: DefaultEngine.ini의 1을 이 변형에서만 0으로 덮는다. LabFixedCVarString이 실제 값을 읽어 LabCVars 메타데이터에 0으로 찍힌다.
		LabSetCVar(TEXT("mass.commands.LockObserversDuringFlushing"), TEXT("0"));
	}
#if !UE_BUILD_TEST
	// 결정 6 바닥 띠(개발 전용). P2(탑뷰)에서는 끈다. CVar는 lab_build 로그와 CSV 메타데이터 LabCVars에 찍히도록 여기서 먼저 둔다.
	const bool bBandFloorArg = FParse::Param(FCommandLine::Get(), TEXT("LabBandFloor"));
	const bool bBandFloor = bBandFloorArg && Config.Preset != 2;
	if (bBandFloor)
	{
		LabSetCVar(TEXT("lab.BandFloor"), TEXT("1"));
	}
	else if (bBandFloorArg)
	{
		UE_LOG(LogLab, Log, TEXT("lab_bandfloor on=0 reason=P2"));
	}
#endif
	UE_LOG(LogLab, Log, TEXT("lab_build commit=%s driver=%s %s cvars=%s crowdfx=%d crowdfx_particles=%d"), TEXT(LAB_BUILD_COMMIT), *GRHIAdapterName, *GRHIAdapterUserDriverVersion, *LabFixedCVarString(),
		Config.bCrowdFx ? 1 : 0, Config.CrowdFxParticles());
	if (Config.bBench)
	{
		ApplyBenchCVars();
	}

	UWorld* World = GetWorld();
	Spawner = World->SpawnActor<ALabCrowdSpawner>();
	Orb = World->SpawnActor<ALabOrb>(FVector::ZeroVector, FRotator::ZeroRotator);
	Orb->SetSpawner(Spawner);
#if !UE_BUILD_TEST
	if (bBandFloor)
	{
		SetupBandFloor();
	}
#endif
	if (Config.bCrowdFx)
	{
		SetupCrowdFx();
	}
	Spawner->SpawnInitial();
	if (ALabCrowdManager* Manager = Spawner->GetManager())
	{
		Manager->SetOrb(Orb); // 변형 manager: 매니저 틱이 오브의 저격 시계와 이펙트를 쓴다
	}
	StartFrame = GFrameCounter;

	Camera = World->SpawnActor<ALabCameraRig>();
	Camera->SetPreset(Config.Preset, FVector::ZeroVector);
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		PC->SetViewTarget(Camera);
	}
}

void ALabGameMode::SetupBandFloor()
{
#if !UE_BUILD_TEST
	// 머티리얼 경로는 코드에 박지 않고 Config/DefaultGame.ini [Lab] BandFloorMaterial에서 읽는다(에셋 세션이 만든 에셋).
	FString MaterialPath;
	if (!GConfig || !GConfig->GetString(TEXT("Lab"), TEXT("BandFloorMaterial"), MaterialPath, GGameIni) || MaterialPath.IsEmpty())
	{
		UE_LOG(LogLab, Warning, TEXT("lab_bandfloor on=0 reason=no_ini_key [Lab] BandFloorMaterial"));
		return;
	}
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!Material)
	{
		UE_LOG(LogLab, Warning, TEXT("lab_bandfloor on=0 reason=material_missing %s"), *MaterialPath);
		return;
	}
	// L_Bench의 바닥(Basic 템플릿의 StaticMeshActor "Floor", Scripts/create-bench-map.py). 액터 라벨은 에디터 전용이라 이름 접두사로 찾는다.
	UStaticMeshComponent* Floor = nullptr;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		if (It->GetName().StartsWith(TEXT("Floor")) && It->GetStaticMeshComponent())
		{
			Floor = It->GetStaticMeshComponent();
			break;
		}
	}
	if (!Floor)
	{
		UE_LOG(LogLab, Warning, TEXT("lab_bandfloor on=0 reason=floor_not_found"));
		return;
	}
	BandFloorMID = UMaterialInstanceDynamic::Create(Material, this);
	// 띠 반경은 저격 구간과 같은 상수 하나에서(FLabSnipeBands::Edges 30/80/200m). 머티리얼 기본값도 같지만 한 곳만 바꾸면 되게 넣는다.
	BandFloorMID->SetScalarParameterValue(TEXT("BandRadius1"), FLabSnipeBands::Edges[0]);
	BandFloorMID->SetScalarParameterValue(TEXT("BandRadius2"), FLabSnipeBands::Edges[1]);
	BandFloorMID->SetScalarParameterValue(TEXT("BandRadius3"), FLabSnipeBands::Edges[2]);
	BandFloorMID->SetVectorParameterValue(TEXT("BandCenter"), FLinearColor(Orb->GetActorLocation()));
	const FString Previous = GetNameSafe(Floor->GetMaterial(0));
	Floor->SetMaterial(0, BandFloorMID);
	UE_LOG(LogLab, Log, TEXT("lab_bandfloor on=1 floor=%s previous=%s material=%s radii=%.0f/%.0f/%.0f"), *Floor->GetOwner()->GetName(), *Previous, *Material->GetName(),
		FLabSnipeBands::Edges[0], FLabSnipeBands::Edges[1], FLabSnipeBands::Edges[2]);
#endif
}

void ALabGameMode::SetupCrowdFx()
{
	const FLabConfig& Config = FLabConfig::Get();
	// 시스템 경로는 코드에 박지 않고 Config/DefaultGame.ini [Lab] CrowdFxSystem에서 읽는다(BandFloorMaterial과 같은 방식). 쿡은 DirectoriesToAlwaysCook의 /Game/Lab이 덮는다.
	FString SystemPath;
	if (!GConfig || !GConfig->GetString(TEXT("Lab"), TEXT("CrowdFxSystem"), SystemPath, GGameIni) || SystemPath.IsEmpty())
	{
		UE_LOG(LogLab, Warning, TEXT("lab_crowdfx on=0 reason=no_ini_key [Lab] CrowdFxSystem"));
		return;
	}
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *SystemPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!System)
	{
		UE_LOG(LogLab, Warning, TEXT("lab_crowdfx on=0 reason=system_missing %s"), *SystemPath);
		return;
	}
	const float Lifetime = Config.CrowdFxLifetime();
	if (Lifetime <= 0.f)
	{
		UE_LOG(LogLab, Warning, TEXT("lab_crowdfx on=0 reason=inner_beyond_outer inner=%.0f outer=%.0f"), Config.CrowdFxInnerCm(), FLabCrowdFx::OuterRadius);
		return;
	}
	// 오브에 붙이지 않고 오브 위치에 둔다. 시스템이 로컬 공간이라 부모의 회전을 따라가지 않게 하려는 것이다(오브는 움직이지 않는다).
	// bPreCullCheck를 끈다: 켜면 스폰 때 시스템의 컬링 거리로 생성 자체를 건너뛸 수 있다. 자동 활성화도 끄고 사용자 변수를 먼저 넣는다(워밍업이 활성화 때 이 값으로 돈다).
	CrowdFx = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, System, Orb->GetActorLocation(), FRotator::ZeroRotator, FVector(1.f),
		/*bAutoDestroy*/ false, /*bAutoActivate*/ false, ENCPoolMethod::None, /*bPreCullCheck*/ false);
	if (!CrowdFx)
	{
		UE_LOG(LogLab, Warning, TEXT("lab_crowdfx on=0 reason=spawn_failed"));
		return;
	}
	// Q2c 입자 그림자: 엔티티 구동 입자의 그림자를 원거리 그림자 스위치(-LabFarShadow)에 묶는다. 시스템 bOverrideCastShadow가 꺼져 있어 컴포넌트 CastShadow가
	// 그대로 쓰이고(NiagaraComponent.cpp:4678), 메시 배치 그림자는 프록시 CastsDynamicShadow && 렌더러 bCastShadows && 섹션 값의 곱이다(NiagaraRendererMeshes.cpp:1290).
	// 그래서 컴포넌트를 끄면 모든 이미터가 꺼지고, 켜도 렌더러에서 꺼 둔 바깥 독립 이미터(Small·Large)는 켜지지 않는다. 옛 방식(엔티티 구동 끔)은 건드리지 않는다.
	const bool bFxShadowSpawn = CrowdFx->CastShadow;
	if (Config.bCrowdFxEntity)
	{
		CrowdFx->SetCastShadow(Config.bFarShadow);
	}
	CrowdFx->SetVariableFloat(TEXT("User.SmallRate"), Config.CrowdFxSmallRate());
	CrowdFx->SetVariableFloat(TEXT("User.LargeRate"), Config.CrowdFxLargeRate());
	CrowdFx->SetVariableFloat(TEXT("User.OuterRadius"), FLabCrowdFx::OuterRadius);
	CrowdFx->SetVariableFloat(TEXT("User.Lifetime"), Lifetime);
	CrowdFx->SetVariableFloat(TEXT("User.Speed"), FLabScenario::MoveSpeed);

	// Q2b 엔티티 구동 입자: 에셋에 약속한 사용자 변수가 있는지 활성화 전에 한 번 본다(값을 넣으면 없는 이름도 덮어쓰기 저장소에 더해지므로 넣기 전에 본다).
	// 없는 변수는 프로세서가 넣지 않는다. Niagara의 배열 함수는 없는 이름에 부를 때마다 경고를 남긴다(NiagaraFunctionLibrary.cpp:716-719).
	FString EntityParams;
	if (Config.bCrowdFxEntity)
	{
		const FNiagaraParameterStore& Params = CrowdFx->GetOverrideParameters();
		auto Has = [&Params](const TCHAR* Name) { return Params.FindParameterOffset(FNiagaraVariableBase(FNiagaraTypeDefinition::GetFloatDef(), FName(Name)), /*IgnoreType*/ true) != nullptr; };
		auto HasArray4 = [this](const TCHAR* Name) { return UNiagaraFunctionLibrary::GetDataInterface<UNiagaraDataInterfaceArrayFloat4>(CrowdFx, FName(Name)) != nullptr; };
		ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>();
		Crowd->bCrowdFxSmallRecs = Has(FLabCrowdFx::SmallRecsParam) && HasArray4(FLabCrowdFx::SmallRecsParam); // 이름이 있을 때만 형을 본다(없는 이름은 Niagara 경고가 한 줄 더 나온다)
		Crowd->bCrowdFxLargeRecs = Has(FLabCrowdFx::LargeRecsParam) && HasArray4(FLabCrowdFx::LargeRecsParam);
		Crowd->bCrowdFxNow = Has(FLabCrowdFx::NowParam);
		const bool bOrbPos = Has(FLabCrowdFx::OrbPosParam);
		if (bOrbPos)
		{
			CrowdFx->SetVariableVec3(FLabCrowdFx::OrbPosParam, Orb->GetActorLocation()); // Position 형이면 엔진이 Position으로 넣는다(NiagaraComponent.cpp SetVariableVec3)
		}
		const TCHAR* Names[4] = { FLabCrowdFx::SmallRecsParam, FLabCrowdFx::LargeRecsParam, FLabCrowdFx::NowParam, FLabCrowdFx::OrbPosParam };
		const bool bOk[4] = { Crowd->bCrowdFxSmallRecs, Crowd->bCrowdFxLargeRecs, Crowd->bCrowdFxNow, bOrbPos };
		for (int32 i = 0; i < 4; ++i)
		{
			if (!bOk[i])
			{
				UE_LOG(LogLab, Warning, TEXT("lab_crowdfx_param missing=%s system=%s (entity-driven particles skip this value until the asset has it)"), Names[i], *System->GetName());
			}
		}
		Crowd->CrowdFx = CrowdFx;
		EntityParams = FString::Printf(TEXT(" small_recs=%d large_recs=%d now=%d orb_pos=%d"), bOk[0] ? 1 : 0, bOk[1] ? 1 : 0, bOk[2] ? 1 : 0, bOk[3] ? 1 : 0);
	}

	CrowdFx->Activate(true);
	UE_LOG(LogLab, Log, TEXT("lab_crowdfx on=1 system=%s small_rate=%.2f large_rate=%.2f outer=%.0f inner=%.0f lifetime=%.2f speed=%.0f particles=%d entity=%d fx_shadow=%d fx_shadow_spawn=%d%s"),
		*System->GetName(), Config.CrowdFxSmallRate(), Config.CrowdFxLargeRate(), FLabCrowdFx::OuterRadius, Config.CrowdFxInnerCm(), Lifetime, FLabScenario::MoveSpeed, Config.CrowdFxParticles(),
		Config.bCrowdFxEntity ? 1 : 0, CrowdFx->CastShadow ? 1 : 0, bFxShadowSpawn ? 1 : 0, *EntityParams);
}

void ALabGameMode::ApplyVariantCVars()
{
	const ELabVariant V = FLabConfig::Get().Variant;
	if (V != ELabVariant::Budget && V != ELabVariant::M3BudgetDist && V != ELabVariant::M3BudgetSig && !FLabConfig::Get().bScreenSigBudget)
	{
		return;
	}
	// a.Budget.Enabled 기본값은 0이다(AnimationBudgetAllocatorCVars.cpp:7).
	LabSetCVar(TEXT("a.Budget.Enabled"), TEXT("1"));
	// 예산 값은 -LabBudgetMs=로 바꾼다(기본 FLabScenario::BudgetMs = 4, ADR-0016). 0 이하는 FLabConfig가 이미 화면 크기 Budget을 껐으므로 여기 오는 것은 변형이 정한 예산뿐이다. 그때는 기본값을 쓴다.
	const float BudgetMs = FLabConfig::Get().BudgetMs > 0.f ? FLabConfig::Get().BudgetMs : FLabScenario::BudgetMs;
	LabSetCVar(TEXT("a.Budget.BudgetMs"), *FString::SanitizeFloat(BudgetMs));
	// CVar만으로는 켜지지 않는다. 월드마다 만들어지는 할당기는 bEnabled(false)로 시작한다(AnimationBudgetAllocator.cpp:71).
	// 이걸 빠뜨린 2026-11-07 첫 E0의 budget 3조건은 무효로 하고 다시 쟀다.
	if (IAnimationBudgetAllocator* Allocator = IAnimationBudgetAllocator::Get(GetWorld()))
	{
		Allocator->SetEnabled(true);
	}
}

void ALabGameMode::ApplyBenchCVars()
{
	// measurement.md "렌더 고정". 해상도와 창 모드는 run-bench.ps1이 -ResX/-ResY로 넘긴다. 품질은 스케일러빌리티 Epic(3)으로 고정한다.
	LabSetCVar(TEXT("r.VSync"), TEXT("0"));
	LabSetCVar(TEXT("t.MaxFPS"), TEXT("0"));
	LabSetCVar(TEXT("r.ScreenPercentage"), TEXT("100"));
	LabSetCVar(TEXT("r.DynamicRes.OperationMode"), TEXT("0"));
	LabSetCVar(TEXT("csv.CompressionMode"), TEXT("0")); // .csv로 쓴다(CsvProfiler.cpp:183-194)
	// 스케일러빌리티 Epic(3). measurement.md "확정값".
	if (GEngine)
	{
		GEngine->Exec(GetWorld(), TEXT("scalability 3"));
	}
}

void ALabGameMode::TickBench()
{
	const FLabConfig& Config = FLabConfig::Get();
	BenchPhaseTime += GetWorld()->GetDeltaSeconds();

	switch (BenchPhase)
	{
	case ELabBenchPhase::Warmup:
		if (BenchPhaseTime >= FLabBench::WarmupSec)
		{
#if CSV_PROFILER
			CSV_METADATA(TEXT("LabN"), *FString::FromInt(Config.N));
			CSV_METADATA(TEXT("LabPreset"), *FString::Printf(TEXT("P%d"), Config.Preset));
			CSV_METADATA(TEXT("LabSeed"), *FString::FromInt(Config.Seed));
			CSV_METADATA(TEXT("LabOrbit"), *Config.Orbit.ToString()); // Spike1 떠다니는 오브. 켜지 않으면 "-"
			CSV_METADATA(TEXT("LabVariant"), FLabConfig::VariantName(Config.Variant));
			CSV_METADATA(TEXT("LabLabel"), *Config.Label);
			// M3b 2종: 종류 수와 에셋 상태. substituted(대형 에셋 없음, 소형으로 대체)는 스모크 전용이라 비교에 쓰지 않는다(LabUnitAssets.h).
			CSV_METADATA(TEXT("LabTypes"), *FString::FromInt(Config.Types));
			CSV_METADATA(TEXT("LabTypesAssets"), Config.Types == 2 ? (FLabUnitTypes::bLargeSubstituted ? TEXT("substituted") : TEXT("ok")) : TEXT("-"));
			CSV_METADATA(TEXT("LabFarM"), *FString::FromInt(Config.FarM)); // 그리는 거리(표현 LOD Off, m4offmove는 Simulation LOD Off도). 기본 200
			CSV_METADATA(TEXT("LabSimLod"), *Config.SimLodString()); // M4: 거리와 틱 간격. M3까지는 "-"
			// Q2 Niagara 배경 무리: 켬 여부와 정상 상태 입자 수 계산값(스폰율 × 수명). 시스템을 못 찾아 켜지 못했으면 LabCrowdFxActive 0
			CSV_METADATA(TEXT("LabCrowdFx"), Config.bCrowdFx ? TEXT("1") : TEXT("0"));
			CSV_METADATA(TEXT("LabCrowdFxParticles"), *FString::FromInt(Config.CrowdFxParticles()));
			CSV_METADATA(TEXT("LabCrowdFxActive"), CrowdFx && CrowdFx->IsActive() ? TEXT("1") : TEXT("0"));
			// Q2b: 엔티티 구동 입자(1) / 옛 방식 전부 독립 입자(0) / 배경 무리 끔(-). 엔티티 구동 입자 수는 CSV 스탯 Lab/CrowdRecs, 모으기 시간은 Lab/CrowdFxGather
			CSV_METADATA(TEXT("LabCrowdFxEntity"), !Config.bCrowdFx ? TEXT("-") : Config.bCrowdFxEntity ? TEXT("1") : TEXT("0"));
			// Q3 빼 보기(plan.md "총괄 묶음 러너"): 단계, 실제로 적용된 값(Budget 할당기, 엔티티 풀, 고정 바운드가 들어간 ISKM 컴포넌트 수, 전환 대책 마스크),
			// 아홉 기법의 설정 켬·끔(LabTech*, FLabConfig::TechList). 러너가 실행마다 기대값과 대조한다.
			CSV_METADATA(TEXT("LabStage"), FLabConfig::StageName(Config.Stage));
			CSV_METADATA(TEXT("LabBudget"), IsBudgetActive() ? TEXT("1") : TEXT("0"));
			// 실제로 걸린 예산(a.Budget.BudgetMs의 값). 할당기가 꺼져 있으면 "-". 참고 묶음 Budget1이 조건을 가리는 키다
			{
				const IConsoleVariable* BudgetVar = IConsoleManager::Get().FindConsoleVariable(TEXT("a.Budget.BudgetMs"));
				CSV_METADATA(TEXT("LabBudgetMs"), IsBudgetActive() && BudgetVar ? *BudgetVar->GetString() : TEXT("-"));
			}
			CSV_METADATA(TEXT("LabPool"), Config.bEntityPool ? TEXT("1") : TEXT("0"));
			{
				const ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>();
				CSV_METADATA(TEXT("LabIskmBounds"), *FString::FromInt(Crowd && Crowd->IsActive() ? Crowd->GetIskmBoundsApplied() : 0));
			}
			CSV_METADATA(TEXT("LabHydrateFixMask"), *FString::FromInt(Config.HydrateFixMask));
			// Q4 근거리 품질: 액터 상한(두 종류 합계)과 화면 안 전환 거리(m). M3 미만 단계에서는 쓰이지 않는 값이다
			CSV_METADATA(TEXT("LabActorCap"), *FString::FromInt(Config.ActorCap));
			CSV_METADATA(TEXT("LabHydrateM"), *FString::Printf(TEXT("%.0f"), Config.HydrateM));
			CSV_METADATA(TEXT("LabFarShadow"), Config.FarShadowString()); // Q4b: 표현 LOD Low(80m~) 인스턴스 그림자. M3 미만 "-"
			for (const FLabConfig::FTech& Tech : Config.TechList())
			{
				CSV_METADATA(Tech.MetaKey, *Tech.Value);
			}
			// Q5: 사다리 단계(FLabConfig::Ladder), 변형 기본값과 다르게 준 인자, 하이드레이션 액터 캡슐 콜리전, HUD 켬(촬영 실행), 촬영 시각.
			// 측정 실행은 LabHud 0·LabShotAt 0이어야 한다. 러너가 대조한다(Docs/Worklog/Q5.md).
			CSV_METADATA(TEXT("LabLadder"), *Config.Ladder().Id);
			CSV_METADATA(TEXT("LabOverrides"), Config.Overrides.IsEmpty() ? TEXT("-") : *Config.Overrides);
			CSV_METADATA(TEXT("LabActorCollision"), Config.ActorCollisionString());
			CSV_METADATA(TEXT("LabHud"), IsHudShown() ? TEXT("1") : TEXT("0"));
			CSV_METADATA(TEXT("LabShotAt"), *FString::Printf(TEXT("%.0f"), Config.ShotAtSec));
			// 측정 방법 5번(plan.md "M3b"): 빌드 커밋, 고정 CVar 값, GPU 드라이버. summarize.py BUILD_KEYS(labbuild·labcvars·labdriver)와 이름을 맞춘다.
			CSV_METADATA(TEXT("LabBuild"), TEXT(LAB_BUILD_COMMIT));
			CSV_METADATA(TEXT("LabCVars"), *LabFixedCVarString());
			CSV_METADATA(TEXT("LabDriver"), *FString::Printf(TEXT("%s %s"), *GRHIAdapterName, *GRHIAdapterUserDriverVersion));
			FCsvProfiler::Get()->BeginCapture(-1, FString(), Config.Label);
#endif
			RespawnsAtStart = Spawner->GetRespawnCount();
			KillsAtStart = Orb->GetKillCount();
			SnipesAtStart = Orb->GetSnipeCount();
			Near30Sum = 0;
			AliveMin = TNumericLimits<int32>::Max();
			AliveMax = 0;
			if (ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>(); Crowd && Crowd->IsActive())
			{
				LargeActorAtStart[0] = Crowd->LargeActorFrames.load();
				LargeActorAtStart[1] = Crowd->LargeActorSkips.load();
				LargeActorAtStart[2] = Crowd->LargeActorInterps.load();
				SmallActorAtStart[0] = Crowd->SmallActorFrames.load();
				SmallActorAtStart[1] = Crowd->SmallActorSkips.load();
				SmallActorAtStart[2] = Crowd->SmallActorInterps.load();
			}
			BenchPhase = ELabBenchPhase::Measure;
			BenchPhaseTime = 0.f;
			BenchFrames = 0;
			UE_LOG(LogLab, Log, TEXT("lab_measure_start n=%d"), Spawner->GetAliveCount());
			UE_LOG(LogLab, Log, TEXT("lab_applied %s"), *AppliedString()); // Q3: 메타데이터와 같은 시점의 실제 적용값(개발 빌드만 로그가 남는다)
		}
		break;

	case ELabBenchPhase::Measure:
		++BenchFrames;
		// Q5 동등성 통계의 측정 구간 집계(Tick이 이번 프레임 값을 먼저 구한다)
		Near30Sum += LastNear30;
		AliveMin = FMath::Min(AliveMin, LastAlive);
		AliveMax = FMath::Max(AliveMax, LastAlive);
		// E5 (b)(plan.md "M5"): -LabBench에 -LabDuration=초를 주면 측정 구간을 그 길이로 늘린다(16분 누수 진단 960). 주지 않으면 지금까지와 같은 60초다
		if (BenchPhaseTime >= (Config.DurationSec > 0.f ? Config.DurationSec : FLabBench::MeasureSec))
		{
			const int32 Alive = Spawner->GetAliveCount();
			const bool bOk = Alive == Config.N && BenchFrames > 0;
			// Q5: 측정 구간의 사망 수(범위 피해 + 저격), 초당 사망 수, 오브 30m 안 개체 수 평균, 살아 있는 총수의 최소·최대. 키는 기존 키 뒤에 붙인다(run-bench.ps1의 정규식과 겹치지 않는다)
			const int32 ZoneKills = Orb->GetKillCount() - KillsAtStart;
			const int32 Snipes = Orb->GetSnipeCount() - SnipesAtStart;
			const int32 Deaths = ZoneKills + Snipes;
			const float DeathRate = Deaths / FMath::Max(BenchPhaseTime, 0.001f);
			const float Near30Mean = float(double(Near30Sum) / FMath::Max(BenchFrames, 1));
			const FString Summary = FString::Printf(TEXT("lab_summary label=%s measure_sec=%.2f frames=%d avg_ms=%.3f n=%d respawns=%d ok=%d deaths=%d zone_kills=%d snipes=%d death_rate=%.3f near30_mean=%.2f alive_min=%d alive_max=%d"),
				*Config.Label, BenchPhaseTime, BenchFrames, BenchPhaseTime * 1000.f / FMath::Max(BenchFrames, 1),
				Alive, Spawner->GetRespawnCount() - RespawnsAtStart, bOk ? 1 : 0, Deaths, ZoneKills, Snipes, DeathRate, Near30Mean, AliveMin, AliveMax);
#if CSV_PROFILER
			CSV_METADATA(TEXT("LabAlive"), *FString::FromInt(Alive));
			CSV_METADATA(TEXT("LabRespawns"), *FString::FromInt(Spawner->GetRespawnCount() - RespawnsAtStart));
			CSV_METADATA(TEXT("LabOk"), bOk ? TEXT("1") : TEXT("0"));
			CSV_METADATA(TEXT("LabDeaths"), *FString::FromInt(Deaths));
			CSV_METADATA(TEXT("LabZoneKills"), *FString::FromInt(ZoneKills));
			CSV_METADATA(TEXT("LabSnipes"), *FString::FromInt(Snipes));
			CSV_METADATA(TEXT("LabDeathRate"), *FString::Printf(TEXT("%.3f"), DeathRate));
			CSV_METADATA(TEXT("LabNear30Mean"), *FString::Printf(TEXT("%.2f"), Near30Mean));
			CSV_METADATA(TEXT("LabAliveMin"), *FString::FromInt(AliveMin));
			CSV_METADATA(TEXT("LabAliveMax"), *FString::FromInt(AliveMax));
			if (ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>(); Crowd && Crowd->IsActive())
			{
				// 애니 갱신 지표(종류별): 측정 구간의 액터×프레임 합, 그중 애니 갱신을 하지 않은 수(보간 + 건너뜀), 그중 보간한 수(ULabActorSyncProcessor).
				// LabLargeActorFrames·Skips는 전과 같은 키와 뜻이다. 건너뜀 비율 = (Skips - Interps) / Frames
				auto Delta = [](const std::atomic<int64>& Now, int64 Start) { return FString::Printf(TEXT("%lld"), (long long)(Now.load() - Start)); };
				CSV_METADATA(TEXT("LabLargeActorFrames"), *Delta(Crowd->LargeActorFrames, LargeActorAtStart[0]));
				CSV_METADATA(TEXT("LabLargeActorSkips"), *Delta(Crowd->LargeActorSkips, LargeActorAtStart[1]));
				CSV_METADATA(TEXT("LabLargeActorInterps"), *Delta(Crowd->LargeActorInterps, LargeActorAtStart[2]));
				CSV_METADATA(TEXT("LabSmallActorFrames"), *Delta(Crowd->SmallActorFrames, SmallActorAtStart[0]));
				CSV_METADATA(TEXT("LabSmallActorSkips"), *Delta(Crowd->SmallActorSkips, SmallActorAtStart[1]));
				CSV_METADATA(TEXT("LabSmallActorInterps"), *Delta(Crowd->SmallActorInterps, SmallActorAtStart[2]));
			}
			FCsvProfiler::Get()->EndCapture();
#endif
			// Test 구성은 로그를 남기지 않는다(Build.h UE_BUILD_TEST 블록, NO_LOGGING = !USE_LOGGING_IN_SHIPPING).
			// 요약은 파일로도 남겨 run-bench.ps1이 읽는다.
			UE_LOG(LogLab, Log, TEXT("%s"), *Summary);
			FFileHelper::SaveStringToFile(Summary, *(FPaths::ProjectSavedDir() / TEXT("Lab") / (Config.Label + TEXT(".summary.txt"))));
			BenchExitCode = bOk ? 0 : 1;
			BenchPhase = ELabBenchPhase::Flush;
			BenchPhaseTime = 0.f;
		}
		break;

	case ELabBenchPhase::Flush:
		// CSV는 별도 스레드가 쓴다. 파일이 닫힐 시간을 둔다.
		if (BenchPhaseTime >= FLabBench::FlushSec)
		{
			BenchPhase = ELabBenchPhase::Done;
			FPlatformMisc::RequestExitWithStatus(false, BenchExitCode, TEXT("LabBench"));
		}
		break;

	default:
		break;
	}
}

FString ALabGameMode::GetHudCrowdLine() const
{
	if (!Spawner || !Orb)
	{
		return FString();
	}
	return FString::Printf(TEXT("alive %d   respawns %d   zone kills %d   snipes %d   t %.0f s"),
		Spawner->GetAliveCount(), Spawner->GetRespawnCount(), Orb->GetKillCount(), Orb->GetSnipeCount(), Elapsed);
}

void ALabGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	// 설정·에셋 오류의 종료 요청(LabRequestExit). 비강제 종료는 종료 코드가 0이 되므로 강제 종료한다(HordeOptLab.h). CSV 캡처는 아직 열리지 않았다
	{
		uint8 ExitCode = 0;
		const TCHAR* CallSite = nullptr;
		if (LabConsumePendingExit(ExitCode, CallSite))
		{
			FPlatformMisc::RequestExitWithStatus(true, ExitCode, CallSite);
			return;
		}
	}
	if (!Spawner)
	{
		return;
	}
#if !UE_BUILD_TEST
	if (BandFloorMID && Orb)
	{
		BandFloorMID->SetVectorParameterValue(TEXT("BandCenter"), FLinearColor(Orb->GetActorLocation())); // 결정 6: 띠 중심은 오브(맵 중앙 고정이지만 매 프레임 넣는다)
	}
	// Q2 배경 무리의 실제 입자 수(5초마다). GPU 이미터는 수 읽기가 몇 프레임 늦다(NiagaraEmitterInstance.cpp:34-48). 진단 로그라 시뮬레이션 틱과의 경합은 무시한다.
	if (CrowdFx && Elapsed >= CrowdFxNextLog)
	{
		CrowdFxNextLog = Elapsed + 5.f;
		const FNiagaraSystemInstanceControllerPtr Controller = CrowdFx->GetSystemInstanceController();
		if (const FNiagaraSystemInstance* Instance = Controller.IsValid() ? Controller->GetSystemInstance_Unsafe() : nullptr)
		{
			FString Counts;
			int32 Total = 0;
			for (const FNiagaraEmitterInstanceRef& Emitter : Instance->GetEmitters())
			{
				const int32 Num = Emitter->GetNumParticles();
				Total += Num;
				Counts += FString::Printf(TEXT(" %s=%d"), *Emitter->GetEmitterHandle().GetName().ToString(), Num);
			}
			// expected = 독립 입자 계산값, recs = 지난 프레임에 넣은 엔티티 구동 원소 수(Q2b, 옛 방식이면 0)
			const ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>();
			UE_LOG(LogLab, Log, TEXT("lab_crowdfx_count t=%.1f total=%d%s expected=%d recs=%d"), Elapsed, Total, *Counts, FLabConfig::Get().CrowdFxParticles(), Crowd ? Crowd->CrowdRecsLast : 0);
		}
	}
	// E3b (8) 화질 캡처(-LabFrameLog): 이전 프레임의 화면 안 30m VAT 인스턴스 수·보이는 액터 수를 남기고 슬롯을 비운다. 프로세서는 이번 프레임 슬롯에 쓴다.
	static const bool bFrameLog = FParse::Param(FCommandLine::Get(), TEXT("LabFrameLog"));
	if (bFrameLog)
	{
		if (const ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>(); Crowd && Crowd->IsActive())
		{
			ULabCrowdSubsystem::FFrameDiag& D = Crowd->FrameDiag[(GFrameCounter - 1) & 1];
			UE_LOG(LogLab, Log, TEXT("lab_frame frame=%llu near_vat=%d near_actors=%d view_actors=%d"), (unsigned long long)(GFrameCounter - 1), D.NearVat.exchange(0), D.NearActors.exchange(0), D.ViewActors.exchange(0));
		}
	}
#endif

	// F1: 상태 오버레이 토글(벤치마크 실행에서는 오버레이가 그려지지 않는다).
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->WasInputKeyJustPressed(EKeys::F1))
		{
			if (ALabHud* Hud = Cast<ALabHud>(PC->GetHUD()))
			{
				Hud->Toggle();
			}
		}
	}

	// budget 변형: Significance Manager는 게임이 시점을 넘겨 갱신해야 한다(SignificanceManager.h:118 Update).
	if ((FLabConfig::Get().Variant == ELabVariant::Budget || FLabConfig::Get().Variant == ELabVariant::M3BudgetDist) && Camera)
	{
		if (USignificanceManager* Sig = USignificanceManager::Get(GetWorld()))
		{
			const FTransform View = Camera->GetActorTransform();
			Sig->Update(TArrayView<const FTransform>(&View, 1));
		}
	}

	// Q5 동등성 통계(모든 단계, 같은 뜻, Docs/Worklog/Q5.md "동등성 통계"): 살아 있는 총수(사망 중 포함, 늘 N이어야 한다), 누적 사망 수, 오브 30m 안 개체 수.
	// 초당 사망 수는 Lab/Deaths의 측정 구간 증가분 ÷ 측정 시간이다(실행 끝 메타데이터 LabDeathRate에도 있다).
	LastAlive = Spawner->GetAliveCount();
	LastNear30 = ComputeNear30();
	CSV_CUSTOM_STAT(Lab, Alive, LastAlive, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, Deaths, GetDeathCount(), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, Near30, LastNear30, ECsvCustomStatOp::Set);

	const float ShotAt = FLabConfig::Get().ShotAtSec;
	if (FLabConfig::Get().bBench)
	{
		// Q5 촬영 실행(-LabBench -LabHud -LabShotAt=초): 측정과 같은 조건에서 창 해상도 스크린샷 한 장. 화면 버퍼를 그대로 읽어 HUD(캔버스)가 함께 찍힌다
		// (UnrealClient.cpp:1866-1872 Draw → Canvas Flush → ProcessScreenShots, GameViewportClient.cpp:2419-2426). HighResShot은 다시 그리므로 쓰지 않는다.
		// 측정 중 한 프레임이 길어지므로 측정 실행은 -LabShotAt을 주지 않는다(메타데이터 LabShotAt).
		if (ShotAt > 0.f && !bShotTaken && Elapsed >= ShotAt)
		{
			bShotTaken = true;
			const FString Path = FPaths::ProjectSavedDir() / TEXT("Screenshots/Lab") / FString::Printf(TEXT("%s-t%.0f.png"), FLabConfig::Get().Label.IsEmpty() ? TEXT("bench") : *FLabConfig::Get().Label, ShotAt);
			FScreenshotRequest::RequestScreenshot(Path, /*bShowUI*/ false, /*bAddFilenameSuffix*/ false);
		}
		TickBench();
		return;
	}

#if !UE_BUILD_TEST
	// 개발용 연속 캡처(-LabShotBurst=N): ShotAt부터 N프레임을 창 해상도로 한 장씩 남긴다. 전환 깜빡임 진단용.
	static int32 BurstLeft = -1;
	int32 Burst = 0;
	if (ShotAt > 0.f && Elapsed >= ShotAt && FParse::Value(FCommandLine::Get(), TEXT("-LabShotBurst="), Burst))
	{
		if (BurstLeft < 0) { BurstLeft = Burst; }
		if (BurstLeft > 0)
		{
			--BurstLeft;
			FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("%s/Screenshots/LabBurst/f%06llu.png"), *FPaths::ProjectSavedDir(), (unsigned long long)GFrameCounter), false, false);
		}
		bShotTaken = true;
	}
#endif
	if (ShotAt > 0.f && !bShotTaken && Elapsed >= ShotAt)
	{
		bShotTaken = true;
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->ConsoleCommand(TEXT("HighResShot 2560x1440")); // 스크린샷은 창 크기와 무관하게 QHD로 렌더한다
		}
	}

	const float Duration = FLabConfig::Get().DurationSec;
	if (Duration > 0.f && Elapsed >= Duration && !bQuitRequested)
	{
		bQuitRequested = true;
		if (const ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>(); Crowd && Crowd->DamageHits > 0)
		{
			UE_LOG(LogLab, Log, TEXT("lab_damage hits=%lld per_sec=%.0f"), Crowd->DamageHits.load(), Crowd->DamageHits.load() / FMath::Max(Elapsed, 0.01f));
		}
		UE_LOG(LogLab, Log, TEXT("lab_applied %s"), *AppliedString()); // Q3: 개발 실행에서도 실제 적용값을 남긴다(벤치마크는 측정 시작 때)
		UE_LOG(LogLab, Log, TEXT("lab_summary elapsed=%.1f n=%d respawns=%d kills=%d snipes=%d avg_ms=%.1f deaths=%d near30=%d actors=%d"),
			Elapsed, Spawner->GetAliveCount(), Spawner->GetRespawnCount(), Orb->GetKillCount(), Orb->GetSnipeCount(),
			1000.f * Elapsed / FMath::Max<uint64>(GFrameCounter - StartFrame, 1), GetDeathCount(), LastNear30, GetHudActors()); // Q5: 누적 사망, 오브 30m 안(마지막 프레임), 액터
		FPlatformMisc::RequestExit(false, TEXT("LabDuration"));
	}
}
