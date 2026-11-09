# M3b 작업 기록

## 측정 방법 구현 (측정 세션, 2026-11-09)

plan.md "M3b" 측정 방법 1\~7번을 `Scripts/`에 구현했다. 빌드·패키징·벤치마크 실행은 하지 않았다.

### 만든 것

- `summarize.py`: `frame_p95_ms`(runs·summary, `_min`/`_max` 포함), `bottleneck`(Game·Render·GPU p50 중 가장 긴 것의 이름, 프레임 제외, 2026-11-09 코디네이터 결정 9205a80), `build`(CSV 메타데이터 `labbuild`·`labcvars`·`labdriver`를 `키=값;`으로 이음, 없으면 빈칸). `--rebuild`는 기존 행도 원본 CSV로 다시 계산한다. 태그 `aa*` 라벨은 summary에서 `<변형>-aa` 조건으로 따로, `trace*`는 summary에서 뺀다.
- `plot.py`: `n-at-60fps.svg`는 프레임 p95 기준 N, 막대 옆에 p50 N과 병목. `n-scaling-P1.svg`는 p95 실선 + p50 점선과 16.7ms 선. `Docs/figures/n-at-60fps.csv`에 값(p95 N, p50 N, 병목). 30k 점은 summary에 있으면 자연히 들어간다. `-aa` 조건은 곡선에 잇지 않는다.
- `spikes.py`: 사용법은 commands.md. 사건 = 중앙값 2배 초과 프레임(붙은 것은 하나로 묶음), 시각 = 프레임 시간 누적합. 주기 판정 = 사건 5개 이상이고 간격이 간격 중앙값의 ±10% 안인 비율 60% 이상. 임계값(5개, 60%)은 측정 세션이 정했다.
- `run-bench.ps1`: `-RunIndex`, `-Trace`(`Saved/Traces/<라벨>.utrace`), `-ExtraArgs`, 변형 이름 `m3vis0 m3vis15 m3hys20 m0full m0static staticpawn m1mass m2mass m3mass`를 ValidateSet에 추가. 게임이 이 이름을 모르면 실행이 실패한다(코드 세션 몫).
- `run-m3b.ps1`(초안): 위상 e3b(기준점 m3mass 포함 24회 섞기, `-NoBase`면 18회) → matrix(M0 24 + staticpawn 6 + M1\~M3 90) → aa 3 → trace 2. 143회 + 버리는 r0 1 = 144회, 약 204분(85초/회). `-DryRun`으로 목록 확인(동작 확인함). 계획서의 "약 170회"는 budget·uro·pawn 같은 P1 전용 변형까지 센 값으로 보이고 이 러너에는 넣지 않았다.
- `Docs/Reference/pc-specs.md`: CPU·RAM·GPU·드라이버·OS·엔진 커밋을 채웠다. commands.md에 행을 더했다.

### M0\~M3 재요약

`summarize.py --milestone M<N> --rebuild`를 M0\~M3에 돌렸다. 재요약 전후 `runs.csv`·`summary.csv`의 기존 열을 모든 행에서 비교했다(행 수 89/63/71/118, 29/20/23/38 조건): **차이 0, 행 수 같음.** 새 열만 더해졌다(`frame_p95_ms`, summary는 `_min`/`_max`도, `bottleneck`, `build`). `build`는 M0\~M3 CSV에 `lab*` 메타데이터가 없어 비었다. `bottleneck`은 처음 정의(프레임 포함)에서는 항상 `frame`이라, 프레임을 빼는 정의로 바꿔 다시 요약했다(기존 열 차이 0 재확인). M0은 `game`, M1\~M3은 `gpu`다.

### N@16.7ms (P1, 로그 보간)

| 단계 변형 | p95 N | p50 N(이전 정의) |
| --- | --- | --- |
| M0 full | 667 | 813 |
| M0 static | 1,044 | 1,196 |
| M1 mass | 15,498 | 20,577 |
| M2 mass | 16,460 | 20,518 |
| M3 mass | 18,475 | 22,046 |
| M3 m2off(M2 + 200m 밖 Off) | 30,812 | 35,763 |

p50 열은 STATUS.md의 기존 수치(813 / 1,196 / 20,577 / 20,518 / 22,046 / 35,763)와 같다. M3 하위 실험(`m3*`)은 20k까지만 재서 20k에서 p95가 16.7ms 아래인 변형은 "범위 밖(20,000 이상)"으로 나온다. 전체는 `Docs/figures/n-at-60fps.csv`.

