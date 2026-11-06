using UnrealBuildTool;

public class HordeOptLabEditorTarget : TargetRules
{
	public HordeOptLabEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("HordeOptLab");
	}
}
