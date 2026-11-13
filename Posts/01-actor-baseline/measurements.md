# 01 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-13 16:52:49\~21:00:00). 경위는 [작업 기록의 측정 절](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3) |
| 빌드 | Test 패키지 df41c13, CSV 메타데이터 `labbuild=df41c131c650` |
| 조건이 바뀐 이유 | 애니메이션 예산 기본값을 2ms에서 4ms로 올렸고([ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md)), 오브의 저격 시점 경합을 고쳐(62b6a42) 전 조건을 다시 쟀다. 이 글의 값 가운데 예산을 쓰는 것은 `S1-budget`뿐이다 |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L1-full`(`-LabStage=M0 -LabVariant=full`), `S1-uro`(`-LabVariant=uro`), `S1-budget`(`-LabVariant=budget`). 인자와 기대 메타데이터는 `Scripts/full-conditions.json` |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full3/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full3/runs.csv)(회차별 한 행) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full3/full-n-at-60fps.csv)(`Scripts/plot.py`가 측정 N 사이를 로그 보간) |
| 트레이스 | 같은 빌드(Test 패키지 df41c13)에서 뜬 트레이스다. 실행 라벨 `Full3traceser-L1-full-N10000-P1-r1`(`Saved/Traces/`, 직렬, 1만 P1, 한 회). 25\~75초 창을 `Scripts/trace-export.ps1 -Command TimerStatistics`로 내보냈다(저장소 밖 `%TEMP%\horde-trace\F3-traceser-L1-full-GT.csv`). 트레이스 실행의 경위는 [작업 기록의 측정 절](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3) |

변형의 구현: URO는 메시의 `bEnableUpdateRateOptimizations = true`이고 간격은 `FAnimUpdateRateParameters` 기본값이다(`Source/HordeOptLab/LabMinion.cpp:80-83` `ALabMinion::SetupVisual`). Budget은 메시를 `USkeletalMeshComponentBudgeted`로 바꾸고(`ALabMinionBudget` 생성자 `LabMinion.cpp:244-247`), Significance Manager가 시점에서의 거리 25 / 50 / 100 / 250m 안에 중요도 1 / 0.75 / 0.5 / 0.25, 그 밖에 0을 준다(`LabTierSignificance` `LabMinion.cpp:229-241`, `FLabScenario::BudgetTiers` `LabConfig.h:161`). 시점은 게임 모드가 매 틱 넘기는 카메라 트랜스폼이다(`LabGameMode.cpp:637` `Sig->Update`). 그래서 본문은 "카메라와의 거리"로 적었다(P1 카메라는 오브 위 9m에서 제자리 회전한다). 예산은 `a.Budget.BudgetMs 4`(`FLabScenario::BudgetMs` `LabConfig.h:162-163`, `LabGameMode.cpp:372-375`에서 `a.Budget.Enabled 1`과 함께 설정. 실행 메타데이터 `LabBudgetMs` = 4, `summary.csv` `budget_ms`).

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. "(그림)"은 본문 산문이 아니라 그림에만 쓴 값이다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차(r1 / r2 / r3) |
| --- | --- | --- | --- | --- |
| 액터 군중 프레임 시간 444ms | 444.279 | `L1-full` 10000 P1 | `frame_p50_ms` | 449.171 / 441.687 / 444.279 |
| 액터 군중 게임 스레드 429ms | 429.438 | 같음 | `game_p50_ms` | 433.98 / 425.808 / 429.438 |
| 액터 군중 렌더 스레드 29.8ms | 29.797 | 같음 | `render_p50_ms` | 28.86 / 29.797 / 30.041 |
| 액터 군중 GPU 50.2ms | 50.168 | 같음 | `gpu_p50_ms` | 50.952 / 50.145 / 50.168 |
| + URO 프레임 시간 430ms | 429.697 | `S1-uro` 10000 P1 | `frame_p50_ms` | 432.829 / 429.697 / 426.468 |
| + URO 게임 스레드 415ms | 414.552 | 같음 | `game_p50_ms` | 417.181 / 414.552 / 411.656 |
| + URO 렌더 스레드 28.8ms | 28.827 | 같음 | `render_p50_ms` | 28.827 / 28.793 / 30.708 |
| + URO GPU 50.7ms | 50.696 | 같음 | `gpu_p50_ms` | 50.696 / 50.435 / 50.918 |
| + Budget 프레임 시간 412ms | 411.565 | `S1-budget` 10000 P1 | `frame_p50_ms` | 412.167 / 411.565 / 403.773 |
| + Budget 게임 스레드 396ms | 395.76 | 같음 | `game_p50_ms` | 396.42 / 395.76 / 387.869 |
| + Budget 렌더 스레드 29.6ms | 29.635 | 같음 | `render_p50_ms` | 29.097 / 29.635 / 30.059 |
| + Budget GPU 49.9ms | 49.936 | 같음 | `gpu_p50_ms` | 49.066 / 50.53 / 49.936 |
| "이 구성에서는 게임 스레드가 가장 느리다" | | | `bottleneck` = `game`(세 행 모두) | |
| 16.7ms의 26.7배 | 26.66 | 계산 | 444.279 ÷ 16.667 | |
| 1,000 게임 스레드 17.0ms | 17.018 | `L1-full` 1000 P1 | `game_p50_ms` | 16.847 / 17.165 / 17.018 |
| 1천을 1만으로 늘리면 170ms(예상) | 170.18 | 계산 | 17.018 × 10 | |
| (그림) 500 프레임 시간 9.50ms | 9.502 | `L1-full` 500 P1 | `frame_p50_ms` | 9.502 / 9.418 / 9.561 |
| (그림) 1,000 프레임 시간 18.0ms | 17.955 | `L1-full` 1000 P1 | `frame_p50_ms` | 17.785 / 18.086 / 17.955 |
| (그림) 2,000 프레임 시간 37.1ms | 37.087 | `L1-full` 2000 P1 | `frame_p50_ms` | 37.16 / 36.991 / 37.087 |
| (그림) 1만 프레임 시간 느린 쪽 5% 경계 470ms | 469.662 | `L1-full` 10000 P1 | `frame_p95_ms` | 474.961 / 469.662 / 468.347 |
| 1,000 느린 쪽 5% 경계 20.2ms | 20.206 | `L1-full` 1000 P1 | `frame_p95_ms` | 20.206 / 20.349 / 20.152 |
| (그림) 개체당 게임 스레드 500: 17.9µs | 17.90 | 계산 | 8.949 ms ÷ 500 | 게임 스레드 8.949 / 8.858 / 8.995 |
| (그림) 개체당 게임 스레드 1,000: 17.0µs | 17.02 | 계산 | 17.018 ms ÷ 1000 | 위 1,000 행 |
| (그림) 개체당 게임 스레드 2,000: 17.6µs | 17.64 | 계산 | 35.287 ms ÷ 2000 | 게임 스레드 35.346 / 35.186 / 35.287 |
| 개체당 게임 스레드 1만: 42.9µs | 42.94 | 계산 | 429.438 ms ÷ 10000 | 위 `L1-full` 10000 |
| "17\~18µs로 평평" | 17.02\~17.90 | 계산 | 위 세 값 | |
| 2천 → 1만, 게임 스레드 12.2배 | 12.17 | 계산 | 429.438 ÷ 35.287 | |
| 60fps를 지키는 최대 개체 수 766 | 766 | `1 L1-full` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준은 902(`n_p50`) |
| 1만은 한계의 13.1배 | 13.05 | 계산 | 10000 ÷ 766 | |
| URO 프레임 시간 −14.6ms(3.28%) | −14.582, −3.282% | 계산 | 429.697 − 444.279, ÷ 444.279 | |
| URO 게임 스레드 −14.9ms | −14.886 | 계산 | 414.552 − 429.438 | (−3.47%) |
| Budget 프레임 시간 −32.7ms(7.36%) | −32.714, −7.363% | 계산 | 411.565 − 444.279, ÷ 444.279 | |
| Budget 게임 스레드 −33.7ms | −33.678 | 계산 | 395.76 − 429.438 | (−7.84%) |
| "세 회차의 흔들림보다 크다" | 프레임 시간 범위 7.482(`L1`), 6.361(`uro`), 8.394(`budget`). 게임 스레드 범위 8.172 / 5.525 / 8.551 | 계산 | [measurement.md](../../Docs/Guides/measurement.md) "차이 판정": 변화가 두 범위 중 큰 쪽보다 크다. URO 14.582 > 7.482, Budget 32.714 > 8.394 | |
| 게임 스레드 한 프레임 455ms | 454.55 | 직렬 트레이스 L1 1만 P1 | `FEngineLoop::Tick` Incl ÷ 110 프레임 | 한 회 |
| CharacterMovement 틱 329ms | 329.43 | 같음 | `UCharacterMovementComponent_TickComponent` Incl | 한 회 |
| 72.5%(그림은 72%) | 72.47% | 계산 | 329.43 ÷ 454.55 | |
| (그림) 스켈레탈 메시 틱 27.6ms · 6% | 27.57 | 같음 | `USkinnedMeshComponent_TickComponent` Incl(Excl 10.94). 6.07% | 한 회 |
| (그림) 애니 평가 마무리 14.5ms · 3% | 14.53 | 같음 | `USkeletalMeshComponent_CompleteParallelAnimationEvaluation` Incl. 3.20% | 한 회 |
| 애니메이션 쪽 두 타이머 합 42.1ms | 42.10 | 계산 | 27.57 + 14.53 | |
| 게임 스레드의 9.26% | 9.26% | 계산 | 42.10 ÷ 454.55 | |
| 애니메이션을 지워도 남는 약 412ms | 412.45 | 계산 | 454.55 − 42.10. 트레이스 한 회에서 뺀 어림이다 | |
| Budget이 줄인 게임 스레드 33.7ms는 이 몫(42.1ms)보다 작다 | 33.678 < 42.10 | 계산 | 측정 실행의 중앙값(`S1-budget` − `L1-full` `game_p50_ms`)과 트레이스 한 회의 합을 견준 어림이다 | |
| (그림) 나머지 83ms · 18% | 83.02 | 계산 | 454.55 − 329.43 − 27.57 − 14.53. 18.26% | |
| 개체당 CharacterMovement 32.9µs | 32.94 | 계산 | 329.43 ms ÷ 10000. 실제 호출은 프레임당 9,992회(1,099,110 ÷ 110)라 호출당은 33.0µs | |
| 스크린샷의 프레임 하나 455ms | 454.55 | 병렬 트레이스 `Full3trace-L1-full-N10000-P1-r1`, 약 49.4\~49.9초 지점의 프레임 하나 | Timing 뷰의 선택 구간 길이(Callees 패널의 `FEngineLoop::Tick` 454.55, `Tick_Engine` 453.92) | 한 프레임 |
| 스크린샷의 CharacterMovement 325ms | 325.49 | 같음 | Timers 패널 `UCharacterMovementComponent_TickComponent` Incl(Excl 324.29) | 한 프레임 |
| 스크린샷의 호출 수 9,985회 | 9,985 | 같음 | Timers 패널 Count | 한 프레임 |
| 71.6% | 71.61% | 계산 | 325.49 ÷ 454.55 | |
| 하위 스텝 1회 / 8회 | | 계산 | 아래 "엔진 소스". 프레임 50ms 미만이면 1회. 1만은 델타가 0.4초로 잘려 0.4 ÷ 0.05 = 8회(일곱 번째까지 `Min(0.05, 남은 시간 × 0.5)` = 0.05, 여덟 번째는 남은 0.05초를 모두 쓴다) | |
| 60ms면 30ms씩 두 번 | | 계산 | `Min(0.05, 0.06 × 0.5)` = 0.03, 남은 0.03은 50ms 이하라 그대로 쓴다 | |
| 기본값 0.05초, 8회 | | 엔진 소스 | 아래 "엔진 소스" | |
| 0.4초 | 0.4 | 엔진 설정 | `BaseGame.ini:199` | |
| URO 0.24, 0.12, 네 프레임에 한 번 | | 엔진 소스 | 아래 "엔진 소스" | |
| URO 기준은 화면 반지름의 제곱. LOD 화면 크기로 약 0.98, 0.69. 건너뛴 프레임은 보간 | | 엔진 소스, 계산 | `SkeletalRender.cpp:173,203`(`MaxDistanceFactor`에 `ScreenRadiusSquared`를 넣고, LOD는 `Square(ScreenSize * 0.5)`와 견준다), `SkinnedMeshComponent.cpp:310-318`. 2 × √0.24 = 0.98, 2 × √0.12 = 0.69 | |
| Budget: 넘는 몫의 0.8배, 2\~10프레임 간격, 최근 1초 | | 엔진 소스 | `AnimationBudgetAllocator.cpp:174`(최근 1초), `:567`(`AlwaysTickFalloffAggression`), `AnimationBudgetAllocatorParameters.h:34`(`MaxTickRate = 10`), `:49`(0.8) | |
| 개체당 시간 2.5배 | 2.52 | 계산 | 42.94 ÷ 17.018 | |
| 예산 4ms, 거리 단계 25 / 50 / 100 / 250m, 중요도 1 / 0.75 / 0.5 / 0.25 / 0 | 4.0 | 코드, 측정 | `LabConfig.h:161-163`, `LabMinion.cpp:229-241`. `S1-budget`의 `budget_ms` = 4(세 회차 모두) | |

`L1-full` 10000 P1에는 버리는 첫 실행 `Full3-L1-full-N10000-P1-r0`(프레임 p50 443.584)이 `runs.csv`에 있다. 요약은 r1\~r3만 쓴다.

## 측정 기록의 변경

- 2026-11-13: 트레이스 값을 Full3 트레이스(df41c13)에서 다시 뽑았다. 직렬 `Full3traceser-L1-full-N10000-P1-r1`에서 446.43 / 322.20 / 27.31 / 15.71이 454.55 / 329.43 / 27.57 / 14.53이 됐다. 애니메이션 쪽 두 타이머의 합은 42.10이고, Budget이 줄인 게임 스레드 33.7ms는 여전히 그보다 작다.
- 2026-11-13: Insights 스크린샷을 Full3 병렬 트레이스로 다시 찍었다(코디네이터). 캡션의 한 프레임 값이 449 / 323ms / 10,052회(72%)에서 455 / 325ms / 9,985회(71.6%)가 됐다.

- 2026-11-13: 측정값을 Test 패키지 08e45f2의 측정에서 df41c13의 최종 측정으로 바꿨다. 애니메이션 예산 기본값을 2ms에서 4ms로 올리고(ADR-0016) 오브의 저격 시점 경합을 고쳐(62b6a42) 전 조건을 다시 쟀기 때문이다. 바뀐 값은 이 글의 모든 수치(1단계 444ms, 최대 개체 수 766, URO −14.6ms, Budget −32.7ms 등)이고, 트레이스 값과 스크린샷은 그대로다.
- 2026-11-13: 측정값을 Test 패키지 730ff18의 측정에서 08e45f2의 최종 측정으로 바꿨다. 200m 밖 입자 메시를 원본 LOD4로 바꿔(bf1a4c8) 측정 조건이 달라졌고, 규약대로 전 조건을 다시 쟀기 때문이다.
- 2026-11-13: Insights 스크린샷에서 트레이스 파일 이름이 보이는 제목 줄을 잘라 냈다. 값과 빨간 상자는 그대로다.
- 2026-11-13: Insights 스크린샷과 그 캡션의 값(449 / 323ms / 10,052회)을 최종 측정 빌드(08e45f2)의 병렬 트레이스 `Full2trace-L1-full-N10000-P1-r1`에서 다시 찍었다. 처음 값은 730ff18의 트레이스에서 찍은 448 / 322ms / 10,048회였다.
- 2026-11-13: 트레이스 값(446.43 / 322.20 / 27.31 / 15.71)은 최종 측정 빌드(08e45f2)의 트레이스 `Full2traceser-L1-full-N10000-P1-r1`에서 다시 뽑았다. 처음 쓴 값은 730ff18의 트레이스 `Full1traceser-L1-full-*`에서 읽은 446.43 / 323.49 / 27.24 / 14.70이었다(같은 방법으로 옛 트레이스를 다시 계산하면 옛 값과 일치했다). 값을 뽑는 `Scripts/trace-export.ps1`은 `UnrealInsights.exe -NoUI` 명령줄 내보내기다. 두 빌드 사이에 `Source/`와 `Config/`는 같고, 바뀐 것은 `Content/Lab/FX/`의 배경 무리 에셋 셋뿐이다(`git diff --stat 730ff18 08e45f2`). 1단계는 배경 무리를 쓰지 않는다.

## 화면의 HUD와 표의 차이

요약의 1단계 화면(`step1-L1-full.webp`) 왼쪽 위 HUD에 보이는 프레임 시간(움직이는 WebP의 한 프레임을 열어 본 값 478.7ms)은 표의 값(444ms)보다 높다. 그 화면은 측정 실행이 아니라 촬영 실행(`Full3shot-L1-full-N10000-P1-r2`, 첫 시도 r1은 창을 앞에 두지 못해 실패, [captures.csv](../../Measurements/Full3/captures.csv))에서 찍었고, 4K 영역 화면 녹화 부하가 섞인다. 수치는 표(Test 패키지 측정 실행)의 값만 쓴다. 촬영 경위는 [Worklog/Q7.md](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3)의 "촬영 끝" 줄에 있다.

## 두 트레이스

본문은 출처가 다른 트레이스 값 두 가지를 쓴다.

| 쓰인 곳 | 트레이스 | 구간 | 값 |
| --- | --- | --- | --- |
| 그림 `actor-frame-flow.svg`의 막대, 본문 455 / 329 / 42.1ms | 직렬 실행 `Full3traceser-L1-full-N10000-P1-r1`(`mass.FullyParallel` 끔) | 25\~75초, 110프레임의 프레임당 평균 | `FEngineLoop::Tick` 454.55, CharacterMovement 329.43 |
| 스크린샷, 캡션, 스크린샷 아래 문장 455 / 325ms / 9,985회 | 병렬 실행 `Full3trace-L1-full-N10000-P1-r1` | 약 49.4\~49.9초 지점의 프레임 하나 | 선택 구간 454.55, CharacterMovement 325.49 |

병렬 트레이스 `Full3trace-L1-full-N10000-P1-r1`의 같은 25\~75초 구간 평균(110프레임)은 `FEngineLoop::Tick` 454.55, CharacterMovement 328.39ms다(`%TEMP%\horde-trace\F3-trace-L1-full-GT.csv`). 직렬 트레이스와 0.00ms, 1.0ms 차이다. 1단계는 Mass 프로세서가 거의 돌지 않아(`PrePhysics` 0.09ms) 직렬·병렬 설정이 액터 비용에 닿지 않고, 차이는 실행 사이의 흔들림이다. 그래서 스크린샷은 병렬 트레이스로 찍는다.

같은 트레이스의 25\~75초 구간 합(내보내기)은 `FEngineLoop::Tick` 50.00초(110회), `UCharacterMovementComponent_TickComponent` Incl 36.12초(1,094,284회, 프레임당 9,948회)다. 스크린샷의 프레임 하나는 이 구간 안의 한 프레임이고, 길이 454.55ms는 구간 평균(454.55ms)과 같다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/actor-summary.svg` | 요약 그림. 세 구성(액터 군중, + URO, + Budget)의 프레임·게임 스레드·렌더 스레드·GPU 시간 막대(1만 P1, 같은 축척)와 16.7ms 선 | 위 1만 P1 행 열두 개 |
| `images/actor-anim-techniques.svg` | 결과 그림. URO와 Budget(예산 4ms)이 줄인 프레임·게임 스레드 시간, 덧댄 뒤의 프레임 시간, 비교용 애니메이션 쪽 두 타이머의 합 42.10(점선). 막대는 측정 실행 세 회차의 중앙값이고, 점선은 같은 빌드의 직렬 트레이스 한 회의 값이다 | 위 변화 행, 트레이스 행 |
| `images/actor-frame-flow.svg` | 한 개체가 한 프레임에 하는 일 네 칸(이동 컴포넌트 틱, 스켈레탈 메시 틱과 AnimBP 갱신, 애니메이션 평가 마무리, 프레임 끝 렌더 갱신)과 칸마다 엔진 타이머 이름. 오른쪽 막대 길이가 1만의 프레임당 시간(329.43, 27.57, 14.53). 네 번째 칸 `UWorld_SendAllEndOfFrameUpdates`의 값은 따로 읽지 않아, 나머지 83.02ms(이 칸과 그 밖의 일)를 점선 막대로 보였다. 맨 아래 16.7ms 막대는 같은 축척 | 위 트레이스 행 |
| `images/actor-frame-vs-n.svg` | 위: 프레임 시간 중앙값·느린 쪽 5% 경계(로그-로그), 16.7ms 선, 50ms 선, 60fps를 지키는 최대 개체 수 766. 아래: 개체당 게임 스레드 시간(µs)과 하위 스텝 횟수, 예상선(1천의 17.02µs로 일정) | 위 `L1-full` 행 |
| `images/insights-gamethread.jpg` | Unreal Insights에서 `Full3trace-L1-full-N10000-P1-r1`을 연 화면이다. 왼쪽 빨간 상자는 Timing 뷰에서 고른 프레임 하나(약 49.4\~49.9초 지점, 454.55ms)다. 오른쪽 빨간 상자는 Timers 패널을 "CharacterMovement"로 거른 결과다(`UCharacterMovementComponent_TickComponent` Count 9,985, Incl 325.49ms, Excl 324.29ms). Callees 패널이 켜져 있어 `FEngineLoop::Tick` 454.55ms, `Tick_Engine` 453.92ms 등이 보인다. 창을 최대화하고 창 캡처(PrintWindow)로 한 번에 찍어 2304폭으로 줄였다. 트레이스 파일 이름이 보이는 제목 줄은 잘라 내고 빨간 상자와 단계 이름을 그렸다. 2026-11-13에 새 트레이스로 다시 찍었다 | 위 "두 트레이스" |
| `../../Docs/figures/steps/step1-L1-full.webp` | 1단계 화면(HUD 켬). HUD 값은 위 "화면의 HUD와 표의 차이" | 촬영 경위는 [Worklog/Q7.md](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3)의 "촬영 끝" 줄 |

