# 02 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-13 16:52:49\~21:00:00). 경위는 [작업 기록의 측정 절](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3) |
| 빌드 | Test 패키지 df41c13, CSV 메타데이터 `labbuild=df41c131c650` |
| 조건이 바뀐 이유 | 애니메이션 예산 기본값을 2ms에서 4ms로 올렸고([ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md)), 오브의 저격 시점 경합을 고쳐(62b6a42) 전 조건을 다시 쟀다. 1·2단계는 예산을 쓰지 않는다 |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1). 측정 창 60초, 세 회차 중앙값 |
| 단계 | 1단계 `L1-full`(`-LabStage=M0 -LabVariant=full`), 2단계 `L2-mass`(`-LabStage=M1 -LabVariant=mass`, `full-conditions.json` note "로직을 Mass로, 표현은 애니 없는 ISM"). 인자와 기대 메타데이터는 `Scripts/full-conditions.json` |
| 잰 호드 총수 | 1단계 500 / 1천 / 2천 / 1만, 2단계 5천 / 1만 / 2만 / 3만. 두 단계가 함께 잰 점은 1만뿐이다 |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full3/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full3/runs.csv)(회차별 한 행), [동등성 검사](../../Measurements/Full3/equivalence.csv) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full3/full-n-at-60fps.csv)(`Scripts/plot.py` `n_at_budget`: 프레임 p95가 16.7을 넘는 두 측정점 사이에서 ms는 선형, N은 로그로 보간) |
| 트레이스 | 본문 "결과" 끝 문단의 값은 아래 "트레이스 값"이다. 같은 빌드(df41c13)의 직렬 트레이스 한 회다 |

실행 라벨은 `Full3-L1-full-N<N>-P1-r<i>`, `Full3-L2-mass-N<N>-P1-r<i>`다. `L1-full` 1만 P1에는 버리는 첫 실행 `r0`이 `runs.csv`에 있고 요약은 r1\~r3만 쓴다. `L2-mass`에는 `r0`이 없다.

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
| 1단계 프레임 시간 444ms | 444.279 | `L1-full` 10000 | `frame_p50_ms` | 449.171 / 441.687 / 444.279 |
| 1단계 게임 스레드 429ms | 429.438 | 같음 | `game_p50_ms` | 433.98 / 425.808 / 429.438 |
| 1단계 렌더 스레드 29.8ms | 29.797 | 같음 | `render_p50_ms` | 28.86 / 29.797 / 30.041 |
| 1단계 GPU 50.2ms | 50.168 | 같음 | `gpu_p50_ms` | 50.952 / 50.145 / 50.168 |
| 2단계 프레임 시간 10.2ms | 10.193 | `L2-mass` 10000 | `frame_p50_ms` | 9.896 / 10.193 / 10.647 |
| 2단계 게임 스레드 6.24ms | 6.24 | 같음 | `game_p50_ms` | 6.24 / 6.24 / 6.199 |
| 2단계 렌더 스레드 6.40ms | 6.396 | 같음 | `render_p50_ms` | 6.276 / 6.493 / 6.396 |
| 2단계 GPU 9.90ms | 9.903 | 같음 | `gpu_p50_ms` | 9.62 / 9.903 / 10.356 |
| 1단계 병목 게임 스레드, 2단계 병목 GPU | | 위 두 행 | `bottleneck` = `game` / `gpu`(세 회차 모두) | |
| 프레임 43.6분의 1 | 43.59 | 계산 | 444.279 ÷ 10.193 | |
| 게임 스레드 68.8분의 1 | 68.82 | 계산 | 429.438 ÷ 6.24 | |
| 1단계 드로우 콜 10,272개 | 10272 | `L1-full` 10000 | `draw_calls_p50` | 10272 / 10268.5 / 10274.5 |
| (그림) 1단계 드로우 콜 575 / 1,090 / 2,100 | 575 / 1090 / 2100 | `L1-full` 500 / 1000 / 2000 | `draw_calls_p50` | 575 / 574 / 575, 1090 / 1089.5 / 1090, 2100 / 2100 / 2101 |
| 2단계 드로우 콜 123개 | 123 / 123 / 123 / 123 | `L2-mass` 5000 / 10000 / 20000 / 30000 | `draw_calls_p50` | 회차 값 모두 123 |
| "드로우 콜이 개체 하나에 하나꼴" | 1.15 / 1.09 / 1.05 / 1.03 | 계산 | 1단계 드로우 콜 ÷ 호드 총수(500 / 1천 / 2천 / 1만) | |
| 1단계 GPU는 60fps 한 프레임의 3.01배 | 3.010 | 계산 | 50.168 ÷ 16.667 | |
| 1단계 개체당 게임 스레드 "17\~18µs"(프레임 50ms 안) | 17.90 / 17.02 / 17.64µs | 계산 | `L1-full` 500 / 1000 / 2000 `game_p50_ms` ÷ N(8.949, 17.018, 35.287). 세 점의 프레임 p50은 9.50 / 18.0 / 37.1ms | 게임 스레드 8.949 / 8.858 / 8.995, 16.847 / 17.165 / 17.018, 35.346 / 35.186 / 35.287 |
| 1단계 개체당 게임 스레드 17.0µs(하위 스텝 없이) | 17.02 | 계산 | 17.018 ms ÷ 1000 | |
| 1단계 개체당 게임 스레드 1만 42.9µs | 42.94 | 계산 | 429.438 ms ÷ 10000 | |
| 2단계 개체당 게임 스레드 0.624µs(1만) | 0.6240 | 계산 | 6.24 ms ÷ 10000 | |
| 2단계 개체당 "0.61\~0.66µs"(그림 0.663 / 0.624 / 0.613 / 0.617. 3만의 0.6175는 부동소수 반올림으로 0.617로 찍힌다) | 0.6626 / 0.6240 / 0.6128 / 0.6175 | 계산 | `L2-mass` 5000 / 10000 / 20000 / 30000 `game_p50_ms`(3.313, 6.24, 12.255, 18.525) ÷ N | 게임 스레드 3.224 / 3.373 / 3.313, 6.24 / 6.24 / 6.199, 12.255 / 12.236 / 12.359, 18.525 / 18.499 / 18.974 |
| 27분의 1 | 27.27 | 계산 | 17.018 ÷ 0.6240 | |
| (참고) 2단계 기울기 0.609µs | 0.6085 | 계산 | (18.525 − 3.313) ÷ 25000 × 1000. 본문에는 쓰지 않았다 | |
| 2단계 60fps 최대 개체 수 약 2만(19,554) | 19554 | `2 L2-mass` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준은 21,775(`n_p50`) |
| 보간식 | 19554 | 계산 | 16.7 ≥ 1만의 10.837이고 2만의 16.897이 16.7을 넘으므로 1만\~2만 사이다. t = (16.7 − 10.837) ÷ (16.897 − 10.837) = 0.9675, N = exp(ln 10000 + t × ln 2) | |
| 1단계 60fps 최대 개체 수 766 | 766 | `1 L1-full` | `full-n-at-60fps.csv` `n_p95` | |
| 25.5배 | 25.53 | 계산 | 19554 ÷ 766 | |
| (그림) 2단계 프레임 p95 8.02 / 10.8 / 16.9 / 24.1ms | 8.016 / 10.837 / 16.897 / 24.052 | `L2-mass` 5000 / 10000 / 20000 / 30000 | `frame_p95_ms` | 7.787 / 8.016 / 8.031, 10.598 / 10.837 / 11.226, 16.972 / 16.891 / 16.897, 24.356 / 24.052 / 23.098 |
| (그림) 1단계 프레임 p95 11.1 / 20.2 / 40 / 470ms | 11.099 / 20.206 / 40.046 / 469.662 | `L1-full` 500 / 1000 / 2000 / 10000 | `frame_p95_ms` | 11.099 / 11.05 / 11.553, 20.206 / 20.349 / 20.152, 40.046 / 40.273 / 39.658, 474.961 / 469.662 / 468.347 |
| "잰 네 호드 총수 모두에서 GPU가 가장 느리다" | | `L2-mass` 5000\~30000 | `bottleneck` = `gpu`(열두 회차 모두). 2만 GPU 15.39 / 게임 12.26 / 렌더 11.31, 3만 GPU 19.37 / 게임 18.53 / 렌더 15.67 | |
| 초당 사망 1단계 62.2, 2단계 62 | 62.183 / 61.998 | 각 10000 | `death_rate`. 동등성 검사 기준 62.204에서 −0.03% / −0.33%(허용 ±3%, `equivalence.csv`) | 62.183 ×3 / 61.998, 62.007, 61.990 |
| "2단계의 프레임은 0.1초보다 훨씬 짧다" | 프레임 p99 11.63ms, 50ms 넘는 프레임 0개 | `L2-mass` 10000 | `frame_p99_ms`(회차 최대 11.954), `hitch_abs` = 0(세 회차) | |
| 0.1초 상한 | 0.1 | 엔진 소스 | 아래 "엔진 소스" | |
| (그림) 저격 "2초마다 다섯", 액터 10,000 | 2초, 5명 | 시나리오 | [measurement.md](../../Docs/Guides/measurement.md) "확정값"의 저격 행. 10,000은 호드 총수 1만 | |
| 표현 LOD 네 단계 | `EMassLOD::Max` = 4(High, Medium, Low, Off) | 코드 | `LabCrowdSubsystem.cpp:87-93`의 `for`가 네 단계를 돈다. 경계 `1.e7f`cm(100km)는 맵 반경 500m 밖이다 | |
| Processor 넷 | 접근, 범위 피해, 저격, 상태 타이머 | 코드 | 위 "2단계의 구현". `ULabNearOrbCountProcessor`(동등성 통계)와 피해 방식 실험용 `ULabDamageGen/ApplyProcessor`(2단계에서는 일하지 않는다)는 세지 않았다 | |

