using UnrealBuildTool;

public class HordeOptLab : ModuleRules
{
	public HordeOptLab(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// M0은 액터 군중, M1부터 Mass 엔티티다.
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"RenderCore",               // 상태 오버레이: GGameThreadTime, GRenderThreadTime
			"RHI",                      // 상태 오버레이: RHIGetGPUFrameCycles
			"AnimationBudgetAllocator", // M0 budget 변형: USkeletalMeshComponentBudgeted
			"SignificanceManager",      // M0 budget 변형: 거리 단계 중요도
			"Niagara",                  // 오브 이펙트(테슬라 코일, 피해 범위)
			// M1 Mass 코어
			"MassEntity",
			"MassCore",                 // FTransformFragment
			"MassCommon",               // FAgentRadiusFragment, 프로세서 그룹 이름
			"MassSpawner",              // UMassEntityConfigAsset, UMassSpawnerSubsystem
			"MassMovement",
			"MassNavigation",           // 엔진 조향, FMassMoveTargetFragment
			"MassLOD",
			"MassRepresentation",       // UMassMovableVisualizationTrait
			"MassActors"                // FMassActorFragment
		});
	}
}
