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
		string HeaderPath = Path.Combine(GenDir, "LabBuildVersion.h");
		string Content = "// HordeOptLab.Build.cs가 만든다. 고치지 않는다.\n#pragma once\n#define LAB_BUILD_COMMIT \"" + Commit + "\"\n";
		WriteHeaderIfChanged(GenDir, HeaderPath, Content);
		PrivateIncludePaths.Add(GenDir);
	}

	/**
	 * 2026-11-09 package.ps1 -Config Test에서 UBT 인스턴스 여럿(쿠킹용 에디터 타깃 + 게임 타깃)이 아직 없는 헤더를 동시에 만들려다
	 * 파일 잠금 충돌로 첫 패키징이 실패했다. 그래서 임시 파일에 쓰고 Move로 바꾸며, 잠금 예외가 나면 다른 인스턴스가 같은 내용을 먼저 썼는지 다시 읽어 확인한다.
	 */
	private static void WriteHeaderIfChanged(string GenDir, string HeaderPath, string Content)
	{
		const int MaxAttempts = 3;
		for (int Attempt = 1; ; ++Attempt)
		{
			try
			{
				if (ReadOrNull(HeaderPath) == Content)
				{
					return;
				}
				try { Directory.CreateDirectory(GenDir); } catch (Exception) { }
				string TempPath = HeaderPath + "." + Guid.NewGuid().ToString("N");
				try
				{
					File.WriteAllText(TempPath, Content);
					File.Move(TempPath, HeaderPath, true);
				}
				finally
				{
					try { if (File.Exists(TempPath)) { File.Delete(TempPath); } } catch (Exception) { }
				}
				return;
			}
			catch (Exception E) when (E is IOException || E is UnauthorizedAccessException)
			{
				System.Threading.Thread.Sleep(200 * Attempt);
				if (ReadOrNull(HeaderPath) == Content)
				{
					return; // 다른 UBT 인스턴스가 먼저 썼다.
				}
				if (Attempt >= MaxAttempts)
				{
					throw;
				}
			}
		}
	}

	private static string ReadOrNull(string FilePath)
	{
		try
		{
			return File.Exists(FilePath) ? File.ReadAllText(FilePath) : null;
		}
		catch (Exception E) when (E is IOException || E is UnauthorizedAccessException)
		{
			return null;
		}
	}
}
