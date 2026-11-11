# NS_LabCrowd 만들기

Q2 Niagara 배경 무리 `/Game/Lab/FX/NS_LabCrowd`와 머티리얼 인스턴스 `MI_LabCrowdSmall`·`MI_LabCrowdLarge`는 엔진 Niagara 템플릿과 굽기 산출물로 만든 에셋이라 커밋한다(Fab 콘텐츠가 아니다). 부모 머티리얼과 메시는 `Scripts/bake-vat.py`의 산출물(`/Game/Lab/VAT/Baked/`, gitignore)이므로 새 체크아웃에서는 먼저 `bake-vat.py`를 소형·대형 둘 다 돌린다. 설계와 값의 근거는 [Docs/Worklog/Q2.md](../Docs/Worklog/Q2.md).

다시 만들어야 하면 Unreal MCP로 아래 순서를 밟는다(2026-11-12에 이 순서로 만들었다). 에디터에서 손으로 해도 같다. 툴세트 이름은 `NiagaraToolsets.NiagaraToolset_System`(아래 "NS"), `editor_toolset.toolsets.material_instance.MaterialInstanceTools`(아래 "MIT")다. 여러 호출은 `editor_toolset.toolsets.programmatic.ProgrammaticToolset.execute_tool_script`로 묶으면 왕복이 준다.

## 1. 굽기 머티리얼에 Niagara 사용 플래그

`bake-vat.py`의 `make_material`이 `MATUSAGE_NIAGARA_MESH_PARTICLES`를 켜고 켜졌는지 확인한다(Q2 2단위). 그래서 굽기를 다시 돌려도 `/Game/Lab/VAT/Baked/M_LabMinionVAT_Bone`·`M_LabMinionLargeVAT_Bone`에 `bUsedWithNiagaraMeshParticles`가 남는다. 따로 할 일은 없다.

그 전에 구운 머티리얼이라 플래그가 없으면 `ObjectTools.set_properties`로 `bUsedWithNiagaraMeshParticles` = true를 넣고 `MaterialTools.recompile` 뒤 `AssetTools.save_assets`로 저장한다. 플래그가 없으면 쿡된 빌드에서 입자가 기본 머티리얼로 그려진다.

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
   5. `AddModule`: EmitterUpdateScript에 `/Niagara/Modules/Emitter/SpawnRate`, ParticleSpawnScript에 `/Niagara/Modules/Spawn/Location/V2/ShapeLocation`, `/Niagara/Modules/Spawn/Velocity/AddVelocityFromPoint`, `/Niagara/Modules/Update/Material/DynamicMaterialParameters`, ParticleUpdateScript에 `/Niagara/Modules/Solvers/SolveForcesAndVelocity`. (MCP의 `AddModule`이 결과를 만들고 나서 `_StrictDict.get()` 오류를 낼 수 있다. 모듈은 더해져 있다.)
   6. `SetStackInputData`:
      - `SpawnRate.SpawnRate` = 연결 `User.SmallRate`(`User.LargeRate`).
      - `InitializeParticle.Lifetime` = 연결 `User.Lifetime`.
      - `ShapeLocation.Shape Primitive` = `ENiagara_LocationShapes::NewEnumerator4`(Ring / Disc), `Ring Radius` = 연결 `User.OuterRadius`, `Disc Coverage` = 0.
      - `AddVelocityFromPoint.Velocity Strength` = HLSL 식 `-User.Speed`(원점을 향해 걷는다).
      - `DynamicMaterialParameters`: `Write Parameter Index 0` = `ENiagara_DynamicMaterialParametersInputMode::NewEnumerator0`(Off), `Write Parameter Index 1` = `NewEnumerator1`(Float). 머티리얼이 Dynamic Parameter 1번을 읽으므로 1번만 쓴다. 쓰기 플래그 `Module.Index1Float0Write`\~`3Write`는 기본값 true 그대로. `Index 1 Param 1` = 다이내믹 입력 `/Niagara/DynamicInputs/UniformRange/UniformRangedFloat`, 입력 스택 `["Index 1 Param 1", "Minimum"]` 0, `["Index 1 Param 1", "Maximum"]` 62/30(41/30) = 걷기 위상. `Index 1 Param 2` 1(Playrate), `Index 1 Param 3` 0(StartFrame), `Index 1 Param 4` 61(40)(EndFrame).
      - 1단위(2026-11-12)에는 같은 값을 폐기된 `DynamicMatParams1`(`Param0Value`\~`Param3Value`)로 넣었고, 2단위에서 `RemoveModule`로 지우고 위 모듈로 바꿨다.
