# 01 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-12 20:34\~11-13 00:39). 경위는 [작업 기록의 재측정 절](../../Docs/Worklog/Q6.md#full2-재측정2026-11-12) |
| 빌드 | Test 패키지 08e45f2, CSV 메타데이터 `labbuild=08e45f24a5e5` |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L1-full`(`-LabStage=M0 -LabVariant=full`), `S1-uro`(`-LabVariant=uro`), `S1-budget`(`-LabVariant=budget`). 인자와 기대 메타데이터는 `Scripts/full-conditions.json` |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full2/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full2/runs.csv)(회차별 한 행) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full2/full-n-at-60fps.csv)(`Scripts/plot.py`가 측정 N 사이를 로그 보간) |
| 트레이스 | 최종 측정 빌드(Test 패키지 08e45f2)에서 뜬 트레이스다. 아래 "측정 기록의 변경" 참고. 실행 라벨 `Full2traceser-L1-full-N10000-P1-r1`(`Saved/Traces/`, 직렬, 1만 P1, 한 회). 25\~75초 창을 `Scripts/trace-export.ps1 -Command TimerStatistics`로 내보냈다(저장소 밖 `%TEMP%\horde-trace\F2-traceser-L1-full-GT.csv`). 표는 [작업 기록의 트레이스 16개 절](../../Docs/Worklog/Q6.md#트레이스-16개사다리-여덟-단계-1만-p1-병렬직렬-한-회씩) |

변형의 구현: URO는 메시의 `bEnableUpdateRateOptimizations = true`이고 간격은 `FAnimUpdateRateParameters` 기본값이다(`Source/HordeOptLab/LabMinion.cpp:80-83` `ALabMinion::SetupVisual`). Budget은 메시를 `USkeletalMeshComponentBudgeted`로 바꾸고(`ALabMinionBudget` 생성자 `LabMinion.cpp:244-247`), Significance Manager가 시점에서의 거리 25 / 50 / 100 / 250m 안에 중요도 1 / 0.75 / 0.5 / 0.25, 그 밖에 0을 준다(`LabTierSignificance` `LabMinion.cpp:229-241`, `FLabScenario::BudgetTiers` `LabConfig.h:161`). 시점은 게임 모드가 매 틱 넘기는 카메라 트랜스폼이다(`LabGameMode.cpp:595-601` `Sig->Update`). 그래서 본문은 "카메라와의 거리"로 적었다(P1 카메라는 오브 위 9m에서 제자리 회전한다). 예산은 `a.Budget.BudgetMs 2`(`FLabScenario::BudgetMs` `LabConfig.h:162`, `LabGameMode.cpp:363-364`에서 `a.Budget.Enabled 1`과 함께 설정).

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. "(그림)"은 본문 산문이 아니라 그림에만 쓴 값이다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차(r1 / r2 / r3) |
| --- | --- | --- | --- | --- |
| 액터 군중 프레임 시간 440ms | 439.958 | `L1-full` 10000 P1 | `frame_p50_ms` | 436.39 / 440.652 / 439.958 |
| 액터 군중 게임 스레드 425ms | 424.871 | 같음 | `game_p50_ms` | 420.75 / 425.783 / 424.871 |
| 액터 군중 렌더 스레드 30.4ms | 30.389 | 같음 | `render_p50_ms` | 29.027 / 30.389 / 31.062 |
| 액터 군중 GPU 50.2ms | 50.227 | 같음 | `gpu_p50_ms` | 50.145 / 50.633 / 50.227 |
| + URO 프레임 시간 423ms | 423.126 | `S1-uro` 10000 P1 | `frame_p50_ms` | 423.126 / 422.889 / 424.087 |
| + URO 게임 스레드 408ms | 408.053 | 같음 | `game_p50_ms` | 408.053 / 407.605 / 408.612 |
| + URO 렌더 스레드 28.5ms | 28.517 | 같음 | `render_p50_ms` | 28.516 / 28.517 / 30.747 |
| + URO GPU 50.4ms | 50.35 | 같음 | `gpu_p50_ms` | 50.37 / 50.35 / 50.344 |
| + Budget 프레임 시간 400ms | 400.403 | `S1-budget` 10000 P1 | `frame_p50_ms` | 398.766 / 400.403 / 401.563 |
| + Budget 게임 스레드 385ms | 384.894 | 같음 | `game_p50_ms` | 383.712 / 384.894 / 386.418 |
| + Budget 렌더 스레드 28.9ms | 28.896 | 같음 | `render_p50_ms` | 28.896 / 28.433 / 31.079 |
| + Budget GPU 50.1ms | 50.142 | 같음 | `gpu_p50_ms` | 50.142 / 50.518 / 48.943 |
| "이 구성에서는 게임 스레드가 가장 느리다" | | | `bottleneck` = `game`(세 행 모두) | |
| 16.7ms의 26.4배 | 26.40 | 계산 | 439.958 ÷ 16.667 | |
| 1,000 게임 스레드 17.0ms | 17.038 | `L1-full` 1000 P1 | `game_p50_ms` | 17.038 / 16.943 / 17.06 |
| 1천을 1만으로 늘리면 170ms(예상) | 170.38 | 계산 | 17.038 × 10 | |
| (그림) 500 프레임 시간 9.36ms | 9.364 | `L1-full` 500 P1 | `frame_p50_ms` | 9.335 / 9.364 / 9.547 |
| (그림) 1,000 프레임 시간 18.0ms | 17.987 | `L1-full` 1000 P1 | `frame_p50_ms` | 17.993 / 17.886 / 17.987 |
| (그림) 2,000 프레임 시간 36.6ms | 36.643 | `L1-full` 2000 P1 | `frame_p50_ms` | 36.69 / 36.643 / 36.615 |
| (그림) 1만 프레임 시간 느린 쪽 5% 경계 464ms | 463.579 | `L1-full` 10000 P1 | `frame_p95_ms` | 463.963 / 462.879 / 463.579 |
| 1,000 느린 쪽 5% 경계 20.0ms | 20.038 | `L1-full` 1000 P1 | `frame_p95_ms` | 20.038 / 19.924 / 20.432 |
| (그림) 개체당 게임 스레드 500: 17.6µs | 17.57 | 계산 | 8.784 ms ÷ 500 | 게임 스레드 8.746 / 8.784 / 8.98 |
| (그림) 개체당 게임 스레드 1,000: 17.0µs | 17.04 | 계산 | 17.038 ms ÷ 1000 | 위 1,000 행 |
| (그림) 개체당 게임 스레드 2,000: 17.4µs | 17.43 | 계산 | 34.861 ms ÷ 2000 | 게임 스레드 34.89 / 34.861 / 34.79 |
| 개체당 게임 스레드 1만: 42.5µs | 42.49 | 계산 | 424.871 ms ÷ 10000 | 위 `L1-full` 10000 |
| "17µs 안팎으로 평평" | 17.04\~17.57 | 계산 | 위 세 값 | |
| 2천 → 1만, 게임 스레드 12.2배 | 12.19 | 계산 | 424.871 ÷ 34.861 | |
| 60fps를 지키는 최대 개체 수 774 | 774 | `1 L1-full` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준은 902(`n_p50`) |
| 1만은 한계의 12.9배 | 12.92 | 계산 | 10000 ÷ 774 | |
| URO 프레임 시간 −16.8ms(3.83%) | −16.832, −3.826% | 계산 | 423.126 − 439.958, ÷ 439.958 | |
| URO 게임 스레드 −16.8ms | −16.818 | 계산 | 408.053 − 424.871 | (−3.96%) |
| Budget 프레임 시간 −39.6ms(8.99%) | −39.555, −8.991% | 계산 | 400.403 − 439.958, ÷ 439.958 | |
| Budget 게임 스레드 −40.0ms | −39.977 | 계산 | 384.894 − 424.871 | (−9.41%) |
| "세 회차의 흔들림보다 크다" | 프레임 시간 범위 4.262(`L1`), 1.198(`uro`), 2.797(`budget`). 게임 스레드 범위 5.033 / 1.007 / 2.706 | 계산 | [measurement.md](../../Docs/Guides/measurement.md) "차이 판정": 변화가 두 범위 중 큰 쪽보다 크다 | |
| 게임 스레드 한 프레임 446ms | 446.43 | 직렬 트레이스 L1 1만 P1 | `FEngineLoop::Tick` Incl ÷ 112 프레임 | 한 회 |
| CharacterMovement 틱 322ms | 322.20 | 같음 | `UCharacterMovementComponent_TickComponent` Incl | 한 회 |
| 72.2%(그림은 72%) | 72.17% | 계산 | 322.20 ÷ 446.43 | |
| (그림) 스켈레탈 메시 틱 27.3ms · 6% | 27.31 | 같음 | `USkinnedMeshComponent_TickComponent` Incl(Excl 11.03). 6.12% | 한 회 |
| (그림) 애니 평가 마무리 15.7ms · 4% | 15.71 | 같음 | `USkeletalMeshComponent_CompleteParallelAnimationEvaluation` Incl. 3.52% | 한 회 |
| 애니메이션 쪽 두 타이머 합 43.0ms | 43.02 | 계산 | 27.31 + 15.71 | |
| 게임 스레드의 9.64% | 9.64% | 계산 | 43.02 ÷ 446.43 | |
| 애니메이션을 지워도 남는 약 403ms | 403.41 | 계산 | 446.43 − 43.02. 트레이스 한 회에서 뺀 어림이다 | |
| (그림) 나머지 81.2ms · 18% | 81.21 | 계산 | 446.43 − 322.20 − 27.31 − 15.71. 18.19% | |
| 개체당 CharacterMovement 32.2µs | 32.22 | 계산 | 322.20 ms ÷ 10000. 실제 호출은 프레임당 9,968회(1,116,437 ÷ 112)라 호출당은 32.3µs | |
| 스크린샷의 프레임 하나 449ms | 448.71 | 병렬 트레이스 `Full2trace-L1-full-N10000-P1-r1`, 약 56초 지점의 프레임 하나 | Timing 뷰의 선택 구간 길이 | 한 프레임 |
| 스크린샷의 CharacterMovement 323ms | 322.94 | 같음 | Timers 패널 `UCharacterMovementComponent_TickComponent` Incl(Excl 321.72) | 한 프레임 |
| 스크린샷의 호출 수 10,052회 | 10,052 | 같음 | Timers 패널 Count | 한 프레임 |
| 72.0% | 71.97% | 계산 | 322.94 ÷ 448.71 | |
| 하위 스텝 1회 / 8회 | | 계산 | 아래 "엔진 소스". 프레임 50ms 미만이면 1회. 1만은 델타가 0.4초로 잘려 0.4 ÷ 0.05 = 8회(일곱 번째까지 `Min(0.05, 남은 시간 × 0.5)` = 0.05, 여덟 번째는 남은 0.05초를 모두 쓴다) | |
| 60ms면 30ms씩 두 번 | | 계산 | `Min(0.05, 0.06 × 0.5)` = 0.03, 남은 0.03은 50ms 이하라 그대로 쓴다 | |
| 기본값 0.05초, 8회 | | 엔진 소스 | 아래 "엔진 소스" | |
| 0.4초 | 0.4 | 엔진 설정 | `BaseGame.ini:199` | |
| URO 0.24, 0.12, 네 프레임에 한 번 | | 엔진 소스 | 아래 "엔진 소스" | |
| URO 기준은 화면 반지름의 제곱. LOD 화면 크기로 약 0.98, 0.69. 건너뛴 프레임은 보간 | | 엔진 소스, 계산 | `SkeletalRender.cpp:173,203`(`MaxDistanceFactor`에 `ScreenRadiusSquared`를 넣고, LOD는 `Square(ScreenSize * 0.5)`와 견준다), `SkinnedMeshComponent.cpp:310-318`. 2 × √0.24 = 0.98, 2 × √0.12 = 0.69 | |
| Budget: 넘는 몫의 0.8배, 2\~10프레임 간격, 최근 1초 | | 엔진 소스 | `AnimationBudgetAllocator.cpp:174`(최근 1초), `:567`(`AlwaysTickFalloffAggression`), `AnimationBudgetAllocatorParameters.h:34`(`MaxTickRate = 10`), `:49`(0.8) | |
| 개체당 시간 2.5배 | 2.5 | 계산 | 42.5 ÷ 17.0 | |
| 예산 2ms, 거리 단계 25 / 50 / 100 / 250m, 중요도 1 / 0.75 / 0.5 / 0.25 / 0 | 2.0 | 코드 | `LabConfig.h:161-162`, `LabMinion.cpp:229-241` | |

`L1-full` 10000 P1에는 버리는 첫 실행 `Full2-L1-full-N10000-P1-r0`(프레임 p50 438.835)이 `runs.csv`에 있다. 요약은 r1\~r3만 쓴다.

## 측정 기록의 변경

- 2026-11-13: 측정값을 Test 패키지 730ff18의 측정에서 08e45f2의 최종 측정으로 바꿨다. 200m 밖 입자 메시를 원본 LOD4로 바꿔(bf1a4c8) 측정 조건이 달라졌고, 규약대로 전 조건을 다시 쟀기 때문이다.
- 2026-11-13: Insights 스크린샷에서 트레이스 파일 이름이 보이는 제목 줄을 잘라 냈다. 값과 빨간 상자는 그대로다.
- 2026-11-13: Insights 스크린샷과 그 캡션의 값(449 / 323ms / 10,052회)을 최종 측정 빌드(08e45f2)의 병렬 트레이스 `Full2trace-L1-full-N10000-P1-r1`에서 다시 찍었다. 처음 값은 730ff18의 트레이스에서 찍은 448 / 322ms / 10,048회였다.
- 2026-11-13: 트레이스 값(446.43 / 322.20 / 27.31 / 15.71)은 최종 측정 빌드(08e45f2)의 트레이스 `Full2traceser-L1-full-N10000-P1-r1`에서 다시 뽑았다. 처음 쓴 값은 730ff18의 트레이스 `Full1traceser-L1-full-*`에서 읽은 446.43 / 323.49 / 27.24 / 14.70이었다(같은 방법으로 옛 트레이스를 다시 계산하면 옛 값과 일치했다). 값을 뽑는 `Scripts/trace-export.ps1`은 `UnrealInsights.exe -NoUI` 명령줄 내보내기다. 두 빌드 사이에 `Source/`와 `Config/`는 같고, 바뀐 것은 `Content/Lab/FX/`의 배경 무리 에셋 셋뿐이다(`git diff --stat 730ff18 08e45f2`). 1단계는 배경 무리를 쓰지 않는다.

## 화면의 HUD와 표의 차이

요약의 1단계 화면(`step1-L1-full.webp`) 왼쪽 위 HUD에 보이는 프레임 시간(542.7ms)은 표의 값(440ms)보다 높다. 그 화면은 측정 실행이 아니라 촬영 실행(`Pre5shot-L1-full-N10000-P1-r1`)에서 찍었고, 4K 영역 화면 녹화 부하가 섞인다. 수치는 표(Test 패키지 측정 실행)의 값만 쓴다. 촬영 경위는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "촬영 준비"에 있다.

## 두 트레이스

본문은 출처가 다른 트레이스 값 두 가지를 쓴다.

| 쓰인 곳 | 트레이스 | 구간 | 값 |
| --- | --- | --- | --- |
| 그림 `actor-frame-flow.svg`의 막대, 본문 446 / 322 / 43.0ms | 직렬 실행 `Full2traceser-L1-full-N10000-P1-r1`(`mass.FullyParallel` 끔) | 25\~75초, 112프레임의 프레임당 평균 | `FEngineLoop::Tick` 446.43, CharacterMovement 322.20 |
| 스크린샷, 캡션, 스크린샷 아래 문장 449 / 323ms / 10,052회 | 병렬 실행 `Full2trace-L1-full-N10000-P1-r1` | 약 56초 지점의 프레임 하나 | 선택 구간 448.71, CharacterMovement 322.94 |

새 병렬 트레이스 `Full2trace-L1-full-N10000-P1-r1`의 같은 25\~75초 구간 평균(111프레임)은 `FEngineLoop::Tick` 450.45, CharacterMovement 323.90ms다(`%TEMP%\horde-trace\F2-trace-L1-full-GT.csv`). 직렬 트레이스와 4.0ms, 1.7ms 차이다. 1단계는 Mass 프로세서가 거의 돌지 않아(`PrePhysics` 0.08ms) 직렬·병렬 설정이 액터 비용에 닿지 않고, 차이는 실행 사이의 흔들림이다. 그래서 스크린샷을 직렬 트레이스로 다시 찍지 않았다. 스크린샷의 한 프레임 값(448.71, 322.94)은 이 병렬 트레이스의 값이다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/actor-summary.svg` | 요약 그림. 세 구성(액터 군중, + URO, + Budget)의 프레임·게임 스레드·렌더 스레드·GPU 시간 막대(1만 P1, 같은 축척)와 16.7ms 선 | 위 1만 P1 행 열두 개 |
| `images/actor-anim-techniques.svg` | 결과 그림. URO와 Budget이 줄인 프레임·게임 스레드 시간, 덧댄 뒤의 프레임 시간, 비교용 애니메이션 쪽 두 타이머의 합 43.02(점선) | 위 변화 행, 트레이스 행 |
| `images/actor-frame-flow.svg` | 한 개체가 한 프레임에 하는 일 네 칸(이동 컴포넌트 틱, 스켈레탈 메시 틱과 AnimBP 갱신, 애니메이션 평가 마무리, 프레임 끝 렌더 갱신)과 칸마다 엔진 타이머 이름. 오른쪽 막대 길이가 1만의 프레임당 시간(322.20, 27.31, 15.71). 네 번째 칸 `UWorld_SendAllEndOfFrameUpdates`의 값은 따로 읽지 않아, 나머지 81.21ms(이 칸과 그 밖의 일)를 점선 막대로 보였다. 맨 아래 16.7ms 막대는 같은 축척 | 위 트레이스 행 |
| `images/actor-frame-vs-n.svg` | 위: 프레임 시간 중앙값·느린 쪽 5% 경계(로그-로그), 16.7ms 선, 50ms 선, 60fps를 지키는 최대 개체 수 774. 아래: 개체당 게임 스레드 시간(µs)과 하위 스텝 횟수, 예상선(1천의 17.04µs로 일정) | 위 `L1-full` 행 |
| `images/insights-gamethread.jpg` | Unreal Insights에서 `Full2trace-L1-full-N10000-P1-r1`을 연 화면이다. 왼쪽 빨간 상자는 Timing 뷰에서 고른 프레임 하나(약 56초 지점, 448.71ms)다. 오른쪽 빨간 상자는 Timers 패널을 "CharacterMovement"로 거른 결과다(`UCharacterMovementComponent_TickComponent` Count 10,052, Incl 322.94ms, Excl 321.72ms). 화면 캡처 둘(왼쪽 절반, 오른쪽 절반)을 이어 붙이고 빨간 상자를 그렸다. 2026-11-13에 다시 찍었다 |
| `../../Docs/figures/steps/step1-L1-full.webp` | 1단계 화면(HUD 켬). HUD 값은 위 "화면의 HUD와 표의 차이" | 촬영 경위는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "촬영 준비" |

