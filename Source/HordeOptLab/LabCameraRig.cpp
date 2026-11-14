#include "LabCameraRig.h"
#include "LabConfig.h"
#include "HordeOptLab.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ALabCameraRig::ALabCameraRig()
{
	PrimaryActorTick.bCanEverTick = true;
#if !UE_BUILD_TEST
	bCamTurn = FParse::Param(FCommandLine::Get(), TEXT("LabCamTurn"));
#endif
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
	Apply(); // 탑뷰도 오브를 따라간다(v2)
}

void ALabCameraRig::Apply()
{
	// 중심은 오브의 지금 위치다. 오브와 같은 함수(FLabOrbit, 월드 시각)라 틱 순서와 무관하게 같은 프레임의 위치를 쓴다
	const FVector Center = Target + FLabConfig::Get().Orbit.LocationAt(GetWorld()->GetTimeSeconds());
	switch (Preset)
	{
	case 2:
		// 탑뷰: 모든 개체가 원거리 밴드에 드는 대조군.
		SetActorLocationAndRotation(Center + FVector(0.f, 0.f, FLabScenario::TopDownHeight), FRotator(-90.f, 0.f, 0.f));
		return;
	default:
	{
		// 오브 시점(대표): 오브 뒤에서 오브 너머 바깥을 보며 1rpm으로 오브 둘레를 돈다.
		// 측정 창 60초에 정확히 한 바퀴라 방향 편차가 평균으로 정리된다.
		float Yaw = 360.f * Elapsed / FLabScenario::OrbViewPeriod;
#if !UE_BUILD_TEST
		if (bCamTurn)
		{
			// 회전 k는 [k·5초, k·5초 + 0.5초]에 90·k → 90·(k+1)도로 돌고, 그 뒤 4.5초는 90·(k+1)도에 멈춘다.
			const int32 K = FMath::FloorToInt(Elapsed / TurnPeriod);
			const float InPeriod = Elapsed - K * TurnPeriod;
			const float Progress = FMath::Clamp(InPeriod / TurnDuration, 0.f, 1.f);
			Yaw = TurnStep * (K + Progress);
			if (TurnStarted != K) { TurnStarted = K; TurnStartFrame = GFrameCounter; }
			if (Progress >= 1.f && TurnLogged != K)
			{
				TurnLogged = K;
				UE_LOG(LogLab, Log, TEXT("lab_turn k=%d start_frame=%llu end_frame=%llu yaw=%.0f"), K, (unsigned long long)TurnStartFrame, (unsigned long long)GFrameCounter, Yaw);
			}
		}
#endif
		const FVector Forward = FRotator(0.f, Yaw, 0.f).Vector();
		const FVector Location = Center - Forward * FLabScenario::OrbViewBack + FVector(0.f, 0.f, FLabScenario::OrbViewHeight);
		SetActorLocationAndRotation(Location, FRotator(FLabScenario::OrbViewPitch, Yaw, 0.f));
		return;
	}
	}
}
