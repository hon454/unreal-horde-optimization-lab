# 08 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-12 20:34\~11-13 00:39). 경위는 [작업 기록의 재측정 절](../../Docs/Worklog/Q6.md#full2-재측정2026-11-12) |
| 빌드 | Test 패키지 08e45f2, CSV 메타데이터 `labbuild=08e45f24a5e5` |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1), 탑뷰(P2). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L7-q2crowd`(`-LabStage=M5 -LabVariant=q2crowd`), `L8-q4final`(`-LabStage=M5 -LabVariant=q4final`) |
| 빼 보기 | `A8-farshadow`(`-LabFarShadow=1`), `A8-fix`(`-LabHydrateFixMask=0`), `A8-hydrate`(`-LabHydrate=0`). 모두 `L8-q4final`에 인자 하나를 더한다 |
| 기대 메타데이터 | `Scripts/full-conditions.json`. `L7`과 `L8`은 단계 이름(`LabVariant`, `LabLadder`)을 빼면 `LabActorCap` 500 / 1000, `LabHydrateM` 30 / 40, `LabFarShadow` 1 / 0 셋만 다르다 |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full2/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full2/runs.csv), [빼 보기 표](../../Docs/figures/full2/full-ablation.csv), [최대 개체 수 CSV](../../Docs/figures/full2/full-n-at-60fps.csv) |
| 참고 측정 | 1280×720 묶음 `Ref1`: [요약 표](../../Measurements/Ref1/summary.csv), [실행별 값](../../Measurements/Ref1/runs.csv). 아래 "해상도를 낮춘 측정" |
| 전환 점수 | Development 참고값, [옛 총괄 묶음 때의 전환 점수](../../Measurements/Full1/transition-dev/runs.csv). 아래 "전환 점수" |
| 트레이스 | 본문과 그림에 트레이스 값을 쓰지 않았다 |

## 8단계의 구현(7단계와 다른 곳)

- 변형 값: `FLabQuality`(`Source/HordeOptLab/LabConfig.h:76-85`). `DefaultActorCap = 500`(`:78`), `DefaultHydrateM = 30.f`(`:79`), `Q4FinalActorCap = 1000`(`:82`), `Q4FinalHydrateM = 40.f`(`:84`). `MaxHydrateM = 79.f`(`:80`)는 인자로 줄 수 있는 상한이다.
- 변형이 값을 고르는 곳: `FLabConfig::Get`(`LabConfig.cpp:5`부터)의 `:196`(상한·거리), `:206`(원거리 그림자 기본값, `q4final`만 끔), `:208-213`(`-LabFarShadow=` 인자가 이긴다).
- 구간과 상한: `ULabCrowdSubsystem::BuildEntityConfig`(`LabCrowdSubsystem.cpp:123-126`). `Bands` = {0, `HydrateCm`, 8000, 20000}cm, `MaxCounts` = {`ActorCap`, 5000, 무제한, 무제한}. 화면 밖 Medium 경계 1500cm(`:144`)는 두 단계가 같다.
- 그림자 설명자 나누기: `bSplitFarShadow = bM3 && !bFarShadow`(`:168`), VAT 설명자(`:170-177`, ISKM 단계에서는 `:205`에서 비운다), ISKM 설명자(`:191-200`). 7단계는 `bFarShadow`가 참이라 설명자가 하나다(`:203`).
- ISKM 그림자 기본: `FLabIskmParams::bCastShadows = true`(`LabConfig.h:185`). 엔진 설명자의 기본은 거짓이다.
- 입자 그림자: `ALabGameMode::SetupCrowdFx`(`LabGameMode.cpp:274`)의 `:309-312`. 엔티티 구동 입자만 스위치를 따르고, 렌더러에서 꺼 둔 500m 밖 독립 입자는 켜지지 않는다(`:305-307` 주석, [engine-notes.md](../../Docs/Reference/engine-notes.md) 23절).
- 트랙 다시 쓰기: `ULabIskmProcessor::Execute`(`LabIskmProcessor.cpp:158`)의 `:253-256`. 범위가 하나면(`GetLODSignificanceRangesNum() > 1`이 거짓) 7단계와 같다.
- 같은 것: 화면 크기 Budget(2ms, `LabConfig.h:162`), 전환 대책 다섯 가지, Simulation LOD, 엔티티 풀, ISKM과 고정 바운드, 200m 밖 엔티티 구동 입자, 캡슐 콜리전 끔.

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. "(그림)"은 그림에만 쓴 값이다. 세 회차 열은 r1 / r2 / r3이다. 따로 적지 않으면 P1, 10000이다.

### 1만에서 7단계와 8단계(요약 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 7단계 프레임 시간 7.79ms | 7.785 | `L7-q2crowd` | `frame_p50_ms` | 7.741 / 7.785 / 7.803 |
| 7단계 게임 스레드 7.63ms | 7.628 | 같음 | `game_p50_ms` | 7.596 / 7.628 / 7.638 |
| (그림) 7단계 렌더 스레드 3.36ms | 3.36 | 같음 | `render_p50_ms` | 3.357 / 3.36 / 3.373 |
| 7단계 GPU 6.10ms | 6.096 | 같음 | `gpu_p50_ms` | 6.096 / 6.072 / 6.129 |
| 8단계 프레임 시간 7.78ms | 7.783 | `L8-q4final` | `frame_p50_ms` | 7.802 / 7.783 / 7.742 |
| 8단계 게임 스레드 7.65ms | 7.648 | 같음 | `game_p50_ms` | 7.667 / 7.648 / 7.609 |
| (그림) 8단계 렌더 스레드 3.54ms | 3.536 | 같음 | `render_p50_ms` | 3.536 / 3.539 / 3.536 |
| 8단계 GPU 5.72ms | 5.72 | 같음 | `gpu_p50_ms` | 5.72 / 5.715 / 5.723 |
| 병목은 두 단계 모두 게임 스레드 | `game` | 위 두 조건 | `bottleneck` | |
| (그림) 프레임 시간 −0.002ms, 범위 안 | −0.002 | 계산 | 7.783 − 7.785. 범위 0.062 / 0.060보다 작다 | |
| 게임 스레드 +0.020ms, 범위 안 | 0.020 | 계산 | 7.648 − 7.628. 범위 0.042 / 0.058보다 작다 | |
| GPU −0.376ms | −0.376 | 계산 | 5.720 − 6.096. 범위 0.057 / 0.008보다 크다 | |
| 렌더 스레드 +0.176ms | 0.176 | 계산 | 3.536 − 3.360. 범위 0.016 / 0.003보다 크다 | |
| 액터 374 → 458개 | 373.79 / 458.13 | 위 두 조건 | `actors_mean`(`Lab/Actors` 평균) | 373.79 / 373.66 / 373.83, 458.05 / 458.13 / 458.15 |
| 액터 84개 많다 | 84.34 | 계산 | 458.13 − 373.79 | |
| 드로우 콜 1,360 → 1,716 | 1360 / 1716 | 위 두 조건 | `draw_calls_p50` | 1360 / 1359 / 1362, 1715 / 1716 / 1717 |
| 대형 건너뜀 0.650% → 20.7% | 0.0065 / 0.2073 | 위 두 조건 | `large_skip_ratio` | 0.0004 / 0.0083 / 0.0065, 0.2073 / 0.2064 / 0.2133 |
| 8단계 프레임 p95 9.42ms | 9.421 | `L8-q4final` | `frame_p95_ms` | 9.464 / 9.421 / 9.287 |

`summary.csv`의 세 회차 중앙값은 열마다 따로 고른다. 그래서 `large_skip_ratio`의 중앙값 행이 프레임 시간의 중앙값 행과 다를 수 있다. 화면 안 액터(`view_actors_mean`)는 280.37 → 364.54다(본문에 쓰지 않았다).

### 7단계에 남은 것(문제 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처 | 세 회차 |
| --- | --- | --- | --- | --- |
| 7단계 프레임 p95 9.34ms | 9.336 | `L7-q2crowd` | `frame_p95_ms` | 9.309 / 9.336 / 9.411 |
| 여유 기준 13.3ms | 13.333 | 설정값 | 16.667 × 0.8. [ADR-0010](../../Docs/Decisions/0010-final-representation-bands.md) 결정 6, [autonomous-decisions.md](../../Docs/Worklog/autonomous-decisions.md) D12 | |
| 16.7ms | 16.667 | 계산 | 1000 ÷ 60 | |
| 13.3ms까지 4.00ms | 3.997 | 계산 | 13.333 − 9.336 | |
| 16.7ms까지 7.33ms | 7.331 | 계산 | 16.667 − 9.336 | |
| GPU가 게임 스레드보다 1.53ms 짧다 | 1.532 | 계산 | 7.628 − 6.096 | |
| 대형 건너뜀 0.650% | 0.0065 | `L7-q2crowd` | `large_skip_ratio` | 위 표 |
| 2만에서 30m 안 액터 비율 69.4% | 0.6937 | `L7-q2crowd` 20000 | `coverhyd_mean` | 0.6937 / 0.6939 / 0.6936 |

`coverhyd_mean`은 프레임마다 `ViewActors / (ViewActors + ViewInstHyd)`의 평균이다. 화면 안, 카메라에서 그 단계의 전환 거리 안에 있는 개체 가운데 액터의 비율이다(`Scripts/summarize.py` 107\~108행 주석, `LabIskmProcessor.cpp:316-321`). 7단계는 30m, 8단계는 40m 안을 센다. 본문은 "화면 안 30m 개체의 69.4%"로 썼다.

### 예상과 실제(원리의 예상, 결과 그림)

| 본문 값 | 정밀한 값 | 조건 | 출처 | 세 회차 |
| --- | --- | --- | --- | --- |
| 4단계 액터 단가 12.5µs | 12.499 | 계산 | (`L4-mass` − `L3-vatcull`) `game_p50_ms` ÷ `L4-mass` `actors_mean` = (7.869 − 3.206) ÷ 373.07 × 1000. 04번 글의 측정 기록과 같다 | |
| 예상 1.05ms | 1.0542 | 계산 | 84.34 × 0.012499 | |
| 실제 +0.020ms | 0.020 | 계산 | 위 표 | |
| 80m 밖 그림자의 몫 GPU 0.386ms | 0.386 | 계산 | `A8-farshadow` − `L8-q4final` `gpu_p50_ms` = 6.106 − 5.720 | `A8-farshadow` 6.106 / 6.118 / 6.081 |
| 실제 GPU −0.376ms | −0.376 | 계산 | 위 표 | |

예상은 상한으로만 읽는다. 4단계 단가는 애니메이션 예산이 거의 일하지 않던 구성(대형 건너뜀 1.29%)의 값이고, 8단계는 액터가 늘어난 만큼 30\~40m의 ISKM 인스턴스가 준다(`ism_band1` 1026.03 → 955.22는 카메라 수평 30\~80m의 ISKM 수, 참고). 실제 +0.020ms가 건너뛰기 덕인지, 인스턴스가 준 덕인지는 가르지 않았다.

### 호드 총수별 값(맞바꿈 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 액터 구간 비율 7단계 1만 98.5% | 0.9854 | `L7-q2crowd` 10000 | `coverhyd_mean` | 0.9854 / 0.9854 / 0.9853 |
| 7단계 2만 69.4% | 0.6937 | 20000 | 같음 | 위 표 |
| (그림) 7단계 3만 48.5% | 0.485 | 30000 | 같음 | 0.4846 / 0.4852 / 0.485 |
| (그림) 8단계 1만 98.9% | 0.9891 | `L8-q4final` 10000 | 같음 | 0.989 / 0.9891 / 0.9891 |
| 8단계 2만 98.6% | 0.9859 | 20000 | 같음 | 0.9858 / 0.9859 / 0.986 |
| 8단계 3만 75.8% | 0.7578 | 30000 | 같음 | 0.7574 / 0.7578 / 0.7581 |
| (그림) 7단계 건너뜀 1만 0.650% | 0.0065 | `L7-q2crowd` 10000 | `large_skip_ratio` | 위 표 |
| (그림) 7단계 2만 0.850% | 0.0085 | 20000 | 같음 | 0.0009 / 0.0088 / 0.0085 |
| (그림) 7단계 3만 0.01% | 0.0001 | 30000 | 같음 | 0.0029 / 0.0001 / 0.0 |
| 8단계 1만 20.7% | 0.2073 | `L8-q4final` 10000 | 같음 | 위 표 |
| 8단계 2만 57.3% | 0.5734 | 20000 | 같음 | 0.5698 / 0.5734 / 0.5979 |
| (그림) 8단계 3만 55.7% | 0.5571 | 30000 | 같음 | 0.555 / 0.5571 / 0.6324 |
| 7단계 2만 액터 492개 | 492.49 | `L7-q2crowd` 20000 | `actors_mean` | 492.46 / 492.51 / 492.49 |
| 8단계 2만 액터 929개 | 929.32 | `L8-q4final` 20000 | 같음 | 929.42 / 929.32 / 929.13 |
| 액터 수가 거의 두 배 | 1.887 | 계산 | 929.32 ÷ 492.49 | |
| 3만에서 8단계도 상한에 닿는다 | 987.19 | `L8-q4final` 30000 | `actors_mean`, 상한 1,000 | 987.0 / 987.19 / 987.23 |
| 다섯에 하나(그림 문장) | 0.2073 | 계산 | 1 ÷ 0.2073 = 4.82 | |
| 최대 개체 수 26,587 → 25,958 | 26587 / 25958 | `7 L7-q2crowd`, `8 L8-q4final` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준(`n_p50`)은 둘 다 30000 `above range` |

최대 개체 수는 `Scripts/plot.py`가 측정 N 사이(2만과 3만)를 로그 보간한 값이다. 그 두 점의 프레임 p95는 7단계 13.987 / 17.851, 8단계 14.12 / 18.132다. 본문은 줄이지 않고 그대로 썼다.

### 최종 구성에서 하나씩 바꾼 측정(빼 보기 그림, 본문)

기준은 `L8-q4final` 10000 P1(r1\~r3)이다. 묶음 끝 A/A(r11\~r13)는 쓰지 않았다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 그림자 켬 프레임 +0.058ms, 범위 안 | 7.841 − 7.783 | `A8-farshadow` | `frame_p50_ms` | 7.806 / 8.032 / 7.841. 범위 0.226 / 0.060보다 작다 |
| (그림) 그림자 켬 게임 스레드 +0.030ms | 7.678 − 7.648 | 같음 | `game_p50_ms` | 7.649 / 7.863 / 7.678 |
| (그림) 그림자 켬 렌더 스레드 −0.043ms | 3.493 − 3.536 | 같음 | `render_p50_ms` | 3.491 / 3.493 / 3.527 |
| 그림자 켬 GPU +0.386ms | 6.106 − 5.720 | 같음 | `gpu_p50_ms` | 위 표 |
| GPU에 1.93ms 여유 | 1.928 | 계산 | `L8-q4final` 7.648 − 5.720 | |
| (그림) 전환 대책 빼기 프레임 −0.017ms, 범위 안 | 7.766 − 7.783 | `A8-fix` | `frame_p50_ms` | 7.897 / 7.765 / 7.766. 범위 0.132 / 0.060보다 작다 |
| (그림) 전환 대책 빼기 게임 스레드 −0.013 / 렌더 −0.032 / GPU +0.014 | 7.635 / 3.504 / 5.734 | 같음 | `game_p50_ms`, `render_p50_ms`, `gpu_p50_ms` | |
| 하이드레이션 빼기 프레임 −1.59ms | 6.191 − 7.783 = −1.592 | `A8-hydrate` | `frame_p50_ms` | 6.192 / 6.191 / 6.161 |
| 게임 스레드 −3.85ms | 3.795 − 7.648 = −3.853 | 같음 | `game_p50_ms` | 3.795 / 3.845 / 3.773 |
| (그림) 렌더 스레드 −1.19ms | 2.351 − 3.536 = −1.185 | 같음 | `render_p50_ms` | 2.351 / 2.388 / 2.338 |
| GPU 5.76ms가 가장 느리다 | 5.757(+0.037) | 같음 | `gpu_p50_ms`, `bottleneck` = `gpu` | 5.757 / 5.769 / 5.731 |
| 절반 넘게 보이지 않았다 | 0.413 | 계산 | 1.592 ÷ 3.853(보인 몫 41.3%) | |
| (그림) 최종 구성 7.78 / 7.65 / 5.72ms | 7.783 / 7.648 / 5.720 | `L8-q4final` | 위 표 | |

차이 판정([measurement.md](../../Docs/Guides/measurement.md) "차이 판정"): `A8-farshadow`의 GPU 차이 0.386은 범위 0.037 / 0.008보다 크다. `A8-hydrate`의 프레임 차이는 범위 0.031 / 0.060보다 크다.

### 탑뷰(본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 7단계 탑뷰 프레임 5.26ms | 5.26 | `L7-q2crowd` 10000 P2 | `frame_p50_ms` | 5.205 / 5.26 / 5.266 |
| 8단계 탑뷰 프레임 4.92ms | 4.918 | `L8-q4final` 10000 P2 | 같음 | 4.905 / 4.925 / 4.918 |
| 탑뷰는 GPU 병목 | `gpu` | 위 두 조건 | `bottleneck`. GPU 4.806 → 4.485(−0.321) | 4.774 / 4.806 / 4.816, 4.471 / 4.486 / 4.485 |
| 카메라 높이 100m | 10000cm | 설정값 | `FLabScenario::TopDownHeight`(`LabConfig.h:158`), [measurement.md](../../Docs/Guides/measurement.md) "확정값" P2 행 | |

프레임 차이 −0.342는 범위 0.061 / 0.020보다 크다. 표현 LOD는 카메라와의 3D 거리로 정하므로 높이 100m에서는 모든 개체가 80m 밖이다([measurement.md](../../Docs/Guides/measurement.md) "품질 계획에서 바뀌는 값"의 거리 밴드 행). 그래서 탑뷰에는 액터가 없고(`actors_mean` 빈칸), 40m·상한 1,000은 뜻이 없다. 탑뷰 화면에는 입자도 없다(`LabConfig.h:277` 주석). 탑뷰의 7단계 → 8단계 차이는 80\~200m ISKM의 그림자 하나다.

### 해상도를 낮춘 측정(1280×720 그림, 본문)

[measurement.md](../../Docs/Guides/measurement.md) "참고 묶음 규약"의 묶음 `Ref1`이다. 같은 Test 패키지 08e45f2, 창 1280×720(`-ExeArgsPrefix "-ResX=1280 -ResY=720"`), 나머지 인자는 최종 측정과 같다. 최종 구성은 러너의 A/A 회차(r11\~r13)로 재서 행 이름이 `L8-q4final-aa`다. 이 묶음 안에서만 견주고 최종 측정의 값과 한 그림에 두지 않았다. 결정은 [autonomous-decisions.md](../../Docs/Worklog/autonomous-decisions.md) D21·D23.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 최종 구성 프레임 7.76ms | 7.758 | `Ref1` `L8-q4final-aa` | `frame_p50_ms` | r11 7.768 / r12 7.758 / r13 7.729 |
| 최종 구성 게임 스레드 7.60ms | 7.596 | 같음 | `game_p50_ms` | 7.616 / 7.596 / 7.572 |
| (그림) 최종 구성 렌더 스레드 3.33ms | 3.33 | 같음 | `render_p50_ms` | 3.342 / 3.318 / 3.33 |
| 최종 구성 GPU 3.05ms(약 3ms) | 3.047 | 같음 | `gpu_p50_ms` | 3.046 / 3.052 / 3.047 |
| 하이드레이션 빼기 프레임 4.03ms | 4.026 | `Ref1` `A8-hydrate` | `frame_p50_ms` | 4.026 / 4.03 / 3.982 |
| 게임 스레드 3.89ms | 3.888 | 같음 | `game_p50_ms` | 3.895 / 3.888 / 3.844 |
| (그림) 렌더 스레드 2.37ms | 2.369 | 같음 | `render_p50_ms` | 2.379 / 2.324 / 2.369 |
| GPU 3.02ms | 3.024 | 같음 | `gpu_p50_ms` | 3.037 / 3.022 / 3.024 |
| 프레임 −3.73ms | −3.732 | 계산 | 4.026 − 7.758. 범위 0.048 / 0.039보다 크다 | |
| 게임 스레드 −3.71ms | −3.708 | 계산 | 3.888 − 7.596 | |
| 모든 행의 병목이 게임 스레드 | `game` | `Ref1` 11행 | `bottleneck` | |

`Ref1`의 `A8-hydrate`는 첫 실행 r0을 러너가 버렸다(4.362ms, 쓰지 않는다). 나머지 빼 보기 행은 최종 측정과 0.2ms 안에서 같다는 것이 [STATUS.md](../../Docs/STATUS.md)의 정리다(본문에 쓰지 않았다).

### 전환 점수(본문 한 문장)

Development 빌드 참고값이다. 옛 총괄 묶음의 소스(730ff18)로, 200m 밖 입자 메시를 바꾸기 전에 쟀다. 최종 구성(40m) 1만 P1, 1280×720, 시드 셋. 절차와 점수의 뜻은 [작업 기록의 전환 점수 절](../../Docs/Worklog/Q6.md#development-전환-점수-묶음2026-11-12)에 있다.

| 조건 | 시드 20261106 | 시드 20261107 | 시드 20261108 |
| --- | --- | --- | --- |
| 대책 전부, 눈에 띈 전환 | 8 / 290(2.76%) | 14 / 275(5.09%) | 9 / 268(3.36%) |
| 대책 끔, 눈에 띈 전환 | 29 / 290(10.0%) | 31 / 275(11.3%) | 30 / 268(11.2%) |
| 배수 | 3.63 | 2.21 | 3.33 |

본문의 "두세 배"는 루트 README와 같은 표현이다. 시드별 배수는 2.21\~3.63이다.

### 설정값과 계산값

| 본문 값 | 값 | 근거 |
| --- | --- | --- |
| 액터 구간 30 → 40m, 상한 500 → 1,000 | 30 / 40, 500 / 1000 | 위 "8단계의 구현", `summary.csv` `hydrate_m`·`actor_cap` |
| 80m 밖 그림자 켬 → 끔 | `LabFarShadow` 1 / 0 | `full-conditions.json`, `LabConfig.cpp:206` |
| 예산 2ms | 2.0 | `LabConfig.h:162` `BudgetMs` |
| ISKM은 예산에 들지 않는다 | | 할당기는 `USkeletalMeshComponentBudgeted`만 등록한다(`AnimationBudgetAllocator/Public/IAnimationBudgetAllocator.h:27` `RegisterComponent`). ISKM은 `UInstancedSkinnedMeshComponent`다 |
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

엔진 코드는 11줄이다(`// ...` 포함). 남긴 줄은 소스를 그대로 옮겼다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| 설명자와 컴포넌트(엔진) | 7 | `MassSkinnedMeshRepresentationTypes.h:71`, `// ...`, `:75`, `// ...`, `:79`, `// ...`, `MassVisualizationComponent.cpp:1337` |
| 트랙 다시 쓰기 조건(엔진) | 4 | `MassVisualizationComponent.cpp:766-769` |
| `FLabQuality` | 7 | `LabConfig.h:78-79`, `// ...`(`:80-81`), `:82`, `// ...`(`:83`), `:84`. 줄 끝 주석을 뺐다 |
| `FLabConfig::Get` | 3 | `LabConfig.cpp:196`, `// ...`(`:197-205`), `:206` |
| 그림자 설명자 | 9 | `LabCrowdSubsystem.cpp:191-192`, `:194-200`(`:193` 주석을 표시 없이 뺐다) |
| 입자 그림자 | 4 | `LabGameMode.cpp:309-312` |
| 트랙 다시 쓰기(Lab) | 4 | `LabIskmProcessor.cpp:253-256` |

