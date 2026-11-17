# 명령과 측정 절차

STATUS.md "명령"에는 매번 쓰는 것만 둔다. 여기에는 스크립트 사양, 인자, 측정 절차를 둔다.

## 스크립트 사양

모든 스크립트는 `Scripts/common.ps1`을 불러 엔진 경로를 얻는다. 엔진 경로는 저장소에 적지 않고 `.uproject`의 `EngineAssociation`을 레지스트리(`HKCU\Software\Epic Games\Unreal Engine\Builds`)에서 찾는다.

| 스크립트 | 하는 일 | 거부 조건 |
| --- | --- | --- |
| `build.ps1` | `HordeOptLabEditor Win64 Development` 빌드 | `UnrealEditor` 또는 `HordeOptLab` 프로세스가 떠 있으면 |
| `run-dev.ps1 -N -Preset -Seed [-Stage 1~8] [-Tag x] [-Extra "..."] [-Small]` | Development 에디터 빌드를 `-game`으로 실행. 개발·동작 확인·오버레이 스크린샷용. 수치는 쓰지 않는다. `-Stage`는 사다리 단계(기본 8), `-Tag`는 로그 이름 `LabDev-s<단계>-<Tag>-N<N>-<P>.log`에 붙는다 | |
| `run-dev-stages.ps1 [-Stages 1,2,…] [-Ablations]` | 사다리 단계마다 `run-dev.ps1 -Small`을 돌리고 종료 코드를 모은다. `-Ablations`는 아래 기법별 기여 표의 스위치를 단계 8에 하나씩 더한다 | |
| `run-spike.ps1 -Batch <이름> -RadiusM 57.3 -HeightM 6 -Speeds <m/s,…> -Configs <구성,…> [-Runs] [-ConfigNs s1=1000] [-ConfigRuns s8=3] [-DryRun]` | 작은 Test 패키지 묶음 러너(v2). 구성은 `s1`\~`s8`(단계), `a8*`(기법별 기여), `s8av`(엔진 회피 더하기), `s<n>ser`(직렬 트레이스). 라벨 `<묶음>-V<속도 번호>-<구성>-N<N>-P1-r<i>`. 메타데이터를 구성의 기대값과 대조한다(`spike-check.py`) | 다른 실행이나 Chrome이 떠 있으면. `progress.log`가 이미 있으면 |
| `package.ps1 [-Config Test]` | `RunUAT BuildCookRun` → `Packaged/<Config>/`. 끝나면 소요 시간을 출력 | 다른 실행이 떠 있으면 |
| `run-bench.ps1 -Milestone <M> -N <수> -Preset <P1\|P2> [-Variant] [-Seed] [-Runs 3] [-Tag x] [-Trace]` | Test 패키지 실행 파일을 `-LabBench` 인자로 띄우고 종료를 기다린다. 라벨은 `<Milestone>-<Variant>-N<수>-<Preset><Tag>`로 만든다. 실행마다 `Saved/Profiling/CSV/<라벨>-rN.csv`를 확인하고, 없으면 실패 | 같은 라벨의 CSV가 있으면. 다른 실행이 떠 있으면 |
| `summarize.py --milestone M<N>` | `Saved/Profiling/CSV/`의 라벨을 읽어 `Measurements/M<N>/runs.csv`에 행을 더하고 `summary.csv`를 다시 쓴다 | |
| `plot.py` | `Measurements/*/summary.csv`로 `Docs/figures/`의 그래프를 다시 그린다: 60fps 최대 N(`n-at-60fps.svg`와 `.csv`), N 스케일링(`n-scaling-P1.svg`) | |
| `summarize.py --milestone M<N> [--rebuild]` (M3b) | 열 추가: `frame_p95_ms`, `bottleneck`(프레임·Game·Render·GPU p50 중 최댓값 이름), `build`(CSV 메타데이터 `LabBuild`·`LabCVars`·`LabDriver`, 없으면 빈칸). `--rebuild`는 기존 행도 원본 CSV로 다시 계산한다. 태그 `aa*` 라벨은 `<변형>-aa` 조건으로 따로 요약, `trace*`는 summary에서 뺀다 | |
| `spikes.py <라벨 접두사 또는 CSV...> [--factor 2] [--verbose] [--with-r0]` (M3b) | 묶음의 주기적 스파이크 검사: 중앙값 2배 초과 프레임의 사건 간격 중앙값과 ±10% 비율, 실행의 주기성 판정, 묶음 요약. 예: `py -3 -I Scripts/spikes.py M3b-` | 대상 없으면 |
| `Scripts/Archive/run-m3b.ps1 [-Phase all\|e3b\|matrix\|aa\|trace\|…] [-NoBase] [-Tag x] [-DryRun]` (M3b) | 묶음 1 러너. E3b (8) 24회를 실행 단위로 섞어 먼저, 전수(M0\~M3, 30k 포함), 묶음 끝 A/A 3회, 20k P1 트레이스 병렬 1 + 직렬 1 순서. `-DryRun`은 목록과 예상 시간만 출력 | 다른 실행이 떠 있으면(run-bench.ps1) |
| `run-bench.ps1 ... [-RunIndex i] [-Trace] [-ExtraArgs "..."]` (M3b) | `-RunIndex`는 r<i> 하나만(실행 단위 섞기), `-Trace`는 `Saved/Traces/<라벨>.utrace`(`-trace=cpu,gpu,frame,counters,mass`), `-ExtraArgs`는 게임 인자 덧붙임 | |
| `noise-table.py <라벨 접두사...>` (M3b) | 접두사별 실행의 프레임 p50·p95, Game·Render p50을 표로 내고 중앙값·최소\~최대·표준편차를 붙인다. 잡음 진단용. 예: `py -3 -I Scripts/noise-table.py M3b-m3mass-N20000-P1aff- M3b-m3mass-N20000-P1noaff-` | |

`run-full.ps1`, `capture-full.ps1`, `full-conditions.json`, `ablation-check.py`, `equivalence-check.py`는 v2 W12에서 새 단계(`-LabStage=1`\~`8`)로 다시 만들었다(아래 "총괄 묶음 러너·검사·요약·촬영"). `run-pool.ps1`은 v2로 옮겼다(아래 "엔티티 풀 긴 실행 러너"). `run-bench.ps1`, `run-budget.ps1`, `run-mgr.ps1`, `run-q4.ps1`, `run-transition.ps1`는 `v1`의 러너다. `-LabVariant`와 옛 단계 이름(`M0`\~`M5`)을 넘기므로 v2 W10 뒤의 빌드에서는 종료 코드 2로 끝난다.

## 사다리 단계와 스위치(v2 W10)

실행 인자 `-LabStage=1`\~`8`이 사다리 단계 하나를 고른다(기본 8 = 최종 구성). 단계는 한 빌드 안의 실행 모드다([ADR-0004](../Decisions/0004-stages-as-run-modes.md)). 한 단계는 앞 단계에서 한 묶음만 바꾼다. 옛 단계 이름(`M0`\~`M5`)과 `-LabVariant=`는 지웠고, 주면 게임이 종료 코드 2로 끝난다(`lab_stage_unknown`, `lab_variant_removed`). 경위와 결정은 [Worklog/V2.md](../Worklog/V2.md) "W10"에 있다.

