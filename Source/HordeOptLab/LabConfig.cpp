#include "LabConfig.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

const FLabConfig& FLabConfig::Get()
{
	static const FLabConfig Config = []
	{
		FLabConfig C;
		const TCHAR* Cmd = FCommandLine::Get();
		FParse::Value(Cmd, TEXT("-LabN="), C.N);
		FParse::Value(Cmd, TEXT("-LabPreset="), C.Preset);
		FParse::Value(Cmd, TEXT("-LabSeed="), C.Seed);
		FParse::Value(Cmd, TEXT("-LabHealthFixed="), C.HealthFixed);
		FParse::Value(Cmd, TEXT("-LabLabel="), C.Label);
		FParse::Value(Cmd, TEXT("-LabDuration="), C.DurationSec);
		FParse::Value(Cmd, TEXT("-LabShotAt="), C.ShotAtSec);
		if (FParse::Value(Cmd, TEXT("-LabHydrateFix="), C.HydrateFix))
		{
			static const int32 LevelMasks[5] = { 0, 1, 3, 7, 31 };
			C.HydrateFixMask = LevelMasks[FMath::Clamp(C.HydrateFix, 0, 4)];
		}
		FParse::Value(Cmd, TEXT("-LabHydrateFixMask="), C.HydrateFixMask);
		FParse::Value(Cmd, TEXT("-LabFullyParallel="), C.FullyParallel);
		FParse::Value(Cmd, TEXT("-LabTypes="), C.Types);
		C.Types = FMath::Clamp(C.Types, 1, 2);
		FParse::Value(Cmd, TEXT("-LabFarM="), C.FarM);
		C.FarM = FMath::Max(C.FarM, 200);
		C.bBench = FParse::Param(Cmd, TEXT("LabBench"));
		C.bHud = FParse::Param(Cmd, TEXT("LabHud")); // Q5: -LabBench와 함께여도 켠다(촬영 실행). CSV 메타데이터 LabHud로 남는다

		FString Stage;
		if (FParse::Value(Cmd, TEXT("-LabStage="), Stage))
		{
			if (Stage.Equals(TEXT("M0"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M0; }
			else if (Stage.Equals(TEXT("M1"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M1; }
			else if (Stage.Equals(TEXT("M2"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M2; }
			else if (Stage.Equals(TEXT("M3"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M3; }
			else if (Stage.Equals(TEXT("M4"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M4; }
			else if (Stage.Equals(TEXT("M5"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M5; }
			else { C.bStageValid = false; }
		}

		FString Variant;
		if (FParse::Value(Cmd, TEXT("-LabVariant="), Variant))
		{
			if (Variant == TEXT("static") || Variant == TEXT("m0static")) { C.Variant = ELabVariant::Static; } // m0static: M3b 묶음 1 라벨
			else if (Variant == TEXT("budget")) { C.Variant = ELabVariant::Budget; }
			else if (Variant == TEXT("uro")) { C.Variant = ELabVariant::Uro; }
			else if (Variant == TEXT("pawn")) { C.Variant = ELabVariant::Pawn; }
			else if (Variant == TEXT("staticpawn")) { C.Variant = ELabVariant::StaticPawn; }
			else if (Variant == TEXT("m0full")) { C.Variant = ELabVariant::Full; } // M3b 묶음 1 라벨
			else if (Variant == TEXT("mass") || Variant == TEXT("noanim") || Variant == TEXT("m3base") || Variant == TEXT("m3colbase") || Variant == TEXT("m3hydbase")
				|| Variant == TEXT("m1mass") || Variant == TEXT("m2mass") || Variant == TEXT("m3mass") || Variant == TEXT("m4mass")) { C.Variant = ELabVariant::Mass; } // noanim: M2 묶음에서 잰 M1, m3base·m3colbase·m3hydbase: E3 하위 실험 묶음(5, 6, 7)에서 다시 잰 M3 대표, m1mass·m2mass·m3mass: M3b 묶음 1, m4mass: M4 대표. 이름만 다르다
			else if (Variant == TEXT("m4off")) { C.Variant = ELabVariant::M4Off; }
			else if (Variant == TEXT("m4m25l50")) { C.Variant = ELabVariant::M4M25L50; }
			else if (Variant == TEXT("m4m25l100")) { C.Variant = ELabVariant::M4M25L100; }
			else if (Variant == TEXT("m4m50l50")) { C.Variant = ELabVariant::M4M50L50; }
			else if (Variant == TEXT("m4avoid")) { C.Variant = ELabVariant::M4Avoid; }
			else if (Variant == TEXT("m4offmove")) { C.Variant = ELabVariant::M4OffMove; }
			else if (Variant == TEXT("m4offpool")) { C.Variant = ELabVariant::M4OffPool; }
			else if (Variant == TEXT("m5iskm") || Variant == TEXT("m5mass")) { C.Variant = ELabVariant::M5Iskm; }
			else if (Variant == TEXT("m5destroy")) { C.Variant = ELabVariant::M5Destroy; }
			else if (Variant == TEXT("m5blend")) { C.Variant = ELabVariant::M5Blend; }
			else if (Variant == TEXT("m5minss")) { C.Variant = ELabVariant::M5MinSS; }
			else if (Variant == TEXT("m5bounds")) { C.Variant = ELabVariant::M5Bounds; }
			else if (Variant == TEXT("q2crowd")) { C.Variant = ELabVariant::Q2Crowd; }
			else if (Variant == TEXT("q4final")) { C.Variant = ELabVariant::Q4Final; }
			else if (Variant == TEXT("m3qold")) { C.Variant = ELabVariant::M3QOld; }
			else if (Variant == TEXT("m3syncall")) { C.Variant = ELabVariant::M3SyncAll; }
			else if (Variant == TEXT("m3vis0")) { C.Variant = ELabVariant::M3Vis0; }
			else if (Variant == TEXT("m3vis15")) { C.Variant = ELabVariant::M3Vis15; }
			else if (Variant == TEXT("m3vis30")) { C.Variant = ELabVariant::M3Vis30; }
			else if (Variant == TEXT("m3hys20")) { C.Variant = ELabVariant::M3Hys20; }
			else if (Variant == TEXT("m3lock0")) { C.Variant = ELabVariant::M3Lock0; }
			else if (Variant == TEXT("m3order")) { C.Variant = ELabVariant::M3Order; }
			else if (Variant == TEXT("m3sig")) { C.Variant = ELabVariant::M3Sig; }
			else if (Variant == TEXT("massst")) { C.Variant = ELabVariant::MassSt; }
			else if (Variant == TEXT("massav")) { C.Variant = ELabVariant::MassAv; }
			else if (Variant == TEXT("massdirlo")) { C.Variant = ELabVariant::MassDirLo; }
			else if (Variant == TEXT("massdirhi")) { C.Variant = ELabVariant::MassDirHi; }
			else if (Variant == TEXT("massevtlo")) { C.Variant = ELabVariant::MassEvtLo; }
			else if (Variant == TEXT("massevthi")) { C.Variant = ELabVariant::MassEvtHi; }
			else if (Variant == TEXT("vatvert")) { C.Variant = ELabVariant::VatVert; }
			else if (Variant == TEXT("vatcull") || Variant == TEXT("m2off")) { C.Variant = ELabVariant::VatCull; } // m2off: M3 묶음에서 잰 M2 + 200m 밖 Off(M3의 비교 상대). 이름만 다르다
			else if (Variant == TEXT("vatcullvis")) { C.Variant = ELabVariant::VatCullVis; }
			else if (Variant == TEXT("vatdie0")) { C.Variant = ELabVariant::VatDie0; }
			else if (Variant == TEXT("vatdie1k")) { C.Variant = ELabVariant::VatDie1k; }
			else if (Variant == TEXT("kill0")) { C.Variant = ELabVariant::Kill0; }
			else if (Variant == TEXT("kill1k")) { C.Variant = ELabVariant::Kill1k; }
			else if (Variant == TEXT("m3uro")) { C.Variant = ELabVariant::M3Uro; }
			else if (Variant == TEXT("m3budgetdist")) { C.Variant = ELabVariant::M3BudgetDist; }
			else if (Variant == TEXT("m3budgetsig")) { C.Variant = ELabVariant::M3BudgetSig; }
			else if (Variant == TEXT("m3lowres")) { C.Variant = ELabVariant::M3LowRes; }
			else if (Variant == TEXT("m3noactor")) { C.Variant = ELabVariant::M3NoActor; }
			else if (Variant == TEXT("m3colquery")) { C.Variant = ELabVariant::M3ColQuery; }
			else if (Variant == TEXT("m3nocol")) { C.Variant = ELabVariant::M3NoCol; }
			else if (Variant == TEXT("m3hyd15")) { C.Variant = ELabVariant::M3Hyd15; }
			else if (Variant == TEXT("m3hyd50")) { C.Variant = ELabVariant::M3Hyd50; }
		}
		// Budget Allocator(중요도 = 화면 크기) 기본값(2026-11-11 사용자 결정 A안): M3·M4·M5 단계는 켜고 그 밖은 끈다. 변형 m3sig는 항상 켠다(이름 호환).
		// 켜지 않는 변형은 비교 상대라 Budget 없이 또는 자기 방식으로 남는다: m3mass·m3base·m3colbase·m3hydbase(M3 Budget 없음 기준 라벨),
		// m3uro(URO)·m3budgetdist·m3budgetsig(자기 중요도 방식)·m3lowres(저품질 액터가 URO)·m3noactor(액터 없음), M3 하위 실험·진단 변형(그 기준점이 m3mass다).
		// M0 계열(budget·uro 등)은 M3 이전 단계라 어차피 꺼진다.
		// Q5: 변형 기본값과 다르게 준 인자를 모은다(LabOverrides, HUD). 기본값과 같은 값을 준 인자는 넣지 않는다.
		TArray<FString> Overrides;
		auto NoteOverride = [&Overrides](const FString& What) { Overrides.Add(What); };
		{
			const bool bPlainMassLabel = Variant == TEXT("m3mass") || Variant == TEXT("m3base") || Variant == TEXT("m3colbase") || Variant == TEXT("m3hydbase");
			bool bOptOut = bPlainMassLabel;
			switch (C.Variant)
			{
			case ELabVariant::M3Uro: case ELabVariant::M3BudgetDist: case ELabVariant::M3BudgetSig: case ELabVariant::M3LowRes: case ELabVariant::M3NoActor:
			case ELabVariant::M3ColQuery: case ELabVariant::M3NoCol: case ELabVariant::M3Hyd15: case ELabVariant::M3Hyd50:
			case ELabVariant::M3QOld: case ELabVariant::M3SyncAll: case ELabVariant::M3Vis0: case ELabVariant::M3Vis15: case ELabVariant::M3Vis30:
			case ELabVariant::M3Hys20: case ELabVariant::M3Lock0: case ELabVariant::M3Order:
				bOptOut = true;
				break;
			default:
				break;
			}
			C.bScreenSigBudget = C.Variant == ELabVariant::M3Sig || (C.Stage >= ELabStage::M3 && !bOptOut);
			int32 ScreenSig = -1;
			if (FParse::Value(Cmd, TEXT("-LabScreenSigBudget="), ScreenSig) && ScreenSig >= 0)
			{
				if ((ScreenSig != 0) != C.bScreenSigBudget) { NoteOverride(FString::Printf(TEXT("screensig_budget=%d"), ScreenSig != 0 ? 1 : 0)); }
				C.bScreenSigBudget = ScreenSig != 0;
			}
		}
		// M5 엔티티 풀링 기본값(plan.md "M5" 결정 1): M5 단계는 켜고 다른 단계는 끈다. m4offpool은 어느 단계에서든 켜고, m4offmove·m5destroy는 끈다.
		C.bEntityPool = C.Variant == ELabVariant::M4OffPool
			|| (C.Stage == ELabStage::M5 && C.Variant != ELabVariant::M4OffMove && C.Variant != ELabVariant::M5Destroy);
		int32 Pool = -1;
		if (FParse::Value(Cmd, TEXT("-LabEntityPool="), Pool) && Pool >= 0)
		{
			if ((Pool != 0) != C.bEntityPool) { NoteOverride(FString::Printf(TEXT("pool=%d"), Pool != 0 ? 1 : 0)); }
			C.bEntityPool = Pool != 0;
		}
		// Q2 Niagara 배경 무리 기본값: 변형 q2crowd만 켠다. run-bench.ps1의 ValidateSet에 q2crowd가 들어가기 전에는 -Variant m5bounds -ExtraArgs "-LabCrowdFx=1"로 같은 구성을 고른다.
		C.bCrowdFx = C.Variant == ELabVariant::Q2Crowd || C.Variant == ELabVariant::Q4Final; // q4final = q2crowd + Q4 품질 값
		int32 CrowdFx = -1;
		if (FParse::Value(Cmd, TEXT("-LabCrowdFx="), CrowdFx) && CrowdFx >= 0)
		{
			if ((CrowdFx != 0) != C.bCrowdFx) { NoteOverride(FString::Printf(TEXT("crowdfx=%d"), CrowdFx != 0 ? 1 : 0)); }
			C.bCrowdFx = CrowdFx != 0;
		}
		// Q2b: 엔티티 구동 입자(기본 1). 0이면 Q2 3단위의 옛 방식. 배경 무리를 켰을 때만 기본값과 다른 인자로 적는다
		int32 CrowdFxEntity = -1;
		if (FParse::Value(Cmd, TEXT("-LabCrowdFxEntity="), CrowdFxEntity) && CrowdFxEntity >= 0)
		{
			C.bCrowdFxEntity = CrowdFxEntity != 0;
			if (!C.bCrowdFxEntity && C.bCrowdFx) { NoteOverride(TEXT("crowdfx_entity=0")); }
		}
		// Q5b: 바깥 독립 입자(기본 1). 0이면 독립 이미터 스폰율 0(엔티티 구동 입자만 남는다). 배경 무리를 켰을 때만 기본값과 다른 인자로 적는다
		int32 CrowdFxOuter = -1;
		if (FParse::Value(Cmd, TEXT("-LabCrowdFxOuter="), CrowdFxOuter) && CrowdFxOuter >= 0)
		{
			C.bCrowdFxOuter = CrowdFxOuter != 0;
			if (!C.bCrowdFxOuter && C.bCrowdFx) { NoteOverride(TEXT("crowdfx_outer=0")); }
		}
#if !UE_BUILD_TEST
		FParse::Value(Cmd, TEXT("-LabCrowdFxInnerM="), C.CrowdFxInnerOverrideM);
#endif
		// Q3 빼 보기 스위치(끄기 전용): 0을 줄 때만 그 기법을 뺀다. 다른 값은 변형·단계 기본값 그대로다(LabConfig.h).
		auto OffSwitch = [Cmd](const TCHAR* Key)
		{
			int32 V = -1;
			return FParse::Value(Cmd, Key, V) && V == 0;
		};
		C.bNoHydrate = OffSwitch(TEXT("-LabHydrate="));
		C.bNoSimLod = OffSwitch(TEXT("-LabSimLod="));
		C.bNoIskm = OffSwitch(TEXT("-LabIskm="));
		C.bNoFixedBounds = OffSwitch(TEXT("-LabFixedBounds="));
		C.bNoFarCull = OffSwitch(TEXT("-LabFarCull="));
		if (C.bNoHydrate) { NoteOverride(TEXT("hydrate=0")); }
		if (C.bNoSimLod) { NoteOverride(TEXT("simlod=0")); }
		if (C.bNoIskm) { NoteOverride(TEXT("iskm=0")); }
		if (C.bNoFixedBounds) { NoteOverride(TEXT("fixed_bounds=0")); }
		if (C.bNoFarCull) { NoteOverride(TEXT("far_cull=0")); }
		// 하이드레이션 액터 캡슐 콜리전(LabConfig.h ActorCollision): 기본 끔, m3colquery는 QueryOnly, 라벨 m3colbase는 켬. -LabActorCollision=1/0이 이긴다(Q5 하위 실험 S8-col)
		C.ActorCollision = C.Variant == ELabVariant::M3ColQuery ? FLabConfig::EActorCollision::QueryOnly
			: Variant == TEXT("m3colbase") ? FLabConfig::EActorCollision::On
			: FLabConfig::EActorCollision::Off;
		{
			int32 ActorCol = -1;
			if (FParse::Value(Cmd, TEXT("-LabActorCollision="), ActorCol) && ActorCol >= 0)
			{
				const FLabConfig::EActorCollision Wanted = ActorCol != 0 ? FLabConfig::EActorCollision::On : FLabConfig::EActorCollision::Off;
				if (Wanted != C.ActorCollision) { NoteOverride(FString::Printf(TEXT("actor_collision=%d"), ActorCol != 0 ? 1 : 0)); }
				C.ActorCollision = Wanted;
			}
		}
		// Q4 근거리 품질: 변형 기본값(m3hyd15·m3hyd50의 전환 거리, q4final의 고른 값), 그다음 인자
		if (C.Variant == ELabVariant::M3Hyd15) { C.HydrateM = 15.f; }
		else if (C.Variant == ELabVariant::M3Hyd50) { C.HydrateM = 50.f; }
		else if (C.Variant == ELabVariant::Q4Final) { C.ActorCap = FLabQuality::Q4FinalActorCap; C.HydrateM = FLabQuality::Q4FinalHydrateM; }
		const int32 DefaultCap = C.ActorCap;
		const float DefaultHydrateM = C.HydrateM;
		FParse::Value(Cmd, TEXT("-LabActorCap="), C.ActorCap);
		FParse::Value(Cmd, TEXT("-LabHydrateM="), C.HydrateM);
		C.ActorCap = FMath::Max(C.ActorCap, 0);
		C.HydrateM = FMath::Clamp(C.HydrateM, 1.f, FLabQuality::MaxHydrateM);
		if (C.ActorCap != DefaultCap) { NoteOverride(FString::Printf(TEXT("actor_cap=%d"), C.ActorCap)); }
		if (C.HydrateM != DefaultHydrateM) { NoteOverride(FString::Printf(TEXT("hydrate_m=%.0f"), C.HydrateM)); }
		// Q4b 원거리 그림자: q4final은 끔, 그 밖은 켬(지금까지의 동작). -LabFarShadow=0|1이 이긴다(빼 보기 A8-farshadow = q4final + -LabFarShadow=1)
		C.bFarShadow = C.Variant != ELabVariant::Q4Final;
		{
			int32 FarShadow = -1;
			if (FParse::Value(Cmd, TEXT("-LabFarShadow="), FarShadow) && FarShadow >= 0)
			{
				if ((FarShadow != 0) != C.bFarShadow) { NoteOverride(FString::Printf(TEXT("far_shadow=%d"), FarShadow != 0 ? 1 : 0)); }
				C.bFarShadow = FarShadow != 0;
			}
		}
		// 시나리오 쪽 인자: 기본값(확정값)과 다르면 적는다
		if (C.HydrateFixMask != 31) { NoteOverride(FString::Printf(TEXT("fixmask=%d"), C.HydrateFixMask)); }
		if (C.Types != 2) { NoteOverride(FString::Printf(TEXT("types=%d"), C.Types)); }
		if (C.FarM != 200) { NoteOverride(FString::Printf(TEXT("far=%d"), C.FarM)); }
		if (C.Seed != 20261106) { NoteOverride(FString::Printf(TEXT("seed=%d"), C.Seed)); }
		if (C.HealthFixed > 0.f) { NoteOverride(FString::Printf(TEXT("health_fixed=%.0f"), C.HealthFixed)); }
		if (C.FullyParallel == 0) { NoteOverride(TEXT("fully_parallel=0")); }
		C.Overrides = FString::Join(Overrides, TEXT("|")); // CSV 메타데이터 행은 쉼표로 나뉘므로 | 로 잇는다(LabCVars와 같다)
		C.N = FMath::Max(C.N, 1);
		C.Preset = FMath::Clamp(C.Preset, 1, 2);
		return C;
	}();
	return Config;
}

const TCHAR* FLabConfig::VariantName(ELabVariant V)
{
	switch (V)
	{
	case ELabVariant::Static: return TEXT("static");
	case ELabVariant::Budget: return TEXT("budget");
	case ELabVariant::Uro: return TEXT("uro");
	case ELabVariant::Pawn: return TEXT("pawn");
	case ELabVariant::Mass: return TEXT("mass");
	case ELabVariant::MassSt: return TEXT("massst");
	case ELabVariant::MassAv: return TEXT("massav");
	case ELabVariant::MassDirLo: return TEXT("massdirlo");
	case ELabVariant::MassDirHi: return TEXT("massdirhi");
	case ELabVariant::MassEvtLo: return TEXT("massevtlo");
	case ELabVariant::MassEvtHi: return TEXT("massevthi");
	case ELabVariant::VatVert: return TEXT("vatvert");
	case ELabVariant::VatCull: return TEXT("vatcull");
	case ELabVariant::VatCullVis: return TEXT("vatcullvis");
	case ELabVariant::VatDie0: return TEXT("vatdie0");
	case ELabVariant::VatDie1k: return TEXT("vatdie1k");
	case ELabVariant::Kill0: return TEXT("kill0");
	case ELabVariant::Kill1k: return TEXT("kill1k");
	case ELabVariant::M3Uro: return TEXT("m3uro");
	case ELabVariant::M3BudgetDist: return TEXT("m3budgetdist");
	case ELabVariant::M3BudgetSig: return TEXT("m3budgetsig");
	case ELabVariant::M3LowRes: return TEXT("m3lowres");
	case ELabVariant::M3NoActor: return TEXT("m3noactor");
	case ELabVariant::M3ColQuery: return TEXT("m3colquery");
	case ELabVariant::M3NoCol: return TEXT("m3nocol");
	case ELabVariant::M3Hyd15: return TEXT("m3hyd15");
	case ELabVariant::M3Hyd50: return TEXT("m3hyd50");
	case ELabVariant::StaticPawn: return TEXT("staticpawn");
	case ELabVariant::M3QOld: return TEXT("m3qold");
	case ELabVariant::M3SyncAll: return TEXT("m3syncall");
	case ELabVariant::M3Vis0: return TEXT("m3vis0");
	case ELabVariant::M3Vis15: return TEXT("m3vis15");
	case ELabVariant::M3Vis30: return TEXT("m3vis30");
	case ELabVariant::M3Hys20: return TEXT("m3hys20");
	case ELabVariant::M3Lock0: return TEXT("m3lock0");
	case ELabVariant::M3Order: return TEXT("m3order");
	case ELabVariant::M3Sig: return TEXT("m3sig");
	case ELabVariant::M4Off: return TEXT("m4off");
	case ELabVariant::M4M25L50: return TEXT("m4m25l50");
	case ELabVariant::M4M25L100: return TEXT("m4m25l100");
	case ELabVariant::M4M50L50: return TEXT("m4m50l50");
	case ELabVariant::M4Avoid: return TEXT("m4avoid");
	case ELabVariant::M4OffMove: return TEXT("m4offmove");
	case ELabVariant::M4OffPool: return TEXT("m4offpool");
	case ELabVariant::M5Iskm: return TEXT("m5iskm");
	case ELabVariant::M5Destroy: return TEXT("m5destroy");
	case ELabVariant::M5Blend: return TEXT("m5blend");
	case ELabVariant::M5MinSS: return TEXT("m5minss");
	case ELabVariant::M5Bounds: return TEXT("m5bounds");
	case ELabVariant::Q2Crowd: return TEXT("q2crowd");
	case ELabVariant::Q4Final: return TEXT("q4final");
	default: return TEXT("full");
	}
}

const TCHAR* FLabConfig::StageName(ELabStage S)
{
	switch (S)
	{
	case ELabStage::M1: return TEXT("M1");
	case ELabStage::M2: return TEXT("M2");
	case ELabStage::M3: return TEXT("M3");
	case ELabStage::M4: return TEXT("M4");
	case ELabStage::M5: return TEXT("M5");
	case ELabStage::M0:
	default: return TEXT("M0");
	}
}

FLabSimLod FLabConfig::SimLod() const
{
	FLabSimLod S; // 대표: Medium 0.5초 / Low 1.0초, 회피 끔
	switch (Variant)
	{
	case ELabVariant::M4Off: S.TickMedium = 0.f; S.TickLow = 0.f; break;
	case ELabVariant::M4M25L50: S.TickMedium = 0.25f; S.TickLow = 0.5f; break;
	case ELabVariant::M4M25L100: S.TickMedium = 0.25f; S.TickLow = 1.0f; break;
	case ELabVariant::M4M50L50: S.TickMedium = 0.5f; S.TickLow = 0.5f; break;
	case ELabVariant::M4Avoid: S.bAvoidance = true; break;
	case ELabVariant::M4OffMove: S.bOffMove = true; break;
	case ELabVariant::M4OffPool: S.bOffMove = true; break;
	default: break;
	}
	if (Stage == ELabStage::M5)
	{
		S.bOffMove = true; // M5 단계는 모든 변형이 m4offmove 구성 위에 있다(plan.md "M5" 결정 9)
	}
	return S;
}

bool FLabConfig::UsesIskm() const
{
	return Stage == ELabStage::M5 && Variant != ELabVariant::M4OffMove && Variant != ELabVariant::M4OffPool && !bNoIskm;
}

FLabIskmParams FLabConfig::IskmParams() const
{
	FLabIskmParams P; // 대표 m5iskm: BlendTime 0, 항상 재생, 그림자 켬
	if (Variant == ELabVariant::M5Blend) { P.BlendTime = 0.25f; }
	if (Variant == ELabVariant::M5MinSS) { P.AnimationMinScreenSize = 0.1f; }
	if ((Variant == ELabVariant::M5Bounds || Variant == ELabVariant::Q2Crowd || Variant == ELabVariant::Q4Final) && !bNoFixedBounds) { P.bFixedBounds = true; } // q2crowd = m5bounds + 배경 무리, q4final = q2crowd + 품질 값
	return P;
}

bool FLabConfig::UsesHydration() const
{
	return Stage >= ELabStage::M3 && Variant != ELabVariant::M3NoActor && !bNoHydrate;
}

bool FLabConfig::CullsFar() const
{
	return (Stage >= ELabStage::M3 || Variant == ELabVariant::VatCull || Variant == ELabVariant::VatCullVis) && !bNoFarCull;
}

float FLabConfig::DrawExtentCm() const
{
	return bNoFarCull ? FLabScenario::MapRadius : FarCm();
}

bool FLabConfig::UsesBudgetActors() const
{
	if (Stage == ELabStage::M0)
	{
		return Variant == ELabVariant::Budget;
	}
	return Stage >= ELabStage::M3 && (Variant == ELabVariant::M3BudgetDist || Variant == ELabVariant::M3BudgetSig || bScreenSigBudget);
}

TArray<FLabConfig::FTech> FLabConfig::TechList() const
{
	auto B = [](bool b) { return FString(b ? TEXT("1") : TEXT("0")); };
	const bool bM3 = Stage >= ELabStage::M3;
	const FString Fix = !bM3 ? TEXT("0") : HydrateFixMask == 31 ? TEXT("1") : HydrateFixMask == 0 ? TEXT("0") : FString::Printf(TEXT("m%d"), HydrateFixMask);
	const FString Sim = !UsesSimLod() ? TEXT("0") : SimLod().bOffMove ? TEXT("1") : TEXT("tick");
	return {
		{ TEXT("LabTechHydrate"), TEXT("hyd"), B(UsesHydration()) },
		{ TEXT("LabTechAnimBudget"), TEXT("bud"), B(UsesBudgetActors()) },
		{ TEXT("LabTechHydrateFix"), TEXT("fix"), Fix },
		{ TEXT("LabTechPool"), TEXT("pool"), B(bEntityPool) },
		{ TEXT("LabTechSimLod"), TEXT("sim"), Sim },
		{ TEXT("LabTechIskm"), TEXT("iskm"), B(UsesIskm()) },
		{ TEXT("LabTechFixedBounds"), TEXT("bnd"), B(IskmParams().bFixedBounds) },
		{ TEXT("LabTechFarCull"), TEXT("far"), B(CullsFar()) },
		{ TEXT("LabTechCrowdFx"), TEXT("fx"), B(bCrowdFx) },
	};
}

FString FLabConfig::TechString() const
{
	TArray<FString> Parts;
	for (const FTech& T : TechList())
	{
		Parts.Add(FString::Printf(TEXT("%s:%s"), T.LogKey, *T.Value));
	}
	return TEXT("tech=") + FString::Join(Parts, TEXT(",")); // 한 키로 묶는다(로그의 pool= 등 기존 키와 겹치지 않게)
}

const TCHAR* FLabConfig::ActorCollisionString() const
{
	if (!UsesHydration())
	{
		return TEXT("-"); // 하이드레이션 액터가 없다(M0 액터는 CharacterMovement가 캡슐을 쓰므로 늘 켜져 있고 바꾸지 않는다)
	}
	switch (ActorCollision)
	{
	case EActorCollision::On: return TEXT("1");
	case EActorCollision::QueryOnly: return TEXT("query");
	default: return TEXT("0");
	}
}

FLabConfig::FLadder FLabConfig::Ladder() const
{
	// plan.md "품질 계획" 사다리 표. 이름은 HUD(영어, 엔진 글꼴)에 쓴다. 4단계는 화면 크기 Budget이 켜진 mass다(m3mass 라벨은 Budget을 끈다).
	switch (Stage)
	{
	case ELabStage::M0:
		if (Variant == ELabVariant::Full) { return { TEXT("1"), TEXT("Actor baseline") }; }
		if (Variant == ELabVariant::Budget) { return { TEXT("1b"), TEXT("Actors + Budget Allocator") }; }
		if (Variant == ELabVariant::Uro) { return { TEXT("1c"), TEXT("Actors + URO") }; }
		break;
	case ELabStage::M1:
		if (Variant == ELabVariant::Mass) { return { TEXT("2"), TEXT("Logic in Mass") }; }
		break;
	case ELabStage::M2:
		if (Variant == ELabVariant::VatCull) { return { TEXT("3"), TEXT("ISM + VAT, cull past 200 m") }; }
		break;
	case ELabStage::M3:
		if (Variant == ELabVariant::Mass && bScreenSigBudget) { return { TEXT("4"), TEXT("Near actors + anim budget") }; }
		break;
	case ELabStage::M5:
		if (Variant == ELabVariant::M4OffPool) { return { TEXT("5"), TEXT("Simulation LOD + entity pool") }; }
		if (Variant == ELabVariant::M5Bounds) { return { TEXT("6"), TEXT("ISKM + fixed bounds") }; }
		if (Variant == ELabVariant::Q2Crowd) { return { TEXT("7"), TEXT("Niagara horizon crowd") }; }
		if (Variant == ELabVariant::Q4Final) { return { TEXT("8"), FString::Printf(TEXT("Actor cap %d, hydrate %.0f m"), ActorCap, HydrateM) }; } // 실제 값(이웃 점 -LabHydrateM=70이면 70)
		break;
	default:
		break;
	}
	return { TEXT("-"), FString() };
}

float FLabConfig::CrowdFxTotalRate() const
{
	return bCrowdFxOuter ? float(N) / FLabScenario::MapRadius * FLabScenario::MoveSpeed : 0.f; // Q5b: -LabCrowdFxOuter=0이면 독립 입자 없음
}

float FLabConfig::CrowdFxLargeRate() const
{
	return Types == 2 ? CrowdFxTotalRate() * FLabCrowdFx::LargeShare : 0.f;
}

float FLabConfig::CrowdFxSmallRate() const
{
	return CrowdFxTotalRate() - CrowdFxLargeRate(); // 1종이면 전부 소형
}

float FLabConfig::CrowdFxInnerCm() const
{
	if (CrowdFxInnerOverrideM > 0.f)
	{
		return CrowdFxInnerOverrideM * 100.f;
	}
	if (bCrowdFxEntity)
	{
		// Q2b: 엔티티는 맵 반경 안에만 있다(초기 배치 [8m, 500m], 재생성 링 [490m, 500m], LabCrowdSpawner.cpp). 그 밖만 독립 입자로 채운다
		return FLabScenario::MapRadius;
	}
	const float EnterCm = FMath::Lerp(FLabCrowdFx::RepLowCm, FarCm(), 1.f - FLabCrowdFx::RepHysteresis);
	return FMath::Sqrt(FMath::Square(EnterCm) - FMath::Square(FLabScenario::OrbViewHeight)) - FLabScenario::OrbViewBack;
}

float FLabConfig::CrowdFxLifetime() const
{
	return (FLabCrowdFx::OuterRadius - CrowdFxInnerCm()) / FLabScenario::MoveSpeed;
}

int32 FLabConfig::CrowdFxParticles() const
{
	const float Lifetime = CrowdFxLifetime();
	return bCrowdFx && Lifetime > 0.f ? FMath::RoundToInt(CrowdFxTotalRate() * Lifetime) : 0;
}

FString FLabConfig::SimLodString() const
{
	if (!UsesSimLod())
	{
		return TEXT("-");
	}
	const FLabSimLod S = SimLod();
	// offmove는 켠 변형에서만 붙인다. 다른 변형의 문자열(CSV 메타데이터 LabSimLod)은 그대로다.
	return FString::Printf(TEXT("%.0f/%.0fm tick=0/%.2f/%.2f avoid=%d%s"), FLabSimLod::MediumDistance / 100.f, FLabSimLod::LowDistance / 100.f, S.TickMedium, S.TickLow, S.bAvoidance ? 1 : 0,
		S.bOffMove ? TEXT(" offmove=1") : TEXT(""));
}

float FLabConfig::HealthFor(uint32 Serial) const
{
	if (HealthFixed > 0.f)
	{
		return HealthFixed;
	}
	// murmur3 마무리 혼합. 시드와 일련번호만 입력이라 같은 실행 조건이면 같은 값이다.
	uint32 H = uint32(Seed) ^ (Serial * 0x9E3779B1u);
	H ^= H >> 16; H *= 0x85EBCA6Bu;
	H ^= H >> 13; H *= 0xC2B2AE35u;
	H ^= H >> 16;
	const float U = float(H >> 8) * (1.f / 16777216.f); // [0, 1)
	return FLabScenario::HealthMin + U * (FLabScenario::HealthMax - FLabScenario::HealthMin);
}

FString FLabConfig::ToLogString() const
{
	// far는 기본(200)이 아닐 때만 붙인다. 기본 실행의 로그는 그대로다. pool·far_rep는 M5 단계이거나 풀링을 켰을 때만 붙인다.
	FString M5;
	if (Stage == ELabStage::M5 || bEntityPool)
	{
		M5 = FString::Printf(TEXT(" pool=%d far_rep=%s"), bEntityPool ? 1 : 0, UsesIskm() ? TEXT("iskm") : TEXT("vat"));
		if (UsesIskm())
		{
			const FLabIskmParams P = IskmParams();
			M5 += FString::Printf(TEXT(" iskm_blend=%.2f iskm_minss=%.2f iskm_shadow=%d iskm_bounds=%d"), P.BlendTime, P.AnimationMinScreenSize, P.bCastShadows ? 1 : 0, P.bFixedBounds ? 1 : 0);
		}
	}
	// screensig_budget는 M3 이상 단계이거나 켰을 때만 붙인다. M0~M2 기본 실행의 로그는 그대로다.
	FString Budget = (Stage >= ELabStage::M3 || bScreenSigBudget) ? FString::Printf(TEXT(" screensig_budget=%d"), bScreenSigBudget ? 1 : 0) : FString();
	// Q4 근거리 품질(액터 상한, 화면 안 전환 거리)은 M3 이상 단계에서만 붙인다
	if (Stage >= ELabStage::M3)
	{
		Budget += FString::Printf(TEXT(" actor_cap=%d hydrate_m=%.0f far_shadow=%d"), ActorCap, HydrateM, bFarShadow ? 1 : 0); // far_shadow: Q4b
	}
	// health_fixed는 디버그 인자를 줬을 때만 붙인다
	const FString Health = HealthFixed > 0.f ? FString::Printf(TEXT(" health_fixed=%.0f"), HealthFixed) : FString();
	// crowdfx는 켰을 때만 붙인다(Q2). 기본 실행의 로그는 그대로다.
	// crowdfx_entity(Q2b), crowdfx_outer(Q5b)는 그 뒤에 붙인다. crowdfx_particles는 독립 입자만이다(엔티티 구동 입자는 CSV 스탯 Lab/CrowdRecs).
	const FString Crowd = bCrowdFx ? FString::Printf(TEXT(" crowdfx=1 crowdfx_particles=%d crowdfx_inner=%.1f crowdfx_entity=%d crowdfx_outer=%d"), CrowdFxParticles(), CrowdFxInnerCm() / 100.f, bCrowdFxEntity ? 1 : 0, bCrowdFxOuter ? 1 : 0) : FString();
	// tech는 빼 보기 아홉 기법의 켬·끔이다(Q3). 모든 실행에 붙여 러너와 사람이 기대값과 대조한다.
	// ladder·overrides·hud·actor_col은 Q5(사다리 단계, 변형 기본값과 다른 인자, HUD 켬, 하이드레이션 액터 캡슐 콜리전). tech 뒤에 늘 붙인다.
	return FString::Printf(TEXT("lab_config stage=%s n=%d preset=P%d seed=%d variant=%s bench=%d label=%s duration=%.1f types=%d simlod=%s%s%s%s%s%s %s ladder=%s overrides=%s hud=%d actor_col=%s"),
		StageName(Stage), N, Preset, Seed, VariantName(Variant), bBench ? 1 : 0, *Label, DurationSec, Types, *SimLodString(),
		FarM != 200 ? *FString::Printf(TEXT(" far=%d"), FarM) : TEXT(""), *M5, *Budget, *Health, *Crowd, *TechString(),
		*Ladder().Id, Overrides.IsEmpty() ? TEXT("-") : *Overrides, bHud ? 1 : 0, ActorCollisionString());
}
