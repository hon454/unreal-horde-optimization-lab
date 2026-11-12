# 00 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-12 20:34\~11-13 00:39). 경위는 [작업 기록의 재측정 절](../../Docs/Worklog/Q6.md#full2-재측정2026-11-12) |
| 빌드 | Test 패키지 08e45f2, CSV 메타데이터 `labbuild=08e45f24a5e5` |
| 조건 | 2560x1440 창, 스케일러빌리티 Epic, `r.VSync=0`, `t.MaxFPS=0`, `r.ScreenPercentage=100`, `r.DynamicRes.OperationMode=0`, Lumen(`r.DynamicGlobalIlluminationMethod=1`)·VSM(`r.Shadow.Virtual.Enable=1`) 켬, `mass.FullyParallel=true`. 2종, 시드 20261106. 측정 창 60초, 세 회차 중앙값 |
| 실행 수 | 계획 178회(요약 행 54개 × 3회 + 트레이스 16회) 모두 성공, 실패 0(`Measurements/Full2/batch.json` `planned_runs_ok`, `failed_runs`) |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full2/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full2/runs.csv), [동등성 검사](../../Measurements/Full2/equivalence.csv) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full2/full-n-at-60fps.csv)(`Scripts/plot.py` `n_at_budget`가 측정 N 사이를 로그 보간) |
| 측정 PC | Ryzen 9 5950X, RTX 4090 24GB, 64GB(2666MT/s), Windows 11, NVIDIA 621.14, UE 5.8.4 소스 빌드. [pc-specs.md](../../Docs/Reference/pc-specs.md) |

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. 실행별 p50·p95는 `Scripts/summarize.py`가 60초 측정 창의 프레임 분포에서 구하고(`percentile`), 요약은 세 회차 값의 중앙값이다(`statistics.median`). 버리는 첫 실행 `Full2-L1-full-N10000-P1-r0`은 요약에 넣지 않는다.

## 본문의 수치와 출처