| 단계 | 앞 단계에서 바꾼 묶음 | 옛 이름(v2 W9까지) |
| --- | --- | --- |
| 1 | 액터 기준선: 모든 개체가 `ALabMinion`(캡슐, `CharacterMovement`, 캡슐끼리 막기) | `M0 full` |
| 2 | Mass, 엔진 기본 경로: 엔진 이동·회피·표현, 가까운 개체는 하이드레이션 액터(캡슐 Trace, 화면 안 35m, 상한 1,000. 단계 8까지 같다, ADR-0021), 나머지는 엔진 `UMassUpdateISMProcessor`의 정지 메시 인스턴스, 200m 밖 그리지 않음 | 없음(새 조합) |
| 3 | 직접 짠 분리: 엔진 회피 대신 `ULabSeparationProcessor`, 엔진 방향 보간 대신 같은 결과의 병렬 사본 `ULabSmoothOrientationProcessor` | `M3` + 정지 메시(새 조합) |
| 4 | 애니메이션 표현: VAT 인스턴스, 액터 애니메이션 예산(화면 크기), 전환 대책 다섯, 최소 유지 시간 2초 | `M3 mass` + 예산 |
| 5 | Simulation LOD(80/200m, 200m 밖 자체 이동) + 엔티티 풀 | `M5 m4offpool` |
| 6 | ISKM(카메라를 따라가는 고정 바운드, 트랙 예약) | `M5 m5bounds` |
| 7 | 배경 무리(엔티티 구동 입자, 맵 밖 독립 입자) | `M5 q2crowd` |
| 8 | 남은 비용 줄이기: 80m 밖 인스턴스 그림자 끔, 액터 메시 고정 바운드(v2 W11). 액터 상한과 전환 거리는 단계 2와 같다(ADR-0021) | `M5 q4final`(전환 거리 40m) + 고정 바운드 |

단계별 스위치와 CSV 메타데이터 값이다. 빈칸은 그 단계에 없는 기법이다(메타데이터 `0`이나 `-`).

| 메타데이터(인자) | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 인스턴스 갱신 프로세서(`lab_ism_processor`) | | engine | engine | vat | vat | iskm | iskm | iskm |
| `LabTechHydrate`(`-LabHydrate=0`이 끈다) | 0 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
| `LabActorCollision`(`-LabActorCollision`) | pawn | trace | trace | trace | trace | trace | trace | trace |
| `LabTechAvoid`(`-LabAvoid`) | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 0 |
| `LabTechSeparate`(`-LabSeparate`) | 1 | 0 | 1 | 1 | 1 | 1 | 1 | 1 |
| `LabSmoothOrient`(`-LabSmoothOrient`) | - | 0 | 1 | 1 | 1 | 1 | 1 | 1 |
| `LabTechInstAnim` | 0 | 0 | 0 | 1 | 1 | 1 | 1 | 1 |
| `LabTechAnimBudget`(`-LabScreenSigBudget`) | 0 | 0 | 0 | 1 | 1 | 1 | 1 | 1 |
| `LabTechHydrateFix`(`-LabHydrateFixMask`) | 0 | 0 | 0 | 1 | 1 | 1 | 1 | 1 |
| `LabRepHold`(`-LabRepHold`) | 0 | 0 | 0 | 2000 | 2000 | 2000 | 2000 | 2000 |
| `LabTechSimLod`(`-LabSimLod=0`이 끈다) | 0 | 0 | 0 | 0 | 1 | 1 | 1 | 1 |
| `LabTechPool`(`-LabEntityPool`) | 0 | 0 | 0 | 0 | 1 | 1 | 1 | 1 |
| `LabTechIskm`(`-LabIskm=0`이 끈다) | 0 | 0 | 0 | 0 | 0 | 1 | 1 | 1 |
| `LabTechFixedBounds`(`-LabFixedBounds=0`이 끈다) | 0 | 0 | 0 | 0 | 0 | 1 | 1 | 1 |
| `LabBoundsStepM`(`-LabBoundsStepM`) | - | - | - | - | - | 10 | 10 | 10 |
| `LabIskmTrackReserve`(`-LabIskmTrackReserve`) | - | - | - | - | - | 1 | 1 | 1 |
| `LabTechCrowdFx`(`-LabCrowdFx`) | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 1 |
| `LabFxOrbFollow`(`-LabFxOrbFollow`) | - | - | - | - | - | - | 1 | 1 |
| `LabTechFarCull`(`-LabFarCull=0`이 끈다) | 0 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
| `LabActorCap`·`LabHydrateM`(`-LabActorCap`, `-LabHydrateM`) | | 1000·35 | 1000·35 | 1000·35 | 1000·35 | 1000·35 | 1000·35 | 1000·35 |
| `LabFarShadow`(`-LabFarShadow`) | - | 1 | 1 | 1 | 1 | 1 | 1 | 0 |
| `LabFixedSkelBounds`(`-LabFixedSkelBounds`, v2 W11) | - | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
| `LabQueryIsolate`(`-LabQueryIsolate`, v2 W11) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| `LabActorOpt`(`-LabActorOpt`, 단계 1의 URO와 Budget Allocator) | 0 | - | - | - | - | - | - | - |

- `-LabFixedSkelBounds=1`은 하이드레이션 액터 메시에 `bComponentUseFixedSkelBounds`를 켜고 `BoundsScale` 1.1을 곱한다(`FLabQuality::FixedSkelBoundsScale`). `-LabQueryIsolate=1`은 게임 모듈 시작 때 `p.Chaos.AccelerationStructureIsolateQueryOnlyObjects`를 1로 둔다. 메타데이터 `LabQueryIsolate`는 그 CVar의 실제 값이다. 근거는 [Worklog/V2.md](../Worklog/V2.md) "W11"과 [engine-notes.md](../Reference/engine-notes.md) 44절이다.
- `-LabSmoothOrient=1`은 엔진 `UMassSmoothOrientationProcessor`를 끄고 같은 식을 청크 병렬로 도는 `ULabSmoothOrientationProcessor`를 켠다. 결과는 엔진과 비트 단위로 같다. 개발 빌드의 `-LabSmoothOrientCheck`는 엔진 프로세서를 두고 사본과 견준다(메타데이터 `check`, 로그 `lab_smoothori_check`). 근거는 [Worklog/V2.md](../Worklog/V2.md) "방향 보간 병렬 사본"과 "방향 보간 병렬 사본: 사다리 배선"이다.
- `-LabActorOpt=1`은 단계 1에서만 뜻이 있다. 단계 1 액터를 `ALabMinionBudget`(메시 `USkeletalMeshComponentBudgeted`, URO 켬)으로 만들고 Animation Budget Allocator를 `-LabBudgetMs`(기본 4ms)로 켠다. 중요도는 하이드레이션 액터와 같은 화면 크기다. Budget Allocator가 도는 동안 엔진이 URO를 끈다. 메타데이터는 `LabActorOpt` 1, `LabBudget` 1, `LabBudgetMs` 4이고 `LabOverrides`에 `actor_opt=1`이 붙는다. 조건 표의 구성은 `s1opt`다. 근거는 [Worklog/V2.md](../Worklog/V2.md) "단계 1의 URO·애니 예산 스위치"와 [engine-notes.md](../Reference/engine-notes.md) 52절이다.

- 메타데이터 `LabStage`와 `LabLadder`는 단계 번호다. 인자로 단계 기본값과 다르게 준 것은 `LabOverrides`에 `|`로 이어 남는다.
- 0/1 스위치(`-LabAvoid`, `-LabSeparate`, `-LabScreenSigBudget`, `-LabEntityPool`, `-LabCrowdFx`, `-LabFxOrbFollow`, `-LabIskmTrackReserve`, `-LabFarShadow`, `-LabFixedSkelBounds`, `-LabQueryIsolate`, `-LabSmoothOrient`, `-LabActorOpt`)는 두 방향 모두 받는다. `-LabHydrate`, `-LabSimLod`, `-LabIskm`, `-LabFixedBounds`, `-LabFarCull`은 0일 때만 뜻이 있다(끄기 전용).
- 정지 조건은 `-LabOrbOrbitSpeed=0`이다. 단계 8과 기법별 기여 조건에만 붙인다([v2-plan.md](../v2-plan.md) "사다리").

### 기법별 기여(v2 W10)

