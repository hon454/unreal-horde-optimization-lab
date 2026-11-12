#include "LabHud.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
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
	/** 독자용 문구(Docs/Worklog/Q5-hud.md "문구 표"). Tag는 강조색 머리, Title은 굵은 첫 줄, Change는 앞 단계에서 바꾼 것 */
	struct FHudStepText { FString Tag; FString Title; FString Change; };

	/** 빼 보기·하위 실험 인자 하나(LabOverrides 토큰)의 문구. bMinus면 최종 구성에서 기법 하나를 뺀 것이다 */
	bool OverrideText(const FString& Token, const FLabConfig& Config, FString& OutWhat, FString& OutChange, bool& bMinus)
	{
		bMinus = true;
		if (Token == TEXT("hydrate=0")) { OutWhat = TEXT("near actors"); OutChange = FString::Printf(TEXT("minions within %.0f m stay instances"), Config.HydrateM); return true; }
		if (Token == TEXT("screensig_budget=0")) { OutWhat = TEXT("animation budget"); OutChange = TEXT("every actor animates every frame"); return true; }
		if (Token == TEXT("fixmask=0")) { OutWhat = TEXT("hand-off fixes"); OutChange = TEXT("no pop-in fixes between actor and instance"); return true; }
		if (Token == TEXT("pool=0")) { OutWhat = TEXT("entity pool"); OutChange = TEXT("dead entities are destroyed and re-created"); return true; }
		if (Token == TEXT("simlod=0")) { OutWhat = TEXT("Simulation LOD"); OutChange = TEXT("far minions update every frame"); return true; }
		if (Token == TEXT("iskm=0")) { OutWhat = TEXT("ISKM"); OutChange = TEXT("instances go back to VAT"); return true; }
		if (Token == TEXT("fixed_bounds=0")) { OutWhat = TEXT("fixed bounds"); OutChange = TEXT("instance bounds are recomputed every frame"); return true; }
		if (Token == TEXT("far_cull=0")) { OutWhat = FString::Printf(TEXT("%d m cull"), Config.FarM); OutChange = TEXT("instances are drawn to the map edge"); return true; }
		if (Token == TEXT("crowdfx=0")) { OutWhat = TEXT("Niagara crowd"); OutChange = FString::Printf(TEXT("nothing is drawn past %d m"), Config.FarM); return true; }
		if (Token == TEXT("far_shadow=1")) { OutWhat = TEXT("far shadow cut"); OutChange = TEXT("instances past 80 m cast shadows again"); return true; }
		bMinus = false;
		if (Token == TEXT("actor_collision=1")) { OutWhat = TEXT("actor collision"); OutChange = TEXT("near actors keep their capsule collision"); return true; }
		if (Token.StartsWith(TEXT("hydrate_m="))) { OutWhat = FString::Printf(TEXT("actors within %.0f m"), Config.HydrateM); OutChange = OutWhat; return true; }
		if (Token.StartsWith(TEXT("actor_cap="))) { OutWhat = FString::Printf(TEXT("actor cap %s"), *GroupThousands(Config.ActorCap)); OutChange = OutWhat; return true; }
		OutWhat = Token;
		OutChange = Token;
		return false;
	}

	FHudStepText StepText(const FLabConfig& Config)
	{
		const FLabConfig::FLadder Ladder = Config.Ladder();
		const FString& Id = Ladder.Id;
		const FString Hyd = FString::Printf(TEXT("%.0f m"), Config.HydrateM);
		const FString Far = FString::Printf(TEXT("%d m"), Config.FarM);
		FHudStepText T;
		T.Tag = Id == TEXT("-") ? FString() : FString::Printf(TEXT("STEP %s / 8"), *Id);
		if (Id == TEXT("1b") || Id == TEXT("1c")) { T.Tag = FString::Printf(TEXT("STEP %s"), *Id); }
		// plan.md "품질 계획" 사다리와 Scripts/full-conditions.json의 note를 평이한 영어로 옮긴 것
		if (Id == TEXT("1")) { T.Title = TEXT("All actors (baseline)"); T.Change = TEXT("every minion is a skeletal-mesh actor"); }
		else if (Id == TEXT("1b")) { T.Title = TEXT("Actors + Budget Allocator"); T.Change = TEXT("+ USkeletalMeshComponentBudgeted, 2 ms budget"); }
		else if (Id == TEXT("1c")) { T.Title = TEXT("Actors + URO"); T.Change = TEXT("+ Update Rate Optimization on every actor"); }
		else if (Id == TEXT("2")) { T.Title = TEXT("Logic in Mass, static instances"); T.Change = TEXT("+ Mass entities drawn as instanced meshes, no animation"); }
		else if (Id == TEXT("3")) { T.Title = FString::Printf(TEXT("Animated instances, cull past %s"), *Far); T.Change = FString::Printf(TEXT("+ vertex animation (VAT), nothing drawn past %s"), *Far); }
		else if (Id == TEXT("4")) { T.Title = TEXT("Actors up close, animation budget"); T.Change = FString::Printf(TEXT("+ actors within %s, Animation Budget Allocator"), *Hyd); }
		else if (Id == TEXT("5")) { T.Title = TEXT("Simulation LOD, entity pool"); T.Change = TEXT("+ far minions update less often, entities reused"); }
		else if (Id == TEXT("6")) { T.Title = TEXT("Skinned instances (ISKM)"); T.Change = TEXT("+ ISKM replaces VAT, fixed bounds"); }
		else if (Id == TEXT("7")) { T.Title = FString::Printf(TEXT("Niagara crowd past %s"), *Far); T.Change = TEXT("+ particles where minions are not drawn"); }
		else if (Id == TEXT("8")) { T.Title = TEXT("Final: quality by distance"); T.Change = FString::Printf(TEXT("+ actors within %s (cap %s), far shadows off"), *Hyd, *GroupThousands(Config.ActorCap)); }
		else { T.Title = FString::Printf(TEXT("%s / %s"), FLabConfig::StageName(Config.Stage), FLabConfig::VariantName(Config.Variant)); }

		// 빼 보기·하위 실험(최종 구성 + 인자): 첫 줄을 "Final minus: <뺀 것>"으로 바꾼다
		if (Id == TEXT("8") && !Config.Overrides.IsEmpty())
		{
			TArray<FString> Tokens;
			Config.Overrides.ParseIntoArray(Tokens, TEXT("|"));
			TArray<FString> Minus, Plus, Changes;
			for (const FString& Token : Tokens)
			{
				FString What, Change;
				bool bMinus = false;
				OverrideText(Token, Config, What, Change, bMinus);
				(bMinus ? Minus : Plus).Add(What);
				Changes.Add(Change);
			}
			T.Tag = Minus.Num() > 0 && Plus.Num() == 0 ? TEXT("ABLATION") : TEXT("SIDE TEST");
			T.Title = Minus.Num() > 0 ? TEXT("Final minus: ") + FString::Join(Minus, TEXT(", ")) : FString();
			if (Plus.Num() > 0)
			{
				T.Title += (T.Title.IsEmpty() ? TEXT("Final plus: ") : TEXT("; plus ")) + FString::Join(Plus, TEXT(", "));
			}
			T.Change = TEXT("= step 8, but ") + FString::Join(Changes, TEXT("; "));
		}
		return T;
	}

	/** 표현 한 줄: 이름, 개체 수, 거리 구간(카메라 거리, 표현 LOD와 같은 기준), 그리는 방식 */
	struct FHudRow { FString Label; int32 Count = 0; FString Range; };
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
	if (ULabCrowdSubsystem* Crowd = GetWorld() ? GetWorld()->GetSubsystem<ULabCrowdSubsystem>() : nullptr)
	{
		Crowd->bHudCounts.store(bVisible, std::memory_order_relaxed); // 숨기면 인스턴스 수 집계를 멈춘다
	}
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
	ULabCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<ULabCrowdSubsystem>();
	if (Crowd)
	{
		Crowd->bHudCounts.store(true, std::memory_order_relaxed); // 다음 프레임부터 VAT·ISKM 프로세서가 인스턴스 수를 넘긴다
	}
	const TSharedPtr<FSlateFontMeasure> Measure = FEngineFontServices::IsInitialized() ? FEngineFontServices::Get().GetFontMeasure() : nullptr;
	if (!Measure.IsValid())
	{
		return;
	}

	// 1080p 기준 배치를 화면 높이에 비례시킨다(720p에서는 0.8에서 멈춘다). 글자 크기는 2560x1440 창을 960px GIF로 줄였을 때(0.375배) 본문이 약 10px로 읽히게 정했다
	// (Docs/Worklog/Q5-hud.md "크기"). 1440p에서 S = 1.33, 본문 15pt × 1.33 = 20pt ≈ 27px → GIF에서 약 10px
	const float S = FMath::Max(Canvas->ClipY / 1080.f, 0.8f);
	const UObject* FontObject = GEngine->GetMediumFont(); // 엔진 Roboto(합성 글꼴, 런타임 캐시)
	auto MakeFont = [FontObject, S](float Points, const TCHAR* Typeface) { return FSlateFontInfo(FontObject, FMath::RoundToFloat(Points * S), FName(Typeface)); };
	const FSlateFontInfo TagFont = MakeFont(14.f, TEXT("Bold"));
	const FSlateFontInfo TitleFont = MakeFont(19.f, TEXT("Bold"));
	const FSlateFontInfo ChangeFont = MakeFont(15.f, TEXT("Regular"));
	const FSlateFontInfo BigFont = MakeFont(27.f, TEXT("Bold"));
	const FSlateFontInfo BodyFont = MakeFont(15.f, TEXT("Regular"));
	const FSlateFontInfo BodyBoldFont = MakeFont(15.f, TEXT("Bold"));
	const FSlateFontInfo CaptionFont = MakeFont(13.f, TEXT("Regular"));
	auto Width = [&Measure](const FString& Text, const FSlateFontInfo& Font) { return float(FVector2D(Measure->Measure(Text, Font)).X); };
	auto Height = [&Measure](const FSlateFontInfo& Font) { return float(FVector2D(Measure->Measure(FString(TEXT("Hg")), Font)).Y); };

	// 1. 단계: 번호와 이름, 앞 단계에서 바꾼 것
	const FHudStepText Step = StepText(Config);

	// 2. 프레임 시간과 목표
	const float FrameMs = bHasAverage ? AvgFrameMs : (WindowFrames > 0 ? float(SumFrameMs / WindowFrames) : 0.f);
	const float GameMs = bHasAverage ? AvgGameMs : (WindowFrames > 0 ? float(SumGameMs / WindowFrames) : 0.f);
	const float RenderMs = bHasAverage ? AvgRenderMs : (WindowFrames > 0 ? float(SumRenderMs / WindowFrames) : 0.f);
	const float GpuMs = bHasAverage ? AvgGpuMs : (WindowFrames > 0 ? float(SumGpuMs / WindowFrames) : 0.f);
	const FString FrameText = FString::Printf(TEXT("%.1f ms"), FrameMs);
	const FString TargetText = FString::Printf(TEXT("target %.1f ms (60 fps)"), TargetMs);
	const FString ThreadText = FString::Printf(TEXT("Game %.1f    Render %.1f    GPU %.1f ms"), GameMs, RenderMs, GpuMs);

	// 3. 표현별 개체 수와 거리 구간(카메라 거리. 표현 LOD가 카메라 기준이다, LabCrowdSubsystem.cpp)
	const int32 Horde = GameMode ? GameMode->GetHudMinions() : 0;
	const int32 Actors = GameMode ? GameMode->GetHudActors() : 0;
	const FString Hyd = FString::Printf(TEXT("%.0f"), Config.HydrateM);
	const FString Far = FString::Printf(TEXT("%d"), Config.FarM);
	const int32 MapM = FMath::RoundToInt(FLabScenario::MapRadius / 100.f);
	const bool bCull = Config.CullsFar();
	TArray<FHudRow, TInlineAllocator<5>> Rows;
	int32 Counted = 0;
	if (Config.Stage == ELabStage::M0)
	{
		Rows.Add({ TEXT("Actors"), Actors, TEXT("all distances") });
		Counted = Actors;
	}
	else
	{
		const bool bHydrate = Config.UsesHydration();
		if (bHydrate)
		{
			Rows.Add({ TEXT("Actors"), Actors, FString::Printf(TEXT("0–%s m"), *Hyd) });
			Counted += Actors;
		}
		// M1은 엔진 ISM 프로세서라 밴드 집계가 없다. 거리와 무관하게 전부 인스턴스다
		const int32 Instances = Config.Stage == ELabStage::M1 ? Horde : (Crowd ? Crowd->HudInstances.load(std::memory_order_relaxed) : 0);
		const TCHAR* How = Config.Stage == ELabStage::M1 ? TEXT("static mesh") : Config.UsesIskm() ? TEXT("ISKM") : TEXT("VAT");
		const FString From = bHydrate ? Hyd : FString(TEXT("0"));
		const FString InstRange = !bCull ? (bHydrate ? FString::Printf(TEXT("%s m+"), *From) : FString(TEXT("all distances")))
			: FString::Printf(TEXT("%s–%s m"), *From, *Far);
		Rows.Add({ TEXT("Instances"), Instances, FString::Printf(TEXT("%s  · %s"), *InstRange, How) });
		Counted += Instances;
		if (Config.UsesCrowdFxEntity() && bCull)
		{
			const int32 Particles = Crowd ? Crowd->CrowdRecsLast : 0;
			Rows.Add({ TEXT("Particles"), Particles, FString::Printf(TEXT("%s–%d m  · Niagara"), *Far, MapM) });
			Counted += Particles;
		}
		if (bCull)
		{
			// 합이 총수와 어긋나는 몫: 그리는 끝 밖에서 입자가 없거나(7단계 전), 멈춘 개체(피격·사망)라 입자로도 그리지 않는다
			const int32 NotDrawn = FMath::Max(Horde - Counted, 0);
			Rows.Add({ TEXT("Not drawn"), NotDrawn, Config.UsesCrowdFxEntity() ? FString::Printf(TEXT("past %s m, hit or dying"), *Far) : FString::Printf(TEXT("past %s m"), *Far) });
		}
	}
	// 맵 밖 독립 입자는 개체가 아니라 배경이라 수를 세지 않는다(Docs/Worklog/Q5-hud.md "배경 입자")
	FString BackdropText;
	if (Config.bCrowdFx && Config.bCrowdFxOuter)
	{
		BackdropText = FString::Printf(TEXT("+ backdrop particles %.0f–%s m (scenery, not counted)"), Config.CrowdFxInnerCm() / 100.f,
			*GroupThousands(FMath::RoundToInt(FLabCrowdFx::OuterRadius / 100.f)));
	}
	const FString HordeText = FString::Printf(TEXT("Horde %s"), *GroupThousands(Horde));
	const FString HordeNote = TEXT("distance from camera");

	// 4. 맨 아래 작은 줄: 내부 변형 이름·단계·프리셋·빌드, 기본값과 다르게 준 인자
