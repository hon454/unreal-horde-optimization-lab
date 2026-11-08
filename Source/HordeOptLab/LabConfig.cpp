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
		C.bBench = FParse::Param(Cmd, TEXT("LabBench"));
		C.bHud = FParse::Param(Cmd, TEXT("LabHud")) && !C.bBench;

		FString Stage;
		if (FParse::Value(Cmd, TEXT("-LabStage="), Stage))
		{
			if (Stage.Equals(TEXT("M0"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M0; }
			else if (Stage.Equals(TEXT("M1"), ESearchCase::IgnoreCase)) { C.Stage = ELabStage::M1; }
			else { C.bStageValid = false; }
		}

		FString Variant;
		if (FParse::Value(Cmd, TEXT("-LabVariant="), Variant))
		{
			if (Variant == TEXT("static")) { C.Variant = ELabVariant::Static; }
			else if (Variant == TEXT("budget")) { C.Variant = ELabVariant::Budget; }
			else if (Variant == TEXT("uro")) { C.Variant = ELabVariant::Uro; }
			else if (Variant == TEXT("pawn")) { C.Variant = ELabVariant::Pawn; }
			else if (Variant == TEXT("mass")) { C.Variant = ELabVariant::Mass; }
			else if (Variant == TEXT("massst")) { C.Variant = ELabVariant::MassSt; }
			else if (Variant == TEXT("massav")) { C.Variant = ELabVariant::MassAv; }
			else if (Variant == TEXT("massdirlo")) { C.Variant = ELabVariant::MassDirLo; }
			else if (Variant == TEXT("massdirhi")) { C.Variant = ELabVariant::MassDirHi; }
			else if (Variant == TEXT("massevtlo")) { C.Variant = ELabVariant::MassEvtLo; }
			else if (Variant == TEXT("massevthi")) { C.Variant = ELabVariant::MassEvtHi; }
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
	default: return TEXT("full");
	}
}

const TCHAR* FLabConfig::StageName(ELabStage S)
{
	switch (S)
	{
	case ELabStage::M1: return TEXT("M1");
	case ELabStage::M0:
	default: return TEXT("M0");
	}
}

FString FLabConfig::ToLogString() const
{
	return FString::Printf(TEXT("lab_config stage=%s n=%d preset=P%d seed=%d variant=%s bench=%d label=%s duration=%.1f"),
		StageName(Stage), N, Preset, Seed, VariantName(Variant), bBench ? 1 : 0, *Label, DurationSec);
}
