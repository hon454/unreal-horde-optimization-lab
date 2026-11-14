# 엔진 소스 확인 기록

엔진 소스와 실행에서 확인한 사실만 적는다. 기억이나 문서의 주장은 적지 않는다. 경로는 엔진 루트 기준이고, 줄 번호는 GitHub `5.8.4-release` 태그에서 읽었다(2026-11-06). PC의 소스 빌드도 5.8.4다(pc-specs.md 2.3). 계획이 틀렸던 항목은 **[계획 수정]**으로 표시했다.

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
| 전체 병렬 | **`mass.FullyParallel`**(기본 `MASS_DO_PARALLEL`). 계획의 `mass.ParallelProcessing`은 없다 **[계획 수정]** | `Engine/Source/Runtime/MassEntity/Private/MassProcessingPhaseManager.cpp:31,36,576` |
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
| ISKM 트랙 풀 누수 | 5.8.4에도 남아 있다. 트랙 할당은 엔티티 키가 트랙 맵에 없을 때, 해제(`DeallocateTrack`)는 `RemoveInstance` 입력에서만이고 그 호출자는 `FMassStaticRepresentationTag`를 요구하는 정적 경로뿐이다. 움직이는 개체를 파괴하면 트랙이 남는다. 우회: 엔티티 풀링(같은 핸들을 다시 쓰면 기존 트랙을 다시 쓴다). 엔진은 `SetAutoPlayData`를 시퀀스 번호가 바뀔 때만 부른다 | `MassRepresentation/Private/MassVisualizationComponent.cpp` 773·864(할당), 1156(해제) |
| ISKM 블렌딩 | BlendTime 크로스페이드와 BlendSpace 트랙이 있다. 없는 것은 AnimBP다. 계획의 "블렌딩 없음"은 틀렸다 **[계획 수정]** | `Engine/Source/Runtime/Engine/Classes/Animation/AnimSequenceTransformProviderData.h:104-127`(`FAnimSequenceTrackAutoPlayData::BlendTime`), `:152-174`(`FAnimSequenceTrackBlendSpaceData`) |
| 엔진 기본 `LODMaxCount` | High/Medium/Low 기본 `{50, 100, 500}`. 밴드 거리보다 이 상한이 먼저 걸린다. 프로젝트 값을 명시한다 | `MassLODTrait.h` / `MassRepresentationParameters`(M0에서 줄 번호 확인) |
| 표현 LOD 경계 히스테리시스 기본값(2026-11-13, 03번 글) | `FMassVisualizationLODParameters::BufferHysteresisOnDistancePercentage = 10.0f`. 거리 경계에 이 퍼센트의 완충을 둔다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation/Public/MassRepresentationFragments.h:309` |
| 계산기의 히스테리시스 비율(2026-11-13, 07번 글) | `BufferHysteresisOnDistanceRatio = 0.1f`. LOD 계산기가 쓰는 비율 기본값이다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassLOD/Public/MassLODCalculator.h:271` |
| ISM 커스텀 데이터 개수 검사 줄(2026-11-13, 03번 글) | `NumCustomDataFloats`를 `StaticMeshInstanceCustomFloats.Num() / UpdateInstanceIds.Num()`로 구하고, 0이 아니면 `ensure`로 `Num() == NumCustomDataFloats * UpdateInstanceIds.Num()`을 검사한 뒤 `SetNumCustomDataFloats`를 부른다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassVisualizationComponent.cpp:566-571` |
| 애니메이션 예산의 계산(2026-11-13, 04번 글) | 가용 예산 = `BudgetInMs - ExternalAnimationTimeSpent`(`:558`). 작업 단위 예산 = `max(가용 예산 / 평균 작업 단위 시간, 이상적 작업 단위 × MinQuality)`(`:561`). 초과분 `WorkUnitsExcess`가 0보다 클 때만 매 프레임 갱신하는 몫을 줄인다(`:566`). 정렬은 `bAlwaysTick`이 먼저, 그다음 중요도가 큰 순이다(`:339`, 3절 위의 `:327`은 주석 줄) | `Engine/Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator/Private/AnimationBudgetAllocator.cpp:558,561,566,339` |
| 애니메이션 예산의 기본값 | `FAnimationBudgetAllocatorParameters::BudgetInMs = 1.0f` | `Engine/Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator/Public/AnimationBudgetAllocatorParameters.h:19` |
| 예산 할당기에 등록되는 것(2026-11-13, 08번 글) | `IAnimationBudgetAllocator::RegisterComponent`는 `USkeletalMeshComponentBudgeted`만 받는다. 등록하면 기본 틱이 꺼지고 URO가 꺼지며 병렬 애니메이션 작업이 할당기로 간다. ISKM은 예산 밖이다 | `Engine/Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator/Public/IAnimationBudgetAllocator.h:22-27` |

## 4. 액터 풀링

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 풀 반환 동작 | `UMassActorSpawnerSubsystem`의 풀링은 액터를 숨기고 보관한다. 컴포넌트 등록과 틱은 유지된다. `USkeletalMeshComponentBudgeted`가 붙어 있으면 숨긴 뒤에도 예산에 남는다 | `MassActors/Private/MassActorSpawnerSubsystem.cpp`(M3 전에 줄 번호 확인) |
| 대처 | `IMassActorPoolableInterface::PrepareForPooling`에서 중요도 0, 틱 끔. `PrepareForGame`에서 복원 | `MassActors/Public/MassActorPoolableInterface.h` |
| 풀에서 꺼낸 액터는 바로 보인다(2026-11-13, 04번 글) | `SetActorHiddenInGame(false)`, `SetActorTransform(…, ETeleportType::ResetPhysics)`, `PrepareForGame` 순이다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassActors/Private/MassActorSpawnerSubsystem.cpp:252-255` |
| `TeleportActor` 기본 구현 | 위치가 다르면 지연 커맨드로 `SetActorTransform(…, ETeleportType::TeleportPhysics)`를 부른다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassRepresentationActorManagement.cpp:47-56` |
| `IsActorReadyForRepresentation` 기본 구현 | 항상 참이다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassRepresentationActorManagement.cpp:58-61` |
| 액터 준비를 기다리는 조건 | `bShouldDefer = (bDoWaitForActorVisualReadiness && bHasMeshFallback) \|\| !bActorReady`. 참이면 액터를 켜지 않고 표현을 유지한다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassRepresentationProcessor.cpp:275-298` |
| 준비 대기 두 플래그의 기본값 | `bKeepActorExtraFrame`(`:202`)과 `bWaitForActorVisualReadiness`(`:211`)는 모두 거짓이다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation/Public/MassRepresentationFragments.h:202,211` |

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
| 풀에서 꺼낸 액터의 변환 | 엔진은 풀에서 꺼낸 액터를 스폰 요청의 변환으로 `SetActorTransform`한 뒤 `PrepareForGame`을 부른다. 요청 변환은 엔티티 변환이라 스케일이 1이고, 액터에 넣어 둔 스케일이 지워진다. `BeginPlay`는 재사용 때 다시 불리지 않으므로 스케일은 `PrepareForGame`에서 다시 넣는다. 액터를 붙일 때와 준비가 끝날 때의 `TeleportActor`도 엔티티 변환 그대로 `SetActorTransform`한다. 풀은 액터 클래스가 키다 | `MassActors/Private/MassActorSpawnerSubsystem.cpp:239,253-255`, `MassRepresentation/Private/MassRepresentationSubsystem.cpp:170,213`, `MassRepresentationActorManagement.cpp:47-56`, `MassRepresentationProcessor.cpp:272,428`(2026-11-10 확인, 실행 `large_wrong_scale` 45/45 → 0) |
| 쿼리 순회에 개체 수 예산 걸기 | `UE::Mass::FExecutionLimiter(EntityLimit)`를 `FMassEntityQuery::ForEachEntityChunk(Context, Limiter, Func)`에 넘기면 한 번 부를 때 `EntityLimit`개까지만 돈다(걸친 청크는 끝까지 돈다). 리미터가 아키타입·청크 색인을 기억해 다음 호출은 멈춘 곳에서 이어 돌고, 끝에 닿으면 처음으로 돌아가 시작 청크 앞에서 멈춘다. 호출 사이에 엔티티 수나 배치가 바뀌면 건너뛰는 개체가 생길 수 있다고 주석에 적혀 있다. 엔티티 컬렉션을 직접 지정한 실행에는 쓸 수 없다. `Engine/Plugins`에서 쓰는 곳은 없다 | `MassEntity/Public/MassEntityTypes.h:40-64`, `MassEntityQuery.h:91-98`, `Private/MassEntityQuery.cpp:398-494`, `Private/MassArchetypeData.cpp:841-900`(2026-11-10 확인) |
| 청크 필터 기본값 | `ShouldTickChunkThisFrame`는 청크 프래그먼트가 없으면 true(항상 돈다), `GetChunkLOD`는 없으면 High | `MassSimulationLOD.h:68-78` |
| 가변 틱 필터를 쓰는 프로세서의 순서 | "이 프레임에 돈다"는 값은 `UMassSimulationLODProcessor`(LOD 그룹)가 한 번 돌 때 쓰고 다음에 돌 때 지운다. 필터를 쓰는 프로세서가 LOD 그룹 뒤에 돈다고 적지 않으면 두 프로세서의 순서가 프레임마다 바뀔 수 있고, 값을 한 번도 읽지 못한 채 주기 하나를 건너뛴다. 엔진의 StateTree·회피 프로세서는 `ExecuteAfter(ProcessorGroupNames::LOD)`를 적는다. 의존성 해결기는 청크 프래그먼트 접근도 보지만 아키타입이 겹치지 않으면 충돌로 보지 않는다 | `MassLOD/Private/MassSimulationLOD.cpp:76-77`, `MassLOD/Public/MassLODTickRateController.h:125,139-154`, `MassAIBehavior/Private/MassStateTreeProcessors.cpp:165`, `MassNavigation/Private/MassAvoidanceProcessors.cpp:288`, `MassEntity/Private/MassProcessorDependencySolver.cpp:360-361`(2026-11-10 확인. 개발 실행의 청크 진단 로그에서 Lab 상태 프로세서가 틱 한 번을 놓쳐 사망 전환이 0.9초·1.35초 늦었고, 순서를 적은 뒤 세 실행에서 다시 나오지 않았다) |
| 청크를 옮긴 뒤의 틱 일정 | 태그가 붙어 다른 청크로 간 엔티티는 그 청크의 일정을 따른다. 엔티티가 들어온 청크는 수정 번호가 바뀌어 다음 LOD 갱신에서 한 번 돌고, 그다음 갱신에서 `TimeUntilNextTick`이 그 LOD의 간격으로 다시 찬다. 그래서 들어온 뒤의 틱은 약 간격 × n + 2프레임에 온다(60fps 고정 실행: Medium 0.5초는 +0.53초·+1.05초, Low 1.0초는 +1.05초) | `MassLOD/Public/MassLODTickRateController.h:139-154`(2026-11-10 확인, 개발 실행의 청크 진단 로그) |
| 결정적 실행에서는 간격 난수가 없다 | `UE::Mass::Utils::IsDeterministic()`은 `FApp::bUseFixedSeed`이고 `-benchmark`·`-FixedSeed`·`-deterministic`이 켠다. 이때 가변 틱 간격의 ±10% 난수와 첫 갱신 분산 난수가 빠진다. Shipping에서는 늘 false다 | `MassCommon/Private/MassCommonUtils.cpp:38-41`, `MassCommon/Public/MassCommonUtils.h:43`, `Launch/Private/LaunchEngineLoop.cpp:2462`, `MassLODTickRateController.h:137,143` |
| 엔진 이동·조향 넷을 한꺼번에 빼는 태그는 `FMassOffLODTag`뿐이다 | 넷의 쿼리가 모두 `FMassOffLODTag`를 `None`으로 요구한다. `FMassCustomMovementTag`를 `None`으로 요구하는 것은 `UMassApplyMovementProcessor` 하나다(스프링 이동 프로세서도 같지만 이 프로젝트는 쓰지 않는다). `FMassLowLODTag`를 보는 것은 회피 프로세서뿐이다. `UMassSmoothOrientationProcessor`에는 Off 태그 전용 쿼리가 따로 있어 가변 틱이 온 프레임에만 회전을 `MoveTarget.Forward`로 맞춘다 | `MassGameplay/MassMovement/Private/MassMovementProcessors.cpp:33`(ApplyForce), `:86-88`(ApplyMovement), `MassAI/MassNavigation/Private/MassSteeringProcessors.cpp:85-87`, `MassSmoothOrientationProcessors.cpp:33`, `:36-40`, `:120-133`, `MassAvoidanceProcessors.cpp:302-303` |
| Off LOD의 엔진 이동은 순간 이동이다 | `UMassOffLODNavigationProcessor`는 Off 태그 엔티티를 가변 틱이 온 프레임에 `MoveTarget.Center`로 옮긴다. 속도와 누적 델타로 걷게 하는 엔진 프로세서는 없다. 이 프로젝트는 M4부터 이 프로세서를 등록하지 않는다(`HordeOptLab.cpp:35-38`) | `MassAI/MassNavigation/Private/MassNavigationProcessors.cpp:31-38`, `:60-61` |
| 가변 틱을 켜면 LOD 태그가 이미 붙고 바뀐다 | Trait이 `bEnableVariableTicking`이면 새 엔티티에 Off 태그를 붙이고, 틱 제어기가 청크의 LOD와 엔티티의 LOD가 다르면 `SwapTags`(옛 태그 → 새 태그) 명령을 넣는다. 밴드를 넘는 개체는 지금도 아키타입을 한 번 옮긴다. 태그를 밖에서 바꾸면 `checkfSlow`에 걸리고 `GetLODFromArchetype`은 Off 태그를 가장 먼저 본다 | `MassGameplay/MassLOD/Private/MassLODTrait.cpp:128-133`, `Public/MassLODTickRateController.h:112-119`, `:165-169`, `Private/MassLODUtils.cpp:19-23`, `Public/MassLODUtils.h:19-38` |
| Simulation LOD 계산기는 먼 밴드부터 고르고 히스테리시스는 한 단계 차이에만 있다 | `ComputeLODFromSettings`는 Off 거리부터 내려오며 처음 걸리는 밴드를 고른다. Low와 Off 거리가 같으면 Off가 된다. 히스테리시스(기본 10%)는 바로 아래 밴드에서 한 단계 올라갈 때만 적용되므로 Off → Medium(두 단계)에는 없다 | `MassGameplay/MassLOD/Public/MassLODCalculator.h:864-888` |
| 새 엔티티는 Simulation LOD에서만 히스테리시스를 받는다 | Simulation LOD는 LOD 태그를 쓰면 프래그먼트의 `LOD`를 Off로 두고 시작해 첫 계산의 `PrevLOD`가 Off다. Off와 Low 사이에 밴드가 있으면 Lerp(Low, Off, 0.9)보다 먼 새 엔티티는 Off로 남는다. 표현 LOD는 `PrevLOD`가 `Max`에서 시작해 첫 계산에 히스테리시스가 없다. 두 Off 거리를 같게 두면 그 사이의 새 엔티티는 그려지면서 Simulation LOD는 Off다(20k, 500m에서 프레임마다 약 550, `Docs/Worklog/M4.md` "그리는 거리 인자") | `MassLOD/Private/MassLODTrait.cpp:128-133`, `MassLOD/Public/MassSimulationLOD.h:25-28`, `MassRepresentation/Public/MassRepresentationFragments.h:37`, `MassLODCalculator.h:453-454,873-880` |
| 가변 틱 프래그먼트의 시각으로 "마지막으로 돈 뒤 흐른 시간"을 셈하면 모자란다 | 엔진은 청크가 돈 **다음** 갱신에서 `DeltaTime`과 `LastTickedTime`을 적는다. 그래서 프로세서가 읽는 `DeltaTime`은 한 주기 전의 간격이다. `지금 − LastTickedTime + 한 프레임`으로 셈해도 20,000 개체 Development 실행에서 자체 시각으로 셈한 값의 약 94%만 나왔다(실행 로그 `LabDev-m4offmove-N20000-P1.log`의 `lab_offmove`, 27초 동안 Off 밴드 개체·초 322,303, 자체 시각으로 옮긴 합 328,185, 엔진 시각으로 셈한 합 308,601). 빠지는 경로는 가르지 못했다 | `Public/MassLODTickRateController.h:157-164`, `Public/MassSimulationLOD.h:37` |
| Off 청크는 틱이 온 프레임에만 LOD를 다시 계산한다 | `ShouldCalculateLODForChunk`는 청크 LOD가 Off가 아니면 매 프레임 참이고, Off면 `ShouldTickThisFrame`일 때만 참이다. Off 밴드에서 안쪽으로 넘어온 개체는 그 청크의 다음 틱까지 Off로 남는다 | `Public/MassLODTickRateController.h:83-97`, `Private/MassSimulationLOD.cpp:90-91` |
| 조향 프로세서의 Off 쿼리 줄(2026-11-13, 05번 글) | `UMassSteerToMoveTargetProcessor`의 쿼리가 `FMassOffLODTag`를 `None`으로 요구하는 줄이다("No need for Off LOD to do steering") | `Engine/Plugins/AI/MassAI/Source/MassNavigation/Private/MassSteeringProcessors.cpp:85-86` |

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
| CSV `CPUUsage_Process`·`CPUUsage_Idle` | Shipping이 아니면 CSV 캡처 중 프레임마다 쓴다. Windows에서 `QueryProcessCycleTime`(게임 프로세스 스레드의 사이클)과 `QueryIdleProcessorCycleTime`(코어별 유휴 사이클 합)의 프레임 간 차이를 같은 분모(프레임 시간 × QPC 주파수 × 1000 × 논리 코어 수)로 나눈 값의 100배다. 분모 단위가 CPU 사이클과 달라 절댓값은 %가 아니지만 같은 프레임 안에서 두 값의 비는 의미가 있다. 유휴가 크게 줄었는데 프로세스 값이 늘지 않으면 그 사이클은 다른 프로세스(또는 DPC·인터럽트)가 썼다 | `Runtime/Launch/Private/LaunchEngineLoop.cpp:1456-1470`, `Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp:883-976` |
| 스레드 시간에 들지 않는 대기(2026-11-13, 00번 글) | `FWindowsPlatformProcess::Sleep`과 이벤트 대기는 `UE::Stats::FThreadIdleStats::FScopeIdle`로 감싸 유휴 시간으로 센다. 이벤트 대기(`FEventWin::Wait`)는 인자 `bIgnoreThreadIdleStats`(기본 거짓)를 `FScopeIdle`에 넘긴다 | `Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp:1870,1940` |
| 게임 스레드의 프레임 끝 동기화(2026-11-13, 00번 글) | `FFrameEndSync`는 게임 스레드를 N−1 렌더 스레드 프레임에 맞추는 울타리(`FRenderThreadFence`, `:2458-2473`)와, RHI 스레드·스왑체인보다 너무 앞서지 않게 하는 추가 울타리(`:2476-2478`)를 쓴다. `r.OneFrameThreadLag` 기본 1, `r.GTSyncType` 기본 0(`<= 0`이면 N−1 렌더 스레드 프레임 뒤에 N−m RHI 스레드 프레임, 도움말 `:2441-2450`) | `Engine/Source/Runtime/RenderCore/Private/RenderingThread.cpp:2435-2478` |

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
| 디스턴스 필드 오브젝트 버퍼 갱신의 잠금 | `FDistanceFieldSceneData::UpdateDistanceFieldObjectBuffers`는 움직인 인스턴스의 업로드 항목을 `ParallelFor`로 만들면서 범위가 둘 이상이면 항목마다 정적 `FCriticalSection DFUpdateCS`를 건다. 움직이는 스태틱 메시 ISM은 인스턴스 수만큼 이 경로를 탄다(프레임 CSV의 `Exclusive/RenderThread/PrepareDistanceFieldScene`). `Full1` `L2-mass` 2만에서 이 값이 실행마다 8.6 / 15.2ms로 갈렸다(Worklog Q6 "Full1 후속 분석"). 잠금이 원인인지는 확인하지 못했다 | `Engine/Source/Runtime/Renderer/Private/DistanceFieldObjectManagement.cpp:574-611` |
| City Sample의 레이 트레이싱 ISM | `r.RayTracing.Geometry.InstancedStaticMeshes=0`("Disable ISM")으로 모든 ISM을 레이 트레이싱 장면에서 뺀다. Niagara도 같은 이유로 끈다 | `../unreal-horde-references/CitySample/Config/DefaultEngine.ini:116-123` |
| CharacterMovement 하위 스텝 | `MaxSimulationTimeStep = 0.05`, `MaxSimulationIterations = 8`. 프레임이 50ms를 넘으면 한 틱에 여러 번 시뮬레이션한다. E0 full P1에서 미니언당 이동 비용이 2k(프레임 약 38ms) 8.9µs → 5k(약 150ms) 19.3µs로 늘어, Game Thread가 N보다 빠르게 는다(2k → 5k에 CharacterMovement 5.4배, Animation·TickActors 2.8배) | `Engine/Private/Components/CharacterMovementComponent.cpp:698-699`, 나누는 규칙과 8회째에 남은 시간을 몰아 쓰는 것은 `GetSimulationTimeStep` `:8162-8186`. 수치는 `Measurements/M0/runs.csv`의 fx 실행 CSV |
| 월드 델타의 상한 0.4초 | `MaxUndilatedFrameTime=0.4`(`Engine/Config/BaseGame.ini:199`). `AWorldSettings::FixupDeltaSeconds`가 월드 델타를 이 값으로 자른다(`WorldSettings.cpp:334-343`). 프레임이 0.4초를 넘으면(액터 1만, 441ms) CharacterMovement는 0.4초를 50ms × 8회로 나눠 하위 스텝 상한에 걸린다(2026-11-12, 글 01 조사에서 확인) |
| URO의 효과 범위 | E0 uro N=1000 P1: Game Thread Animation 4.1 → 3.0ms, CharacterMovement·TickActors는 그대로라 전체 18.4 → 17.5ms. 거리 단계는 `FAnimUpdateRateParameters` 기본값 | `Engine/Classes/Engine/EngineTypes.h:2768-2785`, M0-uro-N1000-P1fxb CSV |
| 병렬 Mass의 게임 스레드 스핀 대기 | 프로세서가 쿼리를 시작할 때 `WaitForQueryResources`가 쿼리·청크 경합 인덱스를 예약하고, 다른 프로세서가 쥐고 있으면 `WaitForChange`가 `FPlatformProcess::YieldThread()` 루프로 돈다. 게임 스레드에서 돌면 그 시간은 CSV의 그 프로세서 스탯에 들어가는 실제 게임 스레드 비용이다(M3 `diag-vt*`: VAT ISM 프로세서 1.41 → 3.68ms, 직렬 1.35ms). 읽기 요구가 겹치는 프래그먼트가 많을수록 경합이 는다 | `Engine/Source/Runtime/MassEntity/Private/MassProcessingQueueTypes.cpp:678`, `Public/MassProcessingQueueTypes.h:760-778`(2026-11-09 확인) |
| Game Thread 시간과 워커 대기 | CSV `GameThreadTime`은 게임 스레드가 태스크를 기다린 시간(`Exclusive/GameThread/EventWait/*`)을 뺀다. Mass 작업이 워커 스레드에서 길어지면 Game Thread p50은 작게 나오고 프레임만 길어진다. E1 `massav` N=20k: GameThreadTime 9.3ms, `EventWait/PrePhysics` 46.5ms, 프레임 57.7ms | `Measurements/M1/runs.csv`, M1-massav-N20000-P1 CSV |
| 병렬 Mass의 질의 대기 | `FMassEntityQuery::ForEachEntityChunk`는 `FScopedEntityQueryContext`에서 `PreQueryCallback`을 먼저 부르고, 병렬 처리 큐에서는 이것이 `FProcessorWorker::WaitForQueryResources`로 같은 자원을 쥔 다른 프로세서가 끝날 때까지 기다린다. 그 시간은 기다린 프로세서의 실행 시간에 잡힌다. M3 20k P1에서 VAT ISM 프로세서가 액터 유무로 1.41 / 3.68ms였는데 첫 청크 전 대기가 0.006 → 2.06ms였고, `mass.FullyParallel=0`이면 1.35 / 1.35ms였다 | `MassEntity/Private/MassEntityQuery.cpp:39-47,258-276`, `MassProcessingQueueTypes.cpp:640-650,678-690`. diag-vtC/vtD/vtE/vtF CSV |
| 하이드레이션 액터 캡슐 | `ACharacter` 캡슐은 `Pawn` 프로필에 `SetShouldUpdatePhysicsVolume(true)`다. M3 20k P1(액터 495개, 3회): 이동 1.22ms, 프레임 15.78. 오버랩·물리 볼륨 갱신을 끄면 15.64, QueryOnly를 더하면 15.49, 콜리전을 끄면 14.23(이동 0.43, EndPhysics 대기 0.52 → 0.06) | `Engine/Private/Character.cpp:77-84`. diag-vtG/vtH/vtJ/vtI CSV, `-LabActorMoveCheap`·`-LabActorQueryOnly`·`-LabActorNoCollision` |
| 이동 프로세서 시간 상한의 의도(2026-11-13, 02번 글) | 두 프로세서의 주석은 "Clamp max delta time to avoid force explosion on large time steps (i.e. during initialization)"이다. 0.1초 상한은 초기화 때의 큰 시간 간격에서 힘이 터지는 것을 막으려는 것이다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassMovement/Private/MassMovementProcessors.cpp:40,99` |
| 거리 필드 장면에 드는 조건(2026-11-13, 06번 글) | `PrimitiveNeedsDistanceFieldSceneData`: (전부 추적하거나 동적 간접 그림자를 드리우고) && 거리 필드 조명에 영향 && (게임에서 그려지거나 숨은 그림자·간접광이 있고) && (동적 그림자를 드리우거나 동적 간접광에 영향)이다. 프록시의 `bSupportsDistanceFieldRepresentation` 기본값은 거짓(`PrimitiveSceneProxy.cpp:394`)이고, 정적 메시 프록시가 불투명 머티리얼과 유효한 거리 필드 데이터가 있을 때 참으로 둔다(`StaticMeshSceneProxy.cpp:340`) | `Engine/Source/Runtime/Engine/Public/SceneManagement.h:1069-1082`, `Engine/Source/Runtime/Engine/Private/PrimitiveSceneProxy.cpp:394`, `Engine/Source/Runtime/Engine/Private/StaticMeshSceneProxy.cpp:340` |

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

## 13. 병렬 Mass의 프로세서 순서와 청크 충돌(2026-11-10, M4 탑뷰 조사)

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 순서가 정해지는 두 단계 | (1) 의존성 해결기가 phase마다 프로세서 배열을 만든다. 그룹 이름 `A.B.C`를 상위 그룹으로 펼쳐 선후를 만들고, 정렬은 `ExecutionPriority`(기본 0) 다음 `TotalWaitingNodes` 큰 순, 한 걸음마다 이미 놓인 노드와 자원이 충돌하지 않는 첫 노드를 뽑는다. (2) 실행 큐(`FProcessingQueue`)는 그 배열을 받아 **자기 의존성 그래프를 따로** 만든다. 이름 해석이 `ExecuteInGroup`과 **정확히 같은** 그룹·클래스 이름만 찾으므로(상위 그룹을 펼치지 않는다) 해결기가 만든 선후의 일부는 실행 때 효력이 없다. 예: `ExecuteAfter("Representation")`는 그룹이 `Representation.VisualizationProcessing`인 표현 프로세서를 기다리지 않는다 | `MassEntity/Private/MassProcessorDependencySolver.cpp:466-532,535,689-719,829-833`, `MassProcessingQueue.cpp:672-760`, `MassRepresentation/Public/MassRepresentationTypes.h:30`, `MassUpdateISMProcessor.cpp:20` |
| 준비된 프로세서를 고르는 규칙 | 게임 스레드를 요구하는 프로세서는 메인 스레드 러너가, 나머지는 비동기 러너가 맡는다. 각 러너는 `GetNextReadyProcessor`로 남은 색인 배열을 앞에서부터 훑어 `Ready`이고 프로세서 충돌(`ContentionIndex`)이 없는 첫 프로세서를 뽑고, 뽑은 색인을 `RemoveSingleSwap`으로 지운다. 배열의 마지막 원소가 지운 자리로 옮겨 가므로 이후 훑는 순서가 바뀐다. 선후가 선언되지 않은 프로세서는 늘 `Ready`라 먼저 뽑힐 수 있다 | `MassProcessingQueue.cpp:140-148,171-190,369-410` |
| 해결기 배열은 재계산마다 다르다 | Development `-game`에서 `-ini:Engine:[Core.Log]:LogMassDependencies=Verbose`로 "Dependency order:"를 찍으면 확인한다(20k P2 20초: PrePhysics 계산 `m3mass` 6번에 5가지, `m4mass` 9번에 9가지). Test 구성은 로그가 없다. `-LogCmds="… Verbose"`는 값에 공백이 있어 `run-dev.ps1 -Extra`로 안 넘어간다 | `MassProcessorDependencySolver.cpp:16,1030-1042` |
| 직접 프래그먼트 충돌은 청크 단위 | 두 질의의 직접 쓰기·읽기가 겹치면 청크 충돌(`ChunkContentionIndex`)이라 같은 청크를 동시에 못 돌리지만 프로세서·질의 전체를 막지 않는다. 프로세서·질의 단위 충돌은 서브시스템 쓰기와 엔티티 생성·구성 변경뿐이다. 청크 하나를 돌리려면 `TryActivateChunk`로 접근권을 얻어야 하고 못 얻으면 `WaitForChange`로 기다린다. 기다림은 호출한 프로세서의 시간(`ForEachEntityChunk`를 감싼 타이머, 예: 표현 LOD의 `CalculateLOD`)으로 잡히고 그 기다림 자체의 타이머는 없다 | `MassProcessingQueueTypes.cpp:203-254,256-360,693-760`, `MassProcessingQueue.cpp:428-448`, `MassRepresentation/Private/MassVisualizationLODProcessor.cpp:80-92` |
| LOD·이동 프로세서의 변환 접근 | 표현 LOD, Simulation LOD, LOD 수집기는 `FTransformFragment`를 **읽고**, `MassSmoothOrientationProcessor`·`MassApplyMovementProcessor`는 **쓴다**(조향은 읽기만). 이 읽기·쓰기가 청크 충돌이다. 표현 LOD와 Simulation LOD는 둘 다 `LOD` 그룹, 수집기 뒤라는 선언뿐이라 둘 사이의 순서 선언은 없다 | `MassVisualizationLODProcessor.cpp:20-50`, `MassLOD/Private/MassSimulationLOD.cpp:73-112`, `MassLODCollectorProcessor.cpp:25-34`, `MassAI/…/MassSmoothOrientationProcessors.cpp:28-40`, `MassMovement/Private/MassMovementProcessors.cpp:80-88`, `MassSteeringProcessors.cpp:66-87` |
| 엔진 프로세서의 순서를 프로젝트 설정으로 바꾸는 길 | `UMassProcessor`는 `config = Mass, defaultconfig`이고 `ExecutionOrder`(그룹·`ExecuteBefore`·`ExecuteAfter`)와 `ExecutionPriority`가 `config` 속성이다. `Config/DefaultMass.ini`의 `[/Script/<모듈>.<프로세서>] ExecutionOrder=(ExecuteInGroup="…",ExecuteAfter=("…"))`로 바꾼다. 구조체를 통째로 쓰므로 생성자의 기존 값을 되풀이해야 하고, 모든 단계(M0\~M4)에 적용된다. 실행 큐는 그룹 이름을 정확 일치로만 찾는다. 이동은 `/Script/MassMovement.MassApplyMovementProcessor`(기본 그룹 `Movement`, `ExecuteAfter ApplyForces`), 부드러운 회전은 `/Script/MassNavigation.MassSmoothOrientationProcessor`(그룹 `Movement`)이다. 쓴 적은 없다(제안) | `MassEntity/Public/MassProcessor.h:42-55,78,237-238,315`, `MassMovement/Private/MassMovementProcessors.cpp:76-77`, `MassNavigation/Private/MassSmoothOrientationProcessors.cpp:25` |
| Simulation LOD 트레이트가 청크를 나누는 것은 태그다 | 공유 프래그먼트(`FMassSimulationLODSharedFragment`, `FMassSimulationVariableTickSharedFragment`)는 같은 파라미터 값에 하나만 만들어져 청크를 나누지 않는다. 청크 프래그먼트는 청크마다 하나다. LOD 태그(`FMassOffLODTag` 시작, 이후 High/Medium/Low)는 LOD마다 아키타입을 나눈다 | `MassLOD/Private/MassLODTrait.cpp:120-157` |
| M4 P2 병렬: 대기는 트레이트 구성을 따라온다 | `m4off`(간격 0)에서도 메인 스레드 러너가 3.93ms(`m3mass` 1.59, `m4mass` 3.66)다. 대기하는 프로세서만 바뀐다: `m4off`는 LOD 수집기(1.56ms), `m4mass`는 표현 LOD(2.10ms). 기다리는 프로세서의 끝이 이동 프로세서(`ApplyMovement`·`SmoothOrientation`)의 끝과 같은 시각이다(0.0\~0.1ms 차이). 청크 충돌이라는 해석은 추정이고 청크 단위 타이머는 없다 | `Worklog/M4.md` "표현 LOD 대기: 가르는 트레이스", 트레이스 `M4-m4off-N20000-P2traceparb-r1` |

## 14. M5 ISKM 에셋(2026-11-11, `Scripts/create-iskm.py`)

줄임말: **ENG** = `Engine/Source/Runtime/Engine`. ASTPD = `UAnimSequenceTransformProviderData`.

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| ASTPD는 스켈레탈 메시의 `bOptimizeForInstancing`을 요구한다 | 렌더 데이터에 통합 본 맵(`HasUnifiedBoneMap`)이 없으면 등록을 거부하고 "requires 'Optimize for Instancing'" 경고를 낸다. 데이터 검증도 같은 조건으로 에러를 낸다. 플래그는 LOD 빌드 설정(`FSkeletalMeshBuildSettings::bOptimizeForInstancing`, AdvancedDisplay)이고, 어느 LOD 하나에 켜져 있으면 메시 전체에 통합 본 맵을 만든다. 켜면 DDC 키에 `_UBM_REPARENT_00`이 붙어 다시 빌드된다. M5 설계 조사의 "스켈레탈 메시에 인스턴싱 플래그는 없다"는 틀렸다 | `ENG/Private/Animation/AnimSequenceTransformProviderData.cpp:475-481`(IsValidFor), `:960-968`(IsDataValid), `ENG/Classes/Engine/EngineTypes.h:3138`, `ENG/Private/SkeletalMeshRenderData.cpp:259-275`, `ENG/Private/SkeletalMesh.cpp:6117-6139` |
| ASTPD 본 수 한도 | LOD마다 `RequiredBones` ≤ `SKINNING_MAX_BONES_PER_GROUP` 1024(메시 공간 회전 레이어가 있으면 256) | `AnimSequenceTransformProviderData.cpp:497-509`, `Engine/Shaders/Shared/SkinningDefinitions.h:60-61` |
| ASTPD 만들기(Python) | 팩토리 `unreal.TransformProviderDataFactory`의 `provider_data_class`를 `AnimSequenceTransformProviderData`로 두고 `AssetTools.create_asset`. `skinned_asset`·`sequences`는 private + EditAnywhere라 `set_editor_property`로 쓸 수 있다(`sequences`는 BlueprintReadOnly지만 막지 않았다, 2026-11-11 실행). `set_editor_property`가 `PostEditChangeProperty`를 불러 레이어 0을 만들고 바운드 계산을 시작한다. 읽기는 UFUNCTION `get_sequences`·`get_skinned_asset`·`get_sequence_play_length`·`get_num_layers` | `UnrealEd/Classes/Factories/TransformProviderFactory.h:19`, `ENG/Classes/Animation/AnimSequenceTransformProviderData.h:906-962,979-988`, `.cpp:784-837` |
| ASTPD 애니메이션 바운드 | `CachedData`는 `IsFilterEditorOnly` 아카이브(쿡)에서만 직렬화된다. 에디터 패키지에는 없고, 에디터는 `PostLoad`(`FApp::CanEverRender()`일 때)·`PostEditChangeProperty`에서 비동기로 다시 계산한다. `HasAnimationBounds`는 UFUNCTION이 아니라 Python에서 볼 수 없다 | `AnimSequenceTransformProviderData.cpp:402-415,716-735,775-778,837` |
| ISKM 머티리얼 사용처 | ISKM 프록시는 `MATUSAGE_SkeletalMesh`와 `MATUSAGE_InstancedSkinnedMesh`를 둘 다 검사한다(하나라도 없으면 그 섹션은 기본 머티리얼). Python은 `MaterialUsage.MATUSAGE_SKELETAL_MESH`·`MATUSAGE_INSTANCED_SKINNED_MESH`, 확인은 `used_with_skeletal_mesh`·`used_with_instanced_skinned_mesh` | `ENG/Private/SkeletalMeshSceneProxy.cpp:229-240`, `ENG/Public/Materials/MaterialInterface.h:131`, `ENG/Public/Materials/Material.h:903` |
| ISKM 인스턴스 커스텀 데이터 | 머티리얼 `PerInstanceCustomData` 노드는 ISM과 ISKM 사용처에서만 인스턴스 데이터를 읽는다. 컴포넌트는 `NumCustomDataFloats`·`SetCustomDataValue(FPrimitiveInstanceId, Index, Value)` | `ENG/Private/Materials/HLSLMaterialTranslator.cpp:14829`, `ENG/Classes/Components/InstancedSkinnedMeshComponent.h:72,221` |
| 스켈레탈 메시 LOD 빌드 설정(Python) | `unreal.SkeletalMeshEditorSubsystem.get_lod_count`, `get_lod_build_settings`, `set_lod_build_settings`(정적 함수, `FScopedSkeletalMeshPostEditChange`로 다시 빌드) | `Editor/SkeletalMeshEditor/Public/SkeletalMeshEditorSubsystem.h:252,306,315`, `Private/SkeletalMeshEditorSubsystem.cpp:1038-1075` |
| Python 커맨드렛 함정 | `EditorAssetLibrary.delete_asset`은 다른 에셋이 참조하면 거부한다(참조하는 쪽부터 지운다). `get_editor_property`로 받은 구조체 배열의 원소를 제자리에서 고치면 반영되지 않는다(새 배열로 다시 넣는다). `EditorValidatorSubsystem.validate_assets_with_settings`의 출력 인자는 반환값 튜플로 온다 | 2026-11-11 `create-iskm.py` 실행 |

## 15. M5 ISKM 코드 경로(2026-11-11, 코드 세션)

줄임말: **MR** = `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation`, **ENG** = `Engine/Source/Runtime/Engine`, **ME** = `Engine/Source/Runtime/MassEntity`.

| 항목 | 내용 | 위치 |
| --- | --- | --- |
| ISKM 트랙과 애니 데이터 | `EndVisualChanges`는 갱신 목록의 엔티티마다 트랙 맵을 찾아 없으면 `AllocateTrack` + `SetAutoPlayData`, 있으면 시퀀스 번호가 다를 때만 `SetAutoPlayData`를 부르고, 인스턴스의 애니 번호를 트랙 번호로 둔다. 애니 데이터 배열에 그 엔티티 칸이 없으면 트랙도 애니 번호도 건드리지 않는다(새 인스턴스는 `AddInstance(…, 0)`이라 트랙 0을 그린다). 그래서 위치와 애니 데이터를 매 프레임 같은 순서로 쌓아야 한다. 같은 처리에서 엔티티마다 `ResolvedAnimStates`에도 읽은 값을 쓴다 | `MR/Private/MassVisualizationComponent.cpp:754-800, 832-892` |
| ISKM 컴포넌트 생성 | 메시 설명자마다 하나. ASTPD면 컴포넌트마다 `UAnimSequenceTransformProviderDataInstance`를 만들어 넣고, `SetAnimationMinScreenSize`·`SetCastShadow`·`SetHasPerInstancePrevTransforms(true)`·`SetCullDistances(0, 1000000)`를 부른 뒤 `AMassVisualizer` 밑에 등록한다. 설명자 생성자가 `InstancedSkinnedMeshComponentClass`를 `UInstancedSkinnedMeshComponent`로 채운다 | `MR/Private/MassVisualizationComponent.cpp:1299-1341`, `MR/Private/MassSkinnedMeshRepresentationTypes.cpp:9-12` |
| 트랙 번호 직접 쓰기 | 트랙 맵(`FEntityToTrackIdMap`, 엔티티 핸들 → 트랙 번호)은 공유 데이터의 public 접근자로 읽고, 공유 데이터는 범위의 `InstancedSkinnedMeshSharedDataPtr->Find(SkinnedMeshComponentRefs[i])`로 찾는다. 트랙 제공자는 `Component->GetTransformProvider()`를 `UAnimSequenceTransformProviderDataInstance`로 캐스트한다. `SetAutoPlayData(Track, Layer 0, Data)`는 public `ENGINE_API`다 | `MR/Public/MassSkinnedMeshRepresentationTypes.h:317-324, 460-464, 619-624`, `ENG/Classes/Animation/AnimSequenceTransformProviderData.h:1069-1077` |
| 자동 재생 데이터 | `FAnimSequenceTrackAutoPlayData`: `SequenceIndex`, `Position`(시작 위치, 초), `PlayRate`, `BlendTime`(0이면 바로 바꾼다), `LoopMode`(`Loop`·`Clamp`). 시퀀스 길이는 `UAnimSequenceTransformProviderData::GetSequencePlayLength(int32)`(`ENGINE_API`) | `ENG/Classes/Animation/AnimSequenceTransformProviderData.h:33-40, 104-127, 926` |
| ISKM 전환과 PrevTransform | 표현 프로세서는 `SkinnedMeshInstance`로 들어가는 프레임에 `PrevTransform`을 현재 위치로 채운다. 액터 준비를 기다리던 중 ISKM으로 돌아오면 액터를 끄고 ISKM으로 둔다 | `MR/Private/MassRepresentationProcessor.cpp:351-366` |
| 엔진 ISKM 위치 함수 | `UMassUpdateInstancedSkinnedMeshProcessor::UpdateMeshTransform`은 `UE_API` static이라 다른 프로세서가 부를 수 있다(설명자의 변환 오프셋 처리 포함). 엔진 프로세서는 게임 스레드, Representation 그룹, VisualizationProcessing 뒤 | `MR/Public/MassUpdateSkinnedMeshProcessor.h:22`, `MR/Private/MassUpdateSkinnedMeshProcessor.cpp:16-40` |
| 커맨드 플러시 안의 동기 API | `AddTagToEntity`·`BatchChangeTagsForEntities` 등의 검사(`CHECK_SYNC_API`)는 `IsProcessing()`만 본다. `FlushCommands`는 처리 중이 아닐 때만 돌므로 지연 커맨드 람다 안에서 동기 API로 태그·프래그먼트를 바꿀 수 있다(엔티티 풀링이 쓴다) | `ME/Private/MassEntityManager.cpp:22, 2512-2545` |
| 청크 표시 프래그먼트 위치 | `FMassVisualizationChunkFragment`(`AreAnyEntitiesVisibleInChunk`)는 MassLOD 모듈이다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassLOD/Public/MassLODFragments.h:136` |
| ASTPD 바운드(실행) | Development 에디터 `-game` 실행에서 로드 직후 `HasAnimationBounds()` 0, `IsCompiling()` 1(비동기 계산 중). Test 패키지(쿡)에서는 아직 보지 않았다(로그 `lab_iskm … bounds=`) | 2026-11-11 `LabDev-m5iskm-M5-N100-P1.log` |
| ISKM 고정 바운드 | `SetPrimitiveBoundsOverride(FBox)`는 값만 저장한다. 값이 유효하면 `CalcBounds`가 인스턴스 반복 없이 이 상자를 돌려주고, `UpdateBounds`로 부르면 월드 좌표 그대로다. 인스턴스 로컬 바운드와 GPU LOD 선택은 이 값을 쓰지 않는다. VSM 빛 반경 검사는 프리미티브 바운드를 쓰지만 방향광은 늘 참이다 | `ENG/Private/Components/InstancedSkinnedMeshComponent.cpp:1249-1253`, `ENG/Public/InstancedSkinnedMeshComponentHelper.h:119-135`, `ENG/Private/InstancedSkinnedMeshSceneProxy.h:139-160`, `Renderer/Private/VirtualShadowMaps/VirtualShadowMapCacheManager.h:165-169` |
| ISKM 트랙 쓰기 줄(2026-11-13, 08번 글) | 갱신 경로는 트랙이 있고 시퀀스 번호가 다를 때 `SetAutoPlayData`를 부른다(`:766-769`). 추가 경로의 같은 동작은 `:857-860`이다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassVisualizationComponent.cpp:766-769,857-860` |
| 트랙 해제 입력(2026-11-13, 05번 글) | 해제 `DeallocateTrack` 호출은 MassGameplay 전체에서 `:1156` 하나이고 입력은 `EntitiesRequiringRemoval`이다. 그 목록은 `FMassLODInstancedSkinnedMeshSignificanceRange::RemoveInstance`(`:1537-1544`)가 채우고, 그 호출은 `UMassStationaryISMSwitcherProcessor`의 `MeshInfo.RemoveInstance`(`:127`)뿐이다. 그 쿼리는 `FMassStaticRepresentationTag`를 요구한다(`:30`) | `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassVisualizationComponent.cpp:1156,1537-1544`, `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassStationaryISMSwitcherProcessor.cpp:30,127` |
| ISKM의 재생 상태 읽기(2026-11-13, 06번 글) | 트랙을 쓴 직후 매번 `GetSequenceIndex`·`GetPosition`·`GetPlayRate`로 재생 상태를 읽어 `ResolvedAnimStates`에 넣는다(UAF 인계용 주석). 끄는 분기가 없다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassVisualizationComponent.cpp:777-787,868-878` |
| ISKM의 인스턴스 변경 뒤 바운드 갱신(2026-11-13, 06번 글) | `UInstancedSkinnedMeshComponent::SendRenderInstanceData_Concurrent`가 장면 프록시가 있을 때 `InstanceDataManager.FlushChanges`가 참이면 `UpdateBounds()`를 부른다 | `Engine/Source/Runtime/Engine/Private/Components/InstancedSkinnedMeshComponent.cpp:495-497` |
| ISKM 바운드 계산의 인스턴스 반복(2026-11-13, 06번 글) | `bFastBuild = false`로 박혀 있다(`:202`). 인스턴스마다 애니메이션 바운드를 변환해 합치고(`:227-235`), 애니메이션 바운드가 없으면 메시 바운드를 인스턴스마다 변환해 합친다(`:245-251`) | `Engine/Source/Runtime/Engine/Public/InstancedSkinnedMeshComponentHelper.h:202,227-235,245-251` |
| ISKM GPU 메시 LOD 선택 조건 | `bUseGpuLodSelection = NumLODs > 1 && 메시 설명자의 bUseGpuLodSelection && (bRenderStatic \|\| 통합 본 맵이 있음)` | `Engine/Source/Runtime/Engine/Private/InstancedSkinnedMeshSceneProxy.cpp:76` |

