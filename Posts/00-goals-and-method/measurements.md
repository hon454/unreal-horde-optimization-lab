# 00 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-13 16:52:49\~21:00:00). 경위는 [작업 기록의 측정 절](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3) |
| 빌드 | Test 패키지 df41c13, CSV 메타데이터 `labbuild=df41c131c650` |
| 조건이 바뀐 이유 | 애니메이션 예산 기본값을 2ms에서 4ms로 올렸고([ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md)), 오브의 저격 시점 경합을 고쳐(62b6a42) 전 조건을 다시 쟀다 |
| 조건 | 2560x1440 창, 스케일러빌리티 Epic, `r.VSync=0`, `t.MaxFPS=0`, `r.ScreenPercentage=100`, `r.DynamicRes.OperationMode=0`, Lumen(`r.DynamicGlobalIlluminationMethod=1`)·VSM(`r.Shadow.Virtual.Enable=1`) 켬, `mass.FullyParallel=true`. 2종, 시드 20261106. 측정 창 60초, 세 회차 중앙값 |
| 실행 수 | 계획 178회(요약 행 54개 × 3회 + 트레이스 16회) 모두 성공(`Measurements/Full3/batch.json` `planned_runs_ok` 178). 앞 창 확인(`fg`)을 못 채운 2회(`Full3-A8-farcull-N10000-P1-r2`, `Full3traceser-L3-vatcull-N10000-P1-r1`)는 수치를 쓰지 않고 새 회차(`r5`, `r2`)로 다시 쟀다. 그래서 `A8-farcull`의 세 회차는 r1, r3, r5다 |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full3/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full3/runs.csv), [동등성 검사](../../Measurements/Full3/equivalence.csv) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full3/full-n-at-60fps.csv)(`Scripts/plot.py` `n_at_budget`가 측정 N 사이를 로그 보간) |
| 측정 PC | Ryzen 9 5950X, RTX 4090 24GB, 64GB(2666MT/s), Windows 11, NVIDIA 621.14, UE 5.8.4 소스 빌드. [pc-specs.md](../../Docs/Reference/pc-specs.md) |

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. 실행별 p50·p95는 `Scripts/summarize.py`가 60초 측정 창의 프레임 분포에서 구하고(`percentile`), 요약은 세 회차 값의 중앙값이다(`statistics.median`). 버리는 첫 실행 `Full3-L1-full-N10000-P1-r0`은 요약에 넣지 않는다.

## 본문의 수치와 출처