SVG 네 개는 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py Posts/01-actor-baseline/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽고, 트레이스 값 네 개(446.43 / 322.20 / 27.31 / 15.71)는 스크립트의 상수다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가져 GitHub의 밝은·어두운 테마 모두에서 같은 색으로 보인다. 처음 글의 `actor-gamethread-breakdown.svg`(같은 네 값의 가로 막대)는 `actor-frame-flow.svg`에 합쳐 지웠다.

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다. 경로는 `Engine/` 아래다.

| 사실 | 위치 |
| --- | --- |
| `MaxSimulationTimeStep = 0.05f`, `MaxSimulationIterations = 8` | `Source/Runtime/Engine/Private/Components/CharacterMovementComponent.cpp:698-699` |
| 나누는 규칙 `FMath::Min(MaxSimulationTimeStep, RemainingTime * 0.5f)`, 마지막 회차는 남은 시간을 모두 쓴다 | 같은 파일 `UCharacterMovementComponent::GetSimulationTimeStep` `:8162-8187` |
| 걷기 루프: `while`가 `Iterations < MaxSimulationIterations`까지 돌고, 스텝마다 `GetSimulationTimeStep`, `MoveAlongFloor`, `FindFloor`를 부른다 | 같은 파일 `UCharacterMovementComponent::PhysWalking` `:5653`, `remainingTime` `:5680`, `while` `:5686`, `Iterations++` `:5688`, `GetSimulationTimeStep` `:5690-5691`, `MoveAlongFloor` `:5744`, `FindFloor` `:5773` |
| `MIN_TICK_TIME = 1e-6f` | 같은 파일 `:97` |
| 월드 델타를 `[MinUndilatedFrameTime, MaxUndilatedFrameTime]`(시간 팽창을 곱한 값)로 자른다 | `Source/Runtime/Engine/Private/WorldSettings.cpp:334-343` `AWorldSettings::FixupDeltaSeconds` |
| `MaxUndilatedFrameTime=0.4`(2.5fps). 프로젝트 `Config/`는 바꾸지 않는다 | `Config/BaseGame.ini:199` |
| URO 기본값: 보이는 메시의 화면 크기 단계 `BaseVisibleDistanceFactorThesholds` = 0.24, 0.12. 그려지지 않은 메시는 `BaseNonRenderedUpdateRate(4)` | `Source/Runtime/Engine/Classes/Engine/EngineTypes.h:2820-2826`(주석 `:2772-2785`가 "MaxDistanceFactor is size on screen, as used by LODs") |
| URO 간격 결정: 화면 크기가 단계 i의 값을 넘으면 i + 1 프레임에 한 번, 어느 단계도 못 넘으면 단계 수 + 1(= 3) | `Source/Runtime/Engine/Private/Components/SkinnedMeshComponent.cpp:283-370` `AnimUpdateRateSetParams`(단계 비교 `:310-319`) |
| Budget 분배: 중요도 큰 순으로 정렬하고(`:327-342`), 예산 ÷ 평균 갱신 시간으로 매 프레임 갱신할 수를 구한 뒤(`:558-570`), 그 뒤를 보간 묶음과 간격 제한 묶음으로 나눈다(`:610-640`) | `Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator/Private/AnimationBudgetAllocator.cpp` |
| Budget은 `a.Budget.Enabled`와 월드 할당기의 `bEnabled`가 둘 다 참일 때만 돈다. 등록 시 URO를 끈다 | [engine-notes.md](../../Docs/Reference/engine-notes.md)(`AnimationBudgetAllocatorCVars.cpp:7`, `AnimationBudgetAllocator.cpp:75,115,135`). 실행의 메타데이터 `LabBudget=1`(`full-conditions.json` 기대값) |

