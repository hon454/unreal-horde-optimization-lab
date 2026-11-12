# 04 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-12 20:34\~11-13 00:39). 경위는 [작업 기록의 재측정 절](../../Docs/Worklog/Q6.md#full2-재측정2026-11-12) |
| 빌드 | Test 패키지 08e45f2, CSV 메타데이터 `labbuild=08e45f24a5e5` |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L3-vatcull`(`-LabStage=M2 -LabVariant=vatcull`), `L4-mass`(`-LabStage=M3 -LabVariant=mass`), `L8-q4final`(`-LabStage=M5 -LabVariant=q4final`) |
| 빼 보기·하위 실험 | `A8-hydrate`(`-LabHydrate=0`), `A8-budget`(`-LabScreenSigBudget=0`), `A8-fix`(`-LabHydrateFixMask=0`), `S8-col`(`-LabActorCollision=1`). 모두 `L8-q4final`에 인자 하나를 더한다 |
| 기대 메타데이터 | `Scripts/full-conditions.json`. `L4-mass`는 `LabTechHydrate=1`, `LabTechAnimBudget=1`, `LabTechHydrateFix=1`, `LabActorCap=500`, `LabHydrateM=30`, `LabActorCollision=0`, `LabBudget=1` |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full2/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full2/runs.csv)(회차별 한 행), [빼 보기 표](../../Docs/figures/full2/full-ablation.csv) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full2/full-n-at-60fps.csv)(`Scripts/plot.py`가 측정 N 사이를 로그 보간) |
| 트레이스 | 본문 "결과"의 트레이스 문단 값은 아래 "트레이스 값"이다. 최종 측정 빌드의 직렬 트레이스 한 회씩이다 |

## 4단계의 구현(3단계와 다른 곳)

- 표현 LOD: `ULabCrowdSubsystem::BuildEntityConfig`(`Source/HordeOptLab/LabCrowdSubsystem.cpp:98-118`). `Stage >= M3`(`:98`)라 High의 표현이 `HighResSpawnedActor`다(`:112`, `UsesHydration()`이 참). 액터 클래스는 `bBudget`(`:101`, `bScreenSigBudget` 참)이라 `ALabMinionMassActorBudget`·`ALabMinionMassActorBudgetLarge`다(`:105`, `:109`). 액터 관리 클래스 `ULabActorManagement`, 준비 대기와 한 프레임 더 두기(`:115-117`).
- 구간과 상한: `Bands` = {0, 3000, 8000, 20000}cm, `MaxCounts` = {500, 5000, 무제한, 무제한}(`:125-131`). `HydrateCm()` = `HydrateM`(기본 30, `LabConfig.h:79`) × 100, `ActorCap` 기본 500(`LabConfig.h:78`). 화면 밖 Medium 경계는 1500cm(`:144`, 변형 `mass`는 `bBaseIsVisible`이 거짓). 3단계는 `bM3`가 거짓이라 화면 밖 경계도 30m이지만 High·Medium 모두 VAT 인스턴스라 그리는 것은 같다.
- 상한이 두 종류의 합계라는 것: [engine-notes.md](../../Docs/Reference/engine-notes.md) 19절(`MassVisualizationTrait.cpp:145`, `MassVisualizationLODProcessor.cpp:70-110`). 이 글은 줄을 다시 열지 않았다.
- 액터: `ALabMinionMassActor`(`LabMinion.cpp:275-281`) 액터 틱 끔(`bCanEverTick = false`, `:279`). 부모 `ALabMinionPawn`(`LabMinion.h:115`, `:141`)의 생성자가 CharacterMovement의 틱을 끈다(`LabMinion.cpp:199-205`, `:212`, 컴포넌트는 `ACharacter`가 필수로 만들어 남는다). `ALabMinionMassActorBudget`(`:380-383`)이 메시를 `USkeletalMeshComponentBudgeted`로 바꾼다. 캡슐 콜리전(`:305-312`), 기본 `EActorCollision::Off`(`LabConfig.h:319`).
- 동기화: `ULabActorSyncProcessor::Execute`(`LabActorSyncProcessor.cpp:119-413`). 위치(`:253`), 시작 시각(`:263-267`), 준비 대기 두 프레임(`:270-277`, `:287-295`), 모션 벡터 지우기(`:331-336`), 화면 크기 중요도(`:346-354`), 대형 건너뜀 스탯(`:355-361`), 사망 색(`:364-369`). 게임 모듈이 M3 이상에서 등록한다(`HordeOptLab.cpp`, code-tour.md "실행 인자에서 설정까지").
- 예산: `ALabGameMode::ApplyVariantCVars`(`LabGameMode.cpp:355-371`). `FLabScenario::BudgetMs = 2.f`(`LabConfig.h:162`).
- 전환 대책 비트(`LabConfig.h:360-364`): `FixStartTime` 1, `FixWait` 2, `FixPassHide` 4, `FixLift` 8, `FixTintReset` 16. 기본 마스크 31(`LabConfig.h:223`). 진홍 지우기는 `ALabMinionMassActor::PrepareForPooling_Implementation`(`LabMinion.cpp:349-359`), 풀에서 꺼낼 때 다시 숨기기는 `PrepareForGame_Implementation`(`:361-376`), 준비됨 판정은 `ULabActorManagement::IsActorReadyForRepresentation`(`LabActorSyncProcessor.cpp:40-44`).
- 같은 것: VAT 프로세서(`ULabVatISMProcessor`, M2\~M4), 그림자 켬, 200m 밖 지우기, 로직 프로세서와 시나리오.

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. "(그림)"은 그림에만 쓴 값이다. 세 회차 열은 r1 / r2 / r3이다. 아래 값은 모두 P1이다.

### 1만에서 3단계와 4단계(요약 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 3단계 프레임 시간 7.91ms | 7.914 | `L3-vatcull` 10000 | `frame_p50_ms` | 7.833 / 7.956 / 7.914 |
| 3단계 게임 스레드 3.21ms | 3.206 | 같음 | `game_p50_ms` | 4.14 / 3.206 / 3.136 |
| (그림) 3단계 렌더 스레드 4.12ms | 4.121 | 같음 | `render_p50_ms` | 4.136 / 4.121 / 4.089 |
| 3단계 GPU 7.54ms | 7.535 | 같음 | `gpu_p50_ms` | 7.45 / 7.578 / 7.535 |
| 4단계 프레임 시간 8.16ms | 8.163 | `L4-mass` 10000 | `frame_p50_ms` | 8.163 / 8.018 / 8.22 |
| 4단계 게임 스레드 7.87ms | 7.869 | 같음 | `game_p50_ms` | 7.869 / 7.713 / 7.923 |
| (그림) 4단계 렌더 스레드 4.69ms | 4.694 | 같음 | `render_p50_ms` | 4.618 / 4.797 / 4.694 |
| (그림) 4단계 GPU 6.94ms | 6.938 | 같음 | `gpu_p50_ms` | 6.926 / 6.942 / 6.938 |
| 병목 GPU → 게임 스레드 | `gpu` → `game` | 위 두 조건 | `bottleneck` | |
| 프레임 시간 +0.249ms | 0.249 | 계산 | 8.163 − 7.914. 범위 0.123 / 0.202보다 크다 | |
| 게임 스레드 +4.66ms, "두 배 넘게" | 4.663, 2.45배 | 계산 | 7.869 − 3.206. 7.869 ÷ 3.206. 범위 1.004 / 0.210보다 크다 | |
| GPU −0.597ms | −0.597 | 계산 | 6.938 − 7.535. 범위 0.128 / 0.016보다 크다 | |
| 렌더 스레드 +0.573ms | 0.573 | 계산 | 4.694 − 4.121. 범위 0.047 / 0.179보다 크다 | |
| 드로우 콜 123 → 1,321 | 123 / 1321 | 위 두 조건 | `draw_calls_p50` | 123 / 123 / 123, 1321 / 1322 / 1321 |
| 액터 프레임 평균 373 | 373.07 | `L4-mass` 10000 | `actors_mean`(`Lab/Actors` 평균) | 373.04 / 373.07 / 373.08 |

`L3-vatcull` 10000 P1 r1의 게임 스레드 4.14ms는 다른 두 회차보다 길다(03번 글의 측정 기록과 같은 행). 중앙값 3.206을 쓴다.

### 여유, 예상, 실제(원리의 예상, 결과 그림)

| 본문 값 | 정밀한 값 | 조건 | 출처 | 세 회차 |
| --- | --- | --- | --- | --- |
| 게임 스레드의 여유 4.33ms | 4.329 | 계산 | 3단계 `gpu_p50_ms` − `game_p50_ms` = 7.535 − 3.206 | |
| 1단계 액터 단가 17.0µs | 17.038 | `L1-full` 1000 | `game_p50_ms` 17.038 ÷ 1000(01번 글의 측정 기록과 같다) | 17.038 / 16.943 / 17.06 |
| 여유가 차는 액터 254개 | 254.08 | 계산 | 4.329 ÷ 0.017038 | |
| 1단계 단가로 본 예상 6.36ms | 6.356 | 계산 | 373.07 × 0.017038 | |
| 액터 하나에 12.5µs | 12.499 | 계산 | 4.663 ÷ 373.07 × 1000 | |
| 여유를 0.33ms 넘었다(그림 문장 "여유를 넘었다") | 0.334 | 계산 | 4.663 − 4.329 | |
| 오브 30m 안 평균 627개 | 627.23 | `L3-vatcull` 10000 | `near30_mean`(오브 수평 30m 안 개체 수 평균) | 중앙값만(`runs.csv`의 `near30_mean`) |
| 카메라는 오브 뒤 15m | 15m | 확정값 | [measurement.md](../../Docs/Guides/measurement.md) "확정값" P1 행 | |

"1단계 액터의 단가"는 1단계(`ACharacter`, CharacterMovement, AnimBP)의 개체당 게임 스레드 시간이고, 4단계 액터에는 없는 이동 컴포넌트와 액터 틱이 들어 있다. 그래서 예상은 상한으로만 읽는다.

### 호드 총수별 값(최대 개체 수 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 3단계 5천 6.92 | 6.92 | `L3-vatcull` 5000 | `frame_p95_ms` | 6.951 / 6.904 / 6.92 |
| (그림) 3단계 1만 8.47 | 8.471 | 10000 | 같음 | 8.416 / 8.518 / 8.471 |
| (그림) 3단계 2만 11.7 | 11.681 | 20000 | 같음 | 11.745 / 11.637 / 11.681 |
| (그림) 3단계 3만 15.3 | 15.257 | 30000 | 같음 | 15.687 / 15.257 / 15.168 |
| (그림) 4단계 5천 6.79 | 6.791 | `L4-mass` 5000 | 같음 | 6.795 / 6.791 / 6.752 |
| (그림) 4단계 1만 9.60 | 9.603 | 10000 | 같음 | 9.554 / 9.603 / 9.609 |
| (그림) 4단계 2만 13.9 | 13.943 | 20000 | 같음 | 14.051 / 13.943 / 13.888 |
| (그림) 4단계 3만 18.4 | 18.446 | 30000 | 같음 | 18.719 / 18.372 / 18.446 |
| 4단계 최대 개체 수 25,636 | 25636 | `4 L4-mass` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준은 30000 `above range`(`n_p50`) |
| 3단계는 3만 안에서 한계에 닿지 않음 | 30000, `above range` | `3 L3-vatcull` | 같은 파일 `n_p95`, `note_p95` | |
| 액터 수 181 / 373 / 493 / 486 | 181.13 / 373.07 / 493.0 / 486.44 | `L4-mass` 5000\~30000 | `actors_mean` | 2만 493.0 / 492.92 / 493.03, 3만 486.4 / 486.44 / 486.44 |
| 5천에서 4단계 프레임 시간이 더 짧다 | 6.171 대 6.418(−0.247) | `L4-mass`·`L3-vatcull` 5000 | `frame_p50_ms`, 둘 다 `bottleneck` = `gpu` | 범위 0.017 / 0.036보다 크다 |
| 대형 액터 건너뜀 1.29%(1만) | 0.0129 | `L4-mass` 10000 | `large_skip_ratio` | 0.0132 / 0.0079 / 0.0129 |
| 대형 액터 건너뜀 49.5%(2만) | 0.4949 | `L4-mass` 20000 | 같음 | 0.4018 / 0.5405 / 0.4949 |

`large_skip_ratio`는 대형 액터만 센다(`LabActorSyncProcessor.cpp:355-361`, 분자 `Lab/LargeActorSkips`, 분모 `Lab/LargeActors`). 할당기가 외부 틱 제어로 둔 컴포넌트가 이번 프레임 애니메이션 틱을 건너뛰었는지(`ShouldTickAnimation()`)로 센다. 소형 액터의 건너뜀은 세지 않았다. 3만의 값(0.3008, 회차 0.0883 / 0.3008 / 0.4266)은 회차마다 크게 갈려 본문에 쓰지 않았다.

### 최종 구성에서 하나씩 바꾼 측정(빼 보기 그림, 본문)

기준은 `L8-q4final` 10000 P1(r1\~r3)이다. 묶음 끝 A/A(r11\~r13)는 쓰지 않았다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 최종 구성 프레임 7.78ms | 7.783 | `L8-q4final` 10000 | `frame_p50_ms` | 7.802 / 7.783 / 7.742 |
| (그림) 최종 구성 게임 스레드 7.65ms | 7.648 | 같음 | `game_p50_ms` | 7.667 / 7.648 / 7.609 |
| 하이드레이션을 빼면 프레임 −1.59ms | 6.191 − 7.783 = −1.592 | `A8-hydrate` | `frame_p50_ms` | 6.192 / 6.191 / 6.161 |
| 게임 스레드 −3.85ms | 3.795 − 7.648 = −3.853 | 같음 | `game_p50_ms` | 3.795 / 3.845 / 3.773 |
| 병목이 GPU로 바뀐다 | `gpu`(GPU 5.757) | 같음 | `bottleneck`, `gpu_p50_ms` | GPU 5.757 / 5.769 / 5.731 |
| 액터 평균 458개, 하나에 8.41µs | 458.13, 8.410 | `L8-q4final` 10000 | `actors_mean`. 3.853 ÷ 458.13 × 1000 | 458.05 / 458.13 / 458.15 |
| 애니메이션 예산을 빼면 프레임 +1.75ms | 9.533 − 7.783 = 1.750 | `A8-budget` | `frame_p50_ms` | 9.542 / 9.533 / 9.529 |
| (그림) 게임 스레드 +1.75ms | 9.394 − 7.648 = 1.746 | 같음 | `game_p50_ms` | 9.397 / 9.394 / 9.392 |
| 캡슐 콜리전을 켜면 프레임 +2.01ms | 9.790 − 7.783 = 2.007 | `S8-col` | `frame_p50_ms` | 9.787 / 9.79 / 9.837 |
| 게임 스레드 +1.54ms | 9.188 − 7.648 = 1.540 | 같음 | `game_p50_ms` | 9.178 / 9.188 / 9.239 |
| 전환 대책을 빼면 −0.017ms, 범위 안 | 7.766 − 7.783 = −0.017 | `A8-fix` | `frame_p50_ms` | 7.897 / 7.765 / 7.766. 범위 0.132 / 0.060보다 작다 |
| (그림) 전환 대책 게임 스레드 −0.013ms | 7.635 − 7.648 = −0.013 | 같음 | `game_p50_ms` | 7.764 / 7.635 / 7.628 |
| 최종 구성 40m, 상한 1,000 | 40, 1000 | `L8-q4final` | `hydrate_m`, `actor_cap`(`FLabQuality::Q4FinalHydrateM`·`Q4FinalActorCap`, `LabConfig.h:82,84`) | |

차이 판정([measurement.md](../../Docs/Guides/measurement.md) "차이 판정"): 프레임 범위는 `L8` 0.060, `A8-hydrate` 0.031, `A8-budget` 0.013, `S8-col` 0.050, `A8-fix` 0.132다. 앞의 세 차이는 두 범위 중 큰 쪽보다 크고, `A8-fix`는 작다. 빼 보기 행의 다른 값(참고): `A8-hydrate` 렌더 스레드 −1.185, GPU +0.037, 드로우 콜 1716 → 175. `A8-budget` 대형 건너뜀 0(예산이 없으므로), `L8`은 0.2073, `S8-col`은 0.1011.

### 설정값과 계산값

| 본문 값 | 값 | 근거 |
| --- | --- | --- |
| 화면 안 30m, 화면 밖 15m, 상한 500 | 3000 / 1500cm, 500 | 위 "4단계의 구현" |
| 예산 2ms | 2.0 | `LabConfig.h:162` `BudgetMs`, `LabGameMode.cpp:364` |
| `a.Budget.Enabled` 기본 0 | 0 | `AnimationBudgetAllocatorCVars.cpp:7` |
| 예산 기본 1ms | 1.0 | `AnimationBudgetAllocatorParameters.h:19` `BudgetInMs = 1.0f`, CVar `a.Budget.BudgetMs`(`AnimationBudgetAllocatorCVars.cpp:195`) |
| 할당기는 꺼진 채 시작 | | `LabGameMode.cpp:365` 주석, [engine-notes.md](../../Docs/Reference/engine-notes.md) 18절(`AnimationBudgetAllocator.cpp:75,115,135`) |
| 대형이 소형보다 2.35배 중요 | 2.35 | 계산: 바운드 반경 94 ÷ 40([measurement.md](../../Docs/Guides/measurement.md) "확정값" 2종 행) |
| 소형 액터 88cm 가라앉음 | 88 | `ACharacter` 캡슐 반높이 기본값(`Engine/Source/Runtime/Engine/Private/Character.cpp:78` `InitCapsuleSize(34.0f, 88.0f)`). 이 프로젝트는 캡슐 크기를 바꾸지 않는다(`LabMinion.cpp:25-33`). 대형은 배율 2.0이라 그 두 배다 |
| 메시가 두 번 틱하면 준비됨 | 2 | `LabActorSyncProcessor.cpp:274` `PendingReadyFrames = 2`, `:287` |
| VAT는 정점마다 뼈 넷까지 | 4 | AnimToTexture 본 영향 기본값([engine-notes.md](../../Docs/Reference/engine-notes.md) 3절, `AnimToTextureDataAsset.h`). `Scripts/bake-vat.py`는 바꾸지 않는다 |
| VAT는 클립 사이를 섞지 않는다 | | `ULabVatISMProcessor::Execute`가 상태로 클립 하나를 고르고 그 클립의 프레임만 넣는다(`LabVatProcessor.cpp:151-157`, 03번 글의 측정 기록) |

## 전환 대책의 경위

본문 "바꾼 것"의 전환 이야기(풀이 액터를 바로 보이게 함, 88cm 가라앉음과 세로 줄무늬, 숨김이 만든 번짐, 하나만 켜서는 줄지 않음, 가장 큰 것은 높이 보정)는 수치 없이 방향만 썼다. 근거는 최종 측정과 조건이 다른 옛 기록이다.

- 대책별 화면 점수: 개발 빌드, 호드 총수 5천, 전환 거리 30m, 고정 1/60초 간격 연속 캡처, 조합마다 1회. [Measurements/M3/shots/transition/README.md](../../Measurements/M3/shots/transition/README.md) "1. 하나씩 빼 보기"(높이 보정이 가장 크다), "2. 하나씩만 켜 보기"(어느 것도 혼자서는 거의 줄이지 못한다).
- 원인을 찾은 순서와 엔진 근거: [Docs/post-material.md](../../Docs/post-material.md) "전환 팝핑은 어디서 생기고 어떻게 줄였나", [Worklog/M3.md](../../Docs/Worklog/M3.md).
- 최종 구성에서 대책을 끈 개발 빌드 점수는 옛 총괄 묶음 때의 참고값이다(`Measurements/Full1/transition-dev/runs.csv`). 본문에는 쓰지 않았다.
- 글 재료의 사실 확인: `Docs/post-review.md` "04 액터 하이드레이션"(글을 다 쓰면 지우는 문서라, 지운 뒤에는 git 이력에서 본다). 거기서 지적한 "새로 스폰한 액터는 `:328`이 아니라 기존 액터 재사용 경로" 문제는 본문에서 풀 경로(`:252`)만 인용해 피했다.

## 트레이스 값

본문 "결과"의 트레이스 문단 값이다. Insights 화면은 찍지 않고 명령줄 내보내기로 뽑았다.

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

요약의 두 화면(`step3-L3-vatcull.webp`, `step4-L4-mass.webp`)은 측정 실행이 아니라 촬영 실행(`Pre5shot-L3-vatcull-N10000-P1-r1`, `Pre5shot-L4-mass-N10000-P1-r1`)에서 찍었다. HUD의 프레임 시간(3단계 8.9ms, 4단계 10.3ms)은 화면 녹화 부하가 섞여 표의 값(7.91, 8.16ms)보다 높다. 수치는 표의 값만 쓴다. 촬영 경위는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "촬영 준비"에 있다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/hyd-summary.svg` | 요약 그림. 3단계와 4단계의 프레임·게임 스레드·렌더 스레드·GPU 막대(1만, 같은 축척), 16.7ms 선, 가장 느린 쪽 표시, 아래 문장에 세 변화 | "1만에서 3단계와 4단계" |
| `images/hyd-principle.svg` | 원리 도식. 3·4단계의 거리 구간(0\~500m)과 액터 하나가 한 프레임에 하는 일 세 칸 | "4단계의 구현", "설정값과 계산값" |
| `images/hyd-transition.svg` | 전환 도식. 네 프레임 동안 VAT 인스턴스와 액터 중 무엇을 그리나, 전환 대책 다섯 가지 | "4단계의 구현"의 대책 비트, "전환 대책의 경위" |
| `images/hyd-expected.svg` | 결과 그림. 여유 4.33, 예상 6.36, 실제 4.66(ms)과 아래 문장(254개, GPU −0.597, 프레임 +0.249) | "여유, 예상, 실제" |
| `images/hyd-maxn.svg` | 결과 그림. 프레임 시간 느린 쪽 5% 경계 대 호드 총수(3단계 점선, 4단계 실선), 16.7ms 선, 4단계의 최대 개체 수 25,636, 아래에 액터 수 | "호드 총수별 값" |
| `images/hyd-ablation.svg` | 빼 보기 그림. 최종 구성 대비 프레임·게임 스레드 차이 네 행 | "최종 구성에서 하나씩 바꾼 측정" |
| `../../Docs/figures/steps/step3-L3-vatcull.webp`, `step4-L4-mass.webp` | 3·4단계 화면(HUD 켬) | 위 "화면의 HUD와 표의 차이" |