## 16. G1 게임 장면에서 확인한 것(2026-11-11, 코드 세션)

줄임말: **MN** = `Engine/Plugins/AI/MassAI/Source/MassNavigation`, **ENG** = `Engine/Source/Runtime/Engine`.

| 항목 | 내용 | 위치 |
| --- | --- | --- |
| 엔진 조향의 목표점 | 조향 목표는 `MoveTarget.Center + Forward × Clamp(ForwardOffset + SteerForward, 0, 내다보는 거리)`이고 `ForwardOffset`은 개체 위치를 `Forward`에 내린 값이다. 개체가 `Center` 뒤에 있으면 목표가 `Center` 자체가 된다. 그래서 `Center`를 오브에 두면 `Forward`와 무관하게 오브로 곧게 간다. 방향을 따르게 하려면 `Center`를 개체 앞(방향 쪽)에 둔다. 내다보는 거리는 `LookAheadTime × DesiredSpeed` | `MN/Private/MassSteeringProcessors.cpp:133, 152-163` |
| 엔진 비켜 가기와 LOD 태그 | 이동 비켜 가기 쿼리는 `FMassNavigationEdgesFragment`를 요구하고 `FMassMediumLODTag`·`FMassLowLODTag`·`FMassOffLODTag`가 없는 개체만 돈다. LOD 태그가 없으면(M1\~M3) 전 개체, Simulation LOD Trait이 있으면(M4·M5) High 밴드(카메라 80m 안)만 돈다. 서 있는 비켜 가기 쿼리도 같다 | `MN/Private/MassAvoidanceProcessors.cpp:295-303, 1120-1127` |
| 엔진 비켜 가기의 벽 | 환경 비켜 가기는 `FMassNavigationEdgesFragment`의 경계선만 본다(보통 ZoneGraph 차선 경계가 채운다). 콜리전 지오메트리는 보지 않는다 | `MN/Private/MassAvoidanceProcessors.cpp:399, 487-495` |
| CharacterMovement RVO 기본값 | `bUseRVOAvoidance` false, `AvoidanceConsiderationRadius` 500. `AvoidanceWeight`는 생성자에서 정하지 않아 0이다. `SetAvoidanceEnabled(true)`가 `UAvoidanceManager::RegisterMovementComponent`로 등록한다(이동 컴포넌트를 붙일 때 `bUseRVOAvoidance`가 이미 참이어도 등록된다) | `ENG/Private/Components/CharacterMovementComponent.cpp:821-829, 1050-1055, 11750-11771`, `ENG/Classes/GameFramework/CharacterMovementComponent.h:1196` |

