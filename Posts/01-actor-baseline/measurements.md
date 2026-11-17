# 01 측정 기록: 액터 기준선

[본문](README.md)의 수치마다 정밀한 값, 출처, 실행별 값, 계산식을 둔다. 본문은 유효 숫자 세 자리로 줄여 쓰고, 최대 N은 유효 숫자 둘로 쓴다([measurement.md](../../Docs/Guides/measurement.md) "지표").

## 묶음과 조건

| 항목 | 값 |
| --- | --- |
| 묶음 | 총괄 묶음 `Full4`. 2026-11-16\~17(`Measurements/Full4/batch.json`) |
| 빌드 | Test 패키지 de0e21b(태그 `v2-full4`). CSV 메타데이터 `labbuild=de0e21bd64ac` |
| 다시 쓴 이유 | `v1` 글(태그 `v1`)을 `v2` 결과로 전면 재작성했다. `v1`은 캡슐끼리 막지 않고 오브가 서 있어 수치를 잇지 않는다([ADR-0017](../../Docs/Decisions/0017-roaming-orb-and-equal-function-ladder.md)) |
| 장면 | 이동 장면(`V0`), 오브 시점 카메라(`P1`), 2종, 시드 20261106, 워밍업 60초 + 측정 180초 |
| 단계 1 | 구성 `s1`(`-LabStage=1`). 호드 총수 1만은 묶음 종류 `ladder`, 500·1천·2천은 `actor` |
| Budget Allocator만 켠 조건 | 구성 `s1opt`(`-LabStage=1 -LabActorOpt=1`, 종류 `sub`). 묶음 종류 `actor`로 500·1천·2천만 쟀다 |
| 라벨 | `Full4-V0-<s1 또는 s1opt>-N<N>-P1-r<회차>`. 본문 조건은 모두 r1, r2, r3이다(`summary.csv` `runs_used`) |
| 버린 실행 | `Full4-V0-s1-N10000-P1-r0`(묶음 첫 실행) |
| 요약 파일 | [요약 표](../../Measurements/Full4/summary.csv), [실행별 값](../../Measurements/Full4/runs.csv), [동등성 검사](../../Measurements/Full4/equivalence.csv), [최대 N 표](../../Docs/figures/full4/full-n-at-60fps.csv) |

- `summary.csv`는 행을 `variant,n,preset`으로 찾는다. 값은 세 실행의 중앙값이고 범위는 최솟값\~최댓값이다.
- 측정 창 180초는 월드 시간이다. 러너가 `GetDeltaSeconds()`를 더해 잰다(`Source/HordeOptLab/LabGameMode.cpp:413`, de0e21b). 초당 사망도 이 시간으로 나눈다(`:537`).
- 구별 판정은 [measurement.md](../../Docs/Guides/measurement.md) "차이 판정"이다. 중앙값의 차이가 0.5ms 이상이고 두 조건 범위 가운데 큰 쪽보다 클 때만 구별된다. `make-figures.py`가 2천의 차이에 같은 판정을 다시 셈한다.
- 반올림: 절대값은 유효 숫자 세 자리, 차이(ms)는 소수 둘째 자리다. CSV 글자에서 반올림한다(half up).

## 본문의 수치와 출처

측정값의 행이다. 조건은 따로 적지 않으면 위 "묶음과 조건"이다. 실행 열은 r1, r2, r3 순서다.

