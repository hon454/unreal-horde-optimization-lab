# 03 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓰고, 개수는 줄이지 않는다. 용어와 측정 방법은 [글 00의 측정 기록](../00-goals-and-method/measurements.md)과 같다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 총괄 묶음 `Full4`. 2026-11-16 14:50:12\~2026-11-17 08:52:12(`batch.json`) |
| 빌드 | Test 패키지 de0e21b(태그 `v2-full4`). CSV 메타데이터 `labbuild=de0e21bd64ac` |
| 조건 | 호드 총수 1만, 이동 장면(`V0`), 오브 시점 카메라(`P1`). 워밍업 60초 + 측정 180초, 조건마다 3회 |
| 본문 조건의 회차 | `V0-s2`, `V0-s3`, `V0-s8`, `V0-s8av`, `V0-a8sep`는 r1, r2, r3이다(`summary.csv` `runs_used`) |
| 트레이스 | `Saved/Traces/Full4trace-V0-s2-N10000-P1-r1.utrace`(병렬), `Full4trace-V0-s3-*`, `Full4trace-V0-s8-*`, `Full4traceser-V0-s2-*`, `Full4traceser-V0-s3-*`(직렬, `-LabFullyParallel=0`). git 밖이다 |
| 요약 파일 | [요약 표](../../Measurements/Full4/summary.csv), [실행별 값](../../Measurements/Full4/runs.csv), [동등성 검사](../../Measurements/Full4/equivalence.csv), [사다리 표](../../Docs/figures/full4/full-ladder.csv), [기법별 기여 표](../../Docs/figures/full4/full-ablation-moving.csv) |
| 단계 3의 정의 | `-LabStage=3`: `bAvoid` 거짓, `bSeparate` 참, `bSmoothOrient` 참(`Source/HordeOptLab/LabConfig.cpp:47-49`). 단계 2는 `bAvoid` 참, `bSeparate` 거짓, `bSmoothOrient` 거짓이다 |

## 본문의 수치와 출처

