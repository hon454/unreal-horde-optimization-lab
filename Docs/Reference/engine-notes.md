# 엔진 소스 확인 기록

엔진 소스와 실행에서 확인한 사실만 적는다. 기억이나 문서의 주장은 적지 않는다. 경로는 엔진 루트 기준이고, 줄 번호는 GitHub `5.8.4-release` 태그에서 읽었다(2026-11-06). PC의 소스 빌드 태그가 다르면(5.8.3 가능) 쓰기 전에 그 태그로 확인하고 여기에 적는다. 계획이 틀렸던 항목은 **[계획 수정]**으로 표시했다.

## 1. Mass 모듈 위치(5.8)

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 코어 모듈 | `MassEntity`는 **5.5부터** 엔진 코어(`Engine/Source/Runtime/MassEntity/`)에 있다. 5.8에서 플러그인에서 옮긴 것이 아니다 **[계획 수정]** | 5.5.4 태그에 `Runtime/MassEntity/MassEntity.Build.cs` 존재 |
| 5.8 신규 모듈 | `Engine/Source/Runtime/Mass/{MassCore, MassSignals, MassEngine, MassDeveloper}` | 디렉터리 목록 |
| Signal | 5.7은 `Engine/Plugins/Runtime/MassGameplay/Source/MassSignals/`, 5.8은 `Engine/Source/Runtime/Mass/MassSignals/Public/`(`MassSignalSubsystem.h`, `MassSignalProcessorBase.h`, `MassSignalTypes.h`) | |
| Query Executor | `MassQueryExecutor.h`는 **5.6부터** 있다 **[계획 수정]** | |
| Sparse Fragment | 5.8 신규. `MassSparseElementsStorage.h` | |
| MassGameplay 모듈 | `MassActors MassCharacterTrajectory MassCommon MassEQS MassGameplayDebug MassGameplayEditor MassGameplayExternalTraits MassGameplayTestSuite MassLOD MassMovement MassMovementEditor MassReplication MassRepresentation MassSimulation MassSmartObjects MassSpawner` | `Engine/Plugins/Runtime/MassGameplay/Source/` |
| MassAI 모듈 | `MassAIBehavior MassAIBehaviorEditor MassAIDebug MassAIReplication MassAITestSuite MassNavMeshNavigation MassNavigation MassNavigationEditor MassZoneGraphNavigation` | `Engine/Plugins/AI/MassAI/Source/`. `Engine/Plugins/AI/MassCrowd/`도 있다 |
| 플러그인 상태 | `MassGameplay`, `MassAI`, `AnimToTexture`: `IsExperimentalVersion true`, `EnabledByDefault false`. `AnimationBudgetAllocator`, `SignificanceManager`: Experimental 아님 | 각 `.uplugin` |
| MassGameplay 의존 플러그인 | `ZoneGraph`, `PoseSearch`, `ZoneGraphAnnotations`, `SmartObjects`, `StateTree`, `DataValidation` | `MassGameplay.uplugin` |
| 스탯 그룹 | `DECLARE_STATS_GROUP(TEXT("Mass"), STATGROUP_Mass, STATCAT_Advanced)`, `STAT_Mass_Total`. CSV: `CSV_SCOPED_TIMING_STAT_EXCLUSIVE(MassProcessorTask)` | `MassEntity/Public/MassEntityTypes.h:28-29`, `MassEntity/Private/MassProcessor.cpp:80` |

## 2. 병렬 처리 CVar

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 전체 병렬 | **`mass.FullyParallel`**(기본 `MASS_DO_PARALLEL`). 계획의 `mass.ParallelProcessing`은 없다 **[계획 수정]** | `Engine/Source/Runtime/Mass/MassEngine/Private/MassProcessingPhaseManager.cpp:31,36,576` |
| 보조 | `mass.UseProcessingQueue`, `mass.MakePrePhysicsTickFunctionHighPriority` | 같은 파일 |

