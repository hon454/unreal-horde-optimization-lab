# 00 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓰고, 최대 N은 유효 숫자 둘로 쓴다([measurement.md](../../Docs/Guides/measurement.md) "지표").

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 총괄 묶음 `Full4`. 2026-11-16 14:50:12\~2026-11-17 08:52:12(`batch.json`의 `started`, `ended`, `resumes` 2) |
| 빌드 | Test 패키지 de0e21b(태그 `v2-full4`). CSV 메타데이터 `labbuild=de0e21bd64ac` |
| 다시 쓴 이유 | `v1`(태그 `v1`)의 글과 수치를 `v2` 결과로 다시 썼다. 오브가 움직이고([ADR-0017](../../Docs/Decisions/0017-roaming-orb-and-equal-function-ladder.md)) 모든 단계가 같은 일 넷을 하므로 `v1` 수치와 잇지 않는다 |
| 렌더 조건 | 2560×1440 창, `sg.*` 3(Epic), `r.VSync=0`, `t.MaxFPS=0`, `r.ScreenPercentage=100`, `r.DynamicRes.OperationMode=0` |
| 렌더 조건(계속) | Lumen `r.DynamicGlobalIlluminationMethod=1`, VSM `r.Shadow.Virtual.Enable=1`, TSR `r.AntiAliasingMethod=4`, `mass.FullyParallel=true` |
| 시나리오 | 2종, 시드 20261106, 오브 궤도 `LabOrbit` = `r57.3/v2/h6`(이동), 구간 `LabWindow` = `60+180`. 정지 장면은 워밍업 15초 + 180초 |
| 실행 수 | 계획 207회 모두 성공(`planned_runs_ok` 207, `planned_runs_missing` 0). 실패 9회는 모두 `a8hyd`이고 본문에 쓰지 않았다(경위는 [V2.md](../../Docs/Worklog/V2.md) "2026-11-17 총괄 묶음 Full4"와 "Full4 a8hyd 보충 측정") |
| 본문 조건의 회차 | `V0-s1`\~`V0-s8`, `V0-a8cap`은 r1, r2, r3이다. 같은 조건 다시 재기 `V0-s8-aa`는 r11, r12, r13이다(`summary.csv` `runs_used`). 버린 실행은 `Full4-V0-s1-N10000-P1-r0` 하나다 |
| 요약 파일 | [요약 표](../../Measurements/Full4/summary.csv)(세 실행 중앙값과 범위), [실행별 값](../../Measurements/Full4/runs.csv), [동등성 검사](../../Measurements/Full4/equivalence.csv), [최대 N 표](../../Docs/figures/full4/full-n-at-60fps.csv) |
| 측정 PC | Ryzen 9 5950X, RTX 4090 24GB, 64GB(2666MT/s), Windows 11, NVIDIA 621.14, UE 5.8.4 소스 빌드([pc-specs.md](../../Docs/Reference/pc-specs.md)) |

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 실행별 p50·p95는 `Scripts/summarize.py`가 측정 창 180초의 프레임 분포에서 선형 보간 백분위수(`percentile`)로 구한다. 요약은 세 실행 값의 중앙값(`statistics.median`)이고 범위는 세 실행의 최솟값\~최댓값이다.

## 본문의 수치와 출처

