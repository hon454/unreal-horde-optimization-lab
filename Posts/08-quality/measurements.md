# 08 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정 `Full3`(2026-11-13 16:52\~21:00, 178회, 실패 0). 경위는 [작업 기록의 Full3 절](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3) |
| 다시 잰 이유 | 애니메이션 예산을 2ms에서 4ms로 올렸고([ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md)), 오브의 저격 시점 경합을 고쳤다(62b6a42). 그래서 전 조건을 새 묶음으로 다시 쟀다 |
| 빌드 | Test 패키지 df41c13, CSV 메타데이터 `labbuild=df41c131c650`. 실행 라벨은 `Full3-<조건>-N<호드 총수>-<카메라>-r<회차>`(예: `Full3-L8-q4final-N10000-P1-r1`) |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1), 탑뷰(P2). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L7-q2crowd`(`-LabStage=M5 -LabVariant=q2crowd`), `L8-q4final`(`-LabStage=M5 -LabVariant=q4final`) |
| 빼 보기 | `A8-farshadow`(`-LabFarShadow=1`), `A8-fix`(`-LabHydrateFixMask=0`), `A8-hydrate`(`-LabHydrate=0`). 모두 `L8-q4final`에 인자 하나를 더한다 |
| 기대 메타데이터 | `Scripts/full-conditions.json`. `L7`과 `L8`은 단계 이름(`LabVariant`, `LabLadder`)을 빼면 `LabActorCap` 500 / 1000, `LabHydrateM` 30 / 40, `LabFarShadow` 1 / 0 셋만 다르다 |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full3/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full3/runs.csv), [빼 보기 표](../../Docs/figures/full3/full-ablation.csv), [최대 개체 수 CSV](../../Docs/figures/full3/full-n-at-60fps.csv) |
| 참고 측정 | 앞선 본문의 1280×720 값은 뺐다. 아래 "뺀 1280×720 측정" |
| 전환 점수 | Development 참고값, [옛 총괄 묶음 때의 전환 점수](../../Measurements/Full1/transition-dev/runs.csv). 아래 "전환 점수" |
| 트레이스 | 본문과 그림에 트레이스 값을 쓰지 않았다 |

## 8단계의 구현(7단계와 다른 곳)

- 변형 값: `FLabQuality`(`Source/HordeOptLab/LabConfig.h:76-85`). `DefaultActorCap = 500`(`:78`), `DefaultHydrateM = 30.f`(`:79`), `Q4FinalActorCap = 1000`(`:82`), `Q4FinalHydrateM = 40.f`(`:84`). `MaxHydrateM = 79.f`(`:80`)는 인자로 줄 수 있는 상한이다.
- 변형이 값을 고르는 곳: `FLabConfig::Get`(`LabConfig.cpp:5`부터)의 `:204`(상한·거리), `:214`(원거리 그림자 기본값, `q4final`만 끔), `:216-221`(`-LabFarShadow=` 인자가 이긴다). 줄 번호는 df41c13의 소스다.
- 구간과 상한: `ULabCrowdSubsystem::BuildEntityConfig`(`LabCrowdSubsystem.cpp:123-126`). `Bands` = {0, `HydrateCm`, 8000, 20000}cm, `MaxCounts` = {`ActorCap`, 5000, 무제한, 무제한}. 화면 밖 Medium 경계 1500cm(`:144`)는 두 단계가 같다.
- 그림자 설명자 나누기: `bSplitFarShadow = bM3 && !bFarShadow`(`:168`), VAT 설명자(`:170-177`, ISKM 단계에서는 `:205`에서 비운다), ISKM 설명자(`:191-200`). 7단계는 `bFarShadow`가 참이라 설명자가 하나다(`:203`).
- ISKM 그림자 기본: `FLabIskmParams::bCastShadows = true`(`LabConfig.h:186`). 엔진 설명자의 기본은 거짓이다.
- 입자 그림자: `ALabGameMode::SetupCrowdFx`(`LabGameMode.cpp:283`)의 `:318-321`. 엔티티 구동 입자만 스위치를 따르고, 렌더러에서 꺼 둔 500m 밖 독립 입자는 켜지지 않는다(`:314-316` 주석, [engine-notes.md](../../Docs/Reference/engine-notes.md) 23절).
- 트랙 다시 쓰기: `ULabIskmProcessor::Execute`(`LabIskmProcessor.cpp:158`)의 `:253-256`. 범위가 하나면(`GetLODSignificanceRangesNum() > 1`이 거짓) 7단계와 같다.
- 같은 것: 화면 크기 Budget(4ms, `LabConfig.h:163`), 전환 대책 다섯 가지, Simulation LOD, 엔티티 풀, ISKM과 고정 바운드, 200m 밖 엔티티 구동 입자, 캡슐 콜리전 끔.

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. "(그림)"은 그림에만 쓴 값이다. 세 회차 열은 r1 / r2 / r3이다. 따로 적지 않으면 P1, 10000이다.

### 1만에서 7단계와 8단계(요약 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 7단계 프레임 시간 8.73ms | 8.728 | `L7-q2crowd` | `frame_p50_ms` | 8.728 / 8.687 / 8.762 |
| (그림) 7단계 게임 스레드 8.57ms | 8.569 | 같음 | `game_p50_ms` | 8.569 / 8.525 / 8.589 |
| (그림) 7단계 렌더 스레드 3.36ms | 3.355 | 같음 | `render_p50_ms` | 3.401 / 3.355 / 3.352 |
| 7단계 GPU 6.06ms | 6.061 | 같음 | `gpu_p50_ms` | 6.12 / 6.025 / 6.061 |
| 8단계 프레임 시간 9.75ms | 9.746 | `L8-q4final` | `frame_p50_ms` | 9.707 / 9.763 / 9.746 |
| (그림) 8단계 게임 스레드 9.59ms | 9.592 | 같음 | `game_p50_ms` | 9.559 / 9.616 / 9.592 |
| (그림) 8단계 렌더 스레드 3.65ms | 3.647 | 같음 | `render_p50_ms` | 3.647 / 3.7 / 3.625 |
| (그림) 8단계 GPU 5.75ms | 5.752 | 같음 | `gpu_p50_ms` | 5.752 / 5.757 / 5.746 |
| 병목은 두 단계 모두 게임 스레드 | `game` | 위 두 조건 | `bottleneck` | |
| 프레임 시간 +1.02ms | 1.018 | 계산 | 9.746 − 8.728. 범위 0.075 / 0.056보다 크다 | |
| 게임 스레드 +1.02ms | 1.023 | 계산 | 9.592 − 8.569. 범위 0.064 / 0.057보다 크다 | |
| GPU −0.309ms | −0.309 | 계산 | 5.752 − 6.061. 범위 0.095 / 0.011보다 크다 | |
| 렌더 스레드 +0.292ms | 0.292 | 계산 | 3.647 − 3.355. 범위 0.049 / 0.075보다 크다 | |
| 액터 373 → 457개 | 373.41 / 457.22 | 위 두 조건 | `actors_mean`(`Lab/Actors` 평균) | 373.41 / 373.44 / 373.41, 457.29 / 457.21 / 457.22 |
| 액터 84개 많다 | 83.81 | 계산 | 457.22 − 373.41 | |
| 드로우 콜 1,360 → 1,713 | 1360 / 1713 | 위 두 조건 | `draw_calls_p50` | 1360 / 1359 / 1360, 1714 / 1713 / 1713 |
| 두 단계 모두 1만에서 애니메이션을 건너뛰지 않는다 | 0 | 위 두 조건 | `large_skipped_ratio`, `small_skipped_ratio`(보간도 0) | 8단계 소형 0.0015 / 0 / 0 |
| 예산 대상 애니메이션 2.76 → 3.37ms | 2.762 / 3.372 | 위 두 조건 | `budgeted_anim_ms`(엔진 통계 `AnimationBudget/GameThread/BudgetedAnimation`의 실행별 중앙값) | 2.775 / 2.762 / 2.723, 3.372 / 3.396 / 3.366 |
| 8단계 프레임 p95 11.5ms | 11.486 | `L8-q4final` | `frame_p95_ms` | 11.6 / 11.439 / 11.486 |

### 7단계에 남은 것(문제 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처 | 세 회차 |
| --- | --- | --- | --- | --- |
| 7단계 프레임 p95 10.6ms | 10.572 | `L7-q2crowd` | `frame_p95_ms` | 10.574 / 10.391 / 10.572 |
| 16.7ms | 16.667 | 계산 | 1000 ÷ 60 | |
| 16.7ms까지 6.09ms | 6.095 | 계산 | 16.667 − 10.572 | |
| GPU가 게임 스레드보다 2.51ms 짧다 | 2.508 | 계산 | 8.569 − 6.061 | |
| 예산 대상 애니메이션 2.76ms, 4ms까지 1.24ms | 2.762, 1.238 | `L7-q2crowd` | `budgeted_anim_ms`. 4 − 2.762 | 위 표 |
| 7단계는 애니메이션을 하나도 건너뛰지 않는다 | 0 | `L7-q2crowd` | `large_skipped_ratio`, `small_skipped_ratio` | 위 표 |
| 2만에서 30m 안 액터 비율 69.2% | 0.6916 | `L7-q2crowd` 20000 | `coverhyd_mean` | 0.6915 / 0.6917 / 0.6916 |

### 예상과 실제(원리의 예상, 결과 그림)

| 본문 값 | 정밀한 값 | 조건 | 출처 | 세 회차 |
| --- | --- | --- | --- | --- |
| 4단계 액터 단가 13.4µs | 13.361 | 계산 | (`L4-mass` − `L3-vatcull`) `game_p50_ms` ÷ `L4-mass` `actors_mean` = (9.136 − 4.156) ÷ 372.73 × 1000([04번 글의 측정 기록](../04-hydration-anim-budget/measurements.md)) | |
| 예상 1.12ms | 1.1198 | 계산 | 83.81 × 0.013361 | |
| 7단계 예산에 1.24ms가 남는다 | 1.238 | 계산 | 위 표 | |
| 실제 +1.02ms, 예상과 거의 같다 | 1.023 | 계산 | 위 표. 예상과의 차이 −0.097 | |
| 80m 밖 그림자의 몫 GPU 0.352ms | 0.352 | 계산 | `A8-farshadow` − `L8-q4final` `gpu_p50_ms` = 6.104 − 5.752 | `A8-farshadow` 6.09 / 6.149 / 6.104 |
| 실제 GPU −0.309ms | −0.309 | 계산 | 위 표 | |

예상 단가는 4단계(액터 30m, 상한 500, 1만)에서 잰 값이다. 3단계 게임 스레드의 회차 폭이 넓다(04번 글의 측정 기록). 예상은 크기를 가늠하는 데만 쓴다.

### 호드 총수별 값(맞바꿈 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 액터 구간 비율 7단계 1만 98.5% | 0.9845 | `L7-q2crowd` 10000 | `coverhyd_mean` | 0.9845 / 0.9845 / 0.9844 |
| 7단계 2만 69.2% | 0.6916 | 20000 | 같음 | 위 표 |
| (그림) 7단계 3만 48.4% | 0.4839 | 30000 | 같음 | 0.483 / 0.4839 / 0.4841 |
| (그림) 8단계 1만 98.8% | 0.9876 | `L8-q4final` 10000 | 같음 | 0.9877 / 0.9876 / 0.9876 |
| 8단계 2만 98.4% | 0.9841 | 20000 | 같음 | 0.9841 / 0.9839 / 0.9841 |
| 8단계 3만 75.6% | 0.7556 | 30000 | 같음 | 0.7552 / 0.7556 / 0.7558 |
| (그림) 7단계 소형 자세 멈춤 1만 0, 2만 0.03%, 3만 0 | 0 / 0.0003 / 0 | `L7-q2crowd` | `small_skipped_ratio` | 2만 0.0003 / 0.0004 / 0.0003, 3만 0.0008 / 0 / 0 |
| (그림) 8단계 소형 1만 0 | 0 | `L8-q4final` 10000 | 같음 | 0.0015 / 0 / 0 |
| 8단계 소형 2만 42.1% | 0.4212 | 20000 | 같음 | 0.4099 / 0.4212 / 0.4313 |
| (그림) 8단계 소형 3만 48.1% | 0.4809 | 30000 | 같음 | 0.4809 / 0.4979 / 0.4523 |
| 8단계 대형 2만 0.36% | 0.0036 | 20000 | `large_skipped_ratio` | 0.001 / 0.0036 / 0.0045 |
| 7단계 2만 액터 492개 | 491.9 | `L7-q2crowd` 20000 | `actors_mean` | 491.9 / 491.91 / 491.88 |
| 8단계 2만 액터 928개 | 928.45 | `L8-q4final` 20000 | 같음 | 928.45 / 928.79 / 928.42 |
| 8단계 2만에서 예산이 찼다 | 4.036 | `L8-q4final` 20000 | `budgeted_anim_ms`, 예산 4ms | 4.046 / 4.036 / 4.018 |
| 3만에서 8단계도 상한에 닿는다 | 985.89 | `L8-q4final` 30000 | `actors_mean`, 상한 1,000 | 985.73 / 985.89 / 986.0 |
| 최대 개체 수 약 2만 2천 → 약 2만(그림 22,033 → 20,037) | 22033 / 20037 | `7 L7-q2crowd`, `8 L8-q4final` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준(`n_p50`)은 27,984 / 24,031 |
| 8단계 2만 16.68ms, 0.02ms 남는다 | 16.681, 16.667 − 16.681 = −0.014 | `L8-q4final` 20000 | `frame_p95_ms` | 16.681 / 16.873 / 16.68 |

"0.02ms만 남는다"는 16.7ms(반올림한 기준)와 16.68ms의 차이다. 1000 ÷ 60 = 16.667ms로 계산하면 2만의 중앙값은 기준을 0.014ms 넘는다. 그래서 `plot.py`의 보간 교차점(20,037)이 2만 바로 위에 있다. 본문은 "약 2만"으로만 쓴다.

최대 개체 수는 `Scripts/plot.py`가 측정 N 사이를 로그 보간한 값이다. 2만의 프레임 p95는 7단계 15.939(범위 15.795\~16.155), 8단계 16.681(16.68\~16.873)이다. 차이 0.742가 두 범위 중 큰 쪽 0.360보다 크다. 3만은 19.127(19.095\~20.256)과 20.844(20.474\~21.121)이고, 차이 1.717이 1.161보다 크다. 그래서 두 단계의 차이는 구별된다.

건너뜀 비율 `*_skipped_ratio`는 그 프레임에 갱신도 보간도 하지 않은 액터의 비율이다(측정 구간 합의 비, `LabActorSyncProcessor.cpp:357-372`, `Scripts/summarize.py`). 본문은 "자세를 멈춘다"로 쓴다. 앞선 측정의 지표 `large_skip_ratio`는 보간한 프레임까지 셌고 소형을 세지 않았다([Worklog/Q7.md](../../Docs/Worklog/Q7.md) "건너뜀 지표"). 그 지표로 쓴 앞선 본문의 "20.7%", "57.3%"는 뺐다.

### 최종 구성에서 하나씩 바꾼 측정(빼 보기 그림, 본문)

기준은 `L8-q4final` 10000 P1(r1\~r3)이다. 묶음 끝 A/A(r11\~r13)는 쓰지 않았다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 그림자 켬 프레임 +0.127ms | 9.873 − 9.746 | `A8-farshadow` | `frame_p50_ms` | 9.873 / 9.866 / 9.953. 범위 0.087 / 0.056보다 크다 |
| 그림자 켬 게임 스레드 +0.117ms | 9.709 − 9.592 | 같음 | `game_p50_ms` | 9.709 / 9.699 / 9.793 |
| (그림) 그림자 켬 렌더 스레드 +0.010ms | 3.657 − 3.647 | 같음 | `render_p50_ms` | 3.636 / 3.657 / 3.683 |
| 그림자 켬 GPU +0.352ms | 6.104 − 5.752 | 같음 | `gpu_p50_ms` | 위 표 |
| GPU에 3.84ms 여유 | 3.840 | 계산 | `L8-q4final` 9.592 − 5.752 | |
| (그림) 전환 대책 빼기 프레임 −0.104ms, 범위 안 | 9.642 − 9.746 | `A8-fix` | `frame_p50_ms` | 9.642 / 9.938 / 9.576. 범위 0.362 / 0.056보다 작다 |
| (그림) 전환 대책 빼기 게임 스레드 −0.091 / 렌더 0 / GPU +0.012 | 9.501 / 3.647 / 5.764 | 같음 | `game_p50_ms`, `render_p50_ms`, `gpu_p50_ms` | 렌더 3.632 / 3.732 / 3.647, GPU 5.766 / 5.764 / 5.762 |
| 하이드레이션 빼기 프레임 −3.57ms | 6.174 − 9.746 = −3.572 | `A8-hydrate` | `frame_p50_ms` | 6.176 / 6.174 / 6.008 |
| 게임 스레드 −5.84ms | 3.753 − 9.592 = −5.839 | 같음 | `game_p50_ms` | 3.753 / 3.825 / 3.689 |
| (그림) 렌더 스레드 −1.30ms | 2.345 − 3.647 = −1.302 | 같음 | `render_p50_ms` | 2.345 / 2.384 / 2.327 |
| GPU 5.74ms가 가장 느리다 | 5.737(−0.015) | 같음 | `gpu_p50_ms`, `bottleneck` = `gpu` | 5.738 / 5.737 / 5.641 |
| 38.8%는 프레임 시간에 보이지 않는다 | 0.388 | 계산 | (5.839 − 3.572) ÷ 5.839 | |
| (그림) 최종 구성 9.75 / 9.59 / 5.75ms | 9.746 / 9.592 / 5.752 | `L8-q4final` | 위 표 | |

### 탑뷰(본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 7단계 탑뷰 프레임 5.25ms | 5.253 | `L7-q2crowd` 10000 P2 | `frame_p50_ms` | 5.215 / 5.253 / 5.27 |
| 8단계 탑뷰 프레임 4.83ms | 4.833 | `L8-q4final` 10000 P2 | 같음 | 4.833 / 4.808 / 4.899 |
| 탑뷰는 GPU 병목 | `gpu` | 위 두 조건 | `bottleneck`. GPU 4.779 → 4.469(−0.310) | 4.779 / 4.797 / 4.771, 4.47 / 4.439 / 4.469 |
| 카메라 높이 100m | 10000cm | 설정값 | `FLabScenario::TopDownHeight`(`LabConfig.h:158`), [measurement.md](../../Docs/Guides/measurement.md) "확정값" P2 행 | |

### 뺀 1280×720 측정

앞선 본문에는 해상도를 1280×720으로 낮춘 참고 묶음 `Ref1`의 값(하이드레이션을 빼면 프레임 −3.73, 게임 스레드 −3.71ms)이 있었다. `Ref1`은 예산 2ms와 경합 수정 전의 빌드(08e45f2)에서 쟀다. 최종 구성의 조건이 바뀌어 본문과 그림 `q-lowres.svg`에서 뺐다. 같은 주장(하이드레이션 액터의 비용 일부가 GPU에 가린다)은 최종 측정의 빼 보기 값(38.8%)으로 썼다.

### 전환 점수(본문 한 문장)

Development 빌드 참고값이다. 옛 총괄 묶음의 소스(730ff18)로, 200m 밖 입자 메시를 바꾸기 전에 쟀다. 최종 구성(40m) 1만 P1, 1280×720, 시드 셋. 절차와 점수의 뜻은 [작업 기록의 전환 점수 절](../../Docs/Worklog/Q6.md#development-전환-점수-묶음2026-11-12)에 있다.

| 조건 | 시드 20261106 | 시드 20261107 | 시드 20261108 |
| --- | --- | --- | --- |
| 대책 전부, 눈에 띈 전환 | 8 / 290(2.76%) | 14 / 275(5.09%) | 9 / 268(3.36%) |
| 대책 끔, 눈에 띈 전환 | 29 / 290(10.0%) | 31 / 275(11.3%) | 30 / 268(11.2%) |
| 배수 | 3.63 | 2.21 | 3.33 |

본문은 이 참고값의 배수와 빌드 사정을 쓰지 않고, 대책을 끄면 "눈에 띄는 전환이 늘어난다"로만 쓴다. 시드별 배수는 2.21\~3.63으로 2\~3배다. 루트 README는 "2\~3배"로 쓴다. 이 값은 화면 캡처로 낸 Development 빌드의 참고값이고, 200m 밖 입자 메시를 바꾸기 전의 빌드(730ff18)에서 쟀다.

### 설정값과 계산값

| 본문 값 | 값 | 근거 |
| --- | --- | --- |
| 액터 구간 30 → 40m, 상한 500 → 1,000 | 30 / 40, 500 / 1000 | 위 "8단계의 구현", `summary.csv` `hydrate_m`·`actor_cap` |
| 80m 밖 그림자 켬 → 끔 | `LabFarShadow` 1 / 0 | `full-conditions.json`, `LabConfig.cpp:206` |
| 예산 4ms | 4.0 | `LabConfig.h:163` `BudgetMs`, [ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md) |
| 넘으면 화면에서 작은 액터부터 건너뛴다 | | 정렬 `AnimationBudgetAllocator.cpp:327-342`, 중요도 = 바운드 반경 ÷ 카메라 거리([04번 글의 측정 기록](../04-hydration-anim-budget/measurements.md)), [engine-notes.md](../../Docs/Reference/engine-notes.md) 29절 |
| ISKM은 예산에 들지 않는다 | | 할당기는 `USkeletalMeshComponentBudgeted`만 등록한다(`AnimationBudgetAllocator/Public/IAnimationBudgetAllocator.h:27` `RegisterComponent`). ISKM은 `UInstancedSkinnedMeshComponent`다 |
| 예산이 차면 중요도가 낮은 액터부터 갱신 간격을 늘린다 | | 매 프레임 갱신 수 `AnimationBudgetAllocator.cpp:558-570`, 남은 메시의 간격 2\~`MaxTickRate`(10) `:593-607,633-643`, 정렬 `:327-342`. [engine-notes.md](../../Docs/Reference/engine-notes.md) 29절 |
| ISKM은 렌더러가 매 프레임 월드 시각으로 재생 위치를 구하고 GPU가 자세를 계산한다 | | `Engine/Source/Runtime/Renderer/Private/Skinning/AnimSequenceTransformProvider.cpp:1415`(그 프레임의 월드 시각), `:1190`(`GetTimePosition(WorldTime)`), `:1280-1305`(활성 트랙마다 샘플과 GPU 평가 헤더를 쓴다). 본문 발췌는 `:1415`, `// ...`, `:1190` |
| 예산을 올린 몫은 게임 스레드 시간이 된다 | | 할당기가 재는 시간은 게임 스레드의 틱과 완료 구간이다(`SkeletalMeshComponentBudgeted.cpp:91-157`, engine-notes.md 29절). 1만에서 늘어난 액터의 비용이 게임 스레드에 남았다(위 "예상과 실제") |
| 중요도 1\~2가 40\~80m, 2\~3이 80\~200m | | 엔진 소스 표의 "중요도" 행, `Bands`(위) |
| 액터는 메시마다 따로 그린다 | | 드로우 콜이 액터 수와 함께 는다(위 표). 04번 글의 측정 기록과 같은 설명 |

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다. 줄임말: **MR** = `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation`, **ML** = `Engine/Plugins/Runtime/MassGameplay/Source/MassLOD`. [engine-notes.md](../../Docs/Reference/engine-notes.md) 22절과 같은 줄을 다시 열어 확인했다.