6. `GetSystemCompileState`가 오류 없음, `GetStackIssues`가 경고만(아래)인지 보고 `AssetTools.save_assets`로 `NS_LabCrowd`, `MI_LabCrowdSmall`, `MI_LabCrowdLarge`를 저장한다.

남는 이슈: `ParticleState` "버전 업그레이드 노트"(정보) 둘. 경고는 없다.

### 걷기 클립 범위 확인(값을 바꿨을 때)

소형 이미터의 `Index 1 Param 2`\~`4`만 잠시 바꿔 같은 시점(오브에서 195m, 높이 4m, 바깥을 봄)에서 찍고 되돌린다. Playrate 0 + StartFrame 120(죽기 클립 안)이면 소형이 모두 같은 쓰러진 자세로 멈춰야 하고, Playrate 1 + StartFrame 0 + EndFrame 2면 모두 거의 같은 선 자세여야 한다. 둘 다 맞으면 네 값이 제자리로 들어간다. 2단위의 결과는 `Measurements/Q2/shots/clip-start120-rate0.jpg`, `clip-end2.jpg`.

## 4. 에디터에서 보기(맵에 저장하지 않는다)

MCP에 Python 실행 툴이 없어, 스크립트를 파일로 쓰고 SlateInspector로 에디터 하단 콘솔 입력란(Cmd)에 `py <파일 경로>`를 넣어 돌렸다(`SlateInspectorToolset.Observe` → `Snapshot`으로 입력란 ref를 찾고 `Type`, `submit` true). 결과는 `unreal.log_warning`으로 남기고 `LogsToolset.GetLogEntries`로 읽는다.

1. `SceneTools.add_to_scene_from_asset`(또는 Python `EditorActorSubsystem.spawn_actor_from_object`)로 `/Game/Lab/FX/NS_LabCrowd`를 원점에 놓는다. 활성화 때 워밍업 268초가 돌아 200m\~1,000m가 바로 찬다.
2. 에디터가 앞 창이 아니면 뷰포트가 갱신되지 않아 입자가 흐르지 않는다. `ObjectTools.set_properties`로 `/Script/UnrealEd.Default__EditorPerformanceSettings`의 `bThrottleCPUWhenNotForeground`를 false(메모리에서만)로 두고, 레벨 뷰포트 실시간을 켠다. 실시간 켜기는 MCP에 없어 SlateInspector로 에디터 하단 콘솔 입력란(Cmd)을 클릭하고 `py unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_set_viewport_realtime(True)`를 넣었다.
3. 스크린샷은 `EditorAppToolset.CaptureViewport`(카메라 변환 지정, UI 없음, `annotations`는 모든 필드를 0으로 넘겨야 호출된다)로 찍었다. 라이트·플레이어 스타트 아이콘이 찍히면 Python `LevelEditorSubsystem.editor_set_game_view(True)`로 게임 뷰를 켠다(끝나면 끈다). 결과가 커서 파일로 저장되고, base64 PNG를 풀어 jpg 품질 90으로 바꿔 `Measurements/Q2/shots/`에 넣었다. 입자를 숨긴 비교 화면은 Python `actor.set_is_temporarily_hidden_in_editor(True)`로 만들었다.
4. 끝나면 `bThrottleCPUWhenNotForeground`를 true로 되돌리고, 임시 액터를 둔 맵은 저장하지 않고 버린다. `SceneTools.remove_from_scene`로 지워도 맵이 "수정됨"으로 남으므로, Python `EditorLoadingAndSavingUtils.new_blank_map(False)`(저장 확인 없음) 뒤 `load_map("/Game/Lab/Maps/L_Bench")`로 디스크에서 다시 연다. `get_dirty_map_packages()`가 빈 목록인지 본다.

바닥: `L_Bench`의 바닥은 반쪽 길이 1,020m(입자 바깥 반경 1,000m + 20m)다(`Scripts/create-bench-map.py`, Q2 2단위). 1단위 스크린샷은 바닥이 520m일 때 찍었다.