조건은 따로 적지 않으면 호드 총수 1만, 이동 장면(`V0`), 오브 시점 카메라(`P1`)다. "세 실행" 열은 r1 / r2 / r3 순서다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열 또는 식) | 세 실행 |
| --- | --- | --- | --- | --- |
| 호드 1만, 늘 1만 | 10000 | 사다리 여덟 조건 | `alive_min` = `alive_max` = 10000, 조건 표 `scenario.target_n` | |
| 60fps = 16.7ms | 16.667 | 계산 | 1000 ÷ 60. 그림과 판정의 선은 16.7(`Scripts/plot.py` `BUDGET_MS`) | |
| 세 번 재고 중앙값 | 3 | 모든 조건 | `runs`, measurement.md "벤치마크 조건" 반복 행 | |
| 최종 구성 프레임 p95 14.2ms | 14.231 | `V0-s8` | `frame_p95_ms` | 14.426 / 14.035 / 14.231 |
| 16.7ms까지의 여유 2.47ms | 2.469 | 계산 | 16.7 − 14.231 | |
| 판정의 0.6ms | 0.6 | 판정식 | [ADR-0019](../../Docs/Decisions/0019-step8-actor-cap-distance-and-fixed-skel-bounds.md) 결정 3 | |
| 판정값 14.8ms | 14.831 | 계산 | 14.231 + 0.6 ≤ 16.7. 러너 출력 "14.23 + 0.6 = 14.83ms" | |
| 반경 500m | 50000cm | 설정 | `Source/HordeOptLab/LabConfig.h:96` `FLabScenario::MapRadius` | |
| 90%, 10% | 일련번호 10의 배수가 대형 | 설정 | measurement.md "확정값" 2종 행, 시나리오 `-LabTypes=2` | |
| 3m/s | 300cm/s | 설정 | `LabConfig.h:104` `MoveSpeed` | |
| 반지름 57.3m, 2m/s | 5730cm, 200cm/s | 설정 | `LabConfig.h:115-116` | |
| 한 바퀴 180초 | 180.0 | 계산 | 2π × 57.3 ÷ 2(`FLabOrbit::PeriodSec`, `LabConfig.h:139`) | |
| 8m 안 피해 | 800cm | 설정 | `LabConfig.h:98` `ZoneRadius` | |
| 2초마다 쏜다 | 2.0초, 5발 | 설정 | `LabConfig.h:105-106`. 5발은 그림에만 쓴다 | |
| 맵 밖 무리 500\~1,000m | 50000, 100000cm | 설정 | `Source/HordeOptLab/LabFarStream.h:28-29`, [ADR-0020](../../Docs/Decisions/0020-far-stream-respawn-handoff.md) | |
| 고정 시드 | 20261106 | 설정 | `LabConfig.h:189`, 조건 표 `common_args` `-LabSeed=20261106` | |
| 오브 뒤 15m, 1분에 한 바퀴 | 1500cm, 60초 | 설정 | `LabConfig.h:110`, `:113` `OrbViewPeriod`. 높이 9m(`:111`)는 그림에만 쓴다 | |
| 오브 30m 안 | 수평 30m | 지표 | `near30_mean`(measurement.md "v2에서 더한 열") | |
| 기준과 1% 안 | 0.01 | 검사 규칙 | 조건 표 `equivalence.death_rate_tol`, `near30_tol` | |
| 겹친 쌍: 절반에서 두 배 | 0.5, 2.0 | 검사 규칙 | `overlap_rel_lo`, `overlap_rel_hi`. 1만 이동 장면의 범위는 118.2\~472.9(`equivalence.csv` `overlap_range`) | |
| 2초마다 액터 하나에 라인 트레이스 | 실행마다 90\~91회 | `V0-s8` | `capsule_traces`(180초 ÷ 2초 = 90 안팎). 시작점 오브 구체 중심, 끝점 대상 액터(`LabOrb.cpp:378-405`) | r1 / r2 / r3: 91 / 91 / 90 |
| 그 액터나 앞을 가린 다른 액터의 캡슐에 맞는가(빗나감 0) | `capsule_miss` 0 | 검사 규칙 | measurement.md "동등성 검사" 캡슐 행. 다른 캡슐에 막힌 것도 통과다(`LabOrb.cpp` `Hits`/`Blocked`). `V0-s8` r1 / r2 / r3의 `capsule_hits`는 22 / 23 / 20 | |
| 왕복 전환, 단계 4부터 초당 0.5 이하 | 0.5 | 검사 규칙 | `round_trip_max_per_sec`. 최소 유지 시간은 단계 4부터(`LabRepHold` 2000, commands.md 스위치 표) | |
| 단계 3\~8은 해당 항목을 모두 통과 | `pass` | `V0-s3`\~`V0-s8` | `equivalence.csv` 1만 `V0` 행. 단계 4\~8은 `ok`, 단계 3은 `report`(아래 주석) | |
| 기준과 0.2% 안 | 최대 0.13% | `V0-s3`\~`V0-s8` | `death_rate_dev_pct` −0.08\~+0.04, `near30_mean_dev_pct` −0.08\~+0.13 | |
| 단계 1과 2는 처음부터 값만 보고하는 조건 | `"equiv": "report"` | `s1`, `s2` | 조건 표 `Scripts/full-conditions.json`의 `s1`·`s2`, measurement.md "동등성 검사" | |
| 단계 1의 초당 사망 44.6% 적다, 단계 2의 30m 안 119.5% 많다 | −44.60%, +119.50% | `V0-s1`, `V0-s2` | `death_rate_dev_pct`, `near30_mean_dev_pct`. 아래 표 | |
| 워밍업 60초, 180초 | 60, 180 | 설정 | `Source/HordeOptLab/LabGameMode.h:18-19`, 조건 표 `scenario` | |
| 약 1만 4천 프레임 | 14080 | `V0-s8` | `runs.csv` `frames`의 중앙값 | 13915 / 14178 / 14080 |
| 약 700프레임 | 704 | 계산 | 14080 × 0.05 | |
| 게임 스레드 11.9ms | 11.881 | `V0-s8` | `game_p50_ms` | 12.023 / 11.79 / 11.881 |
| 프레임 p50 12.8ms | 12.77 | `V0-s8` | `frame_p50_ms` | 12.93 / 12.68 / 12.77 |
| 단계 6과 7의 차이 0.46ms | +0.460 | 계산 | 15.168(`V0-s7`) − 14.708(`V0-s6`). 큰 쪽 범위 0.335 | |
| 액터 캡슐을 끄면 1.86ms 짧다 | −1.855 | 계산 | 12.376(`V0-a8cap`) − 14.231. 큰 쪽 범위 0.391 | `V0-a8cap` 12.39 / 12.365 / 12.376 |
| 0.5ms 아래는 구별되지 않음 | 0.5 | 규칙 | measurement.md "차이 판정", [v2-spec.md](../../Docs/v2-spec.md) 7절(0.5ms 아래 주장은 5회) | |
| 최대 N 약 780 | 781 | `V0-s1` | `full-n-at-60fps.csv` "1 Actor baseline" `n_p95`. 식은 아래 | |
| 최대 N 약 4,100 | 4102 | `V0-s2` | 같은 파일 "2 Mass, engine default path" `n_p95` | |
| 주기적으로 튀는 프레임이 있는 실행 없음 | 0 | 측정 195회, 트레이스 15회 | `Scripts/spikes.py`. V2.md "Full4 a8hyd 보충 측정"의 러너 출력 | |
| 같은 조건 다시 재기 0.08ms | +0.079 | `V0-s8-aa` | 14.31 − 14.231. `V0-s8-aa` 범위 14.301\~14.378 | r11 / r12 / r13: 14.301 / 14.31 / 14.378 |
| 0.5ms 넘게 다르면 참고 | 0.5 | 규칙 | measurement.md "묶음 판정" A/A 행. `batch-check.py` 결과 `BATCH CHECK ok` | |
| 정지 장면 캡슐 빗나감 약 1.7% | 1.667% | 정지 14조건 | 빗나감 합 63 ÷ (270 × 14). `equivalence.csv` `capsule_miss`(`V1-*`, 캡슐 켬) | |
| 비용을 줄이는 단계(2, 3, 5, 8), 표현을 더하는 단계(4, 6, 7) | | | [v2-plan.md](../../Docs/v2-plan.md) "사다리", 조건 표 `role` | |
| 단계 2부터 하이드레이션 | `LabTechHydrate` 1 | 단계 2\~8 | commands.md "사다리 단계와 스위치" | |