| 사실 | 위치 |
| --- | --- |
| 설명자의 중요도 범위 칸(기본 High\~Max)과 그림자 칸(기본 거짓) | `MR/Public/MassSkinnedMeshRepresentationTypes.h:71,75,79`, `SetSignificanceRange` `:140-145`. 정적 메시 설명자는 `MR/Public/MassRepresentationTypes.h:97,101,105,150-155` |
| 그림자는 컴포넌트를 만들 때 한 번 넘어간다 | `MR/Private/MassVisualizationComponent.cpp:1337`(ISKM), `:259`(ISM). 컴포넌트는 설명자 해시마다 하나(`:1299-1305`, 해시에 `bCastShadows` 포함 `MassSkinnedMeshRepresentationTypes.h:124`) |
| 중요도 = LOD 번호 + 구간 안의 위치 | `ML/Public/MassLODCalculator.h:752-753`(거리 경로), `:415-416`(개수 집계 경로) |
| 범위를 넘는 프레임에는 두 컴포넌트에 넣고 다음 프레임 옛 인스턴스를 지운다 | engine-notes.md 22절 "범위를 넘는 프레임"(`MassSkinnedMeshRepresentationTypes.h:665-726`, `MassVisualizationComponent.cpp:812-827`). 이 글은 줄을 다시 열지 않았다 |
| 트랙이 이미 있으면 클립 번호가 다를 때만 다시 쓴다 | `MR/Private/MassVisualizationComponent.cpp:766-769`(갱신 경로), `:857-860`(추가 경로) |
| 트랙 제공자는 컴포넌트마다 만든다 | `MassVisualizationComponent.cpp:1323-1326`(engine-notes.md 22절) |

