# 04 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정 `Full3`(2026-11-13 16:52\~21:00, 178회, 실패 0). 경위는 [작업 기록의 Full3 절](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3) |
| 다시 잰 이유 | 애니메이션 예산을 2ms에서 4ms로 올렸고([ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md)), 오브의 저격 시점 경합을 고쳤다(62b6a42). 그래서 전 조건을 새 묶음으로 다시 쟀다 |
| 빌드 | Test 패키지 df41c13, CSV 메타데이터 `labbuild=df41c131c650`. 실행 라벨은 `Full3-<조건>-N<호드 총수>-P1-r<회차>`(예: `Full3-L4-mass-N10000-P1-r1`) |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L3-vatcull`(`-LabStage=M2 -LabVariant=vatcull`), `L4-mass`(`-LabStage=M3 -LabVariant=mass`), `L8-q4final`(`-LabStage=M5 -LabVariant=q4final`) |
| 빼 보기·하위 실험 | `A8-hydrate`(`-LabHydrate=0`), `A8-budget`(`-LabScreenSigBudget=0`), `A8-fix`(`-LabHydrateFixMask=0`), `S8-col`(`-LabActorCollision=1`). 모두 `L8-q4final`에 인자 하나를 더한다 |
| 기대 메타데이터 | `Scripts/full-conditions.json`. `L4-mass`는 `LabTechHydrate=1`, `LabTechAnimBudget=1`, `LabTechHydrateFix=1`, `LabActorCap=500`, `LabHydrateM=30`, `LabActorCollision=0`, `LabBudget=1` |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full3/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full3/runs.csv)(회차별 한 행), [빼 보기 표](../../Docs/figures/full3/full-ablation.csv) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full3/full-n-at-60fps.csv)(`Scripts/plot.py`가 측정 N 사이를 로그 보간) |
| 예산 값 비교 | 본문 "예산을 어떻게 정했나"는 참고 묶음 `Budget1`의 값이다. 아래 "예산 값 비교" 절에 따로 적는다. 최종 측정과 이어 읽지 않는다 |
| 트레이스 | 본문 "결과"의 트레이스 문단 값은 아래 "트레이스 값"이다. 앞선 측정 빌드의 직렬 트레이스 한 회씩이다 |

## 4단계의 구현(3단계와 다른 곳)

- 표현 LOD: `ULabCrowdSubsystem::BuildEntityConfig`(`Source/HordeOptLab/LabCrowdSubsystem.cpp:98-118`). `Stage >= M3`(`:98`)라 High의 표현이 `HighResSpawnedActor`다(`:112`, `UsesHydration()`이 참). 액터 클래스는 `bBudget`(`:101`, `bScreenSigBudget` 참)이라 `ALabMinionMassActorBudget`·`ALabMinionMassActorBudgetLarge`다(`:105`, `:109`). 액터 관리 클래스 `ULabActorManagement`, 준비 대기와 한 프레임 더 두기(`:115-117`).
- 구간과 상한: `Bands` = {0, 3000, 8000, 20000}cm, `MaxCounts` = {500, 5000, 무제한, 무제한}(`:125-131`). `HydrateCm()` = `HydrateM`(기본 30, `LabConfig.h:79`) × 100, `ActorCap` 기본 500(`LabConfig.h:78`). 화면 밖 Medium 경계는 1500cm(`:144`, 변형 `mass`는 `bBaseIsVisible`이 거짓). 3단계는 `bM3`가 거짓이라 화면 밖 경계도 30m이지만 High·Medium 모두 VAT 인스턴스라 그리는 것은 같다.
- 상한이 두 종류의 합계라는 것: [engine-notes.md](../../Docs/Reference/engine-notes.md) 19절(`MassVisualizationTrait.cpp:145`, `MassVisualizationLODProcessor.cpp:70-110`). 이 글은 줄을 다시 열지 않았다.
- 액터: `ALabMinionMassActor`(`LabMinion.cpp:275-281`) 액터 틱 끔(`bCanEverTick = false`, `:279`). 부모 `ALabMinionPawn`(`LabMinion.h:115`, `:141`)의 생성자가 CharacterMovement의 틱을 끈다(`LabMinion.cpp:199-205`, `:212`, 컴포넌트는 `ACharacter`가 필수로 만들어 남는다). `ALabMinionMassActorBudget`(`:380-383`)이 메시를 `USkeletalMeshComponentBudgeted`로 바꾼다. 캡슐 콜리전(`:305-312`), 기본 `EActorCollision::Off`(`LabConfig.h:327`).
- 동기화: `ULabActorSyncProcessor::Execute`(`LabActorSyncProcessor.cpp:119-432`). 카메라 위치는 `Crowd->SnipeView.Location`(`:133`). 위치(`:253`), 시작 시각(`:263-267`), 준비 대기 두 프레임(`:270-277`, `:287-295`), 모션 벡터 지우기(`:333-338`), 화면 크기 중요도(`:348-356`), 갱신·보간·건너뜀 스탯(`:357-372`), 사망 색(그 뒤). 게임 모듈이 M3 이상에서 등록한다(`HordeOptLab.cpp`, code-tour.md "실행 인자에서 설정까지"). 줄 번호는 df41c13의 소스다.
- 예산: `ALabGameMode::ApplyVariantCVars`(`LabGameMode.cpp:364-382`). `FLabScenario::BudgetMs = 4.f`(`LabConfig.h:163`), `-LabBudgetMs=`로 바꾼다(`LabConfig.cpp:131-136`).
- 전환 대책 비트(`LabConfig.h:368-372`): `FixStartTime` 1, `FixWait` 2, `FixPassHide` 4, `FixLift` 8, `FixTintReset` 16. 기본 마스크 31(`LabConfig.h:225`). 진홍 지우기는 `ALabMinionMassActor::PrepareForPooling_Implementation`(`LabMinion.cpp:349-359`), 풀에서 꺼낼 때 다시 숨기기는 `PrepareForGame_Implementation`(`:361-376`), 준비됨 판정은 `ULabActorManagement::IsActorReadyForRepresentation`(`LabActorSyncProcessor.cpp:40-44`).
- 같은 것: VAT 프로세서(`ULabVatISMProcessor`, M2\~M4), 그림자 켬, 200m 밖 지우기, 로직 프로세서와 시나리오.

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. "(그림)"은 그림에만 쓴 값이다. 세 회차 열은 r1 / r2 / r3이다. 아래 값은 모두 P1이다.

### 1만에서 3단계와 4단계(요약 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 3단계 프레임 시간 7.91ms | 7.909 | `L3-vatcull` 10000 | `frame_p50_ms` | 7.909 / 7.777 / 7.927 |
| 3단계 게임 스레드 4.16ms | 4.156 | 같음 | `game_p50_ms` | 4.156 / 2.959 / 4.176 |
| (그림) 3단계 렌더 스레드 4.12ms | 4.122 | 같음 | `render_p50_ms` | 4.153 / 4.079 / 4.122 |
| 3단계 GPU 7.53ms | 7.529 | 같음 | `gpu_p50_ms` | 7.529 / 7.428 / 7.547 |
| 4단계 프레임 시간 9.42ms | 9.422 | `L4-mass` 10000 | `frame_p50_ms` | 9.422 / 9.324 / 9.568 |
| 4단계 게임 스레드 9.14ms | 9.136 | 같음 | `game_p50_ms` | 9.136 / 9.04 / 9.267 |
| (그림) 4단계 렌더 스레드 4.82ms | 4.817 | 같음 | `render_p50_ms` | 4.802 / 4.817 / 4.965 |
| (그림) 4단계 GPU 6.93ms | 6.926 | 같음 | `gpu_p50_ms` | 6.926 / 6.838 / 6.957 |
| 병목 GPU → 게임 스레드 | `gpu` → `game` | 위 두 조건 | `bottleneck` | |
| 프레임 시간 +1.51ms | 1.513 | 계산 | 9.422 − 7.909. 범위 0.150 / 0.244보다 크다 | |
| 게임 스레드 +4.98ms, "두 배 넘게" | 4.980, 2.20배 | 계산 | 9.136 − 4.156. 9.136 ÷ 4.156. 범위 1.217 / 0.227보다 크다 | |
| GPU −0.603ms | −0.603 | 계산 | 6.926 − 7.529. 범위 0.119 / 0.119보다 크다 | |
| 렌더 스레드 +0.695ms | 0.695 | 계산 | 4.817 − 4.122. 범위 0.074 / 0.163보다 크다 | |
| 드로우 콜 123 → 1,320 | 123 / 1320 | 위 두 조건 | `draw_calls_p50` | 123 / 123 / 123, 1320 / 1320 / 1319 |
| 액터 프레임 평균 373 | 372.73 | `L4-mass` 10000 | `actors_mean`(`Lab/Actors` 평균) | 372.74 / 372.73 / 372.61 |

`L3-vatcull` 10000 P1의 게임 스레드는 회차 사이가 넓다(r2 2.959, 나머지 4.16 안팎). 앞선 측정에서도 한 회차만 다른 값이었다(4.14 / 3.206 / 3.136). 규칙대로 중앙값 4.156을 쓴다. 3단계의 병목은 GPU라 프레임 시간은 이 차이와 무관하다.

### 여유, 예상, 실제(원리의 예상, 결과 그림)

| 본문 값 | 정밀한 값 | 조건 | 출처 | 세 회차 |
| --- | --- | --- | --- | --- |
| 게임 스레드의 여유 3.37ms | 3.373 | 계산 | 3단계 `gpu_p50_ms` − `game_p50_ms` = 7.529 − 4.156 | |
| 1단계 액터 단가 17.0µs | 17.018 | `L1-full` 1000 | `game_p50_ms` 17.018 ÷ 1000(01번 글의 측정 기록과 같다) | 16.847 / 17.165 / 17.018 |
| 여유가 차는 액터 198개 | 198.20 | 계산 | 3.373 ÷ 0.017018 | |
| 1단계 단가로 본 예상 6.34ms | 6.343 | 계산 | 372.73 × 0.017018 | |
| 액터 하나에 13.4µs | 13.361 | 계산 | 4.980 ÷ 372.73 × 1000 | |
| 여유를 1.61ms 넘었다(그림 문장 "여유를 넘었다") | 1.607 | 계산 | 4.980 − 3.373 | |
| 오브 30m 안 평균 627개 | 627.22 | `L3-vatcull` 10000 | `near30_mean`(오브 수평 30m 안 개체 수 평균) | 중앙값만(`runs.csv`의 `near30_mean`) |
| 카메라는 오브 뒤 15m | 15m | 확정값 | [measurement.md](../../Docs/Guides/measurement.md) "확정값" P1 행 | |

"1단계 액터의 단가"는 1단계(`ACharacter`, CharacterMovement, AnimBP)의 개체당 게임 스레드 시간이고, 4단계 액터에는 없는 이동 컴포넌트와 액터 틱이 들어 있다. 그래서 예상은 상한으로만 읽는다.

### 호드 총수별 값(최대 개체 수 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 3단계 5천 6.73 | 6.728 | `L3-vatcull` 5000 | `frame_p95_ms` | 6.945 / 6.728 / 6.672 |
| (그림) 3단계 1만 8.48 | 8.478 | 10000 | 같음 | 8.494 / 8.236 / 8.478 |
| (그림) 3단계 2만 11.6 | 11.636 | 20000 | 같음 | 11.782 / 11.636 / 11.329 |
| (그림) 3단계 3만 15.3 | 15.349 | 30000 | 같음 | 15.601 / 15.349 / 15.326 |
| (그림) 4단계 5천 6.57 | 6.569 | `L4-mass` 5000 | 같음 | 6.569 / 6.767 / 6.538 |
| (그림) 4단계 1만 11 | 10.981 | 10000 | 같음 | 10.981 / 10.532 / 11.244 |
| (그림) 4단계 2만 16.7 | 16.72 | 20000 | 같음 | 16.617 / 17.278 / 16.72 |
| (그림) 4단계 3만 21.1 | 21.115 | 30000 | 같음 | 21.115 / 21.253 / 20.556 |
| 4단계 최대 개체 수 약 2만(그림 19,952) | 19952 | `4 L4-mass` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준(`n_p50`)은 23,601 |
| 3단계는 3만 안에서 한계에 닿지 않음 | 30000, `above range` | `3 L3-vatcull` | 같은 파일 `n_p95`, `note_p95` | |
| 액터 수 181 / 373 / 492 / 486 | 181.13 / 372.73 / 492.23 / 485.71 | `L4-mass` 5000\~30000 | `actors_mean` | 2만 492.24 / 492.12 / 492.23, 3만 485.58 / 485.71 / 485.81 |
| 5천에서 4단계 프레임 시간이 더 짧다 | 6.077 대 6.300(−0.223) | `L4-mass`·`L3-vatcull` 5000 | `frame_p50_ms`, 둘 다 `bottleneck` = `gpu` | 4단계 6.077 / 6.193 / 6.03, 3단계 6.432 / 6.3 / 6.265. 범위 0.163 / 0.167보다 크다 |
| 예산 대상 애니메이션 2.99ms(1만) | 2.994 | `L4-mass` 10000 | `budgeted_anim_ms`(엔진 통계 `AnimationBudget/GameThread/BudgetedAnimation`의 실행별 중앙값) | 2.976 / 2.994 / 3.083 |
| 1만 갱신도 보간도 하지 않은 액터: 대형 0%, 소형 0.02% | 0 / 0.0002 | `L4-mass` 10000 | `large_skipped_ratio`, `small_skipped_ratio` | 소형 보간 0.0002 |
| 2만 예산 대상 애니메이션 4ms에 닿음 | 4.003 | `L4-mass` 20000 | `budgeted_anim_ms` | 3.95 / 4.008 / 4.003 |
| 2만 소형 0.19% 건너뜀 | 0.0019 | `L4-mass` 20000 | `small_skipped_ratio`. 대형 0, 소형 보간 0.0036 | 0.0019 / 0.0049 / 0.0013 |
| 넘는 몫의 0.8배, 2\~10프레임 간격 | | 엔진 소스 | `AnimationBudgetAllocator.cpp:567`(`AlwaysTickFalloffAggression`), `AnimationBudgetAllocatorParameters.h:34`(`MaxTickRate = 10`), `:49`(0.8) | |

건너뜀 비율 `*_skipped_ratio`는 그 프레임에 갱신도 보간도 하지 않은 액터의 비율이고, 측정 구간 합의 비다(분자 = 갱신하지 않은 수 − 보간한 수, 분모 = 동기화한 액터 수. `LabActorSyncProcessor.cpp:357-372`, `Scripts/summarize.py`). 대형은 큰 미니언, 소형은 작은 미니언이다. 앞선 측정의 지표 `large_skip_ratio`는 보간한 프레임까지 건너뜀으로 셌고 소형을 세지 않았다([Worklog/Q7.md](../../Docs/Worklog/Q7.md) "건너뜀 지표"). 그 지표로 쓴 앞선 본문의 "1.29%"와 "49.5%"는 뺐다.

### 최종 구성에서 하나씩 바꾼 측정(빼 보기 그림, 본문)

기준은 `L8-q4final` 10000 P1(r1\~r3)이다. 묶음 끝 A/A(r11\~r13)는 쓰지 않았다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 최종 구성 프레임 9.75ms | 9.746 | `L8-q4final` 10000 | `frame_p50_ms` | 9.707 / 9.763 / 9.746 |
| (그림) 최종 구성 게임 스레드 9.59ms | 9.592 | 같음 | `game_p50_ms` | 9.559 / 9.616 / 9.592 |
| 하이드레이션을 빼면 프레임 −3.57ms | 6.174 − 9.746 = −3.572 | `A8-hydrate` | `frame_p50_ms` | 6.176 / 6.174 / 6.008 |
| 게임 스레드 −5.84ms | 3.753 − 9.592 = −5.839 | 같음 | `game_p50_ms` | 3.753 / 3.825 / 3.689 |
| 병목이 GPU로 바뀐다 | `gpu`(GPU 5.737) | 같음 | `bottleneck`, `gpu_p50_ms` | GPU 5.738 / 5.737 / 5.641 |
| 액터 평균 457개, 하나에 12.8µs | 457.22, 12.771 | `L8-q4final` 10000 | `actors_mean`. 5.839 ÷ 457.22 × 1000 | 457.29 / 457.21 / 457.22 |
| 애니메이션 예산을 빼면 프레임 −0.292ms | 9.454 − 9.746 = −0.292 | `A8-budget` | `frame_p50_ms` | 9.432 / 9.499 / 9.454 |
| (그림) 게임 스레드 −0.281ms | 9.311 − 9.592 = −0.281 | 같음 | `game_p50_ms` | 9.298 / 9.364 / 9.311 |
| 1만에서 4ms 예산이 하나도 건너뛰지 않는다 | 대형 0, 소형 0 | `L8-q4final` 10000 | `large_skipped_ratio`, `small_skipped_ratio`(보간도 0). 소형 최댓값 0.0015 | |
| 캡슐 콜리전을 켜면 프레임 +1.60ms | 11.345 − 9.746 = 1.599 | `S8-col` | `frame_p50_ms` | 11.369 / 11.345 / 11.171 |
| 게임 스레드 +1.10ms | 10.695 − 9.592 = 1.103 | 같음 | `game_p50_ms` | 10.739 / 10.695 / 10.567 |
| 전환 대책을 빼면 −0.104ms, 범위 안 | 9.642 − 9.746 = −0.104 | `A8-fix` | `frame_p50_ms` | 9.642 / 9.938 / 9.576. 범위 0.362 / 0.056보다 작다 |
| (그림) 전환 대책 게임 스레드 −0.091ms | 9.501 − 9.592 = −0.091 | 같음 | `game_p50_ms` | 9.501 / 9.797 / 9.433 |
| 최종 구성 40m, 상한 1,000 | 40, 1000 | `L8-q4final` | `hydrate_m`, `actor_cap`(`FLabQuality::Q4FinalHydrateM`·`Q4FinalActorCap`, `LabConfig.h:82,84`) | |

차이 판정([measurement.md](../../Docs/Guides/measurement.md) "차이 판정"): 프레임 범위는 `L8` 0.056, `A8-hydrate` 0.168, `A8-budget` 0.067, `S8-col` 0.198, `A8-fix` 0.362다. 앞의 세 차이는 두 범위 중 큰 쪽보다 크고, `A8-fix`는 작다. 빼 보기 행의 다른 값(참고): `A8-hydrate` 렌더 스레드 −1.302, GPU −0.015, 드로우 콜 1713 → 175.

앞선 측정에서는 예산을 뺀 행이 최종 구성보다 1.75ms 길었다. 그때 예산은 2ms였고 최종 구성의 대형 건너뜀(보간 포함 옛 지표)이 0.207이었다. 예산을 뺀 행의 프레임 시간은 두 측정에서 9.53, 9.45ms로 거의 같다. 바뀐 것은 최종 구성 쪽이다. 두 측정 사이에는 예산 값과 경합 수정이 함께 바뀌어, 이 문단은 본문에 쓰지 않았다.

### 설정값과 계산값

| 본문 값 | 값 | 근거 |
| --- | --- | --- |
| 화면 안 30m, 화면 밖 15m, 상한 500 | 3000 / 1500cm, 500 | 위 "4단계의 구현" |
| 예산 4ms | 4.0 | `LabConfig.h:163` `BudgetMs`, `LabGameMode.cpp:374-375`. 근거 [ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md) |
| `a.Budget.Enabled` 기본 0 | 0 | `AnimationBudgetAllocatorCVars.cpp:7` |
| 예산 기본 1ms | 1.0 | `AnimationBudgetAllocatorParameters.h:19` `BudgetInMs = 1.0f`, CVar `a.Budget.BudgetMs`(`AnimationBudgetAllocatorCVars.cpp:195`) |
| 할당기는 꺼진 채 시작 | | `LabGameMode.cpp:376` 주석, [engine-notes.md](../../Docs/Reference/engine-notes.md) 18절(`AnimationBudgetAllocator.cpp:75,115,135`) |
| 수요는 최근 1초 안에 그려진 메시 | 1.0초 | `AnimationBudgetAllocator.cpp:174,254-263`([engine-notes.md](../../Docs/Reference/engine-notes.md) 29절) |
| 보간과 건너뜀의 구분, 보간 간격 2\~6, 건너뜀 간격 최대 10 | | `AnimationBudgetAllocator.cpp:574-607,621-643`, `:454-500`(29절) |
| 소형이 대형보다 순위가 낮아 건너뜀이 소형에 몰린다 | | 정렬 `:327-342`와 중요도 식(바운드 반경 94 대 40). 측정: 최종 구성 2만 소형 0.4212, 대형 0.0036(`small_skipped_ratio`, `large_skipped_ratio`) |
| Significance Manager는 등록한 오브젝트마다 중요도 함수를 부른다, 게임이 시점을 넘긴다 | | `Engine/Plugins/Runtime/SignificanceManager/Source/SignificanceManager/Public/SignificanceManager.h:118`(`Update`), `:121`(`RegisterObject`), `:124`(`UnregisterObject`) |
| 매니저는 1단계 하위 실험(예산만 건 액터 호드)에서만 쓴다 | | `ALabMinionBudget::BeginPlay`(`LabMinion.cpp:249-262`, `RegisterObject` `:254`), `ALabGameMode::Tick`의 `Update`(`LabGameMode.cpp:631-639`), 조건 `S1-budget` |
| `LODSignificance`는 거리만 본다 | | LOD 정수 + 밴드 안 비율([engine-notes.md](../../Docs/Reference/engine-notes.md) 3절과 표현 LOD 절, `MassLODCalculator.h:409-416`). 변환식 `1 − LODSignificance ÷ 3`(`LabActorSyncProcessor.cpp:340-347`, 하위 실험 변형) |
| 화면 크기 중요도를 고른 근거 | | [plan.md](../../Docs/plan.md) "실험 E3b"(크기와 거리를 한 수로 합친다), [Worklog/M3b.md](../../Docs/Worklog/M3b.md) "E3 (3c)" |
| 대형이 소형보다 2.35배 중요 | 2.35 | 계산: 바운드 반경 94 ÷ 40([measurement.md](../../Docs/Guides/measurement.md) "확정값" 2종 행) |
| 소형 액터 88cm 가라앉음 | 88 | `ACharacter` 캡슐 반높이 기본값(`Engine/Source/Runtime/Engine/Private/Character.cpp:78` `InitCapsuleSize(34.0f, 88.0f)`). 이 프로젝트는 캡슐 크기를 바꾸지 않는다(`LabMinion.cpp:25-33`). 대형은 배율 2.0이라 그 두 배다 |
| 메시가 두 번 틱하면 준비됨 | 2 | `LabActorSyncProcessor.cpp:274` `PendingReadyFrames = 2`, `:287` |
| VAT는 정점마다 뼈 넷까지 | 4 | AnimToTexture 본 영향 기본값([engine-notes.md](../../Docs/Reference/engine-notes.md) 3절, `AnimToTextureDataAsset.h`). `Scripts/bake-vat.py`는 바꾸지 않는다 |
| VAT는 클립 사이를 섞지 않는다 | | `ULabVatISMProcessor::Execute`가 상태로 클립 하나를 고르고 그 클립의 프레임만 넣는다(`LabVatProcessor.cpp:151-157`, 03번 글의 측정 기록) |

## 예산 값 비교

본문 "예산을 어떻게 정했나"의 값이다. 참고 묶음 `Budget1`이고 최종 측정과 묶음이 다르다. 이 절의 값끼리만 비교한다.

| 항목 | 값 |
| --- | --- |
| 묶음 | `Budget1`(2026-11-13 14:57\~15:47, 36회, 실패 0). 경위는 [작업 기록의 Budget1 절](../../Docs/Worklog/Q7.md#2026-11-13-측정-budget1) |
| 빌드 | Test 패키지 56fab6f, `labbuild=56fab6f830bd`. 예산 기본값을 4ms로 바꾸기 전의 빌드라 모든 실행이 `-LabBudgetMs=`로 값을 준다 |
| 조건 | 최종 구성 `L8-q4final`의 인자에 `-LabBudgetMs=<0, 1, 2, 3, 4, 6>`만 더했다. 0이 끔이다. 호드 총수 1만과 2만, 오브 시점(P1), 세 회차. 다른 `a.Budget.*`는 엔진 기본값 |
| 라벨 | `Budget1-B<예산>-q4final-N<호드 총수>-P1-r<회차>` |
| 파일 | [요약 표](../../Measurements/Budget1/summary.csv)(행 `B<예산>-q4final`), [실행별 값](../../Measurements/Budget1/runs.csv), [그림 수치](../../Docs/figures/budget1/budget-values.csv) |
| 선택 기준과 결정 | [ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md). "구별되지 않는다"는 [measurement.md](../../Docs/Guides/measurement.md) "차이 판정"이다 |

호드 총수 1만, 갱신도 보간도 하지 않은 액터의 비율(세 회차 중앙값, 괄호는 r1 / r2 / r3):

| 예산 | 소형 | 대형 | 프레임 p50(ms) |
| --- | --- | --- | --- |
| 끔 | 0 | 0 | 9.27 (9.261 / 9.27 / 9.346) |
| 1ms | 0.8099 (0.8097 / 0.8099 / 0.8103) | 0.5868 (0.5866 / 0.5868 / 0.5872) | 6.495 (6.496 / 6.495 / 6.46) |
| 2ms | 0.4695 (0.4812 / 0.4695 / 0.4552) | 0.021 (0.029 / 0.021 / 0.0145) | 7.822 (7.822 / 7.757 / 7.848) |
| 3ms | 0.0646 (0.0526 / 0.0646 / 0.0712) | 0 (0.0001 / 0 / 0) | 9.278 (9.276 / 9.278 / 9.3) |
| 4ms | 0 (0 / 0.0014 / 0) | 0 | 9.478 (9.411 / 9.478 / 9.483) |
| 6ms | 0 | 0 | 9.458 (9.458 / 9.493 / 9.444) |

호드 총수 2만:

| 예산 | 프레임 p95(ms) | 세 회차 | 16.7ms 안 |
| --- | --- | --- | --- |
| 끔 | 18.997 | 19.548 / 18.921 / 18.997 | 아니다 |
| 1ms | 13.418 | 13.698 / 13.359 / 13.418 | 예 |
| 2ms | 13.674 | 14.032 / 13.572 / 13.674 | 예 |
| 3ms | 14.815 | 14.854 / 14.651 / 14.815 | 예 |
| 4ms | 16.469 | 16.252 / 16.469 / 16.488 | 예 |
| 6ms | 19.435 | 19.591 / 19.167 / 19.435 | 아니다 |

| 본문 값 | 정밀한 값 | 출처 |
| --- | --- | --- |
| 메시 하나 약 0.007ms | 0.0068 / 0.0068 / 0.0069 | 예산 4ms, 1만, 프레임 CSV(`Saved/Profiling/CSV/Budget1-B4-q4final-N10000-P1-r<N>.csv`, git 밖)의 `AnimationBudget/AverageWorkUnitTimeMs` 실행별 중앙값. 예산 1ms에서는 0.0099, 2ms에서는 0.0077\~0.0079로 예산마다 다르다 |
| 화면 안 액터 364개 | 363.82 | 예산 4ms, 1만, `view_actors_mean`(최근 1초 안에 그려진 액터 수) |
| 실제로 쓰는 시간 약 3.2ms | 3.262(6ms), 3.225(4ms) | 1만, `budgeted_anim_ms`. 6ms 실행 3.205 / 3.296 / 3.262 |
| 1만 소형 81%, 46.9%, 6.46%, 0%(1, 2, 3, 4·6ms) | 0.8099, 0.4695, 0.0646, 0 | 위 1만 표 |
| 1ms는 두 종류 모두 절반 넘게 | 소형 0.8099, 대형 0.5868 | 위 1만 표 |
| 3ms는 대형은 건너뛰지 않는다 | 0(범위 0\~0.0001) | 끔의 범위 0과 차이가 범위 이하라 구별되지 않는다 |
| 4ms의 프레임 시간이 2ms보다 1.66ms 길다 | 9.478 − 7.822 = 1.656 | 위 1만 표 |
| 2만 경계 19ms, 16.5ms, 19.4ms(끔, 4, 6ms), 0.23ms 남는다 | 18.997, 16.469, 19.435. 16.7 − 16.469 = 0.231 | 위 2만 표. 4ms 세 회차 최댓값은 16.488 |
| 6ms는 1만에서 4ms와 같다 | 소형·대형 모두 0, 프레임 9.458 대 9.478 | 차이 0.020이 범위 0.072보다 작다 |
| (그림) 1만 대형 58.7%, 2.10%, 0% | 0.5868, 0.021, 0 | 위 1만 표 |
| (그림) 2만 13.4, 13.7, 14.8ms(1, 2, 3ms) | 13.418, 13.674, 14.815 | 위 2만 표 |
| 최종 구성 2만 소형 42.1%, 대형 0.36% | 0.4212, 0.0036 | 최종 측정 `L8-q4final` 20000 P1의 `small_skipped_ratio`, `large_skipped_ratio`(이 절의 묶음이 아니다) |

- 4ms의 1만 프레임 p50(9.478)과 최종 측정의 최종 구성(9.746)은 묶음이 달라 이어 읽지 않는다.
- 한계: 훑기는 최종 구성(액터 40m, 상한 1,000), 호드 총수 둘, 오브 시점 하나에서 했다. 4\~7단계(액터 30m, 상한 500)에서는 재지 않았다.
- 4ms를 고른 결정은 대형과 소형을 모두 보았다. 대형만 보면 3ms도 두 기준을 만족한다([Worklog/Q7.md](../../Docs/Worklog/Q7.md) "선택 기준 적용").
- 실무에서 더 고려할 수 있는 항목(상태, 화면 중앙과의 각도, 가려짐, 플레이어와의 관계, 종류별 가중치, 떨림 방지 이력)은 엔진 소스의 근거가 없는 일반적인 실무 관행으로 적었다. 엔진 근거가 있는 것은 "엔진은 최근 1초 안에 그려졌는지만 본다" 하나다(`AnimationBudgetAllocator.cpp:174,254-263`). 사망·피격 중인 액터의 건너뜀은 재지 않았다.

## 전환 대책의 경위

본문 "바꾼 것"의 전환 이야기(풀이 액터를 바로 보이게 함, 88cm 가라앉음과 세로 줄무늬, 숨김이 만든 번짐, 하나만 켜서는 줄지 않음, 가장 큰 것은 높이 보정)는 수치 없이 방향만 썼다. 근거는 최종 측정과 조건이 다른 옛 기록이다.

- 대책별 화면 점수: 개발 빌드, 호드 총수 5천, 전환 거리 30m, 고정 1/60초 간격 연속 캡처, 조합마다 1회. [Measurements/M3/shots/transition/README.md](../../Measurements/M3/shots/transition/README.md) "1. 하나씩 빼 보기"(높이 보정이 가장 크다), "2. 하나씩만 켜 보기"(어느 것도 혼자서는 거의 줄이지 못한다).
- 원인을 찾은 순서와 엔진 근거: [Docs/post-material.md](../../Docs/post-material.md) "전환 팝핑은 어디서 생기고 어떻게 줄였나", [Worklog/M3.md](../../Docs/Worklog/M3.md).
- 최종 구성에서 대책을 끈 개발 빌드 점수는 옛 총괄 묶음 때의 참고값이다(`Measurements/Full1/transition-dev/runs.csv`). 본문에는 쓰지 않았다.
- 글 재료의 사실 확인: `Docs/post-review.md` "04 액터 하이드레이션"(글을 다 쓰면 지우는 문서라, 지운 뒤에는 git 이력에서 본다). 거기서 지적한 "새로 스폰한 액터는 `:328`이 아니라 기존 액터 재사용 경로" 문제는 본문에서 풀 경로(`:252`)만 인용해 피했다.

## 트레이스 값

본문 "결과"의 트레이스 문단 값이다. 값은 명령줄 내보내기로 뽑았다. 같은 트레이스의 Insights 화면은 아래 "Insights 화면"에 있다.

| 항목 | 값 |
| --- | --- |
| 트레이스 | `Saved/Traces/Full2traceser-L4-mass-N10000-P1-r1.utrace`(4단계), `Full2traceser-L3-vatcull-N10000-P1-r1.utrace`(3단계). 둘 다 직렬(`mass.FullyParallel` 끔), 저장소 밖 |
| 내보낸 표 | `%TEMP%\horde-trace\F2-traceser-L4-mass-GT.csv`, `F2-traceser-L3-vatcull-GT.csv`(`Scripts/trace-export.ps1 -Command TimerStatistics`, 스레드 `GameThread`, 구간 25\~75초, 저장소 밖) |
| 프레임 수 | 4단계 5,674, 3단계 6,412(`FEngineLoop::Tick` 횟수) |
| 식 | 타이머의 합(초) × 1000 ÷ 프레임 수 = 프레임당 ms. 구간 안 평균이다 |
| 주의 | 직렬 실행이라 게임 스레드 값이 병렬 측정과 다르다. 본문은 "견주지 않는다"고 적었다 |

| 본문 값 | 정밀한 값 | 타이머 | 합(초) / 횟수 |
| --- | --- | --- | --- |
| 스켈레탈 메시 틱 1.05ms, 257번 | 1.0543ms, 프레임당 257.1번 | `USkinnedMeshComponent_TickComponent` Incl(4단계) | 5.981948 / 1,458,570 |
| 애니메이션 평가 완료 0.828ms | 0.8279 | `USkeletalMeshComponent_CompleteParallelAnimationEvaluation` Incl | 4.697633 / 1,451,822 |
| 액터 동기화 0.510ms | 0.5098 | `LabActorSyncProcessor_2147482256` Incl(Excl 2.854808) | 2.892495 / 5,672 |
| `Sync_RenderingThread` 3단계 2.75ms | 2.7488 | 3단계 Incl | 17.625474 / 6,412 |
| `Sync_RenderingThread` 4단계 0.005ms | 0.0045 | 4단계 Incl | 0.025783 / 5,673 |
| `Tick_Engine` 4.68 → 8.37ms | 4.6766 → 8.3672 | 3단계 29.986314초 → 4단계 47.475477초 | |

참고(본문에 쓰지 않았다): `USkeletalMeshComponent_TickAnimation` 0.4894ms(1,205,457번), 프레임 평균 `FEngineLoop::Tick` 3단계 7.7976 → 4단계 8.8117ms. 3단계 표에는 위 세 타이머(스켈레탈 메시 틱, 평가 완료, 액터 동기화)가 없다.

스켈레탈 메시 틱의 호출 수는 프레임당 257번으로, 액터 프레임 평균 373(`actors_mean`)보다 작다. 이 차이의 원인은 보지 않았다(트레이스 한 회, `actors_mean`은 측정 실행의 값). 본문은 "액터 수와 같다"고 쓰지 않았다.

## 화면의 HUD와 표의 차이

요약의 두 화면(`step3-L3-vatcull.webp`, `step4-L4-mass.webp`)은 측정 실행이 아니라 촬영 실행(`Full3shot-L3-vatcull-N10000-P1-r1`, `Full3shot-L4-mass-N10000-P1-r1`, [촬영 색인](../../Measurements/Full3/captures.csv))에서 찍었다. HUD의 프레임 시간은 화면 녹화 부하가 섞여 표의 값보다 높다. 수치는 표의 값만 쓴다. 촬영 경위는 [Worklog/Q7.md](../../Docs/Worklog/Q7.md) "측정: Full3"에 있다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/hyd-summary.svg` | 요약 그림. 3단계와 4단계의 프레임·게임 스레드·렌더 스레드·GPU 막대(1만, 같은 축척), 16.7ms 선, 가장 느린 쪽 표시, 아래 문장에 세 변화 | "1만에서 3단계와 4단계" |
| `images/hyd-principle.svg` | 원리 도식. 3·4단계의 거리 구간(0\~500m)과 액터 하나가 한 프레임에 하는 일 세 칸 | "4단계의 구현", "설정값과 계산값" |
| `images/hyd-transition.svg` | 전환 도식. 네 프레임 동안 VAT 인스턴스와 액터 중 무엇을 그리나, 전환 대책 다섯 가지 | "4단계의 구현"의 대책 비트, "전환 대책의 경위" |
| `images/hyd-expected.svg` | 결과 그림. 여유 3.37, 예상 6.34, 실제 4.98(ms)과 아래 문장(198개, GPU −0.603, 프레임 +1.51) | "여유, 예상, 실제" |
| `images/hyd-maxn.svg` | 결과 그림. 프레임 시간 느린 쪽 5% 경계 대 호드 총수(3단계 점선, 4단계 실선), 16.7ms 선, 4단계의 최대 개체 수 19,952, 아래에 액터 수 | "호드 총수별 값" |
| `images/hyd-ablation.svg` | 빼 보기 그림. 최종 구성 대비 프레임·게임 스레드 차이 네 행, 아래 문장에 예산 4ms | "최종 구성에서 하나씩 바꾼 측정" |
| `images/hyd-budget.svg` | 예산 값 그림. 예산 끔·1·2·3·4·6ms마다 1만의 소형·대형 건너뜀 비율과 2만의 프레임 p95, 16.7ms 선, 고른 값 4ms 표시 | "예산 값 비교" |
| `../../Docs/figures/steps/step3-L3-vatcull.webp`, `step4-L4-mass.webp` | 3·4단계 화면(HUD 켬) | 위 "화면의 HUD와 표의 차이" |