### 스파이크 검사 (`py -3 -I Scripts/spikes.py <접두사>`)

| 묶음 | 실행 수(r0 제외) | 주기적 실행 | 주기적 실행의 간격 중앙값 |
| --- | --- | --- | --- |
| M0 | 87 | 38 (44%) | 7.00초 (6.88\~13.99) |
| M1 (2026-11-08) | 60 | 16 (27%) | 7.00초 (6.98\~7.00) |
| M2 (2026-11-08) | 69 | 41 (59%) | 6.99초 (6.89\~7.01) |
| M3 (2026-11-09) | 114 | **0 (0%)**, 실행당 사건 중앙값 0 | |
| diag-* (11-09 진단) | 25 | 0 | |

**M1·M2 묶음에서 7.0초 주기가 뚜렷하고 M3 묶음에는 없다.** 규약의 기대와 일치한다. 해석상 주의:
- 판정이 엄격해서(±10%) 7초 사건에 다른 스파이크가 섞인 실행(M1 mass 1k P2 r3 6.38초, 2k P1 r3 5.51초 등)은 주기적이 아닌 것으로 센다. 27%\~59%는 하한이다. M1 mass 1k\~20k 실행은 거의 모두 간격 중앙값이 6.9\~7.0초였다.
- N=50k처럼 프레임이 느린 실행은 사건이 0\~2개라 주기를 판정할 수 없다(5개 미만).
- M0에도 7.00초 주기가 있다. 11-08 묶음만의 현상이 아니라 M3 묶음 전의 측정 전반에 있었다.
- `massav`(M1 5k)는 간격 약 2.0초(26\~30건)다. 저격 간격 2초와 같아 환경 잡음이 아니라 시나리오 효과(저격 사망 처리)로 보인다. 7초 주기와 구별된다.

### 질문과 답 (코디네이터/코드 세션)

1. `bottleneck`: 프레임을 빼고 Game/Render/GPU 중 최댓값으로 바꿨다(답: 코디네이터, d761020).
2. E3b 기준점: `m3mass`를 같은 섞기에 기본으로 넣는다(24회, `-NoBase`로 18회). 답: 코디네이터, d761020.
3. 변형 이름: 코드 세션이 별칭을 넣었다(`m0full`=full, `m0static`=static, `m1mass`·`m2mass`·`m3mass`=mass, `staticpawn`). `-LabStage`는 러너의 `-Stage`로 준다. 맞음.
4. 직렬 트레이스: 인자 `-LabFullyParallel=0`이 구현됐다. `run-m3b.ps1 -SerialArgs` 기본값과 같다.
5. 메타데이터 키: `LabBuild`·`LabCVars`·`LabDriver` 그대로. `summarize.py`의 `BUILD_KEYS`와 같다. `LabCVars` 값에 `|`가 있어 여러 빌드가 섞인 조건의 `build`는 ` // `로 잇는다. 패키지를 만든 뒤 실제 CSV로 한 번 확인한다.

### 선행 묶음 (측정 세션, 2026-11-09)

E3b (8) 표현 LOD 값(기준점 포함 24회 + 버리는 r0 1회)과 코드 정비 A/B(9회)를 한 패키지로 연달아 쟀다. 해석은 쓰지 않는다. 대표값 결정은 코디네이터 몫.

**실행 경위.** 패키지: `package.ps1 -Config Test` 두 번째 시도에서 성공(1분 29초). 첫 시도는 11초 만에 실패했다: UBT가 `HordeOptLab.Build.cs`를 여러 타깃(Editor, UnrealPak, ShaderCompileWorker, 게임)에서 동시에 평가하며 `Intermediate/Build/LabGenerated/LabBuildVersion.h`를 동시에 쓰다 `IOException: file used by another process`(`HordeOptLab.Build.cs:96`, `File.WriteAllText`). 헤더가 이미 현재 커밋 값이어서 쓰기가 생략된 재시도는 통과했다. **커밋이 바뀐 직후 첫 패키징에서는 다시 날 수 있다**(코드 세션이 고칠 것: 쓰기 재시도 또는 임시 파일 + 교체). `commit.txt` = `58674cea25d130c62a3d72e831451a6e2c669506 (dirty)`(dirty는 추적 안 된 `Config/DefaultEditor.ini`, 그리고 코드 세션이 고치는 중이던 `Build.cs`·이 Worklog). `build` 열 = `labbuild=58674cea25d1`(CSV 메타데이터 `labbuild`·`labcvars`·`labdriver`가 모두 채워짐, 드라이버 `NVIDIA GeForce RTX 4090 621.14`, `mass.FullyParallel=true`, `LockObserversDuringFlushing=true`). 실행: `run-m3b.ps1 -Phase e3b`(25회 33분 31초, 실패 0) → `-Phase ab -NoR0`(9회 12분 3초, 실패 0). 라벨 `M3b-<변형>-N<N>-P1-r<i>`, ab는 태그 `ab`(`M3b-<변형>-N20000-P1ab-r<i>`, summary 조건 `<변형>-ab`). 스파이크 검사 `spikes.py M3b-`: 실행 33개 중 주기적 0(실행당 사건 중앙값 0). 원본 CSV는 `Saved/Profiling/CSV/M3b-*.csv`, 요약은 `Measurements/M3b/`.

