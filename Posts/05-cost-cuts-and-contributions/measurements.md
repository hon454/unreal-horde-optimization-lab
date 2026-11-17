# 05 측정 기록: 비용을 줄이는 단계와 기법별 기여

[본문](README.md)의 수치마다 정밀한 값, 출처, 실행별 값, 계산식을 둔다.

## 묶음과 조건

- 총괄 묶음 `Full4`. Test 패키지 de0e21b(태그 `v2-full4`, CSV 메타데이터 `LabBuild` de0e21bd64ac), 2026-11-16\~17(`Measurements/Full4/batch.json`).
- 조건: 호드 총수 1만, 오브 시점 카메라(`P1`), 이동 장면(`V0`), 워밍업 60초 + 측정 180초, 조건마다 세 번. 라벨은 `Full4-V0-<구성>-N10000-P1-r<회차>`다.
- 값은 `Measurements/Full4/summary.csv`의 중앙값이다. `summarize.py`가 `runs_used`로 고른 실행의 중앙값이고, 범위는 그 실행의 최솟값\~최댓값이다. 실행별 값은 `Measurements/Full4/runs.csv`다.
- 참고 묶음 `Pool2`. 같은 패키지 de0e21b, 2026-11-17 10:16\~10:51(`Measurements/Pool2/batch.json`). 최종 구성(`s8`, 풀 켬)과 엔티티 풀을 끈 조건(`a8pool`)을 한 번씩 돌렸다. 시나리오는 위와 같고 측정만 960초다. 1분 구간 값은 `Measurements/Pool2/timeline.csv`(`Scripts/pool-timeline.py`)다.
- `Pool2`는 실행이 조건마다 한 번이라 범위가 없다. 차이 판정에 쓰지 않고 시간 추이만 본다([measurement.md](../../Docs/Guides/measurement.md) "엔티티 풀 묶음 규약"). 총괄 묶음의 값과 한 그림에 잇지 않는다. 본문에서는 "참고 측정"이라 적었다.
- 구별 판정은 [measurement.md](../../Docs/Guides/measurement.md) "차이 판정"이다. 중앙값의 차이가 0.5ms 이상이고 두 조건의 범위 가운데 큰 쪽보다 클 때만 구별된다. 프레임 p95는 `Docs/figures/full4/full-ablation-moving.csv`의 `p95_distinct`(`Scripts/plot.py` `distinguishable()`)와 같은 판정이다. `make-figures.py`가 같은 식으로 다시 셈하고 CSV와 다르면 멈춘다.
- 반올림: 절대값은 유효 숫자 세 자리, 차이(ms)는 소수 둘째 자리다. 둘 다 CSV 글자에서 반올림한다(half up). 그래서 12.686 − 11.881 = 0.805는 0.81이다.

## 본문의 수치와 출처

측정값의 행이다. 조건은 위 "묶음과 조건"이고, 실행 열은 회차 순서다.

