# NS_LabBolt 만들기

저격 궤적 `/Game/Lab/FX/NS_LabBolt`는 엔진 Niagara 템플릿으로 만든 에셋이라 커밋한다(Fab 콘텐츠가 아니다). 다시 만들어야 하면 Unreal MCP(`NiagaraToolsets.NiagaraToolset_System`)로 아래 순서를 밟는다. 에디터에서 손으로 해도 같다.

1. `CreateNiagaraSystem`으로 `/Game/Lab/FX/NS_LabBolt`를 만들고, 딸려 온 이미터를 `RemoveEmitter`로 지운다.
2. `AddEmitter`로 엔진 템플릿 DynamicBeam 이미터를 더하고 이름을 `Bolt`로 둔다.
3. `AddUserVariables`로 `BeamEnd`(Vector3f), `BoltColor`(LinearColor), `BoltWidth`(float)를 더한다.
4. `SetStackInputData`로 연결한다.
   - EmitterUpdateScript `BeamEmitterSetup`: `Beam End` = `User.BeamEnd`, `Absolute Beam End` = true. 시작점은 시스템 위치(구체)다.
   - EmitterUpdateScript `SpawnBurst_Instantaneous`: `Spawn Count` = 2(곧은 선 하나).
   - ParticleSpawnScript `InitializeParticle`: `Lifetime` = 0.3. `BeamWidth`: `Beam Width` = `User.BoltWidth`.
   - ParticleUpdateScript `Color`: `Color` = `User.BoltColor`.
5. `editor_toolset.toolsets.asset.AssetTools`의 `save_assets`로 저장한다.

ParticleUpdateScript에 `JitterPosition`(Jitter Amount 0)이 남아 있다. 번개 표현을 시험하던 흔적이고 꺼진 상태다. 굵기와 색은 `ALabOrb`(`BoltWidth`, `BoltColor`)가 실행 시 정한다.