**표 1. E3b (8), P1, 3회 중앙값(최소\~최대), ms.** 액터 수 = CSV `Lab/Actors` p50(액터 총수, "화면 안 액터 수" 스탯은 없다). 전환/초 = `Lab/ActorRebinds` 합 ÷ 측정 시간.

| 변형 | N | 프레임 p50 | 프레임 p95 | Game p50 | Render p50 | GPU p50 | 액터 수 | 전환/초 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| m3mass(기준점) | 20k | 18.04 (17.78\~18.43) | 20.12 (19.68\~20.31) | 16.84 (16.61\~17.23) | 8.11 (7.92\~8.24) | 9.46 (9.41\~9.51) | 493 | 81.0 (80.5\~81.3) |
| m3vis0 | 20k | 16.74 (16.66\~19.21) | 19.80 (18.26\~21.06) | 15.74 (15.68\~18.04) | 7.16 (7.03\~10.37) | 9.13 (9.11\~9.22) | 482 (482\~483) | 103.3 (103.1\~103.6) |
| m3vis15 | 20k | 18.41 (18.26\~18.97) | 20.08 (20.00\~20.43) | 17.28 (17.16\~17.82) | 7.87 (7.81\~8.14) | 9.25 (9.24\~9.26) | 491 | 135.1 (134.7\~135.5) |
| m3hys20 | 20k | 18.15 (17.66\~18.30) | 19.50 (19.36\~19.89) | 16.97 (16.51\~17.11) | 7.83 (7.78\~7.90) | 9.26 (9.25\~9.29) | 493 | 81.2 (81.0\~81.7) |
| m3mass(기준점) | 5k | 7.30 (7.29\~8.23) | 8.45 (8.36\~9.31) | 6.74 (6.73\~7.62) | 3.95 (3.91\~4.13) | 5.84 (5.84\~5.85) | 262 | 28.7 (28.7\~28.7) |
| m3vis0 | 5k | 6.38 (6.38\~6.40) | 7.04 (6.97\~7.21) | 4.99 (4.91\~5.21) | 3.92 (3.85\~4.07) | 5.96 (5.93\~5.97) | 114 | 24.3 (24.3\~24.3) |
| m3vis15 | 5k | 6.55 (6.34\~6.76) | 7.75 (7.28\~7.85) | 6.04 (5.69\~6.26) | 3.90 (3.78\~3.99) | 5.89 (5.88\~5.91) | 172 | 29.6 (29.6\~29.6) |
| m3hys20 | 5k | 7.72 (7.33\~7.86) | 8.61 (8.40\~9.09) | 7.16 (6.77\~7.29) | 3.90 (3.85\~3.93) | 5.79 (5.78\~5.79) | 262 (262\~263) | 28.7 (28.7\~28.7) |

**표 2. 코드 정비 A/B, 20k P1, 3회, ms.** 정비된 코드의 `m3mass`(ab 묶음 안에서 다시 잼) vs `m3qold`(쿼리 정비 전 방식) vs `m3syncall`(ActorSync 필터 없음).

| 변형 | 프레임 p50 | 프레임 p95 | Game p50 | Render p50 | GPU p50 | 액터 수 | 전환/초 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| m3mass | 17.77 (16.25\~18.47) | 19.25 (17.71\~19.89) | 16.61 (15.19\~17.27) | 7.88 (7.08\~7.97) | 9.45 (9.42\~9.48) | 493 | 81.1 (80.8\~82.3) |
| m3qold | 17.01 (16.83\~17.73) | 18.95 (18.85\~19.35) | 15.83 (15.67\~16.53) | 8.03 (8.00\~8.07) | 9.60 (9.54\~9.62) | 495 | 87.1 (86.9\~87.5) |
| m3syncall | 18.04 (17.99\~18.72) | 20.02 (19.51\~20.11) | 16.87 (16.84\~17.51) | 8.03 (7.97\~8.14) | 9.46 (9.45\~9.49) | 495 | 86.0 (85.7\~86.1) |

