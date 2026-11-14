# 05 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓰고, 개수는 줄이지 않는다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정 `Full3`(2026-11-13 16:52:49\~21:00:00, 178회, 실패 0). 경위는 [작업 기록의 Full3 절](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3) |
| 빌드 | Test 패키지 df41c13, CSV 메타데이터 `labbuild=df41c131c650` |
| 조건을 바꾼 이유 | 애니메이션 예산을 2ms에서 4ms로 올렸고([ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md)), 오브의 저격 시점(`SnipeView`) 경합을 고쳤다(62b6a42). 그래서 전 조건을 앞 묶음 `Full2`(08e45f2)에서 `Full3`으로 다시 쟀다 |
| 16분 실행 | 앞 묶음 `Full2`의 패키지(Test 08e45f2, `labbuild=08e45f24a5e5`, 예산 2ms)로 2026-11-13 00:44\~01:17에 돈 엔티티 풀 묶음 `Pool1`. `Full3`에서 다시 돌리지 않았다. 경위는 [Q6.md](../../Docs/Worklog/Q6.md#full2-재측정2026-11-12)의 "엔티티 풀 Pool1", 규약은 [measurement.md](../../Docs/Guides/measurement.md) "엔티티 풀 묶음 규약". 본문은 `Pool1`의 값끼리만 견준다 |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1), 참고로 탑뷰(P2). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L4-mass`(`-LabStage=M3 -LabVariant=mass`), `L5-m4offpool`(`-LabStage=M5 -LabVariant=m4offpool`), 빼 보기 `L8-q4final`, `A8-simlod`(`-LabSimLod=0`), `A8-pool`(`-LabEntityPool=0`). 인자와 기대 메타데이터는 `Scripts/full-conditions.json` |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full3/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full3/runs.csv), [16분 실행의 1분 구간 값](../../Measurements/Pool1/timeline.csv), [16분 실행의 전체 값](../../Measurements/Pool1/summary.csv) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full3/full-n-at-60fps.csv)(`Scripts/plot.py`가 측정 N 사이를 로그 보간) |
| 트레이스 | 본문 "결과"의 트레이스 문단 값은 아래 "트레이스 값"이다. `Full3` 빌드(df41c13)의 직렬 트레이스 한 회씩이다 |

### 4단계와 5단계의 차이(코드)

`full-conditions.json`의 기대값에서 `L4` → `L5`로 바뀌는 것은 `LabPool` 0 → 1, `LabSimLod` `-` → `80/200m tick=0/0.50/1.00 avoid=0 offmove=1`, `LabTechPool`·`LabTechSimLod` 0 → 1뿐이다. 나머지(`LabActorCap` 500, `LabHydrateM` 30, `LabFarShadow` 1, `LabTechIskm` 0 등)는 같다.

- 단계 M5에서 `m4offpool`은 VAT 그대로다: `FLabConfig::UsesIskm`(`Source/HordeOptLab/LabConfig.cpp:324-327`)이 `M4OffPool`이면 거짓. `FLabGameModule::StartupModule`(`HordeOptLab.cpp:37-40`)이 `ULabVatISMProcessor`를 켠 채 둔다.
- Simulation LOD: `UsesSimLod()`(`LabConfig.h:369`, M4 이상)가 참이라 `ULabSimulationLODTrait`을 붙인다(`LabCrowdSubsystem.cpp:66-71`). 값은 `LabMassTraits.cpp:39-69`. `SimLod()`(`LabConfig.cpp:303-322`)가 `M4OffPool`과 M5 단계에서 `bOffMove = true`. 경계 80m·200m는 `FLabSimLod::MediumDistance`·`LowDistance`(`LabConfig.h:126-127`), 간격 0.5초·1.0초는 `TickMedium`·`TickLow`(`:128-129`).
- 청크 필터: `LabUseVariableTick`(`LabMassProcessors.cpp:29-33`)을 접근(`:88-93`)과 범위 피해(`ULabOrbContactProcessor`, `:385-390`)가 부른다. 둘 다 `LabRunAfterSimLod`(`:41-47`)로 LOD 그룹 뒤에 돈다. 범위 피해는 누적 델타를 쓴다(`:431`, `:452`). 저격(`ULabOrbSnipeProcessor`)과 상태(`ULabMinionStateProcessor`, `:649-652` 주석)는 필터를 걸지 않는다.
- Off 이동: `HordeOptLab.cpp:44-48`이 `UMassOffLODNavigationProcessor`의 자동 등록을 끄고(`!bSimLod`) `ULabOffMoveProcessor`를 켠다(`bOffMove`). 관찰자 `ULabOffMoveEnterObserver`(`LabMassProcessors.cpp:303-363`)가 Off에 드는 순간의 시각을 맞춘다. 본문은 관찰자를 적지 않았다.
- 엔티티 풀: `bEntityPool`(`LabConfig.cpp:130-138`)이 `M4OffPool`이면 참. 경로는 `ULabMinionStateProcessor::Execute`(`LabMassProcessors.cpp:720-746`), `ULabCrowdSubsystem::RecycleAndRespawn`(`LabCrowdSubsystem.cpp:332-370`), `SpawnEntities`(`:247-303`), `ReinitPooledEntity`(`:316-330`), 세워 두는 곳 `LabPoolParkLocation`(`:313`, (1e6, 1e6, −1e5)cm).

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. 세 회차 열은 `Full3/runs.csv`의 r1 / r2 / r3이다(`L8-q4final`의 r11\~r13은 다른 비교용 실행이라 요약에 들지 않는다). "(그림)"은 본문 산문이 아니라 그림에만 쓴 값이다.

### 1만 P1의 네 값(요약 그림, 문제·결과 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 4단계 프레임 시간 9.42ms | 9.422 | `L4-mass` 10000 P1 | `frame_p50_ms` | 9.422 / 9.324 / 9.568 |
| 4단계 게임 스레드 9.14ms | 9.136 | 같음 | `game_p50_ms` | 9.136 / 9.04 / 9.267 |
| 4단계 렌더 스레드 4.82ms | 4.817 | 같음 | `render_p50_ms` | 4.802 / 4.817 / 4.965 |
| 4단계 GPU 6.93ms | 6.926 | 같음 | `gpu_p50_ms` | 6.926 / 6.838 / 6.957 |
| 5단계 프레임 시간 8.57ms | 8.57 | `L5-m4offpool` 10000 P1 | `frame_p50_ms` | 8.632 / 8.57 / 8.526 |
| 5단계 게임 스레드 8.25ms | 8.253 | 같음 | `game_p50_ms` | 8.351 / 8.253 / 8.23 |
| 5단계 렌더 스레드 4.81ms(그림) | 4.813 | 같음 | `render_p50_ms` | 4.813 / 5.935 / 4.654 |
| 5단계 GPU 6.97ms | 6.972 | 같음 | `gpu_p50_ms` | 6.972 / 6.986 / 6.844 |
| "두 단계 모두 게임 스레드가 가장 느리다" | | 위 두 조건 | `bottleneck` = `game` | 세 회차 모두 `game` |
| 프레임 시간 −0.852ms | −0.852 | 계산 | 8.570 − 9.422. 범위 0.244 / 0.106보다 크다 | |
| 게임 스레드 −0.883ms | −0.883 | 계산 | 8.253 − 9.136. 범위 0.227 / 0.121보다 크다 | |
| GPU "세 번 잰 범위 안" | +0.046 | 계산 | 6.972 − 6.926. 범위 0.119 / 0.142 안 | |

5단계 렌더 스레드의 r2(5.935)는 다른 두 회차보다 1.1ms 길다. 중앙값은 4.813이고 병목은 세 회차 모두 `game`이라 본문의 판정은 바뀌지 않는다.

### 4단계의 호드 총수별 값(문제 그림)

| 그림 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 게임 스레드 5천 4.30 | 4.3 | `L4-mass` 5000 P1 | `game_p50_ms` | 4.279 / 4.456 / 4.3 |
| 렌더 스레드 5천 3.41 | 3.408 | 같음 | `render_p50_ms` | 3.408 / 3.478 / 3.39 |
| GPU 5천 5.70 | 5.697 | 같음 | `gpu_p50_ms` | 5.697 / 5.743 / 5.649 |
| 게임 스레드 1만 9.14 | 9.136 | 10000 P1 | `game_p50_ms` | 위 표 |
| 렌더 스레드 1만 4.82 | 4.817 | 같음 | `render_p50_ms` | 위 표 |
| GPU 1만 6.93 | 6.926 | 같음 | `gpu_p50_ms` | 위 표 |
| 게임 스레드 2만 14.7 | 14.653 | 20000 P1 | `game_p50_ms` | 14.454 / 14.765 / 14.653 |
| 렌더 스레드 2만 7.38 | 7.375 | 같음 | `render_p50_ms` | 7.375 / 7.566 / 7.215 |
| GPU 2만 9.75 | 9.754 | 같음 | `gpu_p50_ms` | 9.768 / 9.754 / 9.696 |
| 게임 스레드 3만 18.2(본문 18.2ms) | 18.249 | 30000 P1 | `game_p50_ms` | 18.249 / 18.269 / 18.039 |
| 렌더 스레드 3만 9.43 | 9.425 | 같음 | `render_p50_ms` | 9.425 / 9.487 / 9.083 |
| GPU 3만 12.3 | 12.315 | 같음 | `gpu_p50_ms` | 12.323 / 12.253 / 12.315 |
| "5천은 GPU, 1만부터 게임 스레드" | | 네 행 | `bottleneck` 5천 `gpu`, 1만·2만·3만 `game` | |

### 호드 총수별 프레임 시간(결과 그림, 본문)

| 본문 값 | 정밀한 값(4단계 / 5단계) | 조건 | 출처(열) | 세 회차(4단계 ; 5단계) |
| --- | --- | --- | --- | --- |
| (그림) 5천 6.08 / 6.17, 차이 +0.09 | 6.077 / 6.169, +0.092 | 5000 P1 | `frame_p50_ms` | 6.077 / 6.193 / 6.03 ; 6.23 / 6.072 / 6.169 |
| (그림) 1만 9.42 / 8.57, 차이 −0.85 | 9.422 / 8.57, −0.852 | 10000 P1 | 같음 | 위 표 |
| 2만 차이 1.49ms | 15.153 / 13.668, −1.485 | 20000 P1 | 같음 | 14.967 / 15.257 / 15.153 ; 13.63 / 13.832 / 13.668 |
| 3만 차이 2.01ms | 18.942 / 16.931, −2.011 | 30000 P1 | 같음 | 18.942 / 18.979 / 18.755 ; 16.723 / 16.931 / 17.162 |
| "5천에서는 차이가 없다" | +0.092 | 계산 | 범위 0.163 / 0.158 안. 병목은 두 단계 모두 `gpu` | |
| 4단계 최대 개체 수 약 2만 | 19952 | `4 L4-mass` | `full-n-at-60fps.csv` `n_p95` | 프레임 p95: 1만 10.981, 2만 16.72 |
| 5단계 최대 개체 수 약 2만 3천 | 22857 | `5 L5-m4offpool` | 같음 | 프레임 p95: 2만 15.575, 3만 18.991 |

2만·3만의 차이도 두 범위(2만 0.290 / 0.202, 3만 0.224 / 0.439)보다 크다. 앞 측정(`Full2`)에서 5단계는 3만에서도 한계에 닿지 않았다(3만 p95 16.298). `Full3`에서는 3만 p95가 18.991이라 한계가 생겼다. 그래서 본문의 요약과 결과 문장을 최대 개체 수 비교로 고쳤다. 본문의 개체 수는 두 자리로 줄였다(19,952 → 약 2만, 22,857 → 약 2만 3천).

### Simulation LOD의 개체 수와 실제 주기(원리 그림, 본문)

`L5-m4offpool` 10000 P1. 개체 수는 접근 프로세서가 센 걷는 개체의 프레임 평균이다(`Lab/SimTick*`·`Lab/SimBand*`, `LabMassProcessors.cpp:142-166`). 실제 주기 = 평균 프레임 시간 × 밴드 전체 개체 수 ÷ 그 프레임에 돈 개체 수(`Scripts/summarize.py:177-178, 211-212`).

| 본문 값 | 정밀한 값 | 출처(열) | 세 회차 |
| --- | --- | --- | --- |
| High 1,458(그림) | 1458.4 | `sim_band_high`(= `sim_tick_high`, 매 프레임) | 1458.39 / 1458.4 / 1458.42 |
| Medium 2,411(그림) | 2410.76 | `sim_band_medium` | 2410.76 / 2410.8 / 2410.27 |
| 200m 밖 5,997 | 5996.91 | `sim_band_off` | 5996.91 / 5996.85 / 5997.4 |
| 걷는 개체 9,866 | 9866.07 | 계산 | 1458.4 + 2410.76 + 5996.91 |
| 엔진 이동·조향이 처리하는 개체 3,869, 39% | 3869.16, 39.22% | 계산 | 1458.4 + 2410.76; ÷ 9866.07 |
| Medium 설정 0.5초, 실제 0.113초 | 0.5 / 0.1129 | `cfg_tick_medium` / `real_period_medium` | 0.1129 / 0.1125 / 0.1131 |
| Off 설정 1초, 실제 0.328초 | 1.0 / 0.3281 | `cfg_tick_off` / `real_period_off` | 0.3325 / 0.3272 / 0.3281 |
| Off 개체가 한 번에 옮겨지는 거리 약 0.984m | 0.9843 | 계산 | 0.3281초 × 3m/s(`FLabScenario::MoveSpeed`, `LabConfig.h:148`). 실제 주기의 평균으로 셈한 값이다 |
| 80m·200m 경계, 0.5·1초 간격 | 8000 / 20000cm, 0.5 / 1.0초 | 코드 | 위 "4단계와 5단계의 차이" |
| 맵 가장자리 고리 490\~500m(그림) | 49000\~50000cm | 코드 | `FLabScenario::MapRadius`, `RespawnRingWidth`(`LabConfig.h:140-141`) |
| 개체의 60%가 200m 밖 | 0.6 | 계산 | [measurement.md](../../Docs/Guides/measurement.md) "확정값" 맵 반경 행(구간 비율 = 구간 폭 ÷ R = 300 ÷ 500) |

"9,866"과 "200m 밖 5,997"은 5단계에서 센 값이다. 4단계에는 이 스탯이 없다. 4단계의 걷는 개체도 같은 시나리오라 같은 수로 보았다(동등성 검사가 사망 수와 오브 근처 개체 수를 같게 확인한다, measurement.md "총괄 묶음 규약").

### 빼 보기(빼 보기 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 최종 구성 9.75(그림) | 9.746 | `L8-q4final` 10000 P1 | `frame_p50_ms` | 9.707 / 9.763 / 9.746 |
| Simulation LOD를 빼면 10.5(그림), +0.754ms | 10.5, +0.754 | `A8-simlod` 10000 P1 | 같음, 계산 10.500 − 9.746 | 10.5 / 10.638 / 10.49 |
| 엔티티 풀을 빼면 9.68(그림), 0.070ms 차이(−0.070), 범위 안 | 9.676, −0.070 | `A8-pool` 10000 P1 | 같음, 계산 9.676 − 9.746 | 9.575 / 9.762 / 9.676 |
| 판정 | 범위 0.056(`L8`), 0.148(`A8-simlod`), 0.187(`A8-pool`) | 계산 | 0.754 > 0.148, 0.070 < 0.187 | |
| "병목은 세 구성 모두 게임 스레드"(그림) | `game` | 세 조건 | `bottleneck`. 게임 스레드 p50 9.592 / 10.361 / 9.533 | |

앞 측정에서는 Simulation LOD를 빼면 +1.044ms, 풀을 빼면 +0.055ms였다. 방향과 판정(Simulation LOD는 범위 밖, 풀은 범위 안)은 같다. 풀을 뺀 행은 이번에 최종 구성보다 짧지만 범위 안이라, 본문은 "0.070ms 차이"로 적고 그림은 −0.07로 적었다.

### 16분 실행(엔티티 풀 그림, 본문)

`Measurements/Pool1/timeline.csv`. 행 라벨: 풀 켬 `Pool1-L8-q4final-N10000-P1-r1`, 풀 끔 `Pool1-A8-pool-N10000-P1-r1`. 각 1회라 범위가 없다. 열: 트랙 `iskm_tracks_end`, GPU 메모리 `gpu_mem_mean`(MB, 1분 구간 평균), 프레임 `frame_p50`.

| 본문 값 | 정밀한 값 | 실행 | 출처 |
| --- | --- | --- | --- |
| 풀 켬 트랙 3분 약 2만, 그 뒤 20,053에서 멈춤 | 3분 19,853, 4분 20,018, 5분 20,047, 6분 20,050, 7\~16분 20,053 | 풀 켬 | `iskm_tracks_end` |
| 풀 켬 GPU 메모리 약 3.34GB, 평평 | 1\~16분 3,338.9\~3,371.2MB, 16분 3,341.9(그림 3,342) | 풀 켬 | `gpu_mem_mean` |
| 풀 켬 프레임 1분 7.81, 16분 8.08ms | 7.814 / 8.079 | 풀 켬 | `frame_p50` |
| 풀 끔 트랙 16분 119,473 | 119473 | 풀 끔 | `iskm_tracks_end`. 1분 12,481(그림 아래 문장) |
| 분당 약 7,130개 | 7132.8 | 계산 | (119,473 − 12,481) ÷ 15 |
| 풀 끔 GPU 메모리 3.31GB → 5.06GB | 3,310.1 → 5,062.5MB(그림 3,310 / 5,062) | 풀 끔 | `gpu_mem_mean` 1분 / 16분 |
| 풀 끔 프레임 7.74 → 9.64ms | 7.737 / 9.642 | 풀 끔 | `frame_p50` 1분 / 16분 |
| "게임 스레드와 GPU가 함께 늘었다" | 게임 7.6 → 8.486, GPU 5.666 → 6.773 | 풀 끔 | `game_p50`, `gpu_p50` 1분 / 16분 |
| 13분째 GPU 메모리 계단 | 12분 3,760.0 → 13분 4,913.1MB | 풀 끔 | `gpu_mem_mean` |
| 트랙이 호드 총수의 약 두 배에서 멈춤 | 20,053 ÷ 10,000 = 2.0 | 계산 | |
| 60초 측정에서는 구별되지 않음(그림 아래 문장) | 위 "빼 보기" | | |

"처음 1분"과 "마지막 1분"은 `minute` 1과 16의 행이다. GB는 1GB = 1,000MB로 적었다(3,310.1MB → 3.31GB).

그림은 루트 README의 [entity-pool.svg](../../Docs/figures/readme/entity-pool.svg)다(`Scripts/plot-readme.py`가 `timeline.csv`에서 만든다). 그림의 숫자는 16분째 값(트랙 20,053 / 119,473, GPU 메모리 3,342 / 5,062MB, 프레임 8.08 / 9.64ms)과 풀 끔 1분째 값(12,481, 3,310MB, 7.74ms)이다. 5,062.5MB를 그림은 5,062로, 본문은 5.06GB로 적었다. 960초 전체 값(참고, `Pool1/summary.csv`): 풀 켬 프레임 p50 8.061 / p95 9.698, 풀 끔 8.672 / 10.757. 풀 켬도 1분 7.814에서 16분 8.079로 0.265ms 늘었다. 한 번씩 돈 실행이라 이것이 흔들림인지는 가르지 않았다. 본문은 이 차이를 적지 않았다.

사망은 두 실행 모두 960초 동안 58,644건이다(초당 61.09, `Pool1/summary.csv` `deaths`·`death_rate`). 풀 끔의 트랙 증가(초당 약 119)가 사망의 약 두 배인 것과, 풀 켬이 총수의 약 두 배에서 멈추는 것은 "엔티티 하나가 ISKM 컴포넌트 둘(40\~80m 그림자 켬, 80\~200m 그림자 끔)에 트랙을 하나씩 가진다"는 설명과 맞는다. 이 설명은 코드에서 확인하지 않은 추정이라 본문에 쓰지 않았다.

### 탑뷰(한계 목록)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 4단계 게임 스레드 2.65ms | 2.653 | `L4-mass` 10000 P2 | `game_p50_ms` | 2.653 / 2.632 / 2.739 |
| 5단계 게임 스레드 3.11ms | 3.108 | `L5-m4offpool` 10000 P2 | 같음 | 3.144 / 3.041 / 3.108 |
| "프레임은 GPU가 정해 같다" | 7.533 / 7.423 | 같은 두 조건 | `frame_p50_ms`, `bottleneck` = `gpu` | 4단계 7.533 / 7.587 / 7.512 ; 5단계 7.509 / 7.423 / 7.39. 차이 −0.110, 범위 0.075 / 0.119 안 |

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다. 경로는 `Engine/Plugins/` 아래다.

| 사실 | 위치 |
| --- | --- |
| 엔진 이동·조향 넷은 `FMassOffLODTag`가 없는 개체만 돈다 | `Runtime/MassGameplay/Source/MassMovement/Private/MassMovementProcessors.cpp:33`(`UMassApplyForceProcessor`), `:87`(`UMassApplyMovementProcessor`), `AI/MassAI/Source/MassNavigation/Private/MassSteeringProcessors.cpp:86`(`UMassSteerToMoveTargetProcessor`), `MassSmoothOrientationProcessors.cpp:33`(`UMassSmoothOrientationProcessor`의 고해상도 쿼리. Off 개체는 `:38`의 별도 쿼리로 틱이 온 프레임에만 회전한다) |
| Off 개체를 이동 목표 중심으로 옮긴다 | `AI/MassAI/Source/MassNavigation/Private/MassNavigationProcessors.cpp:35`(Off 태그 쿼리), `:61` `CurrentTransform.SetLocation(MoveTarget.Center)` |
| 청크가 이번 프레임에 도는 조건 | `Runtime/MassGameplay/Source/MassLOD/Public/MassLODTickRateController.h:153` |
| ISKM 트랙 할당(엔티티 핸들이 키, 없으면 할당) | `Runtime/MassGameplay/Source/MassRepresentation/Private/MassVisualizationComponent.cpp:761`(`FindOrAddId`), `:773`·`:864`(`AllocateTrack`), `UMassVisualizationComponent::EndVisualChanges` |
| 갱신되지 않은 움직이는 인스턴스는 지우고 트랙 맵은 두고 간다 | 같은 파일 `:812-827`(`SharedIdMap`만 지운다) |
| 트랙을 돌려받는 곳 | 같은 파일 `:1156` `DeallocateTrack`, `UMassVisualizationComponent::ProcessRemovesForComponent`(`:1126`). 이 파일 밖에 `DeallocateTrack` 호출은 없다(MassGameplay 전체 검색) |
| 그 함수의 입력(`EntitiesRequiringRemoval`)을 채우는 곳 | `FMassLODInstancedSkinnedMeshSignificanceRange::RemoveInstance`(같은 파일 `:1537-1547`) ← `FMassInstancedSkinnedMeshInfo::RemoveInstance`(`Public/MassSkinnedMeshRepresentationTypes.h:700-706`) ← `MassStationaryISMSwitcherProcessor.cpp:127`. 그 프로세서의 쿼리는 `FMassStaticRepresentationTag`를 요구한다(`:30`) |

engine-notes.md 3절 "ISKM 트랙 풀 누수"(773·864·1156)와 5절과 같은 내용이다. 이번에 줄을 다시 열어 확인했다.

### 공개 이슈

같은 문제를 다룬 글이 Epic 개발자 포럼에 있다: https://forums.unrealengine.com/t/mass-crowds-using-skinnedmeshinstance-leak-gpu-memory-every-wave-of-spawned-and-killed-entities-permanently-grows-the-animation-track-pool-5-8/2742508 ("Mass crowds using SkinnedMeshInstance leak GPU memory", 5.8). 글의 내용은 읽지 않았다. 해결 여부는 적지 않았다. 본문은 이 주소로 가는 링크 문장만 둔다. 이 글의 근거는 엔진 소스다. [references.md](../../Docs/Reference/references.md) "포럼 스레드"에는 제목만 있다.

### 본문에 넣은 발췌

엔진 코드는 모두 22줄이다(`// ...` 줄 포함). 남긴 줄은 소스를 그대로 옮겼다. 레포 코드는 줄 끝 주석을 뺐다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| `ULabOffMoveProcessor` | 14 | `LabMassProcessors.cpp:224-225`, `// ...`, `:254-256`, `// ...`(`:257-263`), `:264-266`, `// ...`(`:267-271` 거리 0 처리), `:272-274`. 들여쓰기는 줄였다 |
| `ULabSimulationLODTrait` | 7 | `LabMassTraits.cpp:43-44`, `// ...`, `:50`, `// ...`, `:52`, `// ...`(`:53-67`, Off 거리 계산 포함. 기본 200m면 `LowDistance`와 같다), `:68` |
| `LabUseVariableTick` | 2 | `LabMassProcessors.cpp:31-32` |
| `MassLODTickRateController.h` | 1 | `:153`(엔진) |
| `EndVisualChanges` | 14 | `MassVisualizationComponent.cpp:761`, `// ...`, `:771-775`, `// ...`, `:812`, `// ...`, `:823-824`, `// ...`, `:827`(엔진) |
| `ProcessRemovesForComponent` | 7 | 같은 파일 `:1151-1153`, `// ...`, `:1156`, `// ...`, `:1158`(엔진) |
| `ULabMinionStateProcessor::Execute` | 15 | `LabMassProcessors.cpp:720-721`, `// ...`(`:722-724`), `:725-734`, `// ...`(`:735-736`), `:737` |
| `ULabCrowdSubsystem::SpawnEntities` | 8 | `LabCrowdSubsystem.cpp:257-264` |

## 트레이스 값

본문 "결과"의 트레이스 문단 값이다. 값은 명령줄 내보내기로 뽑았다. 같은 트레이스의 Insights 화면은 아래 "Insights 화면"에 있다.

| 항목 | 값 |
| --- | --- |
| 트레이스 | `Saved/Traces/Full3traceser-L4-mass-N10000-P1-r1.utrace`(4단계), `Full3traceser-L5-m4offpool-N10000-P1-r1.utrace`(5단계). 둘 다 직렬(`mass.FullyParallel` 끔), 저장소 밖 |
| 내보낸 표 | `%TEMP%\horde-trace\F3-traceser-L4-mass-GT.csv`, `F3-traceser-L5-m4offpool-GT.csv`(`Scripts/trace-export.ps1 -Command TimerStatistics`, 스레드 `GameThread`, 구간 25\~75초, 저장소 밖) |
| 프레임 수 | 4단계 5,071, 5단계 5,285(`FEngineLoop::Tick` 횟수) |
| 식 | 타이머의 합(초) × 1000 ÷ 프레임 수 = 프레임당 ms. 모두 Incl이고 이 Processor들은 Incl = Excl이다 |

| Processor(타이머 이름 접두) | 4단계 합(초) → 프레임당 ms | 5단계 합(초) → 프레임당 ms |
| --- | --- | --- |
| `MassApplyForceProcessor` | 0.794198 → 0.1566 | 0.495914 → 0.0938 |
| `MassApplyMovementProcessor` | 0.820718 → 0.1618 | 0.331917 → 0.0628 |
| `MassSteerToMoveTargetProcessor` | 2.298443 → 0.4533 | 1.110605 → 0.2101 |
| `MassSmoothOrientationProcessor` | 4.683971 → 0.9237 | 2.014514 → 0.3812 |
| 넷의 합 | 8.597330 → 1.6954 | 3.952950 → 0.7480 |
| `LabOffMoveProcessor` | (없음) | 0.042688 → 0.0081 |

| 본문 값 | 정밀한 값 | 식 |
| --- | --- | --- |
| 넷의 합 1.70ms → 0.748ms | 1.6954 → 0.7480 | 위 표 |
| 화면 문단 "위 값은 구간 합계를 구간의 프레임 수로 나눈 것이다"(4단계) | 1.6963 | 8.60초를 5,070으로 나누면 1.70ms다. 8.60 × 1000 ÷ 5,070(화면의 구간 합계와 `MassSteerToMoveTargetProcessor`의 Count) |
| 같은 문단(5단계) | 0.7480 | 3.953초를 5,285로 나누면 0.748ms다. 3.953 × 1000 ÷ 5,285. 3.953은 내보낸 합 3.952950을 넷째 자리에서 반올림한 값이다. 화면 표시값 3.95초(본문 캡션)로 나누면 0.7474라 0.747이 된다. 본문은 계산식을 싣지 않는다 |
| 실제 44.1% | 44.12% | 0.7480 ÷ 1.6954 |
| `LabOffMoveProcessor` 0.008ms | 0.0081 | 0.042688 × 1000 ÷ 5,285 |

참고(본문에 쓰지 않았다): `LabApproachProcessor` 0.1214 → 0.0322ms, `MassSimulationLODProcessor` 5단계 0.0759ms(0.401024초), `LabOffMoveEnterObserver` 5단계 0.0009ms(2,474번). 직렬 트레이스 한 회라 `summary.csv`의 값과 직접 견주지 않는다. 예상 39%(위 3,869 ÷ 9,866)는 개체 수 비율이고, 실제 44.1%는 시간 비율이다. 앞 빌드의 트레이스에서는 39.7%로 예상과 거의 같았다. 이번에는 5%포인트 크다. 그래서 본문 문장을 "예상보다 조금 덜 줄었다"로 고쳤다. Processor별 비율(5단계 ÷ 4단계)은 `MassApplyForceProcessor` 59.9%, `MassApplyMovementProcessor` 38.8%, `MassSteerToMoveTargetProcessor` 46.4%, `MassSmoothOrientationProcessor` 41.3%다. 개체 수에 비례하지 않는 고정 비용이 있는지는 확인하지 않았다(Off 개체 쿼리, `MassSmoothOrientationProcessors.cpp:38`).

## 화면

요약의 두 화면(`step4-L4-mass.webp`, `step5-L5-m4offpool.webp`)은 측정 실행이 아니라 촬영 실행(`Full3shot-L4-mass-N10000-P1-r1`, `Full3shot-L5-m4offpool-N10000-P1-r1`, [Measurements/Full3/captures.csv](../../Measurements/Full3/captures.csv))에서 찍었다. HUD의 수치는 화면 녹화 부하가 섞여 표의 값과 다르다. 수치는 표의 값만 쓴다.

9분 시점의 화면(본문 엔티티 풀 절 끝): `Chk3-pooloffC`(풀 끔), `Chk3-poolonC`(풀 켬) 촬영 실행의 프레임이다(같은 시드·카메라, 4K 8초 녹화에서 뽑음). 고른 기준과 촬영 경위는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "Pre6 촬영과 풀 끔 떨림 확인"에 있다. 이 실행의 HUD 값은 쓰지 않는다. 확대 이미지는 위가 풀 끔, 아래가 풀 켬이고 연속 세 프레임이다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/simlod-summary.svg` | 요약 그림. 4단계와 5단계의 프레임·게임 스레드·렌더 스레드·GPU 시간 막대(1만 P1)와 16.7ms 선 | "1만 P1의 네 값" 여덟 행 |
| `images/simlod-problem.svg` | 문제 그림. 4단계의 게임 스레드·렌더 스레드·GPU 시간을 호드 총수(5천\~3만)마다 막대로 | "4단계의 호드 총수별 값" 열두 행 |
| `images/simlod-principle.svg` | 원리 도식. 거리 구간 넷(칸 폭은 거리에 비례하지 않는다)마다 그리는 방식, Simulation LOD 등급, Lab 로직 간격, 엔진 이동·조향, Lab Off 이동. 실제 주기와 밴드별 개체 수. 아래는 4단계(파괴)와 5단계(풀)의 흐름 | "Simulation LOD의 개체 수와 실제 주기" |
| `images/simlod-result.svg` | 결과 그림. 프레임 시간 중앙값 대 호드 총수(4단계 점선, 5단계 실선), 차이, 16.7ms 선, 최대 개체 수 문장 | "호드 총수별 프레임 시간" |
| `images/simlod-ablation.svg` | 결과 그림. 최종 구성, Simulation LOD 뺌, 엔티티 풀 뺌의 프레임 시간 중앙값 막대와 세 회차 범위 | "빼 보기" |
| `../../Docs/figures/readme/entity-pool.svg` | 16분 실행의 트랙 수·GPU 메모리·프레임 시간 추이(루트 README와 같은 그림) | "16분 실행" |
| `../../Docs/figures/steps/step4-L4-mass.webp`, `step5-L5-m4offpool.webp` | 4·5단계 화면(HUD 켬) | 위 "화면" |

SVG 다섯 개는 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/05-simulation-lod-entity-pool/make-figures.py`). 값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽는다. 상수는 없다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다.