## 화면의 HUD와 표의 차이

요약의 2단계 화면(`step2-L2-mass.webp`) 왼쪽 위 HUD의 프레임 시간(움직이는 WebP의 한 프레임을 열어 본 값 9.8ms)은 표의 값(10.2ms)과 다르다. 화면은 측정 실행이 아니라 촬영 실행(`Full3shot-L2-mass-N10000-P1-r1`, [captures.csv](../../Measurements/Full3/captures.csv))에서 찍었고 4K 영역 화면 녹화 부하가 섞인다. 1단계 화면의 HUD(같은 방식으로 열어 본 값 478.7ms, 촬영 실행 `Full3shot-L1-full-N10000-P1-r2`)도 표와 다르다. 수치는 표(Test 패키지 측정 실행)의 값만 쓴다. 촬영 경위는 [Worklog/Q7.md](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3)의 "촬영 끝" 줄에 있다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/mass-summary.svg` | 요약 그림. 위: 두 단계의 프레임·게임 스레드·렌더 스레드·GPU 시간 막대(1만 P1, 같은 축척 0\~450ms). 아래: 2단계만 0\~20ms로 확대하고 16.7ms 선 | 위 1만 행 여덟 개 |
| `images/mass-draw-calls.svg` | 문제 그림. 호드 총수(로그)에 따른 드로우 콜(로그). 1단계 네 점, 2단계 네 점, "개체 하나에 하나" 점선(y = N) | 위 드로우 콜 행 |
| `images/mass-processors.svg` | 원리 도식. 왼쪽: 액터마다 이동 컴포넌트·스켈레탈 메시와 AnimBP·렌더 상태 보내기를 틱한다. 오른쪽: Chunk 하나의 Fragment 열(위치, 속도, 이동 목표, 체력, 상태)과 2단계 Processor 여섯 칸의 선언 순서. 수치 없음 | 순서는 [code-tour.md](../../Docs/Guides/code-tour.md) "Mass PrePhysics 안의 선언 순서"(2단계에 해당하는 행만). 1단계 칸은 01번 글 `actor-frame-flow.svg`와 같다 |
| `images/mass-frame-vs-n.svg` | 결과 그림. 위: 두 단계의 프레임 p95(로그-로그), 16.7ms 선, 60fps를 지키는 최대 개체 수 766과 19,554. 두 계열은 잇지 않는다(바뀐 변수가 둘이라 measurement.md "차이 판정"). 아래: 개체당 게임 스레드 시간(µs, 로그) 여덟 점 | 위 프레임 p95 행, 개체당 행, 최대 개체 수 행 |
| `../../Docs/figures/steps/step1-L1-full.webp`, `step2-L2-mass.webp` | 1·2단계 화면(HUD 켬) | 위 "화면의 HUD와 표의 차이" |

| `images/mass-gain.svg` | 결과 그림. 선형 축 막대 둘씩. 위: 60fps를 지키는 최대 개체 수 766과 19,554, "25.5배". 아래: 개체당 게임 스레드 시간 17(1단계, 호드 총수 1천)과 0.624µs(2단계, 1만), "27분의 1" | 위 최대 개체 수 행, 개체당 행, 25.5배와 27분의 1 행 |
| `images/manager-diagram.svg` | 참고 절 도식. 왼쪽: 틱 없는 액터, 매니저의 틱 하나, ISM 컴포넌트 둘. 오른쪽: 엔티티, 로직 Processor(워커 스레드), 표현 쪽 Processor 넷과 위치 올리기(게임 스레드), ISM 컴포넌트 둘. 수치 없음 | 아래 "참고 측정: 매니저 방식"의 구현 행과 트레이스 표 |
| `images/manager-times.svg` | 참고 측정 그림. 게임 스레드 시간과 프레임 시간의 중앙값 막대 열둘(1만·2만 × 세 조건), 세 회차의 최소\~최대 선, 16.7ms 선 | 아래 "참고 측정의 값" |
| `images/manager-gamethread.svg` | 트레이스 그림. 호드 총수 2만에서 게임 스레드의 처리 단계 구성 막대 셋 | 아래 "참고 트레이스의 값" |

`mass-summary`·`mass-draw-calls`·`mass-processors`·`mass-frame-vs-n` 넷은 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/02-mass-logic/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽고 상수로 넣은 측정값은 없다. 스크립트는 끝에 계산값(배율, 개체당 시간, 기울기)을 출력한다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다. 색은 01번 글과 같다(1단계 빨강, 2단계 파랑, 16.7ms 호박색).

`mass-gain`·`manager-diagram`·`manager-times`·`manager-gamethread` 넷은 [Scripts/plot-post02-manager.py](../../Scripts/plot-post02-manager.py)가 만든다(저장소 루트에서 `py -3 -I Scripts/plot-post02-manager.py`). 입력은 `Measurements/Full3/summary.csv`, `Docs/figures/full3/full-n-at-60fps.csv`, `Measurements/Mgr1/summary.csv`, `Measurements/MgrT1/timers.csv`이고 상수로 넣은 측정값은 없다. 스크립트는 끝에 계산값(차이, 비율, 처리 단계의 구성)을 출력한다. 반올림은 반올림-위로다(5.265 → 5.27). 매니저 방식은 보라, 2단계는 파랑이다. 한 스레드 2단계는 그리지 않고 끝의 출력에만 낸다. 넷은 2026-11-14에 브라우저에서 그려 보았다.

## 트레이스 값

본문 "결과" 끝 문단의 값이다. 값은 명령줄 내보내기로 뽑았다. 같은 트레이스의 Insights 화면은 아래 "Insights 화면"에 있다.

| 항목 | 값 |
| --- | --- |
| 트레이스 | `Saved/Traces/Full3traceser-L2-mass-N10000-P1-r1.utrace`(직렬, `mass.FullyParallel` 끔, 저장소 밖) |
| 내보낸 표 | `%TEMP%\horde-trace\F3-traceser-L2-mass-GT.csv`(`Scripts/trace-export.ps1 -Command TimerStatistics`, 스레드 `GameThread`, 구간 25\~75초, 저장소 밖) |
| 프레임 수 | 5,008(`FEngineLoop::Tick` 횟수). `FEngineLoop::Tick` Incl 49.998327초 |
| 식 | 타이머의 합(초) × 1000 ÷ 5,008 = 프레임당 ms. 모두 게임 스레드, 구간 안 평균이다. 화면에서 고른 프레임 하나가 아니다 |
| 주의 | 직렬 실행이라 게임 스레드 값이 병렬 측정(6.24ms)과 다르다. 본문과 이 표는 "직렬 트레이스"로만 적고 측정값과 견주지 않는다 |

| 본문 값 | 정밀한 값 | 타이머 | 합(초) |
| --- | --- | --- | --- |
| 프레임 평균 9.98ms | 9.9837 | `FEngineLoop::Tick` Incl | 49.998327 |
| Mass 처리 단계 3.11ms | 3.1119 | `FMassProcessingPhase::ExecuteTick EMassProcessingPhase::PrePhysics` Incl(횟수 5,008) | 15.584146 |
| `EndVisualChanges` 2.48ms | 2.4801 | `MassVisualizationComponent EndVisualChanges` Incl = Excl | 12.420331 |
| 가장 긴 Processor 0.950ms | 0.9496 | `MassSmoothOrientationProcessor_2147482251` Incl = Excl | 4.755656 |
| 그다음 0.443ms | 0.4435 | `MassSteerToMoveTargetProcessor_2147482252` Incl = Excl | 2.220999 |
| `UMassUpdateISMProcessor` 0.576ms | 0.5755 | `MassUpdateISMProcessor_2147482256` Incl = Excl | 2.882257 |
| `UCharacterMovementComponent_TickComponent` 없음 | 표에 행 없음(`Character`로 찾은 행은 `FActorComponentTickFunction::ExecuteTick`뿐) | 304행 전체 | |

참고(본문에 쓰지 않았다): `Sync_RenderingThread` 2.7107ms(13.575097초), `MassApplyMovementProcessor` 0.1594, `LabApproachProcessor` 0.1339, `MassApplyForceProcessor` 0.1149ms. 타이머 이름 끝의 숫자는 Processor 인스턴스 번호다.

## 참고 측정: 매니저 방식

본문 "참고", "차이의 출처", "그래서 왜 Mass인가" 절의 근거다. 총괄 묶음이 아니라 참고 묶음이라, 값은 이 절 안에서만 견준다. 경위는 [작업 기록](../../Docs/Worklog/Q8.md)의 "manager 변형: 코드", "측정: Mgr1", "트레이스: MgrT1"에 있다.

| 항목 | 값 |
| --- | --- |
| 묶음 | 참고 묶음 `Mgr1`(2026-11-13 23:15\~23:41). 18회 가운데 실패는 0회다 |
| 빌드 | Test 패키지 28346dc, CSV 메타데이터 `labbuild=28346dc710e8`. 최종 측정(df41c13)과 다른 빌드다 |
| 조건 | 최종 측정과 같은 설정(2560x1440 Epic, 시드 20261106, 2종, 워밍업 15초 + 측정 60초). 오브 시점(P1), 호드 총수 1만과 2만, 세 회차 |
| 매니저 방식 | `-LabStage=M1 -LabVariant=manager`. 구현은 `Source/HordeOptLab/LabCrowdManager.h`·`.cpp`의 `ALabManagedMinion`, `ALabCrowdManager`(83ffcbd) |
| 2단계 | `-LabStage=M1 -LabVariant=mass`(`mass.FullyParallel=true`) |
| 한 스레드 2단계 | 2단계에 `-LabFullyParallel=0`(`mass.FullyParallel=false`) |
| 실행 라벨 | `Mgr1-L2-manager-N<N>-P1-r<i>`, `Mgr1-L2-mass-N<N>-P1-r<i>`, `Mgr1-L2-massserial-N<N>-P1-r<i>`. 버린 첫 실행 `Mgr1-L2-manager-N10000-P1-r0`이 `runs.csv`에 있다 |
| 명령 | `powershell -ExecutionPolicy Bypass -File Scripts/run-mgr.ps1 -Batch Mgr1`, `py -3 -I Scripts/summarize.py --batch Mgr1` |
| 요약 파일 | [참고 측정의 요약 표](../../Measurements/Mgr1/summary.csv), [실행별 값](../../Measurements/Mgr1/runs.csv) |
| 차이 판정 | [measurement.md](../../Docs/Guides/measurement.md) "차이 판정": 중앙값의 차이가 두 조건의 범위 가운데 큰 쪽보다 클 때만 차이로 적는다 |

### 참고 측정의 값

`summary.csv`의 행은 `variant`(`L2-manager`, `L2-mass`, `L2-massserial`)와 `n`으로 찾는다. 단위는 ms다. 병목(`bottleneck`)은 여섯 행 모두 `gpu`다.

| 본문 값 | 정밀한 값 | 조건과 열 | 세 회차(r1 / r2 / r3) |
| --- | --- | --- | --- |
| 게임 스레드 5.27 | 5.265 | 매니저 방식 1만 `game_p50_ms` | 5.265 / 5.22 / 5.311 |
| 게임 스레드 6.22 | 6.22 | 2단계 1만 | 6.342 / 6.22 / 6.1 |
| (그림) 게임 스레드 6.46 | 6.46 | 한 스레드 2단계 1만 | 6.579 / 6.347 / 6.46 |
| 게임 스레드 9.89 | 9.893 | 매니저 방식 2만 | 9.978 / 9.877 / 9.893 |
| 게임 스레드 12.2 | 12.178 | 2단계 2만 | 12.178 / 12.077 / 12.233 |
| (그림) 게임 스레드 12.9 | 12.869 | 한 스레드 2단계 2만 | 12.964 / 12.869 / 11.828 |
| (그림) 프레임 시간 10.1 | 10.139 | 매니저 방식 1만 `frame_p50_ms` | 10.139 / 10.036 / 10.511 |
| (그림) 프레임 시간 9.89 | 9.891 | 2단계 1만 | 10.024 / 9.891 / 9.762 |
| (그림) 프레임 시간 10 | 9.995 | 한 스레드 2단계 1만 | 9.995 / 10.024 / 9.919 |
| (그림) 프레임 시간 15 | 14.98 | 매니저 방식 2만 | 14.98 / 15.01 / 14.935 |
| (그림) 프레임 시간 15.1 | 15.094 | 2단계 2만 | 14.488 / 15.094 / 15.243 |
| (그림) 프레임 시간 14.8 | 14.828 | 한 스레드 2단계 2만 | 14.411 / 14.828 / 15.199 |
| "세 조건 모두 GPU가 가장 느리다" | GPU 9.88 / 9.608 / 9.712, 14.683 / 14.808 / 14.498 | 매니저 방식 / 2단계 / 한 스레드, 1만과 2만 `gpu_p50_ms` | `bottleneck` = `gpu` |
| 초당 사망 62(1만) | 61.999 / 62 / 62.024 | 매니저 방식 / 2단계 / 한 스레드 `death_rate` | |
| 초당 사망 123(2만) | 123.324 / 123.328 / 123.332 | 같음 | |

판정에 쓴 계산이다. 범위는 세 회차의 최댓값 − 최솟값이다.

| 본문 문장 | 중앙값의 차이 | 두 조건의 범위 | 판정 |
| --- | --- | --- | --- |
| 프레임 시간이 구별되지 않는다(1만) | 매니저 − 2단계 0.248, 한 스레드 − 2단계 0.104 | 매니저 0.475, 2단계 0.262, 한 스레드 0.105 | 차이가 큰 쪽 범위보다 작다 |
| 프레임 시간이 구별되지 않는다(2만) | 매니저 − 2단계 −0.114, 한 스레드 − 2단계 −0.266 | 매니저 0.075, 2단계 0.755, 한 스레드 0.788 | 같음 |
| 게임 스레드는 매니저 방식이 짧다(1만) | −0.955 | 매니저 0.091, 2단계 0.242 | 차이가 크다 |
| 게임 스레드는 매니저 방식이 짧다(2만) | −2.285 | 매니저 0.101, 2단계 0.156 | 차이가 크다 |
| 한 스레드로 돌려도 구별되지 않는다(1만) | 한 스레드 − 2단계 0.24 | 2단계 0.242, 한 스레드 0.232 | 차이가 큰 쪽 범위보다 작다 |
| 한 스레드로 돌려도 구별되지 않는다(2만) | 0.691 | 2단계 0.156, 한 스레드 1.136 | 같음 |

- 1만의 프레임 시간은 매니저 방식의 세 회차(10.036\~10.511)가 2단계의 세 회차(9.762\~10.024)와 겹치지 않는다. 위 판정 규칙으로는 차이가 아니다. 매니저 방식의 GPU 시간도 같은 만큼 길다(9.88 대 9.608).
- "매니저 방식의 게임 스레드 시간은 2단계와 같은 자릿수다": 1만에서 5.265 대 6.22ms다. 개체당으로는 0.527 대 0.622µs다. 1단계의 429ms는 최종 측정의 값이라 묶음이 다르다. 본문은 두 묶음의 값을 한 문장에 나란히 적지 않았다.
- "1단계의 게임 스레드 시간을 줄인 것은 개체마다 도는 틱과 컴포넌트를 없앤 일": 매니저 방식이 Mass 없이 위 값을 낸다는 측정에서 판단했다. 매니저 방식과 1단계를 같은 묶음에서 재지 않았다.
- 매니저의 틱 시간(CSV 열 `Lab/GameThread/Manager`, 회차별 중앙값)은 1만 4.05 / 4.01 / 3.99, 2만 8.39 / 8.34 / 8.33ms다. `summary.csv`에 없는 열이라 본문에 쓰지 않았다.
- 렌더 스레드 시간(1만 5.348 / 6.347 / 6.125, 2만 8.77 / 11.272 / 10.937)은 매니저 방식이 짧다. 원인을 확인하지 못해 본문에 쓰지 않았다.

### 참고 트레이스의 값

같은 빌드(28346dc)에서 트레이스를 켜고 조건마다 한 번 돌린 실행이다. 회차가 하나라 범위가 없다. 트레이스 실행의 프레임 수치는 위 참고 측정의 값과 견주지 않는다.

| 항목 | 값 |
| --- | --- |
| 트레이스 | `Saved/Traces/MgrT1-L2-mass-N20000-P1-r1.utrace`, `MgrT1-L2-massserial-N20000-P1-r1.utrace`, `MgrT1-L2-manager-N20000-P1-r1.utrace`(저장소 밖) |
| 실행 | `powershell -ExecutionPolicy Bypass -File Scripts/run-mgr.ps1 -Batch MgrT1 -Runs 1 -SkipR0 -Trace` |
| 내보내기 | `powershell -ExecutionPolicy Bypass -File Scripts/trace-export.ps1 -Label MgrT1-L2-mass-N20000-P1-r1 -Command TimerStatistics -Name MgrT1-mass-20000-GT -Extra "-threads=GameThread -startTime=20 -endTime=70 -columns=*"`. 조건마다 같다 |
| 표 | `py -3 -I Scripts/mgr-trace-table.py`가 [타이머 표](../../Measurements/MgrT1/timers.csv)와 [프레임 수](../../Measurements/MgrT1/frames.csv)를 만든다 |
| 구간과 프레임 수 | 20\~70초. `FEngineLoop::Tick` 횟수는 2단계 3,394, 한 스레드 2단계 3,391, 매니저 방식 3,477이다 |
| 식 | 타이머의 합(초) × 1000 ÷ 프레임 수 = 프레임당 ms. `timers.csv`의 `thread` = `GT`, 열 `incl_ms`다 |

값은 호드 총수 2만, 게임 스레드, 프레임당 ms(Incl)다. 값이 둘이면 2단계 / 한 스레드 2단계다.

| 본문 값 | 정밀한 값 | 타이머 또는 식 |
| --- | --- | --- |
| 처리 단계 둘의 합 11 / 11.2 | 11.0355 / 11.2237 | `...EMassProcessingPhase::PrePhysics`(5.5924 / 5.9046) + `...PostPhysics`(5.4431 / 5.3191) |
| 표현 쪽 Processor 넷 5.45 / 2.13 | 5.4494 / 2.1289 | 아래 넷의 합 |
| 표현 LOD 계산 2.57 / 0.226 | 2.5659 / 0.2259 | `MassVisualizationLODProcessor`(Excl은 0.0029 / 0.0011) |
| (합의 항) ISM 위치 쌓기 | 1.6589 / 1.1195 | `MassUpdateISMProcessor` |
| (합의 항) 카메라 거리 수집 | 0.6354 / 0.4423 | `MassLODCollectorProcessor` |
| (합의 항) 표현 전환 | 0.5892 / 0.3412 | `MassVisualizationProcessor` |
| `EndVisualChanges` 5.44 / 5.31 | 5.44 / 5.3142 | `MassVisualizationComponent EndVisualChanges` |
| 로직 Processor 3.58(한 스레드) | 3.5774 | 회전 1.8761 + 조향 0.8515 + 이동 0.2675 + 힘 0.2341 + 접근 0.19 + 범위 피해 0.1025 + 30m 집계 0.0557 |
| (그림) 회색 칸 | 0.1461 / 0.2032 | 처리 단계 둘의 합 − 위 항들 |
| 한 스레드의 표현 쪽 합 7.44 | 7.4431 | 2.1289 + 5.3142 |
| 매니저의 틱 8.40 | 8.3984 | 매니저 방식의 `LabCrowdManager Tick`(Excl 8.3961) |
| 차이 2.64 | −2.6371 | 8.3984 − 11.0355 |
| 엔진 틱 전체가 준 2.90 | −2.9016 | `Tick_Engine` 10.2394(매니저 방식) − 13.141(2단계) |
| 90.9% | 0.9088 | 2.6371 ÷ 2.9016 |

- "로직 Processor는 워커 스레드에서 돈다": 2단계의 게임 스레드 표에 회전·조향·이동·힘·접근·범위 피해 타이머가 없다(프레임당 0.02ms 미만은 표에서 빠진다). 워커 표(`thread` = `WK`)에 있다.
- 로직 Processor 3.58은 `timers.csv`에 남은 일곱 타이머의 합이다. 작업 기록의 3.59는 0.02ms 미만이라 표에서 빠진 상태 타이머와 저격을 더한 값이다. 작업 기록의 5.46은 반올림한 네 값의 합이다.
- "처리 단계의 절반 이상은 표현 쪽이다": 2단계 (5.4494 + 5.44) ÷ 11.0355 = 98.7%, 한 스레드 7.4431 ÷ 11.2237 = 66.3%다.
- 1만에서도 방향이 같다. 매니저의 틱 3.9953 − 처리 단계 둘의 합 5.2278 = −1.2325이고 `Tick_Engine` 차이 −1.2981의 94.9%다. 본문에는 2만만 썼다.
- 한 스레드와 병렬의 `Tick_Engine`은 13.2678 대 13.141로 0.1268 차이다.
- 참고 측정의 게임 스레드 차이(2만 2.285)와 이 트레이스의 `Tick_Engine` 차이(2.9016)는 크기가 다르다. 트레이스는 구성을 읽는 데만 썼다.
- 뒤 단계가 표현 쪽 경로를 쓴다는 문장의 근거는 [code-tour.md](../../Docs/Guides/code-tour.md) "거리 판정과 표현 전환", "액터 하이드레이션", "ISKM 갱신"과 Fragment 표의 `FMassViewerInfoFragment` 행이다.

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
| 소프트웨어 Lumen 장면은 동적 간접광에 영향을 주고 거리 필드가 있는 프리미티브만 넣는다 | engine-notes.md 11절. 2026-11-14에 다시 열었다: `Source/Runtime/Engine/Private/PrimitiveSceneProxy.cpp:1703`(`bAffectsLumen = AffectsDynamicIndirectLighting()`), `:1724-1732`(`AffectsDistanceFieldLighting()`와 `SupportsDistanceFieldRepresentation()`), `:1734`(`bVisibleInLumenScene = bAffectsLumen && bCanBeTraced`) |
| 스켈레탈 메시에는 거리 필드 표현이 없고 정적 메시·Nanite·스플라인 프록시만 켠다 | engine-notes.md 11절(`StaticMeshSceneProxy.cpp:340`, `Rendering/NaniteResources.cpp:1035`). 이번에 다시 열지 않았다 |
| 인스턴스가 바뀐 프리미티브는 Lumen 장면의 갱신 목록에 들고, Lumen은 인스턴스마다 카드의 위치와 경계를 다시 맞춘다 | `Source/Runtime/Renderer/Private/RendererScene.cpp:6359-6363`(인스턴스 갱신 경로의 `LumenUpdatePrimitive`), `:6278-6279`(변환 갱신 경로), `Renderer/Private/Lumen/LumenScene.cpp:926-943`(`FScene::LumenUpdatePrimitive`가 `PendingUpdateOperations`에 넣는다), `:1422-1481`(`UpdateLumenScenePrimitives`의 `UpdateLumenPrimitives`가 프리미티브 그룹마다 `UpdateMeshCards`, `UpdatePrimitiveGroupCullingInfo`). 본문의 "표면 정보"는 카드다 |
| `bAffectDynamicIndirectLighting`의 뜻 "Controls whether the primitive should influence indirect lighting" | `Source/Runtime/Engine/Classes/Components/PrimitiveComponent.h:554-556` |
| Chunk 크기의 기본값 `ChunkMemorySize = 128 * 1024`. 프로젝트 `Config/`는 이 값을 바꾸지 않는다 | `Source/Runtime/MassEntity/Public/MassEntitySettings.h:92`(`UMassEntitySettings`), `Private/MassArchetypeData.cpp:227` |
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
| `LabExcludeFromLumen` | 4 | `LabMinion.h:39-42` 그대로. 위의 문서 주석(`:32-38`)은 뺐다 |
| `ULabCrowdSubsystem::BuildEntityConfig` 표현 | 12 | `LabCrowdSubsystem.cpp:81-83`, `// ...`(`:84-85` 회전·배율), `:86`, `:87-93`. `:86`, `:90`, `:91`의 줄 끝 주석을 뺐다 |

