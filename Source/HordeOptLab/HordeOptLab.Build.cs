using UnrealBuildTool;

public class HordeOptLab : ModuleRules
{
	public HordeOptLab(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// M0은 액터 군중이다. Mass 모듈(MassEntity, MassSpawner, MassRepresentation 등)은 M1에서 더한다.
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"AnimationBudgetAllocator", // M0 budget 변형: USkeletalMeshComponentBudgeted
			"SignificanceManager",      // M0 budget 변형: 거리 단계 중요도
			"Niagara"                   // 오브 이펙트(테슬라 코일, 피해 범위)
		});
	}
}