## 3. Representation LOD와 중요도

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| `LODSignificance` 범위 | `FMassRepresentationLODFragment::LODSignificance`: "Value scaling from 0 to 3, 0 highest LOD we support and 3 being completely off LOD". 계획의 "0.0\~1.0"은 틀렸다 **[계획 수정]** | `MassRepresentation/Public/MassRepresentationFragments.h:46-48`, `PrevLODSignificance` `:76-78` |
| Budget과 방향 | Animation Budget Allocator는 큰 값이 중요하다("Sort by significance, largest first"). 넘길 때 `1 - LODSignificance / 3`처럼 변환한다 | `AnimationBudgetAllocator.cpp:327` |
| Significance 설정 API | `USkeletalMeshComponentBudgeted::SetComponentSignificance(float Significance, bool bNeverSkip, bool bTickEvenIfNotRendered, bool bAllowReducedWork, bool bForceInterpolate)`. BeginPlay에 등록, EndPlay에 해제 | `SkeletalMeshComponentBudgeted.h:36` |
| Budget이 URO를 끈다 | 등록 시 `bEnableUpdateRateOptimizations = false` | `AnimationBudgetAllocator.cpp` |
| Trait | `UMassDistanceVisualizationTrait`·`UMassVisualizationTrait`는 **5.4부터** soft-deprecated. `UMassMovableVisualizationTrait`·`UMassStationaryVisualizationTrait`를 쓴다 | `MassRepresentation/Public/MassMovableVisualizationTrait.h` |
| ISM Custom Data | `FMassInstancedStaticMeshInfo::AddBatchedCustomData<T>(…)`, `WriteCustomDataFloatsAtStartIndex`, `FStaticMeshInstanceVisualizationDesc::CustomDataFloats`. `UMassUpdateISMProcessor`를 상속하지 말고 Representation 그룹에서 그 뒤에 도는 별도 프로세서로 쓴다 | `MassRepresentationTypes.h:665,700` |
| ISKM Mass 경로 | `FMassSkinnedMeshInstanceVisualizationMeshDesc`, `UMassConsumeInstancedSkinnedMeshAnimationProcessor`(엔티티 → `FAnimSequenceTrackAutoPlayData` → ISKM 배치 갱신), `FMassInstancedSkinnedMeshComponentSharedData` | `MassRepresentation/Public/MassSkinnedMeshRepresentationTypes.h`, `MassUpdateSkinnedMeshProcessor.h` |
| ISKM 트랙 풀 누수 | 5.8.4에도 남아 있다. 트랙 해제 경로가 `FMassStaticRepresentationTag`를 요구하는데 동적 처리기는 그 태그를 금지한다. 우회: 사망 시 엔티티를 파괴하지 않고 재활용 | `MassUpdateSkinnedMeshProcessor.cpp`(M5 전에 줄 번호 확인) |
| ISKM 블렌딩 | BlendTime 크로스페이드와 BlendSpace 트랙이 있다. 없는 것은 AnimBP다. 계획의 "블렌딩 없음"은 틀렸다 **[계획 수정]** | `InstancedSkinnedMeshComponent.h` |
| 엔진 기본 `LODMaxCount` | High/Medium/Low 기본 `{50, 100, 500}`. 밴드 거리보다 이 상한이 먼저 걸린다. 프로젝트 값을 명시한다 | `MassLODTrait.h` / `MassRepresentationParameters`(M0에서 줄 번호 확인) |

## 4. 액터 풀링

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 풀 반환 동작 | `UMassActorSpawnerSubsystem`의 풀링은 액터를 숨기고 보관한다. 컴포넌트 등록과 틱은 유지된다. `USkeletalMeshComponentBudgeted`가 붙어 있으면 숨긴 뒤에도 예산에 남는다 | `MassActors/Private/MassActorSpawnerSubsystem.cpp`(M3 전에 줄 번호 확인) |
| 대처 | `IMassActorPoolableInterface::PrepareForPooling`에서 중요도 0, 틱 끔. `PrepareForGame`에서 복원 | `MassActors/Public/MassActorPoolableInterface.h` |

## 5. Simulation LOD와 회피

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 가변 틱 | `FMassSimulationVariableTickParameters::TickRates[EMassLOD::Max]`, `TMassLODTickRateController`가 청크 단위로 이 프레임에 돌릴지 결정 | `MassLOD/Public/MassSimulationLOD.h`, `MassLODTickRateController.h` |
| 회피는 High만 | `UMassMovingAvoidanceProcessor`는 High LOD 엔티티만 처리한다. Simulation LOD를 켜면 먼 개체의 회피는 자동으로 꺼진다 | `MassNavigation/Private/MassAvoidanceProcessors.cpp`(M4 전에 줄 번호 확인) |
| Avoidance 상태 | Experimental | `MassAI.uplugin` |

## 6. Test 구성 매크로(측정 빌드)

`Engine/Source/Runtime/Core/Public/Misc/Build.h` `UE_BUILD_TEST` 블록(298\~330행 부근)과 관련 헤더.

