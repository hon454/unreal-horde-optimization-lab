# 05 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓰고, 개수는 줄이지 않는다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-12 20:34\~11-13 00:39). 경위는 [작업 기록의 재측정 절](../../Docs/Worklog/Q6.md#full2-재측정2026-11-12) |
| 16분 실행 | 같은 패키지로 2026-11-13 00:44\~01:17에 돈 엔티티 풀 묶음 `Pool1`. 같은 절의 "엔티티 풀 Pool1", 규약은 [measurement.md](../../Docs/Guides/measurement.md) "엔티티 풀 묶음 규약" |
| 빌드 | Test 패키지 08e45f2, CSV 메타데이터 `labbuild=08e45f24a5e5`(두 묶음 모두) |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1), 참고로 탑뷰(P2). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L4-mass`(`-LabStage=M3 -LabVariant=mass`), `L5-m4offpool`(`-LabStage=M5 -LabVariant=m4offpool`), 빼 보기 `L8-q4final`, `A8-simlod`(`-LabSimLod=0`), `A8-pool`(`-LabEntityPool=0`). 인자와 기대 메타데이터는 `Scripts/full-conditions.json` |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full2/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full2/runs.csv), [16분 실행의 1분 구간 값](../../Measurements/Pool1/timeline.csv), [16분 실행의 전체 값](../../Measurements/Pool1/summary.csv) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full2/full-n-at-60fps.csv)(`Scripts/plot.py`가 측정 N 사이를 로그 보간) |
| 트레이스 | 본문 "결과"의 트레이스 문단 값은 아래 "트레이스 값"이다. 최종 측정 빌드의 직렬 트레이스 한 회씩이다 |

### 4단계와 5단계의 차이(코드)

`full-conditions.json`의 기대값에서 `L4` → `L5`로 바뀌는 것은 `LabPool` 0 → 1, `LabSimLod` `-` → `80/200m tick=0/0.50/1.00 avoid=0 offmove=1`, `LabTechPool`·`LabTechSimLod` 0 → 1뿐이다. 나머지(`LabActorCap` 500, `LabHydrateM` 30, `LabFarShadow` 1, `LabTechIskm` 0 등)는 같다.

- 단계 M5에서 `m4offpool`은 VAT 그대로다: `FLabConfig::UsesIskm`(`Source/HordeOptLab/LabConfig.cpp:324-327`)이 `M4OffPool`이면 거짓. `FLabGameModule::StartupModule`(`HordeOptLab.cpp:37-40`)이 `ULabVatISMProcessor`를 켠 채 둔다.
- Simulation LOD: `UsesSimLod()`(`LabConfig.h:369`, M4 이상)가 참이라 `ULabSimulationLODTrait`을 붙인다(`LabCrowdSubsystem.cpp:66-71`). 값은 `LabMassTraits.cpp:39-69`. `SimLod()`(`LabConfig.cpp:303-322`)가 `M4OffPool`과 M5 단계에서 `bOffMove = true`. 경계 80m·200m는 `FLabSimLod::MediumDistance`·`LowDistance`(`LabConfig.h:126-127`), 간격 0.5초·1.0초는 `TickMedium`·`TickLow`(`:128-129`).
- 청크 필터: `LabUseVariableTick`(`LabMassProcessors.cpp:29-33`)을 접근(`:88-93`)과 범위 피해(`ULabOrbContactProcessor`, `:385-390`)가 부른다. 둘 다 `LabRunAfterSimLod`(`:41-47`)로 LOD 그룹 뒤에 돈다. 범위 피해는 누적 델타를 쓴다(`:431`, `:452`). 저격(`ULabOrbSnipeProcessor`)과 상태(`ULabMinionStateProcessor`, `:649-652` 주석)는 필터를 걸지 않는다.
- Off 이동: `HordeOptLab.cpp:44-48`이 `UMassOffLODNavigationProcessor`의 자동 등록을 끄고(`!bSimLod`) `ULabOffMoveProcessor`를 켠다(`bOffMove`). 관찰자 `ULabOffMoveEnterObserver`(`LabMassProcessors.cpp:303-363`)가 Off에 드는 순간의 시각을 맞춘다. 본문은 관찰자를 적지 않았다.
- 엔티티 풀: `bEntityPool`(`LabConfig.cpp:130-138`)이 `M4OffPool`이면 참. 경로는 `ULabMinionStateProcessor::Execute`(`LabMassProcessors.cpp:720-746`), `ULabCrowdSubsystem::RecycleAndRespawn`(`LabCrowdSubsystem.cpp:332-370`), `SpawnEntities`(`:247-303`), `ReinitPooledEntity`(`:316-330`), 세워 두는 곳 `LabPoolParkLocation`(`:313`, (1e6, 1e6, −1e5)cm).

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. 세 회차 열은 r1 / r2 / r3이다. "(그림)"은 본문 산문이 아니라 그림에만 쓴 값이다.

### 1만 P1의 네 값(요약 그림, 문제·결과 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 4단계 프레임 시간 8.16ms | 8.163 | `L4-mass` 10000 P1 | `frame_p50_ms` | 8.163 / 8.018 / 8.22 |
| 4단계 게임 스레드 7.87ms | 7.869 | 같음 | `game_p50_ms` | 7.869 / 7.713 / 7.923 |
| 4단계 렌더 스레드 4.69ms | 4.694 | 같음 | `render_p50_ms` | 4.618 / 4.797 / 4.694 |
| 4단계 GPU 6.94ms | 6.938 | 같음 | `gpu_p50_ms` | 6.926 / 6.942 / 6.938 |
| 5단계 프레임 시간 7.65ms | 7.654 | `L5-m4offpool` 10000 P1 | `frame_p50_ms` | 7.646 / 7.654 / 7.695 |
| 5단계 게임 스레드 7.33ms | 7.333 | 같음 | `game_p50_ms` | 7.333 / 7.321 / 7.375 |
| 5단계 렌더 스레드 4.60ms(그림) | 4.601 | 같음 | `render_p50_ms` | 4.618 / 4.591 / 4.601 |
| 5단계 GPU 6.97ms | 6.974 | 같음 | `gpu_p50_ms` | 6.96 / 6.983 / 6.974 |
| "두 단계 모두 게임 스레드가 가장 느리다" | | 위 두 조건 | `bottleneck` = `game` | 세 회차 모두 `game` |
| 프레임 시간 −0.509ms | −0.509 | 계산 | 7.654 − 8.163. 범위 0.202 / 0.049보다 크다 | |
| 게임 스레드 −0.536ms | −0.536 | 계산 | 7.333 − 7.869. 범위 0.210 / 0.054보다 크다 | |
| GPU "거의 같다" | +0.036 | 계산 | 6.974 − 6.938. 범위 0.016 / 0.023보다 크지만 0.5% 차이다 | |

GPU +0.036ms는 [measurement.md](../../Docs/Guides/measurement.md) "차이 판정"으로는 범위 밖이다. 본문은 크기가 작아 "거의 같다"로 적었다. 원인은 보지 않았다.

### 4단계의 호드 총수별 값(문제 그림)

| 그림 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 게임 스레드 5천 4.53 | 4.533 | `L4-mass` 5000 P1 | `game_p50_ms` | 4.502 / 4.561 / 4.533 |
| 렌더 스레드 5천 3.48 | 3.483 | 같음 | `render_p50_ms` | 3.473 / 3.5 / 3.483 |
| GPU 5천 5.73 | 5.726 | 같음 | `gpu_p50_ms` | 5.726 / 5.731 / 5.718 |
| 게임 스레드 1만 7.87 | 7.869 | 10000 P1 | `game_p50_ms` | 위 표 |
| 렌더 스레드 1만 4.69 | 4.694 | 같음 | `render_p50_ms` | 위 표 |
| GPU 1만 6.94 | 6.938 | 같음 | `gpu_p50_ms` | 위 표 |
| 게임 스레드 2만 12.0 | 12.017 | 20000 P1 | `game_p50_ms` | 11.96 / 12.104 / 12.017 |
| 렌더 스레드 2만 7.19 | 7.187 | 같음 | `render_p50_ms` | 7.23 / 7.187 / 7.075 |
| GPU 2만 9.63 | 9.625 | 같음 | `gpu_p50_ms` | 9.609 / 9.659 / 9.625 |
| 게임 스레드 3만 15.9(본문 15.9ms) | 15.932 | 30000 P1 | `game_p50_ms` | 15.932 / 16.009 / 15.925 |
| 렌더 스레드 3만 9.02 | 9.019 | 같음 | `render_p50_ms` | 9.03 / 8.988 / 9.019 |
| GPU 3만 12.5 | 12.484 | 같음 | `gpu_p50_ms` | 12.523 / 12.484 / 12.476 |
| "5천은 GPU, 1만부터 게임 스레드" | | 네 행 | `bottleneck` 5천 `gpu`, 1만·2만·3만 `game` | |

### 호드 총수별 프레임 시간(결과 그림, 본문)

| 본문 값 | 정밀한 값(4단계 / 5단계) | 조건 | 출처(열) | 세 회차(4단계 ; 5단계) |
| --- | --- | --- | --- | --- |
| (그림) 5천 6.17 / 6.19, 차이 +0.02 | 6.171 / 6.192, +0.021 | 5000 P1 | `frame_p50_ms` | 6.171 / 6.176 / 6.159 ; 6.2 / 6.174 / 6.192 |
| (그림) 1만 8.16 / 7.65, 차이 −0.51 | 8.163 / 7.654, −0.509 | 10000 P1 | 같음 | 위 표 |
| 2만 차이 1.27ms | 12.522 / 11.256, −1.266 | 20000 P1 | 같음 | 12.461 / 12.617 / 12.522 ; 11.158 / 11.319 / 11.256 |
| 3만 차이 2.09ms | 16.655 / 14.568, −2.087 | 30000 P1 | 같음 | 16.655 / 16.709 / 16.648 ; 14.258 / 14.568 / 14.693 |
| "5천에서는 차이가 없다" | +0.021 | 계산 | 범위 0.017 / 0.026 안. 병목은 두 단계 모두 `gpu` | |
| 4단계 최대 개체 수 25,636 | 25636 | `4 L4-mass` | `full-n-at-60fps.csv` `n_p95` | 프레임 p95: 2만 13.943, 3만 18.446 |
| 5단계는 3만에서도 한계에 닿지 않음, 3만 16.3ms | 30000, `above range`; 16.298 | `5 L5-m4offpool`; `L5-m4offpool` 30000 P1 | `n_p95`·`note_p95`; `frame_p95_ms` | 16.488 / 16.298 / 16.216(최댓값도 16.7 안) |

2만·3만의 차이도 두 범위(2만 0.156 / 0.161, 3만 0.061 / 0.435)보다 크다.

### Simulation LOD의 개체 수와 실제 주기(원리 그림, 본문)

`L5-m4offpool` 10000 P1. 개체 수는 접근 프로세서가 센 걷는 개체의 프레임 평균이다(`Lab/SimTick*`·`Lab/SimBand*`, `LabMassProcessors.cpp:142-166`). 실제 주기 = 평균 프레임 시간 × 밴드 전체 개체 수 ÷ 그 프레임에 돈 개체 수(`Scripts/summarize.py:177-178, 211-212`).

| 본문 값 | 정밀한 값 | 출처(열) | 세 회차 |
| --- | --- | --- | --- |
| High 1,458(그림) | 1458.47 | `sim_band_high`(= `sim_tick_high`, 매 프레임) | 1458.56 / 1458.47 / 1458.4 |
| Medium 2,411(그림) | 2410.95 | `sim_band_medium` | 2411.55 / 2410.95 / 2410.05 |
| 200m 밖 5,997 | 5996.5 | `sim_band_off` | 5995.85 / 5996.5 / 5997.5 |
| 걷는 개체 9,866 | 9865.92 | 계산 | 1458.47 + 2410.95 + 5996.5 |
| 엔진 이동·조향이 처리하는 개체 3,869, 39% | 3869.42, 39.22% | 계산 | 1458.47 + 2410.95; ÷ 9865.92 |
| Medium 설정 0.5초, 실제 0.110초 | 0.5 / 0.1102 | `cfg_tick_medium` / `real_period_medium` | 0.1102 / 0.1096 / 0.1111 |
| Off 설정 1초, 실제 0.321초 | 1.0 / 0.3209 | `cfg_tick_off` / `real_period_off` | 0.3176 / 0.3209 / 0.3323 |
| Off 개체가 한 번에 옮겨지는 거리 약 0.963m | 0.9627 | 계산 | 0.3209초 × 3m/s(`FLabScenario::MoveSpeed`, `LabConfig.h:148`). 실제 주기의 평균으로 셈한 값이다 |
| 80m·200m 경계, 0.5·1초 간격 | 8000 / 20000cm, 0.5 / 1.0초 | 코드 | 위 "4단계와 5단계의 차이" |
| 맵 가장자리 고리 490\~500m(그림) | 49000\~50000cm | 코드 | `FLabScenario::MapRadius`, `RespawnRingWidth`(`LabConfig.h:140-141`) |
| 개체의 60%가 200m 밖 | 0.6 | 계산 | [measurement.md](../../Docs/Guides/measurement.md) "확정값" 맵 반경 행(구간 비율 = 구간 폭 ÷ R = 300 ÷ 500) |

"9,866"과 "200m 밖 5,997"은 5단계에서 센 값이다. 4단계에는 이 스탯이 없다. 4단계의 걷는 개체도 같은 시나리오라 같은 수로 보았다(동등성 검사가 사망 수와 오브 근처 개체 수를 같게 확인한다, measurement.md "총괄 묶음 규약").

### 빼 보기(빼 보기 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 최종 구성 7.78(그림) | 7.783 | `L8-q4final` 10000 P1 | `frame_p50_ms` | 7.802 / 7.783 / 7.742 |
| Simulation LOD를 빼면 8.83(그림), +1.04ms | 8.827, +1.044 | `A8-simlod` 10000 P1 | 같음, 계산 8.827 − 7.783 | 8.822 / 8.827 / 8.904 |
| 엔티티 풀을 빼면 7.84(그림), +0.055ms, 범위 안 | 7.838, +0.055 | `A8-pool` 10000 P1 | 같음 | 7.838 / 7.807 / 7.936 |
| 판정 | 범위 0.060(`L8`), 0.082(`A8-simlod`), 0.129(`A8-pool`) | 계산 | 1.044 > 0.082, 0.055 < 0.129 | |
| "병목은 세 구성 모두 게임 스레드"(그림) | `game` | 세 조건 | `bottleneck`. 게임 스레드 p50 7.648 / 8.684 / 7.706 | |

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
| 4단계 게임 스레드 2.67ms | 2.673 | `L4-mass` 10000 P2 | `game_p50_ms` | 2.621 / 2.678 / 2.673 |
| 5단계 게임 스레드 3.13ms | 3.127 | `L5-m4offpool` 10000 P2 | 같음 | 3.1 / 3.17 / 3.127 |
| "프레임은 GPU가 정해 같다" | 7.517 / 7.511 | 같은 두 조건 | `frame_p50_ms`, `bottleneck` = `gpu` | 4단계 7.499 / 7.54 / 7.517 ; 5단계 7.518 / 7.502 / 7.511 |

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
| 트레이스 | `Saved/Traces/Full2traceser-L4-mass-N10000-P1-r1.utrace`(4단계), `Full2traceser-L5-m4offpool-N10000-P1-r1.utrace`(5단계). 둘 다 직렬(`mass.FullyParallel` 끔), 저장소 밖 |
| 내보낸 표 | `%TEMP%\horde-trace\F2-traceser-L4-mass-GT.csv`, `F2-traceser-L5-m4offpool-GT.csv`(`Scripts/trace-export.ps1 -Command TimerStatistics`, 스레드 `GameThread`, 구간 25\~75초, 저장소 밖) |
| 프레임 수 | 4단계 5,674, 5단계 6,286(`FEngineLoop::Tick` 횟수) |
| 식 | 타이머의 합(초) × 1000 ÷ 프레임 수 = 프레임당 ms. 모두 Incl이고 이 Processor들은 Incl = Excl이다 |

| Processor(타이머 이름 접두) | 4단계 합(초) → 프레임당 ms | 5단계 합(초) → 프레임당 ms |
| --- | --- | --- |
| `MassApplyForceProcessor` | 0.847657 → 0.1494 | 0.450729 → 0.0717 |
| `MassApplyMovementProcessor` | 0.740211 → 0.1305 | 0.217270 → 0.0346 |
| `MassSteerToMoveTargetProcessor` | 2.654627 → 0.4679 | 1.226173 → 0.1951 |
| `MassSmoothOrientationProcessor` | 5.311797 → 0.9362 | 2.310555 → 0.3676 |
| 넷의 합 | 9.554292 → 1.6839 | 4.204727 → 0.6689 |
| `LabOffMoveProcessor` | (없음) | 0.040224 → 0.0064 |

| 본문 값 | 정밀한 값 | 식 |
| --- | --- | --- |
| 넷의 합 1.68ms → 0.669ms | 1.6839 → 0.6689 | 위 표 |
| 실제 39.7% | 39.72% | 0.6689 ÷ 1.6839 |
| `LabOffMoveProcessor` 0.006ms | 0.0064 | 0.040224 × 1000 ÷ 6,286 |

참고(본문에 쓰지 않았다): `LabApproachProcessor` 0.1100 → 0.0237ms, `MassSimulationLODProcessor` 5단계 0.0671ms(0.421803초), `LabOffMoveEnterObserver` 5단계 0.0008ms(2,543번). 직렬 트레이스 한 회라 `summary.csv`의 값과 직접 견주지 않는다. 예상 39%(위 3,869 ÷ 9,866)는 개체 수 비율이고, 실제 39.7%는 시간 비율이다. 5단계에서 `MassSmoothOrientationProcessor`가 예상보다 덜 줄었다는 징후는 없었다(Off 개체 쿼리, `MassSmoothOrientationProcessors.cpp:38`).

## 화면

요약의 두 화면(`step4-L4-mass.webp`, `step5-L5-m4offpool.webp`)은 측정 실행이 아니라 촬영 실행(`Pre5shot-L4-mass-N10000-P1-r1`, `Pre5shot-L5-m4offpool-N10000-P1-r1`, [Measurements/Pre5/captures.csv](../../Measurements/Pre5/captures.csv))에서 찍었다. HUD의 수치는 화면 녹화 부하가 섞여 표의 값과 다르다. 수치는 표의 값만 쓴다.

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

그림 글자는 소수 둘째 자리, 본문은 유효 숫자 세 자리다. 같은 값이다(그림 0.11 / 0.32 / 0.51, 본문 0.110 / 0.321 / 0.509).

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
- 엔진 이동·조향 넷의 시간이 39% 근처로 줄었는지는 직렬 트레이스 한 회로 확인했다(39.7%, 위 "트레이스 값"). 병렬 실행에서의 값은 보지 않았다.
- 풀 켬에서 트랙이 약 2만에서 멈추는 이유. 위 "16분 실행"의 추정(컴포넌트 둘)은 코드로 확인하지 않았다.
- 풀 끔 13분째 GPU 메모리 계단(약 1.15GB)의 원인.
- 풀 끔에서 시간이 갈수록 프레임 시간 변동(느린 쪽)이 커지는지. `timeline.csv`는 p50만 있다. 풀 끔에서 미니언이 떨려 보인다는 관찰은 화면 근거가 없어 본문에 쓰지 않았다(9분 시점 녹화 4건에서 재현하지 못했다, Worklog/Q6.md "Pre6 촬영과 풀 끔 떨림 확인").
- 16분 실행의 반복. 각 1회라 범위가 없다. 풀 켬의 0.265ms 증가가 흔들림인지 모른다.
- 탑뷰에서 5단계의 게임 스레드가 늘어난 원인. M4 묶음에도 같은 방향의 관찰이 있다(Worklog/M4.md "P2 Game +2.15ms").
- 1만 P1 GPU +0.036ms의 원인.
- 5단계 화면에서 80\~200m 개체의 움직임이 4단계와 같아 보이는지. 움직이는 WebP를 열어 보지 않았다(브라우저 금지). 본문은 설계상 같다는 것까지만 적었다.
- SVG를 화면에 그려 보지 않았다. 글자 겹침은 좌표와 글자 수로만 확인했다.

## 측정 기록의 변경

- 2026-11-13(처음 씀).

## Insights 화면

본문의 Insights 스크린샷이다. 2026-11-13에 찍었다. 값의 계산은 위 트레이스 표가 근거이고, 화면은 같은 트레이스에서 같은 타이머의 구간 합계를 보인다.

| 항목 | 내용 |
| --- | --- |
| 파일 | `images/insights-movement-processors.jpg`. 위가 4단계, 아래가 5단계다 |
| 트레이스 | `Full2traceser-L4-mass-N10000-P1-r1`, `Full2traceser-L5-m4offpool-N10000-P1-r1`(직렬) |
| 거른 말 | Timers 목록 `Mass`, CPU 타이머만 |
| 표시한 행(Incl) | 4단계(Count 5,672): `MassSmoothOrientationProcessor` 5.31s, `MassSteerToMoveTargetProcessor` 2.65s, `MassApplyForceProcessor` 847.57ms, `MassApplyMovementProcessor` 739.88ms. 합 9.55s. 5단계(Count 6,284): 2.31s, 1.23s, 450.57ms, 217.21ms. 합 4.21s |
| 화면에 없는 값 | `LabOffMoveProcessor` 0.006ms는 `Mass`로 거른 목록에 없다 |
| 구간 | Timing 뷰의 시간 눈금자를 끌어 25초부터 75초까지(길이 표시 50s)를 골랐다. 손으로 고른 구간이라 내보내기의 창과 프레임이 한두 개 다를 수 있다. 화면의 합계는 표시 자릿수까지 내보낸 값과 같다 |
| 가공 | 화면 캡처 둘(왼쪽 절반, 오른쪽 절반)을 이어 붙였다. 트레이스 파일 이름이 보이는 제목 줄은 잘라 냈다. 빨간 상자와 단계 이름을 그렸다. 두 트레이스를 견주는 그림은 위아래로 붙였다 |