## 17. Niagara 메시 렌더러와 VAT(2026-11-12, Q2 에셋 세션)

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| Niagara 메시의 인스턴스 커스텀 데이터 | GPU Scene 경로에서 Niagara 메시 렌더러는 인스턴스당 커스텀 데이터 float 1개를 잡고(`SetNumInstanceCustomDataFloats(1)`), GPU 쓰기 패스가 그 0번에 `asfloat(ParticleIndex)`를 쓴다(정점 팩토리가 입자 데이터를 찾는 데 쓴다). 그래서 머티리얼의 `PerInstanceCustomData` 노드는 Niagara에서 입자 인덱스의 비트를 읽는다. ISM용 VAT 머티리얼을 커스텀 데이터 경로 그대로 Niagara에 쓸 수 없다 | `Plugins/FX/Niagara/Source/Niagara/Private/NiagaraRendererMeshes.cpp:1130,1207,1225`, `Plugins/FX/Niagara/Shaders/Private/NiagaraUpdateMeshGPUSceneInstances.usf:208`, `NiagaraVertexFactories/Private/NiagaraMeshVertexFactory.cpp:182`, `Shaders/Private/MaterialTemplate.ush:1501,1532` |
| 개체별 컬링 강제 | 입자 소스 메시 렌더러는 `bForceInstanceCulling = true`로 GPU에서 인스턴스마다 컬링한다(죽은 입자, 거리 컬링, MeshIndex·가시성 태그 불일치를 버리려고) | `NiagaraRendererMeshes.cpp:1159-1165` |
| AnimToTexture의 Niagara 경로 | `GetFrameSwitch`(엔진 에셋, MCP로 노드를 읽음)에 정적 스위치 `AutoPlay`(기본 true)와 `UseDynamicParameters`(기본 false)가 있다. 동적 파라미터를 켜면 AutoPlay 끔은 Dynamic Parameter 0번(Frame, PrevFrame), AutoPlay 켬은 1번(TimeOffset, Playrate, StartFrame, EndFrame)을 읽는다. 자동 재생 프레임은 `GetAutoPlayFrame`이 `StartFrame + fmod((Time + TimeOffset) × Playrate × SampleRate, EndFrame − StartFrame + c)`로 만든다. 파라미터 이름은 `AnimToTextureParamNames` | `/AnimToTexture/Materials/MaterialFunctions/GetFrameSwitch`, `GetAutoPlayFrame`, `Plugins/Experimental/AnimToTexture/Source/AnimToTexture/Public/AnimToTextureDataAsset.h:18-42` |
| 메시 렌더러 그림자 기본값 | `UNiagaraMeshRendererProperties::bCastShadows` 기본 true. 컴포넌트는 시스템이 `bOverrideCastShadow`면 시스템의 `bCastShadow`를 쓴다 | MCP `GetRendererData` 기본값, `NiagaraComponent.cpp:4678` |
| 워밍업 | 시스템 인스턴스 초기화 때 `WarmupTickCount > 0`이면 `AdvanceSimulation(틱 수, 틱 델타)`로 한꺼번에 돈다. GPU 이미터는 틱을 큐에 넣고 다음 렌더에서 처리한다(보통은 큐 전부, 앱이 앞 창이 아니어서 3프레임 넘게 쌓이면 `fx.Niagara.Batcher.TickFlush.Mode` 기본 1이 더미 뷰로 처리) | `Niagara/Private/NiagaraSystemInstance.cpp:899-913`, `NiagaraGpuComputeDispatch.cpp:104-128,480-524` |
| 최대 틱 델타 | `UNiagaraSettings::MaxDeltaTimePerTick` 기본 0.125초. 시스템 `WarmupTickDelta`가 이보다 크면 에디터 스택이 경고한다 | `Niagara/Public/NiagaraSettings.h:181`, `NiagaraEditor/Private/ViewModels/Stack/NiagaraStackEmitterSettingsGroup.cpp:253-269` |
| 시스템 템플릿의 루프 | 엔진 시스템 템플릿 `RadialBurst`의 `SystemState`는 Loop Behavior Once, Loop Duration 2초다. 이 템플릿으로 만든 시스템은 이미터가 Infinite여도 2초 뒤 스폰이 멈추고 완료된다 | MCP `GetModuleInputValues`(`/Niagara/DefaultAssets/Templates/Systems/RadialBurst`에서 만든 시스템) |
| 머티리얼 입력 출력 이름 조회의 한계 | `UMaterialEditingLibrary::GetInputNodeOutputNameForMaterialExpression`은 입력을 돌며 같은 노드를 처음 만난 입력의 출력 이름만 돌려준다. 한 노드의 여러 출력이 같은 노드로 이어지면 모두 첫 출력 이름으로 보인다(MCP `get_expression_inputs`도 같은 결과) | `Editor/MaterialEditor/Private/MaterialEditingLibrary.cpp:1290-1306` |
| 동적 파라미터 모듈 교체 | `/Niagara/Modules/Update/Material/DynamicMatParams1`은 폐기됐다(`bDeprecated` true, 라이브러리 비노출). 대체는 `DynamicMaterialParameters`이고, 정적 스위치 `Write Parameter Index 0`\~`3`(enum `ENiagara_DynamicMaterialParametersInputMode`: NewEnumerator0 Off, NewEnumerator1 Float)으로 쓸 인덱스를 고른다. 새로 더하면 Index 0이 Float, 1\~3이 Off라서 1번만 쓰려면 0을 끄고 1을 Float로 둔다. Float 모드 입력은 `Index N Param 1`\~`4`와 쓰기 플래그 `Module.IndexNFloat0Write`\~`3Write`(기본 true). AnimToTexture 자동 재생 경로에서 Param 1\~4가 TimeOffset, Playrate, StartFrame, EndFrame 자리로 들어가는 것을 화면으로 확인했다(Q2 2단위) | MCP `GetNiagaraScriptDigest`·`AddModule`·`GetModuleInputValues`, `Docs/Worklog/Q2.md` "2단위" |
| 저장하지 않고 맵 다시 열기 | `UEditorLoadingAndSavingUtils::NewBlankMap(false)`는 저장 확인 없이 새 맵을 만든다. 그 뒤 `LoadMap`으로 원래 맵을 디스크에서 연다. MCP `SceneTools.load_level`은 맵이 "수정됨"이면 거부한다 | `Editor/UnrealEd/Private/FileHelpers.cpp:5857-5878`, `Plugins/Experimental/Toolsets/EditorToolset/Content/Python/editor_toolset/toolsets/scene.py:28-41` |
| 컴포넌트에 사용자 변수 넣기(Q2 3단위) | `UNiagaraComponent::SetVariableFloat`는 시스템 인스턴스가 없으면(자동 활성화 끔, 활성화 전) `OverrideParameters`(`FNiagaraUserRedirectionParameterStore`)에 값을 더하고, 있으면 `SetVariable_Deferred`로 미룬다. 인스턴스는 `InitializeSystem`이 `OverrideParameters`를 넘겨 만들고 워밍업도 그때 돈다. 그래서 값을 넣은 뒤 `Activate`해야 워밍업이 그 값으로 돈다. `SpawnSystemAtLocation`의 `bPreCullCheck`(기본 true)는 스폰 때 컬링 검사로 생성을 건너뛸 수 있다 | `Niagara/Private/NiagaraComponent.cpp:1204-1219, 2864-2878`, `Niagara/Public/NiagaraFunctionLibrary.h:93` |
| GPU 이미터 입자 수 읽기 | `FNiagaraEmitterInstance::GetNumParticles`는 GPU 이미터면 수 읽기 펜스가 지났을 때 `GPUExecContext->GetCurrentNumInstances()`, 아니면 누적 스폰 수(`TotalSpawnedParticles`)를 돌려준다. 게임 코드에서 `GetSystemInstanceController()->GetSystemInstance_Unsafe()->GetEmitters()`로 이미터마다 읽을 수 있다 | `Niagara/Private/NiagaraEmitterInstance.cpp:34-53`, `Niagara/Public/NiagaraSystemInstanceController.h:83`, `NiagaraSystemInstance.h:277` |
| 표현 LOD의 Off → Low 히스테리시스 | `ComputeLODFromSettings`는 한 단계 위로 올라갈 때만 히스테리시스를 둔다. Off에서 Low로 들어오는 거리는 `Lerp(Low 거리, Off 거리, 1 − 10%)`다. 밴드가 0/30/80/200m면 Off의 개체는 카메라에서 188m에 들어와야 그려진다(200m가 아니다). 거리는 카메라 기준 3D 거리다 | `MassGameplay/Source/MassLOD/Public/MassLODCalculator.h:864-886` |
| Budget Allocator와 `GlobalAnimRateScale` | `USkeletalMeshComponentBudgeted::TickComponent`는 예산 할당기가 있어도 `Super::TickComponent`를 부르므로 `TickAnimInstances`의 `DeltaTime × GlobalAnimRateScale` 경로를 그대로 탄다. 틱을 건너뛰면 누적 델타가 들어온다 | `Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator/Private/SkeletalMeshComponentBudgeted.cpp:91-133`, `Runtime/Engine/Private/Components/SkeletalMeshComponent.cpp:1755-1785` |
| 재생 위치를 런타임에 읽기(Q1b 확인) | AnimBP: `UAnimInstance::GetUngroupedActivePlayersRead()`가 지난 갱신의 `FAnimTickRecord` 배열을 주고 `SourceAsset`과 `*TimeAccumulator`(시퀀스 플레이어의 재생 시각)를 읽을 수 있다. ISKM: `UAnimSequenceTransformProviderDataInstance::GetPosition(트랙, 0).TargetPosition`·`GetPlayRate`·`GetSequenceIndex`가 GPU로 가는 묶음 트랙 데이터에서 엔진이 계산한 위치를 준다(재생 추적기가 없으면 `WrapPosition(LoopMode, GetTimePosition(월드 시각), 클립 길이)`) | `Runtime/Engine/Classes/Animation/AnimInstance.h:497`, `AnimationAsset.h:382-390`, `AnimSequenceTransformProviderData.h:1113`, `Private/Animation/AnimSequenceTransformProviderData.cpp:1816-1913` |
| Niagara 메시 입자 사용 플래그의 Python 이름 | `EMaterialUsage::MATUSAGE_NiagaraMeshParticles`는 Python에서 `unreal.MaterialUsage.MATUSAGE_NIAGARA_MESH_PARTICLES`이고, `MaterialEditingLibrary.set_material_usage`가 `bUsedWithNiagaraMeshParticles`를 켠다 | `Runtime/Engine/Public/Materials/MaterialInterface.h:103`, `Runtime/Engine/Private/Materials/Material.cpp:1521,1780`, `Editor/MaterialEditor/Private/MaterialEditingLibrary.cpp:814-823` |