| 본문의 수치 | 정밀한 값 | 출처 | 실행별 값 | 계산 |
| --- | --- | --- | --- | --- |
| 단계 8 프레임 p95 −0.94ms | −0.937 | `V0-s8`, `V0-s7` r1\~r3 | s8 14.426, 14.035, 14.231 / s7 15.426, 15.168, 15.091 | 14.231 − 15.168 |
| 단계 7 프레임 p95 15.2ms | 15.168 | `V0-s7` r1\~r3 | 15.426, 15.168, 15.091 | 중앙값 |
| 최종 구성 프레임 p95 14.2ms | 14.231 | `V0-s8` r1\~r3 | 14.426, 14.035, 14.231 | 중앙값 |
| 16.7ms까지의 여유 2.47ms | 2.469 | `V0-s8` | 위와 같다 | 16.7 − 14.231 |
| 단계 8 게임 스레드 12.7 → 11.9ms | 12.686 → 11.881 | `V0-s7`, `V0-s8` | s7 12.931, 12.686, 12.686 / s8 12.023, 11.79, 11.881 | 차이 −0.805 |
| 단계 8 GPU 시간 −0.48ms | −0.484 | `V0-s7`, `V0-s8` | s7 6.648, 6.657, 6.624 / s8 6.18, 6.164, 6.154 | 6.164 − 6.648 |
| 단계 5 프레임 p95 +0.31ms | +0.311 | `V0-s4`, `V0-s5` r1\~r3 | s4 14.37, 14.205, 14.086 / s5 14.7, 14.516, 14.513 | 14.516 − 14.205 |
| Simulation LOD를 끈 조건 −0.24ms | −0.235 | `V0-a8sim` r1\~r3 | 14.15, 13.897, 13.996 | 13.996 − 14.231 |
| 엔티티 풀을 끈 조건 +0.01ms | +0.009 | `V0-a8pool` r1\~r3 | 14.369, 14.088, 14.24 | 14.24 − 14.231 |
| 액터 메시 고정 바운드를 끈 조건 +0.98ms | +0.978 | `V0-a8fsb` r1\~r3 | 15.209, 15.177, 15.275 | 15.209 − 14.231 |
| 액터 메시의 애니메이션 시간 2.71 → 3.63ms | 2.708 → 3.631 | `V0-s8`, `V0-a8fsb`, 열 `budgeted_anim_ms` | s8 2.728, 2.708, 2.683 / fsb 3.631, 3.596, 3.756 | 차이 +0.923, 구별됨 |
| 그림자를 다시 켠 조건 프레임 p95 +0.07ms | +0.071 | `V0-a8shd` r1, r2, r3, r7 | 14.511, 14.219, 14.06, 14.384 | 14.302 − 14.231 |
| 그림자를 다시 켠 조건 GPU 시간 +0.48ms | +0.482 | `V0-a8shd` r1, r2, r3, r7 | 6.648, 6.646, 6.632, 6.645 | 6.646 − 6.164 |
| 하이드레이션을 끈 조건 −6.98ms | −6.984 | `V0-a8hyd` r5, r6, r21 | 7.236, 7.251, 7.247 | 7.247 − 14.231 |
| 하이드레이션을 끈 조건 게임 스레드 5.23ms | 5.23 | `V0-a8hyd` r5, r6, r21 | 5.23, 5.227, 5.35 | 중앙값 |
| 하이드레이션을 끈 조건 GPU 6.21ms, 병목 GPU | 6.211 | `V0-a8hyd`, 열 `bottleneck` gpu | 6.211, 6.23, 6.2 | 중앙값. 렌더 스레드 2.692 |
| 액터 캡슐을 끈 조건 −1.86ms | −1.855 | `V0-a8cap` r1\~r3 | 12.39, 12.365, 12.376 | 12.376 − 14.231 |
| 200m 밖을 그리지 않는 일을 끈 조건 +3.54ms | +3.536 | `V0-a8far` r2, r3, r7 | 17.76, 18.06, 17.767 | 17.767 − 14.231 |
| 같은 조건 게임 스레드 11.9 → 15.1ms | 11.881 → 15.114 | `V0-s8`, `V0-a8far` | far 15.098, 15.114, 15.26 | 차이 +3.233 |
| 엔진 회피로 바꾼 하위 실험 +8.57ms | +8.572 | `V0-s8av` r1\~r3 | 23.169, 22.616, 22.803 | 22.803 − 14.231 |
| 최소 유지 시간을 끄면 왕복 전환 초당 4.34건 | 4.344 | `V0-a8hold`, 열 `sw_round_all` | 4.5, 4.228, 4.344 | 최종 구성은 0, 0, 0 |
| 실행 끝 ISKM 트랙 20,215개 | 20,214.72 | `V0-s8`, 열 `iskm_tracks_last` | 20,214.72, 20,216.61, 20,212.92 | 중앙값 |
| 실행 끝 ISKM 트랙 30,797개 | 30,797.07 | `V0-a8pool`, 열 `iskm_tracks_last` | 30,797.03, 30,807.31, 30,797.07 | 중앙값 |

참고 측정(`Pool2`)의 행이다. 라벨은 `Pool2-V0-s8-N10000-P1-r1`(풀 켬)과 `Pool2-V0-a8pool-N10000-P1-r1`(풀 끔)이고, 실행이 하나씩이다.

| 본문의 수치 | 정밀한 값 | 출처 | 값 | 계산 |
| --- | --- | --- | --- | --- |
| 960초씩 한 번 | 960 | `batch.json` `measure_sec`, `runs_per_config` 1 | 두 실행 OK, 실패 0 | |
| 풀 끔 트랙 분당 약 6,850개 | 6,847.8 | `timeline.csv` `iskm_tracks_end` | 1분 17,711 / 16분 120,428 | (120,428 − 17,711) ÷ 15 |
| 마지막 구간 트랙 120,428개 | 120,428 | 풀 끔 16분 구간 | | |
| 풀 켬 3분 구간 뒤 분당 약 70개 | 70.5 | 풀 켬 `iskm_tracks_end` | 3분 20,232 / 16분 21,149 | (21,149 − 20,232) ÷ 13 |
| 풀 켬 트랙 21,149개, GPU 메모리 3,371MB | 21,149, 3,370.6 | 풀 켬 16분 구간 | | 그림 4의 끝 값. MB는 정수로 반올림 |
| 12분 구간의 계단, 5,105MB | 3,796.8 → 4,690.8 → 5,104.5, 16분 5,105.3 | 풀 끔 `gpu_mem_mean` | 11분, 12분, 13분 구간 | 5,105.3을 정수로 반올림 |
| 13분 구간부터 0.6ms 넘게 높다 | 1.138, 0.730, 0.621, 0.766 | `frame_p95`, 13\~16분 구간 | 풀 끔 − 풀 켬 | 가장 작은 차이 0.621 |
| 풀 끔 GPU 시간 6.08 → 7.22ms | 6.076 → 7.216 | 풀 끔 `gpu_p50` | 1분, 16분 구간 | |

설정값과 엔진·프로젝트 사실의 행이다. 프로젝트 코드의 줄은 측정 패키지 커밋 de0e21b의 것이다.