| 매크로 | Test 값 | 위치 | 영향 |
| --- | --- | --- | --- |
| `STATS` | `FORCE_USE_STATS && !ENABLE_STATNAMEDEVENTS`, 기본 0 | `Build.h:314` | `stat anim` 등 그룹 없음 |
| `ENABLE_STATNAMEDEVENTS` | `!FORCE_USE_STATS && !USE_STATS_WITHOUT_ENGINE` = 1 | `Build.h:308` | `-statnamedevents`로 사이클 스탯이 트레이스 이벤트가 된다 |
| `ALLOW_CONSOLE` | 1 | `Build.h:320` | 콘솔 명령 사용 가능 |
| `DO_CHECK` | `USE_CHECKS_IN_SHIPPING`(기본 0) | `Build.h:302` | Mass의 `check` 꺼짐 |
| `CSV_PROFILER` | `WITH_ENGINE && (!UE_BUILD_SHIPPING \|\| CSV_PROFILER_ENABLE_IN_SHIPPING)` = 1 | `ProfilingDebugging/CsvProfilerConfig.h:31` | `csvprofile` 사용 가능 |
| `UE_TRACE_ENABLED` | `!UE_BUILD_SHIPPING && !IS_PROGRAM` = 1 | `TraceLog/Public/Trace/Config.h:13-15` | Insights 가능 |
| `HAS_GPU_STATS` | `(STATS \|\| CSV_PROFILER_STATS) && !UE_BUILD_SHIPPING` = 1 | `RenderCore/Public/ProfilingDebugging/RealtimeGPUProfiler.h:74` | GPU 시간 CSV·트레이스 가능 |
| `WITH_PROFILEGPU` | `ALLOW_PROFILEGPU_IN_TEST 0`이라 0 | `Build.h:403,411` | `ProfileGPU` 없음 |
| `UE_ENABLE_DEBUG_DRAWING` | `!(SHIPPING \|\| TEST) \|\| WITH_EDITOR` = 0 | `Engine/Public/EngineDefines.h:15` | 디버그 드로우, `a.Budget.Debug.*`(`AnimationBudgetAllocatorCVars.cpp:54 #if ENABLE_DRAW_DEBUG`) 없음 |
| 엔진 CSV 기본 스탯 | `GameThreadTime`, `RenderThreadTime`, `GPUTime`, `RenderThreadTime_CriticalPath`, `GameThreadTime_CriticalPath`, `RHIThreadTime` | `Launch/Private/LaunchEngineLoop.cpp:1518-1525` | 핵심 열의 출처 |
| CSV 압축 | `csv.CompressionMode` -1/0/1. 0이면 `.csv` | `Core/Private/ProfilingDebugging/CsvProfiler.cpp:183-194` | 러너는 0 |

## 7. 측정 도구

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 히치 스냅샷 | 콘솔 `snapshothitches -start/-stop/-test`, `TRACE_BOOKMARK("Hitch detected")`, `Saved/Profiling/Hitches/`. 스탯 그룹이 켜져 있어야 동작하므로 Test에서는 확인 필요 | `Engine/Private/ProfilingDebugging/TraceHitches.cpp:26` |
| Mass 트레이스 채널 | `-trace=…,mass`(MassChannel) | `MassEntity/Public/MassDebugger.h`(M1 전에 확인) |

## 8. Unreal MCP

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 플러그인 | `ModelContextProtocol`(FriendlyName "Unreal MCP", Experimental, `EnabledByDefault false`). 모듈 `ModelContextProtocol`(Runtime), `ModelContextProtocolEngine`(Runtime), `ModelContextProtocolEditor`(Editor). 의존 `ToolsetRegistry` | `Engine/Plugins/Experimental/ModelContextProtocol/ModelContextProtocol.uplugin` |
| 툴셋 | `AllToolsets`가 21개 툴셋 플러그인을 묶는다(`AIModuleToolset`, `AnimationAssistantToolset`, `ConfigSettingsToolset`, `EditorToolset`, `NiagaraToolsets`, `PhysicsToolsets`, `StateTreeToolset`, …) | `Engine/Plugins/Experimental/Toolsets/AllToolsets/AllToolsets.uplugin` |
| 콘솔 | `ModelContextProtocol.StartServer`, `StopServer`, `RefreshTools`, `GenerateClientConfig <ClaudeCode\|Cursor\|VSCode\|Gemini\|…>` → `.mcp.json` | `ModelContextProtocolEditor` 소스 |
| Runtime 모듈 포함 | Runtime 타입 모듈이 있으므로 Test 패키지에 들어갈 수 있다. `.uproject`에서 `TargetAllowList: ["Editor"]`로 제한한다 | |