| 본문의 수치 | 정밀한 값 | 출처 | 실행별 값 | 계산 |
| --- | --- | --- | --- | --- |
| 60fps = 16.7ms | 16.667 | 계산 | | 1000 ÷ 60. 그림과 최대 N의 선은 16.7(`Scripts/plot.py` `BUDGET_MS`) |
| 시나리오 카드의 2m/s, 호드 1만 | | [00 측정 기록](../00-goals-and-method/measurements.md) | | 모든 글이 같은 카드를 쓴다([posting.md](../../Docs/Guides/posting.md) "시나리오 카드") |
| 1만 프레임 p50 597ms | 597.108 | `V0-s1` 10000 `frame_p50_ms` | 599.009, 597.108, 594.037 | 중앙값 |
| 1만 병목은 게임 스레드 | game | `V0-s1` 10000 `bottleneck` | | 게임 580.969 > GPU 54.68 > 렌더 28.661 |
| 최대 N 약 780 | 781 | `full-n-at-60fps.csv` "1 Actor baseline" `n_p95` | | 아래 식 |
| Budget Allocator만 켠 액터 약 760 | 760 | 같은 파일 "1+ Animation Budget Allocator" `n_p95` | | 아래 식 |
| 예산 4ms | 4 | `V0-s1opt` `budget_ms`(메타데이터 `LabBudgetMs`) | 4, 4, 4 | 설정값 아래 표 |
| 게임 스레드 시간의 약 4분의 3 | 76.39% | 트레이스 | | 410.72 ÷ 537.63. 아래 "트레이스" |
| 1천 게임 스레드 17.2ms | 17.244 | `V0-s1` 1000 `game_p50_ms` | 17.258, 16.996, 17.244 | 중앙값 |
| 열 배 하면 172ms | 172.44 | 계산 | | 17.244 × 10 |
| 트레이스 한 프레임 538ms | 537.63 | 트레이스 | | `FEngineLoop::Tick` 49.999963초 ÷ 93 |
| CharacterMovement 틱 411ms | 410.72 | 트레이스 | | `UCharacterMovementComponent_TickComponent` Excl 38.197391초 ÷ 93 |
| 프레임마다 9,983번 | 9,982.7 | 트레이스 | | Count 928,391 ÷ 93 |
| 애니메이션 쪽 두 타이머 44.9ms | 44.88 | 트레이스 | | 26.91 + 17.97(아래 표) |
| 1만은 451프레임 | 451 | `runs.csv` `frames`(r1\~r3) | 451, 451, 451 | 월드 시간 180 ÷ 0.4 = 450, 첫 프레임 하나 더 |
| 초당 사망 32.8 | 32.833 | `equivalence.csv` 1만 `V0-s1` `death_rate` | 32.977, 32.766, 32.833 | 중앙값(`runs.csv` `death_rate`) |
| 기준보다 45% 적다 | −44.60% | `equivalence.csv` `death_rate_dev_pct` | | (32.833 − 59.261) ÷ 59.261. 기준 59.261은 단계 3\~8의 값(`death_rate_ref`) |
| 1만은 보고만 한다 | `report` | `equivalence.csv` `pass` | | 메모 "death_rate dev −44.60%; near30_mean dev −18.59%" |
| 1천 프레임 p95 20ms | 19.956 | `V0-s1` 1000 `frame_p95_ms` | 20.155, 19.783, 19.956 | 세 자리 20.0, 끝의 0을 지웠다 |
| 세 점은 월드 시간이 잘리지 않는다 | 2천 r1 4,620프레임 | `runs.csv` `frames` | | 180초 ÷ 4,620 = 39.0ms ≈ 프레임 p50 38.914. 세 점의 `frame_p99_ms` 최댓값은 43.37ms다 |
| 2천까지 18µs 안팎 | 17.38, 17.24, 18.45 | `V0-s1` `game_p50_ms` ÷ N | | 8.688 ÷ 500, 17.244 ÷ 1,000, 36.896 ÷ 2,000 |
| 이 구간은 프레임이 50ms 안 | 41.252 | `V0-s1` 2000 `frame_p95_ms` | | 2천의 프레임 p95와 p99(42.53)가 50ms 아래다 |
| 1만 58.1µs | 58.10 | `V0-s1` 10000 `game_p50_ms` ÷ N | 582.745, 580.969, 578.155 | 580.969 ÷ 10,000 |
| 예상 172ms 대신 581ms | 580.969 | `V0-s1` 10000 `game_p50_ms` | 위와 같다 | 3.37배 = 580.969 ÷ 172.44(본문에는 배수를 쓰지 않았다) |
| 2천 프레임 p95 41.3 → 36.9ms | 41.252 → 36.941 | `V0-s1`, `V0-s1opt` 2000 `frame_p95_ms` | s1 41.132, 41.252, 41.571 / s1opt 36.838, 36.941, 37.012 | 차이 −4.311. 범위 0.439, 0.174. 구별된다 |
| 1천에서 0.52ms 길다 | +0.516 | `V0-s1`, `V0-s1opt` 1000 `frame_p95_ms` | s1opt 20.472, 20.545, 20.454 | 20.472 − 19.956. 범위 0.372, 0.091. 구별된다 |
| 애니메이션 시간 1천 3.71ms | 3.711 | `V0-s1opt` 1000 `budgeted_anim_ms` | 3.707, 3.711, 3.755 | 중앙값 |
| 2천 4.17ms로 예산에 닿는다 | 4.165 | `V0-s1opt` 2000 `budgeted_anim_ms` | 4.168, 4.165, 4.162 | 4.165 ≥ 4 |
| 최대 N이 정해지는 500\~1천 | | `full-n-at-60fps.csv` `points` | | 16.7ms를 사이에 둔 두 점이 500과 1천이다(아래 식) |

