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
		C.bBench = FParse::Param(Cmd, TEXT("LabBench"));
		C.bHud = FParse::Param(Cmd, TEXT("LabHud")) && !C.bBench;

		FString Stage;
		if (FParse::Value(Cmd, TEXT("-LabStage="), Stage))
		{
			if (Stage.Equals(TEXT("M0"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M0; }
			else if (Stage.Equals(TEXT("M1"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M1; }
			else if (Stage.Equals(TEXT("M2"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M2; }
			else if (Stage.Equals(TEXT("M3"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M3; }
			else if (Stage.Equals(TEXT("M4"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M4; }
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
	default: break;
	}
	return S;
}

FString FLabConfig::SimLodString() const
{
	if (!UsesSimLod())
	{
		return TEXT("-");
	}
	const FLabSimLod S = SimLod();
	return FString::Printf(TEXT("%.0f/%.0fm tick=0/%.2f/%.2f avoid=%d"), FLabSimLod::MediumDistance / 100.f, FLabSimLod::LowDistance / 100.f, S.TickMedium, S.TickLow, S.bAvoidance ? 1 : 0);
}

FString FLabConfig::ToLogString() const
{
	return FString::Printf(TEXT("lab_config stage=%s n=%d preset=P%d seed=%d variant=%s bench=%d label=%s duration=%.1f types=%d simlod=%s"),
		StageName(Stage), N, Preset, Seed, VariantName(Variant), bBench ? 1 : 0, *Label, DurationSec, Types, *SimLodString());
}