"(그림)"은 본문 산문이 아니라 그림에만 쓴 값이다. 조건은 따로 적지 않으면 호드 총수 1만, 오브 시점(P1)이다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열 또는 식) | 세 회차(r1 / r2 / r3) |
| --- | --- | --- | --- | --- |
| 액터 군중이 60fps를 지키는 최대 766개 | 766 | `1 L1-full` | `full-n-at-60fps.csv` `n_p95` | 느린 쪽 5% 경계 500: 11.099, 1천: 20.206 사이에서 보간. 식: 500 × 2^t, t = (16.7 − 11.099) ÷ (20.206 − 11.099) = 0.615 |
| 최종 구성 프레임 시간 중앙값 9.75ms | 9.746 | `L8-q4final` 10000 P1 | `frame_p50_ms` | 9.707 / 9.763 / 9.746 |
| (그림 alt) 1단계 444ms | 444.279 | `L1-full` 10000 P1 | `frame_p50_ms` | 449.171 / 441.687 / 444.279 |
| 오브 30m 안은 맵 넓이의 0.36% | 0.0036 | 계산 | (30 ÷ 500)² | |
| 그 안에 개체의 6% | 0.06 | 계산 | 30 ÷ 500. 밀도 ∝ 1/r이면 반지름당 개체 수가 같다(measurement.md "시나리오") | 측정: `near30_mean` 627.97 ÷ 10000 = 6.28%(`L8-q4final`, 오브 수평 30m). 628.12 / 627.92 / 627.97 |
| 초당 62개가 죽는다 | 62.225 | `L8-q4final` 10000 P1 | `death_rate` | 62.225 / 62.233 / 62.208. 동등성 기준값 62.204(`equivalence.csv`) |
| 같은 수가 다시 생긴다 | 10000 | 같음 | `alive_min` = `alive_max` = 10000 | |
| 16.7ms | 16.667 | 계산 | 1000 ÷ 60. 최대 개체 수의 보간선은 16.7(`plot.py` `BUDGET_MS`) | |
| 3단계 GPU 7.53ms | 7.529 | `L3-vatcull` 10000 P1 | `gpu_p50_ms` | 7.529 / 7.428 / 7.547 |
| 3단계 프레임 시간 7.91ms | 7.909 | 같음 | `frame_p50_ms` | 7.909 / 7.777 / 7.927 |
| 3단계 게임 스레드 4.16ms | 4.156 | 같음 | `game_p50_ms` | 4.156 / 2.959 / 4.176 |
| 4단계 게임 스레드 9.14ms | 9.136 | `L4-mass` 10000 P1 | `game_p50_ms` | 9.136 / 9.04 / 9.267 |
| 4단계 프레임 시간 9.42ms | 9.422 | 같음 | `frame_p50_ms` | 9.422 / 9.324 / 9.568 |
| 4단계에서 게임 스레드가 4.98ms 는다 | +4.980 | 계산 | 9.136 − 4.156 | |
| 프레임 시간은 1.51ms만 는다 | +1.513 | 계산 | 9.422 − 7.909 | |
| 가장 느린 쪽이 늘어난 몫 1.61ms | +1.607 | 계산 | 4단계 게임 스레드 − 3단계 GPU = 9.136 − 7.529 | |
| "가장 느린 쪽이 GPU에서 게임 스레드로 바뀌었다" | | | `bottleneck` `L3-vatcull` = `gpu`, `L4-mass` = `game` | |
| 약 6,100프레임 | 6061 | `L8-q4final` 10000 P1 | `runs.csv` `frames`(측정 창 안의 프레임 수) | 6065 / 6055 / 6061 |
| 같은 구성을 측정 끝에 다시: 0.117ms 짧다 | −0.117 | 계산 | 9.629(`L8-q4final-aa`) − 9.746 | 다시 잰 값 r11 / r12 / r13: 9.627 / 9.629 / 9.675 |
| 두 범위 가운데 큰 쪽 0.056ms | 0.056 | 계산 | 9.763 − 9.707(`L8-q4final`). 다시 잰 값의 범위는 0.048(9.675 − 9.627) | |
| 전환 대책을 빼면 0.104ms 짧다. 범위 0.362ms | −0.104, 0.362 | 계산 | 9.642(`A8-fix`) − 9.746. 범위 9.938 − 9.576 | `A8-fix` 9.642 / 9.938 / 9.576 |
| 애니메이션 예산을 빼면 0.292ms 짧다. 범위 0.067ms | −0.292, 0.067 | 계산 | 9.454(`A8-budget`) − 9.746. 범위 9.499 − 9.432 | `A8-budget` 9.432 / 9.499 / 9.454 |
| 다른 때 다시 잰 2단계 0.114ms 짧다 | −0.114 | 계산 | 10.193 − 10.307([앞선 총괄 측정의 요약 표](../../Measurements/Full2/summary.csv) `L2-mass` 1만 P1) | 앞선 측정 10.185 / 10.439 / 10.307 |
| 3단계 0.005ms 짧다 | −0.005 | 계산 | 7.909 − 7.914(같은 표 `L3-vatcull`) | 앞선 측정 7.833 / 7.956 / 7.914 |
| 1단계 4.32ms(1.0%) 길다 | +4.321, +0.98% | 계산 | 444.279 − 439.958(같은 표 `L1-full`), ÷ 439.958 | 앞선 측정 436.39 / 440.652 / 439.958 |
| 최종 구성 최대 개체 수 20,037 | 20037.2 | `8 L8-q4final` | `n_p95`. 식: exp(ln 20000 + t(ln 30000 − ln 20000)), t = (16.7 − 16.681) ÷ (20.844 − 16.681) = 0.0046 | 2만·3만 행은 아래 "그림의 값" |
| 중앙값으로 판정하면 24,031 | 24031 | `8 L8-q4final` | `n_p50`. 같은 식, t = (16.7 − 15.011) ÷ (18.741 − 15.011) = 0.453 | 2만 15.011, 3만 18.741 |
| 2\~8단계는 5천\~3만, 1단계는 500\~1만에서 잰다 | 5000·10000·20000·30000, 500·1000·2000·10000 | | `full-n-at-60fps.csv` `points` 열, `Scripts/run-full.ps1` 기본 인자(`TargetN`, `MaxNList`, `ActorNList`) | |
| 주기적으로 튀는 프레임 0건 | 0 | 본 실행 162회, 트레이스 16회 | `Scripts/spikes.py`. [Q7.md](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3) "Full3 끝(성공)" 줄 | |
| 동등성 검사 모두 통과 | 54조건 | 요약 행 54개 모두 | `equivalence.csv` `pass` = `ok`(54행 모두). 허용: 초당 사망·30m 안 개체 수 ±3%, 살아 있는 수 = N | |
| 빼 보기 열 가지 | 10 | | `Scripts/full-conditions.json`의 `A8-*` 조건 수 | |

