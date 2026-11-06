using UnrealBuildTool;

public class HordeOptLabTarget : TargetRules
{
	public HordeOptLabTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("HordeOptLab");
	}
}
