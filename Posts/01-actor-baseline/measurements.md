# 01 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 총괄 묶음 `Full1`(2026-11-12, 10:57\~15:01). 경위는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "총괄 묶음 Full1" |
| 빌드 | Test 패키지 730ff18, CSV 메타데이터 `labbuild=43057eb9f8cc` |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L1-full`(`-LabStage=M0 -LabVariant=full`), `S1-uro`(`-LabVariant=uro`), `S1-budget`(`-LabVariant=budget`). 인자와 기대 메타데이터는 `Scripts/full-conditions.json` |
| 요약 파일 | `Measurements/Full1/summary.csv`(세 회차 중앙값과 범위), `Measurements/Full1/runs.csv`(회차별 한 행) |
| N@16.7ms | `Docs/figures/full1/full-n-at-60fps.csv`(`Scripts/plot.py`가 측정 N 사이를 로그 보간) |
| 트레이스 | `Saved/Traces/Full1traceser-L1-full-*`(직렬, 1만 P1, 한 회). 25\~75초 창을 `Scripts/trace-export.ps1 -Command TimerStatistics`로 내보냈다(저장소 밖 `%TEMP%\horde-trace\F1-traceser-L1-full-GT.csv`). 표는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "Full1 후속 분석"의 "트레이스 16개" |

변형의 구현: URO는 메시의 `bEnableUpdateRateOptimizations = true`이고 거리 단계는 `FAnimUpdateRateParameters` 기본값이다(`Source/HordeOptLab/LabMinion.cpp` `ALabMinion::SetupVisual`). Budget은 메시를 `USkeletalMeshComponentBudgeted`로 바꾸고(`ALabMinionBudget` 생성자), Significance Manager가 오브에서의 거리 25 / 50 / 100 / 250m에 중요도 1 / 0.75 / 0.5 / 0.25, 그 밖에 0을 준다(`LabTierSignificance`, `FLabScenario::BudgetTiers` `LabConfig.h:161`). 예산은 `a.Budget.BudgetMs 2`(`FLabScenario::BudgetMs` `LabConfig.h:162`, `LabGameMode.cpp:363-364`에서 `a.Budget.Enabled 1`과 함께 설정).

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차(r1 / r2 / r3) |
| --- | --- | --- | --- | --- |
| 액터 군중 프레임 p50 441ms | 441.461 | `L1-full` 10000 P1 | `frame_p50_ms` | 438.629 / 441.461 / 446.316 |
| 액터 군중 프레임 p95 466ms | 466.35 | 같음 | `frame_p95_ms` | 463.88 / 466.35 / 471.869 |
| 액터 군중 Game p50 427ms | 426.933 | 같음 | `game_p50_ms` | 423.679 / 426.933 / 429.778 |
| + URO 프레임 p50 422ms | 422.238 | `S1-uro` 10000 P1 | `frame_p50_ms` | 422.238 / 422.069 / 424.678 |
| + URO 프레임 p95 449ms | 449.075 | 같음 | `frame_p95_ms` | 범위 446.303\~449.969 |
| + URO Game p50 408ms | 407.664 | 같음 | `game_p50_ms` | 407.664 / 407.16 / 409.976 |
| + Budget 프레임 p50 401ms | 400.772 | `S1-budget` 10000 P1 | `frame_p50_ms` | 399.012 / 401.005 / 400.772 |
| + Budget 프레임 p95 424ms | 424.217 | 같음 | `frame_p95_ms` | 범위 422.669\~424.705 |
| + Budget Game p50 386ms | 385.538 | 같음 | `game_p50_ms` | 383.747 / 385.538 / 385.697 |
| 16.7ms의 26.5배 | 26.49 | 계산 | 441.461 ÷ 16.667 | |
| 1초에 2.27장 | 2.265 | 계산 | 1000 ÷ 441.461 | |
| 500 프레임 p50 9.03ms | 9.028 | `L1-full` 500 P1 | `frame_p50_ms` | 8.983 / 9.028 / 9.113 |
| 1,000 프레임 p50 17.5ms | 17.539 | `L1-full` 1000 P1 | `frame_p50_ms` | 17.539 / 17.41 / 17.773 |
| 2,000 프레임 p50 36.4ms | 36.391 | `L1-full` 2000 P1 | `frame_p50_ms` | 36.027 / 36.391 / 36.436 |
| 1,000 프레임 p95 19.7ms | 19.734 | `L1-full` 1000 P1 | `frame_p95_ms` | 19.734 / 19.446 / 19.864 |
| 개체당 Game 500: 17.0µs | 16.99 | 계산 | 8.493 ms ÷ 500 | Game p50 8.45 / 8.493 / 8.581 |
| 개체당 Game 1,000: 16.6µs | 16.62 | 계산 | 16.624 ms ÷ 1000 | Game p50 16.624 / 16.495 / 16.855 |
| 개체당 Game 2,000: 17.3µs | 17.30 | 계산 | 34.604 ms ÷ 2000 | Game p50 34.203 / 34.604 / 34.659 |
| 개체당 Game 1만: 42.7µs | 42.69 | 계산 | 426.933 ms ÷ 10000 | 위 `L1-full` 10000 |
| "17µs 안팎으로 평평" | 16.62\~17.30 | 계산 | 위 세 값 | |
| 2천 → 1만, Game 12.3배 | 12.34 | 계산 | 426.933 ÷ 34.604 | |
| 1천을 1만으로 늘리면 166ms(예상) | 166.24 | 계산 | 16.624 × 10 | |
| N@16.7ms 797 | 797 | `1 L1-full` | `full-n-at-60fps.csv` `n_p95` | p50 기준은 934(`n_p50`) |
| 1만은 한계의 12.5배 | 12.55 | 계산 | 10000 ÷ 797 | |
| URO 프레임 −19.2ms(4.35%) | −19.223, −4.354% | 계산 | 422.238 − 441.461, ÷ 441.461 | |
| URO Game −19.3ms | −19.269 | 계산 | 407.664 − 426.933 | (−4.51%) |
| Budget 프레임 −40.7ms(9.22%) | −40.689, −9.217% | 계산 | 400.772 − 441.461, ÷ 441.461 | |
| Budget Game −41.4ms | −41.395 | 계산 | 385.538 − 426.933 | (−9.70%) |
| "세 회차 범위를 넘는 차이" | 프레임 기준 범위 7.687(`L1`), 2.609(`uro`), 1.993(`budget`). Game 범위 6.099 / 2.816 / 1.950 | 계산 | [measurement.md](../../Docs/Guides/measurement.md) "차이 판정": 변화가 두 범위 중 큰 쪽보다 크다 | |
| 게임 스레드 한 프레임 446ms | 446.43 | 직렬 트레이스 L1 1만 P1 | `FEngineLoop::Tick` Incl ÷ 112 프레임 | 한 회 |
| CharacterMovement 틱 323ms | 323.49 | 같음 | `UCharacterMovementComponent_TickComponent` Incl | 한 회 |
| 72.5% | 72.46% | 계산 | 323.49 ÷ 446.43 | |
| 스켈레탈 메시 틱 27.2ms | 27.24 | 같음 | `USkinnedMeshComponent_TickComponent` Incl(Excl 11.33) | 한 회 |
| 애니 평가 마무리 14.7ms | 14.70 | 같음 | `USkeletalMeshComponent_CompleteParallelAnimationEvaluation` Incl | 한 회 |
| 애니메이션 쪽 타이머 합 41.9ms | 41.94 | 계산 | 27.24 + 14.70. 본문 "약 9%"는 41.94 ÷ 446.43 = 9.39% | |
| 애니메이션을 지워도 남는 약 404ms | 404.49 | 계산 | 446.43 − 41.94. 트레이스 한 회에서 뺀 어림이다 | |
| 그림의 나머지 81.0ms(18%) | 81.00 | 계산 | 446.43 − 323.49 − 27.24 − 14.70 | |
| 개체당 CharacterMovement 32.3µs | 32.35 | 계산 | 323.49 ms ÷ 10000. 실제 호출은 프레임당 9,920회(1,111,002 ÷ 112)라 호출당은 32.6µs | |
| Insights 한 프레임 447ms | 447.4 | 스크린샷 | `FEngineLoop::Tick` 막대(선택 구간 447.88ms) | |
| Insights CharacterMovement 322ms, 10,048회 | 321.66 | 스크린샷 | Timers 창 Incl, Count | |
| Insights 71.9% | 71.90% | 계산 | 321.66 ÷ 447.4 | |
| 하위 스텝 1회 / 8회 | | 계산 | 아래 "엔진 소스". 프레임 50ms 미만이면 1회. 1만은 델타가 0.4초로 잘려 0.4 ÷ 0.05 = 8회 | |
| 60ms면 30ms씩 두 번 | | 계산 | `Min(0.05, 0.06 × 0.5)` = 0.03 | |
| 예산 2ms, 거리 단계 | 2.0 | 코드 | `LabConfig.h:162`, `BudgetTiers` `:161` | |

`L1-full` 10000 P1에는 버리는 첫 실행 `Full1-L1-full-N10000-P1-r0`(프레임 p50 441.156)이 `runs.csv`에 있다. 요약은 r1\~r3만 쓴다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/actor-gamethread-breakdown.svg` | 446.43ms를 CharacterMovement 323.49, 스켈레탈 메시 틱 27.24, 애니 평가 마무리 14.70, 나머지 81.00으로 나눈 가로 막대. 16.7ms 막대를 같은 축척으로 | 위 트레이스 행 |
| `images/actor-frame-vs-n.svg` | 왼쪽: 프레임 p50·p95(로그-로그), 16.7ms 선, 50ms 선, N@16.7ms 797. 오른쪽: 개체당 Game µs와 하위 스텝 횟수 | 위 `L1-full` 행 |
| `images/insights-gamethread.jpg` | Unreal Insights Timing 창. 게임 스레드 한 프레임(`Csv 69`, 56.5\~57.0초 부근)을 선택하고 Timers 창에서 `CharacterMovement`로 거른 화면. 코디네이터가 찍었다 | 트레이스 파일 이름과 병렬·직렬 구분은 코디네이터 확인이 필요하다 |
| `../../Docs/figures/steps/step1-L1-full.webp` | 1단계 화면(HUD 켬, 녹화 부하가 섞여 HUD 프레임 시간이 측정값보다 높다) | 촬영 경위는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "촬영 준비" |