그림 글자는 소수 둘째 자리, 본문은 유효 숫자 세 자리다. 같은 값이다(그림 0.11 / 0.33 / 0.85, 본문 0.113 / 0.328 / 0.852). 결과 그림의 최대 개체 수 문장은 정확한 값(19,952 / 22,857)을 쓴다.

## 경위의 근거

본문 "틀렸던 가설"은 수치를 붙이지 않았다. 근거는 최종 측정과 조건이 다른 옛 묶음이라 비교에 쓰지 않는다.

- 틱 간격만 건 변형이 프레임을 줄이지 못했다, 간격을 바꿔도 같았다: M4 묶음(Test 패키지 7ff3fce), `m4mass` 대 `m3mass`와 틱 간격 스윕(`m4m25l50`, `m4m25l100`, `m4m50l50`). [Worklog/M4.md](../../Docs/Worklog/M4.md) "`m4mass` 대 `m3mass`", "하위 실험 (20k P1)". 최종 측정의 사다리에는 틱 간격만 바꾼 단계가 없다(Simulation LOD는 5단계에 Off 이동·풀과 함께 들어온다).
- 이유 셋(Lab 로직은 작았다, LOD 프로세서가 비용을 더했다, 엔진 이동·조향이 매 프레임 돈다): 같은 문서 "트레이스 분석"의 1·2·3절(직렬 트레이스, 20k P1).
- 실제 주기가 설정보다 짧다: 같은 문서 "설정 간격 대 실제 주기"와 개발 실행 로그. 본문의 실제 주기 값(0.110·0.321초)은 최종 측정의 값이다.
- 200m 밖을 Off로 두고 자체 프로세서로 옮기자 줄었다: E4 (d) 묶음(ddd7c39), 같은 문서 "E4 (d)". 최종 측정에서 같은 방향은 4 → 5단계 차이와 `A8-simlod`가 보인다.
- 저격·상태 타이머에 필터를 걸지 않은 이유(피격 마지막 자세로 멈춤): 같은 문서 "사망 전환 지연: 원인과 수정". 본문은 이유를 적지 않았다.
- 엔티티 풀을 5단계에 넣은 이유(다음 단계에서 표현만 바꾸기): [plan.md](../../Docs/plan.md) "M5" 결정 1과 변형 표(`m4offpool`이 기준점).
- 글 재료의 사실 확인: `Docs/post-review.md` "06 Simulation LOD"(글을 다 쓰면 지우는 문서라, 지운 뒤에는 git 이력에서 본다).