| 본문의 수치 | 값 | 출처 | 비고 |
| --- | --- | --- | --- |
| 80m 밖 그림자 끔, 중요도 Low는 80m부터 | 표현 LOD Medium/Low 경계 80m | `LabConfig.cpp` 58, `LabCrowdSubsystem.cpp` 179\~184, [measurement.md](../../Docs/Guides/measurement.md) "확정값" 거리 밴드 | 엔진 중요도 [2, 3)이 Low(engine-notes 22절, `MassLODCalculator.h` 409\~416, 745\~753) |
| 200m 밖 입자의 그림자도 같은 스위치 | | [measurement.md](../../Docs/Guides/measurement.md) "확정값" 미니언 그림자 | |
| 35m 안 액터, 35\~200m ISKM, 200m 밖 입자 | 35m, 200m | [ADR-0021](../../Docs/Decisions/0021-unified-actor-cap-and-hydrate-distance.md), measurement.md "확정값" 거리 밴드 | 단계 6부터 35\~200m가 ISKM |
| Simulation LOD 80m, 200m | 8,000cm, 20,000cm | `LabConfig.h` 68\~74 `FLabSimLod`, `LabMassTraits.cpp` 40\~67 | 200m 밖은 Off(`LODDistance[Off]`) |
| 자체 로직 0.5초, Off 이동 1초 | 0.5, 1.0 | `FLabSimLod::TickMedium`, `TickLow`, `TickRates[Off] = TickLow`(`LabMassTraits.cpp` 66) | CSV 메타데이터 `sim_lod` "80/200m tick=0/0.50/1.00" |
| 접근과 범위 피해만 간격을 따른다 | | `LabMassProcessors.cpp` 89(접근), 217(Off 이동), 436(범위 피해)의 `LabUseVariableTick` | |
| 바운드 배율 1.1 | 1.1 | `LabConfig.h` 31 `FLabQuality::FixedSkelBoundsScale`, `LabMinion.cpp` 272\~277 | 단계 8만 켬(`LabConfig.cpp` 59) |
| 최소 유지 시간 2초 | 2,000ms | `LabConfig.h` 23 `DefaultRepHoldMs`, `summary.csv` `rep_hold_ms` 2000 | 끈 조건은 0 |
| 왕복 전환: 2초 안에 같은 경계를 되넘은 수 | | [measurement.md](../../Docs/Guides/measurement.md) 지표 표 `sw_round*` | |
| 200m 밖을 그리지 않는 일을 끄면 맵 끝까지 ISKM으로 그린다 | 표현 LOD Off 거리 200m → 1,000m | [용어집](../../Docs/Guides/glossary.md) 5.2절 `a8far`(`LabConfig.h` 354, 410\~418) | 200m 밖은 ISKM Low(그림자 끔). Simulation LOD의 200m 경계는 그대로다 |
| 한 실행은 240초 | 60 + 180 | measurement.md "벤치마크 조건" | |
| 0.5ms 규칙 | 0.5ms | measurement.md "차이 판정" | |
| 액터 메시의 애니메이션 시간 | `budgeted_anim_ms` | `Scripts/summarize.py` 47, 230\~232 | 엔진 CSV 스탯 `AnimationBudget/GameThread/BudgetedAnimation`의 실행별 중앙값 |
| 액터 애니메이션 예산 4ms | 4ms | [ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md), `summary.csv` `budget_ms` 4(메타데이터 `LabBudgetMs`) | 단계 4부터(`LabConfig.cpp` 50) |

## 그림의 값

`py -3 -I Posts/05-cost-cuts-and-contributions/make-figures.py`가 그림 1, 2, 4를 만든다. 환경 변수 `LAB_FIG_CHECK=1`을 주면 그림에 쓴 글자를 찍는다. 그림 3은 글 06의 도식을 상대 경로로 다시 쓴다.

### 그림 1 `images/steps-5-and-8.svg`

호드 총수 1만, 오브 시점 카메라, 이동 장면, 세 번 잰 중앙값(ms)이다. 프레임은 p95, 스레드는 p50이다. 굵은 글자는 구별되는 차이다.

| 단계 | 프레임 p95 | 게임 스레드 | 렌더 스레드 | GPU |
| --- | --- | --- | --- | --- |
| 단계 4 | 14.205 (14.086\~14.37) | 11.825 (11.699\~11.838) | 5.626 (5.502\~5.668) | 7.142 (7.113\~7.157) |
| 단계 5 | 14.516 (14.513\~14.7) | 12.065 (12.043\~12.204) | 5.377 (5.301\~5.39) | 7.154 (7.132\~7.19) |
| 5 − 4 | +0.31, 구별 안 됨 | +0.24, 구별 안 됨 | −0.25, 구별 안 됨 | +0.01, 구별 안 됨 |
| 단계 7 | 15.168 (15.091\~15.426) | 12.686 (12.686\~12.931) | 3.981 (3.977\~4.128) | 6.648 (6.624\~6.657) |
| 단계 8 | 14.231 (14.035\~14.426) | 11.881 (11.79\~12.023) | 4.12 (4.102\~4.125) | 6.164 (6.154\~6.18) |
| 8 − 7 | −0.94, 구별됨 | −0.81, 구별됨 | +0.14, 구별 안 됨 | −0.48, 구별 안 됨 |