## 18. 빼 보기 스위치에서 확인한 것(2026-11-12, Q3 코드 세션)

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| Budget Allocator가 실제로 도는 조건 | 할당기는 CVar `a.Budget.Enabled`(`GAnimationBudgetEnabled`, 기본 0)를 `bCachedEnabled`에 담고, 틱·등록 경로가 `bCachedEnabled && bEnabled`일 때만 돈다. `bEnabled`는 월드 할당기의 `SetEnabled`/`GetEnabled`다. 그래서 "적용됨"은 둘 다 참인지로 본다(CSV 메타데이터 `LabBudget`) | `Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator/Private/AnimationBudgetAllocatorCVars.cpp:7`, `Private/AnimationBudgetAllocator.cpp:75,115,135,1328-1337`, `Public/IAnimationBudgetAllocator.h:73-76` |
| 액터 표현 판정 함수 | `UE::Mass::Representation::IsValidActorRepresentation(EMassRepresentationType)`는 `MassRepresentationTypes.h`에 있다. VAT 프로세서의 `dehydrate` 조건(이전 표현이 액터, 지금 인스턴스)을 ISKM 프로세서에도 같은 함수로 넣었다 | `Plugins/Runtime/MassGameplay/Source/MassRepresentation/Public/MassRepresentationTypes.h:52` |