조건은 따로 적지 않으면 위 "조건"이다. "세 실행" 열은 r1 / r2 / r3 순서다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열 또는 식) | 세 실행 |
| --- | --- | --- | --- | --- |
| 단계 2 프레임 p50 34.1ms | 34.058 | `V0-s2` | `frame_p50_ms` | 34.084 / 34.058 / 33.702 |
| 단계 3 프레임 p50 12.4ms | 12.412 | `V0-s3` | `frame_p50_ms` | 12.534 / 12.363 / 12.412 |
| 단계 3 프레임 p95 14.1ms | 14.148 | `V0-s3` | `frame_p95_ms` | 14.265 / 13.975 / 14.148 |
| 16.7ms까지의 여유 2.55ms | 2.552 | 계산 | 16.7 − 14.148 | |
| 단계 2 프레임 p95 36.8(그림) | 36.785 | `V0-s2` | `frame_p95_ms` | 37.212 / 36.694 / 36.785 |
| 게임 스레드 18ms → 11.4ms | 18.027, 11.356 | `V0-s2`, `V0-s3` | `game_p50_ms` | 18.027 / 18.092 / 17.904; 11.488 / 11.312 / 11.356 |
| GPU 12.2ms → 7.07ms | 12.23, 7.069 | `V0-s2`, `V0-s3` | `gpu_p50_ms` | 12.206 / 12.247 / 12.23; 7.165 / 7.043 / 7.069 |
| 렌더 스레드 7.29 → 5.67(그림) | 7.294, 5.669 | `V0-s2`, `V0-s3` | `render_p50_ms` | 7.294 / 7.442 / 7.162; 5.594 / 5.669 / 5.696 |
| 겹친 쌍 12,442 → 219 | 12441.87, 218.72 | `V0-s2`, `V0-s3` | `overlap_mean` | 12441.87 / 12654.02 / 11354.9; 223.73 / 216.19 / 218.72 |
| 깊게 겹친 쌍 3,767 → 0(요약, 결과) | 3766.79, 0.0 | `V0-s2`, `V0-s3` | `overlap_deep_mean` | 3766.79 / 3910.4 / 3253.71; 0 / 0 / 0 |
| 액터 평균 971 → 577 | 970.75, 577.11 | `V0-s2`, `V0-s3` | `actors_mean` | 970.75 / 968.03 / 971.55; 577.11 / 577.12 / 576.31 |
| 30m 안 2,147 → 978 | 2147.28, 977.54 | `V0-s2`, `V0-s3` | `near30_mean` | 2150.10 / 2147.28 / 2138.74; 977.54 / 978.74 / 977.39 |
| 30m 안이 기준의 두 배를 넘는다 | 2.195배 | 계산 | 2147.28 ÷ 978.245(`near30_mean_ref`). `near30_mean_dev_pct` +119.5 | |
| 초당 사망 9% 적다 | −9.00% | `V0-s2` | `equivalence.csv` `death_rate_dev_pct`. 53.928 ÷ 59.261 − 1 | |
| 기준과 0.1% 안 | −0.08%, −0.07% | `V0-s3` | `death_rate_dev_pct`, `near30_mean_dev_pct` | |
| 겹친 쌍 219는 허용 범위 안 | 118.2\~472.9 | `V0-s3` | `equivalence.csv` `overlap_range`, `overlap_ok` = `ok` | |
| 엔진 회피로 바꾸면 +8.57ms | +8.572 | `V0-s8av` − `V0-s8` | 22.803 − 14.231. `full-ablation-moving.csv` `delta_p95_ms`. 큰 쪽 범위 0.553 | 23.169 / 22.616 / 22.803 |
| 겹친 쌍 234 → 11,660(그림) | 233.53, 11659.6 | `V0-s8`, `V0-s8av` | `overlap_mean` | 235.73 / 229.24 / 233.53; 12424.68 / 11516.86 / 11659.6 |
| 깊게 겹친 쌍 0 → 3,529 | 0.0, 3529.05 | `V0-s8`, `V0-s8av` | `overlap_deep_mean` | 0 / 0 / 0; 3914.76 / 3402.99 / 3529.05 |
| 밀어내기를 끄면 −0.93ms | −0.926 | `V0-a8sep` − `V0-s8` | 13.305 − 14.231. 큰 쪽 범위 0.391 | 13.434 / 13.047 / 13.305 |
| 겹친 쌍 1,644 | 1643.83 | `V0-a8sep` | `overlap_mean` | 1643.83 / 1643.89 / 1643.64 |
| 겹친 쌍이 7배 | 7.04배 | 계산 | 1643.83 ÷ 233.53 | |
| 옮긴 개체 하나당 0.67cm | 0.67 | `V0-s8` | `sep_push_cm_mean`(스탯 `Lab/SepPushMean`의 측정 창 평균) | |
| 소형끼리 깊은 겹침의 문턱 40cm | 40cm | 설정 | 반지름 합 80cm의 절반. 몸통 반지름 소형 40, 대형 94(`LabMassTraits.cpp:16-17`). 깊게 겹친 쌍의 정의 `LabOverlap.h:8,46` | |
| 한 프레임에 걸어 들어오는 거리 약 10cm | 10.2cm | 계산 | 300cm/s(`LabConfig.h:104` `MoveSpeed`) × 34.058ms(`V0-s2` `frame_p50_ms`) | |
| 이웃 후보 24개, 힘을 내는 이웃 6개 | 24, 6 | 엔진 소스 | 아래 "엔진 소스" | |
| 128개씩, 반복 2번, 1.2배 | 128, 2, 1.2 | 설정 | `LabSeparation.h:93` `BatchSize`, `:22` `Iterations`, `:34` `Skin` 0.2와 `:69` `Scale` = 1 + Skin | |
| 1만 개체가 모두 회피 순회에 든다 | | 엔진 소스, 설정 | 쿼리가 Medium·Low·Off LOD 태그를 뺀다(`MassAvoidanceProcessors.cpp:301-303`). 단계 2는 Simulation LOD Trait을 붙이지 않아 LOD 태그가 없다(`LabCrowdSubsystem.cpp:146-151`, `LabConfig.h` `UsesSimLod`는 단계 5부터) | |
| 방향 보간 사본만 바꾸면 프레임 p95는 구별되지 않는다(참고) | −0.190 | 다른 묶음 | 아래 "참고 값"의 `V2SmoothOri` | |

- "16.7ms 안에 처음 든다": 사다리에서 프레임 p95가 16.7ms보다 작은 첫 단계가 단계 3이다(`full-ladder.csv`: 단계 1 665.254, 단계 2 36.785, 단계 3 14.148).
- 기법을 끈 조건과 하위 실험의 판정은 [measurement.md](../../Docs/Guides/measurement.md) "차이 판정"이다. 두 조건 모두 차이가 큰 쪽 범위보다 크고 0.5ms 이상이라 구별된다(`full-ablation-moving.csv` `p95_distinct` = `yes`).

## 동등성 검사