설정값과 엔진·프로젝트 사실의 행이다. 프로젝트 코드의 줄은 측정 패키지 커밋 de0e21b의 것이다.

| 본문의 수치 | 값 | 출처 | 비고 |
| --- | --- | --- | --- |
| 하위 스텝 50ms, 여덟 번 | `MaxSimulationTimeStep = 0.05f`, `MaxSimulationIterations = 8` | `CharacterMovementComponent.cpp:698-699` | 단계 1은 이 값을 바꾸지 않는다(`Source/`, `Config/` grep 0건) |
| 월드 시간 상한 0.4초 | `MaxUndilatedFrameTime=0.4` | `Engine/Config/BaseGame.ini:199`, `WorldSettings.cpp:334-343` | 프로젝트 `Config/`가 덮지 않는다(grep 0건) |
| 1만에서 여덟 번 | 0.4 ÷ 0.05 = 8 | 계산 | 일곱 번째까지 `Min(0.05, 남은 시간 × 0.5)` = 0.05초, 여덟 번째는 남은 0.05초를 쓴다. 실행에서 세지 않았다 |
| 하위 스텝 설정은 엔진 기본값, 바꾼 것은 걷는 속도(3m/s)·진행 방향 보기·컨트롤러 없이 이동하기 셋, 오브 쪽 입력 | `MaxWalkSpeed` 300(엔진 기본 600), `bOrientRotationToMovement`, `bRunPhysicsWithNoController` | `Source/HordeOptLab/LabMinion.cpp:39-44`, 입력 `:165` `AddMovementInput` | 이 셋 말고는 엔진 기본값이다(`MaxSimulationTimeStep`, `MaxSimulationIterations` 포함) |
| 캡슐이 Pawn 채널을 막는다 | `ECR_Block` | `LabMinion.cpp:68`(`UsesActorBlocking`, `LabConfig.h:435`) | 조건 표 `s1`의 기대값 `LabTechSeparate` 1, `LabActorCollision` `pawn` |
| 캡슐 반지름 = 몸통 반지름 | 소형 40cm, 대형 94cm | `LabMinion.cpp:67`, `LabUnitAssets.cpp:45`(소형), `Config/DefaultGame.ini:41` `LargeBoundRadius=94` | 대형은 액터 배율 2(`DefaultGame.ini:54`)로 나눠 넣는다. 반높이 88은 엔진 기본값 |
| 스윕이 막히면 미끄러지고, 겹쳐 있으면 밀어낸다 | | [engine-notes.md](../../Docs/Reference/engine-notes.md) 39절 | `MoveAlongFloor`는 `SafeMoveUpdatedComponent(..., true, Hit)`로 스윕한다(`CharacterMovementComponent.cpp:5563`) |
| 메시를 `USkeletalMeshComponentBudgeted`로 바꾼다 | `SetDefaultSubobjectClass` | `LabMinion.cpp:192-199` `ALabMinionBudget`, 스폰 클래스 `LabCrowdSpawner.cpp:351` | |
| 예산 4ms | `FLabScenario::BudgetMs = 4` | `LabConfig.h:121`, [ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md), `LabGameMode.cpp:383-391` | `a.Budget.Enabled 1`, `a.Budget.BudgetMs 4`, 월드의 Budget Allocator `SetEnabled(true)` |
| 중요도 = 몸통 반지름 ÷ 카메라 거리 | | `LabMinion.cpp:207-214` `ALabMinionBudget::Tick` | |
| 두 조건의 차이는 Budget Allocator와 중요도를 액터 틱마다 셈하는 일 | | `LabMinion.cpp:201-216` `ALabMinionBudget::Tick`이 틱마다 카메라 거리로 `SetComponentSignificance`를 부른다 | 단계 1의 `ALabMinion`에는 이 계산이 없다 |
| 500\~1천에서 잰 애니메이션 시간은 예산 4ms 아래, 갱신 수는 세지 않았다 | 1.764, 3.711 < 4 | `V0-s1opt` `budgeted_anim_ms` | 갱신 수(`NumAlwaysTicked` CSV 스탯)는 `summary.csv`에 없다. "모든 메시를 매 프레임 갱신"은 엔진 식(`AnimationBudgetAllocator.cpp` `CalculateWorkDistributionAndQueue`)에서 나온 추론이다 |
| 예산 안이면 모두 매 프레임, 넘으면 낮은 중요도부터 간격을 늘린다 | | [engine-notes.md](../../Docs/Reference/engine-notes.md) 29절, `AnimationBudgetAllocator.cpp:561, 566-567` | 정렬은 중요도가 큰 순이다(`:327-342`) |
| 엔진은 등록할 때 URO를 끈다 | | `AnimationBudgetAllocator.cpp:1191-1197` `RegisterComponent`, 인터페이스 주석 `IAnimationBudgetAllocator.h:20-27` | [engine-notes.md](../../Docs/Reference/engine-notes.md) 52절 |
| 단계 1의 기본 메시도 URO를 켜지 않는다 | `bEnableUpdateRateOptimizations = false` | `SkeletalMeshComponent.cpp:493` | `ALabMinion`은 이 값을 바꾸지 않는다. `ALabMinionBudget`은 생성자에서 켜고, 엔진이 등록 때 끈다(`LabMinion.cpp:198`) |
| 단계 2에서 CharacterMovement의 틱이 없어진다 | | `LabMinion.cpp:224-238`(`ALabMinionPawn`이 틱을 끈다), 하이드레이션 액터 `ALabMinionMassActor`가 이를 물려받는다(`LabMinion.h:123`) | 다음 글 예고 |