**잡음 기준.** A/A 묶음(`-Phase aa`)은 아직 안 잼. 지금은 세 실행의 범위만 있다. 같은 조건 `m3mass` 20k P1이 한 패키지의 두 묶음(e3b 라운드, ab 라운드)에서 프레임 p50 18.04(17.78\~18.43)와 17.77(16.25\~18.47)로 나왔다. 참고 수치: 정비 전 코드의 M3 `m3base` 20k P1은 프레임 p50 15.46, p95 17.26(`Measurements/M3/summary.csv`, 11-09 이전 묶음, 비교는 "참고").

**메모.** `m3vis0` 20k의 프레임·Render 범위는 세 실행 중 한 실행이 큰 쪽이다(최대 프레임 p50 19.21, Render 10.37). 액터 수는 거의 상한(High 500) 근처에서 일정하고, `m3vis0`·`m3vis15`의 5k는 114·172로 다르다.

## 코드 정비 (코드 세션, 2026-11-09)

plan.md "M3b" 코드 정비 핵심 5 + 두 줄 + 기본 프로세서 점검. 대상 소스는 `Source/HordeOptLab/LabMassProcessors.cpp`(Approach, Contact, Snipe, State, DamageGen, DamageApply), `LabVatProcessor.cpp`, `LabActorSyncProcessor.cpp`, `LabCrowdSubsystem.cpp`, `LabOrb.cpp`(커밋 915773c 기준). 설계는 사용자와 하나씩 정했다(아래 "결정").

### 프로세서별 읽기·쓰기 표(정비 전)

표기: **W** 쓰기(ReadWrite 선언), **R** 읽기(ReadOnly 선언), **tag** 아키타입 태그 판정(`DoesArchetypeHaveTag`, 선언 없음), **defer** 지연 명령으로 태그 변경, **밖** 쿼리 밖 접근(`GetFragmentDataChecked`·`GetFragmentDataPtr`). 엔진 프래그먼트는 `FTransformFragment` = Xf, `FMassMoveTargetFragment` = MoveTgt, `FMassRepresentationFragment` = Rep, `FMassRepresentationLODFragment` = RepLOD, `FMassActorFragment` = Actor.

| 프로세서(실행 순서 선언) | Xf | MoveTgt | FLabHealth | FLabMinion(Serial, StateTimeLeft) | Rep | RepLOD | Actor | 태그 Hit / Dead / CodeDrivenMovement | 쿼리 밖 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Approach(그룹 Tasks) | R | W | | | | | | Hit·Dead 없음 필터 | |
| OrbContact(after Movement) | R | | W | W(StateTimeLeft) | | | | 없음 필터, defer +Dead −Code | 없음. 공유 상태 `Crowd->Nearest`·`KillCount`는 잠금 |
| OrbSnipe(after Contact) | R | | R | W 선언, **W는 쿼리 밖** | | | | 없음 필터, defer +Hit −Code | `GetFragmentDataChecked<FLabMinionFragment>(Pick.Target).StateTimeLeft = HitDuration`(`LabMassProcessors.cpp:234`). `Crowd->PendingSnipeTick` 읽고 0으로 씀(오브 틱이 씀, 잠금 없음) |
| MinionState(after Snipe) | 밖 R | | | W | | | | Hit·Dead 중 하나 필터, defer −Hit +Dead, DestroyEntities | 개발 빌드 진단 `GetFragmentDataPtr<FTransformFragment>`(`:286`). `StateTimeLeft = DeathDuration`은 즉시 쓰고 태그는 지연 → 그 프레임에 Hit 태그 + Dead 시간(plan.md 3번) |
| DamageGen(after State, E1 (d)·kill 변형만) | | | W | W(StateTimeLeft) | | | | 없음 필터, defer +Hit −Code | 이벤트 엔티티 생성은 지연 명령 안(`FMassDeferredCreateCommand`) |
| DamageApply(after DamageGen) | | | W 선언(HealthQuery, 실제 접근은 밖) | | | | | | `GetFragmentDataPtr<FLabHealthFragment>(Event.Target)`(`:461`). HealthQuery는 솔버에 쓰기를 알리려고 둔 것 |
| VatISM(엔진 `UMassUpdateISMProcessor` 상속, after Representation, 게임 스레드) | R | | | **R(StateTimeLeft, Serial)** | W | R | | **tag** Hit·Dead | 없음. 공유 상태 `Crowd->VatClips`·`SnipeView`·`HitDuration`·`DeathDuration` 읽음 |
| ActorSync(after Representation, 게임 스레드) | R | | | **R(StateTimeLeft, Serial)** | R | R | W | **tag** Hit·Dead. 액터 유무 필터 없음 → 전 엔티티 순회(plan.md 4번) | 없음. `ALabMinionMassActor`·AnimInstance를 직접 씀 |
| ULabCrowdSubsystem::SpawnEntities(게임 스레드, 생성 직후) | 밖 W | | 밖 W | 밖 W(Serial) | | | | | `GetFragmentDataChecked` 셋(`LabCrowdSubsystem.cpp:138-140`). 생성 컨텍스트 안의 초기화라 처리 중 접근은 아니다. 그대로 둔다 |
| ALabOrb::TickMass(액터 틱) | | | | | | | | | 프래그먼트 없음. `OrbCenter`·`SnipeView`·`PendingSnipeTick` 씀, `SnipeTargets`·`Nearest`는 잠금으로 읽음 |
| 엔진: Steering·Movement | W(Movement) | R | | | | | | Code 필터 | |
| 엔진: Representation·VisualizationLOD·LODCollector | R | | | | W | W(VisualizationLOD) | W | | 액터 붙이기·떼기는 `UMassRepresentationActorManagement` 콜백 |