## 경위의 근거

- "미니언 ISM을 Lumen 장면에 넣은 채로 돌린 시험 실행에서는 GPU 시간이 크게 늘었다"(Lumen): 옛 시험 실행이고 최종 측정에 대응 조건이 없어 수치를 쓰지 않았다. 근거는 [engine-notes.md](../../Docs/Reference/engine-notes.md) 11절 "움직이는 ISM과 Lumen" 행과 [measurement.md](../../Docs/Guides/measurement.md) "확정값"의 "미니언과 Lumen, 그림자" 행(2천, Test 패키지 시험, 원본 `Saved/Profiling/CSV/trial/`은 저장소 밖)이다. 어느 Lumen 패스가 시간을 썼는지는 확인하지 않았다.
- 글 재료 가운데 쓰지 않은 것: 옛 묶음의 "17배"·"25배"와 그 분해(애니 없는 액터, CharacterMovement를 끈 액터의 변형은 최종 측정에 없다), 병렬 처리 비교(최종 측정에 직렬 조건이 없다), 개발 빌드에서 이동 상한 때문에 처치가 줄었던 관찰(개발 빌드 값이다. 본문은 엔진 소스와 최종 측정의 사망 수만 썼다), 옛 묶음 2만의 회차 간 흔들림(최종 측정의 2단계 2만 프레임 p50 범위는 15.34\~16.07ms로 0.73ms다). 근거와 판단은 `Docs/post-review.md` "02 Mass 코어"(글을 다 쓰면 지우는 문서라, 지운 뒤에는 git 이력에서 본다)와 `Docs/post-material.md` "02 Mass 코어"다.
- 2단계의 비교 상대: 옛 계획은 애니 없는 정적 메시 액터 변형과 견줘 변수를 하나로 두었다(`Docs/plan.md` "M1"). 최종 측정의 사다리는 1단계(액터 군중) 다음이 2단계라 로직과 표현이 함께 바뀐다. 본문은 이것을 "변수 둘"로 적었다.