1만, 이동 장면, `equivalence.csv`. 기준은 단계 3\~8의 중앙값이다(초당 사망 59.261, 30m 안 978.245).

| 조건 | 초당 사망(차이) | 30m 안(차이) | 겹친 쌍(분류, 판정) | 검사 결과 |
| --- | --- | --- | --- | --- |
| `V0-s2` | 53.928(−9.00%) | 2147.28(+119.50%) | 12441.9(`engine`, `ok`) | `report` |
| `V0-s3` | 59.215(−0.08%) | 977.54(−0.07%) | 218.7(`sep`, `ok`) | `report`(왕복 전환 4.32/s, 최소 유지 시간 없음) |
| `V0-s8` | 59.285(+0.04%) | 979.54(+0.13%) | 233.5(`sep`, `ok`) | `ok` |
| `V0-s8av` | 54.338(−8.31%) | 2066.45(+111.24%) | 11659.6(`engine`, `ok`) | `report` |
| `V0-a8sep` | 59.713(+0.76%) | 836.21(−14.52%) | 1643.8(`off`, `ok`) | `report` |

- `engine`(엔진 회피)과 `off`(겹침 방지 끔)의 겹친 쌍은 절대 범위로 보고만 한다(각각 5천\~1만 6천, 800\~3천. [measurement.md](../../Docs/Guides/measurement.md) "실패 등급" 뒤 항목). 본문의 "허용 범위"는 밀어내기 프로세서 조건의 범위 118.2\~472.9다.
- 단계 3의 `report`는 왕복 전환만의 까닭이다. 단계 3은 최소 유지 시간이 없어 왕복 전환을 검사하지 않고 값만 보고한다. 본문의 "네 항목"(초당 사망, 30m 안, 겹친 쌍, 깊게 겹친 쌍)은 모두 기준 안이다. 캡슐 빗나감은 다섯 조건 모두 0이다(`capsule_miss`).
- 깊게 겹친 쌍은 동등성 검사의 항목이 아니다. 본문과 그림은 겹친 쌍이 프레임 시간의 영향을 받는 것을 보완하려고 함께 보였다.

## 트레이스 값

트레이스 실행은 측정 60초를 기록하고 수치를 비교에 쓰지 않는다. 본문은 어디에 시간을 쓰는지만 이 값으로 적는다. 구간은 모두 트레이스 시각 70\~120초(50초)다. 내보내기는 `Scripts/trace-export.ps1`이고 결과 CSV는 저장소 밖(세션 스크래치)에 두었다.

| 본문 값 | 정밀한 값 | 트레이스 | 식 | 내보내기 |
| --- | --- | --- | --- | --- |
| 엔진 회피가 작업자 하나에서 24.7ms | 24.727 | `Full4trace-V0-s2` | `MassMovingAvoidanceProcessor` 1,523번의 길이 평균. 1,523번은 32개 스레드에 나뉘어 돌았고 두 번이 시간상 겹친 적이 없다 | `-Command TimingEvents -Extra "-threads=* -timers=*MovingAvoidance* -startTime=70 -endTime=120 -columns=ThreadId,ThreadName,TimerName,StartTime,EndTime,Depth"` |
| 한 프레임 32.8ms | 32.824 | 같은 트레이스 | `Game Frames` 1,524개의 길이 평균. `FEngineLoop::Tick` 49.999초 ÷ 1,524 = 32.808 | 같은 내보내기, 아래 `GameThread` 통계 |
| 프레임의 4분의 3 | 0.753 | 계산 | 24.727 ÷ 32.824 | |
| 게임 스레드가 기다린 시간 15.1ms | 15.066 | 같은 트레이스 | `GameThread`의 `WaitForTasks` Incl 22.960초 ÷ 1,524 | `-Command TimerStatistics -Extra "-threads=GameThread -startTime=70 -endTime=120 -columns=* -sortBy=TotalInclusiveTime"` |
| 직렬 엔진 회피 23.5ms | 23.486 | `Full4traceser-V0-s2` | `MassMovingAvoidanceProcessor` Incl 28.793초 ÷ 1,226번(Excl과 같다). 같은 구간 프레임 40.749ms(`FEngineLoop::Tick` 49.999초 ÷ 1,227) | 같은 `TimerStatistics`, `GameThread` |
| 엔진 방향 보간 0.82ms | 0.821 | `Full4traceser-V0-s2` | `MassSmoothOrientationProcessor` Incl 1.007초 ÷ 1,226 | 같다 |
| 밀어내기 프로세서 0.82ms | 0.824 | `Full4traceser-V0-s3` | `LabSeparationProcessor` Incl 3.078초 ÷ 3,737. Excl(한 스레드 몫) 0.421ms. 같은 구간 프레임 13.372ms(49.998초 ÷ 3,739) | 같은 `TimerStatistics`, `GameThread` |
| 방향 보간 병렬 사본 0.22ms | 0.223 | `Full4traceser-V0-s3` | `LabSmoothOrientationProcessor` Incl 0.833초 ÷ 3,738 | 같다 |
| 액터 동기화 2.71 → 1.89ms(본문에 쓰지 않음) | 2.710, 1.894 | 직렬 s2, s3 | `LabActorSyncProcessor` Incl 3.322초 ÷ 1,226, 7.079초 ÷ 3,737 | 같다 |