## 확인하지 않은 것

- 5단계에서 엔티티 풀만의 몫. 최종 측정의 사다리에 풀만 뺀 5단계가 없다. `A8-pool`은 최종 구성(ISKM, 입자)에서 뺀 값이다.
- 엔진 이동·조향 넷의 시간은 직렬 트레이스 한 회로만 확인했다(44.1%, 위 "트레이스 값"). 개체 수 비율 39%보다 큰 원인과 병렬 실행에서의 값은 보지 않았다.
- 풀 켬에서 트랙이 약 2만에서 멈추는 이유. 위 "16분 실행"의 추정(컴포넌트 둘)은 코드로 확인하지 않았다.
- 풀 끔 13분째 GPU 메모리 계단(약 1.15GB)의 원인.
- 풀 끔에서 시간이 갈수록 프레임 시간 변동(느린 쪽)이 커지는지. `timeline.csv`는 p50만 있다. 풀 끔에서 미니언이 떨려 보인다는 관찰은 화면 근거가 없어 본문에 쓰지 않았다(9분 시점 녹화 4건에서 재현하지 못했다, Worklog/Q6.md "Pre6 촬영과 풀 끔 떨림 확인").
- 16분 실행의 반복. 각 1회라 범위가 없다. 풀 켬의 0.265ms 증가가 흔들림인지 모른다.
- 탑뷰에서 5단계의 게임 스레드가 늘어난 원인. M4 묶음에도 같은 방향의 관찰이 있다(Worklog/M4.md "P2 Game +2.15ms").
- 5단계 1만 P1 렌더 스레드 r2(5.935)가 다른 회차보다 긴 원인.
- 5단계 화면에서 80\~200m 개체의 움직임이 4단계와 같아 보이는지. 움직이는 WebP를 열어 보지 않았다(브라우저 금지). 본문은 설계상 같다는 것까지만 적었다.
- SVG를 화면에 그려 보지 않았다. 글자 겹침은 좌표와 글자 수로만 확인했다.