뺀 주석 가운데 작업 이름을 담은 것: `LabConfig.h:81` "Q4 묶음에서 고른 값", `:83` "Q4b 표현 LOD 재편", `LabConfig.cpp:205` "Q4b 원거리 그림자", `LabCrowdSubsystem.cpp:193` "Q4b". `LabConfig.h:78-79`의 줄 끝 주석("확정값 …")도 짧게 하려고 뺐다. 상수 이름 `Q4FinalActorCap`·`Q4FinalHydrateM`과 변형 이름 `Q4Final`은 코드 그대로라 남겼다.

## 경위의 근거

본문 "원리"의 "거리를 80m 구간 끝 가까이까지 늘려 보니, 멀수록 건너뜀 비율이 빠르게 늘었다"는 수치 없이 방향만 썼다. 근거는 최종 측정과 조건이 다른 옛 기록이다(액터 캡슐 콜리전 켬, 옛 배경 무리).

- 거리별 건너뜀 비율과 프레임 p95: [Worklog/Q4.md](../../Docs/Worklog/Q4.md), [ADR-0010](../../Docs/Decisions/0010-final-representation-bands.md) "맥락"·"고려한 대안".
- 80m 밖 그림자를 끈 이유와 경계 화면(Development 동작 확인): [ADR-0010](../../Docs/Decisions/0010-final-representation-bands.md) 결정 2·4, [80m 경계 그림자 끔·켬 두 장](../../Measurements/Q4/shots/q4b-boundary80m-pair.jpg).
- 그림자 끔을 유지하고 "GPU 0.39ms, 프레임 구별 안 됨"으로 쓰는 결정: [autonomous-decisions.md](../../Docs/Worklog/autonomous-decisions.md) D23.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/q-summary.svg` | 요약 그림. 7·8단계의 프레임·게임 스레드·렌더 스레드·GPU 막대(1만), 16.7ms 선, 아래 문장 | "1만에서 7단계와 8단계" |
| `images/q-headroom.svg` | 문제 그림. 7단계 프레임 p95·게임 스레드·GPU, 13.3·16.7ms 선, 남는 시간, 건너뜀 | "7단계에 남은 것" |
| `images/q-bands.svg` | 원리 도식. 두 단계의 거리 구간(0\~250m 축척), 그림자 빗금, 두 변화가 쓰는 자원 | "8단계의 구현" |
| `images/q-expected.svg` | 결과 그림. 게임 스레드 예상 1.05 / 실제 0.020, GPU 그림자 몫 −0.386 / 실제 −0.376 | "예상과 실제" |
| `images/q-tradeoff.svg` | 결과 그림. 1만·2만·3만에서 액터 구간 비율과 대형 건너뜀(7·8단계), 최대 개체 수 | "호드 총수별 값" |
| `images/q-ablation.svg` | 빼 보기 그림. 세 행의 프레임·게임 스레드·렌더 스레드·GPU 차이 | "최종 구성에서 하나씩 바꾼 측정" |
| `images/q-lowres.svg` | 1280×720 그림. 최종 구성과 하이드레이션 뺀 구성의 네 값 | "해상도를 낮춘 측정" |
| `../../Docs/figures/steps/step7-L7-q2crowd.webp`, `step8-L8-q4final.webp` | 7·8단계 화면(HUD 켬) | 아래 "화면" |

SVG 일곱 개는 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/08-quality/make-figures.py`). 측정값은 `summary.csv`, `full-n-at-60fps.csv`, `Ref1/summary.csv`에서 읽고 상수로 적은 측정값은 없다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다. 반올림은 반올림-위(`Decimal`)이고 음수는 U+2212다.

