# 02 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-12 20:34\~11-13 00:39). 경위는 [작업 기록의 재측정 절](../../Docs/Worklog/Q6.md#full2-재측정2026-11-12) |
| 빌드 | Test 패키지 08e45f2, CSV 메타데이터 `labbuild=08e45f24a5e5` |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1). 측정 창 60초, 세 회차 중앙값 |
| 단계 | 1단계 `L1-full`(`-LabStage=M0 -LabVariant=full`), 2단계 `L2-mass`(`-LabStage=M1 -LabVariant=mass`, `full-conditions.json` note "로직을 Mass로, 표현은 애니 없는 ISM"). 인자와 기대 메타데이터는 `Scripts/full-conditions.json` |
| 잰 호드 총수 | 1단계 500 / 1천 / 2천 / 1만, 2단계 5천 / 1만 / 2만 / 3만. 두 단계가 함께 잰 점은 1만뿐이다 |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full2/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full2/runs.csv)(회차별 한 행), [동등성 검사](../../Measurements/Full2/equivalence.csv) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full2/full-n-at-60fps.csv)(`Scripts/plot.py` `n_at_budget`: 프레임 p95가 16.7을 넘는 두 측정점 사이에서 ms는 선형, N은 로그로 보간) |
| 트레이스 | 본문 "결과" 끝 문단의 값은 아래 "트레이스 값"이다. 최종 측정 빌드의 직렬 트레이스 한 회다 |

실행 라벨은 `Full2-L1-full-N<N>-P1-r<i>`, `Full2-L2-mass-N<N>-P1-r<i>`다. `L1-full` 1만 P1에는 버리는 첫 실행 `r0`이 `runs.csv`에 있고 요약은 r1\~r3만 쓴다. `L2-mass`에는 `r0`이 없다.

## 2단계의 구현