최대 N의 식(`Scripts/plot.py` `n_at_budget`, 잰 N 사이를 로그 보간):

- 단계 1: 500에서 10.829, 1천에서 19.956. t = (16.7 − 10.829) ÷ (19.956 − 10.829) = 0.643. N = 500 × 2^0.643 = 781.
- Budget Allocator만 켠 액터: 500에서 10.946, 1천에서 20.472. t = (16.7 − 10.946) ÷ (20.472 − 10.946) = 0.604. N = 500 × 2^0.604 = 760.
- `make-figures.py`가 같은 식으로 다시 셈해 `full-n-at-60fps.csv`와 다르면 멈춘다.
- 동등성 검사에서 500\~2천의 여섯 점은 `report`다. 같은 총수에 견줄 Mass 단계가 없어서다(`equivalence.csv` 메모 "no peers"). 기능 미달(`UNMET`)이 아니라 최대 N 보간에서 빼지 않는다. 여섯 점 모두 살아 있는 총수가 N과 같고, 겹친 쌍이 0이고, 캡슐 빗나감이 0이다.

## 트레이스

| 항목 | 값 |
| --- | --- |
| 파일 | `Saved/Traces/Full4trace-V0-s1-N10000-P1-r1.utrace`(git 밖, 307,821,009바이트). 같은 패키지 de0e21b의 병렬 트레이스 실행이다 |
| 조건 | 이동 장면, 호드 총수 1만, 오브 시점 카메라, 워밍업 60초 + 측정 60초(월드 시간), `-trace=cpu,gpu,frame,counters,mass`(조건 표 `trace_args`) |
| 내보내기 | `Scripts/trace-export.ps1 -Label Full4trace-V0-s1-N10000-P1-r1 -Command TimerStatistics -Name w1410-s1-GT-70-120 -Extra "-threads=GameThread -startTime=70 -endTime=120 -columns=* -sortBy=TotalInclusiveTime"` |
| 결과 파일 | `%TEMP%\horde-trace\w1410-s1-GT-70-120.csv`(git 밖) |
| 구간 | 70\~120초. 글 02와 03이 쓴 구간과 같다. `FEngineLoop::Tick` 93개가 구간에 걸친다(온전한 프레임 91개와 양 끝의 잘린 프레임 둘) |
| 프레임당 값 | 타이머의 구간 합 ÷ 93. 글 02와 같은 셈법이고, 글 02의 값(한 프레임 537.63ms, CharacterMovement Excl 410.73ms, 76.4%, 9,983번)과 같다 |

트레이스의 프레임은 구간 안에서 약 530\~560ms다(`-Command TimingEvents -Extra "-threads=GameThread -timers=FEngineLoop::Tick"`, 결과 `w1410-s1-ticks.csv`). 트레이스를 켠 실행이라 측정 실행(프레임 p50 597ms)과 견주지 않고 비율만 읽는다.