단계 1과 2의 동등성 값(1만, 이동 장면, `equivalence.csv`):

| 조건 | 초당 사망 | 차이 | 30m 안 | 차이 |
| --- | --- | --- | --- | --- |
| 기준(단계 3\~8) | 59.261 | | 978.245 | |
| `V0-s1` | 32.833 | −44.60% | 796.35 | −18.59% |
| `V0-s2` | 53.928 | −9.00% | 2147.28 | +119.50% |

- 단계 3의 `pass`가 `report`인 까닭: 왕복 전환이 초당 4.32건이다. 단계 3은 최소 유지 시간이 없어 왕복 전환을 검사하지 않고 값만 보고한다(`round_ok` = `report`, 메모 "no min hold"). 나머지 항목은 `ok`다.
- 단계 1의 1만은 월드 시간이 프레임마다 잘려 측정 창 180초에 451프레임뿐이다(`runs.csv` `frames`). 그래서 같은 일을 하지 못한다. 자세한 것은 글 01이 다룬다.

최대 N의 식(`Scripts/plot.py` `n_at_budget`, 잰 N 사이를 로그 보간):

- 단계 1: 500에서 10.829, 1천에서 19.956. t = (16.7 − 10.829) ÷ (19.956 − 10.829) = 0.643. N = 500 × 2^0.643 = 781.
- 단계 2: 2,500에서 9.201, 5천에서 19.698. t = (16.7 − 9.201) ÷ (19.698 − 9.201) = 0.714. N = 2,500 × 2^0.714 = 4,102.