## 19. 액터 상한과 전환 거리(2026-11-12, Q4)

줄임말: **MG** = `Engine/Plugins/Runtime/MassGameplay/Source`.

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 표현 LOD 상한은 종류 합계다 | 표현 Trait은 LOD 파라미터 값으로 공유 프래그먼트를 `GetOrCreateSharedFragment`하므로 파라미터가 같은 두 종류(소형·대형 템플릿)는 공유 프래그먼트 하나, LOD 계산기 하나를 쓴다. 표현 LOD 프로세서는 공유 프래그먼트마다 `PrepareExecution`·`AdjustDistancesFromCount`를 부른다. 그래서 `LODMaxCount[High]` 500은 두 종류를 합친 액터 수 상한이다 | `MG/MassRepresentation/Private/MassVisualizationTrait.cpp:145`, `MassVisualizationLODProcessor.cpp:70-110` |
| 상한이 거리를 줄이는 방식 | `AdjustDistancesFromCountForRuntimeData`는 LOD마다 거리 구간(LOD당 250개, `MaxBucketsPerLOD`)을 가까운 것부터 세어 상한을 넘는 구간에서 다음 LOD의 거리를 당긴다. 구간마다 화면 안 개체를 먼저 센다. 그래서 상한이 걸리면 가까운 개체부터, 같은 거리면 화면 안 개체부터 액터가 된다 | `MG/MassLOD/Public/MassLODCalculator.h:534-642`, `MassLODTypes.h:23` |
| High 경계에는 히스테리시스가 없다 | `ComputeLODFromSettings`는 Off부터 내려오며 거리를 비교하고, 히스테리시스는 "한 단계 위로 오를 때" 그 LOD 거리와 다음 LOD 거리 사이(`Lerp(D[i], D[i+1], 1 − 10%)`)에만 둔다. 반복이 `LODDistIdx > 0`에서 멈추므로 High(0)로 들어갈 때와 High에서 나올 때 모두 검사하지 않는다. 그래서 액터 ↔ 인스턴스 전환은 화면 안 30m(화면 밖 15m) 한 거리에서 일어난다. Low → Medium 진입은 `H + 0.9 × (80m − H)`라 전환 거리 H를 바꾸면 함께 움직인다(30m면 75m, 50m면 77m) | `MG/MassLOD/Public/MassLODCalculator.h:862-886` |

## 20. Test 패키지 HUD와 스크린샷, 액터 캡슐(2026-11-12, Q5)

줄임말: **E** = `Engine/Source/Runtime/Engine`, **MG** = `Engine/Plugins/Runtime/MassGameplay/Source`.

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| `AHUD::DrawHUD`는 Test에서도 불린다 | `AHUD::PostRender`는 빌드 구성 조건 없이 `bShowHUD && FApp::CanEverRender()`이면 `DrawHUD()`를 부른다. 빌드 조건이 걸린 것은 디버그 그리기(`ENABLE_DRAW_DEBUG`, Test에서 0, 6절)뿐이다. 게임 뷰포트가 플레이어마다 HUD의 `PostRender`를 부른다 | `E/Private/HUD.cpp:151-177`, `E/Private/GameViewportClient.cpp:2150-2181` |
| HUD의 시간 값 출처 | `GGameThreadTime`(대기를 뺀 게임 스레드 시간)은 `FViewport::Draw`가, `GRenderThreadTime`은 Slate RHI 렌더러가 매 프레임 쓰고 `RHIGetGPUFrameCycles()`는 GPU 프로파일러가 쓴다. 셋 다 `STATS` 없이 쓰이고 CSV `GameThreadTime`·`RenderThreadTime`·`GPUTime`이 같은 값을 읽는다. 그래서 HUD 숫자와 CSV 열은 같은 정의다(HUD는 1초 평균) | `E/Private/UnrealClient.cpp:1831-1845`, `SlateRHIRenderer/Private/SlateRHIRenderer.cpp:1359`, `RHI/Private/GPUProfiler.cpp:2265-2284`, `Launch/Private/LaunchEngineLoop.cpp:1518-1520` |
| 글꼴을 선명하게 | `FCanvasTextItem`을 `FSlateFontInfo`(엔진 Roboto, 포인트 크기)로 만들면 런타임 글꼴 캐시가 그 크기로 래스터화한다. `UFont`와 배율로 그리면 `LegacyFontSize × DPIScale`로 래스터화한 뒤 배율만큼 늘려 흐려진다. 런타임 글꼴은 알파 전용 텍스처라 블렌드가 반투명 계열로 강제된다. 엔진 Roboto 합성 글꼴의 서체 이름은 `Regular`·`Bold`·`Italic`·`Light`(에셋 문자열) | `E/Private/UserInterface/CanvasItem.cpp:972-1000, 1193-1195`, `E/Public/CanvasItem.h:448-454` |
| 글자 폭 재기 | `FEngineFontServices::Get().GetFontMeasure()->Measure(문자열, FSlateFontInfo)`. 캔버스 텍스트 아이템의 `GetTextSize`는 protected다 | `E/Public/EngineFontServices.h:29-33`, `SlateCore/Public/Fonts/FontMeasure.h:29` |
| HUD가 찍히는 스크린샷 | `FScreenshotRequest::RequestScreenshot(경로, bShowUI=false, …)`는 뷰포트가 `Draw`(HUD 포함)와 캔버스 플러시를 마친 뒤 `ProcessScreenShots`에서 뷰포트 렌더 타깃을 읽으므로 HUD가 함께 찍힌다. `HighResShot`은 더미 뷰포트에 다시 그리고 모션 블러 쇼 플래그를 끈다 | `E/Private/UnrealClient.cpp:1862-1872`(일반), `:1711-1721`(HighResShot), `E/Private/GameViewportClient.cpp:2369-2433` |
| 하이드레이션 액터의 캡슐 콜리전 | `ALabMinion`(`ACharacter`)의 캡슐은 `Pawn` 프로필 그대로이고(코드는 Pawn 채널 응답만 무시로 둔다), Mass 액터 스포너가 풀에서 액터를 꺼낼 때 `SetActorEnableCollision(true)`(액터 단위)를 부른다. 그래서 컴포넌트 콜리전을 코드가 바꾸지 않으면 켜져 있다. Q5 1b단위부터 하이드레이션 액터(`ALabMinionMassActor` 계열)는 `BeginPlay`에서 기본으로 `NoCollision`과 오버랩·물리 볼륨 갱신 끔을 둔다(옛 `m3nocol` 처리). 켬은 `-LabActorCollision=1`과 라벨 `m3colbase`, QueryOnly는 `m3colquery`다. 그 전(M3b\~Q5 1단위)의 코드는 `m3colquery`·`m3nocol`·`-LabActorCollision=0`만 바꾸고 기본은 켬이었다. M0 액터(`ALabMinion`)는 바꾸지 않는다 | `Source/HordeOptLab/LabMinion.cpp:24-26`, `ALabMinionMassActor::BeginPlay`, `LabConfig.cpp` `ActorCollision`, `MG/MassActors/Private/MassActorSpawnerSubsystem.cpp:327` |

## 21. 엔티티 위치를 Niagara 배열로 넘기기(2026-11-12, Q2b 코드 세션)

줄임말: **MG** = `Engine/Plugins/Runtime/MassGameplay/Source`, **NI** = `Engine/Plugins/FX/Niagara/Source/Niagara`.

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| Off(`CulledByDistance`) 청크는 0.5초마다만 표현을 다시 본다 | 표현 LOD의 Off 개체는 가시성이 `CulledByDistance`이고, 표현이 None인 엔티티는 그 가시성 태그 청크로 모인다. 표현 LOD 프로세서는 이 청크를 `FarEntityQuery`로 따로 돌며 청크 필터 `ShouldUpdateVisualizationForChunk`를 걸고, 표현 프로세서의 `UpdateChunkVisibility`는 이 청크의 갱신 간격을 `NotVisibleUpdateRate`(기본 0.5초, ± 10% 난수, 결정적 실행이면 난수 없음)로 둔다. 그래서 Off에서 그리는 거리로 들어온 개체는 최대 약 0.55초 늦게 그려진다. 1만 P1 개발 실행에서 그려지지 않은 걷는 엔티티의 카메라 3D 거리 최솟값이 183.9\~185.0m였다(진입 거리 188m, `Docs/Worklog/Q2.md` "Q2b") | `MG/MassLOD/Public/MassLODCalculator.h:447-458`, `MG/MassRepresentation/Private/MassVisualizationLODProcessor.cpp:40-44`, `MassRepresentationProcessor.cpp:576-596, 600-606`, `Public/MassRepresentationFragments.h:179-181` |
| 배열 사용자 변수를 C++에서 넣기 | `UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(UNiagaraComponent*, FName, TConstArrayView<FVector4f>)`(블루프린트용이 아닌 판). 내부는 `GetDataInterface`로 덮어쓰기 저장소의 배열 DI를 찾아 프록시의 `SetArrayData`로 복사한다. 에디터에서만 쓰는 변형 DI 복제(`CreateAndSetVariant`)는 게임 월드가 아닐 때만 돌아 `-game` 실행에는 없다 | `NI/Classes/NiagaraDataInterfaceArrayFunctionLibrary.h:172`, `NI/Private/NiagaraDataInterfaceArrayFunctionLibrary.cpp:33-49, 432-435`, `NI/Classes/NiagaraDataInterfaceArrayImpl.h:529-545`, `NI/Private/NiagaraDataInterfaceArray.cpp:175-196` |
| 없는 이름은 부를 때마다 경고 | `UNiagaraFunctionLibrary::GetDataInterface`는 이름이 없거나 형이 다르면 `LogNiagara` Warning을 매번 남긴다(자세한 경고 설정과 무관). 매 프레임 넣는 코드는 이름을 미리 한 번 확인한다 | `NI/Private/NiagaraFunctionLibrary.cpp:702-735` |
| 사용자 변수 이름 찾기 | 컴포넌트의 덮어쓰기 저장소(`FNiagaraUserRedirectionParameterStore`)는 `User.X`로 저장하고 접두사 없는 `X`를 열쇠로 하는 넘김표를 둔다. `FindParameterOffset`에 `User.X`를 주면 넘김표에서 못 찾고 저장소에서 바로 찾는다. 저장소는 컴포넌트에 시스템을 넣을 때 시스템의 노출 변수로 채워져 활성화 전에도 확인할 수 있다 | `NI/Public/NiagaraUserRedirectionParameterStore.h:35-53`, `NI/Private/NiagaraUserRedirectionParameterStore.cpp:46-69`, `NI/Private/NiagaraComponent.cpp:3892-3900, 4620-4672` |
| `SetVariableFloat`는 없는 이름에 조용하다 | 인스턴스가 있으면 `SetVariable_Deferred`(다음 틱에 반영), 없으면 덮어쓰기 저장소에 더한다(없는 이름도 더해진다). 경고는 없다. 그래서 이름 확인은 값을 넣기 전에 한다 | `NI/Private/NiagaraComponent.cpp:2864-2878` |

## 22. LOD 중요도 범위로 원거리 그림자 끄기(2026-11-12, Q4b)

줄임말: **MG** = `Engine/Plugins/Runtime/MassGameplay/Source`, **MR** = `MG/MassRepresentation`.

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 그림자는 컴포넌트 단위다 | ISKM·ISM 메시 설명자의 `bCastShadows`는 엔진이 설명자마다 만드는 컴포넌트의 `SetCastShadow`로만 넘어간다. 인스턴스 단위 그림자 플래그는 없다. 컴포넌트는 설명자 해시(`bCastShadows` 포함)로 찾으므로 그림자만 다른 두 설명자는 컴포넌트 둘이 된다 | `MR/Private/MassVisualizationComponent.cpp:1299-1337`(ISKM), `:259`(ISM), `MR/Public/MassSkinnedMeshRepresentationTypes.h:118-137`, `MR/Public/MassRepresentationTypes.h:136` |
| 설명자의 LOD 중요도 범위 | `FMassSkinnedMeshInstanceVisualizationMeshDesc`와 `FMassStaticMeshInstanceVisualizationMeshDesc`에 `MinLODSignificance`(포함, 기본 High)·`MaxLODSignificance`(제외, 기본 Max)와 `SetSignificanceRange`가 있다. 엔진은 모든 설명자의 경계 값을 모아 범위를 만들고 범위마다 그 범위를 덮는 설명자의 컴포넌트를 넣는다 | `MR/Public/MassSkinnedMeshRepresentationTypes.h:69-75, 140-145`, `MR/Public/MassRepresentationTypes.h:97-101, 150-154`, `MR/Private/MassVisualizationComponent.cpp:1371-1419`(ISKM), `:331`(ISM) |
| 중요도와 표현 LOD | `LODSignificance` = LOD 정수 + 밴드 안 비율. 개수 집계 경로는 버킷 번호 ÷ 버킷 수라 [LOD, LOD+1)을 넘지 않는다. 그래서 [1, 2)가 Medium, [2, 3)이 Low이고 범위 경계 2.0이 표현 LOD Medium/Low 경계(80m)와 같다 | `MG/MassLOD/Public/MassLODCalculator.h:409-416, 745-753` |
| 범위를 넘는 프레임 | `AddBatchedTransform`·`AddBatchedAnimationData`·`AddBatchedCustomData`에 이전 중요도를 주면 새 범위와, 새 범위에 없는 옛 범위 컴포넌트 모두에 넣는다. 다음 프레임 옛 컴포넌트에서 갱신되지 않은 움직이는 인스턴스는 지워진다. 그래서 한 프레임 겹쳐 그리고 넘어간다 | `MR/Public/MassSkinnedMeshRepresentationTypes.h:665-726`, `MR/Private/MassVisualizationComponent.cpp:1440-1475, 812-827` |
| 컴포넌트마다 트랙이 따로다 | ASTPD면 컴포넌트마다 `UAnimSequenceTransformProviderDataInstance`를 만들고 트랙 맵도 공유 데이터(컴포넌트)마다다. 트랙이 이미 있으면 클립 번호가 다를 때만 `SetAutoPlayData`를 부르므로, 예전에 머물던 컴포넌트로 돌아오면 옛 시작 시각의 같은 클립 트랙이 남아 위상이 튈 수 있다. `ULabIskmProcessor`가 범위가 바뀐 프레임에 새 컴포넌트의 트랙을 직접 다시 쓴다 | `MR/Private/MassVisualizationComponent.cpp:1323-1326, 761-775, 852-866` |

