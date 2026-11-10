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
| ISM Custom Data | `FMassInstancedStaticMeshInfo::AddBatchedCustomData<T>(…)`, `WriteCustomDataFloatsAtStartIndex`, `FStaticMeshInstanceVisualizationDesc::CustomDataFloats`. 커스텀 데이터는 위치와 같은 순서로 매 프레임 배치에 쌓이고, 플러시할 때 개수가 위치 수 × 인스턴스당 개수와 같은지 검사한다. 처음 적은 "상속하지 말고 뒤에 도는 별도 프로세서"는 근거가 없었다 **[계획 수정]**. City Sample은 `UMassUpdateISMProcessor`를 상속해 한 반복에서 `UpdateISMTransform`(public static)과 `AddBatchedCustomData`를 부르고 엔진 것을 ini에서 끈다 | `MassRepresentationTypes.h:561-589,665,700`, `MassVisualizationComponent.cpp:566-571`, `CitySampleMassCrowd/Private/MassCrowdUpdateISMVertexAnimationProcessor.cpp`, `CitySample/Config/DefaultMass.ini:131-138` |
| 프로세서 자동 등록 실행 시 변경 | `UMassProcessor::SetShouldAutoRegisterWithGlobalList(bool)`. CDO에서만 유효하다 | `MassEntity/Public/MassProcessor.h:180-185` |
| `UMassUpdateISMProcessor` | 게임 스레드 전용(`bRequiresGameThreadExecution`), Representation 그룹 뒤, 보이는 개체가 있는 청크만, `FMassStaticRepresentationTag`가 있는 개체는 뺀다 | `MassRepresentation/Private/MassUpdateISMProcessor.cpp:15-37` |
| AnimToTexture 자동 재생 | `FAnimToTextureAutoPlayData`(TimeOffset, PlayRate, StartFrame, EndFrame, float 4개)를 인스턴스 커스텀 데이터로 넣고 프레임은 `(Time + TimeOffset) * PlayRate * SampleRate`. 굽기는 `UAnimToTextureBPLibrary::AnimationToTexture(DataAsset)`(BlueprintCallable). 모드 `Vertex`/`Bone`(기본 Bone), 정밀도 기본 8비트, 본 영향 기본 4 | `AnimToTexture/Public/AnimToTextureInstancePlaybackHelpers.h`, `Private/AnimToTextureInstancePlaybackHelpers.cpp:29,90`, `AnimToTextureDataAsset.h:44-70,192,201`, `AnimToTextureEditor/Public/AnimToTextureBPLibrary.h:31` |
| AnimToTexture 바운드 | Bone 모드는 정적 메시 바운드를 뼈 위치 범위(`BoneMinBBox`/`BoneSizeBBox`)로만 넓히고, Vertex 모드는 모든 프레임의 정점 범위로 넓힌다. 뼈에서 먼 정점(무기 끝)은 Bone 바운드 밖으로 나가 Virtual Shadow Map이 그 페이지를 무효화하지 않는다(죽기 애니 중 칼날 그림자가 남음, 2026-11-08) | `AnimToTextureBPLibrary.cpp:245,290,1108`, `Measurements/M3/shots/vat-shadow/README.md` |
| 엔진 군중 표현 기본값 | High 고품질 액터, Medium 저품질 액터, Low ISM, Off 없음. 거리(화면 밖) 0/5/10/50m, (화면 안) 0/10/50/100m, 최대 수 10/20/500/무제한. `bKeepLowResActors = true` | `Engine/Plugins/AI/MassCrowd/Source/MassCrowd/Private/MassCrowdVisualizationTrait.cpp:16-38` |
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
| 가변 틱 | `FMassSimulationVariableTickParameters::TickRates[EMassLOD::Max]`, `TMassLODTickRateController`가 청크 단위로 이 프레임에 돌릴지 결정. **기본은 꺼져 있다**(`bEnableVariableTicking = false`). 켜면 Trait이 `FMassSimulationVariableTickFragment`와 청크 프래그먼트를 붙인다. 델타는 누적이다(`TickRate.DeltaTime = Time − LastTickedTime`). 그 델타와 청크 필터 `ShouldTickChunkThisFrame`를 쓰는 프로세서만 건너뛴다 | `MassLOD/Public/MassLODTrait.h:52`, `MassLODTrait.cpp:144`, `MassLODTickRateController.h` `UpdateTickRateFromLOD`(2026-11-09 확인) |
| LOD 태그 | `FMassOffLODTag` 등은 `bSetLODTags` 또는 가변 틱을 켤 때만 붙는다. 없으면 어떤 프로세서도 LOD를 모른다 | `MassLODTrait.cpp:129` |
| Simulation LOD 기본값 | 거리 0 / 50 / 100 / 300m, 상한 High 100 / Medium 200 / Low 300 / Off 무제한, 틱 0 / 0.5 / 1.0 / 1.5초, 히스테리시스 10% | `MassLOD/Private/MassSimulationLOD.cpp:20-39` |
| Off 개체 스냅 | `UMassOffLODNavigationProcessor`가 Off 태그 엔티티의 위치를 매 틱 `MoveTarget.Center`로 옮긴다. 자동 등록(기본 true). 우리 Center는 오브라 Off를 두면 안 된다 | `MassAI/.../MassNavigation/Private/MassNavigationProcessors.cpp:23-61`, `MassEntity/Public/MassProcessor.h:252` |
| TickRates를 읽는 엔진 프로세서 | Off 내비게이션, 부드러운 회전, SimpleMovement뿐. `UMassApplyMovementProcessor`·조향은 Off만 제외하고 매 프레임 프레임 델타(상한 0.1초) | `MassNavigationProcessors.cpp`, `MassSmoothOrientationProcessors.cpp`, `MassMovement/Private/MassSimpleMovementTrait.cpp:43`, `MassMovementProcessors.cpp:41,87,100`, `MassSteeringProcessors.cpp:86` |
| 회피는 High만 | `UMassMovingAvoidanceProcessor`·장애물 회피 쿼리가 Medium·Low·Off 태그를 제외한다. LOD 태그가 있어야 동작한다 | `MassNavigation/Private/MassAvoidanceProcessors.cpp:301-303,1125-1127`(2026-11-09 확인) |
| City Sample의 틱 절약 | 엔진에 맡기지 않고 자기 프로세서가 청크 LOD를 읽어 갱신 간격을 정한다(Off면 1.9\~2.1초) | `MassAIBehavior/Private/MassZoneGraphAnnotationFragments.cpp:19-48` |
| Avoidance 상태 | Experimental | `MassAI.uplugin` |
| Trait 값을 코드로 넣는 길 | `UMassSimulationLODTrait`의 `Params`·`bEnableVariableTicking`·`VariableTickParams`는 `protected`다. 밖에서 못 쓰므로 자식 클래스를 만들어 채운다. `FMassSimulationLODParameters`는 `LODDistance[4]`·`BufferHysteresisOnDistancePercentage`(10)·`LODMaxCount[4]`·`bSetLODTags`, `FMassSimulationVariableTickParameters`는 `TickRates[4]`·`bSpreadFirstSimulationUpdate`(false) | `MassLOD/Public/MassLODTrait.h:47-57`, `MassSimulationLOD.h:83-120`(2026-11-10 확인) |
| 새 엔티티는 Off에서 시작 | 가변 틱이나 `bSetLODTags`를 켜면 템플릿이 `LOD = Off`와 `FMassOffLODTag`로 시작한다. LOD 프로세서가 실제 LOD를 계산해 태그 교체를 지연 명령으로 넣으므로, 새로 만든(재생성 포함) 엔티티는 첫 프레임 동안 Off 아키타입에 있다. 그 프레임에 `UMassOffLODNavigationProcessor`가 위치를 `MoveTarget.Center`로 옮기고(청크 프래그먼트의 `bShouldTickThisFrame` 기본값이 true라 필터를 통과한다), 이동·조향은 Off를 제외한다. Off 밴드를 두지 않아도 이 한 프레임은 생긴다 | `MassLODTrait.cpp:128-133`, `MassLODFragments.h:90`, `MassNavigationProcessors.cpp:35-37,61`(2026-11-10 확인) |
| 청크가 이 프레임에 도는 조건 | `bShouldTickThisFrame = TimeUntilNextTick <= 0 \|\| 청크 수정 번호가 바뀜`. 수정 번호는 그 청크에 엔티티가 들어오거나 나갈 때마다 오른다(`AddMultipleInstances`·`RemoveMultipleInstances`). 그래서 태그가 바뀌어(피격·사망·LOD 전환) 엔티티가 드나든 청크는 틱 간격과 무관하게 다음 프레임에 돈다. 간격에는 ±10% 난수가 붙는다(`FApp::bUseFixedSeed`면 고정) | `MassLODTickRateController.h:127-154`, `MassEntity/Internal/MassArchetypeData.h:276-287`, `MassCommon/Private/MassCommonUtils.cpp:38-41`(2026-11-10 확인) |
| 누적 델타가 갱신되는 때 | `FMassSimulationVariableTickFragment::DeltaTime`은 청크가 돈 **다음 프레임**에 `Time − LastTickedTime`으로 갱신된다(`bWasChunkTicked`일 때만). 합은 흐른 시간과 같지만 값은 한 주기 늦고, 엔티티가 다른 청크로 옮겨 가면(태그 변경) 옮기기 전의 값을 그대로 들고 간다. 상태가 바뀐 직후의 델타에는 이전 상태에서 흐른 시간이 섞인다 | `MassLODTickRateController.h:157-171` |
| Simulation LOD는 거리만 본다 | `FMassSimulationLODLogic`은 `FLODDefaultLogic` 그대로다(`bDoVisibilityLogic = false`). 화면 안팎을 가리지 않고 가장 가까운 뷰어까지의 거리로 LOD를 정한다. 표현 LOD(`FMassRepresentationLODLogic`)만 시야를 본다 | `MassLOD/Public/MassLODLogic.h:29-54` |
| 청크 필터 기본값 | `ShouldTickChunkThisFrame`는 청크 프래그먼트가 없으면 true(항상 돈다), `GetChunkLOD`는 없으면 High | `MassSimulationLOD.h:68-78` |

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
| 툴 스크립트 | `editor_toolset…ProgrammaticToolset.execute_tool_script`는 `execute_tool()`로 다른 툴을 묶어 부를 뿐 `unreal` 모듈을 들여오지 못한다(허용 `copy`·`json`·`re`·`time`·`datetime`·`math`). 콘솔 명령을 실행하는 툴도 없다. 에디터 Python이 필요하면 커맨드렛으로 돌린다 | 실행 중 `get_execution_environment`(2026-11-09) |
| 블루프린트 프로퍼티 | `ObjectTools.get_properties`·`set_properties`는 블루프린트 경로를 받으면 CDO(`Default__X_C`)로 바꿔 읽는다. 블루프린트 에셋 자체의 프로퍼티(AnimBP `TargetSkeleton` 등)에는 닿지 않는다. 그래프 노드 경로(`…:AnimGraph.AnimGraphNode_SequencePlayer_0`)는 노드 그대로 읽고 쓴다 | 실행 결과(2026-11-09) |
| AnimBP 스켈레톤 | `UAnimBlueprint::TargetSkeleton`은 `EditAnywhere`라 Python `set_editor_property("target_skeleton", …)`로 바꿀 수 있다. AnimBP를 복제해 다른 스켈레톤으로 옮길 때 쓴다(`Scripts/create-anim-bp-large.md`) | `Runtime/Engine/Classes/Animation/AnimBlueprint.h:90-91` |

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
| Unreal MCP 툴셋 | 툴은 `list_toolsets`/`describe_toolset`/`call_tool` 셋으로 노출된다(`bEnableToolSearch` 기본 true). 블루프린트 그래프 DSL(`BlueprintTools.write_graph_dsl`)은 이벤트 그래프용이고 애님 그래프·스테이트 머신을 다루지 않는다. 애니메이션 툴셋은 Control Rig·Sequencer뿐이다. 다만 DSL이 아닌 개별 그래프 툴(`get_node_infos`, `create_node`, `connect_pins`, `compile_blueprint`)과 `ObjectTools.set_properties`는 애님 그래프 노드에도 듣는다. 2026-11-08 이것으로 시퀀스 플레이어의 숨은 핀(`ShowPinForProperties`)을 드러내 변수에 잇고 블렌드 리스트 설정을 바꿨다(`Scripts/bind-anim-start.md`). 프로퍼티 바인딩(`UAnimGraphNodeBinding_Base::PropertyBindings`)은 리플렉션에 노출되지 않아 쓰지 못한다 | 실행 중 `tools/call list_toolsets`, `get_graph_dsl_docs`. `AnimGraph/Private/AnimGraphNodeBinding_Base.h:38-39` |
| BP_TeslaCoil 구성 | 루트 아래 `SM_TorchBase`(z360, roll 180, x4, `M_Target_Metal`), 구체(z400, 오버레이 `MI_Mesh_Overlay_TeslaCoil`), `NS_TeslaCoil`(`User.PositionTarget`), `NS_Boundary_Cylinder`(`User.Radius`/`Height`/`Color`). GAS 상태 효과를 참조한다 | AssetHost에서 BP 인스턴스 컴포넌트 조회, 에셋 문자열 |