## 그림의 값

### `final-reading.svg`

`V0-s8`, 1만, `P1`, ms. 병목은 `summary.csv` `bottleneck` = `game`이다.

| 행 | 중앙값 | 범위 | 세 실행 |
| --- | --- | --- | --- |
| 프레임 p95 | 14.231 | 14.035\~14.426 | 14.426 / 14.035 / 14.231 |
| 프레임 p50 | 12.77 | 12.68\~12.93 | 12.93 / 12.68 / 12.77 |
| 게임 스레드 | 11.881 | 11.79\~12.023 | 12.023 / 11.79 / 11.881 |
| 렌더 스레드 | 4.12 | 4.102\~4.125 | 4.125 / 4.12 / 4.102 |
| GPU | 6.164 | 6.154\~6.18 | 6.18 / 6.164 / 6.154 |

그림의 "여유 2.47"은 16.7 − 14.231, "14.8"은 14.231 + 0.6이다.

### `judging-difference.svg`

프레임 p95(`frame_p95_ms`), 1만, `P1`, ms. 판정은 스크립트가 차이 판정 규칙(큰 쪽 범위보다 크고 0.5ms 이상)으로 계산한다.

| 행 | 조건 | 중앙값 | 범위 | 판정 |
| --- | --- | --- | --- | --- |
| 최종 구성 | `V0-s8` | 14.231 | 14.035\~14.426(0.391) | 기준 |
| 측정 끝에 다시 잰 최종 구성 | `V0-s8-aa` | 14.31 | 14.301\~14.378(0.077) | +0.079 < 0.391, 구별되지 않음 |
| 단계 6 ISKM | `V0-s6` | 14.708 | 14.695\~14.715(0.020) | 기준 |
| 단계 7 배경 무리 | `V0-s7` | 15.168 | 15.091\~15.426(0.335) | +0.460 > 0.335, 0.5 아래라 구별되지 않음 |
| 액터 캡슐을 끈 조건 | `V0-a8cap` | 12.376 | 12.365\~12.39(0.025) | −1.855, 0.391보다 크고 0.5 이상, 차이 있음 |

세 실행: `V0-s6` 14.695 / 14.715 / 14.708, `V0-s7` 15.426 / 15.168 / 15.091.

### `max-entities-method.svg`

프레임 p95(`frame_p95_ms`), 이동 장면, `P1`, ms. 단계 1의 1만(665.254)은 그림 밖이라 주석에만 적었다.

| 점 | 중앙값 | 세 실행 |
| --- | --- | --- |
| 단계 1, 500 | 10.829 | 10.829 / 10.768 / 10.832 |
| 단계 1, 1천 | 19.956 | 20.155 / 19.783 / 19.956 |
| 단계 1, 2천 | 41.252 | 41.132 / 41.252 / 41.571 |
| 단계 1, 1만 | 665.254 | 665.254 / 668.172 / 653.996 |
| 단계 2, 2,500 | 9.201 | 9.331 / 9.201 / 9.192 |
| 단계 2, 5천 | 19.698 | 19.517 / 19.869 / 19.698 |
| 단계 2, 1만 | 36.785 | 37.212 / 36.694 / 36.785 |

그림의 "20"은 19.956을 세 자리로 줄인 20.0에서 끝의 0을 지운 값이다. 교차점 781과 4,102는 `full-n-at-60fps.csv`에서 읽는다.

### `scenario.svg`

측정값은 없고 설정값만 그렸다. 값의 출처는 위 표의 시나리오 행이다. 점의 자리는 시드 20261106의 난수로 그린 그림이고 측정 실행의 배치가 아니다. 반지름을 고르게 뽑아 밀도가 1/r을 따르게 했다(measurement.md "시나리오"). 축척과 궤도의 돌림 방향은 개념이다.