## 5. 엔티티 구동 이미터(Q2b)

그려지지 않는 엔티티(표현 None)의 위치를 코드(`ULabCrowdFxGatherProcessor`)가 매 프레임 배열로 넣고, 이미터는 그 배열 길이만큼 입자를 그 프레임에만 그린다. 위 3절의 `Small`·`Large`는 500m 밖 독립 이미터로 그대로 둔다(안쪽 반경은 코드가 `User.Lifetime`으로 정한다). 계약은 [Docs/Worklog/Q2.md](../Docs/Worklog/Q2.md) "Q2b"의 "에셋 단위에 넘길 것", 경위는 같은 절의 "에셋 단위 진행". 2026-11-12에 이 순서로 만들었다.

1. 사용자 변수. `AddUserVariables`로 `LabNow`(float 0), `LabOrbPos`(`/Script/Niagara.NiagaraPosition`, 0). `Vector`(`/Script/CoreUObject.Vector`)로 넣으면 다른 Vec3(`Vector3f`)와 형이 다르니 Position을 쓴다. 코드의 `SetVariableVec3`가 Position 변수면 Position으로 넣는다. 배열 변수 `LabSmallRecs`·`LabLargeRecs`(Vector 4 Array)는 `AddUserVariables`로 만들 수 없다(오류 없이 아무것도 생기지 않는다. 엔진 `FNiagaraParameterStore::SetParameter_InternalUseOnly`가 DI를 받지 않는다). 아래 5-4에서 다이내믹 입력의 배열 입력을 `User.LabSmallRecs`에 `Linked`로 연결하면 그 이름의 사용자 변수가 DI 인스턴스와 함께 생긴다. `Lifetime` 기본값은 166.6667(= (100,000 − 50,000) / 300)로 바꿨다(코드가 실행 때 넣는다).
2. 시스템 `SetSystemData`: `bOverrideCastShadow` false(켜 두면 컴포넌트가 시스템 값으로 그림자를 정해 이미터별로 가를 수 없다). 나머지는 3절 그대로다(워밍업 268초는 독립 이미터의 새 수명 166.67초를 덮는다).
3. 이미터마다(이름 `SmallEnt`, `LargeEnt`) 아래를 한다. 괄호 안이 대형 값이다.
   1. `AddEmitter`: 템플릿 `/Niagara/DefaultAssets/Templates/Emitters/Minimal`.
   2. `SetEmitterData`: `SimTarget` GPUComputeSim, `bLocalSpace` false(월드 공간, 원소가 월드 좌표다), `CalculateBoundsMode` Fixed, `FixedBounds` 3절과 같음, `bDeterminism` true, `RandomSeed` 3(4), `AllocationMode` ManualEstimate, `PreAllocationCount` 12000(1500)(1만에서 원소 약 5,600·620, 2만 기준), **`InterpolatedSpawnMode` NoInterpolation**. 보간 스폰을 끄면 새 입자는 첫 프레임에 업데이트를 돌지 않는다. 그래서 수명 0.001초 입자가 스폰된 프레임에 그려지고 다음 프레임의 `ParticleState`에서 죽는다.
   3. 스프라이트 렌더러를 `RemoveRenderer`(인덱스 0)로 지우고 메시 렌더러를 `AddRenderer`. `SetRendererData`: 메시·회전·스케일·머티리얼은 3절과 같음(`SM_LabMinionVAT_Bone` + `MI_LabCrowdSmall`, 대형 `SM_LabMinionLargeVAT_Bone` 스케일 2 + `MI_LabCrowdLarge`, yaw −90), `FacingMode` Velocity, **`bCastShadows` true**, **`bEnableCameraDistanceCulling` true, `MinCameraDistance` 17500, `MaxCameraDistance` 200000**, **`MotionVectorSetting` Approximate**(이 입자는 업데이트를 돌지 않아 `Particles.Previous.Position`이 채워지지 않는다. Approximate는 이전 위치를 위치 − 속도 × 델타로 만든다).
   4. 이미터 업데이트: `AddModule` `/Niagara/Modules/Emitter/SpawnPerFrame`. `SetStackInputData` `["Spawn Count"]` = 다이내믹 입력 `/Niagara/DynamicInputs/Math/GetVector4ArrayCount`, `["Spawn Count", "Vector 4 Array"]` = `Linked` `User.LabSmallRecs`(`User.LabLargeRecs`), 형 `/Script/Niagara.NiagaraDataInterfaceArrayFloat4`.
   5. 입자 스폰(실행 순서대로):
      - `InitializeParticle.Lifetime` = 0.001.
      - `AddSetParametersModule`: `Particles.LabRec`(`/Script/CoreUObject.Vector4f`). `["Particles.LabRec"]` = 다이내믹 입력 `/Niagara/DynamicInputs/Arrays/SelectVector4_FromArray`, 그 아래 `Array Sampling Mode` = `ENiagaraArraySamplingMode` NewEnumerator1(Direct Set), `Direct Array Mode` = `ENiagara_WrapClamp` NewEnumerator0(Clamp), `Direct Array Index` = 다이내믹 입력 `/Niagara/DynamicInputs/Execution/ReturnExecIndex`(GPU 스폰에서 이번 프레임 스폰 순번), `Vector4 Selection Array` = `Linked` `User.LabSmallRecs`(`User.LabLargeRecs`).
      - `AddSetParametersModule`: `Particles.Position`(`/Script/Niagara.NiagaraPosition`)과 `Particles.Velocity`(`/Script/CoreUObject.Vector3f`). 값은 HLSL 식(rvalue 하나):
        - Position: `float3(Particles.LabRec.xy + normalize(User.LabOrbPos.xy - Particles.LabRec.xy) * User.Speed * max(User.LabNow - Particles.LabRec.z, 0.0), 0.0)`
        - Velocity: `float3(normalize(User.LabOrbPos.xy - Particles.LabRec.xy) * User.Speed, 0.0)`
      - `AddModule` `/Niagara/Modules/Update/Material/DynamicMaterialParameters`: `Write Parameter Index 0` = `/Niagara/Enums/Utility/ENiagara_DynamicMaterialParametersInputMode` NewEnumerator0(Off), `Write Parameter Index 1` = NewEnumerator1(Float), `Index 1 Param 1` = HLSL 식 `Particles.LabRec.w`(TimeOffset = 걷기 위상 W), `Index 1 Param 2` 1, `Index 1 Param 3` 0, `Index 1 Param 4` 61(40).
   6. 입자 업데이트는 템플릿의 `ParticleState` 그대로다(수명이 지나면 죽인다).