| 무엇 | 위치 |
| --- | --- |
| 엔티티 구성(Trait 일곱, 실행할 때 만든다) | `Source/HordeOptLab/LabCrowdSubsystem.cpp:55-93` `ULabCrowdSubsystem::BuildEntityConfig`(2단계는 `bM3`, `UsesSimLod()`, `UsesIskm()`이 모두 거짓이라 `:60-65`의 Trait 다섯과 `:81-93`의 표현만 남는다. 회피 Trait은 변형 `massav`에서만 붙는다) |
| Lab Fragment와 Tag | `Source/HordeOptLab/LabMassTypes.h`: `FLabMinionFragment`(`:17-24`), `FLabAnimStateFragment`(`:36-45`), `FLabHealthFragment`(`:58-64`), `FLabUnitTypeParams`(`:70-79`, `FMassConstSharedFragment`), `FLabMinionTag`·`FLabHitTag`·`FLabDeadTag`(`:92-110`) |
| Fragment를 붙이는 Trait | `Source/HordeOptLab/LabMassTraits.cpp:11-37` `ULabMinionTrait::BuildTemplate`. 이동 Trait `ULabMovementTrait`(`:72-79`, 속도 3m/s, 가속 2048) |
| 접근 | `Source/HordeOptLab/LabMassProcessors.cpp:71-201` `ULabApproachProcessor`(Tasks 그룹, `FMassMoveTargetFragment`를 오브 쪽으로) |
| 범위 피해 | 같은 파일 `:367-479` `ULabOrbContactProcessor`. 피해 식은 1단계 `ALabOrb::Tick`(`LabOrb.cpp:111-133`)과 같은 `Lerp(ZoneDpsEdge, ZoneDpsCenter, Closeness) × 델타` |
| 이동 멈춤 | 같은 파일 `:63-66` `StopMoving`(`FMassCodeDrivenMovementTag`를 지연 명령으로 뗀다) |
| 저격 | 같은 파일 `:483-585` `ULabOrbSnipeProcessor` |
| 상태 타이머, 지우기와 재생성 | 같은 파일 `:629-747` `ULabMinionStateProcessor`. 2단계는 풀을 쓰지 않아 `:733-746`의 지연 파괴(`Defer().DestroyEntities`)와 `RequestRespawn` 경로다 |
| ISM 갱신 Processor 고르기 | `Source/HordeOptLab/HordeOptLab.cpp:38` 2단계(`Stage < M2`)는 엔진 `UMassUpdateISMProcessor`를 등록한다 |
| 기본으로 꺼진 Processor 셋 켜기 | `Config/DefaultMass.ini:6-13` |
| 그림자 켬 | `LabCrowdSubsystem.cpp:86` `MeshDesc.bCastShadows = true` |
| Lumen 장면에서 빼기 | `Source/HordeOptLab/LabMinion.h:39-42` `LabExcludeFromLumen`(`SetAffectDynamicIndirectLighting(false)`), 호출은 `LabCrowdSubsystem.cpp:404-446` `ExcludeVisualsFromLumen`(오브 틱이 부른다, `LabOrb.cpp:244`) |
| 정적 메시 | `Scripts/make-static-minion.py`(`UAnimToTextureBPLibrary::ConvertSkeletalMeshToStaticMesh`, 레퍼런스 포즈, LOD 0) |

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 모두 P1이다. "(그림)"은 본문 산문이 아니라 그림에만 쓴 값이다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차(r1 / r2 / r3) |
| --- | --- | --- | --- | --- |
| 1단계 프레임 시간 440ms | 439.958 | `L1-full` 10000 | `frame_p50_ms` | 436.39 / 440.652 / 439.958 |
| 1단계 게임 스레드 425ms | 424.871 | 같음 | `game_p50_ms` | 420.75 / 425.783 / 424.871 |
| 1단계 렌더 스레드 30.4ms | 30.389 | 같음 | `render_p50_ms` | 29.027 / 30.389 / 31.062 |
| 1단계 GPU 50.2ms | 50.227 | 같음 | `gpu_p50_ms` | 50.145 / 50.633 / 50.227 |
| 2단계 프레임 시간 10.3ms | 10.307 | `L2-mass` 10000 | `frame_p50_ms` | 10.185 / 10.439 / 10.307 |
| 2단계 게임 스레드 6.30ms | 6.303 | 같음 | `game_p50_ms` | 6.332 / 6.303 / 6.274 |
| 2단계 렌더 스레드 6.48ms | 6.481 | 같음 | `render_p50_ms` | 6.353 / 6.482 / 6.481 |
| 2단계 GPU 10.0ms | 10.021 | 같음 | `gpu_p50_ms` | 9.91 / 10.151 / 10.021 |
| 1단계 병목 게임 스레드, 2단계 병목 GPU | | 위 두 행 | `bottleneck` = `game` / `gpu`(세 회차 모두) | |
| 프레임 42.7분의 1 | 42.69 | 계산 | 439.958 ÷ 10.307 | |
| 게임 스레드 67.4분의 1 | 67.41 | 계산 | 424.871 ÷ 6.303 | |
| 1단계 드로우 콜 10,274개 | 10274 | `L1-full` 10000 | `draw_calls_p50` | 10274 / 10272 / 10276 |
| (그림) 1단계 드로우 콜 575 / 1,091 / 2,100 | 575 / 1091 / 2100 | `L1-full` 500 / 1000 / 2000 | `draw_calls_p50` | 575 / 574 / 575, 1090 / 1091 / 1091, 2100 ×3 |
| 2단계 드로우 콜 122\~123개 | 123 / 123 / 122 / 123 | `L2-mass` 5000 / 10000 / 20000 / 30000 | `draw_calls_p50` | 회차 값 122\~123 |
| "드로우 콜이 개체 하나에 하나꼴" | 1.15 / 1.09 / 1.05 / 1.03 | 계산 | 1단계 드로우 콜 ÷ 호드 총수(500 / 1천 / 2천 / 1만) | |
| 1단계 GPU는 60fps 한 프레임의 3.01배 | 3.014 | 계산 | 50.227 ÷ 16.667 | |
| 1단계 개체당 게임 스레드 "17µs 안팎"(프레임 50ms 안) | 17.57 / 17.04 / 17.43µs | 계산 | `L1-full` 500 / 1000 / 2000 `game_p50_ms` ÷ N(8.784, 17.038, 34.861). 세 점의 프레임 p50은 9.36 / 18.0 / 36.6ms | 게임 스레드 8.746 / 8.784 / 8.98, 16.943 / 17.038 / 17.06, 34.79 / 34.861 / 34.89 |
| 1단계 개체당 게임 스레드 17.0µs(하위 스텝 없이) | 17.04 | 계산 | 17.038 ms ÷ 1000 | |
| 1단계 개체당 게임 스레드 1만 42.5µs | 42.49 | 계산 | 424.871 ms ÷ 10000 | |
| 2단계 개체당 게임 스레드 0.630µs(1만) | 0.6303 | 계산 | 6.303 ms ÷ 10000 | |
| 2단계 개체당 "0.62µs 안팎"(그림 0.664 / 0.630 / 0.617 / 0.619. 2만의 0.6175는 부동소수 반올림으로 0.617로 찍힌다) | 0.6644 / 0.6303 / 0.6175 / 0.6193 | 계산 | `L2-mass` 5000 / 10000 / 20000 / 30000 `game_p50_ms`(3.322, 6.303, 12.35, 18.578) ÷ N | 게임 스레드 회차 값은 `runs.csv` |
| 27.0분의 1 | 27.03 | 계산 | 17.04 ÷ 0.6303 | |
| (참고) 2단계 기울기 0.610µs | 0.6102 | 계산 | (18.578 − 3.322) ÷ 25000 × 1000. 본문에는 쓰지 않았다 | |
| 2단계 60fps 최대 개체 수 20,753 | 20753 | `2 L2-mass` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준은 22,853(`n_p50`) |
| 보간식 | 20753 | 계산 | t = (16.7 − 16.069) ÷ (22.989 − 16.069) = 0.0912, N = exp(ln 20000 + t × ln 1.5) | |
| 1단계 60fps 최대 개체 수 774 | 774 | `1 L1-full` | `full-n-at-60fps.csv` `n_p95` | |
| 26.8배 | 26.81 | 계산 | 20753 ÷ 774 | |
| (그림) 2단계 프레임 p95 8.13 / 10.9 / 16.1 / 23.0ms | 8.133 / 10.931 / 16.069 / 22.989 | `L2-mass` 5000 / 10000 / 20000 / 30000 | `frame_p95_ms` | 8.287 / 8.018 / 8.133, 10.86 / 11.098 / 10.931, 16.738 / 15.876 / 16.069, 23.754 / 22.74 / 22.989 |
| (그림) 1단계 프레임 p95 11.0 / 20.0 / 39.5 / 464ms | 11.01 / 20.038 / 39.488 / 463.579 | `L1-full` 500 / 1000 / 2000 / 10000 | `frame_p95_ms` | 11.01 / 10.904 / 11.121, 20.038 / 19.924 / 20.432, 39.809 / 39.488 / 39.337, 463.963 / 462.879 / 463.579 |
| "잰 네 호드 총수 모두에서 GPU가 가장 느리다" | | `L2-mass` 5000\~30000 | `bottleneck` = `gpu`(열두 회차 모두). 2만 GPU 14.59 / 게임 12.35 / 렌더 11.53, 3만 GPU 19.29 / 게임 18.58 / 렌더 15.93 | |
| 초당 사망 1단계 62.2, 2단계 62.0 | 62.183 / 61.99 | 각 10000 | `death_rate`. 동등성 검사 기준 62.206에서 −0.04% / −0.35%(허용 ±3%, `equivalence.csv`) | 62.183 ×3 / 62.032, 61.989, 61.990 |
| "2단계의 프레임은 0.1초보다 훨씬 짧다" | 프레임 p99 11.57ms, 50ms 넘는 프레임 0개 | `L2-mass` 10000 | `frame_p99_ms`(회차 최대 12.453), `hitch_abs` = 0(세 회차) | |
| 0.1초 상한 | 0.1 | 엔진 소스 | 아래 "엔진 소스" | |
| (그림) 저격 "2초마다 다섯", 액터 10,000 | 2초, 5명 | 시나리오 | [measurement.md](../../Docs/Guides/measurement.md) "확정값"의 저격 행. 10,000은 호드 총수 1만 | |
| 표현 LOD 네 단계 | `EMassLOD::Max` = 4(High, Medium, Low, Off) | 코드 | `LabCrowdSubsystem.cpp:87-93`의 `for`가 네 단계를 돈다. 경계 `1.e7f`cm(100km)는 맵 반경 500m 밖이다 | |
| Processor 넷 | 접근, 범위 피해, 저격, 상태 타이머 | 코드 | 위 "2단계의 구현". `ULabNearOrbCountProcessor`(동등성 통계)와 피해 방식 실험용 `ULabDamageGen/ApplyProcessor`(2단계에서는 일하지 않는다)는 세지 않았다 | |