트랙 다시 쓰기 동작을 다룬 공개 이슈는 찾지 못했다. [references.md](../../Docs/Reference/references.md)와 [engine-notes.md](../../Docs/Reference/engine-notes.md)에도 없다. 웹 검색은 하지 않았다. 해결은 `ULabIskmProcessor::Execute`(`Source/HordeOptLab/LabIskmProcessor.cpp:253-256`)다. 확인은 개발 빌드 로그 `lab_range_check`로 했다(라벨 `Q4bdev-q4final-N10000-P1-r1`, [Worklog/Q4.md](../../Docs/Worklog/Q4.md) "Q4b 표현 LOD 재편" 확인 표 (c)). 처음 40건이 모두 안쪽으로(Low → Medium) 넘는 경우였고, 바깥으로 넘는 경우는 확인하지 않았다.

### 본문에 넣은 발췌

엔진 코드는 14줄이다(`// ...` 포함). 남긴 줄은 소스를 그대로 옮겼다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| 설명자와 컴포넌트(엔진) | 7 | `MassSkinnedMeshRepresentationTypes.h:71`, `// ...`, `:75`, `// ...`, `:79`, `// ...`, `MassVisualizationComponent.cpp:1337` |
| 트랙 다시 쓰기 조건(엔진) | 4 | `MassVisualizationComponent.cpp:766-769` |
| ISKM 재생 위치(엔진) | 3 | `Renderer/Private/Skinning/AnimSequenceTransformProvider.cpp:1415`, `// ...`, `:1190`. 두 줄의 들여쓰기 깊이가 달라 맨 앞 탭을 지웠다 |
| `FLabQuality` | 7 | `LabConfig.h:78-79`, `// ...`(`:80-81`), `:82`, `// ...`(`:83`), `:84`. 줄 끝 주석을 뺐다 |
| `FLabConfig::Get` | 3 | `LabConfig.cpp:204`, `// ...`(`:205-213`), `:214` |
| 그림자 설명자 | 9 | `LabCrowdSubsystem.cpp:191-192`, `:194-200`(`:193` 주석을 표시 없이 뺐다) |
| 입자 그림자 | 4 | `LabGameMode.cpp:318-321` |
| 트랙 다시 쓰기(Lab) | 4 | `LabIskmProcessor.cpp:253-256` |