## 측정 기록의 변경

- 2026-11-13(처음 씀).
- 2026-11-13: 수치를 총괄 묶음 `Full2`(08e45f2)에서 `Full3`(df41c13)으로 바꿨다. 애니메이션 예산 2ms → 4ms(ADR-0016)와 `SnipeView` 경합 수정(62b6a42)으로 전 조건을 다시 쟀기 때문이다. 5단계 3만이 60fps 한계에 닿아 요약과 결과의 최대 개체 수 문장을 고쳤다. Insights 화면과 `Pool1` 16분 실행은 앞 빌드의 것 그대로다.
- 2026-11-13: 트레이스 값을 Full3 트레이스(df41c13)에서 다시 뽑았다. 엔진 이동·조향 넷의 비율이 39.7%에서 44.1%로 바뀌어 본문의 결과 문장을 "예상보다 조금 덜 줄었다"로 고쳤다.
- 2026-11-13: Insights 스크린샷을 Full3 트레이스로 다시 찍었다(코디네이터). "표시한 행"은 새 화면의 값이다.
- 2026-11-14: 본문 문장을 다듬었다. 화면 문단의 계산식(8.60초 ÷ 5,070, 3.953초 ÷ 5,285)을 본문에서 빼고 위 "트레이스 값" 표에만 둔다. 수치는 바꾸지 않았다.