### 본문에 넣은 발췌

글 전체의 엔진 코드는 30줄이다(12 + 13 + 5). 줄인 곳은 `// ...`로 적었고, 남긴 줄은 소스를 그대로 옮겼다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| `GetSimulationTimeStep` | 12 | `CharacterMovementComponent.cpp:8162-8163, 8165-8168, 8170-8171`, `// ...`(`:8172-8182` 마지막 회차의 else 분기와 경고), `:8183`, `:8186-8187`. `static` 경고 카운터(`:8164`), 주석(`:8169`, `:8185`), 빈 줄(`:8184`)을 뺐다 |
| `PhysWalking` 루프 | 13 | `:5680`, `:5686`(조건 뒤쪽을 `// ...`로 줄임), `:5687-5688`, `:5690-5691`, `:5744`, `:5773`, 닫는 괄호 |
| `FixupDeltaSeconds` | 5 | `WorldSettings.cpp:334-335`, `// ...`(`:336-340`), `:341`, `:342` |

`WorldSettings`·`BaseGame.ini`의 0.4초 상한은 engine-notes.md에 아직 없다(이 작업은 그 파일을 고치지 않는다. [Worklog/Posts.md](../../Docs/Worklog/Posts.md)에 남겼다).

## 경위의 근거

본문 "앞 마일스톤에서도 같은 방향을 보았다" 문단은 수치를 붙이지 않았다. 근거는 M0 묶음(Test 패키지 96f8202, 1종, 체력 100)이고 최종 측정과 조건이 달라 비교에 쓰지 않는다.

