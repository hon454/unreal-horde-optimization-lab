#include "LabHud.h"
#include "LabConfig.h"
#include "LabGameMode.h"
#include "CanvasItem.h"
#include "DynamicRHI.h"      // RHIGetGPUFrameCycles: CSV GPUTime과 같은 값(LaunchEngineLoop.cpp:1520)
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineFontServices.h"
#include "Fonts/FontMeasure.h"
#include "Fonts/SlateFontInfo.h"
#include "Misc/App.h"
#include "RenderTimer.h"     // GGameThreadTime, GRenderThreadTime: CSV GameThreadTime·RenderThreadTime과 같은 값(LaunchEngineLoop.cpp:1518-1519)

namespace
{
	constexpr double GraphSeconds = 5.0;   // 그래프가 보이는 시간
	constexpr float GraphMaxMs = 33.3f;    // 그래프 위 끝(30fps)
	constexpr float TargetMs = 1000.f / 60.f; // 60fps 선

	const FLinearColor PanelColor(0.f, 0.f, 0.f, 0.55f);
	const FLinearColor GraphBackColor(1.f, 1.f, 1.f, 0.06f);
	const FLinearColor TextColor(0.95f, 0.95f, 0.95f, 1.f);
	const FLinearColor DimColor(0.62f, 0.65f, 0.70f, 1.f);
	const FLinearColor AccentColor(0.45f, 0.75f, 1.f, 1.f);
	const FLinearColor GoodColor(0.30f, 0.78f, 0.42f, 0.95f);
	const FLinearColor WarnColor(0.98f, 0.66f, 0.22f, 0.95f);
	const FLinearColor BadColor(0.95f, 0.28f, 0.24f, 0.95f);
	const FLinearColor TargetLineColor(1.f, 1.f, 1.f, 0.45f);

	/** 로캘과 무관한 천 단위 쉼표(10,000) */
	FString GroupThousands(int32 Value)
	{
		FString Digits = FString::FromInt(FMath::Abs(Value));
		for (int32 i = Digits.Len() - 3; i > 0; i -= 3)
		{
			Digits.InsertAt(i, TEXT(','));
		}
		return Value < 0 ? TEXT("-") + Digits : Digits;
	}

	FLinearColor BarColor(float Ms)
	{
		return Ms <= TargetMs ? GoodColor : Ms <= GraphMaxMs ? WarnColor : BadColor;
	}

	/** 캔버스 글자 한 줄. 판 위라 그림자는 옅게 둔다 */
	void LabText(UCanvas* Canvas, const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Color, float X, float Y, float Scale)
	{
		FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.6f), FVector2D(1.f, 1.f) * FMath::Max(1.f, FMath::RoundToFloat(Scale)));
		Canvas->DrawItem(Item);
	}

	void LabRect(UCanvas* Canvas, float X, float Y, float W, float H, const FLinearColor& Color)
	{
		FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), Color);
		Tile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tile);
	}
}

void ALabHud::BeginPlay()
{
	Super::BeginPlay();
	bVisible = FLabConfig::Get().bHud;
	History.SetNum(HistoryCapacity);
}

void ALabHud::Toggle()
{
	bVisible = !bVisible;
	WindowStart = 0.0; // 다시 켤 때 이전 기록을 쓰지 않는다
	HistoryCount = 0;
	bHasAverage = false;
}

void ALabHud::Sample()
{
	const double Now = FPlatformTime::Seconds();
	// 프레임 시간은 실제 경과 시간(FApp::GetDeltaTime, 시간 배율·최대 델타 제한 없음)이다. CSV FrameTime과 같은 출처다
	const float FrameMs = float(FApp::GetDeltaTime() * 1000.0);
	SumFrameMs += FrameMs;
	SumGameMs += FPlatformTime::ToMilliseconds(GGameThreadTime);
	SumRenderMs += FPlatformTime::ToMilliseconds(GRenderThreadTime);
	SumGpuMs += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
	++WindowFrames;
	if (WindowStart <= 0.0)
	{
		WindowStart = Now;
	}
	if (Now - WindowStart >= 1.0)
	{
		AvgFrameMs = float(SumFrameMs / WindowFrames);
		AvgGameMs = float(SumGameMs / WindowFrames);
		AvgRenderMs = float(SumRenderMs / WindowFrames);
		AvgGpuMs = float(SumGpuMs / WindowFrames);
		bHasAverage = true;
		WindowStart = Now;
		WindowFrames = 0;
		SumFrameMs = SumGameMs = SumRenderMs = SumGpuMs = 0.0;
	}
	History[HistoryNext] = { Now, FrameMs };
	HistoryNext = (HistoryNext + 1) % HistoryCapacity;
	HistoryCount = FMath::Min(HistoryCount + 1, HistoryCapacity);
}