최종 구성(단계 8)에서 기법 하나를 끄고 같은 조건으로 잰다. 조건 표의 이름은 `ablation`이다. 단계 8에 스위치 하나를 더한다. 총괄 묶음의 조건 표는 위 열일곱 행 모두(이동 조건. 정지 조건은 `a8fxf`와 `a8stp`를 뺀 열다섯 행)와 하위 실험 `s8av`(분리를 엔진 회피로 바꾼 것)다. 하위 실험 `s8iso`는 `-Only extra`로만 돈다. 근거는 [Worklog/V2.md](../Worklog/V2.md) "W12"에 있다.

| 구성(`run-spike.ps1`) | 끄는 것 | 인자 | 바뀌는 메타데이터 |
| --- | --- | --- | --- |
| `a8hyd` | 하이드레이션 액터 → 전 구간 ISKM | `-LabHydrate=0` | `LabTechHydrate` 0, `LabActorCollision` `-` |
| `a8bud` | 액터 애니메이션 예산 | `-LabScreenSigBudget=0` | `LabTechAnimBudget` 0, `LabBudget` 0 |
| `a8fix` | 전환 대책 다섯 | `-LabHydrateFixMask=0` | `LabTechHydrateFix` 0 |
| `a8hold` | 최소 유지 시간 | `-LabRepHold=0` | `LabRepHold` 0 |
| `a8pool` | 엔티티 풀 | `-LabEntityPool=0` | `LabTechPool` 0 |
| `a8sim` | Simulation LOD와 200m 밖 자체 이동 | `-LabSimLod=0` | `LabTechSimLod` 0, `LabSimLod` `-` |
| `a8iskm` | ISKM → VAT 인스턴스 | `-LabIskm=0` | `LabTechIskm` 0, `LabIskmBounds` 0 |
| `a8bnd` | 고정 바운드 | `-LabFixedBounds=0` | `LabTechFixedBounds` 0, `LabIskmBounds` 0 |
| `a8far` | 200m 밖 지우기 | `-LabFarCull=0` | `LabTechFarCull` 0 |
| `a8fx` | 배경 무리 | `-LabCrowdFx=0` | `LabTechCrowdFx` 0 |
| `a8sep` | 겹침 방지(Lab 분리) | `-LabSeparate=0` | `LabTechSeparate` 0 |
| `a8cap` | 액터 캡슐 | `-LabActorCollision=0` | `LabActorCollision` 0 |
| `a8res` | ISKM 트랙 예약 | `-LabIskmTrackReserve=0` | `LabIskmTrackReserve` 0 |
| `a8fxf` | 입자 외삽 목표 매 프레임 | `-LabFxOrbFollow=0` | `LabFxOrbFollow` 0 |
| `a8stp` | 바운드 상자 따라가기 | `-LabBoundsStepM=0` | `LabBoundsStepM` 0 |
| `a8shd` | 80m 밖 그림자 끔 → 켬 | `-LabFarShadow=1` | `LabFarShadow` 1 |
| `a8fsb` | 액터 메시 고정 바운드 → 피직스 에셋 바운드(v2 W11) | `-LabFixedSkelBounds=0` | `LabFixedSkelBounds` 0 |

- 하위 실험 `s8iso`(단계 8 + `-LabQueryIsolate=1`, v2 W11)와 `s8av`(엔진 회피 더하기)가 `run-spike.ps1`과 조건 표에 있다. `run-dev-stages.ps1 -Ablations`는 기법별 기여 열일곱 개와 두 하위 실험, 정지 조건을 돈다. `run-spike.ps1`의 `s8sooff`(단계 8 + `-LabSmoothOrient=0`)는 방향 보간 사본을 끈 확인용 구성이고 조건 표에는 없다.
- v2 W11부터 `run-spike.ps1`은 `s8` 뒤에 조각을 이어 붙인 구성 이름을 받는다: `fsb`·`fsb0`(고정 바운드 켬·끔), `iso`·`iso0`(칸 나누기), `c<개수>`(`-LabActorCap`), `h<미터>`(`-LabHydrateM`), `shd0`·`shd1`(`-LabFarShadow`). 예: `s8h45`, `s8c750`. 조각마다 기대 메타데이터 하나를 대조한다.

```bash
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Stage 8 -Tag a8sep -Extra "-LabSeparate=0"
powershell -ExecutionPolicy Bypass -File Scripts/run-dev-stages.ps1 -Ablations
powershell -ExecutionPolicy Bypass -File Scripts/run-spike.ps1 -Batch <묶음> -RadiusM 57.3 -HeightM 6 -Speeds 2 -Configs "s8,a8sep" -Runs 3 -DryRun
```

## 벤치마크 러너 인자(게임 측)

`-LabBench` 가 있으면 다음 순서로 동작하고 끝나면 종료한다.

1. 렌더 CVar 고정(measurement.md "렌더 고정")과 `-LabN= -LabPreset= -LabSeed= -LabStage=` 적용. 모든 값을 로그 `lab_config`에 한 줄로 남긴다.
2. 스포너가 정상 상태 분포로 N개 배치. 카메라를 프리셋에 놓는다.
3. 워밍업(이동 60초, 정지 15초, `-LabWarmup=`으로 바꾼다). 그 사이 N 유지 확인(사망·재생성이 동작하는지).
4. `csvprofile start`(압축 끔, 라벨을 파일 이름으로). 측정 구간(이동 180초, 정지 60초, `-LabDuration=`으로 바꾼다).
5. `csvprofile stop`. 프레임 수, N, 전환 수 등 요약을 로그 `lab_summary`에 남긴다. 조건이 깨졌으면(N 불일치 등) 종료 코드 1.
6. `-Trace`면 2\~5 동안 `-trace=cpu,gpu,frame,counters,mass`로 트레이스를 남긴다. 트레이스 실행의 수치는 비교에 쓰지 않는다(이벤트 기록 비용).

## 기법별 기여 스위치(Q3)

이 절과 아래 "전환 점수", "근거리 품질 인자", "총괄 묶음 조건과 HUD"는 `v1`의 기록이다. 변형 이름(`q4final` 등)은 v2 W10에서 지웠다. 지금의 인자는 위 "사다리 단계와 스위치(v2 W10)"에 있다. 옛 명령의 `-Stage M5 -Variant q4final`은 `-Stage 8`, `-Variant q2crowd`는 `-Stage 7`로 바꿔 읽는다.

기법별 기여 표([plan.md](../plan.md) "품질 계획")의 행은 "최종 구성 변형 + 인자 하나"다. 최종 구성은 Q4부터 `q4final`(`-LabStage=M5`, `q2crowd` + 액터 상한 1,000·전환 거리 40m·80m 밖 그림자 끔, 아래 "근거리 품질 인자(Q4)")이고 아홉 행 모두 `q4final` 위에서 대조를 통과했다(Worklog/Q4.md, 그때 전환 거리는 79m). Q4b에서 열째 행 farshadow를 더했다(조건 표 `A8-farshadow`). 새 인자 다섯(`-LabHydrate`·`-LabSimLod`·`-LabIskm`·`-LabFixedBounds`·`-LabFarCull`)은 0일 때만 뜻이 있고(끄기 전용), 나머지 넷은 기존 인자다. 경위와 설계 선택은 [Worklog/Q3.md](../Worklog/Q3.md).