## 9. 에셋과 피직스

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 에셋 마이그레이션 | `IAssetTools::MigratePackages(PackageNames, DestinationPath, FMigrationOptions)`가 `BlueprintCallable`이라 Python 커맨드렛에서 부를 수 있다. `FMigrationOptions::bPrompt`는 스크립트에서 항상 false, 충돌 시 기본 `Skip` | `Developer/AssetTools/Public/IAssetTools.h:241-266,633` |
| 피직스 바디 타입 | `PhysType_Default`는 컴포넌트를 따르고, `PhysType_Simulated`는 컴포넌트와 무관하게 시뮬레이션한다(Paragon 미니언이 배치 직후 늘어지는 원인) | `PhysicsCore/Public/BodySetupEnums.h:26-31` |
| 바디 설정 접근 | `UPhysicsAsset::SkeletalBodySetups`는 `UPROPERTY(instanced)`뿐이라 Python `get_editor_property`로 읽을 수 없다. 바디 타입을 Python으로 바꾸지 못한다 | `Runtime/Engine/Classes/PhysicsEngine/PhysicsAsset.h:212-213` |
| 런처 엔진의 Test 구성 | 설치형 엔진 빌드의 게임 구성 기본값은 `Shipping;Development;DebugGame`. 런처 5.8.4(`G:\Epic Games\UE_5.8`)에도 Test 라이브러리가 없어 Test 패키지를 만들 수 없다 | `Engine/Build/InstalledEngineBuild.xml:104`, 런처 설치본 `Engine/Intermediate/Build/Win64/x64/UnrealGame/` |

## 10. 실행·도구에서 확인한 것(2026-11-06)

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| Test 구성 로그 | Test에서는 `NO_LOGGING = !USE_LOGGING_IN_SHIPPING`이라 로그 파일이 생기지 않는다. 러너는 요약을 `Saved/Lab/<라벨>.summary.txt`와 CSV 메타데이터(`LabAlive`, `LabRespawns`, `LabOk`)로 남긴다 | `Core/Public/Misc/Build.h` `UE_BUILD_TEST` 블록 `NO_LOGGING` |
| CSV 형식 | 첫 열 `EVENTS`. 캡처 중 새 스탯이 생기면 뒤 행일수록 열이 늘어나고, 끝의 헤더 행이 전체 열 목록이다. 메타데이터 행의 키는 소문자(`[labn],500`) | 실행 결과(시험 측정) |
| CSV 파일 이름 | `FCsvProfiler::BeginCapture(-1, 폴더, 파일이름)`. 확장자는 압축 여부로 붙는다. `csvprofile STARTFILE=`로도 정할 수 있다 | `Core/Private/ProfilingDebugging/CsvProfiler.cpp` `HandleCSVProfileCommand`, `BeginCaptureInternal` |
| 정적 메시 변환 | `UAnimToTextureBPLibrary::ConvertSkeletalMeshToStaticMesh`는 커맨드렛에서 `-nullrhi`면 실패하고 `-AllowCommandletRendering`이 필요하다. 패키지 이름은 점 없는 경로여야 한다(`CreatePackage`에서 `check`) | `AnimToTextureBPLibrary.cpp:775-813`, `MeshUtilities.cpp:680` |
| Unreal MCP 툴셋 | 툴은 `list_toolsets`/`describe_toolset`/`call_tool` 셋으로 노출된다(`bEnableToolSearch` 기본 true). 블루프린트 그래프 DSL(`BlueprintTools.write_graph_dsl`)은 이벤트 그래프용이고 애님 그래프·스테이트 머신을 다루지 않는다. 애니메이션 툴셋은 Control Rig·Sequencer뿐이다 | 실행 중 `tools/call list_toolsets`, `get_graph_dsl_docs` |
| BP_TeslaCoil 구성 | 루트 아래 `SM_TorchBase`(z360, roll 180, x4, `M_Target_Metal`), 구체(z400, 오버레이 `MI_Mesh_Overlay_TeslaCoil`), `NS_TeslaCoil`(`User.PositionTarget`), `NS_Boundary_Cylinder`(`User.Radius`/`Height`/`Color`). GAS 상태 효과를 참조한다 | AssetHost에서 BP 인스턴스 컴포넌트 조회, 에셋 문자열 |