네 단계의 `bottleneck` 열은 모두 game이다.

### 그림 2 `images/contributions.svg`

같은 조건이다. 차이는 최종 구성(14.231ms)과의 프레임 p95 차이다. 실행은 `runs_used`다.

| 그림의 행 | 프레임 p95 (범위) | 차이 | 구별 | 실행 |
| --- | --- | --- | --- | --- |
| 최종 구성(단계 8) `V0-s8` | 14.231 (14.035\~14.426) | | | r1 r2 r3 |
| 200m 밖을 그리지 않는 일 끔 `a8far` | 17.767 (17.76\~18.06) | +3.536 | 구별됨 | r2 r3 r7 |
| 액터 메시 고정 바운드 끔 `a8fsb` | 15.209 (15.177\~15.275) | +0.978 | 구별됨 | r1 r2 r3 |
| ISKM 고정 바운드 끔 `a8bnd` | 14.887 (14.682\~14.999) | +0.656 | 구별됨 | r1 r2 r3 |
| ISKM 끔 `a8iskm` | 13.559 (13.488\~13.711) | −0.672 | 구별됨 | r1 r2 r3 |
| 겹침 방지 끔 `a8sep` | 13.305 (13.047\~13.434) | −0.926 | 구별됨 | r1 r2 r3 |
| 액터 캡슐 끔 `a8cap` | 12.376 (12.365\~12.39) | −1.855 | 구별됨 | r1 r2 r3 |
| 하이드레이션 끔 `a8hyd` | 7.247 (7.236\~7.251) | −6.984 | 구별됨 | r5 r6 r21 |
| 입자의 오브 따라가기 끔 `a8fxf` | 14.564 (14.265\~14.672) | +0.333 | 구별 안 됨 | r1 r2 r3 |
| ISKM 상자 따라가기 끔 `a8stp` | 14.346 (14.252\~14.393) | +0.115 | 구별 안 됨 | r1 r2 r3 |
| 80m 밖 그림자 다시 켬 `a8shd` | 14.302 (14.06\~14.511) | +0.071 | 구별 안 됨 | r1 r2 r3 r7 |
| 엔티티 풀 끔 `a8pool` | 14.24 (14.088\~14.369) | +0.009 | 구별 안 됨 | r1 r2 r3 |
| ISKM 트랙 예약 끔 `a8res` | 14.239 (14.16\~14.436) | +0.008 | 구별 안 됨 | r1 r2 r3 |
| 전환 대책 끔 `a8fix` | 14.231 (14.072\~14.295) | 0 | 구별 안 됨 | r1 r2 r3 |
| 액터 애니메이션 예산 끔 `a8bud` | 14.042 (13.912\~14.078) | −0.189 | 구별 안 됨 | r1 r2 r3 |
| 최소 유지 시간 끔 `a8hold` | 14.021 (13.901\~14.1) | −0.21 | 구별 안 됨 | r1 r2 r3 |
| Simulation LOD 끔 `a8sim` | 13.996 (13.897\~14.15) | −0.235 | 구별 안 됨 | r1 r2 r3 |
| 배경 무리 끔 `a8fx` | 13.922 (13.785\~14.023) | −0.309 | 구별 안 됨 | r1 r2 r3 |
| 엔진 회피로 바꿈 `s8av` | 22.803 (22.616\~23.169) | +8.572 | 구별됨 | r1 r2 r3 |

행 아래 글의 값이다. 최종 구성의 값은 GPU 6.164ms, 겹친 쌍 233.53, 왕복 전환 0, 트랙 20,214.72다.

| 행 | 그림의 글 | 정밀한 값 | 열 | 실행별 값 |
| --- | --- | --- | --- | --- |
| `a8hyd` | 게임 스레드 5.23ms, GPU 6.21ms | 5.23, 6.211 | `game_p50_ms`, `gpu_p50_ms` | 5.23, 5.227, 5.35 / 6.211, 6.23, 6.2 |
| `a8iskm` | GPU 시간 +1.24ms | 7.406 − 6.164 = 1.242 | `gpu_p50_ms` | 7.395, 7.407, 7.406 |
| `a8sep` | 겹친 쌍 234 → 1,644 | 233.53 → 1,643.83 | `overlap_mean` | 1,643.83, 1,643.89, 1,643.64 |
| `a8hold` | 왕복 전환 초당 0 → 4.34건 | 0 → 4.344 | `sw_round_all` | 4.5, 4.228, 4.344 |
| `a8pool` | 실행 끝 트랙 20,215 → 30,797 | 20,214.72 → 30,797.07 | `iskm_tracks_last` | 30,797.03, 30,807.31, 30,797.07 |
| `a8shd` | GPU 시간 +0.48ms | 6.646 − 6.164 = 0.482 | `gpu_p50_ms` | 6.648, 6.646, 6.632, 6.645 |
| `a8fx` | GPU 시간 −0.62ms | 5.542 − 6.164 = −0.622 | `gpu_p50_ms` | 5.546, 5.542, 5.528 |