## 확인하지 않은 것

- 로직과 표현 가운데 어느 쪽이 얼마를 줄였는지. 최종 측정에 둘을 따로 바꾼 조건이 없다.
- GPU 50.2ms가 어느 패스에서 나는지와 2단계에서 무엇이 줄었는지. 프레임 CSV에는 패스별 값이 없고, 트레이스에서는 게임 스레드 값만 뽑았다(GPU 패스는 보지 않았다). 본문은 드로우 콜이 개체 수를 따라간다는 측정만 적고 GPU 시간의 원인으로 적지 않았다.
- "1단계는 액터마다 메시 컴포넌트를 따로 그린다"는 드로우 콜 ÷ 호드 총수가 1.03\~1.15인 측정과 스켈레탈 메시 컴포넌트의 구조로 판단했다. 드로우 콜의 내역(그림자, 속도 패스)은 세지 않았다.
- 게임 스레드 6.24ms의 내역. 본문은 남는 일의 종류(Processor 순회, ISM 위치 올리기)만 적었다. 내역은 위 "트레이스 값"에 있다(직렬 트레이스라 측정값 6.24ms와 견주지 않는다).
- 병렬 처리가 번 몫. 최종 측정은 모두 `mass.FullyParallel=true`다. 참고 측정에서는 한 스레드로 돌린 조건과 게임 스레드 시간이 구별되지 않았다.
- 이동 상한(0.1초)이 2단계에서 실제로 걸리지 않는다는 것은 프레임 p99와 50ms 넘는 프레임 수로 판단했다. 엔티티의 이동 거리를 직접 세지 않았다.
- 화면: 1·2단계 WebP를 재생해 보지 않았다(브라우저를 띄우지 않는다). "걷는 동작 없이 한 자세로 움직인다", "피격·사망 때 멈춰 섰다가 사라진다"는 코드(`UMassUpdateISMProcessor`는 위치만 쓰고, 피격·사망은 이동 태그를 뗀다)에서 판단했다.
- `make-figures.py`의 SVG 넷을 화면에서 보지 않았다. 글자와 값만 텍스트로 대조했다.
- 매니저 방식: 매니저의 틱 안(로직, ISM 위치 쓰기)을 나눈 타이머가 없다. ISM 위치를 쓰는 데 드는 시간은 확인하지 못했다.
- 병렬 2단계에서 `MassVisualizationLODProcessor`의 Incl 2.57ms 가운데 워커 스레드를 기다린 시간이 있는지. Excl은 0.003ms이고 한 스레드 실행의 Incl은 0.226ms다. 표에는 타이머의 위아래 관계가 없다.
- 참고 트레이스는 조건마다 한 번이다. 차이의 크기가 회차 사이 흔들림보다 큰지 알 수 없다.
- 매니저 방식의 렌더 스레드 시간이 짧은 원인, 1만에서 GPU 시간이 0.27ms 긴 원인.
- 매니저 방식으로 표현 LOD, 하이드레이션, Simulation LOD를 만들었을 때의 비용. 만들지 않았다.
- GPU가 병목이 아닌 조건(낮은 해상도)에서 게임 스레드의 차이가 프레임 시간에 드러나는지.