"(그림)"은 본문 산문이 아니라 그림에만 쓴 값이다. 조건은 따로 적지 않으면 호드 총수 1만, 오브 시점(P1)이다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열 또는 식) | 세 회차(r1 / r2 / r3) |
| --- | --- | --- | --- | --- |
| 액터 군중이 60fps를 지키는 최대 774개 | 774 | `1 L1-full` | `full-n-at-60fps.csv` `n_p95` | 느린 쪽 5% 경계 500: 11.01, 1천: 20.04 사이에서 보간 |
| 최종 구성 프레임 시간 중앙값 7.78ms | 7.783 | `L8-q4final` 10000 P1 | `frame_p50_ms` | 7.802 / 7.783 / 7.742 |
| (그림 alt) 1단계 440ms | 439.958 | `L1-full` 10000 P1 | `frame_p50_ms` | 436.39 / 440.652 / 439.958 |
| 오브 30m 안은 맵 넓이의 0.36% | 0.0036 | 계산 | (30 ÷ 500)² | |
| 그 안에 개체의 6% | 0.06 | 계산 | 30 ÷ 500. 밀도 ∝ 1/r이면 반지름당 개체 수가 같다(measurement.md "시나리오") | 측정: `near30_mean` 628.46 ÷ 10000 = 6.28%(`L8-q4final`, 오브 수평 30m) |
| 초당 62개가 죽는다 | 62.233 | `L8-q4final` 10000 P1 | `death_rate` | 범위 62.232\~62.243. 동등성 기준값 62.206(`equivalence.csv`) |
| 같은 수가 다시 생긴다 | 10000 | 같음 | `alive_min` = `alive_max` = 10000 | |
| 16.7ms | 16.667 | 계산 | 1000 ÷ 60. 최대 개체 수의 보간선은 16.7(`plot.py` `BUDGET_MS`) | |
| 3단계 GPU 7.54ms | 7.535 | `L3-vatcull` 10000 P1 | `gpu_p50_ms` | 7.45 / 7.578 / 7.535 |
| 3단계 프레임 시간 7.91ms | 7.914 | 같음 | `frame_p50_ms` | 7.833 / 7.956 / 7.914 |
| 3단계 게임 스레드 3.21ms | 3.206 | 같음 | `game_p50_ms` | 4.14 / 3.206 / 3.136 |
| 4단계 게임 스레드 7.87ms | 7.869 | `L4-mass` 10000 P1 | `game_p50_ms` | 7.869 / 7.713 / 7.923 |
| 4단계 프레임 시간 0.249ms만 는다 | +0.249 | 계산 | 8.163 − 7.914 | 4단계 프레임 8.163 / 8.018 / 8.22 |
| "가장 느린 쪽이 GPU에서 게임 스레드로 바뀌었다" | | | `bottleneck` `L3-vatcull` = `gpu`, `L4-mass` = `game` | |
| 약 7,500프레임 | 7545 | `L8-q4final` 10000 P1 | `runs.csv` `frames`(측정 창 안의 프레임 수) | 7510 / 7545 / 7596 |
| 같은 구성을 측정 끝에 다시: 0.023ms 차이 | −0.023 | 계산 | 7.760(`L8-q4final-aa`) − 7.783 | 다시 잰 값 r11 / r12 / r13: 7.823 / 7.76 / 7.755 |
| 애니메이션 예산을 빼면 1.75ms 길다 | +1.750 | 계산 | 9.533(`A8-budget`) − 7.783 | `A8-budget` 9.542 / 9.533 / 9.529 |
| 최종 구성 최대 개체 수 25,958 | 25957.9 | `8 L8-q4final` | `n_p95`. 식: exp(ln 20000 + t(ln 30000 − ln 20000)), t = (16.7 − 14.12) ÷ (18.132 − 14.12) = 0.643 | 2만·3만 행은 아래 "그림의 값" |
| "중앙값은 3만에서도 16.7ms 안" | 15.955 | `L8-q4final` 30000 P1 | `frame_p50_ms`. `full-n-at-60fps.csv` `note_p50` = `above range` | 16.326 / 15.955 / 15.825 |
| 2\~8단계는 5천\~3만, 1단계는 500\~1만에서 잰다 | 5000·10000·20000·30000, 500·1000·2000·10000 | | `full-n-at-60fps.csv` `points` 열, `Scripts/run-full.ps1` 기본 인자(`TargetN`, `MaxNList`, `ActorNList`) | |
| 주기적으로 튀는 프레임 0건 | 0 | 본 실행 162회, 트레이스 16회 | `Scripts/spikes.py`. [Q6.md](../../Docs/Worklog/Q6.md) "Full2 재측정" 끝 줄 | |
| 동등성 검사 모두 통과 | 54조건 | 요약 행 54개 모두 | `equivalence.csv` `pass` = `ok`(54행 모두). 허용: 초당 사망·30m 안 개체 수 ±3%, 살아 있는 수 = N | |
| 빼 보기 열 가지 | 10 | | `Scripts/full-conditions.json`의 `A8-*` 조건 수 | |

## 그림의 값

### 요약 그림 `ladder.svg`

README의 사다리 그림을 그대로 가져다 쓴다(`Docs/figures/readme/ladder.svg`, `Scripts/plot-readme.py`가 만든다). 호드 총수 1만, P1, 세 회차 중앙값(ms)이다.

| 단계(조건) | 프레임 시간 | 게임 스레드 | 렌더 스레드 | GPU |
| --- | --- | --- | --- | --- |
| 1 액터 군중(`L1-full`) | 439.958 | 424.871 | 30.389 | 50.227 |
| 2 Mass 로직(`L2-mass`) | 10.307 | 6.303 | 6.481 | 10.021 |
| 3 ISM + VAT(`L3-vatcull`) | 7.914 | 3.206 | 4.121 | 7.535 |
| 4 액터 하이드레이션(`L4-mass`) | 8.163 | 7.869 | 4.694 | 6.938 |
| 5 Simulation LOD(`L5-m4offpool`) | 7.654 | 7.333 | 4.601 | 6.974 |
| 6 ISKM(`L6-m5bounds`) | 7.506 | 7.344 | 3.247 | 5.356 |
| 7 Niagara 배경 무리(`L7-q2crowd`) | 7.785 | 7.628 | 3.36 | 6.096 |
| 8 품질 올리기(`L8-q4final`) | 7.783 | 7.648 | 3.536 | 5.72 |

병목(`bottleneck`, 게임 스레드·렌더 스레드·GPU 중앙값 가운데 가장 긴 것)과 프레임 시간의 세 회차다.