4. `GetSystemCompileState` 오류·경고 0, `GetStackIssues` 정보만(`SpawnPerFrame` 실험단계 표시, 보간 스폰 끔 안내, `ParticleState` 버전 노트)인지 보고 `AssetTools.save_assets`로 저장한다.

MCP로 만들 때 남는 것: MCP `SetEmitterData`는 `PostEditChange`를 부르지 않아 `InterpolatedSpawnMode`를 바꿔도 스폰 스크립트의 용도(템플릿의 `ParticleSpawnScriptInterpolated`)가 바뀌지 않는다. 그래서 로드할 때마다 `LogNiagara: Disabling interpolated spawn because emitter flag and script type don't match` 경고가 이미터마다 한 줄 나오고 엔진이 메모리에서 용도를 고친다(`NiagaraEmitter.cpp:861-878`). GPU 계산 스크립트는 이미터 값(NoInterpolation)으로 컴파일되므로 동작은 같다(`NiagaraScript.cpp:1187-1193`). 경고를 없애려면 에디터에서 두 이미터의 Interpolated Spawn Mode를 UI로 한 번 바꿨다가 NoInterpolation으로 되돌리고 저장한다. 에디터에서 손으로 만들면 UI가 용도를 바꾸므로 이 문제가 없다.

확인: 배열은 실행 때 코드가 넣으므로 에디터 뷰포트에는 엔티티 구동 입자가 없다. `run-dev.ps1 -N 10000 -Preset P1 -Duration 25 -ShotAt 20 -Stage M5 -Variant q4final`의 로그에 `lab_crowdfx ... small_recs=1 large_recs=1 now=1 orb_pos=1`이 있고 `lab_crowdfx_param missing=`이 없어야 한다. 독립 이미터를 잠시 끄고(`SetEmitterData` `bIsEnabled` false, 확인 뒤 되돌려 저장) 같은 실행을 하면 500m 밖이 비고 188\~500m가 엔티티 구동 입자로 찬다.
