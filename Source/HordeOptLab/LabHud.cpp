#include "LabHud.h"
#include "LabConfig.h"
#include "LabCrowdSubsystem.h"
#include "LabGameMode.h"
#include "LabOrb.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
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

void ALabHud::DrawOrbPanel()
{
	if (!OrbForPanel.IsValid())
	{
		for (TActorIterator<ALabOrb> It(GetWorld()); It; ++It)
		{
			OrbForPanel = *It;
			break;
		}
		if (!OrbForPanel.IsValid())
		{
			return;
		}
	}
	const TSharedPtr<FSlateFontMeasure> Measure = FEngineFontServices::IsInitialized() ? FEngineFontServices::Get().GetFontMeasure() : nullptr;
	if (!Measure.IsValid())
	{
		return;
	}

	// 값: 오브 액터의 실제 위치와 프레임 사이 이동. 속도는 0.25초 창마다 한 번 바꿔 숫자가 떨리지 않게 한다
	const FVector Pos = OrbForPanel->GetActorLocation();
	const double Now = GetWorld()->GetTimeSeconds();
	if (OrbPanelWindowTime < 0.)
	{
		OrbPanelWindowTime = Now;
		OrbPanelWindowStart = Pos;
		OrbPanelLast = Pos;
	}
	OrbPanelWindowPath += FVector::Dist2D(Pos, OrbPanelLast);
	OrbPanelLast = Pos;
	if (Now - OrbPanelWindowTime >= 0.25)
	{
		OrbPanelSpeed = float(OrbPanelWindowPath / (Now - OrbPanelWindowTime));
		OrbPanelDir = (Pos - OrbPanelWindowStart).GetSafeNormal2D();
		OrbPanelWindowTime = Now;
		OrbPanelWindowStart = Pos;
		OrbPanelWindowPath = 0.;
	}
	const bool bMoving = OrbPanelSpeed >= 5.f && !OrbPanelDir.IsNearlyZero(); // 0.05m/s 아래는 정지로 적는다(표시 자릿수의 절반)

	// 글자: 1440p에서 값 27pt, 낱말 20pt. 720p에서는 0.8배에서 멈춘다(상태 오버레이와 같은 규칙)
	const float S = FMath::Max(Canvas->ClipY / 1080.f, 0.8f);
	const UObject* FontObject = GEngine->GetMediumFont();
	const FSlateFontInfo LabelFont(FontObject, FMath::RoundToFloat(15.f * S), FName(TEXT("Regular")));
	const FSlateFontInfo ValueFont(FontObject, FMath::RoundToFloat(20.f * S), FName(TEXT("Bold")));
	auto Width = [&Measure](const FString& Text, const FSlateFontInfo& Font) { return float(FVector2D(Measure->Measure(Text, Font)).X); };
	const float ValueH = float(FVector2D(Measure->Measure(FString(TEXT("Hg0")), ValueFont)).Y);
	const float LabelH = float(FVector2D(Measure->Measure(FString(TEXT("Hg0")), LabelFont)).Y);

	// 부호는 늘 적고 소수 한 자리. 숫자 칸은 가장 넓은 값의 폭으로 고정하고 오른쪽에 맞춰 자릿수가 바뀌어도 흔들리지 않는다
	auto Signed = [](double Meters)
	{
		const double R = FMath::RoundToDouble(Meters * 10.) / 10.;
		return FString::Printf(TEXT("%s%.1f"), R < 0. ? TEXT("−") : TEXT("+"), FMath::Abs(R));
	};
	const FString XText = Signed(Pos.X / 100.);
	const FString YText = Signed(Pos.Y / 100.);
	const FString SpeedText = FString::Printf(TEXT("%.1f"), OrbPanelSpeed / 100.f);
	const FString PosLabel = TEXT("위치");
	const FString SpeedLabel = TEXT("속도");
	const FString StopText = TEXT("정지");
	const FString TitleText = TEXT("오브");
	const FString ArrowNote = TEXT("화살표: 위 +X, 오른쪽 +Y"); // 진행 방향 화살표의 기준(월드 축). 카메라가 돌아도 바뀌지 않는다
	const FSlateFontInfo TitleFont(FontObject, FMath::RoundToFloat(14.f * S), FName(TEXT("Bold")));   // 상태 오버레이의 머리 글자와 같은 크기
	const FSlateFontInfo NoteFont(FontObject, FMath::RoundToFloat(13.f * S), FName(TEXT("Regular"))); // 상태 오버레이의 작은 줄과 같은 크기
	const float TitleH = float(FVector2D(Measure->Measure(FString(TEXT("Hg0")), TitleFont)).Y);
	const float Gap = 10.f * S;
	const float Pad = 16.f * S; // 상태 오버레이와 같은 안쪽 여백
	const float LabelW = FMath::Max(Width(PosLabel, LabelFont), Width(SpeedLabel, LabelFont)) + Gap * 1.6f;
	const float AxisW = FMath::Max(Width(TEXT("X"), LabelFont), Width(TEXT("Y"), LabelFont)) + Gap * 0.6f;
	const float NumW = Width(TEXT("+000.0"), ValueFont);
	const float UnitW = Width(TEXT(" m"), LabelFont);
	const float SpeedNumW = Width(TEXT("00.0"), ValueFont);
	const float ArrowW = ValueH * 1.1f;
	const float Row1W = LabelW + AxisW + NumW + UnitW + Gap * 2.f + AxisW + NumW + UnitW;
	const float InnerW = FMath::Max(Row1W, Width(TitleText, TitleFont) + Gap * 2.f + Width(ArrowNote, NoteFont));
	const float PanelW = InnerW + 2.f * Pad;
	const float RowH = ValueH + 2.f * S;
	const float PanelH = TitleH + 6.f * S + 2.f * RowH + 2.f * Pad;

	// 자리: 화면 오른쪽 위에 고정한다. 왼쪽 위 상태 오버레이와 같은 바깥 여백(20 × S)과 바탕색을 써서 두 판이 짝을 이루고 서로 겹치지 않는다
	const float PX = Canvas->ClipX - 20.f * S - PanelW;
	const float PY = 20.f * S;
	LabRect(Canvas, PX, PY, PanelW, PanelH, PanelColor);

	auto Baseline = [ValueH, LabelH](float RowY) { return RowY + (ValueH - LabelH) * 0.75f; }; // 작은 글자를 큰 글자의 밑선 가까이에 맞춘다
	float X = PX + Pad;
	float Y = PY + Pad;
	LabText(Canvas, TitleText, TitleFont, AccentColor, X, Y, S);
	LabText(Canvas, ArrowNote, NoteFont, DimColor, PX + PanelW - Pad - Width(ArrowNote, NoteFont), Y + (TitleH - float(FVector2D(Measure->Measure(FString(TEXT("Hg0")), NoteFont)).Y)) * 0.75f, S);
	Y += TitleH + 6.f * S;
	LabText(Canvas, PosLabel, LabelFont, DimColor, X, Baseline(Y), S);
	X += LabelW;
	const FString* Values[2] = { &XText, &YText };
	const TCHAR* Axes[2] = { TEXT("X"), TEXT("Y") };
	for (int32 i = 0; i < 2; ++i)
	{
		LabText(Canvas, Axes[i], LabelFont, AccentColor, X, Baseline(Y), S);
		X += AxisW;
		LabText(Canvas, *Values[i], ValueFont, TextColor, X + NumW - Width(*Values[i], ValueFont), Y, S);
		X += NumW;
		LabText(Canvas, TEXT(" m"), LabelFont, DimColor, X, Baseline(Y), S);
		X += UnitW + Gap * 2.f;
	}

	X = PX + Pad;
	Y += RowH;
	LabText(Canvas, SpeedLabel, LabelFont, DimColor, X, Baseline(Y), S);
	X += LabelW;
	// 방향 화살표: 월드 기준이다(위에서 내려다본 지도처럼 화면 위가 +X, 오른쪽이 +Y). 글리프가 아니라 선으로 그려 글꼴에 기대지 않는다
	auto DrawArrow = [this, S, ArrowW, ValueH](float AtX, float AtY, const FVector& WorldDir)
	{
		FVector2D Dir(WorldDir.Y, -WorldDir.X);
		if (!Dir.Normalize())
		{
			return;
		}
		const FVector2D Center(AtX + ArrowW * 0.5f, AtY + ValueH * 0.5f);
		const float Half = ArrowW * 0.45f;
		const FVector2D Tip = Center + Dir * Half;
		const FVector2D Tail = Center - Dir * Half;
		const FVector2D Side(-Dir.Y, Dir.X);
		const float Thick = FMath::Max(2.f, FMath::RoundToFloat(2.5f * S));
		const FVector2D Ends[3] = { Tail, Tip - Dir * Half * 0.8f + Side * Half * 0.55f, Tip - Dir * Half * 0.8f - Side * Half * 0.55f };
		for (const FVector2D& End : Ends)
		{
			FCanvasLineItem Item(End, Tip);
			Item.SetColor(AccentColor);
			Item.LineThickness = Thick;
			Canvas->DrawItem(Item);
		}
	};
	if (!bMoving)
	{
		LabText(Canvas, StopText, ValueFont, TextColor, X, Y, S);
	}
	else
	{
		LabText(Canvas, SpeedText, ValueFont, TextColor, X + SpeedNumW - Width(SpeedText, ValueFont), Y, S);
		X += SpeedNumW;
		LabText(Canvas, TEXT(" m/s"), LabelFont, DimColor, X, Baseline(Y), S);
		X += Width(TEXT(" m/s"), LabelFont) + Gap * 1.5f;
		DrawArrow(X, Y, OrbPanelDir);
	}

	// 카메라 판: 오브 판 바로 아래, 같은 폭과 양식. 카메라가 오브 둘레를 도는 각속도와 한 바퀴 시간, 지금 보는 방향을 적는다.
	// 값은 설정이 아니라 실제 카메라에서 구한다: 오브에서 본 카메라의 방위각(수평 거리가 1m 아래면 카메라 요)의 프레임 사이 변화를 0.5초 창으로 모은다.
	const APlayerCameraManager* Cam = PlayerOwner ? PlayerOwner->PlayerCameraManager.Get() : nullptr;
	if (!Cam)
	{
		return;
	}
	const FVector CamOffset = Cam->GetCameraLocation() - Pos;
	const FRotator CamRot = Cam->GetCameraRotation();
	const float CamAngle = CamOffset.SizeSquared2D() > FMath::Square(100.) ? FMath::RadiansToDegrees(FMath::Atan2(CamOffset.Y, CamOffset.X)) : CamRot.Yaw;
	if (CamPanelWindowTime < 0.)
	{
		CamPanelWindowTime = Now;
		CamPanelAngleLast = CamAngle;
	}
	CamPanelWindowTurn += FMath::FindDeltaAngleDegrees(CamPanelAngleLast, CamAngle);
	CamPanelAngleLast = CamAngle;
	if (Now - CamPanelWindowTime >= 0.5)
	{
		CamPanelRate = FMath::Abs(float(CamPanelWindowTurn / (Now - CamPanelWindowTime)));
		CamPanelWindowTime = Now;
		CamPanelWindowTurn = 0.;
	}
	const bool bTurning = CamPanelRate >= 0.05f; // 0.05도/초 아래는 고정으로 적는다
	const FString CamTitle = TEXT("카메라");
	const FString TurnLabel = TEXT("회전");
	const FString LapLabel = TEXT("한 바퀴");
	const FString LookLabel = TEXT("보는 방향");
	const FString FixedText = TEXT("고정");
	const FString RateText = FString::Printf(TEXT("%.1f"), CamPanelRate);
	const FString RpmText = FString::Printf(TEXT("(%.1f rpm)"), CamPanelRate / 6.f);
	const FString LapText = bTurning ? FString::Printf(TEXT("%.0f"), 360.f / CamPanelRate) : FString(TEXT("−"));
	const float CamLabelW = FMath::Max(Width(TurnLabel, LabelFont), Width(LapLabel, LabelFont)) + Gap * 1.6f;
	const float CamNumW = Width(TEXT("000.0"), ValueFont);
	const float CY0 = PY + PanelH + 8.f * S;
	LabRect(Canvas, PX, CY0, PanelW, PanelH, PanelColor);
	X = PX + Pad;
	Y = CY0 + Pad;
	LabText(Canvas, CamTitle, TitleFont, AccentColor, X, Y, S);
	Y += TitleH + 6.f * S;
	LabText(Canvas, TurnLabel, LabelFont, DimColor, X, Baseline(Y), S);
	X += CamLabelW;
	if (!bTurning)
	{
		LabText(Canvas, FixedText, ValueFont, TextColor, X, Y, S);
	}
	else
	{
		LabText(Canvas, RateText, ValueFont, TextColor, X + CamNumW - Width(RateText, ValueFont), Y, S);
		X += CamNumW;
		LabText(Canvas, TEXT(" °/s"), LabelFont, DimColor, X, Baseline(Y), S);
		X += Width(TEXT(" °/s"), LabelFont) + Gap * 1.5f;
		LabText(Canvas, RpmText, LabelFont, DimColor, X, Baseline(Y), S);
	}
	X = PX + Pad;
	Y += RowH;
	LabText(Canvas, LapLabel, LabelFont, DimColor, X, Baseline(Y), S);
	X += CamLabelW;
	LabText(Canvas, LapText, ValueFont, TextColor, X + CamNumW - Width(LapText, ValueFont), Y, S);
	X += CamNumW;
	LabText(Canvas, TEXT(" s"), LabelFont, DimColor, X, Baseline(Y), S);
	// 보는 방향은 판 오른쪽 끝에 맞춘다
	const float LookX = PX + PanelW - Pad - ArrowW - Gap * 0.8f - Width(LookLabel, LabelFont);
	LabText(Canvas, LookLabel, LabelFont, DimColor, LookX, Baseline(Y), S);
	DrawArrow(PX + PanelW - Pad - ArrowW, Y, CamRot.Vector().GetSafeNormal2D());
}

void ALabHud::DrawHUD()
{
	Super::DrawHUD();
	if (Canvas && GEngine && FLabConfig::Get().Orbit.bAids)
	{
		DrawOrbPanel(); // Spike1: 상태 오버레이와 따로 켠다(측정 실행에서도 그린다)
	}
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
