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
			C.bStageValid = Stage.Equals(TEXT("M0"), ESearchCase::IgnoreCase);
		}

		FString Variant;
		if (FParse::Value(Cmd, TEXT("-LabVariant="), Variant))
		{
			if (Variant == TEXT("static")) { C.Variant = ELabVariant::Static; }
			else if (Variant == TEXT("budget")) { C.Variant = ELabVariant::Budget; }
			else if (Variant == TEXT("uro")) { C.Variant = ELabVariant::Uro; }
			else if (Variant == TEXT("pawn")) { C.Variant = ELabVariant::Pawn; }
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
	default: return TEXT("full");
	}
}

const TCHAR* FLabConfig::StageName(ELabStage S)
{
	switch (S)
	{
	case ELabStage::M0:
	default: return TEXT("M0");
	}
}

FString FLabConfig::ToLogString() const
{
	return FString::Printf(TEXT("lab_config stage=%s n=%d preset=P%d seed=%d variant=%s bench=%d label=%s duration=%.1f"),
		StageName(Stage), N, Preset, Seed, VariantName(Variant), bBench ? 1 : 0, *Label, DurationSec);
}