| 행 | 빼는 것 → 대신 | 인자 | 메타데이터에서 0이 되는 키 |
| --- | --- | --- | --- |
| final | 없음 | 없음 | 없음(`LabTech*` 아홉 모두 1) |
| hydrate | 하이드레이션 → 전 구간 ISKM(액터 0) | `-LabHydrate=0` | `LabTechHydrate` |
| budget | 액터 애니메이션 예산 → 예산 없는 액터(`USkeletalMeshComponent`, 매 프레임 애니) | `-LabScreenSigBudget=0` | `LabTechAnimBudget`, `LabBudget` |
| fix | 전환 대책 다섯 → 모두 끔 | `-LabHydrateFixMask=0` | `LabTechHydrateFix`, `LabHydrateFixMask` |
| pool | 엔티티 풀 → 매번 생성·파괴 | `-LabEntityPool=0` | `LabTechPool`, `LabPool` |
| simlod | Simulation LOD → 엔진 이동·매 프레임 로직을 전 개체에 | `-LabSimLod=0` | `LabTechSimLod`(`LabSimLod`는 `-`) |
| iskm | ISKM → VAT ISM(고정 바운드는 넣을 컴포넌트가 없다) | `-LabIskm=0` | `LabTechIskm`, `LabIskmBounds` |
| bounds | 고정 바운드 → 기본 바운드 | `-LabFixedBounds=0` | `LabTechFixedBounds`, `LabIskmBounds` |
| farcull | 200m 밖 지우기 → 맵 끝까지 그리기(배경 무리는 그대로 켬) | `-LabFarCull=0` | `LabTechFarCull` |
| crowdfx | Niagara 배경 무리 → 빈 수평선 | `-LabCrowdFx=0` | `LabTechCrowdFx`, `LabCrowdFxActive` |
| farshadow | 80m 밖 인스턴스 그림자 끔 → 켬(Q4b) | `-LabFarShadow=1` | `LabFarShadow`가 1이 된다(`LabTech*` 아홉은 그대로) |

```bash
# Test 패키지(Q5 뒤, run-bench.ps1의 ValidateSet에 q2crowd가 들어가기 전에는 -Variant m5bounds -ExtraArgs "-LabCrowdFx=1 <인자>")
powershell -ExecutionPolicy Bypass -File Scripts/run-bench.ps1 -Milestone M5 -Variant m5bounds -N 10000 -Preset P1 -Types 2 -Runs 1 -Tag hydrate -ExtraArgs "-LabCrowdFx=1 -LabHydrate=0"
# Development 작은 실행 + 메타데이터 대조(라벨에 행 이름을 넣으면 ablation-check.py가 기대값을 고른다)
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Stage 7 -Extra "-LabHydrate=0 -LabBench -LabLabel=Q3dev-hydrate-N100-P1-r1"
py -3 -I Scripts/ablation-check.py Saved/Profiling/CSV/Q3dev-hydrate-N100-P1-r1.csv
```

- `lab_config` 끝의 `tech=hyd:1,bud:1,fix:1,pool:1,sim:1,iskm:1,bnd:1,far:1,fx:1`이 아홉 기법의 설정이다. 단계가 그 기법을 아직 갖지 않으면 0이다(M4 `m4mass`의 Simulation LOD는 틱 간격만이라 `tick`, 전환 대책 일부만 켜면 `m<마스크>`).
- 실제 적용값은 `lab_applied`(개발 빌드 로그, 벤치마크는 측정 시작 때, 개발 실행은 끝날 때)와 CSV 메타데이터 `LabStage`·`LabBudget`(Budget Allocator가 도는가)·`LabPool`·`LabIskmBounds`(고정 바운드가 들어간 ISKM 메시 종류 수, 2종이면 2. Q4b부터 컴포넌트가 아니라 메시 종류를 센다)·`LabHydrateFixMask`다.
- `-LabFarM`(그리는 거리)은 표현 Off와 Simulation LOD Off, 배경 무리 안쪽 반경을 함께 옮기므로 farcull 행에 쓰지 않는다. `-LabFarCull=0`은 표현 Off만 맵 반경의 두 배(1,000m)로 옮긴다.

## 전환 점수(Q3)

`Scripts/transition-score.py`는 60fps 고정 간격 연속 캡처와 그 실행의 개발 로그로 전환 사건마다 화면 튐을 잰다. ISKM과 VAT, 전환 대책 켬·끔은 같은 시드·같은 인자로 찍고 표현만 바꾼다. 캡처는 `Saved/Screenshots/LabBurst/`에 쌓이므로 실행마다 다른 폴더로 옮긴다.

```bash
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 10000 -Preset P1 -Duration 21 -ShotAt 10 -Stage 7 -Tag burst -Extra "-LabShotBurst=600 -benchmark -fps=60"
py -3 Scripts/transition-score.py <캡처 폴더> Saved/Logs/LabDev-s7-burst-N10000-P1.log --event both
```

`--event`는 `hydrate`(인스턴스 → 액터, 기본), `dehydrate`(액터 → 인스턴스), `both`. VAT 표현은 `-Extra`에 `-LabIskm=0`, 대책 끔은 `-LabHydrateFixMask=0`을 더한다.

## 근거리 품질 인자(Q4)

최종 구성 변형은 `q4final`(= `q2crowd` + 액터 상한 1,000 + 화면 안 전환 거리 40m + 80m 밖 인스턴스 그림자 끔, `-LabStage=M5`)이다. Q4 묶음 때는 전환 거리 79m·그림자 켬이었고 Q4b에서 바꿨다. 경위는 [Worklog/Q4.md](../Worklog/Q4.md).

| 인자 | 뜻 | 기본 |
| --- | --- | --- |
| `-LabActorCap=<개수>` | 액터 상한(표현 LOD High의 `LODMaxCount`, 두 종류 합계) | 500, `q4final` 1,000 |
| `-LabHydrateM=<미터>` | 화면 안에서 액터로 바뀌는 카메라 거리(1\~79m). 화면 밖 15m와 80·200m 밴드는 그대로 | 30, `m3hyd15` 15, `m3hyd50` 50, `q4final` 40 |
| `-LabFarShadow=0\|1` | 표현 LOD Low(80m\~그리는 끝) 인스턴스의 그림자. M3 이상 단계에서만 뜻이 있다. ISKM과 VAT ISM 모두 메시 설명자를 LOD 중요도 범위로 둘로 나눠 끈다 | 1, `q4final` 0 |

위 "기본" 열은 `v1`의 값이다. 지금 `-LabActorCap`과 `-LabHydrateM`의 기본값은 단계 2\~8 모두 1,000과 35m다([ADR-0021](../Decisions/0021-unified-actor-cap-and-hydrate-distance.md), 위 단계별 스위치 표).

CSV 메타데이터 `LabActorCap`·`LabHydrateM`·`LabFarShadow`(M3 미만 `-`), 로그 `lab_config`의 `far_shadow=`, 스탯 `Lab/ViewActors`(화면 안 보이는 액터)·`Lab/ViewActors30`·`Lab/ViewInst30`(화면 안 카메라 30m 안 액터·인스턴스)·`Lab/ViewInstHyd`(전환 거리 안 인스턴스). `summarize.py`가 `view_actors_*`·`cover30_*`·`coverhyd_mean` 열로 낸다.

```bash
# Q4 묶음 러너(run-bench.ps1을 조건마다 부른다. "N:프리셋:상한:거리:횟수:태그앞:태그뒤"를 ;로 잇는다)
powershell -ExecutionPolicy Bypass -File Scripts/run-q4.ps1 -Jobs "10000:P1:1000:79:3::;10000:P2:1000:79:3::" -FirstWithR0
py -3 -I Scripts/summarize.py --milestone Q4
# 빠른 회전 뒤 채워지는 시간(개발 빌드 참고, -LabCamTurn -LabFrameLog 로그)
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 10000 -Preset P1 -Stage 8 -Tag turn -Duration 71 -Extra "-LabCamTurn -LabFrameLog -benchmark -fps=60"
py -3 -I Scripts/turn-fill.py Saved/Logs/LabDev-s8-turn-N10000-P1.log
```

## 총괄 묶음 조건과 HUD(Q5)

이 절은 `v1`의 기록이다(조건 이름 `L1`\~`L8`, `A8-*`는 v2의 조건 표에 없다). v2의 조건 표와 러너는 아래 "총괄 묶음 러너·검사·요약·촬영"에 있다. `-LabHud`, `-LabShotAt`, `-LabActorCollision`, `-LabBudgetMs`의 뜻은 그대로다.