뺀 주석 가운데 작업 이름을 담은 것: `LabConfig.h:81` "Q4 묶음에서 고른 값", `:83` "Q4b 표현 LOD 재편", `LabConfig.cpp:205` "Q4b 원거리 그림자", `LabCrowdSubsystem.cpp:193` "Q4b". `LabConfig.h:78-79`의 줄 끝 주석("확정값 …")도 짧게 하려고 뺐다. 상수 이름 `Q4FinalActorCap`·`Q4FinalHydrateM`과 변형 이름 `Q4Final`은 코드 그대로라 남겼다.

## 경위의 근거

본문 "원리"의 "거리를 80m 구간 끝 가까이까지 늘려 보면, 멀수록 건너뜀 비율이 빠르게 는다"는 수치 없이 방향만 썼다. 근거는 최종 측정과 조건이 다른 옛 기록이다(예산 2ms, 보간을 포함한 옛 건너뜀 지표, 액터 캡슐 콜리전 켬, 옛 배경 무리). 예산 4ms에서 거리를 다시 훑지는 않았다. 본문 한계의 "전환 거리를 고른 측정은 예산이 지금보다 작을 때의 것이다"가 이 사실이다.

- 거리별 건너뜀 비율과 프레임 p95: [Worklog/Q4.md](../../Docs/Worklog/Q4.md), [ADR-0010](../../Docs/Decisions/0010-final-representation-bands.md) "맥락"·"고려한 대안".
- 80m 밖 그림자를 끈 이유와 경계 화면(Development 동작 확인): [ADR-0010](../../Docs/Decisions/0010-final-representation-bands.md) 결정 2·4, [80m 경계 그림자 끔·켬 두 장](../../Measurements/Q4/shots/q4b-boundary80m-pair.jpg).
- 그림자 끔을 유지하는 결정: [autonomous-decisions.md](../../Docs/Worklog/autonomous-decisions.md) D23. 그때의 "프레임 구별 안 됨"은 이번 측정에서 +0.127ms로 구별된다. 끈 채로 두는 결정은 그대로다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/q-summary.svg` | 요약 그림. 7·8단계의 프레임·게임 스레드·렌더 스레드·GPU 막대(1만), 16.7ms 선, 아래 문장 | "1만에서 7단계와 8단계" |
| `images/q-headroom.svg` | 문제 그림. 7단계 프레임 p95·게임 스레드·GPU, 16.7ms 선, 남는 시간, 예산 대상 애니메이션과 남는 예산 | "7단계에 남은 것" |
| `images/q-bands.svg` | 원리 도식. 두 단계의 거리 구간(0\~250m 축척), 그림자 빗금, 두 변화가 쓰는 자원 | "8단계의 구현" |
| `images/q-expected.svg` | 결과 그림. 게임 스레드 예상 1.12 / 실제 1.02, GPU 그림자 몫 −0.352 / 실제 −0.309, 아래 문장에 예산 대상 애니메이션 2.76 → 3.37 | "예상과 실제" |
| `images/q-tradeoff.svg` | 결과 그림. 1만·2만·3만에서 액터 구간 비율과 소형 자세 멈춤 비율(7·8단계), 대형 2만 값, 최대 개체 수 | "호드 총수별 값" |
| `images/q-ablation.svg` | 빼 보기 그림. 세 행의 프레임·게임 스레드·렌더 스레드·GPU 차이 | "최종 구성에서 하나씩 바꾼 측정" |
| `../../Docs/figures/steps/step7-L7-q2crowd.webp`, `step8-L8-q4final.webp` | 7·8단계 화면(HUD 켬) | 아래 "화면" |

SVG 여섯 개는 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/08-quality/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽고 상수로 적은 측정값은 없다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다. 반올림은 반올림-위(`Decimal`)이고 음수는 U+2212다.

## 화면

요약의 두 화면은 측정 실행이 아니라 촬영 실행(`Full3shot-L7-q2crowd-N10000-P1-r1`, `Full3shot-L8-q4final-N10000-P1-r1`, 같은 패키지 df41c13, [촬영 색인](../../Measurements/Full3/captures.csv))에서 찍었다. HUD의 값에는 화면 녹화 부하가 섞여 표의 값과 다르다. 수치는 표의 값만 쓴다. 촬영 경위는 [Worklog/Q7.md](../../Docs/Worklog/Q7.md) "측정: Full3"에 있다.

## 확인하지 않은 것

- 7 → 8단계의 게임 스레드 +1.02ms의 내역. 늘어난 액터의 몫과 30\~40m ISKM 인스턴스 감소의 몫을 가르지 않았다. 트레이스(`Full3trace-L7`, `-L8`)를 내보내지 않았다.
- 전환 거리(40m)와 상한(1,000)의 몫을 따로 잰 값. 최종 측정에 그 조건이 없다.
- 7단계 2만의 건너뜀이 0.03%로 작은 이유. 예산 대상 애니메이션이 3.67ms로 4ms 안이다(`budgeted_anim_ms`). 8단계 2만은 4.04ms로 예산에 닿는다.
- 사망·피격 중인 액터가 얼마나 자세를 멈추는지. 상태별로 세지 않았다.
- 건너뛴 액터가 화면에서 ISKM보다 덜 부드러운지. 나란히 견준 화면이 없다. 할당기는 건너뛴 프레임을 보간할 수 있다(`AnimationBudgetAllocatorParameters.h:58-79`).
- 렌더 스레드 +0.292ms의 내역.
- 그림자를 켤 때 게임 스레드가 0.117ms 는 이유.
- 80m 밖 그림자를 켰을 때 화면 품질이 얼마나 오르는지.
- 7·8단계 화면에서 40m 액터 구간이 실제로 보이는지. 움직이는 WebP를 열어 보지 않았다(브라우저 금지). 이미지 설명은 설정값으로만 적었다.
- SVG는 `q-tradeoff.svg`만 브라우저로 그려 보았다. 나머지는 좌표와 글자 폭 추정으로만 확인했다.

## 여유 기준을 뺀 기록

- 2026-11-13: 본문과 `q-headroom.svg`에서 여유 기준 13.3ms(16.7ms의 80%)를 뺐다. 그 값은 계획에서 근거 없이 둔 어림값이었고, 8단계의 선택을 가르지 않았다(7단계 9.34ms, 8단계 9.42ms). 고르는 기준의 "프레임 시간"은 16.7ms 안으로 고쳤다. 남는 시간의 크기는 09번 글이 같은 측정의 비용(캡슐 콜리전 +2.01ms)과 견준다.
- 2026-11-13(예산 4ms 뒤): 7단계에 남는 시간은 6.09ms(16.7 − 10.6)이고, 기준은 16.7ms 하나다.