## 11. M1 Mass 코어에서 확인한 것(2026-11-07)

| 항목 | 내용 | 근거 |
| --- | --- | --- |
| 실행 시 EntityConfig | `UMassEntityConfigAsset::AddTrait(TSubclassOf<UMassEntityTraitBase>)`는 에디터 전용(`WITH_EDITOR`)이다. 패키지에서는 `FMassEntityConfig::AddTrait(UMassEntityTraitBase&)`(`:61`)에 `NewObject`로 만든 Trait을 넘긴다. 소유자가 있으면 `ConfigGuid`를 경로로 결정적으로 만들고 템플릿 ID는 그 GUID로 만든다 | `MassSpawner/Public/MassEntityConfigAsset.h:159`, `Private/MassEntityConfigAsset.cpp:33,137` |
| 기본으로 꺼진 프로세서 | `UMassLODCollectorProcessor`, `UMassVisualizationLODProcessor`, `UMassRepresentationProcessor`(하위 `UMassVisualizationProcessor`)는 `bAutoRegisterWithProcessingPhases = false`다. ISM 표현을 쓰려면 `Config/DefaultMass.ini`에서 켠다. 안 켜면 로직은 돌지만 아무것도 그려지지 않는다 | `MassLODCollectorProcessor.cpp:14`, `MassVisualizationLODProcessor.cpp:13`, `MassRepresentationProcessor.cpp:54`. 프로세서는 `config=Mass, defaultconfig`(`MassEntity/Public/MassProcessor.h:78`) |
| 이동 프로세서의 시간 상한 | `UMassApplyForceProcessor`와 `UMassApplyMovementProcessor`는 프레임 시간을 0.1초로 자른다. 프레임이 100ms를 넘으면 엔티티가 실제보다 느리게 간다. CharacterMovement는 하위 스텝으로 나눠 이 손실이 없다. 개발 빌드(N=2000, 평균 64ms, 자주 100ms 초과)에서 M1의 범위 처치가 M0의 약 60%로 나온 원인이다. N=300(평균 7.6ms)에서는 처치·재생성 수가 M0과 같았다 | `MassMovement/Private/MassMovementProcessors.cpp:41,100`. 실행: 2026-11-07 개발 빌드 `-LabStage=M1` |
| 엔진 조향의 이동 목표 | `UMassSteerToMoveTargetProcessor`는 `FMassMoveTargetFragment`의 `Center`·`Forward`·`DesiredSpeed`를 쓰고, `IntentAtGoal`이 `Stand`일 때만 도착 감속한다. 이동 프로세서는 `FMassCodeDrivenMovementTag`가 있는 엔티티만 옮긴다(태그를 빼면 멈춘다) | `MassNavigation/Private/MassSteeringProcessors.cpp:62-87,145-215`, `MassMovementProcessors.cpp:88` |
| ISM용 머티리얼 사용 플래그 | 바탕 머티리얼에 `bUsedWithInstancedStaticMeshes`가 꺼져 있으면 쿠킹하지 않은 `-game` 실행에서 ISM이 기본(회색) 머티리얼로 그려진다. `Scripts/make-static-minion.py`가 켠다 | 실행: 2026-11-07 스크린샷 |
| ACharacter 캡슐 | 기본 반지름 34, 반높이 88 | `Engine/Source/Runtime/Engine/Private/Character.cpp:78` |
| Lumen 장면 포함 조건 | 소프트웨어 Lumen은 `AffectsDynamicIndirectLighting()`이 참이고 거리 필드(또는 하이트필드) 표현을 지원하는 프리미티브만 장면에 넣는다. 거리 필드 지원은 스태틱 메시·Nanite·스플라인 프록시만 켠다. 스켈레탈 메시 캐릭터는 원래 Lumen 장면 밖이다 | `Engine/Private/PrimitiveSceneProxy.cpp:1701-1734`, `StaticMeshSceneProxy.cpp:340`, `Rendering/NaniteResources.cpp:1035` |
| 움직이는 ISM과 Lumen | M1 N=2000(Test 패키지 시험): 미니언 ISM이 Lumen 장면에 있으면 GPU 29.0ms, `bAffectDynamicIndirectLighting`만 끄면 6.0ms, `bAffectDistanceFieldLighting`만 끄면 5.8ms, Lumen 전체를 끄면 4.0ms. 어느 Lumen 패스가 시간을 쓰는지는 GPU 트레이스로 아직 보지 않았다 | `Saved/Profiling/CSV/trial/`(t-indoff, t-dfoff, t-nolumen, trytwo-r1) |
| City Sample의 레이 트레이싱 ISM | `r.RayTracing.Geometry.InstancedStaticMeshes=0`("Disable ISM")으로 모든 ISM을 레이 트레이싱 장면에서 뺀다. Niagara도 같은 이유로 끈다 | `../unreal-horde-references/CitySample/Config/DefaultEngine.ini:116-123` |
| CharacterMovement 하위 스텝 | `MaxSimulationTimeStep = 0.05`, `MaxSimulationIterations = 8`. 프레임이 50ms를 넘으면 한 틱에 여러 번 시뮬레이션한다. E0 full P1에서 미니언당 이동 비용이 2k(프레임 약 38ms) 8.9µs → 5k(약 150ms) 19.3µs로 늘어, Game Thread가 N보다 빠르게 는다(2k → 5k에 CharacterMovement 5.4배, Animation·TickActors 2.8배) | `Engine/Private/Components/CharacterMovementComponent.cpp:698-699`, 나누는 규칙과 8회째에 남은 시간을 몰아 쓰는 것은 `GetSimulationTimeStep` `:8162-8186`. 수치는 `Measurements/M0/runs.csv`의 fx 실행 CSV |
| URO의 효과 범위 | E0 uro N=1000 P1: Game Thread Animation 4.1 → 3.0ms, CharacterMovement·TickActors는 그대로라 전체 18.4 → 17.5ms. 거리 단계는 `FAnimUpdateRateParameters` 기본값 | `Engine/Classes/Engine/EngineTypes.h:2768-2785`, M0-uro-N1000-P1fxb CSV |
| 병렬 Mass의 게임 스레드 스핀 대기 | 프로세서가 쿼리를 시작할 때 `WaitForQueryResources`가 쿼리·청크 경합 인덱스를 예약하고, 다른 프로세서가 쥐고 있으면 `WaitForChange`가 `FPlatformProcess::YieldThread()` 루프로 돈다. 게임 스레드에서 돌면 그 시간은 CSV의 그 프로세서 스탯에 들어가는 실제 게임 스레드 비용이다(M3 `diag-vt*`: VAT ISM 프로세서 1.41 → 3.68ms, 직렬 1.35ms). 읽기 요구가 겹치는 프래그먼트가 많을수록 경합이 는다 | `Engine/Source/Runtime/MassEntity/Private/MassProcessingQueueTypes.cpp:678`, `Public/MassProcessingQueueTypes.h:760-778`(2026-11-09 확인) |
| Game Thread 시간과 워커 대기 | CSV `GameThreadTime`은 게임 스레드가 태스크를 기다린 시간(`Exclusive/GameThread/EventWait/*`)을 뺀다. Mass 작업이 워커 스레드에서 길어지면 Game Thread p50은 작게 나오고 프레임만 길어진다. E1 `massav` N=20k: GameThreadTime 9.3ms, `EventWait/PrePhysics` 46.5ms, 프레임 57.7ms | `Measurements/M1/runs.csv`, M1-massav-N20000-P1 CSV |
| 병렬 Mass의 질의 대기 | `FMassEntityQuery::ForEachEntityChunk`는 `FScopedEntityQueryContext`에서 `PreQueryCallback`을 먼저 부르고, 병렬 처리 큐에서는 이것이 `FProcessorWorker::WaitForQueryResources`로 같은 자원을 쥔 다른 프로세서가 끝날 때까지 기다린다. 그 시간은 기다린 프로세서의 실행 시간에 잡힌다. M3 20k P1에서 VAT ISM 프로세서가 액터 유무로 1.41 / 3.68ms였는데 첫 청크 전 대기가 0.006 → 2.06ms였고, `mass.FullyParallel=0`이면 1.35 / 1.35ms였다 | `MassEntity/Private/MassEntityQuery.cpp:39-47,258-276`, `MassProcessingQueueTypes.cpp:640-650,678-690`. diag-vtC/vtD/vtE/vtF CSV |
| 하이드레이션 액터 캡슐 | `ACharacter` 캡슐은 `Pawn` 프로필에 `SetShouldUpdatePhysicsVolume(true)`다. M3 20k P1(액터 495개, 3회): 이동 1.22ms, 프레임 15.78. 오버랩·물리 볼륨 갱신을 끄면 15.64, QueryOnly를 더하면 15.49, 콜리전을 끄면 14.23(이동 0.43, EndPhysics 대기 0.52 → 0.06) | `Engine/Private/Character.cpp:77-84`. diag-vtG/vtH/vtJ/vtI CSV, `-LabActorMoveCheap`·`-LabActorQueryOnly`·`-LabActorNoCollision` |