총괄 묶음(`Full1`, 지금은 `Full2`)의 조건(사다리 8단계 `L1`\~`L8`, 기법별 기여 열 행 `A8-*`, 하위 실험 `S1-budget`·`S1-uro`·`S8-col`)은 `Scripts/full-conditions.json`에 인자 줄과 기대 메타데이터로 있다. 표와 경위는 [Worklog/Q5.md](../Worklog/Q5.md).

| 인자 | 뜻 |
| --- | --- |
| `-LabHud` | 상태 오버레이를 켠다. Q5부터 `-LabBench`와 함께 줘도 켜진다(촬영 실행). 메타데이터 `LabHud` |
| `-LabShotAt=<초>` + `-LabBench` | 게임 시작부터 그 초에 창 해상도 스크린샷 한 장(`Saved/Screenshots/Lab/<라벨>-t<초>.png`, HUD 포함). 메타데이터 `LabShotAt`. 측정 실행은 주지 않는다 |
| `-LabActorCollision=1` | 하이드레이션 액터의 캡슐 콜리전을 켠다(`Pawn` 프로필 그대로, 하위 실험 `S8-col`). 기본은 끔(`m3nocol`과 같은 처리, Q5 1b단위부터)이고 `=0`은 기본과 같다. `m3colquery`는 QueryOnly, 라벨 `m3colbase`는 켬. M0 액터는 늘 켬이다. 메타데이터 `LabActorCollision`(적용값 `0`·`1`·`query`, 액터가 없으면 `-`) |
| `-LabBudgetMs=<ms>` | 애니메이션 예산 값(`a.Budget.BudgetMs`, 기본 4, [ADR-0016](../Decisions/0016-animation-budget-value.md)). 0 이하는 예산 끔이고 `-LabScreenSigBudget=0`(기법별 기여 `A8-budget`)과 같은 길이다. 변형이 스스로 켜는 예산(M0 `budget`, `m3budgetdist`, `m3budgetsig`)은 0 이하를 받아도 기본값 4로 켜진다. 4와 다르면 `LabOverrides`에 `budget_ms=<값>`이 붙는다. 메타데이터 `LabBudgetMs`(실제 걸린 CVar 값, 예산이 꺼져 있으면 `-`). 참고 묶음 `Budget1`([Worklog/Q7.md](../Worklog/Q7.md)) |

동등성 통계: CSV 스탯 `Lab/Alive`(살아 있는 총수, 사망 중 포함), `Lab/Deaths`(누적 사망 = 범위 피해 사망 + 저격), `Lab/Near30`(오브 수평 30m 안 개체 수). 실행 끝 메타데이터 `LabDeaths`·`LabZoneKills`·`LabSnipes`·`LabDeathRate`·`LabNear30Mean`·`LabAliveMin`·`LabAliveMax`, 같은 값이 `Saved/Lab/<라벨>.summary.txt`의 `lab_summary` 줄 끝에 있다. 시작 메타데이터 `LabLadder`(사다리 단계), `LabOverrides`(변형 기본값과 다르게 준 인자, `|`로 잇는다, 없으면 `-`).

```bash
# 메타데이터 대조. 라벨이 Full<n>-<단계>-<변형>-N<N>-<P1|P2>-r<i>면 조건을 라벨에서 고른다. 아니면 --cond
py -3 -I Scripts/ablation-check.py Saved/Profiling/CSV/Full2-L8-q4final-N10000-P1-r1.csv
py -3 -I Scripts/ablation-check.py <CSV> --cond L8-q4final --shot
```

## 측정 절차

1. 측정 전에 잴 조건, 라벨, 걸리는 시간을 채팅에 알리고 답을 받는다.
2. `package.ps1`로 그 커밋의 Test 패키지를 만든다. 패키지 폴더에 커밋 해시를 남긴다.
3. 묶음의 맨 첫 실행 1회(`r0`)만 버린다(`run-full.ps1`이 기본으로 돈다). 그 뒤 조건별 3회, 회차가 바깥 루프다.
4. 실행마다 러너가 CSV 메타데이터를 조건 표의 기대값과 대조한다(`ablation-check.py`). Test 패키지는 로그를 쓰지 않는다(개발 빌드는 로그의 `lab_config`·`lab_summary`). N이 유지되지 않았거나 프레임 수가 비정상이면 러너가 그 실행을 실패로 적고 묶음 끝에서 새 회차 번호로 한 번 다시 잰다.
5. `summarize.py` → `runs.csv`, `summary.csv`. `equivalence-check.py` → `equivalence.csv`. `spikes.py`. 차이 판정은 measurement.md.
6. STATUS.md "측정 결과"에 묶음을 적는다. 마일스톤이 끝나면 Worklog로 옮긴다.

## 스크린샷·영상 실행

- 오버레이 스크린샷(`stat unit`, `a.Budget.Debug.Enabled 1`, 밴드 색 디버그 뷰)은 `run-dev.ps1`로 Development에서 찍는다. 그 실행의 수치는 쓰지 않는다.
- Test 패키지 스크린샷은 `stat unit`만 가능하다. Q5부터는 `-LabHud`가 단계 이름, 프레임·Game·Render·GPU 시간(1초 평균), 미니언·액터 수, 프레임 시간 그래프를 그린다(위 "총괄 묶음 조건과 HUD(Q5)").
- 영상은 수치 실행에서 찍지 않는다. 라벨 `visualN`으로 따로 돌린다.

## 측정 PC 사양 읽기

[pc-specs.md](../Reference/pc-specs.md)의 값은 PowerShell에서 아래 명령으로 읽었다. 구성이 바뀌면 같은 명령으로 다시 읽는다.

- CPU, RAM, GPU: `Get-CimInstance Win32_Processor`, `Win32_PhysicalMemory`, `Win32_VideoController`
- GPU 드라이버: `nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv`
- 저장 장치: `Get-Partition -DriveLetter G | Get-Disk`
- 모니터 모델: `Get-CimInstance -Namespace root\wmi WmiMonitorID`
- 전원 구성표: `powercfg /getactivescheme`
- GPU 스케줄링, 게임 모드: `HKLM:\SYSTEM\CurrentControlSet\Control\GraphicsDrivers`의 `HwSchMode`, `HKCU:\Software\Microsoft\GameBar`의 `AutoGameModeEnabled`(없으면 기본값)
- 엔진 커밋: `git -C "G:\Epic Games\UE_5.8.4" rev-parse --short HEAD`, `git -C "G:\Epic Games\UE_5.8.4" log -1 --format=%cs`

## Unreal MCP

- 서버는 에디터가 뜨면 자동으로 열린다: `Config/DefaultEditorPerProjectUserSettings.ini`의 `bAutoStartServer=True`, `ServerPortNumber=8001`(다른 프로젝트의 기본 8000과 겹치지 않게). 수동은 콘솔 `ModelContextProtocol.StartServer`.
- 클라이언트 설정: 프로젝트 루트 `.mcp.json`(gitignore)에 `"unreal-mcp": {"type": "http", "url": "http://127.0.0.1:8001/mcp"}`. 새 체크아웃이면 에디터 콘솔 `ModelContextProtocol.GenerateClientConfig ClaudeCode`로 다시 만든다. Claude Code는 에디터를 띄운 뒤 시작해야 툴이 잡힌다.
- 2026-11-06 확인: `-nullrhi`로 띄운 에디터가 약 15초 뒤 `initialize`에 응답했다.
- 툴 호출은 게임 스레드에서 직렬. 겹쳐 부르지 않는다. 대량 변경 전후 저장. 새 `UFUNCTION`은 에디터 재시작.
- Claude Code는 `claude --add-dir ..\unreal-horde-references`로 시작한다(`.claude/settings.json`의 `additionalDirectories`에도 있다).