## 23. Niagara 컴포넌트 그림자와 렌더러 그림자(2026-11-12, Q2c 코드 세션)

줄임말: **NI** = `Engine/Plugins/FX/Niagara/Source/Niagara`.

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 메시 배치 그림자는 곱이다 | Niagara 메시 렌더러의 `MeshBatch.CastShadow`(레이 트레이싱 그림자도 같다)는 `SceneProxy.CastsDynamicShadow() && bCastShadows(렌더러 속성) && Section.bCastShadow`다. 스프라이트 렌더러도 `CastsDynamicShadow() && bCastShadows`다. 그래서 컴포넌트를 끄면 모든 이미터가 꺼지고, 컴포넌트를 켜도 렌더러에서 끈 이미터는 켜지지 않는다 | `NI/Private/NiagaraRendererMeshes.cpp:124, 1290-1292`, `NI/Private/NiagaraRendererSprites.cpp:106, 912-914` |
| 프록시 값 | 프록시 설명에 `CastShadow(CastsDynamicShadow())`를 넘기고, 엔진 프록시는 `bCastDynamicShadow && CastShadow`로 정한다 | `NI/Private/NiagaraComponent.cpp:479`, `Engine/Source/Runtime/Engine/Private/PrimitiveSceneProxy.cpp:355` |
| 컴포넌트 기본값은 끔 | `UPrimitiveComponent` 생성자가 `CastShadow = false`(`bCastDynamicShadow = true`)로 두고 `UNiagaraComponent` 생성자는 바꾸지 않는다. 시스템 `bOverrideCastShadow`가 꺼져 있으면 `SetAsset`이 컴포넌트 값을 그대로 둔다. 그래서 `SpawnSystemAtLocation`으로 만든 컴포넌트는 렌더러 그림자를 켜도 그림자가 없다(실행 로그 `fx_shadow_spawn=0`). 풀로 돌려받는 컴포넌트도 `SetCastShadow(false)`가 된다 | `Engine/Source/Runtime/Engine/Private/Components/PrimitiveComponent.cpp:341-343`, `NI/Private/NiagaraComponent.cpp:4678`, `NI/Private/NiagaraComponentPool.cpp:138` |
| 실행 중 바꾸기 | `SetCastShadow`는 값이 바뀌면 `MarkRenderStateDirty`로 프록시를 다시 만든다 | `PrimitiveComponent.cpp:2212-2219` |

## 24. Niagara 입자 스폰, 배열 DI, MCP 한계(2026-11-12, Q2b 에셋 세션 기록을 이 문서로 옮김)

줄임말: **NI** = `Engine/Plugins/FX/Niagara/Source/Niagara`. 아래 위치는 Q2 Worklog가 적은 것이고 문서 점검 때 소스에서 다시 확인했다(`ExecIndex` 행 제외).

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 보간 스폰 끔(`NoInterpolation`) | 새 입자가 스폰된 프레임에 업데이트 스크립트를 돌지 않는다. 그래서 수명을 프레임보다 아주 짧게 두면 프레임 N에 스폰된 입자가 N에 그려지고 N+1의 업데이트에서 죽는다(엔티티 구동 입자의 근거) | `NI/Public/NiagaraCommon.h:230-243` |
| 배열 DI의 GPU 동기 기본값 | `GpuSyncMode`가 `SyncCpuToGpu`다. 이미터 스크립트(CPU)의 개수 읽기와 입자 스크립트(GPU)의 원소 읽기가 같은 배열에서 온다 | `NI/Classes/NiagaraDataInterfaceArray.h:141-143` |
| GPU 스폰의 `ExecIndex` | 스폰 그룹 안의 순번이다(`GGPUExecIndex = GLinearThreadId − GSpawnStartInstance − GroupSpawnStartIndex`). 소스는 이 점검에서 다시 열지 않았다 | `NiagaraEmitterInstanceShader.usf:162-191`(Q2.md 인용) |
| 컴포넌트에서 워밍업을 덮어쓸 수 없다 | `bOverrideWarmupSettings`·`WarmupTickCount`·`WarmupTickDelta`가 `private:` 구역(88줄부터)에 있고 설정 함수가 없다. 워밍업을 바꾸려면 시스템 에셋을 고친다 | `NI/Classes/NiagaraComponent.h:88, 138-148` |
| MCP로 배열(DI) 사용자 변수를 만들 수 없다 | `FNiagaraParameterStore::SetParameter_InternalUseOnly`가 데이터 인터페이스 키를 받으면 오류 로그만 남기고 돌아온다. MCP `AddUserVariables`로 배열 변수를 만들면 오류 없이 아무것도 생기지 않은 까닭이다. 대신 다이내믹 입력의 배열 입력을 사용자 변수에 연결하면(`SetStackInputData`) DI 인스턴스와 변수가 함께 생긴다 | `NI/Private/NiagaraParameterStore.cpp:1457-1461` |
| 이미터 로드 때 보간 스폰 불일치 경고 | 로드 때 이미터의 `InterpolatedSpawnMode`와 스폰 스크립트 종류가 다르면 엔진이 메모리에서 `NoInterpolation`으로 맞추고 "Disabling interpolated spawn…" 경고를 남긴다. MCP `SetEmitterData`는 `PostEditChange`를 부르지 않아 스폰 스크립트 용도가 템플릿 값으로 저장돼 이 경고가 생겼다. 동작은 같고, 에디터에서 한 번 다시 저장하면 없어진다고 Q2 세션이 적었다(쿡된 빌드에서의 경고 여부는 보지 않았다) | `NI/Private/NiagaraEmitter.cpp:859-879`(Q2.md는 861-878로 적었다) |
| 화면 왼쪽 위 "CsvProfiler frame: N" 메시지 | 비출시 구성에서 csv 캡처 중(`IsCapturing`)이고 `bShowScreenMessages`(= `!GIsHighResScreenshot && !GIsDumpingMovie && GAreScreenMessagesEnabled`)이거나 `csv.AlwaysShowFrameCount`(기본 false)일 때 그린다. 캡처를 끄지 않고 `DisableAllScreenMessages` 명령(`-ExecCmds=DisableAllScreenMessages`)으로 `GAreScreenMessagesEnabled`를 꺼서 없앤다. 글자는 소문자 "frame"이다. HUD(`AHUD::DrawHUD`)는 이 플래그와 무관하다. Test 구성에서 `-ExecCmds`가 먹는지는 Worklog Q6 "촬영 준비"에 적는다 | `Engine/Private/UnrealEngine.cpp:13630, 13786-13799`(그리기), `5951-5954, 10595-10598`(명령), `608-612`(cvar) |

## 25. 카메라 리그의 FOV(2026-11-12, README 시나리오 그림)

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| `ALabCameraRig`의 가로 FOV | `ACameraActor`의 카메라 컴포넌트 기본값 90도 그대로다(리그는 FOV를 바꾸지 않는다). 저격의 화면 안 판정도 이 값과 16:9를 쓴다 | `Engine/Source/Runtime/Engine/Private/Camera/CameraComponent.cpp:78`, `Source/HordeOptLab/LabOrb.cpp:155` |


## 26. Niagara 메시 입자의 GPU 비용 경로(2026-11-12, 입자 GPU 비용 조사)

줄임말: **NI** = `Engine/Plugins/FX/Niagara/Source/Niagara`, **RD** = `Engine/Source/Runtime/Renderer/Private`. 실행 결과는 [Worklog/Q6.md](../Worklog/Q6.md) "입자 GPU 비용 조사".

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 메시 렌더러의 LOD 방식 기본값 | `FNiagaraMeshRendererMeshProperties::LODMode` 기본은 `LODLevel`, `LODLevel` 기본 0이다. 지정하지 않으면 모든 입자가 LOD0을 그린다. `PerParticle`이면 메시의 LOD 범위 전부를 메시 데이터로 만들고 입자마다 화면 크기로 LOD를 고른다(LOD가 둘 이상일 때 `bEnableLODCulling`) | `NI/Public/NiagaraMeshRendererMeshProperties.h:29-65, 126, 139`, `NI/Private/NiagaraRendererMeshes.cpp:234-275` |
| 모션 벡터 끔이 속도 패스를 빼지 않는다 | Niagara 프록시의 속도 관련성은 `DrawsVelocity()`다. `DrawsVelocity()`는 `HasDynamicTransform()`(= Movable)이나 WPO 속도(`bHasWorldPositionOffsetVelocity`, WPO 머티리얼이고 `r.Velocity.EnableVertexDeformation`이 켜져 있으면)만으로도 참이다. 렌더러 `MotionVectorSetting` = Disable은 `bAlwaysHasVelocity`(시스템의 어느 렌더러든 모션 블러를 쓰면 참)만 끈다. 그래서 Movable 컴포넌트에 VAT(WPO) 머티리얼을 쓰는 입자는 설정과 무관하게 속도 패스에서 다시 그려진다 | `NI/Private/NiagaraComponent.cpp:303, 393`, `NI/Private/NiagaraSystemRenderData.cpp:297`, `Engine/Source/Runtime/Engine/Public/PrimitiveSceneProxy.h:923-927`, `Engine/Source/Runtime/Engine/Private/PrimitiveSceneProxy.cpp:620-624` |
| Lumen 카드 캡처 대상 | 카드 캡처는 `AffectsDynamicIndirectLighting()`이고 카드 OBB와 겹치는 프리미티브 그룹에 대해 돈다. 메시 카드가 있는 프리미티브의 경로라 Niagara 메시 입자는 카드 캡처 비용을 만들지 않는다(입자 컴포넌트의 `AffectDynamicIndirectLighting`을 꺼도 GPU 시간이 같았다) | `RD/Lumen/LumenSceneCardCapture.cpp:770-795`, `RD/Lumen/LumenSceneRendering.cpp:397-403` |
| `LumenSceneLighting`은 비동기 계산 큐에서 돈다 | `r.Lumen.AsyncCompute`(기본 1)와 `r.LumenScene.Lighting.AsyncCompute`(기본 1)가 모두 켜져 있으면 비동기 계산 큐에서 돈다. 그래픽 큐가 바쁘면 타이머의 시작\~끝 구간이 길어져 패스 자체 비용보다 크게 잰다 | `RD/Lumen/Lumen.cpp:24-29`, `RD/Lumen/LumenSceneLighting.cpp:64-68, 209-212` |
| AnimToTexture는 LOD를 골라 굽는다 | 데이터 에셋의 `SkeletalLODIndex`(구울 스켈레탈 LOD), `StaticLODIndex`(구워 넣을 스태틱 LOD), 둘 다 기본 0이다. 저폴리 VAT 메시는 원본 스켈레탈 LOD를 골라 따로 구우면 UV 채널 매핑이 맞는다. 스태틱 메시를 감축해 만든 LOD는 정점이 굽기와 대응하지 않아 모양이 깨진다(이번 시험에서 확인) | `Engine/Plugins/Experimental/AnimToTexture/Source/AnimToTexture/Public/AnimToTextureDataAsset.h:124-139` |
| 스켈레탈 LOD 하나를 스태틱 메시로 바꾸기 | `ConvertSkeletalMeshToStaticMesh(SkeletalMesh, PackageName, LODIndex)`는 `LODIndex` ≥ 0이면 그 LOD 하나만 스태틱 메시의 LOD0(소스 모델 0)에 넣는다(−1이면 모든 LOD를 복사). 그래서 원본 LOD N을 구울 때 데이터 에셋은 `SkeletalLODIndex` = N, `StaticLODIndex` = 0이다(2026-11-12 입자 전용 저폴리 메시, `bake-vat.py --lod`) | `Engine/Plugins/Experimental/AnimToTexture/Source/AnimToTextureEditor/Private/AnimToTextureBPLibrary.cpp:777-790, 830-868` |
| Bone 모드 텍스처와 인스턴스는 LOD마다 따로다 | `BonePosition`·`BoneRotation` 텍스처는 참조 스켈레톤 전체 뼈 수(`GetRawBoneNum`) × (프레임 + 1) 크기라 LOD와 무관하다. `BoneWeight` 텍스처는 스태틱 메시 정점마다 한 칸(`FindBestResolution(2, NumVertices)`)이고 그 위치를 UV 채널에 써 넣는다(`CreateUVChannel`). 머티리얼 인스턴스는 `BoneWeightsTexture`와 `BoneWeightRowsPerFrame`을 받는다. 그래서 정점 수가 다른 LOD는 BoneWeight 텍스처와 인스턴스를 따로 가져야 하고, 부모 머티리얼(`MF_BoneAnimation`을 부르는 것)은 함께 쓸 수 있다 | `Engine/Plugins/Experimental/AnimToTexture/Source/AnimToTextureEditor/Private/AnimToTextureBPLibrary.cpp:252-343, 664-677`, `Engine/Plugins/Experimental/AnimToTexture/Source/AnimToTextureEditor/Private/AnimToTextureSkeletalMesh.cpp:292-298` |
| 실행 인자로 ini 값 덮어쓰기 | `-ini:<ini 이름>:[<섹션>]:<키>=<값>`이 그 키를 실행 때 바꾼다. 입자 메시 비교에서 `-ini:Game:[Lab]:CrowdFxSystem=<복제한 시스템 경로>`로 에셋을 바꾸지 않고 시스템을 골랐다(로그 `lab_crowdfx ... system=NS_LabCrowd_Lod3`로 확인) | `Engine/Source/Runtime/Core/Private/Misc/ConfigCacheIni.cpp:1716-1718, 2322-2332` |
| 에디터 스크립트의 LOD 만들기 | `UStaticMeshEditorSubsystem::SetLods`(Python `EditorStaticMeshLibrary.set_lods`)는 `-run=pythonscript` 커맨들릿에서 -1을 돌려주고 아무것도 하지 않는다(`CheckIfInEditorAndPIE`). 에디터를 `-ExecutePythonScript=`로 띄우면 된다. 커맨들릿에서는 `get_editor_subsystem(StaticMeshEditorSubsystem)`도 None이다 | `Engine/Source/Editor/StaticMeshEditor/Private/StaticMeshEditorSubsystem.cpp:358-362` |

## 27. 한 프레임의 처리 단계와 프로세서 자리(2026-11-12, 코드 투어)