## 12. M3b 코드 정비에서 확인한 것(2026-11-09)

| 항목 | 내용 | 근거 |
| --- | --- | --- |
| 프로세서 그룹 의존성 | `ExecuteAfter("그룹 이름")`은 솔버가 그 그룹의 자식 노드 전부로 펼친다. 그룹 노드의 의존성은 자식에게 내려간다. 그룹 안의 프로세서는 자기 `ExecuteAfter`를 따로 가질 수 있다(Contact가 LabLogic 그룹이면서 Movement 뒤) | `MassEntity/Private/MassProcessorDependencySolver.cpp:585-623,680-760` |
| 지연 명령 종류 | `FMassDeferredSetCommand`(람다, 플러시 때 실행), `FMassCommandSwapTags<TOld, TNew>`, `FMassCommandAddTag`. 람다 안에서 `GetFragmentDataChecked`로 쓰는 것이 "쿼리 밖 쓰기"를 지연으로 바꾸는 길이다 | `MassEntity/Public/MassCommands.h:65,93,1294,1359,1673-1743` |
| 기본 커맨드 버퍼 | `FMassEntityManager::Defer()`는 게임 스레드에서만 쓴다. 스포너 서브시스템 틱(액터 스폰 콜백)에서 태그를 붙일 때 이것을 쓴다 | `MassEntity/Public/MassEntityManager.h:910,1625` |
| 태그 중복 추가 | `AddTagToEntity`는 아키타입에 이미 그 태그가 있으면 아무것도 하지 않는다 | `MassEntity/Private/MassEntityManager.cpp:1404-1416` |
| 액터가 붙는 길목 | 새 스폰과 풀 재사용 모두 `UMassActorSpawnerSubsystem`이 `PrepareForGame`을 부른 뒤 같은 PostSpawn 델리게이트(`UMassRepresentationActorManagement::OnPostActorSpawn`)를 지난다. 액터를 떼는 `ReleaseActorOrCancelSpawning`은 static이라 하위 클래스가 끼어들 수 없다 | `MassActors/Private/MassActorSpawnerSubsystem.cpp:255,480`, `MassRepresentation/Private/MassRepresentationProcessor.cpp:481`, `MassRepresentationActorManagement.cpp:92` |
| 액터 재사용 | 표현이 액터로 돌아올 때 `FMassActorFragment`에 액터가 남아 있으면 스폰 요청 없이 그 액터를 다시 쓴다(`NewActor = Actor`). 그때는 PostSpawn 콜백이 불리지 않는다 | `MassRepresentationProcessor.cpp` UpdateRepresentation의 HighRes/LowRes 분기 |
| 표현 LOD 파라미터 | `FMassVisualizationLODParameters`: `BaseLODDistance`(화면 밖), `VisibleLODDistance`(화면 안), `BufferHysteresisOnDistancePercentage`(기본 10), `LODMaxCount`. Trait의 `LODParams` | `MassRepresentation/Public/MassRepresentationFragments.h:299-312`, `MassVisualizationTrait.h:52` |
| 옵저버 잠금 CVar | `mass.commands.LockObserversDuringFlushing`, 기본 false. 제거 옵저버가 없으면 켜도 된다는 주석 | `MassEntity/Private/MassCommandBuffer.cpp:17-25` |
| DefaultEngine.ini의 CVar 섹션 | `[ConsoleVariables]` 섹션을 GEngineIni에서 읽어 `SetBySystemSettingsIni`로 적용한다. City Sample이 같은 섹션에 옵저버 잠금을 둔다 | `Core/Private/HAL/ConsoleManager.cpp:607,697-698`, `CitySample/Config/DefaultEngine.ini:405` |
| 등록된 프로세서 보기 | `UMassCompositeProcessor`가 그래프를 처음 만들 때 `LogMass`에 "flat processing graph"로 노드와 의존성을 찍는다(`mass.LogNewProcessingGraph` 기본 1). `LogMass` 기본 수준이 Warning이라 `-LogCmds="LogMass Log"`가 필요하다. 우리 프로세서는 모두 PrePhysics 단계다 | `MassEntity/Private/MassProcessor.cpp:22-31,555-575`, `MassEntityTypes.h:26` |
| VAT 프레임 → 텍스처 행 | 머티리얼 함수 `BoneIndexUVs`의 Custom 노드: `UV.y = (0.5 / TextureSize.y) + (int(Frame) / TextureSize.y)`. 정수로 자르고 텍스처 필터는 `TF_Nearest`다. 그래서 걷기 루프의 End + 0.99는 End 행이고 다음 클립과 섞이지 않는다 | `AnimToTexture/Content/Materials/MaterialFunctions/BoneIndexUVs.uasset`(문자열), `AnimToTextureEditor/Public/AnimToTextureUtils.h:269` |
| GPU 드라이버 전역 | `GRHIAdapterName`, `GRHIAdapterUserDriverVersion`, `GRHIAdapterInternalDriverVersion`, `GRHIAdapterDriverDate` | `RHI/Public/RHIGlobals.h:819-822` |
| UBT ExternalDependencies | `ModuleRules.ExternalDependencies`에 넣은 파일이 바뀌면 Build.cs를 다시 평가한다(`.git/HEAD`로 커밋 해시 헤더 생성에 씀) | `Source/Programs/UnrealBuildTool/Configuration/Rules/ModuleRules.cs:1439` |
