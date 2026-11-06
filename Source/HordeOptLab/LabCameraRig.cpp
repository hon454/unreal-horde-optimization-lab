#include "LabCameraRig.h"
#include "LabConfig.h"

ALabCameraRig::ALabCameraRig()
{
	PrimaryActorTick.bCanEverTick = true;
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
	switch (Preset)
	{
	case 2:
		// 탑뷰: 모든 개체가 원거리 밴드에 드는 대조군.
		SetActorLocationAndRotation(Target + FVector(0.f, 0.f, FLabScenario::TopDownHeight), FRotator(-90.f, 0.f, 0.f));
		return;
	case 3:
		// 오브 지상: 오브 뒤 5m 높이에서 오브 너머 +X 방향 고정. 몰려오는 개체를 정면으로 보는, 근거리 액터 비중이 가장 큰 대조군.
		SetActorLocationAndRotation(Target + FVector(-FLabScenario::GroundOffset, 0.f, FLabScenario::GroundEyeHeight), FRotator(FLabScenario::GroundPitch, 0.f, 0.f));
		return;
	default:
	{
		// 오브 시점(대표): 오브 뒤에서 오브 너머 바깥을 보며 1rpm으로 오브 둘레를 돈다.
		// 측정 창 60초에 정확히 한 바퀴라 방향 편차가 평균으로 정리된다.
		const float Yaw = 360.f * Elapsed / FLabScenario::OrbViewPeriod;
		const FVector Forward = FRotator(0.f, Yaw, 0.f).Vector();
		const FVector Location = Target - Forward * FLabScenario::OrbViewBack + FVector(0.f, 0.f, FLabScenario::OrbViewHeight);
		SetActorLocationAndRotation(Location, FRotator(FLabScenario::OrbViewPitch, Yaw, 0.f));
		return;
	}
	}
}
