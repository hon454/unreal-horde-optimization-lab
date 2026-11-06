#include "LabGameMode.h"
#include "HordeOptLab.h"
#include "LabCameraRig.h"
#include "LabConfig.h"
#include "LabCrowdSpawner.h"
#include "LabOrb.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "SignificanceManager.h"

ALabGameMode::ALabGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr;
}

void ALabGameMode::StartPlay()
{
	Super::StartPlay();

	const FLabConfig& Config = FLabConfig::Get();
	UE_LOG(LogLab, Log, TEXT("%s"), *Config.ToLogString());

	ApplyVariantCVars();

	UWorld* World = GetWorld();
	Spawner = World->SpawnActor<ALabCrowdSpawner>();
	Orb = World->SpawnActor<ALabOrb>(FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator);
	Orb->SetSpawner(Spawner);
	Spawner->SpawnInitial();

	Camera = World->SpawnActor<ALabCameraRig>();
	Camera->SetPreset(Config.Preset, FVector::ZeroVector);
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		PC->SetViewTarget(Camera);
	}
}

void ALabGameMode::ApplyVariantCVars()
{
	if (FLabConfig::Get().Variant != ELabVariant::Budget)
	{
		return;
	}
	// a.Budget.Enabled 기본값은 0이다(AnimationBudgetAllocatorCVars.cpp:7).
	IConsoleManager::Get().FindConsoleVariable(TEXT("a.Budget.Enabled"))->Set(1, ECVF_SetByCode);
	IConsoleManager::Get().FindConsoleVariable(TEXT("a.Budget.BudgetMs"))->Set(FLabScenario::BudgetMs, ECVF_SetByCode);
}

void ALabGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;

	// budget 변형: Significance Manager는 게임이 시점을 넘겨 갱신해야 한다(SignificanceManager.h:118 Update).
	if (FLabConfig::Get().Variant == ELabVariant::Budget && Camera)
	{
		if (USignificanceManager* Sig = USignificanceManager::Get(GetWorld()))
		{
			const FTransform View = Camera->GetActorTransform();
			Sig->Update(TArrayView<const FTransform>(&View, 1));
		}
	}

	const float ShotAt = FLabConfig::Get().ShotAtSec;
	if (ShotAt > 0.f && !bShotTaken && Elapsed >= ShotAt)
	{
		bShotTaken = true;
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->ConsoleCommand(TEXT("HighResShot 1"));
		}
	}

	const float Duration = FLabConfig::Get().DurationSec;
	if (Duration > 0.f && Elapsed >= Duration && !bQuitRequested)
	{
		bQuitRequested = true;
		UE_LOG(LogLab, Log, TEXT("lab_summary elapsed=%.1f n=%d respawns=%d kills=%d snipes=%d"),
			Elapsed, Spawner->GetMinions().Num(), Spawner->GetRespawnCount(), Orb->GetKillCount(), Orb->GetSnipeCount());
		FPlatformMisc::RequestExit(false, TEXT("LabDuration"));
	}
}
