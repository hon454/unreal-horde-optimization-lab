using System;
using System.IO;
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
			"RHI",                      // 상태 오버레이: RHIGetGPUFrameCycles. CSV 메타데이터: GPU 드라이버 버전
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
			"MassActors",               // FMassActorFragment
			// M2 VAT
			"AnimToTexture"             // UAnimToTextureDataAsset(클립 프레임 범위)
		});

		AddBuildVersionHeader();
	}

	/**
	 * M3b 측정 방법 5번: 게임이 CSV 메타데이터 LabBuild에 쓰는 빌드 커밋 해시를 헤더 LabBuildVersion.h로 만든다.
	 * .git/HEAD와 그것이 가리키는 ref 파일을 읽고(git 실행 없음), 두 파일을 ExternalDependencies에 넣어 커밋이 바뀌면 UBT가 이 Build.cs를 다시 평가하게 한다.
	 * 헤더는 내용이 바뀔 때만 다시 써서 LabGameMode.cpp만 다시 컴파일된다. 작업 트리의 dirty 여부는 알 수 없다. 그것은 package.ps1이 commit.txt에 남긴다.
	 */
	private void AddBuildVersionHeader()
	{
		string ProjectDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", ".."));
		string GitDir = Path.Combine(ProjectDir, ".git");
		string Commit = "unknown";
		try
		{
			string HeadPath = Path.Combine(GitDir, "HEAD");
			if (File.Exists(HeadPath))
			{
				ExternalDependencies.Add(HeadPath);
				string Head = File.ReadAllText(HeadPath).Trim();
				if (Head.StartsWith("ref: "))
				{
					string Ref = Head.Substring(5).Trim();
					string RefPath = Path.Combine(GitDir, Ref.Replace('/', Path.DirectorySeparatorChar));
					if (File.Exists(RefPath))
					{
						ExternalDependencies.Add(RefPath);
						Commit = File.ReadAllText(RefPath).Trim();
					}
					else
					{
						string PackedPath = Path.Combine(GitDir, "packed-refs");
						if (File.Exists(PackedPath))
						{
							ExternalDependencies.Add(PackedPath);
							foreach (string Line in File.ReadAllLines(PackedPath))
							{
								if (Line.EndsWith(" " + Ref)) { Commit = Line.Substring(0, Line.IndexOf(' ')); break; }
							}
						}
					}
				}
				else
				{
					Commit = Head; // detached HEAD
				}
			}
		}
		catch (Exception E)
		{
			Console.WriteLine("HordeOptLab.Build.cs: git commit unavailable ({0})", E.Message);
		}
		if (Commit.Length > 12) { Commit = Commit.Substring(0, 12); }

		string GenDir = Path.Combine(ProjectDir, "Intermediate", "Build", "LabGenerated");
		Directory.CreateDirectory(GenDir);
		string HeaderPath = Path.Combine(GenDir, "LabBuildVersion.h");
		string Content = "// HordeOptLab.Build.cs가 만든다. 고치지 않는다.\n#pragma once\n#define LAB_BUILD_COMMIT \"" + Commit + "\"\n";
		if (!File.Exists(HeaderPath) || File.ReadAllText(HeaderPath) != Content)
		{
			File.WriteAllText(HeaderPath, Content);
		}
		PrivateIncludePaths.Add(GenDir);
	}
}