- 직렬 트레이스는 Mass 프로세서를 게임 스레드에서 차례로 돌린다. 밀어내기 프로세서 안의 `ParallelFor`는 직렬에서도 작업자로 나뉜다([Worklog/V2.md](../../Docs/Worklog/V2.md) "W4"의 "비용의 출처").
- 병렬 트레이스 `Full4trace-V0-s3`에서 밀어내기 프로세서의 벽시계 길이는 평균 1.294ms, 중앙값 1.137ms다(3,975번). 한 프레임은 평균 12.572ms다. 본문에는 쓰지 않았다.
- 예상의 근거: 병렬 트레이스에서 게임 스레드의 기다림(15.066ms)이 사라지면 프레임은 가장 느린 스레드인 게임 스레드에 맞춰진다고 보았다. 게임 스레드 시간은 측정값 18.027ms(`V0-s2`)를 썼다. 기다림이 모두 엔진 회피 때문인지는 가르지 않았다.

### 표현과 밀어내기의 순서(한계 1)

병렬 트레이스의 `TimingEvents`(`-timers=*Separation*,*Visualization*,...`, 70\~120초)에서 프레임마다 처음 시작한 이벤트를 견줬다. "앞"은 표현 프로세서가 밀어내기 프로세서가 시작하기 전에 끝난 프레임, "겹침"은 밀어내기가 끝나기 전에 시작해 밀어내기와 겹친 프레임, "뒤"는 밀어내기가 끝난 뒤 시작한 프레임이다.

| 트레이스, 프로세서 | 프레임 | 앞 | 겹침 | 뒤 |
| --- | --- | --- | --- | --- |
| `Full4trace-V0-s8`, `MassVisualizationProcessor` | 3,814 | 24(0.6%) | 347(9.1%) | 3,443(90.3%) |
| `Full4trace-V0-s8`, `LabIskmProcessor` | 3,814 | 18(0.5%) | 228(6.0%) | 3,568(93.6%) |
| `Full4trace-V0-s8`, `LabFarHitIskmProcessor` | 3,814 | 33(0.9%) | 343(9.0%) | 3,438(90.1%) |
| `Full4trace-V0-s3`, `MassVisualizationProcessor` | 3,975 | 3,656(92.0%) | 319(8.0%) | 0 |
| `Full4trace-V0-s3`, `MassUpdateISMProcessor` | 3,975 | 71(1.8%) | 3,799(95.6%) | 105(2.6%) |

- 본문 "약 10%"는 최종 구성의 엔진 표현 프로세서 첫 행의 앞 + 겹침이다(371 ÷ 3,814 = 9.7%).
- 단계 3은 다르다. 엔진 표현 프로세서가 거의 늘 이동과 밀어내기보다 먼저 돌고, ISM 갱신은 거의 늘 밀어내기와 겹친다. 단계 3에는 밀어내기 프로세서와 프로세서 단위로 서로 막는 표현 프로세서(`LabFarHitIskmProcessor`)가 없기 때문으로 본다. 본문은 최종 구성의 값만 썼다.
- 원인의 설명은 [engine-notes.md](../../Docs/Reference/engine-notes.md) 48절(서브시스템 충돌, 쿼리 밖 쓰기)과 [code-tour.md](../../Docs/Guides/code-tour.md) "순서가 정해져 있지 않은 곳"이다. 밀어내기는 위치를 모은 쿼리가 끝난 뒤에 쓴다(`LabMassProcessors.cpp:785-817`).
- "겹침"에는 밀어내기의 풀이 동안(쓰기 전) 읽은 프레임과 쓰는 중에 읽은 프레임이 섞인다. 둘을 가르는 타이머는 없다.

## 그림의 값