| 그림의 칸 | 타이머 | 구간 합(초), Count | 프레임당(ms) | 게임 스레드 프레임의 몫 |
| --- | --- | --- | --- | --- |
| 게임 스레드 한 프레임 | `FEngineLoop::Tick` Incl | 49.999963, 93 | 537.63 | 100% |
| 1 걷게 하고 캡슐을 스윕으로 옮긴다 | `UCharacterMovementComponent_TickComponent` Excl | 38.197391, 928,391 | 410.72 | 76.39% |
| 2 AnimBP로 애니메이션 상태를 갱신한다 | `USkinnedMeshComponent_TickComponent` Incl | 2.502503, 921,266 | 26.91 | 5.01% |
| 3 병렬로 계산한 포즈를 메시에 반영한다 | `USkeletalMeshComponent_CompleteParallelAnimationEvaluation` Incl | 1.671582, 921,266 | 17.97 | 3.34% |
| 4 바뀐 상태를 렌더 스레드로 보낸다 | `UWorld_SendAllEndOfFrameUpdates` Incl | 0.411929, 92 | 4.43 | 0.82% |
| 나머지 | 537.63 − 위 넷 | | 77.60 | 14.43% |

- 칸 1은 글 02와 값을 맞추려고 Excl을 썼다. Incl은 38.320052초(프레임당 412.04ms, 76.64%)다. 차이 1.32ms는 CharacterMovement 틱 안의 이름 있는 타이머(`USceneComponent::UpdateOverlaps` 등)이고 "나머지"에 들어간다. 이 안의 하위 스텝, 스윕, 바닥 찾기는 이름 있는 타이머가 없어 나누지 못했다.
- 칸 2\~4는 Incl이다. 칸 2의 AnimBP 갱신이 이름 있는 하위 타이머이기 때문이다.
- 네 타이머는 서로 겹치지 않는다. `FActorComponentTickFunction::ExecuteTick` Incl 41.384121 = CharacterMovement Incl 38.320052 + 스켈레탈 메시 틱 2.502503 + 자기 시간 0.561566이다. 포즈 반영과 렌더 스레드로 보내기는 이 바깥에서 돈다.
- 스켈레탈 메시 틱 Incl − Excl = 2.502503 − 1.030092 = 1.472411초다. 이 안에 `USkeletalMeshComponent_TickAnimation` 1.392001초가 든다고 판단했다(부모 관계는 펼쳐 보지 않았다). 그래서 그림 칸 2를 "AnimBP로 애니메이션 상태를 갱신한다"로 적었다.
- 나머지 77.60ms에서 큰 것은 `ProcessUntilTasksComplete` Excl 37.86ms, `UWorld_Tick` Excl 13.30ms, `WorldTick` 10.71ms, `FActorComponentTickFunction::ExecuteTick` Excl 6.04ms다. 이름 없는 액터 틱과 틱 작업의 기다림이 섞여 그림에는 "틱 작업의 관리와 그 밖의 게임 스레드 일"로만 적었다.
- `-threads=GameThread`로 걸러도 GPU 타이머(`LumenScreenProbeGather` 등)와 `Frame` 행이 결과에 섞인다. 위 표에는 쓰지 않았다.

다른 구간으로 본 대조(같은 방법, CharacterMovement는 Incl):

| 구간 | 프레임 | 한 프레임(ms) | CharacterMovement Incl(ms) | 몫 |
| --- | --- | --- | --- | --- |
| 70\~120초(본문 구간) | 93(잘린 둘 포함) | 537.63 | 412.04 | 76.64% |
| 75\~120초 | 84(잘린 둘 포함) | 535.71 | 410.29 | 76.59% |
| 90.337923\~169.700410초(측정 구간 안의 온전한 프레임) | 143 | 554.98 | 430.62 | 77.59% |

- 트레이스 실행의 측정 구간은 87.6\~170.9초다. 월드 시간(프레임마다 0.4초로 잘린 값)을 첫 프레임부터 더해 60초가 되는 프레임(151번)과 120초가 되는 프레임(301번)으로 셈했다(`w1410-s1-ticks.csv`). 본문의 70\~120초는 워밍업 끝 17.6초를 포함한다.
- 측정 구간 안의 값은 결과 파일 `w1410-s1-GT-aligned.csv`다. Excl로는 77.35%이고, 호출은 프레임마다 10,013번이다(1,431,915 ÷ 143). 구간에 걸친 `FEngineLoop::Tick`은 144개이고 온전한 프레임은 143개다. 하나는 구간 끝에 걸친 조각이다(`I.Min` 0.000002초). 프레임당 값은 143으로 나눴다(Incl 79.362429초 ÷ 143 = 554.98ms). 세 구간 모두 "약 4분의 3"이다.