| 단계 | 병목 | 프레임 시간 r1 / r2 / r3 |
| --- | --- | --- |
| 1 | game | 436.39 / 440.652 / 439.958 |
| 2 | gpu | 10.185 / 10.439 / 10.307 |
| 3 | gpu | 7.833 / 7.956 / 7.914 |
| 4 | game | 8.163 / 8.018 / 8.22 |
| 5 | game | 7.646 / 7.654 / 7.695 |
| 6 | game | 7.498 / 7.506 / 7.524 |
| 7 | game | 7.741 / 7.785 / 7.803 |
| 8 | game | 7.802 / 7.783 / 7.742 |

### `judging-difference.svg`

프레임 시간 중앙값(`frame_p50_ms`, ms). 1만 P1.

| 행 | 조건 | 중앙값 | 범위 | 큰 쪽 범위와 판정 |
| --- | --- | --- | --- | --- |
| 최종 구성(8단계) | `L8-q4final` | 7.783 | 7.742\~7.802(0.060) | 기준 |
| 같은 구성을 측정 끝에 다시 | `L8-q4final-aa`(r11\~r13) | 7.760 | 7.755\~7.823(0.068) | −0.023, 0.068 → 구별되지 않음 |
| 전환 대책을 뺐을 때 | `A8-fix` | 7.766 | 7.765\~7.897(0.132) | −0.017, 0.132 → 구별되지 않음 |
| 애니메이션 예산을 뺐을 때 | `A8-budget` | 9.533 | 9.529\~9.542(0.013) | +1.750, 0.060 → 차이 있음 |

`A8-fix` 세 회차는 7.897 / 7.765 / 7.766이다. 판정 규칙은 [measurement.md](../../Docs/Guides/measurement.md) "차이 판정"이다.

### `max-entities-method.svg`

`L8-q4final`, P1, ms.

| 호드 총수 | 느린 쪽 5% 경계(`frame_p95_ms`) | 그 세 회차 | 중앙값(`frame_p50_ms`) | 그 세 회차 |
| --- | --- | --- | --- | --- |
| 5,000 | 6.931 | 6.958 / 6.885 / 6.931 | 5.75 | 5.807 / 5.732 / 5.75 |
| 10,000 | 9.421 | 9.464 / 9.421 / 9.287 | 7.783 | 7.802 / 7.783 / 7.742 |
| 20,000 | 14.12 | 14.2 / 14.12 / 13.983 | 12.436 | 12.514 / 12.436 / 12.359 |
| 30,000 | 18.132 | 18.404 / 18.132 / 17.754 | 15.955 | 16.326 / 15.955 / 15.825 |

그림의 곡선은 점 사이를 `n_at_budget`와 같은 로그 보간으로 그렸다. 교차점 25,958은 `full-n-at-60fps.csv`에서 읽는다.

### `stage-map.svg`

측정값은 없고 설정값만 그렸다.

| 그린 것 | 값 | 출처 |
| --- | --- | --- |
| 단계마다 켠 기법 | `LabTech*`, `LabCrowdFx` | `Scripts/full-conditions.json` 각 `L*` 조건의 `expect` |
| 1단계 모든 거리가 액터, 2단계 모든 거리가 정적 ISM | | `LabHud.cpp` `DrawHUD`(M0 "all distances", M1 "static mesh" 전부 인스턴스) |
| 액터 구간 30m(상한 500), 8단계 40m(상한 1,000) | `LabHydrateM`, `LabActorCap` | `full-conditions.json`, `LabConfig.h` `FLabQuality`(`:78-84`) |
| 80m, 200m 경계 | 8000cm, `LabFarM` 200 | `LabCrowdSubsystem.cpp:125` `Bands` |
| 화면 밖은 15m 안만 액터 | 1500cm | `LabCrowdSubsystem.cpp:144` `BaseLODDistance[Medium]` |
| 8단계 80m 밖 그림자 끔 | `LabFarShadow` 0 | `full-conditions.json` `L8-q4final`. 4\~7단계는 1 |
| 7단계부터 200m 밖 입자 | `LabCrowdFx` 1 | 같은 파일 `L7-q2crowd`, `L8-q4final` |

### `frame-pipeline.svg`

개념도다. 칸의 길이(게임 스레드 4, 렌더 스레드 3, GPU 7)는 지어낸 값이고 측정값이 아니다. 그림 안에 그렇게 적었다.

### 단계별 화면 `step8-L8-q4final.webp`

`Docs/figures/steps/`의 8단계 화면을 그대로 쓴다. HUD를 켠 촬영 실행에서 찍어, HUD의 숫자에는 화면 녹화 부하가 섞인다. 본문의 수치는 HUD를 끈 측정 실행의 값만 쓴다. 촬영 경위는 [Q6.md](../../Docs/Worklog/Q6.md) "촬영 준비"에 있다.