SVG 두 개는 표준 라이브러리 Python으로 값을 넣어 쓴 정적 파일이다(생성 스크립트는 저장소에 두지 않았다). 배경은 흰색이라 GitHub의 밝은·어두운 테마 모두에서 같은 색으로 보인다.

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다.

| 사실 | 위치 |
| --- | --- |
| `MaxSimulationTimeStep = 0.05f`, `MaxSimulationIterations = 8` | `Engine/Source/Runtime/Engine/Private/Components/CharacterMovementComponent.cpp:698-699` |
| 나누는 규칙 `FMath::Min(MaxSimulationTimeStep, RemainingTime * 0.5f)`, 마지막 회차는 남은 시간을 모두 쓴다 | 같은 파일 `UCharacterMovementComponent::GetSimulationTimeStep` `:8162-8187` |
| 월드 델타를 `[MinUndilatedFrameTime, MaxUndilatedFrameTime]`로 자른다 | `Engine/Source/Runtime/Engine/Private/WorldSettings.cpp:334-343` `AWorldSettings::FixupDeltaSeconds` |
| `MaxUndilatedFrameTime=0.4`(2.5fps). 프로젝트 `Config/`는 바꾸지 않는다 | `Engine/Config/BaseGame.ini:199` |
| URO 거리 단계는 `FAnimUpdateRateParameters` 기본값 | [engine-notes.md](../../Docs/Reference/engine-notes.md) 11절(`EngineTypes.h:2768-2785`, 이번에 다시 열지 않음) |
| Budget은 `a.Budget.Enabled`와 월드 할당기의 `bEnabled`가 둘 다 참일 때만 돈다. 등록 시 URO를 끈다 | engine-notes.md(`AnimationBudgetAllocatorCVars.cpp:7`, `AnimationBudgetAllocator.cpp:75,115,135`). 실행의 메타데이터 `LabBudget=1`(`full-conditions.json` 기대값) |