### Insights 화면

코디네이터가 2026-11-17에 찍어 본문 "문제" 절에 넣었다(`images/insights-gamethread.jpg`). 아래 표는 찍기 전에 적은 계획이고, 화면의 값은 표 아래에 있다.

| 항목 | 값 |
| --- | --- |
| 트레이스 | `Saved/Traces/Full4trace-V0-s1-N10000-P1-r1.utrace` |
| 볼 구간 | Timing 뷰에서 70\~120초를 고른다(글 02, 03의 화면과 같은 구간) |
| 볼 트랙 | `GameThread`. Timers 창을 "CharacterMovement"로 거른다. `FEngineLoop::Tick`이 함께 보이면 좋다 |
| 화면에 보여야 할 값 | `UCharacterMovementComponent_TickComponent` Count 928,391, Incl 38.320초, Excl 38.197초. `FEngineLoop::Tick` Count 93, Incl 50.000초. 손으로 구간을 고르면 한두 프레임 다를 수 있다 |
| 저장할 파일 | `Posts/01-actor-baseline/images/insights-gamethread.jpg`(jpg, 창 제목 줄은 잘라 낸다) |
| 넣은 뒤 | README의 주석을 풀고, 캡션 값을 화면 값(Incl 38.01초, Excl 37.89초)으로 맞췄다 |

화면의 값(시간 눈금자를 손으로 끌어 고른 구간 49.72초):

| 행 | Count | Incl | Excl | 내보낸 값과의 차이 |
| --- | --- | --- | --- | --- |
| `UCharacterMovementComponent_TickComponent` | 921,277 | 38.01초 | 37.89초 | Count −7,114(프레임 하나에 못 미친다), Incl −0.31초 |
| `FEngineLoop::Tick` | 93 | 49.72초 | | Incl −0.28초 |

- 화면 값으로 셈한 몫은 37.89 ÷ 49.72 = 76.2%다. 내보낸 값의 76.39%와 같은 "약 4분의 3"이다. 본문의 수치는 내보낸 값 그대로 두었다.
- 그림은 화면 왼쪽 절반과 오른쪽 절반을 확대 캡처해 붙인 것이다(2128폭). 창 제목 줄은 잘랐다. 아래 띠는 같은 구간에서 Timers 목록만 `FEngineLoop::Tick`으로 거르고 다시 찍어 붙였다. 빨간 상자는 붙인 뒤에 그렸다.

## 그림의 값

`py -3 -I Posts/01-actor-baseline/make-figures.py`가 SVG 넷을 만든다. 환경 변수 `LAB_FIG_CHECK=1`을 주면 그림에 쓴 글자를 찍는다. 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽고, 트레이스 값은 스크립트의 상수다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다.

### `images/step1-threads.svg`

`V0-s1`, 이동 장면, 오브 시점 카메라, ms. 두 총수 모두 `bottleneck`이 game이다.

| 행 | 1천 중앙값 | 1천 범위 | 1만 중앙값 | 1만 범위 |
| --- | --- | --- | --- | --- |
| 프레임 p95 | 19.956 | 19.783\~20.155 | 665.254 | 653.996\~668.172 |
| 프레임 p50 | 18.255 | 17.992\~18.27 | 597.108 | 594.037\~599.009 |
| 게임 스레드 | 17.244 | 16.996\~17.258 | 580.969 | 578.155\~582.745 |
| 렌더 스레드 | 3.812 | 3.768\~3.871 | 28.661 | 28.362\~28.931 |
| GPU | 6.162 | 6.122\~6.34 | 54.68 | 52.757\~54.775 |

그림의 "20"은 19.956을 세 자리로 줄인 20.0에서 끝의 0을 지운 값이다. 1천 프레임 p50의 18.3은 18.255의 half up이다.

### `images/actor-frame.svg`

값은 위 "트레이스"의 첫 표다. 그림의 백분율은 소수 첫째 자리이고 끝의 0은 지웠다(5.02 → 5). 아래 글의 "9,983번"은 928,391 ÷ 93이다. 칸 1의 막대는 Excl, 칸 2\~4는 Incl이다(위 "트레이스").

### `images/frame-p95-by-n.svg`

프레임 p95(`frame_p95_ms`), 이동 장면, 오브 시점 카메라, ms. 세로 선은 세 실행의 범위다.