SVG 여섯 개는 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/04-hydration-anim-budget/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽는다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다.

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
| 화면 크기 중요도 | 4 | `LabActorSyncProcessor.cpp:350-353` |
| 할당기 켜기 | 7 | `LabGameMode.cpp:363-364`, `// ...`(`:365-366` 주석), `:367-370` |
| `TeleportActor` | 10 | `LabActorSyncProcessor.cpp:46-47`, `:51`(`:48-50` 주석을 표시 없이 뺐다), `// ...`(`:52-57` 대형 배율), `:58-59`, `:60`(줄 끝 주석을 뺐다), `:61-63` |
| `HideUntilPoseReady` | 11 | `LabMinion.cpp:319-322`, `// ...`(`:323-327` 패스 숨김을 끈 기록용 분기), `:328-333` |

뺀 주석 가운데 작업 이름을 담은 것: `LabCrowdSubsystem.cpp:126` "확정값 LODMaxCount(High는 Q4에서 인자로)", `LabActorSyncProcessor.cpp:348` "E3 (3c)(2026-11-09 사용자 결정)", `LabMinion.cpp:303-304` "plan.md M3b", "Q5.md 1b단위".

## 확인하지 않은 것

- 4단계에서 GPU가 0.597ms 준 원인. 가까운 VAT 인스턴스가 액터로 바뀐 몫으로 짐작하지만, 패스별 값을 새 트레이스로 가르지 않았다.
- 액터 하나 12.5µs의 내역. 직렬 트레이스에서 메시 틱 1.05, 평가 완료 0.828, 동기화 0.510ms(프레임당)를 읽었지만 병렬 측정과 직접 맞춰 합치지 않았다. 호출 수 257이 `actors_mean` 373과 다른 이유도 보지 않았다.
- `S8-col`에서 프레임 시간(+2.01)이 게임 스레드(+1.54)보다 더 는 이유.
- `A8-budget`의 +1.75ms가 무엇에서 나는지. 예산을 빼면 액터 클래스가 `ALabMinionMassActor`(일반 스켈레탈 메시)로 바뀐다(`LabCrowdSubsystem.cpp:101,109`). 4단계에서 예산만 끈 측정은 최종 측정에 없다.
- 소형 액터의 애니메이션 건너뜀 비율. 대형만 센다.
- 전환 대책의 화면 효과를 최종 측정 빌드에서 다시 잰 값. 본문은 옛 개발 빌드 기록의 방향만 썼다.
- 4단계 화면에서 액터의 AnimBP 동작과 전환이 실제로 보이는지. 움직이는 WebP를 열어 보지 않았다(브라우저 금지). 이미지 설명은 "액터다"까지만 적었다.
- SVG를 화면에 그려 보지 않았다. 글자 겹침은 좌표와 글자 수로만 확인했다.