## 그림의 값

### 요약 그림 `ladder.svg`

README의 사다리 그림을 그대로 가져다 쓴다(`Docs/figures/readme/ladder.svg`, `Scripts/plot-readme.py`가 만든다). 호드 총수 1만, P1, 세 회차 중앙값(ms)이다.

| 단계(조건) | 프레임 시간 | 게임 스레드 | 렌더 스레드 | GPU |
| --- | --- | --- | --- | --- |
| 1 액터 군중(`L1-full`) | 444.279 | 429.438 | 29.797 | 50.168 |
| 2 Mass 로직(`L2-mass`) | 10.193 | 6.24 | 6.396 | 9.903 |
| 3 ISM + VAT(`L3-vatcull`) | 7.909 | 4.156 | 4.122 | 7.529 |
| 4 액터 하이드레이션(`L4-mass`) | 9.422 | 9.136 | 4.817 | 6.926 |
| 5 Simulation LOD(`L5-m4offpool`) | 8.57 | 8.253 | 4.813 | 6.972 |
| 6 ISKM(`L6-m5bounds`) | 8.452 | 8.292 | 3.247 | 5.373 |
| 7 Niagara 배경 무리(`L7-q2crowd`) | 8.728 | 8.569 | 3.355 | 6.061 |
| 8 품질 올리기(`L8-q4final`) | 9.746 | 9.592 | 3.647 | 5.752 |

병목(`bottleneck`, 게임 스레드·렌더 스레드·GPU 중앙값 가운데 가장 긴 것)과 프레임 시간의 세 회차다.

| 단계 | 병목 | 프레임 시간 r1 / r2 / r3 |
| --- | --- | --- |
| 1 | game | 449.171 / 441.687 / 444.279 |
| 2 | gpu | 9.896 / 10.193 / 10.647 |
| 3 | gpu | 7.909 / 7.777 / 7.927 |
| 4 | game | 9.422 / 9.324 / 9.568 |
| 5 | game | 8.632 / 8.57 / 8.526 |
| 6 | game | 8.452 / 8.459 / 8.417 |
| 7 | game | 8.728 / 8.687 / 8.762 |
| 8 | game | 9.707 / 9.763 / 9.746 |

### `judging-difference.svg`

프레임 시간 중앙값(`frame_p50_ms`, ms). 1만 P1.

| 행 | 조건 | 중앙값 | 범위 | 큰 쪽 범위와 판정 |
| --- | --- | --- | --- | --- |
| 최종 구성(8단계) | `L8-q4final` | 9.746 | 9.707\~9.763(0.056) | 기준 |
| 같은 구성을 측정 끝에 다시 | `L8-q4final-aa`(r11\~r13) | 9.629 | 9.627\~9.675(0.048) | −0.117, 0.056 → 차이 있음 |
| 전환 대책을 뺐을 때 | `A8-fix` | 9.642 | 9.576\~9.938(0.362) | −0.104, 0.362 → 구별되지 않음 |
| 애니메이션 예산을 뺐을 때 | `A8-budget` | 9.454 | 9.432\~9.499(0.067) | −0.292, 0.067 → 차이 있음 |

