# NS_LabCrowd 만들기

Q2 Niagara 배경 무리 `/Game/Lab/FX/NS_LabCrowd`와 머티리얼 인스턴스 `MI_LabCrowdSmall`·`MI_LabCrowdLarge`는 엔진 Niagara 템플릿과 굽기 산출물로 만든 에셋이라 커밋한다(Fab 콘텐츠가 아니다). 부모 머티리얼과 메시는 `Scripts/bake-vat.py`의 산출물(`/Game/Lab/VAT/Baked/`, gitignore)이므로 새 체크아웃에서는 먼저 `bake-vat.py`를 소형·대형 둘 다 돌린다. 설계와 값의 근거는 [Docs/Worklog/Q2.md](../Docs/Worklog/Q2.md).

다시 만들어야 하면 Unreal MCP로 아래 순서를 밟는다(2026-11-12에 이 순서로 만들었다). 에디터에서 손으로 해도 같다. 툴세트 이름은 `NiagaraToolsets.NiagaraToolset_System`(아래 "NS"), `editor_toolset.toolsets.material_instance.MaterialInstanceTools`(아래 "MIT")다. 여러 호출은 `editor_toolset.toolsets.programmatic.ProgrammaticToolset.execute_tool_script`로 묶으면 왕복이 준다.

## 1. 굽기 머티리얼에 Niagara 사용 플래그

1. `ObjectTools.set_properties`로 `/Game/Lab/VAT/Baked/M_LabMinionVAT_Bone`과 `M_LabMinionLargeVAT_Bone`에 `bUsedWithNiagaraMeshParticles` = true.
2. `MaterialTools.recompile` 뒤 `AssetTools.save_assets`.

`bake-vat.py`가 머티리얼을 다시 만들면 이 플래그가 사라진다. `bake-vat.py`에 넣기 전까지는 굽기 뒤에 이 단계를 다시 한다. 플래그가 없으면 쿡된 빌드에서 입자가 기본 머티리얼로 그려진다.

## 2. 머티리얼 인스턴스

1. MIT `create`: 폴더 `/Game/Lab/FX`, 이름 `MI_LabCrowdSmall`, 부모 `/Game/Lab/VAT/Baked/MI_LabMinionVAT_Bone`. 대형은 `MI_LabCrowdLarge`, 부모 `MI_LabMinionLargeVAT_Bone`.
2. MIT `set_static_switch_parameter`로 두 인스턴스 모두 `AutoPlay` = true, `UseDynamicParameters` = true. 엔진 `GetFrameSwitch`가 프레임을 Dynamic Material Parameter 1번(TimeOffset, Playrate, StartFrame, EndFrame)에서 읽게 된다. Niagara 메시 렌더러는 인스턴스 커스텀 데이터 0번에 입자 인덱스를 쓰므로 커스텀 데이터 경로는 쓸 수 없다(`Docs/Reference/engine-notes.md` 17절).

## 3. 시스템과 이미터