### `frame-pipeline.svg`

개념도다. 칸의 길이(게임 스레드 4, 렌더 스레드 3, GPU 7)는 지어낸 값이고 그림 안에 그렇게 적었다.

### `stage-map.svg`

측정값은 없고 설정값만 그렸다.

| 그린 것 | 값 | 출처 |
| --- | --- | --- |
| 단계마다 켠 기법 | `LabTech*`, `lab_ism_processor` | commands.md "사다리 단계와 스위치(v2 W10)" 표 |
| 액터 구간 35m, 상한 1,000(단계 2\~8) | `LabHydrateM` 35, `LabActorCap` 1000 | [ADR-0021](../../Docs/Decisions/0021-unified-actor-cap-and-hydrate-distance.md), `LabCrowdSubsystem.cpp:206-209` |
| 80m, 200m 경계 | 8000cm, `RepOffCm()` 기본 200m | `LabCrowdSubsystem.cpp:208` `Bands` |
| 화면 밖은 15m 안만 액터 | 1500cm | `LabCrowdSubsystem.cpp:219` `BaseLODDistance[Medium]` |
| 단계 2·3 정지 메시, 4·5 VAT, 6부터 ISKM | 메시 설명자 | `LabCrowdSubsystem.cpp:168` 주석, commands.md `lab_ism_processor` 행 |
| 단계 7부터 200m 밖과 맵 밖 입자 | `LabTechCrowdFx` 1 | commands.md 스위치 표, [code-tour.md](../../Docs/Guides/code-tour.md) "입자 갱신" |
| 단계 8의 80m 밖 그림자 끔(입자 포함) | `LabFarShadow` 0 | commands.md 스위치 표, measurement.md "품질 계획에서 바뀌는 값" 미니언 그림자 행 |

### 최종 구성의 화면 `step8.jpg`

`Measurements/Full4/shots/step8.jpg`는 촬영 실행 `Full4shot-V0-s8-N10000-P1-r1`의 움직이는 WebP에서 120프레임 가운데 60번 프레임을 뽑은 것이다([V2.md](../../Docs/Worklog/V2.md) "2026-11-17 W14-3 단계 화면", [captures.csv](../../Measurements/Full4/captures.csv)). HUD를 켜고 녹화와 함께 돈 실행이라 HUD의 숫자는 측정값이 아니다. 본문은 화면의 숫자를 인용하지 않는다.

SVG 여섯은 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/00-goals-and-method/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽는다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다.

## 경위와 장치의 근거

| 본문의 말 | 출처 |
| --- | --- |
| 러너는 에디터, 빌드 도구, 브라우저가 떠 있으면 시작하지 않는다 | `Scripts/run-full.ps1:218-220`(`$Blockers`: 에디터, 게임, `ShaderCompileWorker`, `UnrealBuildTool`, chrome, msedge, firefox, ffmpeg) |
| 게임 창이 앞에 없던 실행은 버리고 다시 잰다 | `run-full.ps1:340-346`(2초마다 앞 창 표본), `:356-375`(측정 구간 안에 게임이 아닌 표본이 있으면 실패). measurement.md "측정 중 PC 상태" 행 |
| 0.6ms는 실행 하나가 중앙값보다 길게 나올 수 있는 폭이다 | measurement.md "지표" 목표 판정. 근거 값은 앞선 작은 묶음에서 나와 본문에 수치를 쓰지 않았다 |
| 겹친 쌍은 프레임 시간을 따라 는다 | V2.md "2026-11-17 Full4 캡슐 빗나감 진단"의 "부차" 절. 밀어내기는 프레임마다 위치를 한 번 고친다(`Source/HordeOptLab/LabSeparation.h:20-34` 주석). 이 묶음의 예: `V0-a8hyd` 프레임 p50 6.595ms에서 51.83, `V0-s8` 12.77ms에서 233.53(`overlap_mean`) |
| 정지 장면의 캡슐 빗나감은 장치의 결함이다 | V2.md "2026-11-17 Full4 캡슐 빗나감 진단"의 "원인" 절, measurement.md "실패 등급", 조건 표 `capsule_miss_basis` |
| 단계 2는 Mass Avoidance로, 나머지 단계는 밀어내기로 겹치지 않게 한다 | 조건 표 `s2` `LabTechAvoid` 1, `LabTechSeparate` 0. 다른 단계는 `LabTechSeparate` 1. 밀어내기는 반지름 합 × 1.2까지 뗀다(`Source/HordeOptLab/LabSeparation.h:31-34` `Skin = 0.2f`) |
| 그림 `stage-map.svg`의 200m 밖은 그리지 않는다(단계 2\~6) | `Source/HordeOptLab/LabCrowdSubsystem.cpp:216` `LODRepresentation[EMassLOD::Off] = None`. 단계 7부터는 200m 밖을 입자로 그린다(위 `stage-map.svg` 표) |
| 스레드 시간은 p50만 쓴다 | measurement.md "지표"의 스레드 시간 행(대기가 섞여 p95 해석이 어렵다) |
| 오브가 움직이면 고정 목표에서만 통하는 기법이 드러난다 | ADR-0017 "맥락"과 결정 3, [v2-spec.md](../../Docs/v2-spec.md) 4.2절 |

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다(2026-11-17). 경로는 `Engine/` 아래다.

