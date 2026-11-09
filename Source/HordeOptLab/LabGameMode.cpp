#include "LabGameMode.h"
#include "HordeOptLab.h"
#include "LabCameraRig.h"
#include "LabConfig.h"
#include "LabCrowdSpawner.h"
#include "LabCrowdSubsystem.h"
#include "LabHud.h"
#include "InputCoreTypes.h"
#include "LabOrb.h"
#include "Engine/Engine.h"
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
	UE_LOG(LogLab, Log, TEXT("lab_build commit=%s driver=%s %s cvars=%s"), TEXT(LAB_BUILD_COMMIT), *GRHIAdapterName, *GRHIAdapterUserDriverVersion, *LabFixedCVarString());
	if (Config.bBench)
	{
		ApplyBenchCVars();
	}

	UWorld* World = GetWorld();
	Spawner = World->SpawnActor<ALabCrowdSpawner>();
	Orb = World->SpawnActor<ALabOrb>(FVector::ZeroVector, FRotator::ZeroRotator);
	Orb->SetSpawner(Spawner);
	Spawner->SpawnInitial();
	StartFrame = GFrameCounter;

	Camera = World->SpawnActor<ALabCameraRig>();
	Camera->SetPreset(Config.Preset, FVector::ZeroVector);
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		PC->SetViewTarget(Camera);
	}
}

void ALabGameMode::ApplyVariantCVars()
{
	const ELabVariant V = FLabConfig::Get().Variant;
	if (V != ELabVariant::Budget && V != ELabVariant::M3BudgetDist && V != ELabVariant::M3BudgetSig)
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
			// 측정 방법 5번(plan.md "M3b"): 빌드 커밋, 고정 CVar 값, GPU 드라이버. summarize.py BUILD_KEYS(labbuild·labcvars·labdriver)와 이름을 맞춘다.
			CSV_METADATA(TEXT("LabBuild"), TEXT(LAB_BUILD_COMMIT));
			CSV_METADATA(TEXT("LabCVars"), *LabFixedCVarString());
			CSV_METADATA(TEXT("LabDriver"), *FString::Printf(TEXT("%s %s"), *GRHIAdapterName, *GRHIAdapterUserDriverVersion));
			FCsvProfiler::Get()->BeginCapture(-1, FString(), Config.Label);
#endif
			RespawnsAtStart = Spawner->GetRespawnCount();
			BenchPhase = ELabBenchPhase::Measure;
			BenchPhaseTime = 0.f;
			BenchFrames = 0;
			UE_LOG(LogLab, Log, TEXT("lab_measure_start n=%d"), Spawner->GetAliveCount());
		}
		break;

	case ELabBenchPhase::Measure:
		++BenchFrames;
		if (BenchPhaseTime >= FLabBench::MeasureSec)
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