SVG 넷은 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/00-goals-and-method/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽는다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다.

## 시나리오와 측정 절차의 출처

| 본문 값 | 값 | 출처 |
| --- | --- | --- |
| 반경 500m, 재생성 링 490\~500m | 50000cm, 1000cm | `Source/HordeOptLab/LabConfig.h:140-141` `FLabScenario` |
| 3m/s | 300cm/s | `LabConfig.h:148` |
| 체력은 개체마다 다르다(60\~240) | 60, 240 | `LabConfig.h:146-147`, [measurement.md](../../Docs/Guides/measurement.md) "품질 계획에서 바뀌는 값" |
| 근접 90%, 시즈 10% | 일련번호 10의 배수가 대형 | measurement.md "확정값" 2종 행 |
| 8m 안 피해 | 반경 8m, 초당 100\~300 | measurement.md "확정값" |
| 2초마다 5개 저격 | 2.0, 5 | `LabConfig.h:149-150`. 60초에 150발(`snipes` 150) |
| 처음 배치는 오래 돌린 뒤의 분포 | 밀도 ∝ 1/r | measurement.md "시나리오" |
| 15초 뒤 60초 | 15, 60 | `LabGameMode.h:16-17` `WarmupSec`, `MeasureSec`. CSV 기록은 `LabGameMode.cpp:444` `BeginCapture`, `:502` `EndCapture` |
| 고정 시드 | 20261106 | `LabConfig.h:201` |
| 오브 뒤 15m, 높이 9m, 1분에 한 바퀴 | 1500, 900, 60초 | `LabConfig.h:154-157`(피치 −12도) |
| 탑뷰 100m | 10000cm | `LabConfig.h:158` |
| 3% 안 | ±3% | measurement.md "총괄 묶음 규약" 동등성 행, `Scripts/equivalence-check.py` |
| 세 번, 프로세스를 새로 띄운다 | 3회 | measurement.md "벤치마크 조건" 반복 행 |
| 수직 동기화·프레임 상한·동적 해상도 끔 | | CSV 메타데이터 `labcvars`(`summary.csv` `build` 열) |
| 러너가 Chrome·에디터를 보면 시작하지 않는다 | | measurement.md "총괄 묶음 규약" 실행 검사 행(`Scripts/run-full.ps1`) |
| 게임 창이 앞에 있는지 2초마다 확인 | 2초 | measurement.md "확정값" 측정 중 PC 상태 행(`Scripts/run-bench.ps1`). `summary.csv` `fg_ok`가 54행 모두 `3/3` |
| 끝에 최종 구성을 한 번 더 잰다 | 회차 11\~13 | measurement.md "총괄 묶음 규약" 반복 행 |
| HUD: 1초 평균, 16.7ms 초록, 33.3ms까지 주황, 5초 막대 | 1.0, 1000/60, 33.3, 5.0 | `Source/HordeOptLab/LabHud.cpp:19-21`(상수), `:44-47` `BarColor`, `:173-183`(1초 평균) |
| Test는 프로파일링 도구를 남긴다 | | [ADR-0001](../../Docs/Decisions/0001-test-package-for-measurement.md) 표(`CSV_PROFILER`, `UE_TRACE_ENABLED`, `DO_CHECK`, `UE_ENABLE_DEBUG_DRAWING`) |

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다. 경로는 `Engine/` 아래다.