- GPU 차이의 구별: `a8iskm`(범위 0.012, 0.026)과 `a8fx`(0.018, 0.026)는 0.5ms 이상이라 구별된다. `a8shd`는 0.482ms라 범위(0.016, 0.026)를 넘어도 0.5ms 아래라 구별되지 않는다.
- `a8shd`는 네 실행의 중앙값이다. r1이 느린 실행이라 r7로 다시 쟀고 r7도 느려 둘 다 요약에 들어갔다(`summary.csv` `slow_note` "slow again r1").
- `a8far`의 r1은 느린 실행이라 r7로 바꿨다(`slow_note` "replaced r1").
- `a8hyd`는 r5, r6, r21이다. r1\~r4는 실패했고 r21은 두 번째 재측정이다. r21은 나머지보다 약 4시간 뒤에 쟀다([Worklog/V2.md](../../Docs/Worklog/V2.md) "2026-11-17 Full4 a8hyd 보충 측정").

다른 것이 함께 빠지는 행(그림의 †)은 [measurement.md](../../Docs/Guides/measurement.md) "변수가 둘 이상 섞인 행"과 [용어집](../../Docs/Guides/glossary.md) 5.2절이다.

| 행 | 함께 빠지는 것 | 근거 |
| --- | --- | --- |
| `a8hyd`(`-LabHydrate=0`) | 액터 캡슐, 액터 애니메이션 예산, 액터 메시 고정 바운드. 전 구간이 ISKM | measurement.md 187행 |
| `a8iskm`(`-LabIskm=0`) | ISKM 고정 바운드, 트랙 예약, 먼 피격 규칙 `-LabFarHitIskm` | measurement.md 188행 |
| `a8fx`(`-LabCrowdFx=0`) | 먼 피격 규칙 `-LabFarHitIskm`, 맵 밖 무리 그리기 | measurement.md 189행 |

기법을 끄면 그 자리에 들어가는 것이다. 근거는 [용어집](../../Docs/Guides/glossary.md) 5.2절과 [commands.md](../../Docs/Guides/commands.md)의 인자 표다.

| 그림의 행 | 끄면 대신 들어가는 것 | 인자 |
| --- | --- | --- |
| 200m 밖을 그리지 않는 일 끔 `a8far` | 200m 밖 엔티티를 맵 끝까지 그림자를 끈 ISKM으로 그린다. 입자는 맵 밖 무리만 그린다 | `-LabFarCull=0` |
| 액터 메시 고정 바운드 끔 `a8fsb` | 액터 메시가 애니메이션을 갱신할 때마다 피직스 에셋의 바디로 바운드를 다시 셈한다 | `-LabFixedSkelBounds=0` |
| ISKM 고정 바운드 끔 `a8bnd` | 엔진이 ISKM 컴포넌트의 바운드를 셈할 때 인스턴스를 모두 돈다 | `-LabFixedBounds=0` |
| ISKM 끔 `a8iskm` | 35\~200m를 VAT 인스턴스로 그린다 | `-LabIskm=0` |
| 겹침 방지 끔 `a8sep` | 개체를 서로 떼어 놓지 않는다. 겹친 쌍이 늘어 같은 일이 아니다 | `-LabSeparate=0` |
| 액터 캡슐 끔 `a8cap` | 가까운 액터가 캡슐 콜리전 없이 그려진다 | `-LabActorCollision=0` |
| 하이드레이션 끔 `a8hyd` | 가까운 개체도 액터 없이 ISKM으로 그린다 | `-LabHydrate=0` |
| 입자의 오브 따라가기 끔 `a8fxf` | 입자가 향할 오브의 자리를 매 프레임 넣지 않고, 시작 전에 한 번만 넣는다 | `-LabFxOrbFollow=0` |
| ISKM 상자 따라가기 끔 `a8stp` | 고정 바운드 상자를 카메라로 옮기지 않고 처음 오브 자리 둘레에 둔다 | `-LabBoundsStepM=0` |
| 80m 밖 그림자 다시 켬 `a8shd` | 80m 밖 ISKM 인스턴스와 200m 밖 입자도 그림자를 그린다 | `-LabFarShadow=1` |
| 엔티티 풀 끔 `a8pool` | 죽은 개체의 엔티티를 파괴하고 새로 만든다 | `-LabEntityPool=0` |
| ISKM 트랙 예약 끔 `a8res` | 트랙 풀을 처음에 용량까지 늘려 두지 않는다 | `-LabIskmTrackReserve=0` |
| 전환 대책 끔 `a8fix` | 인스턴스와 액터가 바뀌는 프레임의 보정 다섯을 모두 끈다 | `-LabHydrateFixMask=0` |
| 액터 애니메이션 예산 끔 `a8bud` | 액터 메시가 `USkeletalMeshComponentBudgeted` 대신 `USkeletalMeshComponent`다. 메시가 매 프레임 애니메이션을 평가한다 | `-LabScreenSigBudget=0` |
| 최소 유지 시간 끔 `a8hold` | 바뀐 개체가 2초 안에 바로 전의 표현으로 되돌아갈 수 있다 | `-LabRepHold=0` |
| Simulation LOD 끔 `a8sim` | 80m 밖 개체도 접근·범위 피해 로직을 매 프레임 돌리고, 200m 밖 개체도 엔진의 이동·조향 프로세서로 옮긴다 | `-LabSimLod=0` |
| 배경 무리 끔 `a8fx` | 200m 밖에 아무것도 그리지 않는다 | `-LabCrowdFx=0` |
| 엔진 회피로 바꿈 `s8av` | 분리 프로세서 대신 엔진 회피를 쓴다. 끈 것이 아니라 바꾼 조건이다 | `-LabAvoid=1 -LabSeparate=0` |