표가 보여 주는 결함(plan.md 코드 정비 1\~4와 같다): (1) 표현 둘(VatISM, ActorSync)이 로직 프래그먼트 `FLabMinionFragment`를 읽는다. 쓰는 쪽이 넷(Contact, Snipe, State, DamageGen)이라 솔버가 표현을 그 넷과 묶고, 병렬 실행에서 게임 스레드가 자원 대기로 돈다(M3 diag-vt*, engine-notes.md 11절). (2) Snipe가 쿼리 밖에서 `FLabMinionFragment`를 쓴다. (3) State가 시간은 즉시, 태그는 지연으로 바꿔 한 프레임 동안 Hit 태그에 Dead 시간이 들어 있다. VatISM이 그 프레임에 돌면(순서 선언 없음) Hit 클립 시각이 음수가 되어 0으로 잘린다. (4) VatISM·ActorSync와 로직 사이에 실행 순서 선언이 없다. (5) ActorSync는 액터가 붙은 엔티티(20k P1에서 약 500)를 찾으려고 20k를 모두 순회한다.

### 결정(사용자, 2026-11-09, 하나씩)

1. **표현 전용 프래그먼트 `FLabAnimStateFragment { ELabMinionState State; float StateStartTime; }`**(A안). 재생 시각 = 월드 시각 − StateStartTime. Walk는 StateStartTime = −위상(`LabWalkPhase(Serial)`)이라 M2와 같은 위상이다. 쓰는 쪽: SpawnEntities(Walk), Contact(Dead, 쿼리 안), State(Dead, 쿼리 안, `StateTimeLeft`와 같은 자리), DamageGen 치명(Hit, 쿼리 안), Snipe(Hit, 지연 명령 `FMassDeferredSetCommand`로 태그 추가와 같은 플러시에서. 대상이 순회 뒤에 정해져 쿼리 안에서 쓸 수 없다). 읽는 쪽: VatISM, ActorSync만. 둘은 태그를 보지 않고 `State`만 보므로 3번(한 프레임 튐)이 구조적으로 사라진다. 버린 안 B(표현 프래그먼트에 `StateTimeLeft` 사본을 매 프레임 복사): 표현이 여전히 태그를 봐야 해서 3번을 따로 고쳐야 한다.
2. **그룹 `LabLogic`**(A안). Contact·Snipe·State·DamageGen·DamageApply가 `ExecuteInGroup = "LabLogic"`, VatISM·ActorSync가 `ExecuteAfter("LabLogic")`. Approach는 엔진 조향이 Tasks 뒤에 돌아야 해서 `Tasks`에 남긴다(MoveTgt만 써서 표현과 무관). 솔버는 그룹 이름 의존성을 자식 노드 전부로 펼친다(`MassProcessorDependencySolver.cpp:680-719`). 엔진(`Movement`·`Tasks`·`Representation`)과 City Sample(`Behavior`·`Traffic`)이 같은 방식이다. 버린 안 B(`ExecuteAfter(State)`만): 체인 끝이 바뀌면 같이 고쳐야 한다.
3. **액터 태그 `FLabActorTag` + 자가 정리**(A안). 붙이기: `ULabActorManagement::OnPostActorSpawn`(새 스폰과 풀 재사용 모두 이 델리게이트를 지난다, `MassActorSpawnerSubsystem.cpp:255,480`)에서 `EntityManager->Defer().AddTag`. 스포너 서브시스템 틱 중 Mass 처리가 워커에서 돌 수 있어 즉시 바꾸지 않는다(`MassEntityManager.h:1625`). 떼기: ActorSync가 "태그는 있는데 `FMassActorFragment`가 비었다"를 보면 `Defer().RemoveTag`. 떼는 쪽 엔진 훅이 없어서다(`ReleaseActorOrCancelSpawning`은 static, `MassRepresentationProcessor.cpp:481`). 표현 종류가 아니라 액터 포인터로 판단하는 이유: 액터를 한 프레임 더 두는 동안 LOD가 되돌아오면 엔진이 같은 액터를 다시 써서(`MassRepresentationProcessor.cpp` "NewActor = Actor") OnPostActorSpawn이 다시 불리지 않는다. 버린 안 B(엔진 `FMassVisualizationChunkFragment`의 표현 종류 비트로 청크 거르기): 액터 엔티티가 청크에 흩어져 있어 거의 거르지 못한다. 확인 스탯 `Lab/ActorTagsRemoved`.