## 처음 준비

클론에서 측정까지의 순서(엔진 등록, Fab 두 팩, `Scripts/restore-assets.ps1`로 에셋 복구, 빌드, 작은 실행, 패키징)는 [setup.md](setup.md)에 있다. 아래 두 절은 복구 스크립트가 부르는 스크립트를 하나씩 돌릴 때 본다.

## 에셋 마이그레이션

Fab 원본 팩은 `../unreal-horde-references/AssetHost/`(런처 5.8 엔진 프로젝트)에 받고, 쓰는 것만 `Scripts/migrate-assets.py`로 옮긴다. AssetHost를 연 엔진(런처 `G:\Epic Games\UE_5.8`)의 커맨드렛으로 돌린다. `LAB_MIGRATE_DRY=1`이면 의존성 목록과 크기만 출력한다.

```bash
LAB_MIGRATE_SET=minion LAB_MIGRATE_DRY=0 LAB_MIGRATE_DEST="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Content" "G:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "G:/Epic Games/UE_5.8.4/Games/unreal-horde-references/AssetHost/AssetHost.uproject" -run=pythonscript -script="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Scripts/migrate-assets.py" -unattended -nosplash -nullrhi -stdout
```

2026-11-06 실행: 51개 패키지, 원본 기준 282.7MB(옮긴 뒤 `Content/ParagonMinions/` 244MB). 소프트 참조로 Dusk 메시와 Super 텍스처 일부가 함께 온다.

`siege` 세트(`LAB_MIGRATE_SET=siege`)는 M3b 2종의 대형(시즈 미니언) 메시·피직스·애니 셋이다. 2026-11-09 실행: 의존성 36개 중 새 패키지 11개(원본 기준 56.5MB), 나머지는 이미 있어 건너뜀(`EAssetMigrationConflict::Skip`). 종료 코드 1이 나면 [troubleshooting.md](troubleshooting.md)의 AssetHost 줄을 본다.

`orbfx` 세트(`LAB_MIGRATE_SET=orbfx`)는 오브의 타워·구체·이펙트다. 2026-11-06에 AssetHost 커맨드렛이 Python 플러그인을 올리기 전에 로그 없이 종료 코드 1로 끝나는 일이 있었다. 이때 `MI_Mesh_Overlay_TeslaCoil`과 부모 `M_Mesh_Overlay`는 같은 `/Game` 경로로 `.uasset`을 직접 복사했다. 의존성(MPC, `MF_ExposureCompensation`, 노이즈 텍스처)은 이미 옮겨져 있었다.

## 파생 에셋과 맵

- `Scripts/make-static-minion.py`: `static` 변형의 `/Game/Lab/Derived/SM_LabMinion`을 만든다(gitignore). 커맨드렛에 `-AllowCommandletRendering`이 필요하다. 새 체크아웃에서는 에셋 이전 뒤 한 번 돌린다.
- `Scripts/bake-vat.py`: M2 VAT(걷기·피격·죽기)를 Bone·Vertex 모드로 굽고 정적 메시·데이터 에셋·텍스처·머티리얼을 `/Game/Lab/VAT/Baked/`(gitignore)에 만든다. 실행은 `make-static-minion.py`와 같다(`-AllowCommandletRendering`). 새 체크아웃에서 M2 이상을 돌리기 전에 한 번 돌린다.
- `Scripts/create-iskm.py`: M5 ISKM 에셋(종류마다 `bOptimizeForInstancing`을 켠 스켈레탈 메시 복제본, ASTPD, ISKM 머티리얼, 가산 피격 클립의 비가산 복제본 `A_<종류>_HitReact_Front_Full`)을 `/Game/Lab/ISKM/Baked/`(gitignore)에 만든다. ISKM은 가산 클립을 기준 자세에 더하지 않아 복제본을 쓴다([engine-notes.md](../Reference/engine-notes.md) 46절). 실행은 `make-static-minion.py`와 같다(`-AllowCommandletRendering`). `LAB_UNIT`을 비우면 두 종류, `small`/`large`면 하나. `LAB_ISKM_VERIFY=1`이면 만들지 않고 저장된 에셋을 새 프로세스에서 읽어 확인만 한다. 새 체크아웃에서 M5를 돌리기 전에 한 번 돌린다. 클립 순서와 커스텀 데이터 번호는 [Worklog/M5.md](../Worklog/M5.md) "에셋 세션".
- `Scripts/create-bench-map.py`: `L_Bench`를 엔진 Basic 템플릿으로 다시 만든다(바닥을 입자 바깥 반경 1,000m + 20m(반쪽 길이 1,020m)로 키우고 VolumetricCloud 삭제). 기존 맵이 있으면 `.umap` 파일을 먼저 지운다(스크립트의 `delete_asset`이 커맨드렛에서 듣지 않았다).

```bash
"G:/Epic Games/UE_5.8.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/HordeOptLab.uproject" -run=pythonscript -script="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Scripts/make-static-minion.py" -unattended -nosplash -AllowCommandletRendering -stdout
```

## 벤치마크 실행(구현됨)

이 절의 `-Milestone M0` 식 명령은 `v1` 때의 것이다. `v2`의 실행은 "총괄 묶음 러너·검사·요약·촬영" 절과 [Scripts/README.md](../../Scripts/README.md)를 따른다.

- `package.ps1`이 `Packaged/Test/Windows/`를 만들고 `Packaged/Test/commit.txt`에 커밋 해시(작업 트리가 더러우면 `(dirty)`)를 남긴다. 쿡은 `L_Bench`와, 코드가 경로로 부르는 `/Game/ParagonMinions`·`/Game/NiagaraExamples`·`/Game/Lab` 폴더다(`Config/DefaultGame.ini`).
- `run-bench.ps1 -Milestone M0 -N <n> -Preset <P1|P2> -Variant <v> [-Runs 3] [-Tag x] [-ResX -ResY]`: r0 + r1\~rRuns. 실행마다 CSV를 패키지 `Saved/Profiling/CSV/`에서 프로젝트 `Saved/Profiling/CSV/`로 옮기고 `Saved/Lab/<라벨>.summary.txt`를 출력한다. 해상도 기본값은 2560x1440 창이다(measurement.md "확정값").
- `Scripts/Archive/run-e1.ps1 [-Tag x]`: M1 E1 (a) 묶음(mass N {1k, 2k, 5k, 20k, 50k} x P1·P2 x 3회, 첫 조건만 r0). M1부터 변형 이름은 라벨용 `mass`다. 개발 실행은 `run-dev.ps1 -Extra "-LabStage=M1"`.
- `Scripts/Archive/run-e2.ps1 [-Tag x] [-OnlySub]`: M2 E2 묶음. (a) N {1k, 5k, 20k, 50k} x P1·P2 x 3회로 M1(`noanim`, `run-bench.ps1 -Stage M1`)과 M2(`mass`)를 번갈아, (b)(c)(d) 변형(`vatvert`, `vatcull`, `vatcullvis`, `vatdie0`, `vatdie1k`)은 5k P1 x 3회. 약 90분. (d)는 다시 설계해 `run-bench.ps1 -Milestone M2 -Variant kill0|kill1k -N 5000`으로 쟀다.
- `run-bench.ps1 -Stage <M>`: 결과 폴더·라벨은 `-Milestone`, 실행 단계만 바꾼다(같은 묶음에서 이전 단계를 잴 때).
- `summarize.py`는 `py -3`(3.9)로 돌린다. pandas 없이 표준 라이브러리만 쓴다.

## Unreal MCP를 HTTP로 직접 부르기

이 세션처럼 MCP 클라이언트가 붙어 있지 않을 때는 에디터를 `-nullrhi`로 띄운 뒤 JSON-RPC로 부른다. `initialize` 응답 헤더의 `Mcp-Session-Id`를 이후 요청에 붙인다.