### 그림 3 `../06-engine-findings/images/entity-pool.svg`

글 06의 도식(커밋 583a770, 그 글의 `make-figures.py`가 만든다)을 그대로 링크했다. 계획안([V2-w14-plan.md](../../Docs/Worklog/V2-w14-plan.md) 3.2절)이 글 05와 06에 같은 도식을 쓰게 한다. 수치가 없는 도식이고, 근거는 아래 "엔진 소스와 코드 위치"다. "A의 트랙은 남는다"는 `EndVisualChanges`의 지우기 반복이 트랙 맵을 건드리지 않는 것이다. 풀 쪽은 `ULabCrowdSubsystem::RecycleAndRespawn`과 `SpawnEntities`의 순서다. 실제로는 방금 죽은 엔티티를 같은 플러시에서 바로 다시 꺼내고, 남은 것만 맵 밖 먼 곳(`LabPoolParkLocation`)에 세운다.

### 그림 4 `images/pool-long-run.svg`

참고 측정 `Pool2`다. 최종 구성, 호드 총수 1만, 오브 시점 카메라, 이동 장면, 측정 960초를 1분 구간으로 나눴다. 실행이 조건마다 하나라 범위가 없다. 그림의 글자는 16분 구간의 값 여섯이다. 트랙 수는 개수라 줄이지 않고, GPU 메모리는 정수로 반올림하고(3,370.6 → 3,371, 5,105.3 → 5,105) 프레임 p95는 유효 숫자 세 자리로 적었다.

| 구간(분) | 트랙 풀 켬 | 트랙 풀 끔 | GPU 메모리 풀 켬(MB) | GPU 메모리 풀 끔(MB) |
| --- | --- | --- | --- | --- |
| 1 | 17,211 | 17,711 | 3,371.0 | 3,369.6 |
| 2 | 20,048 | 24,971 | 3,360.7 | 3,417.9 |
| 3 | 20,232 | 31,940 | 3,373.4 | 3,618.5 |
| 4 | 20,338 | 38,420 | 3,375.0 | 3,795.3 |
| 5 | 20,408 | 44,972 | 3,345.9 | 3,888.8 |
| 6 | 20,476 | 51,673 | 3,351.9 | 3,918.2 |
| 7 | 20,549 | 58,752 | 3,363.6 | 3,912.8 |
| 8 | 20,615 | 66,071 | 3,363.4 | 3,858.8 |
| 9 | 20,685 | 72,937 | 3,365.5 | 3,817.3 |
| 10 | 20,747 | 79,468 | 3,371.5 | 3,823.3 |
| 11 | 20,816 | 86,049 | 3,348.9 | 3,796.8 |
| 12 | 20,887 | 92,756 | 3,360.4 | 4,690.8 |
| 13 | 20,950 | 99,901 | 3,371.5 | 5,104.5 |
| 14 | 21,019 | 107,148 | 3,363.4 | 5,102.9 |
| 15 | 21,084 | 113,948 | 3,374.3 | 5,106.2 |
| 16 | 21,149 | 120,428 | 3,370.6 | 5,105.3 |

| 구간(분) | 프레임 p95 풀 켬 | 프레임 p95 풀 끔 | GPU p50 풀 켬 | GPU p50 풀 끔 |
| --- | --- | --- | --- | --- |
| 1 | 14.102 | 14.035 | 6.048 | 6.076 |
| 2 | 14.793 | 14.506 | 6.264 | 6.306 |
| 3 | 14.632 | 14.384 | 6.236 | 6.329 |
| 4 | 14.74 | 14.722 | 6.105 | 6.266 |
| 5 | 13.857 | 13.658 | 6.201 | 6.487 |
| 6 | 14.314 | 14.339 | 6.125 | 6.509 |
| 7 | 14.307 | 14.413 | 6.088 | 6.365 |
| 8 | 15.124 | 15.062 | 6.315 | 6.551 |
| 9 | 14.632 | 14.852 | 6.237 | 6.554 |
| 10 | 14.668 | 15.051 | 6.064 | 6.437 |
| 11 | 13.844 | 14.116 | 6.19 | 6.61 |
| 12 | 14.095 | 14.529 | 6.168 | 7.071 |
| 13 | 14.559 | 15.697 | 6.104 | 7.173 |
| 14 | 14.815 | 15.545 | 6.283 | 7.425 |
| 15 | 14.794 | 15.415 | 6.24 | 7.32 |
| 16 | 14.468 | 15.234 | 6.061 | 7.216 |