### 바꾼 것

- `LabMassTypes.h`: `FLabAnimStateFragment`, `FLabActorTag`, `LabWalkPhase()`. `LabMassTraits.cpp`: 템플릿에 `FLabAnimStateFragment` 추가.
- `LabMassProcessors.*`: 그룹 이름 `LabLogicGroupName`, `LabUseOldQueries()`(m3qold). Contact·State·DamageGen이 `FLabAnimStateFragment`를 쿼리 안에서 쓴다. Snipe는 `FLabMinionFragment`와 `FLabAnimStateFragment`를 쓰기로 선언하고, 고른 대상은 지연 명령으로 쓴다(m3qold는 정비 전처럼 쿼리 밖 쓰기). `PendingSnipeTick`은 `std::atomic<uint32>`로 `exchange(0)`. State는 `FTransformFragment`를 읽기로 선언해 진단 로그의 쿼리 밖 읽기를 없앴다.
- `LabVatProcessor.*`: `FLabAnimStateFragment`만 읽고 태그를 보지 않는다. `ExecuteAfter("LabLogic")`. m3qold는 `FLabMinionFragment` + 태그, 순서 선언 없음.
- `LabActorSyncProcessor.*`: 위와 같은 읽기·순서. 쿼리에 `FLabActorTag` All 필터(m3qold·m3syncall은 없음). 액터가 비면 태그를 뗀다. 재바인딩 판정을 `BoundSerial`(로직 프래그먼트) 대신 `BoundEntity`(엔티티 핸들)로. `OnPostActorSpawn`이 태그를 붙인다.
- `LabMinion.*`: `ALabMinionStaticPawn : ALabMinionStatic`(정적 메시 + CharacterMovement 틱 끔). `LabCrowdSpawner.cpp`가 `staticpawn`에 쓴다.
- `LabConfig.*`: 변형 `staticpawn`, `m3qold`, `m3syncall`, `m3vis0`, `m3vis15`, `m3hys20`. 별칭 `m0full` = full, `m0static` = static, `m1mass`·`m2mass`·`m3mass` = mass(설정 같고 라벨만 다르다. `-LabStage`는 러너가 따로 준다). 인자 `-LabFullyParallel=0`(게임 모드가 `mass.FullyParallel`을 0으로, massst와 같은 경로. 측정 세션 질문 3·4의 답).
- `LabCrowdSubsystem.cpp`: SpawnEntities가 `FLabAnimStateFragment`를 Walk·−위상으로 초기화. E3b (8) 변형: `m3vis0`는 `BaseLODDistance[Medium] = 0`(화면 밖 High 밴드 0m), `m3vis15`는 1500, `m3hys20`은 `BufferHysteresisOnDistancePercentage = 20`(`FMassVisualizationLODParameters`, `MassRepresentationFragments.h:299-309`). 코드 정비 목록 밖이지만 `run-m3b.ps1`이 이 이름을 부르므로 함께 넣었다.
- `Config/DefaultEngine.ini`: `[ConsoleVariables] mass.commands.LockObserversDuringFlushing=1`. 이 섹션은 GEngineIni에서 `SetBySystemSettingsIni`로 읽힌다(`ConsoleManager.cpp:607`).
- `LabGameMode.cpp`·`HordeOptLab.Build.cs`: CSV 메타데이터(아래).