## 화면의 HUD와 표의 차이

요약의 2단계 화면(`step2-L2-mass.webp`) 왼쪽 위 HUD의 프레임 시간(11.1ms)은 표의 값(10.3ms)보다 높다. 화면은 측정 실행이 아니라 촬영 실행(`Pre5shot-L2-mass-N10000-P1-r1`)에서 찍었고 4K 영역 화면 녹화 부하가 섞인다. 1단계 화면의 HUD(542.7ms)도 같다. 수치는 표(Test 패키지 측정 실행)의 값만 쓴다. 촬영 경위는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "촬영 준비"에 있다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/mass-summary.svg` | 요약 그림. 위: 두 단계의 프레임·게임 스레드·렌더 스레드·GPU 시간 막대(1만 P1, 같은 축척 0\~450ms). 아래: 2단계만 0\~20ms로 확대하고 16.7ms 선 | 위 1만 행 여덟 개 |
| `images/mass-draw-calls.svg` | 문제 그림. 호드 총수(로그)에 따른 드로우 콜(로그). 1단계 네 점, 2단계 네 점, "개체 하나에 하나" 점선(y = N) | 위 드로우 콜 행 |
| `images/mass-processors.svg` | 원리 도식. 왼쪽: 액터마다 이동 컴포넌트·스켈레탈 메시와 AnimBP·렌더 상태 보내기를 틱한다. 오른쪽: Chunk 하나의 Fragment 열(위치, 속도, 이동 목표, 체력, 상태)과 2단계 Processor 여섯 칸의 선언 순서. 수치 없음 | 순서는 [code-tour.md](../../Docs/Guides/code-tour.md) "Mass PrePhysics 안의 선언 순서"(2단계에 해당하는 행만). 1단계 칸은 01번 글 `actor-frame-flow.svg`와 같다 |
| `images/mass-frame-vs-n.svg` | 결과 그림. 위: 두 단계의 프레임 p95(로그-로그), 16.7ms 선, 60fps를 지키는 최대 개체 수 774와 20,753. 두 계열은 잇지 않는다(바뀐 변수가 둘이라 measurement.md "차이 판정"). 아래: 개체당 게임 스레드 시간(µs, 로그) 여덟 점 | 위 프레임 p95 행, 개체당 행, 최대 개체 수 행 |
| `../../Docs/figures/steps/step1-L1-full.webp`, `step2-L2-mass.webp` | 1·2단계 화면(HUD 켬) | 위 "화면의 HUD와 표의 차이" |

SVG 넷은 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/02-mass-logic/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽고 상수로 넣은 측정값은 없다. 스크립트는 끝에 계산값(배율, 개체당 시간, 기울기)을 출력한다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다. 색은 01번 글과 같다(1단계 빨강, 2단계 파랑, 16.7ms 호박색).

## 트레이스 값

본문 "결과" 끝 문단의 값이다. Insights 화면은 찍지 않고 명령줄 내보내기로 뽑았다.

| 항목 | 값 |
| --- | --- |
| 트레이스 | `Saved/Traces/Full2traceser-L2-mass-N10000-P1-r1.utrace`(직렬, `mass.FullyParallel` 끔, 저장소 밖) |
| 내보낸 표 | `%TEMP%\horde-trace\F2-traceser-L2-mass-GT.csv`(`Scripts/trace-export.ps1 -Command TimerStatistics`, 스레드 `GameThread`, 구간 25\~75초, 저장소 밖) |
| 프레임 수 | 5,038(`FEngineLoop::Tick` 횟수). `FEngineLoop::Tick` Incl 49.998456초 |
| 식 | 타이머의 합(초) × 1000 ÷ 5,038 = 프레임당 ms. 모두 게임 스레드, 구간 안 평균이다. 화면에서 고른 프레임 하나가 아니다 |
| 주의 | 직렬 실행이라 게임 스레드 값이 병렬 측정(6.30ms)과 다르다. 본문과 이 표는 "직렬 트레이스"로만 적고 측정값과 견주지 않는다 |

| 본문 값 | 정밀한 값 | 타이머 | 합(초) |
| --- | --- | --- | --- |
| 프레임 평균 9.92ms | 9.9244 | `FEngineLoop::Tick` Incl | 49.998456 |
| Mass 처리 단계 3.01ms | 3.0122 | `FMassProcessingPhase::ExecuteTick EMassProcessingPhase::PrePhysics` Incl(횟수 5,037) | 15.175215 |
| `EndVisualChanges` 2.52ms | 2.5152 | `MassVisualizationComponent EndVisualChanges` Incl = Excl | 12.671333 |
| 가장 긴 Processor 0.925ms | 0.9251 | `MassSmoothOrientationProcessor_2147482251` Incl = Excl | 4.660537 |
| 그다음 0.440ms | 0.4398 | `MassSteerToMoveTargetProcessor_2147482252` Incl = Excl | 2.215693 |
| `UMassUpdateISMProcessor` 0.573ms | 0.5729 | `MassUpdateISMProcessor_2147482256` Incl = Excl | 2.886137 |
| `UCharacterMovementComponent_TickComponent` 없음 | 표에 행 없음(`Character`로 찾은 행은 `FActorComponentTickFunction::ExecuteTick`뿐) | 304행 전체 | |

참고(본문에 쓰지 않았다): `Sync_RenderingThread` 2.7704ms(13.957100초), `MassApplyMovementProcessor` 0.1506, `LabApproachProcessor` 0.1297, `MassApplyForceProcessor` 0.1077ms. 타이머 이름 끝의 숫자는 Processor 인스턴스 번호다.

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다. 경로는 `Engine/` 아래다.

| 사실 | 위치 |
| --- | --- |
| 이동 Processor가 델타를 0.1초로 자른다. 주석 "Clamp max delta time to avoid force explosion on large time steps (i.e. during initialization)" | `Plugins/Runtime/MassGameplay/Source/MassMovement/Private/MassMovementProcessors.cpp:40-41`(`UMassApplyForceProcessor::Execute`), `:99-100`(`UMassApplyMovementProcessor::Execute`) |
| 이동은 `Velocity = DesiredVelocity`, `Location += Velocity × DeltaTime` 한 번이고, `FMassCodeDrivenMovementTag`가 있는 엔티티만 옮긴다 | 같은 파일 `:115`, `:127`, 쿼리 `:88` |
| Mass ISM 메시 설명자의 그림자 기본값 `bCastShadows = false` | `Plugins/Runtime/MassGameplay/Source/MassRepresentation/Public/MassRepresentationTypes.h:103-105` `FMassStaticMeshInstanceVisualizationMeshDesc` |
| 기본으로 등록하지 않는 Processor 셋: `bAutoRegisterWithProcessingPhases = false` | `Plugins/Runtime/MassGameplay/Source/MassLOD/Private/MassLODCollectorProcessor.cpp:14`, `MassRepresentation/Private/MassVisualizationLODProcessor.cpp:13`, `MassRepresentation/Private/MassRepresentationProcessor.cpp:54`(하위 `UMassVisualizationProcessor`가 상속) |
| `UMassUpdateISMProcessor`는 게임 스레드에서 돌고, 보이는 엔티티가 있는 Chunk의 위치를 `UpdateISMTransform`으로 쌓는다 | `Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassUpdateISMProcessor.cpp:15-22`(`bRequiresGameThreadExecution`), `:24-36`(쿼리, `AreAnyEntitiesVisibleInChunk`), `:38-69`(`Execute`) |
| 쌓은 ISM 데이터는 PostPhysics 단계가 끝날 때 `EndVisualChanges`가 컴포넌트에 올린다 | [engine-notes.md](../../Docs/Reference/engine-notes.md) 27절(`MassRepresentationSubsystem.cpp:444-445,526-540`). 이번에 다시 열지 않았다 |
| 컴포넌트는 메시 설명자마다 하나다(종류 둘 = ISM 컴포넌트 둘) | engine-notes.md 22절(`MassVisualizationComponent.cpp:259`). 이번에 다시 열지 않았다 |
| 소프트웨어 Lumen 장면은 동적 간접광에 영향을 주고 거리 필드가 있는 프리미티브만 넣는다 | engine-notes.md 11절(`PrimitiveSceneProxy.cpp:1701-1734`). 이번에 다시 열지 않았다 |
| Processor 기본 처리 단계 PrePhysics | engine-notes.md 27절(`MassEntity/Public/MassProcessor.h:244`) |

### 본문에 넣은 발췌

엔진 코드는 한 블록 14줄이다(빈 줄 하나를 더하면 15줄). 남긴 줄은 소스를 그대로 옮겼다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| `UMassApplyMovementProcessor::Execute` | 14 | `MassMovementProcessors.cpp:99-100`, 빈 줄 `:101`, `:102-103`, `// ...`(`:104-107` 배열 뷰 셋), `:108-109`, `// ...`(`:110-114`), `:115`, `// ...`(`:116-126` 변환 참조, 디버그 분기, 현재 위치 읽기), `:127-128`, `:129-130`. 함수 머리(`:96-98`)와 뒤의 디버그 쿼리·닫는 괄호(`:131-146`)는 뺐다 |