| 사실 | 위치 |
| --- | --- |
| CSV의 `GameThreadTime`, `RenderThreadTime`, `GPUTime`은 `GGameThreadTime`, `GRenderThreadTime`, `RHIGetGPUFrameCycles()`다 | `Source/Runtime/Launch/Private/LaunchEngineLoop.cpp:1518-1520` `UpdateCoreCsvStats_EndFrame` |
| CSV의 `FrameTime`은 앞 프레임의 같은 지점부터 흐른 실제 시간이다 | `Source/Runtime/Core/Private/ProfilingDebugging/CsvProfiler.cpp:4383-4389` |
| 게임 스레드 시간 = 흐른 시간 − `Waits` | `Source/Runtime/Engine/Private/UnrealClient.cpp:1842-1844` `FViewport::Draw` |
| 게임 스레드는 N−1 렌더 스레드 프레임과 맞추고, 추가 울타리가 너무 앞서지 않게 한다 | `Source/Runtime/RenderCore/Private/RenderingThread.cpp:2458-2478` `FFrameEndSync`, 기본값 `r.OneFrameThreadLag`·`r.GTSyncType` `:2435-2450` |

본문의 엔진 코드 발췌는 `UnrealClient.cpp:1842-1844`의 3줄 하나다. 남긴 줄은 소스 그대로이고 앞쪽 들여쓰기만 줄였다.

## 확인하지 않은 것

- 단계 2는 프레임 p50(34.058ms)이 세 스레드 시간(게임 18.027, 렌더 7.294, GPU 12.23ms)보다 크게 길다(`V0-s2`). 본문의 "가장 느린 스레드가 프레임 시간을 정한다"가 단계 2에서는 맞지 않는다. 원인은 글 02가 트레이스로 봤다. 게임 스레드가 작업자 스레드 하나의 Mass Avoidance를 기다린 시간이 프레임을 늘린다. 기다림이 모두 Mass Avoidance 때문인지는 프레임마다 맞춰 보지 않았다(글 02 측정 기록).
- 사망 색 머티리얼을 그대로 둔 결정([V2-w14-plan.md](../../Docs/Worklog/V2-w14-plan.md) 6절의 다섯째 한계)은 결정 기록을 찾지 못해 본문에 넣지 않았다. measurement.md "확정값" 2종 행에는 단계 1 액터와 정지 메시 인스턴스의 대형에 사망 색이 없다고 적혀 있다.
- 겹친 쌍이 같은 시간 간격에서 같다는 확인(`s8` 208.2, `a8hyd` 208.3)은 Development 진단 실행의 값이라 본문에 쓰지 않았다.
- 정지 장면 빗나감의 원인은 Development 진단 실행으로 확인했다. Test 패키지에서는 비율(1.667%)만 같은 쪽임을 봤다.
- 그림의 HUD 숫자와 측정값의 차이는 재지 않았다.
- `a8hyd`의 보충 실행은 다른 조건보다 늦게 쟀지만 본문에 쓰지 않는 조건이다.
- SVG는 데스크톱 앱의 브라우저 창으로 열어 겹침과 잘림을 눈으로 봤다. 어두운 바탕 위에서는 보지 않았다(SVG가 자기 흰 배경을 가진다).