### CSV 메타데이터 키(측정 세션 질문 5의 답)

| 키(CSV에는 소문자) | 값 | 출처 |
| --- | --- | --- |
| `LabBuild` | 빌드 커밋 해시 12자리. 작업 트리 dirty 여부는 모른다(그것은 `package.ps1`의 `commit.txt`) | `HordeOptLab.Build.cs`가 `.git/HEAD`와 ref 파일을 읽어 `Intermediate/Build/LabGenerated/LabBuildVersion.h`에 `LAB_BUILD_COMMIT`을 쓴다. 두 파일을 `ExternalDependencies`에 넣어 커밋이 바뀌면 UBT가 Build.cs를 다시 평가한다. git 실행 없음 |
| `LabCVars` | `이름=값`을 `\|`로 이음. r.VSync, t.MaxFPS, r.ScreenPercentage, r.DynamicRes.OperationMode, sg.ShadowQuality, sg.AntiAliasingQuality, sg.GlobalIlluminationQuality, sg.EffectsQuality, r.DynamicGlobalIlluminationMethod, r.Shadow.Virtual.Enable, r.AntiAliasingMethod, mass.FullyParallel, mass.commands.LockObserversDuringFlushing. 쉼표는 CSV 메타데이터 행을 깨므로 쓰지 않는다 | `LabGameMode.cpp` `LabFixedCVarString()`, 캡처 시작 직전 값 |
| `LabDriver` | `<어댑터 이름> <사용자 드라이버 버전>` | `GRHIAdapterName`, `GRHIAdapterUserDriverVersion`(`RHIGlobals.h:819-821`) |

개발 실행 로그에도 같은 값이 `lab_build commit=… driver=… cvars=…` 한 줄로 남는다.

### 확인만 한 것(6번, 걷기 루프 끝)

가설 "걷기 루프 `Fmod(Local, Length)`가 End + 0.99까지 가서 다음 클립 첫 행이 섞인다"는 사실이 아니다. 엔진 머티리얼 함수 `BoneIndexUVs`의 Custom 노드가 `UV.y = (0.5 / TextureSize.y) + (int(Frame) / TextureSize.y)`로 행을 정수로 자르고, 굽는 텍스처의 필터가 `TF_Nearest`다(`AnimToTextureUtils.h:269`). End + 0.99는 End 행을 읽는다. 고치지 않았다(engine-notes.md 12절).

### 엔진 기본 프로세서 점검

개발 빌드 `-LabStage=M3 -LabN=2000`에 `-LogCmds="LogMass Log"`를 주면 `UMassCompositeProcessor`가 처음 만든 "Pre Physics Group flat processing graph"를 찍는다(`mass.LogNewProcessingGraph` 기본 1, `MassProcessor.cpp:555`). 우리 프로세서는 모두 PrePhysics 단계이고 다른 다섯 단계의 그래프는 비어 있다. 정비 전 23개가 등록돼 있었다.

| 처리 | 프로세서 | 근거 |
| --- | --- | --- |
| 끔(8) | `MassLookAtTargetGridProcessor`, `MassZoneGraphAnnotationTagUpdateProcessor`(MassAIBehavior), `MassEnvQueryTestProcessor_MassEntityTags`, `MassEnvQueryGeneratorProcessor_MassEntityHandles`(MassEQS), `MassActiveSmartObjectSignalProcessor`(MassSmartObjects), `DebugVisLocationProcessor`(MassGameplayDebug), `MassNavigationSmoothHeightProcessor`(MassNavigation), `MassUpdateInstancedSkinnedMeshProcessor`(MassRepresentation) | 우리 엔티티에 없는 프래그먼트(LookAt, ZoneGraph, EQS, SmartObject, 디버그, 높이 보정, ISKM)를 요구해 매 프레임 빈 순회와 그래프 노드만 만든다. `MassNavigationSmoothHeightProcessor`는 다른 16개 뒤에 오도록 의존성이 걸려 있었다. `Config/DefaultMass.ini`에 `bAutoRegisterWithProcessingPhases=False`. ISKM(M5)에서 `MassUpdateInstancedSkinnedMeshProcessor`를 다시 켠다 |
| 둠(엔진 7) | `MassApplyForceProcessor`, `MassApplyMovementProcessor`, `MassSmoothOrientationProcessor`, `MassSteerToMoveTargetProcessor`, `MassLODCollectorProcessor`, `MassVisualizationLODProcessor`, `MassVisualizationProcessor` | 이동·조향·LOD·표현. `MassApplyForceProcessor`는 힘이 0이라 결과를 바꾸지 않지만 이동 Trait의 일부이고 `massav` 변형의 회피가 힘을 쓴다. M4 (d)에서 이동을 우리 프로세서로 바꾸면 그때 다시 본다 |
| 둠(Lab 8) | Approach, OrbContact, OrbSnipe, MinionState, DamageGen, DamageApply, VatISM, ActorSync | DamageGen·DamageApply는 변형이 아니면 첫 분기에서 돌아간다 |