#if UE_BUILD_TEST
	const TCHAR* BuildName = TEXT("Test build");
#elif UE_BUILD_SHIPPING
	const TCHAR* BuildName = TEXT("Shipping build");
#else
	const TCHAR* BuildName = TEXT("Development build");
#endif
	FString FooterText = FString::Printf(TEXT("%s · %s · P%d · %d %s · %s"), FLabConfig::VariantName(Config.Variant), FLabConfig::StageName(Config.Stage),
		Config.Preset, Config.Types, Config.Types == 1 ? TEXT("type") : TEXT("types"), BuildName);
	if (!Config.Overrides.IsEmpty())
	{
		FooterText += TEXT(" · ") + Config.Overrides.Replace(TEXT("|"), TEXT(", "));
	}
	const FString GraphCaption = FString::Printf(TEXT("frame time, last %.0f s"), GraphSeconds);
	const FString GraphTarget = FString::Printf(TEXT("%.1f ms"), TargetMs);

	// 판 크기
	const float Pad = 16.f * S;
	const float Gap = 6.f * S;
	const float Section = 14.f * S;
	const float TagGap = 12.f * S;
	const float Indent = 18.f * S;
	const float ColGap = 14.f * S;
	const float TagW = Step.Tag.IsEmpty() ? 0.f : Width(Step.Tag, TagFont) + TagGap;
	float LabelW = 0.f, CountW = 0.f, RangeW = 0.f;
	for (const FHudRow& Row : Rows)
	{
		LabelW = FMath::Max(LabelW, Width(Row.Label, BodyFont));
		CountW = FMath::Max(CountW, Width(GroupThousands(Row.Count), BodyBoldFont));
		RangeW = FMath::Max(RangeW, Width(Row.Range, BodyFont));
	}
	CountW = FMath::Max(CountW, Width(TEXT("00,000"), BodyBoldFont)); // 수가 바뀌어도 열이 흔들리지 않게
	float InnerW = 380.f * S;
	InnerW = FMath::Max(InnerW, TagW + Width(Step.Title, TitleFont));
	InnerW = FMath::Max(InnerW, Width(Step.Change, ChangeFont));
	InnerW = FMath::Max(InnerW, Width(FrameText, BigFont) + ColGap + Width(TargetText, BodyFont));
	InnerW = FMath::Max(InnerW, Width(ThreadText, BodyFont));
	InnerW = FMath::Max(InnerW, Width(HordeText, BodyBoldFont) + ColGap + Width(HordeNote, CaptionFont));
	InnerW = FMath::Max(InnerW, Indent + LabelW + ColGap + CountW + ColGap + RangeW);
	InnerW = FMath::Max(InnerW, Width(BackdropText, CaptionFont));
	InnerW = FMath::Max(InnerW, Width(FooterText, CaptionFont));
	const float TitleH = Height(TitleFont), ChangeH = Height(ChangeFont), BigH = Height(BigFont), BodyH = Height(BodyFont), CaptionH = Height(CaptionFont);
	const float GraphH = 72.f * S;
	float PanelH = Pad + TitleH + 2.f * S + ChangeH + Section + BigH + BodyH + Section + BodyH + Gap * 0.5f + Rows.Num() * BodyH;
	if (!BackdropText.IsEmpty()) { PanelH += 2.f * S + CaptionH; }
	PanelH += Section + GraphH + 3.f * S + CaptionH + Gap + CaptionH + Pad;
	const float X0 = 20.f * S;
	const float Y0 = 20.f * S;
	LabRect(Canvas, X0, Y0, InnerW + 2.f * Pad, PanelH, PanelColor);

	// 글
	const float X = X0 + Pad;
	float Y = Y0 + Pad;
	if (!Step.Tag.IsEmpty())
	{
		LabText(Canvas, Step.Tag, TagFont, AccentColor, X, Y + (TitleH - Height(TagFont)) * 0.7f, S); // 제목 밑선에 맞춘다
	}
	LabText(Canvas, Step.Title, TitleFont, TextColor, X + TagW, Y, S);
	Y += TitleH + 2.f * S;
	LabText(Canvas, Step.Change, ChangeFont, DimColor, X, Y, S);
	Y += ChangeH + Section;

	// 프레임 시간: 목표 안이면 초록, 30fps까지 주황, 넘으면 빨강(그래프 막대와 같은 규칙)
	LabText(Canvas, FrameText, BigFont, BarColor(FrameMs), X, Y, S);
	LabText(Canvas, TargetText, BodyFont, DimColor, X + Width(FrameText, BigFont) + ColGap, Y + (BigH - BodyH) * 0.8f, S);
	Y += BigH;
	LabText(Canvas, ThreadText, BodyFont, TextColor, X, Y, S);
	Y += BodyH + Section;

	LabText(Canvas, HordeText, BodyBoldFont, TextColor, X, Y, S);
	LabText(Canvas, HordeNote, CaptionFont, DimColor, X + Width(HordeText, BodyBoldFont) + ColGap, Y + (BodyH - CaptionH) * 0.8f, S);
	Y += BodyH + Gap * 0.5f;
	for (const FHudRow& Row : Rows)
	{
		const bool bNotDrawn = Row.Label == TEXT("Not drawn");
		const FString Count = GroupThousands(Row.Count);
		const float RowX = X + Indent;
		LabText(Canvas, Row.Label, BodyFont, bNotDrawn ? DimColor : TextColor, RowX, Y, S);
		LabText(Canvas, Count, BodyBoldFont, bNotDrawn ? DimColor : TextColor, RowX + LabelW + ColGap + CountW - Width(Count, BodyBoldFont), Y, S); // 수는 오른쪽 맞춤
		LabText(Canvas, Row.Range, BodyFont, DimColor, RowX + LabelW + ColGap + CountW + ColGap, Y, S);
		Y += BodyH;
	}
	if (!BackdropText.IsEmpty())
	{
		Y += 2.f * S;
		LabText(Canvas, BackdropText, CaptionFont, DimColor, X, Y, S);
		Y += CaptionH;
	}
	Y += Section;

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
	LabText(Canvas, GraphTarget, CaptionFont, DimColor, GX + GW - Width(GraphTarget, CaptionFont) - 4.f * S, TargetY - CaptionH - 1.f * S, S);
	Y += GraphH + 3.f * S;
	LabText(Canvas, GraphCaption, CaptionFont, DimColor, X, Y, S);
	Y += CaptionH + Gap;
	LabText(Canvas, FooterText, CaptionFont, DimColor, X, Y, S);
}