| 사실 | 위치 |
| --- | --- |
| CSV의 `GameThreadTime`, `RenderThreadTime`, `GPUTime`은 `GGameThreadTime`, `GRenderThreadTime`, `RHIGetGPUFrameCycles()`다 | `Source/Runtime/Launch/Private/LaunchEngineLoop.cpp:1518-1520` `UpdateCoreCsvStats_EndFrame` |
| CSV의 `FrameTime`은 앞 프레임의 같은 지점부터 흐른 실제 시간이다(기본 경로) | `Source/Runtime/Core/Private/ProfilingDebugging/CsvProfiler.cpp:4383-4390` |
| 게임 스레드 시간 = 흐른 시간 − `Waits` | `Source/Runtime/Engine/Private/UnrealClient.cpp:1842-1844` `FViewport::Draw`(함수 시작 `:1789`), 프레임마다 `GameThread.Reset()` `:1858` |
| `Waits`에 들어가는 것: `Sleep`과 이벤트 대기의 `FThreadIdleStats::FScopeIdle` | `Source/Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp:1870`(`Sleep`), `:1940`(이벤트 `Wait`) |
| 렌더 스레드 시간도 쉰 시간을 뺀다 | `Source/Runtime/SlateRHIRenderer/Private/SlateRHIRenderer.cpp:1359` |
| 게임 스레드는 N−1 렌더 스레드 프레임과 맞춘다. 추가 울타리가 RHI 스레드나 스왑체인보다 너무 앞서지 않게 한다 | `Source/Runtime/RenderCore/Private/RenderingThread.cpp:2458-2478` `FFrameEndSync`, `Sync` `:2480`, `:2505-2510` |
| 기본값 `r.OneFrameThreadLag` 1, `r.GTSyncType` 0 | 같은 파일 `:2435-2450` |

### 본문에 넣은 발췌

글 전체의 엔진 코드는 17줄이다(3 + 14). 줄인 곳은 `// ...`로 적었다. 남긴 줄은 소스 그대로이고, 앞쪽 들여쓰기만 줄였다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| `FViewport::Draw`의 게임 스레드 시간 | 3 | `UnrealClient.cpp:1842-1844` |
| `FFrameEndSync`의 울타리 | 14 | `RenderingThread.cpp:2458-2462`, `// ...`(`:2463-2468` 생성자), `:2469-2473`, `// ...`(`:2474-2475` 배열 선언과 빈 줄), `:2476-2477` |

## 경위의 근거

본문 "수치를 흔든 것과 막는 장치"의 목록은 수치를 붙이지 않았다. 최종 측정 전의 묶음에서 있었던 일이고, 그 묶음의 값은 비교에 쓰지 않는다.

- 모니터를 끈 채 비워 둔 뒤 뒤쪽 조건이 느려진 일: [Worklog/M0.md](../../Docs/Worklog/M0.md) "2026-11-08 E0 재측정", measurement.md "확정값"의 측정 중 PC 상태 행.
- Chrome이 떠 있을 때의 주기적 스파이크: [Worklog/M5.md](../../Docs/Worklog/M5.md) "Chrome을 닫고 확인", measurement.md "벤치마크 조건" 묶음 검사 행.
- 게임 창이 뒤에 있을 때 게임 스레드가 길었던 일: [Worklog/M3b.md](../../Docs/Worklog/M3b.md) "페이즈 첫 실행 효과".
- 에디터를 켠 동안 실행마다 수준이 흩어진 일(러너가 에디터를 보면 멈추는 이유): Worklog/M3b.md "잡음 진단(어피니티)".
- 글 재료의 사실 확인: `Docs/post-review.md` "00-3 환경이 수치를 흔든 사례"(글을 다 쓰면 지우는 문서라, 지운 뒤에는 git 이력에서 본다).

## 확인하지 않은 것

- `Waits`에 들어가지 않는 기다림(바쁜 대기, 기다리는 동안 다른 작업을 대신 돌리는 경우)이 게임 스레드 시간에 얼마나 섞이는지는 보지 않았다. measurement.md "함정"에 같은 스레드 시간이 기다림이 찍히는 자리에 따라 달라진 사례가 있다. 본문은 "잠든 시간이 빠진다"까지만 적었다.
- "프레임 시간은 가장 느린 쪽에 맞춰진다"는 엔진의 동기화 규칙과 사다리의 값으로 설명했다. 단계마다 프레임 시간이 가장 느린 스레드보다 조금 긴 이유(4단계 8.163 대 7.869)는 따로 가르지 않았다.
- 3단계 게임 스레드는 회차 사이가 넓다(3.136\~4.14). 본문의 비교(3.21 → 7.87)는 가장 큰 회차를 써도 성립한다.
- 1단계 1만은 측정 창 60초에 150프레임이라, 느린 쪽 5% 경계가 일곱여 프레임으로 정해진다.
- 단계별 화면 `step8-L8-q4final.webp`는 다른 세션이 다시 찍는 중이다(이 글을 쓰는 동안 작업 트리에서 바뀌었다). 본문은 HUD의 배치만 설명하고 화면의 숫자를 인용하지 않는다.
- SVG는 브라우저로 열어 보지 않았다. 글자와 값은 텍스트로 대조하고 글자 폭은 어림으로 확인했다.