정비 뒤 그래프(15개): ApplyForce, LODCollector → ApplyMovement → SmoothOrientation → **Contact → Snipe → State → DamageGen → DamageApply**, VisualizationLOD → Visualization, Approach → SteerToMoveTarget, **ActorSync·VatISM은 로직 다섯과 Visualization 뒤**. 선언한 순서가 그래프에 그대로 나온다.

### 확인

- `Scripts/build.ps1` 성공(경고는 엔진 API 폐기 예고뿐, 정비 전과 같다).
- `run-dev.ps1 -Small`(N=100, P1, 20초) 종료 코드 0: `-LabStage=M3` mass, M2 mass, M1 mass, M0 full. 새 변형(`m3qold`·`m3syncall` M3, `staticpawn` M0)도 0. 모두 `respawns=43 kills=8 snipes=39`로 같다(시드·시나리오가 그대로다). 프로세서를 끈 뒤 M3·M2·M1을 다시 돌려 0.
- M3 N=2000 13초: `lab_actors synced=120\~133 rebinds` 로그로 태그 필터 상태에서 액터 동기화가 돈다. `lab_build commit=6274752b9668 driver=NVIDIA GeForce RTX 4090 621.14 cvars=r.VSync=0|…|mass.FullyParallel=true|mass.commands.LockObserversDuringFlushing=true`로 메타데이터 값이 찍힌다(개발 실행이라 `r.ScreenPercentage=0`, 벤치에서는 러너 값).
- 측정 세션이 `run-bench.ps1`·`run-dev.ps1`에 `m3qold`·`m3syncall`·별칭을 넣었다(53c7b05). `run-m3b.ps1 -SerialArgs "-LabFullyParallel=0"`은 그대로 동작한다.
- 패키징·벤치마크는 하지 않았다(측정 세션 몫). A/B(`m3qold`·`m3syncall` vs `m3mass`, 20k P1 3회)는 묶음 1에서 잰다.

### 패키징 경합(코디네이터 보고, 2026-11-09)

- 증상: 측정 세션의 `package.ps1 -Config Test` 첫 실행이 실패했다. UBT 인스턴스 여럿(쿠킹용 에디터 타깃 + 게임 타깃)이 `Intermediate/Build/LabGenerated/LabBuildVersion.h`를 동시에 만들려다 파일 잠금이 충돌했다. 헤더가 아직 없을 때 "없으면 쓴다" 분기가 모든 프로세스에서 참이 된다. 개발 빌드는 UBT가 하나라 드러나지 않았다.
- 고침(`HordeOptLab.Build.cs` `WriteHeaderIfChanged`): 임시 파일 `LabBuildVersion.h.<guid>`에 쓰고 `File.Move(…, overwrite)`로 바꾼다. `IOException`·`UnauthorizedAccessException`이면 200ms×시도 뒤 다시 읽어 내용이 같으면 성공(다른 인스턴스가 먼저 씀), 다르면 3회까지 재시도 후 예외. 디렉터리 생성 예외도 삼킨다. UBT는 net10.0(`Engine/Source/Programs/Shared/UnrealEngine.csproj.props:32`)이라 덮어쓰기 `Move` 오버로드가 있다.
- 상태: 파일만 고쳤다. 측정 세션의 패키징과 선행 묶음이 도는 동안 `build.ps1`·`run-dev.ps1`을 돌리지 않았고 커밋도 하지 않았다. 묶음이 끝나면 코디네이터 신호로 빌드와 `run-dev.ps1 -Small`을 확인한 뒤 커밋한다.