SVG 일곱 개는 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/04-hydration-anim-budget/make-figures.py`). 측정값은 최종 측정의 `summary.csv`와 `full-n-at-60fps.csv`, 예산 값 그림만 `Measurements/Budget1/summary.csv`에서 읽는다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다.

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다. 경로는 `Engine/` 아래다. 줄임말: **ABA** = `Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator`, **MG** = `Plugins/Runtime/MassGameplay/Source`.

| 사실 | 위치 |
| --- | --- |
| 할당기는 예산을 메시 하나의 평균 갱신 시간으로 나눠 갱신할 수를 구하고, 넘는 몫이 있을 때만 줄인다 | `ABA/Private/AnimationBudgetAllocator.cpp:558,561,566`(`FAnimationBudgetAllocator::CalculateWorkDistributionAndQueue`, `:425`) |
| 중요도가 큰 순으로 정렬한다 | 같은 파일 `:327` 주석 "Sort by significance, largest first", `:339`(`QueueSortedComponentIndices`, `:172`) |
| 예산 기본 1ms, 켜기 CVar 기본 0 | `ABA/Public/AnimationBudgetAllocatorParameters.h:19`, `ABA/Private/AnimationBudgetAllocatorCVars.cpp:7,195` |
| 중요도 설정 API | `ABA/Public/SkeletalMeshComponentBudgeted.h:36` `SetComponentSignificance` |
| 엔진 풀은 액터를 꺼내자마자 보이게 하고 옮긴 뒤 `PrepareForGame`을 부른다 | `MG/MassActors/Private/MassActorSpawnerSubsystem.cpp:252-255`. 새로 스폰한 액터도 콜리전을 켜고 보이게 한다(`:327-328`) |
| 엔진은 액터를 붙일 때와 준비가 끝날 때 엔티티 변환으로 옮긴다 | `MG/MassRepresentation/Private/MassRepresentationProcessor.cpp:272,428`(`UMassRepresentationProcessor::UpdateRepresentation`, `:72`) |
| `TeleportActor` 기본 구현은 지연 명령으로 `SetActorTransform`을 부른다 | `MG/MassRepresentation/Private/MassRepresentationActorManagement.cpp:47-56`(`:53`) |
| 준비됨 기본값은 참이다. `SetActorEnabled`는 틱과 콜리전만 바꾼다 | 같은 파일 `:58-61`, `:30-45` |
| 준비 대기는 인스턴스가 있을 때와 준비 안 됨일 때 표현 전환을 미룬다 | `MassRepresentationProcessor.cpp:275-298` |
| `bKeepActorExtraFrame`·`bWaitForActorVisualReadiness` 기본값 거짓 | `MG/MassRepresentation/Public/MassRepresentationFragments.h:202,211` |
| `ACharacter` 캡슐 반높이 기본 88 | `Source/Runtime/Engine/Private/Character.cpp:78` |

풀이 액터를 바로 보이게 하는 동작과 `TeleportActor`가 엔티티 변환(발밑)으로 옮기는 동작을 다룬 공개 이슈는 찾지 못했다. [references.md](../../Docs/Reference/references.md)와 [engine-notes.md](../../Docs/Reference/engine-notes.md)에도 없다. 두 동작은 엔진의 버그라기보다 이 프로젝트의 액터 원점(캡슐 중심)과 엔티티 원점(발밑)이 다르고 모션 블러를 켜 둔 조건에서 드러난 기본 동작이다. 해결은 `ULabActorManagement::TeleportActor`(`Source/HordeOptLab/LabActorSyncProcessor.cpp:46-63`)와 `ALabMinionMassActor::HideUntilPoseReady`(`Source/HordeOptLab/LabMinion.cpp:319-333`)다.

### 본문에 넣은 발췌

엔진 코드는 14줄이다(`// ...` 포함). 남긴 줄은 소스를 그대로 옮겼다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| 할당기(엔진) | 7 | `AnimationBudgetAllocator.cpp:558`, `// ...`, `:561`, `// ...`, `:566`, `// ...`, `:339` |
| 풀에서 꺼내기(엔진) | 4 | `MassActorSpawnerSubsystem.cpp:252-253`, `// ...`(`:254` 빈 줄), `:255` |
| 엔티티 변환으로 옮기기(엔진) | 3 | `MassRepresentationProcessor.cpp:272`, `// ...`, `MassRepresentationActorManagement.cpp:53` |
| `BuildEntityConfig` | 8 | `LabCrowdSubsystem.cpp:109`, `:112`, `:115-117`, `// ...`(`:118-124`), `:125`, `:126`(줄 끝 주석을 뺐다) |
| 캡슐 콜리전 | 7 | `LabMinion.cpp:305-309`, `:311-312`(`:310` 주석을 뺐다) |
| 화면 크기 중요도 | 4 | `LabActorSyncProcessor.cpp:352-355` |
| 할당기 켜기 | 8 | `LabGameMode.cpp:372`, `:374-375`, `// ...`(`:373`, `:376-377` 주석), `:378-381` |
| `TeleportActor` | 10 | `LabActorSyncProcessor.cpp:46-47`, `:51`(`:48-50` 주석을 표시 없이 뺐다), `// ...`(`:52-57` 대형 배율), `:58-59`, `:60`(줄 끝 주석을 뺐다), `:61-63` |
| `HideUntilPoseReady` | 11 | `LabMinion.cpp:319-322`, `// ...`(`:323-327` 패스 숨김을 끈 기록용 분기), `:328-333` |