| 호드 총수 | 단계 1 | 범위 | Budget Allocator만 켠 액터 | 범위 |
| --- | --- | --- | --- | --- |
| 500 | 10.829 | 10.768\~10.832 | 10.946 | 10.842\~11.07 |
| 1천 | 19.956 | 19.783\~20.155 | 20.472 | 20.454\~20.545 |
| 2천 | 41.252 | 41.132\~41.571 | 36.941 | 36.838\~37.012 |

| 그림의 다른 값 | 값 | 출처 |
| --- | --- | --- |
| 최대 N 781, 760 | 781, 760 | `full-n-at-60fps.csv` `n_p95` |
| 2천의 차이 −4.31 | −4.311 | 36.941 − 41.252, 구별된다 |
| 애니메이션 시간 1.76, 3.71, 4.17 | 1.764, 3.711, 4.165 | `V0-s1opt` `budgeted_anim_ms`(500 실행 1.764, 1.759, 1.774) |
| 단계 1의 1만 665ms | 665.254 | `V0-s1` 10000 `frame_p95_ms` |

- `budgeted_anim_ms`는 엔진 CSV 스탯 `AnimationBudget/GameThread/BudgetedAnimation`의 실행별 중앙값이다(`Scripts/summarize.py`). Budget Allocator에 등록한 메시의 틱과 포즈 반영이 게임 스레드에서 쓴 시간을 잰다([engine-notes.md](../../Docs/Reference/engine-notes.md) 29절 "메시 하나의 시간").
- 500의 차이(+0.117ms)는 범위(0.228) 안이라 구별되지 않는다. 본문에는 쓰지 않았다.

### `images/per-entity-cost.svg`

`V0-s1` `game_p50_ms` ÷ 호드 총수(µs). 하위 스텝 횟수는 위 설정값 표의 계산이다. 1만은 빗금으로 그렸다.

| 호드 총수 | 게임 스레드(ms) | 개체당(µs) | 하위 스텝 | 근거 |
| --- | --- | --- | --- | --- |
| 500 | 8.688 | 17.376 | 1회 | 프레임 p99 최댓값 11.343ms < 50ms |
| 1천 | 17.244 | 17.244 | 1회 | 20.825ms < 50ms |
| 2천 | 36.896 | 18.448 | 1회 | 43.37ms < 50ms |
| 1만 | 580.969 | 58.097 | 8회 | 월드 시간 상한 0.4초 |

예상선은 1천의 17.244µs다. 아래 글의 172ms는 17.244 × 10, 581ms는 580.969다. 2천은 실행마다 50ms를 넘는 프레임이 하나 있다(`hitch_abs` 1, 1, 1, 실행당 약 4,600프레임 가운데). 본문의 "하위 스텝이 한 번"은 이 하나를 뺀 프레임의 말이다.

## 단계 1 화면

`Measurements/Full4/shots/step1.jpg`는 촬영 실행 `Full4shot-V0-s1-N10000-P1-r1`의 움직이는 WebP(13프레임)에서 0부터 센 6번 프레임이다([Worklog/V2.md](../../Docs/Worklog/V2.md) "2026-11-17 W14-3 단계 화면", [captures.csv](../../Measurements/Full4/captures.csv)). HUD를 켜고 화면 녹화와 함께 돈 실행이라 HUD 숫자는 측정값이 아니다. 화면의 프레임 시간 748.9ms는 측정값 597ms보다 높고, 본문은 인용하지 않는다. 촬영 8초에 13프레임뿐이라 움직이는 그림 대신 정지 그림 한 장을 실었다([V2-w14-plan.md](../../Docs/Worklog/V2-w14-plan.md) 결정 3).

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다(2026-11-17). 경로는 `Engine/` 아래다.