- 9\~12분 구간도 풀 끔이 0.22\~0.43ms 높다. 0.5ms 아래라 본문은 0.6ms를 넘는 13분 구간부터를 적었다.
- 960초는 궤도 5.33바퀴라 같은 번호의 구간은 두 실행에서 궤도 위상이 같다. 구간끼리의 흔들림(풀 켬 13.84\~15.12ms)은 궤도 위상을 따른다([Worklog/V2.md](../../Docs/Worklog/V2.md) "2026-11-17 엔티티 풀 긴 실행 러너").
- 실행 전체의 요약(`Pool2/summary.csv`)은 풀 켬 프레임 p95 14.584ms, 풀 끔 14.842ms다. 본문에는 쓰지 않았다.

## 단계 8 화면

`Docs/figures/steps/step8-final.webp`는 `Saved/Capture/Full4/Full4shot-V0-s8-N10000-P1-r1.webp`의 바이트 사본이다(960×540, 15fps, 8초, SHA-256 앞 12자 9185eb5551c8, [Worklog/V2.md](../../Docs/Worklog/V2.md) "2026-11-17 W14-3 단계 화면"). `capture-full.ps1`이 HUD를 켜고 워밍업 15초, 측정 40초로 찍었다. HUD 숫자는 측정값이 아니고 본문에 인용하지 않았다.

## 엔진 소스와 코드 위치

엔진은 소스 빌드 UE 5.8.4다(2026-11-17 확인). 줄임말: **MR** = `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation`.

| 본문의 주장 | 위치 | 확인한 것 |
| --- | --- | --- |
| 트랙 맵의 키는 엔티티 핸들, 없을 때만 새 트랙 | `MR/Private/MassVisualizationComponent.cpp` 761, 773(갱신), 852, 864(추가) | `TrackMap.FindOrAddId(핸들)`, 없으면 `ASTPDI->AllocateTrack()` |
| 지우기 반복은 트랙 맵을 건드리지 않는다 | 같은 파일 812\~827 | `SharedIdMap.RemoveByElementId`와 `RemoveInstances`만 부른다 |
| `DeallocateTrack`은 정지 표현 경로에서만 | 같은 파일 1126\~1171(1156), 1537\~1547(1544) | `ProcessRemovesForComponent`가 `EntitiesRequiringRemoval`만 본다. 이 배열은 `FMassLODInstancedSkinnedMeshSignificanceRange::RemoveInstance`만 채운다 |
| 위 `RemoveInstance`를 부르는 곳 | `MR/Private/MassStationaryISMSwitcherProcessor.cpp` 127, 264(쿼리 30), `MassStationaryISMRepresentationFragmentDestructor.cpp` 91(쿼리 28) | 두 프로세서의 쿼리가 `FMassStaticRepresentationTag`를 `All`로 요구한다 |
| 렌더러는 잡혀 있는 트랙을 매 프레임 평가 | `Engine/Source/Runtime/Renderer/Private/Skinning/AnimSequenceTransformProvider.cpp` 1456 | `Tracks.EnumerateActiveTracks`로 활성 트랙마다 갱신 요청을 만든다([engine-notes.md](../../Docs/Reference/engine-notes.md) 42절) |
| 그림자는 컴포넌트 단위 | `MR/Private/MassVisualizationComponent.cpp` 1337(ISKM), 259(ISM) | 설명자의 `bCastShadows`를 컴포넌트의 `SetCastShadow`로만 넘긴다(engine-notes 22절) |
| 컴포넌트마다 트랙이 따로 | 같은 파일 1323\~1325 | ASTPD면 컴포넌트마다 제공자 인스턴스를 만든다 |
| 애니 갱신마다 바운드를 다시 셈 | `Engine/Source/Runtime/Engine/Private/PhysicsEngine/PhysAnim.cpp` 494\~512 | `FinalizeAnimationUpdate`가 `InvalidateCachedBounds()` 뒤 `UpdateBounds()` |
| 그 계산이 애니메이션 시간 스탯 안에 든다 | `Engine/Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator/Private/SkeletalMeshComponentBudgeted.cpp` 94, 139, `Engine/Source/Runtime/Engine/Private/Components/SkeletalMeshComponent.cpp` 4980\~4989, 3181, 3354, 3360 | 스탯 `BudgetedAnimation`은 `TickComponent`와 `CompleteParallelAnimationEvaluation`를 잰다. 뒤의 것이 `PostAnimEvaluation` → `FinalizeAnimationUpdate`를 부른다 |
| 고정 바운드는 메시 에셋 바운드 + 루트 이동 | `Engine/Source/Runtime/Engine/Private/Components/SkinnedMeshComponent.cpp` 2250\~2252, 2271, 2300\~2301 | 고정이면 `GetBounds()`, 아니면 `PhysicsAsset->CalcAABB`, 끝에 `BoundsScale` |
| 엔진 이동·조향이 Off 개체를 뺀다 | `MassGameplay/Source/MassMovement/Private/MassMovementProcessors.cpp` 33, 87, `Engine/Plugins/AI/MassAI/Source/MassNavigation/Private/MassSteeringProcessors.cpp` 86 | 쿼리가 `FMassOffLODTag`를 `None`으로 요구한다(engine-notes 5절) |
| 청크 필터 | `MassGameplay/Source/MassLOD/Public/MassSimulationLOD.h` 68 | `ShouldTickChunkThisFrame` |