## Insights 화면

본문의 Insights 스크린샷이다. 2026-11-13에 아래 트레이스로 다시 찍었다. 값의 계산은 위 트레이스 표가 근거이고, 화면은 같은 트레이스에서 같은 타이머의 구간 합계를 보인다.

| 항목 | 내용 |
| --- | --- |
| 파일 | `images/insights-mass-processors.jpg` |
| 트레이스 | `Full3traceser-L2-mass-N10000-P1-r1`(직렬) |
| 거른 말 | Timers 목록 `Mass`, CPU 타이머만 |
| 표시한 행(Count, Incl) | PrePhysics 5,009 / 15.58s, `MassVisualizationComponent EndVisualChanges` 5,008 / 12.42s, `MassSmoothOrientationProcessor` 4.76s, `MassUpdateISMProcessor` 2.88s, `MassSteerToMoveTargetProcessor` 2.22s |
| 화면에 없는 값 | 프레임 평균 9.98ms(`FEngineLoop::Tick`)는 거른 목록에 없다 |
| 구간 | Timing 뷰의 시간 눈금자를 끌어 25초부터 75초까지(길이 표시 50s)를 골랐다. 손으로 고른 구간이라 내보내기의 창과 프레임이 한두 개 다를 수 있다. 화면의 합계는 표시 자릿수까지 내보낸 값과 같다. PrePhysics의 Count만 화면이 하나 많다(5,009 대 내보내기 5,008) |
| 가공 | 창을 최대화하고 Timing 뷰를 한 칸 축소한 뒤, 창 캡처(PrintWindow)로 화면 전체를 한 번에 찍어 2304폭으로 줄였다. 트레이스 파일 이름이 보이는 제목 줄은 잘라 냈다. 빨간 상자와 단계 이름을 그렸다. 두 트레이스를 견주는 그림은 위아래로 붙였다 |