SVG 다섯은 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/03-separation/make-figures.py`). 측정값은 `summary.csv`와 `equivalence.csv`에서 읽고, 트레이스 값은 스크립트의 상수다(위 "트레이스 값"). 폭 800 기준으로 글자는 16px 이상이고 자기 흰 배경을 가진다.

### `summary.svg`

1만, `V0`, `P1`, ms. 막대는 중앙값, 가는 선은 세 실행의 범위다. 값은 위 "본문의 수치와 출처"의 단계 2·3 행이다.

| 행 | 단계 2 | 단계 3 |
| --- | --- | --- |
| 프레임 p95 | 36.785(36.694\~37.212) | 14.148(13.975\~14.265) |
| 프레임 p50 | 34.058(33.702\~34.084) | 12.412(12.363\~12.534) |
| 게임 스레드 | 18.027 | 11.356 |
| 렌더 스레드 | 7.294 | 5.669 |
| GPU | 12.23 | 7.069 |

그림의 "18"은 18.027을 세 자리로 줄인 18.0에서 끝의 0을 지운 값이다. 아래 글의 "여유 2.55"는 16.7 − 14.148이다.

### `step2-trace.svg`

위 "트레이스 값"의 행이다. 병렬: 한 프레임 32.824, 엔진 회피 24.727, 게임 스레드 기다림 15.066. 직렬: 단계 2 엔진 회피 23.486, 단계 3 밀어내기 프로세서 0.824, 단계 2 엔진 방향 보간 0.821, 단계 3 방향 보간 병렬 사본 0.223. 아래 글의 "1,523번"은 병렬 트레이스의 엔진 회피 실행 수다.

### `principle.svg`

측정값은 없다. 칸의 값은 설정값이다.

| 그린 것 | 값 | 출처 |
| --- | --- | --- |
| 장애물 격자의 칸 2.5m | 250cm | `Engine/Plugins/AI/MassAI/Source/MassNavigation/Private/MassNavigationSubsystem.cpp:13` |
| 이웃 후보 24개 | 6 × 4 | `MassAvoidanceProcessors.cpp:49-51`, `:53-112` `FindCloseObstacles` |
| 힘을 내는 이웃 6개 | `MaxColliders` | `MassAvoidanceProcessors.cpp:832` |
| 가속 상한으로 자른다 | `ClampVector(SteeringForce, MaxSteerAccel)` | `MassAvoidanceProcessors.cpp:1078` |
| 한 스레드에서 차례로 | `ForEachEntityChunk` | `MassAvoidanceProcessors.cpp:331` |
| 둘레 9칸, 겹친 개체 모두 | | `LabSeparation.h:140-197` `Resolve` |
| 반복마다 남은 겹침의 절반을 푼다, 반복 2번, 같은 사본을 읽는다 | `Relax` 0.5, `Iterations` 2 | `LabSeparation.h:12`, `:22`, `:23`, `:198` `Delta *= Params.Relax`, `:75-89`. 한 반복의 이동은 자기 반지름의 절반으로도 자른다(`:24` `MaxStepFrac`) |
| 128개씩 ParallelFor | `BatchSize` | `LabSeparation.h:78`, `:93` |

그림의 점 배치는 개념이다. 엔진 회피 쪽 점 26개 가운데 24개를 후보, 가까운 6개를 힘을 내는 이웃으로 칠했다.

### `same-work.svg`

1만, `V0`, `P1`. 기준 띠는 `death_rate_ref`와 `near30_mean_ref`의 ±1%, 겹친 쌍의 띠는 `overlap_range`다.

| 항목 | 단계 2 | 단계 3 | 기준 |
| --- | --- | --- | --- |
| 초당 사망 | 53.928 | 59.215 | 59.261 ± 1% |
| 오브 30m 안 개체 수 | 2147.28 | 977.54 | 978.245 ± 1% |
| 겹친 쌍 | 12441.87 | 218.72 | 118.2\~472.9 |
| 깊게 겹친 쌍 | 3766.79 | 0 | 없음 |

### `final-swaps.svg`

1만, `V0`, `P1`. 프레임 p95(ms)와 겹친 쌍. 괄호의 차이는 최종 구성과의 차이이고 판정 규칙은 "차이 판정"이다.

| 행 | 조건 | 프레임 p95(범위) | 차이 | 겹친 쌍 |
| --- | --- | --- | --- | --- |
| 최종 구성 | `V0-s8` | 14.231(14.035\~14.426) | 기준 | 233.53 |
| 엔진 회피로 바꾼 조건 | `V0-s8av` | 22.803(22.616\~23.169) | +8.572 | 11659.6 |
| 밀어내기를 끈 조건 | `V0-a8sep` | 13.305(13.047\~13.434) | −0.926 | 1643.83 |

아래 글의 "7배"는 1643.83 ÷ 233.53 = 7.04다.

### 단계 3의 화면 `step3-separation.webp`

`Docs/figures/steps/step3-separation.webp`는 촬영 실행 `Full4shot-V0-s3-N10000-P1-r1`의 움직이는 WebP 원본이다(960×540, 15fps, 8초. [captures.csv](../../Measurements/Full4/captures.csv), [V2.md](../../Docs/Worklog/V2.md) "2026-11-17 W14-3 단계 화면"). HUD를 켜고 녹화와 함께 돈 실행이라 HUD의 숫자는 측정값이 아니다. 본문은 화면의 숫자를 인용하지 않는다. 화면 설명의 "35m 안은 액터, 그 밖은 정지 메시 인스턴스"는 단계 2·3의 표현이다(글 00 `stage-map.svg`의 측정 기록).

## Insights 화면

코디네이터가 2026-11-17에 찍어 README의 "문제" 절에 넣었다(`images/insights-avoidance-separation.jpg`). 아래 표는 찍기 전에 적은 계획이고, 화면의 값은 표 아래에 있다. 찍는 법은 [Worklog/Posts.md](../../Docs/Worklog/Posts.md) "Insights 스크린샷(글 02\~07)"과 같다(창 최대화, 구간 선택, Timers 거르기, PrintWindow 창 캡처, 위아래 붙이기, 제목 줄 자르기).

| 항목 | 값 |
| --- | --- |
| 저장할 파일 | `Posts/03-separation/images/insights-avoidance-separation.jpg`(jpg, 위아래로 붙인 한 장) |
| 위 | `Saved/Traces/Full4traceser-V0-s2-N10000-P1-r1.utrace` |
| 아래 | `Saved/Traces/Full4traceser-V0-s3-N10000-P1-r1.utrace` |
| 구간 | 트레이스 시각 70\~120초(Timing 뷰의 시간 눈금자를 끌어 고른다) |
| 트랙과 목록 | Timers 목록을 `Mass`로 거르고 GameThread(CPU)만. 위는 `MassMovingAvoidanceProcessor`, 아래는 `LabSeparationProcessor`와 `LabSmoothOrientationProcessor` 행에 표시를 단다 |
| 화면에서 읽을 값 | 위: `MassMovingAvoidanceProcessor` Count 1,226, Incl 28.79초(프레임당 23.5ms). 아래: `LabSeparationProcessor` Count 3,737, Incl 3.08초(0.82ms), `LabSmoothOrientationProcessor` 3,738, 0.833초(0.22ms) |
| 넣은 뒤 | README의 주석을 풀고 캡션을 화면 값으로 맞춘다. 구간을 손으로 고르면 Count가 한두 개 다를 수 있다. 그 차이는 이 절에 적는다 |

화면의 값(두 트레이스 모두 손으로 끌어 고른 구간 50.21초):

| 트레이스 | 행 | Count | Incl | 프레임당 |
| --- | --- | --- | --- | --- |
| 위(단계 2 직렬) | `MassMovingAvoidanceProcessor` | 1,232 | 28.92초 | 23.47ms |
| 아래(단계 3 직렬) | `LabSeparationProcessor` | 3,755 | 3.09초 | 0.823ms |
| 아래(단계 3 직렬) | `LabSmoothOrientationProcessor` | 3,755 | 837.21ms | 0.223ms |

- 프레임당 값은 본문의 23.5ms, 0.82ms와 맞다. Count는 기대값보다 6과 18 많다. 구간이 0.21초 길어서다.
- Timers 목록은 위를 `Avoidance`, 아래를 `LabS`로 걸렀다. `Mass`로 거르면 행이 많아 표시한 행이 묻힌다.
- 그림은 트레이스마다 화면 왼쪽 절반과 오른쪽 절반을 확대 캡처해 붙이고, 게임 스레드 트랙 아래를 잘라 위아래로 붙인 것이다(2128폭). 창 제목 줄은 잘랐고 빨간 상자는 붙인 뒤에 그렸다.

병렬 트레이스의 "작업자 하나" 그림을 따로 싣는다면 `Full4trace-V0-s2-N10000-P1-r1`의 Timing 뷰에서 100초 근처 프레임 서넛을 확대하고, `MassMovingAvoidanceProcessor`가 작업자 트랙 하나에서 길게 도는 구간과 GameThread의 `WaitForTasks`를 표시한다(`insights-avoidance-worker.jpg`). 지금 본문은 이 화면 없이 숫자로만 적었다.

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다(2026-11-17). 줄임말 **MN** = `Engine/Plugins/AI/MassAI/Source/MassNavigation`.

| 사실 | 위치 |
| --- | --- |
| 이동 회피는 `ForEachEntityChunk`로 돈다(한 스레드) | `MN/Private/MassAvoidanceProcessors.cpp:331` |
| 쿼리가 Medium·Low·Off LOD 태그를 뺀다 | 같은 파일 `:301-303` |
| 이웃 후보 상한 24 = 6 × 4, 가까운 칸부터 채우면 멈춘다 | 같은 파일 `:49-51`, `:95-110` |
| 개체마다 후보를 찾는다 | 같은 파일 `:778-779` |
| 힘을 내는 이웃은 6개 | 같은 파일 `:832-836` |
| 힘을 조향 힘에 더하고 가속 상한으로 자른다 | 같은 파일 `:421`, `:1046-1048`, `:1078` |
| 장애물 격자 칸 2.5m | `MN/Private/MassNavigationSubsystem.cpp:13` |
| 엔진 방향 보간은 `ForEachEntityChunk`로 돈다 | `MN/Private/MassSmoothOrientationProcessors.cpp:52`, `:120` |
| 엔진 방향 보간은 개체마다 회전과 `DesiredFacing`만 쓴다 | 같은 파일 `:107`, `:112`, `:131` |
| `ParallelForEachEntityChunk`는 Chunk마다 작업을 띄운다 | [engine-notes.md](../../Docs/Reference/engine-notes.md) 49절(`Engine/Source/Runtime/MassEntity/Private/MassEntityQuery.cpp:530-647`) |
| 힘 적용과 위치 적용 | engine-notes.md 37절(`MassMovementProcessors.cpp:21-58`, `:65-128`) |

본문의 엔진 코드 발췌는 둘이고 합계 12줄이다. `MassAvoidanceProcessors.cpp`의 11줄(`// ...` 넷 포함)은 331, 389, 778, 779, 832, 1078행이다. `MassSmoothOrientationProcessors.cpp`의 1줄은 52행이다. 남긴 줄은 소스 그대로이고 앞쪽 들여쓰기만 줄였다.

