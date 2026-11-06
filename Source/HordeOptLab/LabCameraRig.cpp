#include "LabCameraRig.h"
#include "LabConfig.h"

ALabCameraRig::ALabCameraRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
}

void ALabCameraRig::SetPreset(int32 InPreset, const FVector& InTarget)
{
	Preset = InPreset;
	Target = InTarget;
	Elapsed = 0.f;
	Apply();
}

void ALabCameraRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	if (Preset == 1)
	{
		Apply();
	}
}

void ALabCameraRig::Apply()
{
	FVector Location;
	switch (Preset)
	{
	case 2:
		Location = Target + FVector(0.f, 0.f, FLabScenario::TopDownHeight);
		SetActorLocationAndRotation(Location, FRotator(-90.f, 0.f, 0.f));
		return;
	case 3:
		Location = Target + FVector(-FLabScenario::GroundDistance, 0.f, FLabScenario::GroundEyeHeight);
		break;
	default:
	{
		// 1rpm. 측정 창 60초에 정확히 한 바퀴.
		const float Angle = UE_TWO_PI * Elapsed / FLabScenario::OrbitPeriod;
		Location = Target + FVector(FLabScenario::OrbitRadius * FMath::Cos(Angle), FLabScenario::OrbitRadius * FMath::Sin(Angle), FLabScenario::OrbitHeight);
		break;
	}
	}
	SetActorLocationAndRotation(Location, (Target - Location).Rotation());
}
