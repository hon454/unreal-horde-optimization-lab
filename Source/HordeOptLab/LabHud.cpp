#include "LabHud.h"
#include "LabConfig.h"
#include "LabGameMode.h"
#include "DynamicRHI.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "HAL/PlatformMemory.h"
#include "RenderTimer.h"

void ALabHud::BeginPlay()
{
	Super::BeginPlay();
	bVisible = FLabConfig::Get().bHud;
}

void ALabHud::DrawHUD()
{
	Super::DrawHUD();

	const float DeltaMs = GetWorld()->GetDeltaSeconds() * 1000.f;
	SmoothedFrameMs = SmoothedFrameMs <= 0.f ? DeltaMs : FMath::Lerp(SmoothedFrameMs, DeltaMs, 0.1f);

	if (!bVisible || FLabConfig::Get().bBench || !Canvas)
	{
		return;
	}

	const FLabConfig& Config = FLabConfig::Get();
	const ALabGameMode* GameMode = GetWorld()->GetAuthGameMode<ALabGameMode>();
	const FPlatformMemoryStats Mem = FPlatformMemory::GetStats();

	// 엔진이 프레임마다 갱신하는 스레드 시간(RenderTimer.h)과 GPU 프레임 시간(DynamicRHI.h).
	const float GameMs = FPlatformTime::ToMilliseconds(GGameThreadTime);
	const float RenderMs = FPlatformTime::ToMilliseconds(GRenderThreadTime);
	const float GpuMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("HordeOptLab  %s / %s   N=%d   P%d   seed %d"),
		FLabConfig::StageName(Config.Stage), FLabConfig::VariantName(Config.Variant), Config.N, Config.Preset, Config.Seed));
	Lines.Add(FString::Printf(TEXT("%.0f fps   frame %.2f ms   game %.2f   render %.2f   gpu %.2f"),
		SmoothedFrameMs > 0.f ? 1000.f / SmoothedFrameMs : 0.f, SmoothedFrameMs, GameMs, RenderMs, GpuMs));
	Lines.Add(FString::Printf(TEXT("memory %.0f MB used physical"), Mem.UsedPhysical / (1024.0 * 1024.0)));
	if (GameMode)
	{
		Lines.Add(GameMode->GetHudCrowdLine());
	}

	UFont* Font = GEngine->GetMediumFont();
	const float Scale = Canvas->ClipY / 1080.f;
	float Y = 16.f * Scale;
	for (const FString& Line : Lines)
	{
		Canvas->SetDrawColor(FColor::Black);
		Canvas->DrawText(Font, Line, 17.f * Scale, Y + 1.f * Scale, Scale, Scale);
		Canvas->SetDrawColor(FColor::White);
		Canvas->DrawText(Font, Line, 16.f * Scale, Y, Scale, Scale);
		Y += 22.f * Scale;
	}
}