## 화면

요약의 두 화면은 측정 실행이 아니라 촬영 실행(`Pre6shot-L7-q2crowd-N10000-P1-r1`, `Pre6shot-L8-q4final-N10000-P1-r1`, 같은 패키지 08e45f2)에서 찍었다. HUD의 값에는 화면 녹화 부하가 섞여 표의 값과 다르다. 수치는 표의 값만 쓴다. 촬영 경위는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "Pre6 촬영과 풀 끔 떨림 확인"에 있다.

## 확인하지 않은 것

- 7 → 8단계에서 게임 스레드가 거의 늘지 않은 이유. 애니메이션 예산의 건너뛰기와 30\~40m ISKM 인스턴스 감소 가운데 어느 몫인지 가르지 않았다. 트레이스(`Full2trace-L7`, `-L8`)를 내보내지 않았다.
- 전환 거리(40m)와 상한(1,000)의 몫을 따로 잰 값. 최종 측정에 그 조건이 없다.
- 1만의 대형 건너뜀 비율 0.207이 거리를 고를 때의 옛 묶음 값보다 큰 이유. 옛 총괄 묶음(입자 메시를 바꾸기 전)의 같은 값은 0.003이었다([STATUS.md](../../Docs/STATUS.md)).
- 7단계 2만·3만의 건너뜀이 1% 밑인 이유. 같은 액터 수(약 490)인 4단계 2만은 49.5%다(04번 글). 건너뜀은 액터 수만으로 정해지지 않는다.
- 소형 액터의 건너뜀 비율. 대형만 센다(`LabActorSyncProcessor.cpp:355-361`).
- 건너뛴 액터가 화면에서 ISKM보다 덜 부드러운지. 나란히 견준 화면이 없다. 할당기는 건너뛴 프레임을 보간할 수 있다(`AnimationBudgetAllocatorParameters.h:58-79`).
- 렌더 스레드 +0.176ms의 내역.
- 80m 밖 그림자를 켰을 때 화면 품질이 얼마나 오르는지.
- 7·8단계 화면에서 40m 액터 구간이 실제로 보이는지. 움직이는 WebP를 열어 보지 않았다(브라우저 금지). 이미지 설명은 설정값으로만 적었다.
- SVG를 화면에 그려 보지 않았다. 글자 겹침은 좌표와 글자 폭 추정으로만 확인했다.