1. NS `CreateNiagaraSystem`: 이름 `NS_LabCrowd`, 경로 `/Game/Lab/FX`, 템플릿 `/Niagara/DefaultAssets/Templates/Systems/RadialBurst`. 딸려 온 이미터 셋(`RibbonTrailFollower`, `OmnidirectionalBurst`, `Ribbon_Trail_Leader`)을 `RemoveEmitter`로 지운다.
2. **시스템 상태를 무한 루프로 바꾼다.** `SetStackInputData`: 스크립트 `SystemUpdateScript`, 모듈 `SystemState`, 입력 `Loop Behavior` = `ENiagara_EmitterStateOptions::NewEnumerator0`(Infinite). 템플릿 값은 Once, 2초라 그대로 두면 처음 2초만 스폰하고 끝난다.
3. NS `SetSystemData`: `bFixedBounds` true, `FixedBounds` (−105000, −105000, −100)\~(105000, 105000, 1000), `bOverrideCastShadow` true, `bCastShadow` false, `WarmupTime` 268, `WarmupTickDelta` 0.125.
4. `AddUserVariables`(모두 float): `OuterRadius` 100000, `Speed` 300, `Lifetime` 266.6667, `SmallRate` 54, `LargeRate` 6.
5. 이미터마다(이름 `Small`, `Large`) 아래를 한다. 괄호 안이 대형 값이다.
   1. `AddEmitter`: 템플릿 `/Niagara/DefaultAssets/Templates/Emitters/Minimal`.
   2. `SetEmitterData`: `SimTarget` GPUComputeSim, `bLocalSpace` true, `CalculateBoundsMode` Fixed, `FixedBounds` 위와 같음, `bDeterminism` true, `RandomSeed` 1(2), `AllocationMode` ManualEstimate, `PreAllocationCount` 16000(2000).
   3. 스프라이트 렌더러를 `RemoveRenderer`(인덱스 0)로 지우고 `AddRenderer`로 `/Script/Niagara.NiagaraMeshRendererProperties`를 더한다.
   4. `SetRendererData`: `Meshes[0].mesh` `/Game/Lab/VAT/Baked/SM_LabMinionVAT_Bone`(`SM_LabMinionLargeVAT_Bone`), `Meshes[0].rotation` yaw −90, `Meshes[0].scale` 1(2), `bOverrideMaterials` true, `OverrideMaterials` [`MI_LabCrowdSmall`(`MI_LabCrowdLarge`)], `bCastShadows` false, `FacingMode` Velocity.
   5. `AddModule`: EmitterUpdateScript에 `/Niagara/Modules/Emitter/SpawnRate`, ParticleSpawnScript에 `/Niagara/Modules/Spawn/Location/V2/ShapeLocation`, `/Niagara/Modules/Spawn/Velocity/AddVelocityFromPoint`, `/Niagara/Modules/Update/Material/DynamicMatParams1`, ParticleUpdateScript에 `/Niagara/Modules/Solvers/SolveForcesAndVelocity`. (MCP의 `AddModule`이 결과를 만들고 나서 `_StrictDict.get()` 오류를 낼 수 있다. 모듈은 더해져 있다.)
   6. `SetStackInputData`:
      - `SpawnRate.SpawnRate` = 연결 `User.SmallRate`(`User.LargeRate`).
      - `InitializeParticle.Lifetime` = 연결 `User.Lifetime`.
      - `ShapeLocation.Shape Primitive` = `ENiagara_LocationShapes::NewEnumerator4`(Ring / Disc), `Ring Radius` = 연결 `User.OuterRadius`, `Disc Coverage` = 0.
      - `AddVelocityFromPoint.Velocity Strength` = HLSL 식 `-User.Speed`(원점을 향해 걷는다).
      - `DynamicMatParams1`: `Module.Param0WriteEnabled`\~`Param3WriteEnabled` 모두 true. `Param0Value` = 다이내믹 입력 `/Niagara/DynamicInputs/UniformRange/UniformRangedFloat`, `Minimum` 0, `Maximum` 62/30(41/30) = 걷기 위상. `Param1Value` 1(Playrate), `Param2Value` 0(StartFrame), `Param3Value` 61(40)(EndFrame).
6. `GetSystemCompileState`가 오류 없음, `GetStackIssues`가 경고만(아래)인지 보고 `AssetTools.save_assets`로 `NS_LabCrowd`, `MI_LabCrowdSmall`, `MI_LabCrowdLarge`를 저장한다.

남는 경고: `DynamicMatParams1` "모듈 폐기됨"(동작한다), `ParticleState` "버전 업그레이드 노트"(정보).

## 4. 에디터에서 보기(맵에 저장하지 않는다)

1. `SceneTools.add_to_scene_from_asset`로 `/Game/Lab/FX/NS_LabCrowd`를 원점에 놓는다. 활성화 때 워밍업 268초가 돌아 200m\~1,000m가 바로 찬다.
2. 에디터가 앞 창이 아니면 뷰포트가 갱신되지 않아 입자가 흐르지 않는다. `ObjectTools.set_properties`로 `/Script/UnrealEd.Default__EditorPerformanceSettings`의 `bThrottleCPUWhenNotForeground`를 false(메모리에서만)로 두고, 레벨 뷰포트 실시간을 켠다. 실시간 켜기는 MCP에 없어 SlateInspector로 에디터 하단 콘솔 입력란(Cmd)을 클릭하고 `py unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_set_viewport_realtime(True)`를 넣었다.
3. 스크린샷은 `EditorAppToolset.CaptureViewport`(카메라 변환 지정, UI 없음)로 찍었다. 결과가 커서 파일로 저장되고, base64 PNG를 풀어 jpg 품질 90으로 바꿔 `Measurements/Q2/shots/`에 넣었다. 입자를 숨긴 비교 화면은 Python `actor.set_is_temporarily_hidden_in_editor(True)`로 만들었다.
4. 끝나면 `SceneTools.remove_from_scene`로 지우고 `bThrottleCPUWhenNotForeground`를 true로 되돌린다. 맵(`L_Bench`)은 저장하지 않는다.