void ALabHud::DrawHUD()
{
	Super::DrawHUD();
	if (!bVisible || !Canvas || !GEngine)
	{
		return; // 숨겨 둔 측정 실행에서는 아무것도 하지 않는다
	}
	Sample();

	const FLabConfig& Config = FLabConfig::Get();
	const ALabGameMode* GameMode = GetWorld()->GetAuthGameMode<ALabGameMode>();
	const TSharedPtr<FSlateFontMeasure> Measure = FEngineFontServices::IsInitialized() ? FEngineFontServices::Get().GetFontMeasure() : nullptr;
	if (!Measure.IsValid())
	{
		return;
	}

	// 1080p 기준 배치를 화면 높이에 비례시킨다. 720p에서는 0.8에서 멈춰 글자가 읽히게 한다(작아지는 대신 판이 화면의 조금 더 넓은 몫을 차지한다)
	const float S = FMath::Max(Canvas->ClipY / 1080.f, 0.8f);
	const UObject* FontObject = GEngine->GetMediumFont(); // 엔진 Roboto(합성 글꼴, 런타임 캐시)
	auto MakeFont = [FontObject, S](float Points, const TCHAR* Typeface) { return FSlateFontInfo(FontObject, FMath::RoundToFloat(Points * S), FName(Typeface)); };
	const FSlateFontInfo TitleFont = MakeFont(15.f, TEXT("Bold"));
	const FSlateFontInfo SubFont = MakeFont(10.f, TEXT("Regular"));
	const FSlateFontInfo BigFont = MakeFont(21.f, TEXT("Bold"));
	const FSlateFontInfo BodyFont = MakeFont(11.5f, TEXT("Regular"));
	const FSlateFontInfo CaptionFont = MakeFont(9.f, TEXT("Regular"));
	auto Width = [&Measure](const FString& Text, const FSlateFontInfo& Font) { return float(FVector2D(Measure->Measure(Text, Font)).X); };
	auto Height = [&Measure](const FSlateFontInfo& Font) { return float(FVector2D(Measure->Measure(FString(TEXT("Hg")), Font)).Y); };

	// 글 내용
	const FLabConfig::FLadder Ladder = Config.Ladder();
	const FString StepText = Ladder.Id == TEXT("-") ? FString() : Ladder.Id;
	const FString TitleText = Ladder.Id == TEXT("-") ? FString::Printf(TEXT("%s / %s"), FLabConfig::StageName(Config.Stage), FLabConfig::VariantName(Config.Variant)) : Ladder.Name;
	FString SubText = FString::Printf(TEXT("%s  ·  %s  ·  P%d  ·  %d %s"), FLabConfig::VariantName(Config.Variant), FLabConfig::StageName(Config.Stage), Config.Preset,
		Config.Types, Config.Types == 1 ? TEXT("type") : TEXT("types"));
	if (!Config.Overrides.IsEmpty())
	{
		SubText += TEXT("  ·  ") + Config.Overrides.Replace(TEXT("|"), TEXT(", "));
	}
	const float FrameMs = bHasAverage ? AvgFrameMs : (WindowFrames > 0 ? float(SumFrameMs / WindowFrames) : 0.f);
	const float GameMs = bHasAverage ? AvgGameMs : (WindowFrames > 0 ? float(SumGameMs / WindowFrames) : 0.f);
	const float RenderMs = bHasAverage ? AvgRenderMs : (WindowFrames > 0 ? float(SumRenderMs / WindowFrames) : 0.f);
	const float GpuMs = bHasAverage ? AvgGpuMs : (WindowFrames > 0 ? float(SumGpuMs / WindowFrames) : 0.f);
	const FString FrameText = FString::Printf(TEXT("%.2f ms"), FrameMs);
	const FString FpsText = FString::Printf(TEXT("%.0f fps"), FrameMs > 0.f ? 1000.f / FrameMs : 0.f);
	const FString ThreadText = FString::Printf(TEXT("Game %.2f    Render %.2f    GPU %.2f ms"), GameMs, RenderMs, GpuMs);
	const FString CountText = GameMode ? FString::Printf(TEXT("Minions %s    Actors %s"), *GroupThousands(GameMode->GetHudMinions()), *GroupThousands(GameMode->GetHudActors())) : FString();
	const FString CaptionText = FString::Printf(TEXT("frame time, last %.0f s  (1 s averages above)"), GraphSeconds);
	const FString TargetText = TEXT("16.7 ms");

	// 판 크기
	const float Pad = 14.f * S;
	const float Gap = 6.f * S;
	const float StepGap = 10.f * S;
	const float StepW = StepText.IsEmpty() ? 0.f : Width(StepText, TitleFont) + StepGap;
	float InnerW = 330.f * S;
	InnerW = FMath::Max(InnerW, StepW + Width(TitleText, TitleFont));
	InnerW = FMath::Max(InnerW, Width(SubText, SubFont));
	InnerW = FMath::Max(InnerW, Width(FrameText, BigFont) + 12.f * S + Width(FpsText, BodyFont));
	InnerW = FMath::Max(InnerW, Width(ThreadText, BodyFont));
	InnerW = FMath::Max(InnerW, Width(CountText, BodyFont));
	const float TitleH = Height(TitleFont), SubH = Height(SubFont), BigH = Height(BigFont), BodyH = Height(BodyFont), CaptionH = Height(CaptionFont);
	const float GraphH = 64.f * S;
	const float PanelH = Pad + TitleH + 2.f * S + SubH + Gap * 2.f + BigH + Gap * 0.5f + BodyH + Gap * 0.5f + BodyH + Gap * 2.f + GraphH + 3.f * S + CaptionH + Pad;
	const float X0 = 20.f * S;
	const float Y0 = 20.f * S;
	LabRect(Canvas, X0, Y0, InnerW + 2.f * Pad, PanelH, PanelColor);

	// 글
	const float X = X0 + Pad;
	float Y = Y0 + Pad;
	if (!StepText.IsEmpty())
	{
		LabText(Canvas, StepText, TitleFont, AccentColor, X, Y, S);
	}
	LabText(Canvas, TitleText, TitleFont, TextColor, X + StepW, Y, S);
	Y += TitleH + 2.f * S;
	LabText(Canvas, SubText, SubFont, DimColor, X, Y, S);
	Y += SubH + Gap * 2.f;
	LabText(Canvas, FrameText, BigFont, TextColor, X, Y, S);
	// fps는 큰 글자의 밑선에 맞춘다(두 글꼴 높이 차의 대부분이 위쪽 여백이다)
	LabText(Canvas, FpsText, BodyFont, BarColor(FrameMs), X + Width(FrameText, BigFont) + 12.f * S, Y + (BigH - BodyH) * 0.8f, S);
	Y += BigH + Gap * 0.5f;
	LabText(Canvas, ThreadText, BodyFont, TextColor, X, Y, S);
	Y += BodyH + Gap * 0.5f;
	LabText(Canvas, CountText, BodyFont, TextColor, X, Y, S);
	Y += BodyH + Gap * 2.f;

	// 그래프: 막대 하나 = 최근 GraphSeconds초를 나눈 구간의 최댓값(히치가 묻히지 않게)
	const float GX = X;
	const float GY = Y;
	const float GW = InnerW;
	LabRect(Canvas, GX, GY, GW, GraphH, GraphBackColor);
	const float BarW = FMath::Max(2.f, FMath::RoundToFloat(2.f * S));
	const int32 NumBars = FMath::Max(1, FMath::FloorToInt(GW / BarW));
	TArray<float, TInlineAllocator<512>> Bars;
	Bars.Init(0.f, NumBars);
	const double Now = FPlatformTime::Seconds();
	for (int32 k = 0; k < HistoryCount; ++k)
	{
		const FSample& Sm = History[(HistoryNext - 1 - k + HistoryCapacity) % HistoryCapacity];
		const double Age = Now - Sm.Time;
		if (Age > GraphSeconds)
		{
			break; // 오래된 것부터 버린다(고리 버퍼를 새것부터 거꾸로 읽는다)
		}
		const int32 Index = FMath::Clamp(int32((1.0 - Age / GraphSeconds) * NumBars), 0, NumBars - 1);
		Bars[Index] = FMath::Max(Bars[Index], Sm.Ms);
	}
	for (int32 i = 0; i < NumBars; ++i)
	{
		if (Bars[i] <= 0.f)
		{
			continue;
		}
		const float H = FMath::Min(Bars[i] / GraphMaxMs, 1.f) * GraphH;
		LabRect(Canvas, GX + i * BarW, GY + GraphH - H, FMath::Max(1.f, BarW - 1.f), H, BarColor(Bars[i]));
	}
	const float TargetY = GY + GraphH - TargetMs / GraphMaxMs * GraphH;
	LabRect(Canvas, GX, TargetY, GW, FMath::Max(1.f, FMath::RoundToFloat(S)), TargetLineColor);
	LabText(Canvas, TargetText, CaptionFont, DimColor, GX + GW - Width(TargetText, CaptionFont) - 4.f * S, TargetY - CaptionH - 1.f * S, S);
	Y += GraphH + 3.f * S;
	LabText(Canvas, CaptionText, CaptionFont, DimColor, X, Y, S);
}