## Insights 화면

본문의 Insights 스크린샷이다. 2026-11-13에 Full3 트레이스로 다시 찍었다. 값의 계산은 위 트레이스 표가 근거이고, 화면은 같은 트레이스에서 같은 타이머의 구간 합계를 보인다.

| 항목 | 내용 |
| --- | --- |
| 파일 | `images/insights-movement-processors.jpg`. 위가 4단계, 아래가 5단계다 |
| 트레이스 | `Full3traceser-L4-mass-N10000-P1-r1`, `Full3traceser-L5-m4offpool-N10000-P1-r1`(직렬) |
| 거른 말 | Timers 목록 `Mass`, CPU 타이머만 |
| 표시한 행(Incl) | 4단계(Count 5,070): `MassSmoothOrientationProcessor` 4.68s, `MassSteerToMoveTargetProcessor` 2.3s, `MassApplyMovementProcessor` 820.68ms, `MassApplyForceProcessor` 794.24ms. 5단계(Count 5,284): 2.01s, 1.11s, `MassApplyForceProcessor` 495.91ms, `MassApplyMovementProcessor` 331.84ms |
| 화면에 없는 값 | `LabOffMoveProcessor` 0.008ms는 `Mass`로 거른 목록에 없다 |
| 구간 | Timing 뷰의 시간 눈금자를 끌어 25초부터 75초까지(길이 표시 50s)를 골랐다. 손으로 고른 구간이라 내보내기의 창과 프레임이 한두 개 다를 수 있다. 초 단위 합계는 표시 자릿수까지 내보낸 값과 같다. ms로 보이는 행과 Count는 조금 다르다. 내보낸 값은 4단계 `MassApplyMovementProcessor` 820.72ms, `MassApplyForceProcessor` 794.20ms, 5단계 Count 5,285(`MassApplyForceProcessor`만 5,284), `MassApplyMovementProcessor` 331.92ms다. 본문과 캡션은 내보낸 값을 쓴다 |
| 가공 | 창을 최대화하고 Timing 뷰를 한 칸 축소한 뒤, 창 캡처(PrintWindow)로 화면 전체를 한 번에 찍어 2304폭으로 줄였다. 트레이스 파일 이름이 보이는 제목 줄은 잘라 냈다. 빨간 상자와 단계 이름을 그렸다. 두 트레이스를 견주는 그림은 위아래로 붙였다 |