- 애니메이션을 정적 메시로 바꾼 변형 `static`, CharacterMovement 틱을 끈 변형 `pawn`: [Worklog/M0.md](../../Docs/Worklog/M0.md) "2026-11-08 E0 재측정"과 "STATUS.md에서 옮김"의 6번 항목. 변형마다 한계가 조금씩만 오른 것은 같은 절의 60fps 최대 N 줄이다.
- 하위 스텝으로 CharacterMovement가 개체 수보다 빨리 는 첫 관찰(2천 → 5천): [engine-notes.md](../../Docs/Reference/engine-notes.md) 11절 "CharacterMovement 하위 스텝" 행. 본문은 이 옛 값을 쓰지 않고 최종 측정의 500 / 1천 / 2천 / 1만으로 다시 보였다.
- 글 재료의 사실 확인: `Docs/post-review.md` "01 액터 군중 기준선"(글을 다 쓰면 지우는 문서라, 지운 뒤에는 git 이력에서 본다).

## 확인하지 않은 것

- 하위 스텝을 끄고(`MaxSimulationIterations` 1 등) 다시 잰 직접 실험은 없다. "1만에서 8회"는 엔진 소스의 규칙과 0.4초 상한으로 계산한 값이고, 실행에서 횟수를 세지 않았다.
- 0.4초 상한 때문에 1만 실행에서는 게임 시간이 실제 시간보다 느리게 흐른다(440ms 프레임에 0.4초). 사망 수가 2단계 1만과 약 0.3% 차이로 맞는 이유(최종 측정의 초당 사망 62.18 / 61.99)(동등성 검사 통과)와 이 상한의 관계는 보지 않았다.
- Budget·URO 실행의 트레이스는 없다. Budget이 줄인 40.0ms가 애니메이션 타이머에서 났는지, URO가 Budget의 절반쯤만 줄인 이유는 확인하지 않았다. 글 재료의 "포즈 계산이 워커에서 돌아서" 설명은 쓰지 않았다.
- URO 실행에서 개체마다 실제로 고른 갱신 간격(화면 크기 분포)은 세지 않았다. 본문의 URO 설명은 엔진 기본값의 규칙이다.
- 트레이스는 직렬(`mass.FullyParallel` 끔) 실행이다. 1단계는 Mass 프로세서가 거의 돌지 않아(`PrePhysics` 0.08ms) 병렬 트레이스와 값이 비슷하다(병렬 `F2-trace-L1-full-GT.csv`: `FEngineLoop::Tick` 450.45, CharacterMovement 323.90).
- 그림의 "AnimBP 갱신이 스켈레탈 메시 틱 안에 있다"는 Incl − Excl(27.31 − 11.03 = 16.28)이 `USkeletalMeshComponent_TickAnimation` 15.21과 맞는 것으로 판단했다. 트레이스에서 부모 관계를 직접 펼쳐 보지는 않았다.
- 네 번째 칸 `UWorld_SendAllEndOfFrameUpdates`가 그림의 "나머지 81.0ms" 안에 든다는 것은 앞 세 타이머 안에서 불리지 않는 월드 틱 단계라는 판단이다. 그 타이머의 값은 읽지 않았다.