## 프로젝트 코드 발췌의 원본

| 발췌 | 위치(de0e21b, 지금 `v2`와 같다) |
| --- | --- |
| `FLabSeparationSolver::Solve`의 반복 15줄 | `Source/HordeOptLab/LabSeparation.h:75-89` |
| `ULabSmoothOrientationProcessor::Execute`의 순회 2줄 | `Source/HordeOptLab/LabSmoothOrientation.cpp:270-271` |
| 밀어내기 프로세서의 순서(이동 뒤) | `Source/HordeOptLab/LabMassProcessors.cpp:758-763` |
| 단계 3부터 회피 Trait을 붙이지 않는다 | `Source/HordeOptLab/LabCrowdSubsystem.cpp:152-166`, `LabConfig.cpp:47-49` |
| 게임 모듈의 등록 | `Source/HordeOptLab/HordeOptLab.cpp:56-57`(밀어내기), `:64-72`(방향 보간) |

`git diff v2-full4 HEAD`로 보면 이 파일들 가운데 바뀐 것은 `LabConfig.cpp`의 주석 다섯 줄뿐이다.

## 참고 값(다른 묶음과 개발 빌드)

본문은 이 값 가운데 "참고" 하나만 쓰고 수치는 쓰지 않는다(구별되지 않는다). 나머지는 경위의 근거다.