`WorldSettings`·`BaseGame.ini`의 0.4초 상한은 engine-notes.md에 아직 없다(이 작업은 그 파일을 고치지 않는다. [Worklog/Posts.md](../../Docs/Worklog/Posts.md)에 남겼다).

## 경위의 근거

본문 "앞 마일스톤에서도 같은 방향을 보았다" 문단은 수치를 붙이지 않았다. 근거는 M0 묶음(Test 패키지 96f8202, 1종, 체력 100)이고 최종 측정과 조건이 달라 비교에 쓰지 않는다.

- 애니메이션을 정적 메시로 바꾼 변형 `static`, CharacterMovement 틱을 끈 변형 `pawn`: [Worklog/M0.md](../../Docs/Worklog/M0.md) "2026-11-08 E0 재측정"과 "STATUS.md에서 옮김"의 6번 항목. 변형마다 한계가 조금씩만 오른 것은 같은 절의 60fps 최대 N 줄이다.
- 하위 스텝으로 CharacterMovement가 개체 수보다 빨리 는 첫 관찰(2천 → 5천): [engine-notes.md](../../Docs/Reference/engine-notes.md) 11절 "CharacterMovement 하위 스텝" 행. 본문은 이 옛 값을 쓰지 않고 최종 측정의 500 / 1천 / 2천 / 1만으로 다시 보였다.
- 글 재료의 사실 확인: `Docs/post-review.md` "01 액터 군중 기준선"(글을 다 쓰면 지우는 문서라, 지운 뒤에는 git 이력에서 본다).

