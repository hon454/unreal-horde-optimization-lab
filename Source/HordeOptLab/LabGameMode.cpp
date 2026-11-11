#include "LabGameMode.h"
#include "HordeOptLab.h"
#include "LabCameraRig.h"
#include "LabConfig.h"
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
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "IAnimationBudgetAllocator.h"
#include "SignificanceManager.h"
#include "RHI.h" // GRHIAdapterName, GRHIAdapterUserDriverVersion
#include "LabBuildVersion.h" // LAB_BUILD_COMMIT. HordeOptLab.Build.cs가 .git/HEAD에서 만든다

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
		FPlatformMisc::RequestExitWithStatus(false, 2, TEXT("LabStage"));
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
		// 진단 D안(2026-11-09): DefaultEngine.ini의 1을 이 변형에서만 0으로 덮는다. LabFixedCVarString이 실제 값을 읽어 LabCVars 메타데이터에 0으로 찍힌다.
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
	UE_LOG(LogLab, Log, TEXT("lab_build commit=%s driver=%s %s cvars=%s"), TEXT(LAB_BUILD_COMMIT), *GRHIAdapterName, *GRHIAdapterUserDriverVersion, *LabFixedCVarString());
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
	Spawner->SpawnInitial();
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

void ALabGameMode::ApplyVariantCVars()
{
	const ELabVariant V = FLabConfig::Get().Variant;
	if (V != ELabVariant::Budget && V != ELabVariant::M3BudgetDist && V != ELabVariant::M3BudgetSig && !FLabConfig::Get().bScreenSigBudget)
	{
		return;
	}
	// a.Budget.Enabled 기본값은 0이다(AnimationBudgetAllocatorCVars.cpp:7).
	LabSetCVar(TEXT("a.Budget.Enabled"), TEXT("1"));
	LabSetCVar(TEXT("a.Budget.BudgetMs"), *FString::SanitizeFloat(FLabScenario::BudgetMs));
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
			CSV_METADATA(TEXT("LabVariant"), FLabConfig::VariantName(Config.Variant));
			CSV_METADATA(TEXT("LabLabel"), *Config.Label);
			// M3b 2종: 종류 수와 에셋 상태. substituted(대형 에셋 없음, 소형으로 대체)는 스모크 전용이라 비교에 쓰지 않는다(LabUnitAssets.h).
			CSV_METADATA(TEXT("LabTypes"), *FString::FromInt(Config.Types));
			CSV_METADATA(TEXT("LabTypesAssets"), Config.Types == 2 ? (FLabUnitTypes::bLargeSubstituted ? TEXT("substituted") : TEXT("ok")) : TEXT("-"));
			CSV_METADATA(TEXT("LabFarM"), *FString::FromInt(Config.FarM)); // 그리는 거리(표현 LOD Off, m4offmove는 Simulation LOD Off도). 기본 200
			CSV_METADATA(TEXT("LabSimLod"), *Config.SimLodString()); // M4: 거리와 틱 간격. M3까지는 "-"
			// 측정 방법 5번(plan.md "M3b"): 빌드 커밋, 고정 CVar 값, GPU 드라이버. summarize.py BUILD_KEYS(labbuild·labcvars·labdriver)와 이름을 맞춘다.
			CSV_METADATA(TEXT("LabBuild"), TEXT(LAB_BUILD_COMMIT));
			CSV_METADATA(TEXT("LabCVars"), *LabFixedCVarString());
			CSV_METADATA(TEXT("LabDriver"), *FString::Printf(TEXT("%s %s"), *GRHIAdapterName, *GRHIAdapterUserDriverVersion));
			FCsvProfiler::Get()->BeginCapture(-1, FString(), Config.Label);
#endif
			RespawnsAtStart = Spawner->GetRespawnCount();
			if (ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>(); Crowd && Crowd->IsActive())
			{
				LargeActorFramesAtStart = Crowd->LargeActorFrames.load();
				LargeActorSkipsAtStart = Crowd->LargeActorSkips.load();
			}
			BenchPhase = ELabBenchPhase::Measure;
			BenchPhaseTime = 0.f;
			BenchFrames = 0;
			UE_LOG(LogLab, Log, TEXT("lab_measure_start n=%d"), Spawner->GetAliveCount());
		}
		break;

	case ELabBenchPhase::Measure:
		++BenchFrames;
		// E5 (b)(plan.md "M5"): -LabBench에 -LabDuration=초를 주면 측정 구간을 그 길이로 늘린다(16분 누수 진단 960). 주지 않으면 지금까지와 같은 60초다
		if (BenchPhaseTime >= (Config.DurationSec > 0.f ? Config.DurationSec : FLabBench::MeasureSec))
		{
			const int32 Alive = Spawner->GetAliveCount();
			const bool bOk = Alive == Config.N && BenchFrames > 0;
			const FString Summary = FString::Printf(TEXT("lab_summary label=%s measure_sec=%.2f frames=%d avg_ms=%.3f n=%d respawns=%d ok=%d"),
				*Config.Label, BenchPhaseTime, BenchFrames, BenchPhaseTime * 1000.f / FMath::Max(BenchFrames, 1),
				Alive, Spawner->GetRespawnCount() - RespawnsAtStart, bOk ? 1 : 0);
#if CSV_PROFILER
			CSV_METADATA(TEXT("LabAlive"), *FString::FromInt(Alive));
			CSV_METADATA(TEXT("LabRespawns"), *FString::FromInt(Spawner->GetRespawnCount() - RespawnsAtStart));
			CSV_METADATA(TEXT("LabOk"), bOk ? TEXT("1") : TEXT("0"));
			if (ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>(); Crowd && Crowd->IsActive())
			{
				// E3 (3c) 지표: 측정 구간에서 대형 액터가 있었던 프레임 합(액터×프레임)과 그중 애니 틱을 건너뛴 수(ULabActorSyncProcessor). 비율 = Skips / Frames
				CSV_METADATA(TEXT("LabLargeActorFrames"), *FString::Printf(TEXT("%lld"), (long long)(Crowd->LargeActorFrames.load() - LargeActorFramesAtStart)));
				CSV_METADATA(TEXT("LabLargeActorSkips"), *FString::Printf(TEXT("%lld"), (long long)(Crowd->LargeActorSkips.load() - LargeActorSkipsAtStart)));
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
	if (!Spawner)
	{
		return;
	}
#if !UE_BUILD_TEST
	if (BandFloorMID && Orb)
	{
		BandFloorMID->SetVectorParameterValue(TEXT("BandCenter"), FLinearColor(Orb->GetActorLocation())); // 결정 6: 띠 중심은 오브(맵 중앙 고정이지만 매 프레임 넣는다)
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

	if (FLabConfig::Get().bBench)
	{
		TickBench();
		return;
	}

	const float ShotAt = FLabConfig::Get().ShotAtSec;
#if !UE_BUILD_TEST
	// 개발용 연속 캡처(-LabShotBurst=N): ShotAt부터 N프레임을 창 해상도로 한 장씩 남긴다. 전환 깜빡임 진단용(2026-11-08).
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
		UE_LOG(LogLab, Log, TEXT("lab_summary elapsed=%.1f n=%d respawns=%d kills=%d snipes=%d avg_ms=%.1f"),
			Elapsed, Spawner->GetAliveCount(), Spawner->GetRespawnCount(), Orb->GetKillCount(), Orb->GetSnipeCount(),
			1000.f * Elapsed / FMath::Max<uint64>(GFrameCounter - StartFrame, 1));
		FPlatformMisc::RequestExit(false, TEXT("LabDuration"));
	}
}