| 본문의 말 | 값 | 출처 |
| --- | --- | --- |
| 방향 보간 사본만 바꾸면 프레임 p95는 구별되지 않는다(참고). "틀렸던 가설" 절의 마지막 문장도 이 값에 기댄다 | −0.190(16.648 → 16.458) | 묶음 `V2SmoothOri`(Test c5943df, 단계 8, 당시 전환 거리 40m). [autonomous-decisions.md](../../Docs/Worklog/autonomous-decisions.md) D61, [V2.md](../../Docs/Worklog/V2.md) "방향 보간 병렬 사본". 차이 −0.190은 큰 쪽 범위(`s8` 16.551\~16.950, 0.399)보다 작고 0.5ms 아래라 구별되지 않는다([measurement.md](../../Docs/Guides/measurement.md) "차이 판정"). 세 실행의 범위는 겹치지 않았지만 그것은 판정 기준이 아니다 |
| 사본의 결과가 엔진과 비트 단위로 같다 | HighRes 22,289,609번, LowRes 1,340,704번 견줌, 다름 0 | 개발 검사 `-LabSmoothOrientCheck`, V2.md "방향 보간 병렬 사본"의 "같은 결과의 근거". 단계 3·5는 "사다리 배선" 절 |
| 게임 스레드가 밀어내기 프로세서를 기다렸다(틀렸던 가설) | 프레임당 약 0.52ms, 프레임의 81% | 트레이스 `V2SepWait-V0-s8-N10000-P1-r1`(Test 0faf712). V2.md "분리 프로세서 기다림", D60 |
| 결과가 같은 더 빠른 풀이를 만들었다 | 5,101프레임, 50,435,797개체 견줌, 다름 0 | 같은 절 "후보 1의 결과". 개발 검사 `-LabSepCheck` |
| 빨라지지 않았다 | Execute 전체 1.062 → 1.179ms(칸 순 사본), 해시만 한 번: −0.024ms | 같은 절의 표(개발 빌드, 참고값). 고치기 전 두 실행의 차(1.062, 1.117)보다 작다 |
| 작업자들이 ParallelFor에 차례로 붙는다 | 14\~26개 스레드가 약 8µs 간격 | 같은 절 "원인"(트레이스 `V2SepWait`) |
| 한 스레드 격자 만들기 | 계수 정렬, 해시 표 32,768칸의 0 채우기와 누적 합 | 같은 절 "후보 1의 결과"의 결론, `LabSeparation.h:109-132` `Build` |
| 순서 미선언의 비율(D60) | 19% | `V2SepWait`(단계 8, 당시 빌드). 이 기록의 `Full4` 트레이스에서는 약 10%다(위 "표현과 밀어내기의 순서") |
| 고정 시간 간격에서는 겹친 쌍이 같다(한계 2) | `s8` 208.2, `a8hyd` 208.3 | 개발 빌드 `-benchmark -fps=78`(12.82ms), 이동 장면 90초. V2.md "2026-11-17 Full4 캡슐 빗나감 진단"의 "부차" |
| 겹친 쌍이 프레임 시간을 따라 는다(한계 2) | 예: `V0-a8hyd` 프레임 p50 6.595ms에서 51.83, `V0-s8` 12.77ms에서 233.53 | `summary.csv` `overlap_mean`. 같은 절의 표(24조건이 같은 방향) |