### 매니저 방식 트레이스의 화면

본문 "차이의 출처" 절의 스크린샷이다. 코디네이터가 2026-11-14에 찍었다. 값의 계산은 위 "참고 트레이스의 값"이 근거이고, 화면은 같은 트레이스에서 같은 타이머의 구간 합계를 보인다.

| 항목 | 내용 |
| --- | --- |
| 파일 | `images/insights-manager-gamethread.jpg`(두 트레이스를 위아래로 붙인 한 장, 2304폭) |
| 위 | `MgrT1-L2-mass-N20000-P1-r1`, Timers 목록 `Mass`, CPU 타이머만. 고른 구간은 49.64초(19.94\~69.58초)다 |
| 위의 표시한 행(Count, Incl) | PrePhysics 3,370 / 18.84s, `MassVisualizationComponent EndVisualChanges` 3,370 / 18.31s, `MassVisualizationLODProcessor` 3,370 / 8.65s(Excl 9.73ms), `MassUpdateISMProcessor` 3,369 / 5.59s |
| 아래 | `MgrT1-L2-manager-N20000-P1-r1`, Timers 목록 `LabCrowdManager`. 고른 구간은 50.15초다 |
| 아래의 표시한 행(Count, Incl) | `LabCrowdManager Tick` 3,487 / 29.28s(Excl 29.28s) |
| 구간 | 시간 눈금자를 손으로 끌어 골랐다. 내보내기의 창(20\~70초)과 프레임 수가 다르다(2단계 3,370 대 3,394, 매니저 방식 3,487 대 3,477) |
| 프레임당 값 | 화면 값으로 계산해도 내보내기 값과 같다. 18.84 ÷ 3,370 = 5.59ms, 18.31 ÷ 3,370 = 5.43ms, 8.65 ÷ 3,370 = 2.57ms, 29.28 ÷ 3,487 = 8.40ms |
| 화면에 없는 값 | `Tick_Engine`, 한 스레드 2단계의 값은 화면에 없다. 내보낸 표가 근거다 |
| 가공 | 다른 글의 화면과 같다. 제목 줄을 잘라 내고 빨간 상자와 이름("2단계", "매니저")을 그렸다 |