뺀 주석 가운데 작업 이름을 담은 것: `LabCrowdSubsystem.cpp:126` "확정값 LODMaxCount(High는 Q4에서 인자로)", `LabActorSyncProcessor.cpp:350` "E3 (3c)", `LabMinion.cpp:303-304` "plan.md M3b", "Q5.md 1b단위".

## 확인하지 않은 것

- 4단계에서 GPU가 0.603ms 준 원인. 가까운 VAT 인스턴스가 액터로 바뀐 몫으로 짐작하지만, 패스별 값을 새 트레이스로 가르지 않았다.
- 액터 하나 13.4µs의 내역. 직렬 트레이스에서 메시 틱 1.05, 평가 완료 0.828, 동기화 0.510ms(프레임당)를 읽었지만 병렬 측정과 직접 맞춰 합치지 않았다. 호출 수 257이 `actors_mean` 373과 다른 이유도 보지 않았다.
- `S8-col`에서 프레임 시간(+1.60)이 게임 스레드(+1.10)보다 더 는 이유.
- `A8-budget`의 −0.292ms가 무엇에서 나는지. 예산을 빼면 액터 클래스가 `ALabMinionMassActor`(일반 스켈레탈 메시)로 바뀐다(`LabCrowdSubsystem.cpp:101,109`). 4단계에서 예산만 끈 측정은 최종 측정에 없다.
- 트레이스 값(아래 "트레이스 값")은 앞선 측정 빌드의 트레이스다. 최종 측정의 표와 빌드가 다르다.
- 전환 대책의 화면 효과를 최종 측정 빌드에서 다시 잰 값. 본문은 옛 개발 빌드 기록의 방향만 썼다.
- 4단계 화면에서 액터의 AnimBP 동작과 전환이 실제로 보이는지. 움직이는 WebP를 열어 보지 않았다(브라우저 금지). 이미지 설명은 "액터다"까지만 적었다.
- SVG는 `hyd-budget.svg`와 `hyd-maxn.svg`만 브라우저로 그려 보았다. 나머지는 좌표와 글자 수로만 확인했다.