```bash
curl -s -X POST http://127.0.0.1:8001/mcp -H "Content-Type: application/json" -H "Accept: application/json, text/event-stream" -H "Mcp-Session-Id: <id>" -d '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"list_toolsets","arguments":{}}}'
```

## 총괄 묶음 러너·검사·요약·촬영

v2 W12에서 다시 만들었다. 조건과 기대 메타데이터는 `Scripts/full-conditions.json`(v2: 구성, 묶음 종류, 시나리오 값, 동등성 범위)이 정한다. 러너는 패키지 exe를 직접 부르고 시작할 때 방해 프로세스(`UnrealEditor`, 게임, `ShaderCompileWorker`, `UnrealBuildTool`, `chrome`, `ffmpeg`, 창이 있는 `msedge`·`firefox`)가 떠 있으면 멈춘다. 측정 중에는 PC를 쓰지 않는다. 경위와 설계 선택은 [Worklog/V2.md](../Worklog/V2.md) "W12"에 있다. `v1`의 인자와 조건은 태그 `v1`의 이 문서에 있다.

```bash
# 계획만 본다(게임을 띄우지 않는다): 묶음 종류별 슬롯·실행 수·예상 시간, 슬롯마다 명령줄, 실행 순서, 라벨 검사
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -Batch Full4 -DryRun
# 마른 실행 묶음(종류마다 하나씩, 1회씩): 라벨 V2W12Dry-...
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -Batch V2W12Dry -Runs 1 -NFilter "500,10000,20000" -Pick "ladder:s1,ladder:s8,ablation:a8fsb,still:s8,maxn:s8,actor:s1,trace:s2"
# 총괄 묶음 Full4(본 실행 207회, 잰 길이로 13.44시간. 사용자가 시작을 요청한 뒤에 한다. 패키지 커밋에 dirty가 있으면 시작하지 않는다. 연달아 3회 실패하면 종료 코드 2로 멈춘다)
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -Batch Full4
# 끊겼으면 이어 한다(progress.log에 있는 라벨은 건너뛴다)
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -Batch Full4 -Resume
# 포기한 실행(batch.json giveups)을 회차 + 20으로 한 번 더 잰다. 먼저 -DryRun을 붙여 더 잴 라벨을 본다
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -Batch Full4 -Resume -RetryGiveups -DryRun
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -Batch Full4 -Resume -RetryGiveups
# 일부만: -Only ladder,ablation,still,sub,topview,maxn,maxn_mid,maxn_low,actor,aa,trace,traceser,extra (쉼표로 이은 한 문자열)
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -Batch Full4 -Only ladder,ablation -Runs 1

# 묶음 검사와 요약(러너가 끝에 summarize, spikes, equivalence, batch-check를 돌리고 동등성 FAIL이나 묶음 판정 실패면 종료 코드 1이다. 동등성 UNMET은 0)
py -3 -I Scripts/summarize.py --batch Full4
py -3 -I Scripts/equivalence-check.py --batch Full4   # 시험은 --out <csv>, 합성 입력은 --csv-dir <프레임 CSV 폴더> --dir <runs.csv 폴더>
py -3 -I Scripts/batch-check.py --batch Full4   # A/A, 단계 8의 모든 실행, 느린 실행. --dir <폴더> --out <slow.csv>로 스크래치에 돌릴 수 있다
py -3 -I Scripts/spikes.py --batch Full4
py -3 -I Scripts/plot.py --batch Full4          # 확인용은 --out <폴더>. --equivalence <csv>: 기능 미달(UNMET) 점을 최대 N 보간에서 뺀다(기본 Measurements/<묶음>/equivalence.csv)
py -3 -I Scripts/ablation-check.py --selftest    # 조건 표의 기대 키와 묶음 종류의 구성을 검사. 기준 CSV는 Saved/Profiling/CSV의 가장 최근 측정 CSV(--reference <CSV>로 바꾼다)
py -3 -I Scripts/ablation-check.py <CSV> --build <커밋 해시 앞 12자>

# 촬영(측정이 끝난 뒤 따로)
powershell -ExecutionPolicy Bypass -File Scripts/capture-full.ps1 -Batch Full4 -DryRun
powershell -ExecutionPolicy Bypass -File Scripts/capture-full.ps1 -Batch Full4
```

| 인자(`run-full.ps1`) | 뜻 |
| --- | --- |
| `-Batch` | 묶음 이름(필수). 폴더 `Measurements/<Batch>/`, 라벨 접두사. 같은 이름과 라벨은 다시 쓰지 않는다(`progress.log`가 있으면 `-Resume` 아니면 거부) |
| `-Only` | 묶음 종류만: `ladder`(8단계 × 1만 × 이동), `ablation`(기법별 기여 17행 × 이동), `still`(단계 8과 기법별 기여 15행 × 정지), `sub`(`s8av`), `topview`(단계 8 × 이동 × 탑뷰 P2, 참고 한 점), `maxn`(단계 2 × 5천, 단계 3\~8 × 2만), `maxn_mid`(단계 3\~8 × 1.25만), `maxn_low`(단계 2 × 2,500), `actor`(단계 1과 `s1opt` × {500, 1천, 2천}), `aa`(단계 8 반복, 회차 11\~13), `trace`(단계 1\~8), `traceser`(단계 2\~8 직렬), `extra`(후보 `s8iso`. 적어야 돈다). 비우면 `extra`를 뺀 전부 |
| `-Pick`, `-NFilter` | 예행용. `-Pick "ladder:s1,still:*"`은 묶음 종류:구성이 맞는 슬롯만, `-NFilter 500,10000`은 그 N인 슬롯만 남긴다 |
| `-Runs` | 반복 묶음의 회차 수를 덮어쓴다(기본은 조건 표의 3). 트레이스와 A/A는 조건 표 값이다 |
| `-Order round\|cond` | 기본 `round`: 회차가 바깥 루프(r1 전 슬롯, r2 전 슬롯 역순, r3). `cond`: 같은 조건 연달아 |
| `-Resume`, `-DryRun`, `-SkipR0`, `-NoFgCheck`, `-NoClock` | 이어 하기, 계획만, `r0` 건너뛰기, 앞 창 검사 끄기, CPU 클럭 기록 끄기(측정에는 앞 창 검사를 끄지 않는다) |
| `-AllowDirty` | 패키지의 `commit.txt`에 `dirty`가 있어도 시작한다. 러너 시험 전용이다. 측정 묶음에는 쓰지 않는다 |
| `-RetryGiveups` | 계획 실행과 그 실패 재측정이 둘 다 실패한 실행을 회차 + 20(21\~23, A/A는 31\~33)으로 한 번 더 잰다(기본 끔). 실패 재측정 뒤, 느린 실행 재측정 앞에 돈다. 이 실행이 OK면 `batch.json`이 그 계획 실행을 OK로 센다. 또 실패하면 더 재지 않는다. `-DryRun`과 함께 주면 `progress.log`를 읽어 더 잴 라벨을 찍는다 |
| `-MaxConsecFail` | 실행이 연달아 이 횟수만큼 실패하면 묶음을 멈춘다(기본 3, 0이면 끈다). 본 실행, 실패 재측정, 두 번째 재측정(`-RetryGiveups`), 느린 실행 재측정을 이어서 센다. 멈추면 `progress.log`에 `ABORT` 줄, `batch.json`의 `state`가 `aborted`, 종료 코드 2다. 원인을 고친 뒤 `-Resume`으로 이어 한다 |
| `-StartupSec`, `-TimeoutFactor` | 예상 시간 계산용 프로세스 시작 시간(기본 30초), 시간 제한 = 배수(기본 3) × 한 실행 예상 시간 |
| `-ExePath`, `-ExeArgsPrefix`, `-PkgRootOverride`, `-NoProcCheck` | 러너 시험용(게임 없이 파싱·재시도·이어 하기 점검). 측정에는 쓰지 않는다. `-ExeArgsPrefix`는 공백으로 나누므로 경로에 공백이 없어야 한다 |