## 측정 기록의 변경

- 2026-11-14: "바꾼 것"의 Lumen 문장을 두 문단과 `LabExcludeFromLumen` 발췌로 늘렸다(글 재료 "움직이는 ISM과 Lumen"). 새 수치는 없다. 엔진 사실은 위 "엔진 소스"의 Lumen 행 넷이다. 디스턴스 필드의 정의(가장 가까운 표면까지의 거리를 적은 3D 데이터)는 개념 설명이고 엔진 소스로 확인하지 않았다. 시험 실행이 늘린 GPU 시간이 어느 Lumen 패스에서 났는지는 여전히 확인하지 않았다. 본문은 Lumen 장면 갱신의 경로와 시험 실행의 방향을 따로 적고 원인으로 잇지 않았다.
- 2026-11-14: 배경 지식 "Mass Entity와 Fragment"에 Mass의 장단점을 더했다(사용자 지시). 캐시 설명은 원리이고 측정값이 아니다. 캐시 라인 64바이트는 x86-64 CPU의 일반 사실이며 이 PC에서 확인하지 않았다. 캐시 미스는 재지 않았다.
- 2026-11-14: 본문의 매니저 절 셋을 "참고" 절 하나로 합치고 매니저 방식의 설명을 앞에 두었다. 한 스레드 2단계는 본문과 그림 둘(`manager-times`, `manager-gamethread`)에서 뺐다. 그 값은 이 기록의 표에 남긴다. 표에서 "(그림)"이라 적은 한 스레드 행은 이제 그림에 없다. 측정값은 바뀌지 않았다.
- 2026-11-14: 매니저 방식 트레이스의 Insights 화면을 넣었다(코디네이터). 본문 캡션은 화면 값이다.
- 2026-11-14: 참고 측정(매니저 방식)을 더했다. 사용자가 Mass가 번 몫과 액터의 틱을 없애서 번 몫을 가르는 측정을 정했기 때문이다. 최종 측정의 값은 그대로다. 본문 "한 문장으로"의 첫 문단과 셋째 글머리표를 참고 측정에 맞춰 고쳤다. 그림 넷(`mass-gain`, `manager-diagram`, `manager-times`, `manager-gamethread`)을 더했다.