## 확인하지 않은 것

- 단계 2의 프레임 p50(34.058ms)이 게임 스레드 시간(18.027ms)보다 긴 까닭. 트레이스의 `WaitForTasks`(15.066ms)와 크기가 비슷하지만, 게임 스레드 시간에서 이 기다림이 빠지는 경로는 엔진 소스로 따라가지 않았다. 글 02의 몫으로 남겼다.
- 게임 스레드의 기다림이 모두 엔진 회피 때문인지. 병렬 트레이스에서 기다림의 끝과 회피의 끝을 프레임마다 맞춰 보지 않았다.
- 엔진 회피가 개체를 오브 둘레에 뭉치게 하는 기제. 30m 안 개체 수와 겹친 쌍만 쟀다. W3의 설명(뒤에서 미는 조향이 회피 힘보다 세다)은 V2.md에서도 추정이다. 본문은 기제를 쓰지 않았다.
- 단계 2→3에서 장면 변화의 몫과 회피 구현의 몫을 나누는 일. 두 몫을 가르는 조건(단계 2의 장면을 둔 채 회피 비용만 빼기)은 잴 수 없었다.
- 순서 미선언이 화면에 보이는지. 밀어내기가 옮기는 거리의 평균은 1cm 아래지만 최대값(`lab_sep` 로그의 5초 최대 40\~75cm, V2.md "분리 프로세서 기다림")에 걸린 개체가 화면 안에 있는지는 보지 않았다.
- 병렬 트레이스에서 엔진 회피가 작업자 하나에서 도는 Insights 화면은 찍지 않았다(위 "Insights 화면").
- 그림은 Edge 헤드리스로 PNG를 만들어 겹침과 잘림을 눈으로 봤다. 어두운 바탕에서는 보지 않았다(SVG가 자기 흰 배경을 가진다).
- 링크한 글 02, 04, 07의 폴더(`02-mass-engine-path`, `04-quality-by-distance`, `07-summary`)는 모두 있다.