이 글의 엔진 코드 발췌는 `EndVisualChanges` 한 블록 13줄이다(761, 773, 812, 816\~817, 823\~824, 826, 827행과 `// ...` 넷). 글 하나의 엔진 코드 합계 30줄 제한 안이다.

프로젝트 코드 발췌(커밋 de0e21b):

| 발췌 | 위치 |
| --- | --- |
| ISKM 설명자 둘 | `Source/HordeOptLab/LabCrowdSubsystem.cpp` 179\~184(`ULabCrowdSubsystem::BuildEntityConfig`) |
| 고정 바운드 | `Source/HordeOptLab/LabMinion.cpp` 272\~277(`ALabMinionMassActor::BeginPlay`). 275행 주석은 뺐다 |
| 풀에서 꺼내기 | `Source/HordeOptLab/LabCrowdSubsystem.cpp` 264\~274(`ULabCrowdSubsystem::SpawnEntities`), 줄인 곳은 270\~273 |
| 상태 프로세서의 풀 분기 | `Source/HordeOptLab/LabMassProcessors.cpp` 1463\~1474(`ULabMinionStateProcessor::Execute`), 파괴 경로 1481 |

본문은 80m 밖 그림자 끄기를 단계 8 절의 한 문단으로 줄였다. 본문에서 뺀 설명(그림자는 컴포넌트 단위, 중요도 Low는 80m부터, ISKM 설명자 둘의 발췌)은 이 기록의 같은 이름 행과 [engine-notes.md](../../Docs/Reference/engine-notes.md) 22절에 남는다.

## 확인하지 않은 것

- 단계 5가 프레임 시간을 줄이지 못한 원인. 트레이스로 가르지 않았다. 본문에는 원인을 적지 않았다.
- 실행 끝 트랙 수(20,215)가 호드 총수의 약 두 배인 까닭. 종류마다 80m 안쪽과 바깥쪽 ISKM 설명자가 컴포넌트 둘이 되고(`LabCrowdSubsystem.cpp` 179\~184, 먼 피격 규칙의 설명자가 종류마다 하나 더 있다, 190\~206), 컴포넌트마다 트랙 맵이 따로이며(engine-notes 22절), `Lab/IskmTracks`가 컴포넌트의 트랙 맵 크기를 더한다(`LabIskmProcessor.cpp` 193). 80m 밖 그림자를 다시 켠 조건(설명자 하나)은 10,263.425로 최종 구성의 0.508배다. 코드 읽기와 이 비로 판단했고 컴포넌트별 트랙 수는 따로 세지 않았다. 본문에서는 뺐다.
- 풀을 켠 긴 실행에서 3분 구간 뒤에도 트랙이 분당 약 70개 느는 까닭. 종류가 맞는 풀 엔티티가 없어 새로 만든 엔티티(`PoolCreated`)로 추정한다. 세지 않았다.
- 풀을 끈 긴 실행의 12분 구간 GPU 메모리 계단이 무엇인지. 트랙 수에 맞춘 뼈 변환 버퍼의 재할당으로 추정한다(engine-notes 42절). 엔진 코드로 확인하지 않았다.
- 풀을 끈 실행의 GPU 시간 증가가 트랙 평가 때문인지. 트레이스로 가르지 않았다. 본문은 엔진 소스의 사실(활성 트랙 평가)과 잰 값을 나란히 적었다.
- 풀을 끈 총괄 조건에서 소형 ISKM 트랙 예약(9,964)을 넘었는지와 그 영향. measurement.md 190행의 추정이다. 실행 끝 트랙 30,797은 모든 컴포넌트의 합이다.
- 80m 밖 그림자 끔이 화면에서 보이는지, 고정 바운드(배율 1.1)에서 쓰러지는 자세가 화면 가장자리에서 잘리는지. 화면으로 보지 않았다([Worklog/V2.md](../../Docs/Worklog/V2.md) "W11" 확인하지 못한 것).
- 하이드레이션을 끈 조건의 몸통이 겹친 쌍(51.83)은 프레임 시간이 짧아 준 값이다. 분리는 프레임마다 한 번 위치를 고치므로 겹친 쌍이 프레임 시간을 따라 는다. 그래서 최종 구성(233.53)과 견주지 않았다(measurement.md 180행). 고정 시간 간격에서는 둘이 같다는 진단이 있다.
- 탑뷰와 정지 장면의 기법별 기여는 이 글에 쓰지 않았다. 정지 장면 값은 `Docs/figures/full4/full-ablation-still.csv`에 있다.