줄임말: **ME** = `Engine/Source/Runtime/MassEntity`, **MG** = `Engine/Plugins/Runtime/MassGameplay/Source`, **MN** = `Engine/Plugins/AI/MassAI/Source/MassNavigation`. 정리한 흐름은 [code-tour.md](../Guides/code-tour.md)에 있다.

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 프로세서 기본값 | `ProcessingPhase` 기본 `PrePhysics`, `bAutoRegisterWithProcessingPhases` 기본 true, `bRequiresGameThreadExecution` 기본 false | `ME/Public/MassProcessor.h:244,252,259` |
| 처리 단계와 틱 그룹 | PrePhysics → `TG_PrePhysics`, StartPhysics → `TG_StartPhysics`, DuringPhysics → `TG_DuringPhysics`, EndPhysics → `TG_EndPhysics`, PostPhysics → `TG_PostPhysics`, FrameEnd → `TG_LastDemotable`. 같은 틱 그룹의 액터 틱과 Mass 단계 사이에는 선후 선언이 없다. `mass.MakePrePhysicsTickFunctionHighPriority`(기본 true)는 PrePhysics 틱 함수와 그 선행 틱을 높은 우선순위로 올릴 뿐이다 | `ME/Private/MassProcessingPhaseManager.cpp:32,44-51,479-483` |
| ISM·ISKM 일괄 갱신 시점 | `UMassRepresentationSubsystem`이 PrePhysics 단계가 시작할 때 `BeginVisualChanges`, PostPhysics 단계가 끝날 때 `EndVisualChanges`를 부른다. 표현 프로세서가 쌓은 위치·커스텀 데이터·애니 데이터는 이때 컴포넌트에 올라간다 | `MG/MassRepresentation/Private/MassRepresentationSubsystem.cpp:444-445,526-540` |
| 그룹 이름 | `SyncWorldToMass`, `Tasks`, `Avoidance`, `ApplyForces`, `Movement`(MassCommon), `LODCollector`, `LOD`(MassLOD), `Representation`, `Representation.VisualizationProcessing`(MassRepresentation, 모든 `UMassVisualizationProcessor` 하위 클래스가 든다) | `MG/MassCommon/Private/MassCommonTypes.cpp:10-15`, `MG/MassLOD/Private/MassLODSubsystem.cpp:26-27`, `MG/MassRepresentation/Public/MassRepresentationTypes.h:25-31` |
| 엔진 프로세서의 자리 | `UMassLODCollectorProcessor` LODCollector, `SyncWorldToMass` 뒤. `UMassSimulationLODProcessor`·`UMassVisualizationLODProcessor` LOD, LODCollector 뒤. `UMassSteerToMoveTargetProcessor` 그룹 없음, Tasks 뒤·Avoidance 앞. `UMassApplyForceProcessor` ApplyForces, Avoidance 뒤. `UMassApplyMovementProcessor` Movement, ApplyForces 뒤. `UMassSmoothOrientationProcessor` Movement. `UMassRepresentationProcessor` Representation, LOD 뒤이고 하위 `UMassVisualizationProcessor`는 그룹만 VisualizationProcessing으로 바꾼다(LOD 뒤는 남는다) | `MG/MassLOD/Private/MassLODCollectorProcessor.cpp:18-19`, `MG/MassLOD/Private/MassSimulationLOD.cpp:76-77`, `MG/MassRepresentation/Private/MassVisualizationLODProcessor.cpp:15-16`, `MN/Private/MassSteeringProcessors.cpp:62-63`, `MG/MassMovement/Private/MassMovementProcessors.cpp:25-26,76-77`, `MN/Private/MassSmoothOrientationProcessors.cpp:25`, `MG/MassRepresentation/Private/MassRepresentationProcessor.cpp:56-57,523` |
| 엔진 ISKM 애니 프로세서 | `UMassConsumeInstancedSkinnedMeshAnimationProcessor`는 자동 등록(기본값)이고 Representation 그룹, VisualizationProcessing 뒤, 게임 스레드다. 쿼리가 `FMassRepresentationAnimationFragment`를 요구해 그 프래그먼트가 없는 엔티티는 돌지 않는다 | `MG/MassRepresentation/Private/MassRepresentationAnimationProcessor.cpp:17-33` |
| 쿼리로 프로세서 빼기 | 의존성 해결기는 쿼리에 맞는 아키타입이 하나도 없는 프로세서를 그래프에서 뺀다(`ShouldAllowQueryBasedPruning`, 게임 실행에서 `QueryBasedPruning` 기본이 Prune). 그래서 등록된 프로세서와 그래프의 노드는 다를 수 있다 | `ME/Private/MassProcessorDependencySolver.cpp:968-1012`, `ME/Private/MassProcessor.cpp:385-388`, `ME/Public/MassProcessor.h:320-322` |

## 28. URO 기준과 Animation Budget Allocator의 배분(2026-11-13, 글 검토)

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| URO 기준의 단위 | `BaseVisibleDistanceFactorThesholds`(0.24, 0.12)와 견주는 `MaxDistanceFactor`는 화면 크기가 아니라 화면 반지름의 제곱(`ScreenRadiusSquared`)이다. LOD는 `Square(ScreenSize * 0.5)`와 견주므로 LOD 화면 크기로는 2 × √0.24 = 0.98, 2 × √0.12 = 0.69다 | `Engine/Source/Runtime/Engine/Private/SkeletalRender.cpp:173,203`, `Components/SkinnedMeshComponent.cpp:310-318` |
| 예산을 넘을 때의 배분 | 매 프레임 갱신하는 수는 `WorkUnitBudget − WorkUnitsExcess × AlwaysTickFalloffAggression(0.8)`이다. 나머지는 간격이 늘고 상한은 `MaxTickRate`(10)다 | `AnimationBudgetAllocator.cpp:567`, `AnimationBudgetAllocatorParameters.h:34,49` |
| 그려지지 않은 메시 | 최근 1초 동안 그려지지 않은 메시는 틱하지 않는다(`bTickEvenIfNotRendered`가 꺼져 있을 때) | `AnimationBudgetAllocator.cpp:174,256` |
| 1만에서의 예산 사용 | 엔진 통계 `AnimationBudget/GameThread/BudgetedAnimation`의 실행별 중앙값이 4단계 1.96\~1.98ms, 7단계 2.01\~2.02ms, 8단계 1.86\~1.88ms다(호드 1만, 오브 시점). 예산 2ms에 닿아 있다 | 프레임 CSV `Saved/Profiling/CSV/Full2-L4-mass-N10000-P1-r<N>.csv` 등(git 밖) |

## 29. Animation Budget Allocator의 한 프레임(2026-11-13, Q7 코드 세션)

줄임말: **ABA** = `Engine/Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator`, **ENG** = `Engine/Source/Runtime/Engine`. 28절을 넓힌다. 줄 번호는 5.8.4 소스에서 다시 열어 확인했다.

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 갱신 시점 | 할당기는 `FWorldDelegates::OnWorldPreActorTick`에서 `Update`를 부른다. 이 대리자는 틱 그룹보다 먼저 방송된다. 그래서 그 프레임의 갱신·보간·건너뜀은 `TG_PrePhysics`의 액터 틱과 Mass PrePhysics 처리보다 먼저 정해진다 | `ABA/Private/AnimationBudgetAllocator.cpp:797-803`, `ENG/Private/LevelTick.cpp:1675,1750` |
| 수요(틱할 메시) | 등록된 메시 중 최근 1초 안에 그려진 것만 센다(`World->TimeSeconds − 1.0`과 `GetLastRenderTime`을 견준다). `bTickEvenIfNotRendered`가 꺼져 있으면 그려지지 않은 메시는 틱을 끈다(`DisableComponentTick`). 켜져 있으면 중요도 순으로 `MaxTickedOffsreenComponents`(4)개까지만 틱한다 | `AnimationBudgetAllocator.cpp:174,254-263,279-301,346-395` |
| 정렬 | `bAlwaysTick`이 먼저, 그다음 중요도가 큰 순이다 | `AnimationBudgetAllocator.cpp:327-342` |
| 메시 하나의 시간 | `USkeletalMeshComponentBudgeted::TickComponent`와 `CompleteParallelAnimationEvaluation`이 게임 스레드에서 걸린 시간을 각각 재서 넘긴다. 워커 스레드의 병렬 애니메이션 평가 시간은 들어가지 않는다. 엔진 CSV 통계 `AnimationBudget/BudgetedAnimation`도 같은 두 구간을 잰다 | `ABA/Private/SkeletalMeshComponentBudgeted.cpp:91-157`(재는 줄 `:94,99,127,139,144,150`) |
| 평균 작업 단위 | 지난 프레임의 메시별 시간(틱 + 완료)의 평균을 `WorkUnitSmoothingSpeed`(5)로 부드럽게 따라간다. 처음 값은 `InitialEstimatedWorkUnitTimeMs`(0.08ms)다 | `AnimationBudgetAllocator.cpp:202-203,549-550`, `AnimationBudgetAllocatorParameters.h:41,87` |
| 매 프레임 갱신하는 수 | 작업 단위 예산 = `max((BudgetInMs − 외부 애니 시간) / 평균 작업 단위, 수요 × MinQuality)`. 넘은 몫 = `max(0, 수요 − 작업 단위 예산)`. 매 프레임 갱신 수 = `clamp(작업 단위 예산 − 넘은 몫 × AlwaysTickFalloffAggression(0.8), bAlwaysTick 수, 수요)` | `AnimationBudgetAllocator.cpp:558-570` |
| 보간 몫 | 남은 예산 = `max(0, 작업 단위 예산 − 매 프레임 갱신 메시의 실제 시간 / 평균)`. 보간 수 = `min(max(남은 예산 − 넘은 몫 × InterpolationFalloffAggression(0.4), min(MaxInterpolatedComponents(16), bNeverThrottle 수)), 남은 수요)`. 보간 메시의 틱 간격은 순위에 따라 2에서 `InterpolationMaxRate`(6)까지 선형이다 | `AnimationBudgetAllocator.cpp:574-591,621-631` |
| 나머지(건너뜀) | 남은 메시는 간격을 2에서 `min(ceil(max(1, 남은 수요 / 남은 예산) × 2), MaxTickRate(10))`까지 순위에 따라 늘린다. 보간은 하지 않는다 | `AnimationBudgetAllocator.cpp:593-607,633-643` |
| 프레임마다 갱신·보간·건너뜀 | 메시는 `(GFrameCounter + FrameOffset) % TickRate == 0`인 프레임에 갱신한다(`EnableExternalUpdate(true)`). 보간 메시는 그 밖의 프레임에도 화면 안이면 틱을 켜고 갱신은 끈다(이전 두 포즈 사이를 잇는다). 건너뛰는 메시는 틱을 끄고 갱신도 끈다. 새 간격은 갱신하는 프레임에만, `StateChangeThrottle`이 음수일 때만 반영된다 | `AnimationBudgetAllocator.cpp:440,454-500`, `ENG/Private/Components/SkeletalMeshComponent.cpp:1918-1923,2909-2911` |
| `ShouldTickAnimation()`의 뜻 | 외부 틱 제어 중이면 `bExternalUpdate`를 그대로 돌려준다. 보간 프레임과 건너뛴 프레임 모두 거짓이다. 보간 여부는 `IsUsingExternalInterpolation()`(`bExternalInterpolate && bExternalEvaluationRateLimited`)과 틱 함수 켬으로 가른다 | `ENG/Private/Components/SkeletalMeshComponent.cpp:1918-1923`, `ENG/Classes/Components/SkinnedMeshComponent.h:1015-1039` |
| 등록 때 바뀌는 것 | 할당기가 켜져 있으면 등록하는 메시의 URO를 끄고(`bEnableUpdateRateOptimizations = false`) 외부 틱 제어를 켠다 | `AnimationBudgetAllocator.cpp:1191-1197` |
| `SetComponentSignificance` 기본 인자 | `bNeverSkip = false`, `bTickEvenIfNotRendered = false`, `bAllowReducedWork = true`, `bForceInterpolate = false`. 마지막 인자가 컴포넌트 데이터의 `bNeverThrottle`이 된다. 등록 때 기본값은 `bNeverThrottle = true`지만 이 함수를 기본 인자로 부르면 거짓이 된다 | `ABA/Public/SkeletalMeshComponentBudgeted.h:36`, `AnimationBudgetAllocator.cpp:54,151-165` |
| 예산 값의 하한 | `a.Budget.BudgetMs`는 0.1보다 작게 넣으면 0.1로 올린다. 예산을 끄는 길은 이 값이 아니라 `a.Budget.Enabled`와 월드 할당기의 `SetEnabled`다 | `ABA/Private/AnimationBudgetAllocatorCVars.cpp:195-203`, 28절 위 `LabGameMode.cpp` `IsBudgetActive` |
| 추가 CSV 통계 | `NumAlwaysTicked`, `NumInterpolated`, `NumThrottled`, `NumTicked`, `AnimQuality`, `AverageTickRate`는 `WITH_EXTRA_BUDGET_CSV_STATS`(= `WITH_TICK_DEBUG` = 0)일 때만 CSV에 남는다. 기본 빌드에 남는 것은 `AverageWorkUnitTimeMs`와 `BudgetedAnimation`, `Exclusive/GameThread/AnimationBudgetAllocator`다 | `ABA/Private/AnimationBudgetAllocator.h:15-24`, `AnimationBudgetAllocator.cpp:553,808` |

`a.Budget.*` CVar의 엔진 기본값(`FAnimationBudgetAllocatorParameters`와 CVar 파일)이다. 이 프로젝트가 바꾸는 것은 `a.Budget.Enabled`(1)와 `a.Budget.BudgetMs`(기본 2, `-LabBudgetMs=`)뿐이다.

| CVar | 기본값 | 위치 |
| --- | --- | --- |
| `a.Budget.Enabled` | 0 | `AnimationBudgetAllocatorCVars.cpp:7-14` |
| `a.Budget.BudgetMs` | 1.0(하한 0.1) | `AnimationBudgetAllocatorParameters.h:19`, `AnimationBudgetAllocatorCVars.cpp:195-203` |
| `a.Budget.MinQuality` | 0.0 | `AnimationBudgetAllocatorParameters.h:27` |
| `a.Budget.MaxTickRate` | 10 | `:34` |
| `a.Budget.WorkUnitSmoothingSpeed` | 5.0 | `:41` |
| `a.Budget.AlwaysTickFalloffAggression` | 0.8(범위 0.1\~0.9) | `:49` |
| `a.Budget.InterpolationFalloffAggression` | 0.4 | `:58` |
| `a.Budget.InterpolationMaxRate` | 6 | `:65` |
| `a.Budget.MaxInterpolatedComponents` | 16 | `:72` |
| `a.Budget.InterpolationTickMultiplier` | 0.75 | `:79` |
| `a.Budget.InitialEstimatedWorkUnitTime` | 0.08ms | `:87` |
| `a.Budget.MaxTickedOffsreen` | 4 | `:94` |
| `a.Budget.StateChangeThrottleInFrames` | 30 | `:101` |
| `a.Budget.BudgetFactorBeforeReducedWork` | 1.5 | `:108` |
| `a.Budget.BudgetFactorBeforeReducedWorkEpsilon` | 0.25 | `:115` |
| `a.Budget.BudgetPressureSmoothingSpeed` | 3.0 | `:122` |
| `a.Budget.ReducedWorkThrottleMinInFrames` | 2 | `:129` |
| `a.Budget.ReducedWorkThrottleMaxInFrames` | 20 | `:136` |
| `a.Budget.BudgetFactorBeforeAggressiveReducedWork` | 2.0 | `:143` |
| `a.Budget.ReducedWorkThrottleMaxPerFrame` | 4 | `:150` |
| `a.Budget.BudgetPressureBeforeEmergencyReducedWork` | 2.5 | `:157` |
| `a.Budget.AutoCalculatedSignificanceMaxDistance` | 30000(300m) | `:165` |
| `a.Budget.MaintainReduceWorkWhenOffScreen` | 0 | `AnimationBudgetAllocatorCVars.cpp:18-25` |
| `a.Budget.ComponentsWithZeroSigAreNonRendered` | 0 | `:27-34` |
| `a.Budget.ForceTickWhenComponentExitsOffScreen` | 0 | `:36-43` |
| `a.Budget.EmergencyReduceWorkDisabled` | 0 | `:45-52` |