- 2026-11-13: 트레이스 값을 Full3 트레이스(df41c13)에서 다시 뽑았다. 직렬 `Full3traceser-L2-mass-N10000-P1-r1`에서 프레임 평균 9.92 → 9.98ms, PrePhysics 3.01 → 3.11, `EndVisualChanges` 2.52 → 2.48, SmoothOrientation 0.925 → 0.950, SteerToMoveTarget 0.440 → 0.443, UpdateISM 0.573 → 0.576ms다. 스크린샷 캡션의 구간 합계도 새 내보내기 값으로 바꿨다.
- 2026-11-13: Insights 스크린샷을 Full3 트레이스로 다시 찍었다(코디네이터). "표시한 행"은 새 화면의 값이다.

- 2026-11-13: 측정값을 Test 패키지 08e45f2의 측정에서 df41c13의 최종 측정으로 바꿨다. 애니메이션 예산 기본값을 2ms에서 4ms로 올리고(ADR-0016) 오브의 저격 시점 경합을 고쳐(62b6a42) 전 조건을 다시 쟀기 때문이다. 바뀐 값은 이 글의 모든 측정 수치(2단계 프레임 시간 10.3 → 10.2ms, 최대 개체 수 20,753 → 19,554 등)이고, 트레이스 값과 스크린샷은 그대로다.