`A8-fix` 세 회차는 9.642 / 9.938 / 9.576이다. 판정 규칙은 [measurement.md](../../Docs/Guides/measurement.md) "차이 판정"이다. 그림의 판정은 스크립트가 규칙대로 계산한다. 이전 묶음에서는 같은 구성을 다시 잰 값이 범위 안이었으나 이번에는 범위 밖이다. 원인은 확인하지 않았다. 예산을 뺀 행의 부호도 바뀌었다(이전 묶음 +1.750). 호드 1만에서 예산 4ms는 프레임 시간을 줄이지 않는다.

### `max-entities-method.svg`

`L8-q4final`, P1, ms.

| 호드 총수 | 느린 쪽 5% 경계(`frame_p95_ms`) | 그 세 회차 | 중앙값(`frame_p50_ms`) | 그 세 회차 |
| --- | --- | --- | --- | --- |
| 5,000 | 6.735 | 6.618 / 6.735 / 6.813 | 5.578 | 5.501 / 5.578 / 5.671 |
| 10,000 | 11.486 | 11.6 / 11.439 / 11.486 | 9.746 | 9.707 / 9.763 / 9.746 |
| 20,000 | 16.681 | 16.681 / 16.873 / 16.68 | 15.011 | 14.942 / 15.146 / 15.011 |
| 30,000 | 20.844 | 21.121 / 20.844 / 20.474 | 18.741 | 18.902 / 18.741 / 18.458 |

그림의 곡선은 점 사이를 `n_at_budget`와 같은 로그 보간으로 그렸다. 교차점 20,037은 `full-n-at-60fps.csv`에서 읽는다.

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

`Docs/figures/steps/`의 8단계 화면을 그대로 쓴다. 촬영 실행 `Full3shot-L8-q4final-N10000-P1-r1`([captures.csv](../../Measurements/Full3/captures.csv))에서 HUD를 켜고 찍어, HUD의 숫자에는 화면 녹화 부하가 섞인다. 본문의 수치는 HUD를 끈 측정 실행의 값만 쓴다. 촬영 경위는 [Q7.md](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3)의 "촬영 끝" 줄에 있다.

SVG 넷은 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/00-goals-and-method/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽는다. `judging-difference.svg`의 판정은 스크립트가 차이 판정 규칙으로 계산한다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다.

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

- 같은 구성을 다시 잰 값이 달라진 원인(1단계 +4.32ms, 측정 끝 재측정 −0.117ms). 앞선 총괄 측정과는 빌드, 측정일, 예산 기본값, 오브 수정이 모두 다르다. 2·3단계는 예산을 쓰지 않아 구성은 같지만, 한 변수만 다른 비교는 아니다.
- `Waits`에 들어가지 않는 기다림(바쁜 대기, 기다리는 동안 다른 작업을 대신 돌리는 경우)이 게임 스레드 시간에 얼마나 섞이는지는 보지 않았다. measurement.md "함정"에 같은 스레드 시간이 기다림이 찍히는 자리에 따라 달라진 사례가 있다. 본문은 "잠든 시간이 빠진다"까지만 적었다.
- "프레임 시간은 가장 느린 쪽에 맞춰진다"는 엔진의 동기화 규칙과 사다리의 값으로 설명했다. 단계마다 프레임 시간이 가장 느린 스레드보다 조금 긴 이유(4단계 9.422 대 9.136)는 따로 가르지 않았다.
- 3단계 게임 스레드는 회차 사이가 넓다(2.959\~4.176). 본문의 비교(4.16 → 9.14)는 가장 작은 회차 2.959를 써도 늘어난다.
- 1단계 1만은 측정 창 60초에 150프레임이라, 느린 쪽 5% 경계가 일곱여 프레임으로 정해진다.
- 단계별 화면 `step8-L8-q4final.webp`는 같은 묶음의 촬영(`Full3shot-*`)이다. 본문은 HUD의 배치만 설명하고 화면의 숫자를 인용하지 않는다. 작업 기록에 적힌 그 화면의 HUD 프레임 시간(12.9ms)은 녹화 부하가 섞여 측정값(9.746)보다 높다.
- SVG는 브라우저로 열어 보지 않았다. 글자와 값은 텍스트로 대조하고 글자 폭은 어림으로 확인했다.