`-DryRun`의 예상 시간은 둘이다. 상한은 실행마다 시작 30초 + 워밍업 + 측정 + 마무리 2초다. 둘째 값은 지금 러너의 잰 실행 길이(`V2W12Dry`: 이동 249초, 정지 205초, 트레이스 128초, 단계 1의 1만 이동 365초)다.

탑뷰 한 점의 장치 확인(Development 빌드, 측정이 없을 때 한다. 러너를 거치지 않고 한 줄로 확인한다): `powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 10000 -Preset P2 -Stage 8 -Extra "-LabOrbOrbitRadius=57.3","-LabOrbOrbitSpeed=2","-LabOrbHeight=6" -Duration 30 -ShotAt 20`. 스크린샷(`Saved/Screenshots/Lab/`)이 오브 위 100m에서 수직으로 내려다본 장면이어야 한다. `-Duration 60 -ShotAt 50`으로 한 번 더 찍어 오브가 화면 한가운데에 있고 지면의 배치가 궤도를 도는 만큼(2m/s × 30초 = 60m) 옮겨 갔는지 견준다. 로그의 `lab_config` 줄에 Preset 2가 찍혀야 한다.

`capture-full.ps1`은 조건 표의 `capture` 절(단계 1\~8, 이동, 1만)을 `-LabHud -LabShotAt=45 -LabDuration=40 -LabWarmup=15`로 띄워 창이 뜬 24초 뒤부터 8초를 ffmpeg(`gdigrab`, 게임 창 클라이언트 영역의 물리 화면 좌표로 `-i desktop` 영역 녹화, `__COMPAT_LAYER=HighDpiAware`)로 녹화하고 960px 15fps 움직이는 WebP(`libwebp_anim -q:v 60 -loop 0`)로 만든다(`-Format gif`면 팔레트 2패스 GIF). 화면 메시지는 `-ExecCmds=DisableAllScreenMessages`로 끈다. 결과는 git 밖 `Saved/Capture/<Batch>/<라벨>.webp`, 경로는 `Measurements/<Batch>/captures.csv`에 적는다. 인자 `-Steps 8`, `-Seconds`, `-RecordDelaySec`, `-Width`, `-Fps`, `-Quality`, `-Format`, `-Run`, `-WarmupSec`(기본 15).

러너가 쓰는 조건 이름은 `<V0|V1>-<구성>`(V0 이동, V1 정지)이다. 라벨은 `<묶음>-V0-s8-N10000-P1-r1`이다. 소형 확인 묶음(`run-spike.ps1`)의 라벨 `<묶음>-V<속도 번호>-<구성>-...`과 같은 모양이라 `summarize.py`가 둘 다 읽는다. 소형 묶음에서 속도 번호는 `-Speeds`의 순서다.

### 엔티티 풀 긴 실행 러너

`Scripts/run-pool.ps1`은 v2로 옮겼다([Worklog/V2.md](../Worklog/V2.md) "2026-11-17 엔티티 풀 긴 실행 러너"). 최종 구성(`s8`, 풀 켬)과 풀을 끈 조건(`a8pool`)을 총괄 묶음과 같은 시나리오(1만, 이동 장면, 궤도 2m/s, 카메라 P1, 워밍업 60초)로 돌리고 측정 구간만 960초로 늘린다. 라벨은 `<묶음>-V0-s8-N10000-P1-r1`, `<묶음>-V0-a8pool-N10000-P1-r1`이다. 묶음 이름은 `Pool2`처럼 `[A-Z][A-Za-z]+<숫자>` 꼴이어야 `summarize.py`가 읽는다. 규약은 [measurement.md](measurement.md) "엔티티 풀 묶음 규약"이다.

```bash
# 계획만 본다(게임을 띄우지 않는다): 실행 수, 예상 시간, 명령줄, 라벨 검사
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/run-pool.ps1 -Batch Pool2 -DryRun
# 본 측정(960초 실행 둘, 약 35분). 사용자가 시작을 요청한 뒤에 한다. 패키지 커밋에 dirty가 있으면, 다른 프로그램이 떠 있으면 시작하지 않는다
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/run-pool.ps1 -Batch Pool2
# 끊겼으면 이어 한다
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/run-pool.ps1 -Batch Pool2 -Resume
# 러너 시험(측정 60초, 창이 열린 브라우저가 있어도 시작한다. 수치는 쓰지 않는다)
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/run-pool.ps1 -Batch PoolSmoke1 -DurationSec 60 -AllowBrowsers
# 구간별 값만 다시 만든다(러너가 끝에 부른다). --bin은 구간 길이(초)
py -3 -I Scripts/pool-timeline.py --batch Pool2 --bin 60
```

| 인자 | 뜻 |
| --- | --- |
| `-Batch` | 묶음 이름(필수). `progress.log`가 있으면 `-Resume`이 아닌 한 거부한다. 같은 라벨은 다시 쓰지 않는다 |
| `-DurationSec` | 측정 구간(초). 기본 960. 메타데이터 `LabWindow`가 `60+<초>`인지 대조한다 |
| `-Conds` | 쉼표로 이은 구성 이름(`full-conditions.json`의 `configs`). 기본 `s8,a8pool` |
| `-Runs` | 구성마다 회차 수. 기본 1. 2 이상이면 짝수 회차는 구성 순서를 뒤집는다 |
| `-Resume`, `-DryRun`, `-NoFgCheck`, `-NoClock` | 이어 하기, 계획만, 앞 창 검사 끄기, CPU 클럭 기록 끄기(측정에는 앞 창 검사를 끄지 않는다) |
| `-StartupSec`, `-TimeoutFactor` | 예상 시간용 시작 시간(기본 30초), 시간 제한 배수(기본 2, 한 실행 예상 시간 기준) |
| `-MaxConsecFail` | 연속 실패 중단 횟수(기본 2, 0이면 끈다). 멈추면 `ABORT` 줄, `batch.json`의 `state`가 `aborted`, 종료 코드 2 |
| `-AllowDirty`, `-AllowBrowsers` | 시험 전용. 패키지 dirty 허용, 창이 열린 chrome·msedge·firefox 허용. 측정에는 쓰지 않는다 |

- 실패한 실행(종료 코드, CSV 없음, 앞 창이 게임이 아님, 메타데이터 대조 틀림, 시간 초과)은 `Measurements/<묶음>/failures.csv`에 적고, 묶음 끝에서 새 회차(회차 + `-Runs`)로 한 번 다시 잰다. 다시 실패하면 포기하고 종료 코드 1이다.
- 끝에서 `summarize.py`(`runs.csv`, `summary.csv`)와 `pool-timeline.py`(`timeline.csv`)를 돌린다. 동등성과 묶음 판정(`equivalence-check.py`, `batch-check.py`)은 돌지 않는다.
- `timeline.csv`의 열: `label`, `minute`, `frames`, `frame_p50`, `game_p50`, `gpu_p50`, `iskm_tracks_end`, `iskm_tracks_max`, `gpu_mem_mean`, `gpu_mem_max`, `frame_p95`(맨 끝에 더했다). 구간은 측정 구간의 누적 프레임 시간으로 60초씩 자른다. 값은 프레임 CSV의 `FrameTime`, `GameThreadTime`, `GPUTime`, `Lab/IskmTracks`, `GPUMem/LocalUsedMB`에서 나온다.
- 960초는 궤도 5.33바퀴다(한 바퀴 180초). 60초 구간마다 궤도 위상이 다르다. 두 조건은 같은 시드와 궤도라 같은 번호의 구간은 같은 위상이다.