## Insights 화면

본문의 Insights 스크린샷이다. 2026-11-13에 찍었다. 값의 계산은 위 트레이스 표가 근거이고, 화면은 같은 트레이스에서 같은 타이머의 구간 합계를 보인다.

| 항목 | 내용 |
| --- | --- |
| 파일 | `images/insights-mesh-tick.jpg` |
| 트레이스 | `Full2traceser-L4-mass-N10000-P1-r1`(직렬) |
| 거른 말 | Timers 목록 `MeshComponent`, CPU 타이머만 |
| 표시한 행(Count, Incl) | `USkinnedMeshComponent_TickComponent` 1,458,813 / 5.98s, `USkeletalMeshComponent_CompleteParallelAnimationEvaluation` 1,451,827 / 4.7s. 프레임 수 5,672는 같은 구간을 `Mass`로 걸렀을 때 Processor 행의 Count다 |
| 화면에 없는 값 | `ULabActorSyncProcessor`, `Sync_RenderingThread`, `Tick_Engine`과 3단계 트레이스의 값은 찍지 않았다 |
| 구간 | Timing 뷰의 시간 눈금자를 끌어 25초부터 75초까지(길이 표시 50s)를 골랐다. 손으로 고른 구간이라 내보내기의 창과 프레임이 한두 개 다를 수 있다. 화면의 합계는 표시 자릿수까지 내보낸 값과 같다 |
| 가공 | 화면 캡처 둘(왼쪽 절반, 오른쪽 절반)을 이어 붙였다. 트레이스 파일 이름이 보이는 제목 줄은 잘라 냈다. 빨간 상자와 단계 이름을 그렸다. 두 트레이스를 견주는 그림은 위아래로 붙였다 |