## 30. Windows에서 게임 프로세스의 종료 코드(2026-11-13, Q7 코드 세션)

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 비강제 종료의 코드 | `FWindowsPlatformMisc::RequestExitWithStatus(false, 코드)`는 엔진 종료를 요청하고 `PostQuitMessage(코드)`를 부를 뿐이다. `GuardedMain`은 틱 루프가 끝나면 초기화 때의 `ErrorLevel`(보통 0)을 돌려주고, 실행 경로에서 `WM_QUIT`의 코드를 읽는 곳이 없다. 그래서 프로세스 종료 코드는 0이다 | `Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformMisc.cpp:1474-1522`(1520줄), `Engine/Source/Runtime/Launch/Private/Launch.cpp:188-204` |
| 강제 종료의 코드 | `RequestExitWithStatus(true, 코드)`는 로그를 비운 뒤 `TerminateProcess(코드)`다. `WindowsPlatform.RaiseExceptionOnFaultTermination`(기본 거짓)이 참이면 코드가 0이 아닐 때 먼저 예외를 던진다 | `WindowsPlatformMisc.cpp:1493-1515`, `:140-144` |
| 이 프로젝트에 주는 뜻 | 설정·에셋 오류는 강제 종료로 코드 2·3을 낸다(`LabRequestExit`). 벤치 끝의 비강제 종료(`BenchExitCode`)는 늘 0이라 실패는 `LabOk`와 CSV 유무로 잡는다 | `Source/HordeOptLab/HordeOptLab.h`, Worklog/Q7.md "종료 코드" |

## 31. 엔진 Mass가 ISM에 쓰는 호출과 이동 프로세서의 식(2026-11-13, 변형 `manager` 코드 세션)

변형 `manager`가 2단계와 같은 결과를 내도록 옮긴 줄이다(Worklog/Q8.md "manager 변형: 코드"). `MR`은 `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation`, `MN`은 `Engine/Plugins/AI/MassAI/Source/MassNavigation`이다.

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 표현이 있는 개체의 가시성 | `UpdateEntityVisibility`는 현재 표현이 `None`이 아니면 가시성을 `CanBeSeen`으로 둔다. 2단계는 모든 LOD가 `StaticMeshInstance`라 화면 밖 개체도 `CanBeSeen`이다. 그래서 `UMassUpdateISMProcessor`의 청크 필터(`AreAnyEntitiesVisibleInChunk`)가 아무 청크도 거르지 않고 전 개체의 변환을 매 프레임 쌓는다 | `MR/Private/MassRepresentationProcessor.cpp:601-610`, `MR/Private/MassUpdateISMProcessor.cpp:30,56-65` |
| 쌓는 변환 | `AddBatchedTransform`은 `LocalTransform * 엔티티 변환`과 `LocalTransform * 지난 프레임 변환`을 쌓는다 | `MR/Private/MassVisualizationComponent.cpp:909-927` |
| ISM에 쓰는 호출 | `EndVisualChanges`는 쌓인 개체마다 엔티티 → 인스턴스 ID 해시 맵을 찾는다. 있으면 `UpdateInstanceTransformById` + `SetPreviousTransformById`, 없으면 `AddInstanceById` + `SetPreviousTransformById`를 부른다. 그 프레임에 쌓이지 않은 인스턴스는 `RemoveInstancesById`로 뗀다. 매번 `SetNumCustomDataFloats`와 `SetHasPerInstancePrevTransforms(true)`도 부른다 | `MR/Private/MassVisualizationComponent.cpp:566-665`(갱신 617-621, 제거 647, 추가 652-658) |
| ISM 컴포넌트를 만드는 설정 | `NewObject<UInstancedStaticMeshComponent>` 뒤에 `SetStaticMesh`, 머티리얼 덮어쓰기, `SetCullDistances(0, 1000000)`, `SetupAttachment(루트)`, `SetCanEverAffectNavigation(false)`, `NoCollision` 프로필, `SetCastShadow(설명자 값)`, `Mobility = 설명자 값`, `SetReceivesDecals(false)`, `RegisterComponent` 순이다. 설명자의 `Mobility` 기본값은 `Movable`이다 | `MR/Private/MassVisualizationComponent.cpp:238-262`, `MR/Public/MassRepresentationTypes.h:115` |
| 비주얼라이저 액터의 루트 | `AMassVisualizer`의 루트는 `Static` 모빌리티의 `USceneComponent`다 | `MR/Private/MassVisualizer.cpp:8-12` |
| `UpdateInstanceTransformById` | 인스턴스 ID를 색인으로 바꿔 `UpdateInstanceTransform`을 부른다. `SetPreviousTransformById`는 `bHasPreviousTransforms`를 `check`한다 | `Engine/Source/Runtime/Engine/Private/InstancedStaticMesh.cpp:3857-3876` |
| 이동 목표의 기본 동작 | `FMassMoveTargetFragment::CurrentAction`의 기본값은 `Move`다. 새 동작을 만들지 않으면 조향은 Move 갈래를 탄다 | `MN/Public/MassNavigationFragments.h:79` |
| 조향의 Move 갈래 | 조향 방향 = (`Center + Forward × Clamp(ForwardOffset + SteerForward, 0, 내다보는 거리)` − 위치)의 수평 방향. 속도 = `DesiredSpeed × CalcDirectionalSpeedScale(지금 방향, 조향 방향)`에 뒤처짐 보정(`Lerp(속도, MaxSpeed, min(−ForwardOffset ÷ 내다보는 거리, 1))`)을 건 값. 힘 = `(1 ÷ ReactionTime) × (조향 속도 − DesiredVelocity)`. 코드 구동 태그는 `Optional`이라 멈춘 개체도 돈다 | `MN/Private/MassSteeringProcessors.cpp:26-45,87,133-237` |
| 조향 파라미터 기본값 | `ReactionTime 0.3`, `LookAheadTime 1.0`, `bAllowSpeedVariance true` | `MN/Public/Steering/MassSteeringFragments.h:66-78` |
| 힘과 이동 | 힘 적용: `DesiredVelocity = (DesiredVelocity + 힘 × dt)`를 `MaxSpeed`로 자른다. 이동: `Velocity = DesiredVelocity`, 위치에 `Velocity × dt`를 더한다. 두 dt는 0.1초 상한이다. 이동만 코드 구동 태그를 요구한다 | `Engine/Plugins/Runtime/MassGameplay/Source/MassMovement/Private/MassMovementProcessors.cpp:41,56-57,88,100,115-128` |
| 회전 | 바라는 방향 = `LerpAngle(DesiredVelocity의 방위, MoveTarget.Forward의 방위, 이동 목표 가중치 ÷ 두 가중치의 합)`. 새 방위 = `ExponentialSmoothingAngle(지금 방위, 바라는 방향, dt, OrientationSmoothingTime)`. dt는 0.1초 상한이다. 기본값은 이동 중 가중치 0.4 / 0.6, 평활 시간 0.3초다 | `MN/Private/MassSmoothOrientationProcessors.cpp:47,72-112`, `MN/Public/SmoothOrientation/MassSmoothOrientationFragments.h:39-51`, `MN/Public/MassNavigationUtils.h:19-51` |

## 32. Mass Chunk 크기(2026-11-14, 글 02)

- `UMassEntitySettings::ChunkMemorySize`의 기본값은 `128 * 1024`다(`Engine/Source/Runtime/MassEntity/Public/MassEntitySettings.h:92`).
- Archetype은 만들 때 값이 0이면 이 기본값을 쓰고, 1KB\~512KB로 자른다(`Private/MassArchetypeData.cpp:19-25, 227`).

## 33. 글 재료 보강에서 확인한 것(2026-11-14, 글 02\~08)

경로 약어는 위 절과 같다(`MG` = `Engine/Plugins/Runtime/MassGameplay/Source`, `MR` = `MG/MassRepresentation`).

| 항목 | 확인한 것 | 위치 |
| --- | --- | --- |
| 움직이는 인스턴스의 Lumen 장면 갱신 | 인스턴스나 변환이 바뀐 프리미티브는 `LumenUpdatePrimitive`로 갱신 목록(`PendingUpdateOperations`)에 든다. `UpdateLumenScenePrimitives`가 프리미티브 그룹마다 `UpdateMeshCards`와 `UpdatePrimitiveGroupCullingInfo`를 부른다 | `Renderer/Private/RendererScene.cpp:6278-6279, 6359-6363`, `Renderer/Private/Lumen/LumenScene.cpp:926-943, 1422-1481` |
| `bAffectDynamicIndirectLighting`의 뜻 | 프리미티브가 간접광에 영향을 줄지 정한다. 끄면 Lumen 장면 포함 조건(11절) 가운데 이쪽만 꺼진다 | `Engine/Classes/Components/PrimitiveComponent.h:554-556` |
| 화면 밖 인스턴스와 청크 필터 | 표현이 `None`이 아닌 개체는 화면 밖이어도 `CanBeSeen`이라, `AreAnyEntitiesVisibleInChunk` 필터가 그 청크를 거르지 않는다(31절과 같은 사실). post-material의 "화면 밖 개체는 원래 갱신하지 않는다"는 이 줄로 정정했다 | `MR/Private/MassRepresentationProcessor.cpp:605` `UpdateEntityVisibility` |
| 퍼핏 | `EAgentComponentState`는 액터가 엔티티를 만드는 상태(`EntityPendingCreation`, `EntityCreated`)와 Mass가 움직이는 액터(주석 "Actor driven by Mass (puppet)")의 상태(`PuppetPendingInitialization` 등)를 나눈다 | `MG/MassActors/Public/MassAgentComponent.h:15-29` |
| 변환기의 복사 방향 | 태그(`FMassCharacterMovementCopyToMassTag`, `CopyToActorTag` 등)가 엔티티와 액터 사이의 복사 방향을 정한다 | `MG/MassActors/Public/Translators/MassCharacterMovementTranslators.h:23, 44`, `MassSceneComponentLocationTranslator.h:21, 43` |
| 속도 버퍼 | 정점의 지금 위치와 이전 프레임 위치를 각각 화면에 투영한 차이다 | `Shaders/Private/VelocityShader.usf:78-96, 277-280` |
| 속도 버퍼를 읽는 패스 | 모션 블러와 TSR이 읽는다 | `Renderer/Private/PostProcess/PostProcessMotionBlur.cpp:1334`, `TemporalSuperResolution.cpp:1898, 2537` |
| 인스턴스별 이전 변환이 없을 때 | 인스턴스가 컴포넌트와 함께만 움직였다고 본다(지금 변환 × 프리미티브의 `WorldToPreviousWorld`). 그래서 이전 변환이 빠지면 번지지 않고 블러가 빠진다. 번짐은 틀린 이전 변환에서 난다 | `Shaders/Private/SceneData.ush:1211-1236` |
| ISKM 재생 위치 | 렌더러가 매 프레임 그 프레임의 월드 시각으로 트랙의 재생 위치를 구하고, 활성 트랙마다 샘플과 GPU 평가 헤더를 쓴다. Animation Budget Allocator의 예산 밖이다 | `Renderer/Private/Skinning/AnimSequenceTransformProvider.cpp:1190, 1280-1305, 1415` |

## 34. `HashCombineFast`는 섞지 않는다(2026-11-15, v2 W1)

- `Engine/Source/Runtime/Core/Public/Templates/TypeHash.h`의 `HashCombineFast(A, B)`는 `A ^ (B + 0x9e3779b9 + (A << 6) + (A >> 2))`다. `GetTypeHash(uint32)`는 값을 그대로 돌려준다.
- 그래서 `HashCombineFast(일련번호, 틱)`의 최솟값으로 대상을 고르면 틱은 모든 후보에 같은 수를 더할 뿐이고, 작은 일련번호가 늘 최소가 된다. 같은 식을 Python으로 흉내 내 후보 2,000개와 틱 200개로 돌리면 200번 모두 가장 작은 일련번호가 뽑힌다.
- 같은 파일의 `MurmurFinalize32`로 섞으면 순서가 틱마다 바뀐다. `MurmurFinalize32(HashCombineFast(MurmurFinalize32(일련번호), 틱))`은 같은 흉내에서 200번에 서로 다른 대상 189개를 뽑고, 뽑힌 순위의 평균이 1,043이다(고르면 1,000).
- Lab은 저격 대상 선택에 이 식을 쓴다(`LabSnipe.h`의 `LabSnipeHash`).

## 35. Mass 프로세서의 서브시스템 접근 선언(2026-11-15, v2 W2)

- 서브시스템마다 특성 `TMassExternalSubsystemTraits<T>`를 특수화한다. 기본값은 `GameThreadOnly = true`, `ThreadSafeWrite = !GameThreadOnly`다(`Engine/Source/Runtime/Mass/MassCore/Public/Mass/ExternalSubsystemTraits.h:25-36`). 옛 헤더 `MassExternalSubsystemTraits.h`는 5.8에서 폐기 예정이다.
- `AddSubsystemRequirement<T>(접근)`은 `GameThreadOnly`가 참이면 그 프로세서를 게임 스레드에 묶는다(`MassEntity/Public/MassRequirements.h:100-123`).
- 의존성 계산은 `ThreadSafeWrite`가 참인 서브시스템을 따로 모은다. 그 서브시스템의 쓰기 선언은 읽기처럼 다뤄 프로세서 사이에 순서를 만들지 않는다(`MassEntity/Private/MassProcessorDependencySolver.cpp:1063-1086`, 헤더 주석 "all RW and RO operations will be viewed as RO").
- 프로세서 단위의 선언은 `ProcessorRequirements`에 한다(엔진 예: `MassVisualizationLODProcessor.cpp:50`). 엔진이 `Execute` 전에 `Context.CacheSubsystemRequirements(ProcessorRequirements)`를 부르고, 실패하면 `Execute`를 건너뛴다(`MassEntity/Private/MassProcessor.cpp:346-353`).
- 쿼리는 실행할 때 자기 요구 조건으로 접근 범위를 다시 잡는다(`MassEntity/Private/MassEntityQuery.cpp:50-53`). 그래서 프로세서 단위로 선언한 서브시스템은 `ForEachEntityChunk` 밖에서 얻는다.
- 얻는 함수는 `FMassExecutionContext::GetSubsystem<T>()`(읽기)와 `GetMutableSubsystem<T>()`(쓰기)다(`MassEntity/Public/MassExecutionContext.h:732-746`).