SVG 네 개는 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/01-actor-baseline/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽고, 트레이스 값 네 개(454.55 / 329.43 / 27.57 / 14.53)는 스크립트의 상수다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가져 GitHub의 밝은·어두운 테마 모두에서 같은 색으로 보인다. 처음 글의 `actor-gamethread-breakdown.svg`(같은 네 값의 가로 막대)는 `actor-frame-flow.svg`에 합쳐 지웠다.

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
- 0.4초 상한 때문에 1만 실행에서는 게임 시간이 실제 시간보다 느리게 흐른다(444ms 프레임에 0.4초). 사망 수가 2단계 1만과 약 0.3% 차이로 맞는 이유(최종 측정의 초당 사망 62.18 / 62.00)(동등성 검사 통과)와 이 상한의 관계는 보지 않았다.
- Budget·URO 실행의 트레이스는 없다. Budget이 줄인 33.7ms가 애니메이션 타이머에서 났는지, URO가 Budget의 절반쯤만 줄인 이유는 확인하지 않았다. 글 재료의 "포즈 계산이 워커에서 돌아서" 설명은 쓰지 않았다.
- URO 실행에서 개체마다 실제로 고른 갱신 간격(화면 크기 분포)은 세지 않았다. 본문의 URO 설명은 엔진 기본값의 규칙이다.
- 트레이스는 직렬(`mass.FullyParallel` 끔) 실행이다. 1단계는 Mass 프로세서가 거의 돌지 않아(`PrePhysics` 0.09ms) 병렬 트레이스와 값이 비슷하다(병렬 `F3-trace-L1-full-GT.csv`: `FEngineLoop::Tick` 454.55, CharacterMovement 328.39).
- 그림의 "AnimBP 갱신이 스켈레탈 메시 틱 안에 있다"는 Incl − Excl(27.57 − 10.94 = 16.63)이 `USkeletalMeshComponent_TickAnimation` 15.69와 맞는 것으로 판단했다. 트레이스에서 부모 관계를 직접 펼쳐 보지는 않았다.
- 네 번째 칸 `UWorld_SendAllEndOfFrameUpdates`가 그림의 "나머지 83ms" 안에 든다는 것은 앞 세 타이머 안에서 불리지 않는 월드 틱 단계라는 판단이다. 그 타이머의 값은 읽지 않았다.