프로젝트 코드 발췌는 줄 끝 주석을 뺐다(주석에 작업 단계 이름이 있다). 줄인 곳은 `// ...`로 적었다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| `ULabMinionTrait::BuildTemplate` | 9 | `LabMassTraits.cpp:15-21`, `// ...`(`:22-35`), `:36`. `:16`, `:17`, `:20`의 줄 끝 주석을 뺐다 |
| `ULabOrbContactProcessor::Execute` | 15 | `LabMassProcessors.cpp:425`, `// ...`(`:426-435`), `:436-437`, `// ...`(`:438-452`), `:453-455`, `// ...`(`:456-457`), `:458-460`(`:458` 줄 끝 주석 뺌), `// ...`(`:461` `++Kills`), `:462-463` |
| `ULabCrowdSubsystem::BuildEntityConfig` 표현 | 12 | `LabCrowdSubsystem.cpp:81-83`, `// ...`(`:84-85` 회전·배율), `:86`, `:87-93`. `:86`, `:90`, `:91`의 줄 끝 주석을 뺐다 |

## 경위의 근거

- "넣은 채로 돌린 시험 실행에서 GPU 시간이 크게 늘었다"(Lumen): 옛 시험 실행이고 최종 측정에 대응 조건이 없어 수치를 쓰지 않았다. 근거는 [engine-notes.md](../../Docs/Reference/engine-notes.md) 11절 "움직이는 ISM과 Lumen" 행과 [measurement.md](../../Docs/Guides/measurement.md) "확정값"의 "미니언과 Lumen, 그림자" 행(2천, Test 패키지 시험, 원본 `Saved/Profiling/CSV/trial/`은 저장소 밖)이다. 어느 Lumen 패스가 시간을 썼는지는 확인하지 않았다.
- 글 재료 가운데 쓰지 않은 것: 옛 묶음의 "17배"·"25배"와 그 분해(애니 없는 액터, CharacterMovement를 끈 액터의 변형은 최종 측정에 없다), 병렬 처리 비교(최종 측정에 직렬 조건이 없다), 개발 빌드에서 이동 상한 때문에 처치가 줄었던 관찰(개발 빌드 값이다. 본문은 엔진 소스와 최종 측정의 사망 수만 썼다), 옛 묶음 2만의 회차 간 흔들림(최종 측정의 2단계 2만 프레임 p50 범위는 14.68\~15.14ms로 0.46ms다). 근거와 판단은 `Docs/post-review.md` "02 Mass 코어"(글을 다 쓰면 지우는 문서라, 지운 뒤에는 git 이력에서 본다)와 `Docs/post-material.md` "02 Mass 코어"다.
- 2단계의 비교 상대: 옛 계획은 애니 없는 정적 메시 액터 변형과 견줘 변수를 하나로 두었다(`Docs/plan.md` "M1"). 최종 측정의 사다리는 1단계(액터 군중) 다음이 2단계라 로직과 표현이 함께 바뀐다. 본문은 이것을 "변수 둘"로 적었다.