| 사실 | 위치 |
| --- | --- |
| `MaxSimulationTimeStep = 0.05f`, `MaxSimulationIterations = 8` | `Source/Runtime/Engine/Private/Components/CharacterMovementComponent.cpp:698-699` |
| 나누는 규칙, 마지막 회차는 남은 시간을 모두 쓴다 | 같은 파일 `GetSimulationTimeStep` `:8162-8187` |
| 걷기 루프: `Iterations < MaxSimulationIterations`까지 돌고 스텝마다 `GetSimulationTimeStep`, `MoveAlongFloor`, `FindFloor` | 같은 파일 `PhysWalking` `:5653`, `while` `:5686`, `Iterations++` `:5688`, `:5690-5691`, `MoveAlongFloor` `:5744`, `FindFloor` `:5774` |
| `MoveAlongFloor`는 스윕으로 옮긴다 | 같은 파일 `:5552`, `SafeMoveUpdatedComponent(..., true, Hit)` `:5563` |
| 월드 델타를 `MaxUndilatedFrameTime`(시간 팽창을 곱한 값)으로 자른다 | `Source/Runtime/Engine/Private/WorldSettings.cpp:334-343` `FixupDeltaSeconds`, 부르는 곳 `LevelTick.cpp:1599` |
| `MaxUndilatedFrameTime=0.4` | `Config/BaseGame.ini:199` |
| Budget Allocator가 켜져 있으면 등록하는 메시의 URO를 끄고 외부 틱 제어를 켠다 | `Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator/Private/AnimationBudgetAllocator.cpp:1187-1197` `RegisterComponent`, `Public/IAnimationBudgetAllocator.h:20-27` |
| 매 프레임 갱신 수 = clamp(예산 − 넘친 몫 × 0.8, 건너뛰지 않을 수, 수요) | 같은 `.cpp` `:561`(`WorkUnitBudget`), `:566-567` `CalculateWorkDistributionAndQueue`(시작 `:425`) |
| 스켈레탈 메시의 URO 기본값은 끔 | `Source/Runtime/Engine/Private/Components/SkeletalMeshComponent.cpp:493` |

### 본문에 넣은 발췌

글 전체의 엔진 코드는 28줄이다(8 + 12 + 6 + 2). 줄인 곳은 `// ...`로 적었고, 남긴 줄은 소스 그대로이며 앞쪽 들여쓰기만 줄였다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| `GetSimulationTimeStep` | 8 | `CharacterMovementComponent.cpp:8165-8171`, `// ...`(`:8172-8187`) |
| `PhysWalking` 루프 | 12 | `:5686`(조건 뒤쪽을 `// ...`로 줄임), `:5687-5688`, `// ...`(`:5689`), `:5690-5691`, `// ...`, `:5744`, `// ...`, `:5774`, `// ...`, 루프를 닫는 괄호 |
| `RegisterComponent` | 6 | `AnimationBudgetAllocator.cpp:1191`, `:1193-1197`(빈 줄 `:1192`를 뺐다) |
| `CalculateWorkDistributionAndQueue` | 2 | `:566-567` |

## 확인하지 않은 것

- Unreal Insights 화면의 구간은 손으로 골라 내보내기의 구간보다 0.28초 짧다. 위 "Insights 화면"에 차이를 적었다.
- 1만에서 초당 사망이 적은 원인. 월드 시간이 0.4초 단위로 흐르는 것과 캡슐끼리 막혀 오브 둘레가 덜 빽빽한 것(오브 30m 안 796.35, 기준 978.245)이 후보다. 가르지 않았다. 본문은 두 사실을 원인 관계 없이 적었다.
- 하위 스텝 횟수는 실행에서 세지 않았다. 1만의 개체당 시간이 3.37배인 것이 하위 스텝 때문인지, 장면의 밀도 차이 때문인지 하위 스텝을 끈 실행으로 가르지 않았다.
- 1천에서 Budget Allocator만 켠 액터가 0.52ms 긴 원인. 액터 틱마다 중요도를 셈하는 일(`ALabMinionBudget::Tick`)과 Budget Allocator 자체의 일이 후보다. 트레이스가 없어 가르지 않았다.
- 2천에서 Budget Allocator만 켠 액터의 게임 스레드가 4.20ms 준 곳(36.896 → 32.692). 건너뛴 메시의 애니메이션 말고 무엇이 줄었는지 보지 않았다. 그 조건의 건너뜀·보간 비율 열(`small_skipped_ratio` 등)은 단계 1에서 비어 있다.
- 트레이스는 1만 한 회다. 2천이나 Budget Allocator를 켠 조건의 CharacterMovement 비중은 보지 않았다.
- 트레이스의 프레임당 값은 잘린 프레임 둘을 포함한 93으로 나눴다(글 02와 같은 셈법). 온전한 프레임만의 평균보다 약 0.5% 작다(온전한 91프레임의 길이 합 49.150초 ÷ 91 = 540.1ms).
- 본문 구간 70\~120초는 트레이스 실행의 워밍업 끝(87.6초 전)을 포함한다. 측정 구간 안에서는 몫이 77.6%(Incl)로 1%p 크다.
- SVG는 Edge 헤드리스로 PNG를 그려 겹침을 봤다. 어두운 바탕에서는 보지 않았다(SVG가 자기 흰 배경을 가진다).