## 확인하지 않은 것

- 하위 스텝을 끄고(`MaxSimulationIterations` 1 등) 다시 잰 직접 실험은 없다. "1만에서 8회"는 엔진 소스의 규칙과 0.4초 상한으로 계산한 값이고, 실행에서 횟수를 세지 않았다.
- 0.4초 상한 때문에 1만 실행에서는 게임 시간이 실제 시간보다 느리게 흐른다(441ms 프레임에 0.4초). 사망 수가 2단계 1만과 0.3% 안으로 맞는 이유(동등성 검사 통과)와 이 상한의 관계는 보지 않았다.
- Budget·URO 실행의 트레이스는 없다. Budget이 줄인 41.4ms가 애니메이션 타이머에서 났는지, URO가 Budget의 절반쯤만 줄인 이유는 확인하지 않았다. 글 재료의 "포즈 계산이 워커에서 돌아서" 설명은 쓰지 않았다.
- `S1-budget` r3의 GPU p50이 112.456ms로 다른 두 회차(50.246, 48.787)와 다르다. 프레임 p50(400.772)은 다른 회차와 같아 본문 값에 영향이 없고, 원인은 보지 않았다.
- 트레이스는 직렬(`mass.FullyParallel` 끔) 실행이다. 1단계는 Mass 프로세서가 거의 돌지 않아(`PrePhysics` 0.05ms) 병렬 트레이스와 값이 같다(병렬 `F1-trace-L1-full-GT.csv`: `FEngineLoop::Tick` 446.43, CharacterMovement 323.06).
- 도식의 "AnimBP 갱신이 스켈레탈 메시 틱 안에 있다"는 Incl − Excl(27.24 − 11.33 = 15.91)이 `USkeletalMeshComponent_TickAnimation` 14.90과 맞는 것으로 판단했다. 트레이스에서 부모 관계를 직접 펼쳐 보지는 않았다.
- Insights 스크린샷이 어느 트레이스 파일인지(병렬·직렬).