## 확인하지 않은 것

- 로직과 표현 가운데 어느 쪽이 얼마를 줄였는지. 최종 측정에 둘을 따로 바꾼 조건이 없다.
- GPU 50.2ms가 어느 패스에서 나는지와 2단계에서 무엇이 줄었는지. 프레임 CSV에는 패스별 값이 없고, 트레이스에서는 게임 스레드 값만 뽑았다(GPU 패스는 보지 않았다). 본문은 드로우 콜이 개체 수를 따라간다는 측정만 적고 GPU 시간의 원인으로 적지 않았다.
- "1단계는 액터마다 메시 컴포넌트를 따로 그린다"는 드로우 콜 ÷ 호드 총수가 1.03\~1.15인 측정과 스켈레탈 메시 컴포넌트의 구조로 판단했다. 드로우 콜의 내역(그림자, 속도 패스)은 세지 않았다.
- 게임 스레드 6.30ms의 내역. 본문은 남는 일의 종류(Processor 순회, ISM 위치 올리기)만 적었다. 내역은 위 "트레이스 값"에 있다(직렬 트레이스라 측정값 6.30ms와 견주지 않는다).
- 병렬 처리가 번 몫. 최종 측정은 모두 `mass.FullyParallel=true`다.
- 이동 상한(0.1초)이 2단계에서 실제로 걸리지 않는다는 것은 프레임 p99와 50ms 넘는 프레임 수로 판단했다. 엔티티의 이동 거리를 직접 세지 않았다.
- 화면: 1·2단계 WebP를 재생해 보지 않았다(브라우저를 띄우지 않는다). "걷는 동작 없이 한 자세로 움직인다", "피격·사망 때 멈춰 섰다가 사라진다"는 코드(`UMassUpdateISMProcessor`는 위치만 쓰고, 피격·사망은 이동 태그를 뗀다)에서 판단했다.
- SVG 넷을 화면에서 보지 않았다. 글자와 값만 텍스트로 대조했다.
