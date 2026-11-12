# 03 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-12 20:34\~11-13 00:39). 경위는 [작업 기록의 재측정 절](../../Docs/Worklog/Q6.md#full2-재측정2026-11-12) |
| 빌드 | Test 패키지 08e45f2, CSV 메타데이터 `labbuild=08e45f24a5e5` |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1), 참고로 탑뷰(P2). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L2-mass`(`-LabStage=M1 -LabVariant=mass`), `L3-vatcull`(`-LabStage=M2 -LabVariant=vatcull`). 인자와 기대 메타데이터는 `Scripts/full-conditions.json`. `L3`의 기대값은 `LabTechFarCull=1`, 나머지 기법 0 |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full2/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full2/runs.csv)(회차별 한 행) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full2/full-n-at-60fps.csv)(`Scripts/plot.py`가 측정 N 사이를 로그 보간). `3 L3-vatcull`은 `n_p95` 30000, `note_p95` = `above range` |
| 트레이스 | 아래 "트레이스". 최종 측정 빌드(Test 패키지 08e45f2)에서 뜬 1만 P1 트레이스 한 회씩이다 |

3단계의 구현(2단계와 다른 곳):

- 표현 LOD 구간: `ULabCrowdSubsystem::BuildEntityConfig`(`Source/HordeOptLab/LabCrowdSubsystem.cpp:119-137`). 조건 `Variant == ELabVariant::VatCull`이 이 블록에 들어간다. `Bands` = {0, `HydrateCm`, 8000, `RepOffCm()`} = {0, 3000, 8000, 20000}cm(`HydrateCm` 기본 30m, `RepOffCm()` = `FarCm()` 기본 200m, `-LabFarCull=0`이 아니므로). Off의 표현은 `None`(`:137`). High·Medium·Low의 표현은 `:89`에서 넣은 `StaticMeshInstance` 그대로다(`bM3`가 거짓이라 `:99-118` 액터 블록을 건너뛴다). 2단계는 `:119`의 조건이 거짓이라 모든 구간이 `:90-91`의 0 / 1e7cm 그대로다(모두 High, 그리지 않는 구간 없음). 변형 이름 `vatcull`·`m2off`는 같은 값(`LabConfig.cpp:85`), `CullsFar()`가 참(`LabConfig.cpp:345`).
- 메시: 스포너가 M2 이상에서 VAT 메시와 데이터 에셋을 읽는다(`LabCrowdSpawner.cpp:136-157`, 경로는 `Config/DefaultGame.ini` `[LabUnitAssets]`, 모드 `Bone`). 2단계는 `Scripts/make-static-minion.py`의 정적 메시다.
- 프로세서: `FLabGameModule::StartupModule`(`Source/HordeOptLab/HordeOptLab.cpp:34,38-39`)이 `Stage >= M2`면 `UMassUpdateISMProcessor`의 자동 등록을 끄고 `ULabVatISMProcessor`를 켠다.
- 같은 것: 그림자 켬(`LabCrowdSubsystem.cpp:86`), Lumen 장면 제외(`ExcludeVisualsFromLumen`, `:404-446`, 모든 Mass 단계), 로직 프로세서와 시나리오. 그림자 캐시 무효화는 기본값이다(`bWantAlways = false`, `:427`).

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. "(그림)"은 본문 산문이 아니라 그림에만 쓴 값이다. 세 회차 열은 r1 / r2 / r3이다.

### 1만 P1의 네 값(요약 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 2단계 프레임 시간 10.3ms | 10.307 | `L2-mass` 10000 P1 | `frame_p50_ms` | 10.185 / 10.439 / 10.307 |
| 2단계 게임 스레드 6.30ms | 6.303 | 같음 | `game_p50_ms` | 6.332 / 6.303 / 6.274 |
| 2단계 렌더 스레드 6.48ms | 6.481 | 같음 | `render_p50_ms` | 6.353 / 6.482 / 6.481 |
| 2단계 GPU 10.0ms | 10.021 | 같음 | `gpu_p50_ms` | 9.91 / 10.151 / 10.021 |
| 3단계 프레임 시간 7.91ms | 7.914 | `L3-vatcull` 10000 P1 | `frame_p50_ms` | 7.833 / 7.956 / 7.914 |
| 3단계 게임 스레드 3.21ms(그림) | 3.206 | 같음 | `game_p50_ms` | 4.14 / 3.206 / 3.136 |
| 3단계 렌더 스레드 4.12ms(그림) | 4.121 | 같음 | `render_p50_ms` | 4.136 / 4.121 / 4.089 |
| 3단계 GPU 7.54ms(그림) | 7.535 | 같음 | `gpu_p50_ms` | 7.45 / 7.578 / 7.535 |
| "두 단계 모두 GPU가 가장 느리다" | | 위 두 행 | `bottleneck` = `gpu` | |
| 프레임 시간 −2.39ms | −2.393 | 계산 | 7.914 − 10.307 | |
| 2단계의 23.2% | 23.22% | 계산 | 2.393 ÷ 10.307 | |
| 게임 스레드 −3.10ms | −3.097 | 계산 | 3.206 − 6.303 | |
| GPU −2.49ms | −2.486 | 계산 | 7.535 − 10.021 | |
| 세 변화가 흔들림보다 크다 | 범위: 프레임 0.254 / 0.123, 게임 0.058 / 1.004, GPU 0.241 / 0.128 | 계산 | [measurement.md](../../Docs/Guides/measurement.md) "차이 판정": 변화가 두 범위 중 큰 쪽보다 크다 | |

`L3-vatcull` 10000 P1 r1의 게임 스레드 4.14ms는 다른 두 회차보다 0.93ms 길다. 프레임·GPU는 다른 회차와 같다(병목이 GPU). 중앙값은 3.206이다. 원인은 보지 않았다.

### 인스턴스 수와 거리

| 본문 값 | 정밀한 값 | 조건 | 출처 | 세 회차 |
| --- | --- | --- | --- | --- |
| 3단계가 한 프레임에 올린 인스턴스 평균 3,761 | 3,760.68 | `L3-vatcull` 10000 P1 | `ism_band0` + `ism_band1` + `ism_band2` + `ism_band3` = 585.85 + 1024.74 + 2150.09 + 0 | 합 3,760.72 / 3,761.20 / 3,758.96 |
| 1만의 37.6% | 37.61% | 계산 | 3,760.68 ÷ 10,000 | |
| 2단계는 1만 모두를 인스턴스로 둔다 | | 설정 | 모든 구간 `StaticMeshInstance`, 그리지 않는 구간 없음(위 구현). 2단계는 엔진 프로세서라 `Lab/IsmBand*`를 세지 않는다 | |
| 개체의 60%가 200m 밖, 200m 안 40% | 0.6 / 0.4 | 계산 | [measurement.md](../../Docs/Guides/measurement.md) "확정값" 맵 반경 행: 밀도 ∝ 1/r에서 구간 비율 = 구간 폭 ÷ R = 300 ÷ 500 | |
| 맵 반경 500m | 500 | 확정값 | 같은 표 | |
| 200m에서 화면 높이 0.4% | | 계산 | 같은 표 "200m 밖(Off 밴드)" 행 | |
| 구간 경계 30 / 80 / 200m(그림) | 3000 / 8000 / 20000cm | 코드 | `LabCrowdSubsystem.cpp:125` | |
| 히스테리시스 10% | 10.0 | 엔진 소스 | 아래 "엔진 소스" | |
| 초당 30프레임으로 굽기 | 30.0 | 코드 | `Scripts/bake-vat.py:198` `sample_rate` | |
| 커스텀 데이터 두 개 | `Frame`, `PrevFrame` | 코드 | `FLabVatFrameData`(`LabVatProcessor.h:54-58`) | |
| 칼날 쪽 바운드 204cm 모자람 | 수정 전 Y −72, 수정 후 Y −276(cm, 로컬) | 굽기 로그 | [Measurements/M3/shots/vat-shadow/README.md](../../Measurements/M3/shots/vat-shadow/README.md) "바운드 전후". 측정 시간이 아니라 메시 상자의 값이다 | |

`ism_band0`\~`3`은 VAT 프로세서가 그 프레임에 위치와 커스텀 데이터를 쌓은 개체를 카메라와의 수평 거리 0\~30 / 30\~80 / 80\~200 / 200m 밖으로 센 값의 프레임 평균이다(`LabVatProcessor.cpp:181-183, 212-215`, `Scripts/summarize.py:102`). 엔진 ISM 갱신과 같이 보이는 개체가 있는 청크만 돈다(`AreAnyEntitiesVisibleInChunk`). 37.6%가 기하 예상 40%와 가까워, 이 필터가 건너뛴 청크는 적다고 본다(확인하지 않았다).

### 2단계의 호드 총수별 값(문제 그림)

| 그림 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| GPU 5천 7.23 | 7.228 | `L2-mass` 5000 P1 | `gpu_p50_ms` | 7.359 / 7.098 / 7.228 |
| GPU 1만 10.0 | 10.021 | `L2-mass` 10000 P1 | 같음 | 위 표 |
| GPU 2만 14.6 | 14.59 | `L2-mass` 20000 P1 | 같음 | 14.367 / 14.59 / 14.787 |
| GPU 3만 19.3 | 19.291 | `L2-mass` 30000 P1 | 같음 | 19.581 / 18.867 / 19.291 |
| 렌더 스레드 5천 4.17 | 4.168 | `L2-mass` 5000 P1 | `render_p50_ms` | 4.168 / 4.142 / 4.182 |
| 렌더 스레드 1만 6.48 | 6.481 | 10000 P1 | 같음 | 위 표 |
| 렌더 스레드 2만 11.5 | 11.532 | 20000 P1 | 같음 | 11.285 / 11.599 / 11.532 |
| 렌더 스레드 3만 15.9 | 15.928 | 30000 P1 | 같음 | 15.668 / 15.939 / 15.928 |
| 게임 스레드 5천 3.32 | 3.322 | 5000 P1 | `game_p50_ms` | 3.365 / 3.322 / 3.303 |
| 게임 스레드 1만 6.30 | 6.303 | 10000 P1 | 같음 | 위 표 |
| 게임 스레드 2만 12.4 | 12.35 | 20000 P1 | 같음 | 12.112 / 12.35 / 12.405 |
| 게임 스레드 3만 18.6 | 18.578 | 30000 P1 | 같음 | 18.102 / 18.653 / 18.578 |
| "병목은 모두 GPU" | | 네 행 | `bottleneck` = `gpu` | |

### 프레임 시간의 느린 쪽 5% 경계와 최대 개체 수(결과 그림)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 2단계 5천 8.13 | 8.133 | `L2-mass` 5000 P1 | `frame_p95_ms` | 8.287 / 8.018 / 8.133 |
| (그림) 2단계 1만 10.9 | 10.931 | 10000 P1 | 같음 | 10.86 / 11.098 / 10.931 |
| (그림) 2단계 2만 16.1 | 16.069 | 20000 P1 | 같음 | 16.738 / 15.876 / 16.069 |
| (그림) 2단계 3만 23.0 | 22.989 | 30000 P1 | 같음 | 23.754 / 22.74 / 22.989 |
| (그림) 3단계 5천 6.92 | 6.92 | `L3-vatcull` 5000 P1 | 같음 | 6.951 / 6.904 / 6.92 |
| (그림) 3단계 1만 8.47 | 8.471 | 10000 P1 | 같음 | 8.416 / 8.518 / 8.471 |
| (그림) 3단계 2만 11.7 | 11.681 | 20000 P1 | 같음 | 11.745 / 11.637 / 11.681 |
| 3단계 3만 15.3ms | 15.257 | 30000 P1 | 같음 | 15.687 / 15.257 / 15.168 |
| 2단계 최대 개체 수 20,753 | 20753 | `2 L2-mass` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준은 22,853(`n_p50`) |
| 3단계는 3만에서도 한계에 닿지 않음 | 30000, `above range` | `3 L3-vatcull` | 같은 파일 `n_p95`, `note_p95` | 세 회차 최댓값 15.687도 16.7 안 |
| 3단계 GPU 3만 13.6ms | 13.576 | `L3-vatcull` 30000 P1 | `gpu_p50_ms` | 13.582 / 13.503 / 13.576 |

### 트레이스 값(예상과 실제 그림, 본문)

정밀한 값 = 25\~75초 창의 타이머 합(초) × 1000 ÷ 창 안의 프레임 수(`FEngineLoop::Tick` 횟수)다. 아래 "트레이스"에 파일과 프레임 수가 있다.

| 본문 값 | 정밀한 값(2단계 / 3단계) | 트레이스 | 타이머(열) | 예상 = 2단계 × 0.4 |
| --- | --- | --- | --- | --- |
| 쌓은 위치를 ISM 컴포넌트에 넣는다 2.52 / 1.23 | 2.5152 / 1.2323 | 직렬, 게임 스레드 | `MassVisualizationComponent EndVisualChanges` Incl(12.671333 / 7.901813초) | 1.0061 → 1.01 |
| 디스턴스 필드 장면 2.65 / 1.36 | 2.6467 / 1.3575 | 병렬, 렌더 스레드 | `FSceneRenderer::PrepareDistanceFieldScene` Excl(13.378891 / 8.719137초) | 1.0587 → 1.06 |
| 그림자 그리기 2.51 / 1.66 | 2.5143 / 1.6550 | 병렬, GPU | `ShadowDepths` Excl(12.709799 / 10.630309초) | 1.0057 → 1.01 |
| 색과 재질 1.40 / 0.83 | 1.3981 / 0.8269 | 병렬, GPU | `BasePass` Excl(7.067248 / 5.311380초) | 0.5592 → 0.56 |
| 움직임 벡터 1.43 / 0.68 | 1.4288 / 0.6796 | 병렬, GPU | `RenderVelocities(%s)` Excl(7.222490 / 4.364974초) | 0.5715 → 0.57 |
| VAT 프로세서 0.458ms / 엔진 ISM 프로세서 0.573ms | 0.5729(2단계 `MassUpdateISMProcessor_*`) / 0.4579(3단계 `LabVatISMProcessor_*`) | 직렬, 게임 스레드 | Incl(2.886137 / 2.935854초) | |

"실제가 예상보다 크다"는 다섯 행 모두 3단계 값이 예상 값보다 크다는 비교다(트레이스 한 회라 범위가 없다).

### 탑뷰 카메라(틀렸던 가설)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 2단계 GPU 6.51ms | 6.506 | `L2-mass` 10000 P2 | `gpu_p50_ms` | 6.46 / 6.525 / 6.506 |
| 3단계 GPU 7.20ms | 7.196 | `L3-vatcull` 10000 P2 | `gpu_p50_ms` | 7.142 / 7.211 / 7.196 |
| 변화 +0.690ms | 0.690 | 계산 | 7.196 − 6.506. 범위 0.065 / 0.069보다 크다 | |
| "먼 개체가 화면에 없는 탑뷰" | | 확정값 | [measurement.md](../../Docs/Guides/measurement.md) "저격" 행: P2는 화면에 200m 밖이 없다. P2 높이 100m | |

P2의 다른 값(참고): 프레임 p50 7.252 → 7.544, 게임 스레드 6.428 → 2.673, 병목은 둘 다 `gpu`. P2에서도 두 변수(VAT, 200m 밖 지우기)가 함께 바뀌었다. 본문은 "VAT의 GPU 비용이 있다"의 방향 근거로만 쓰고, 0.690ms를 VAT만의 비용이라고 쓰지 않았다.

## 트레이스

| 쓰인 곳 | 파일(`Saved/Traces/`, 저장소 밖) | 내보낸 표(`%TEMP%\horde-trace\`, 저장소 밖) | 창 안 프레임 수 |
| --- | --- | --- | --- |
| 게임 스레드 값(2단계) | `Full2traceser-L2-mass-N10000-P1-r1.utrace`(직렬) | `F2-traceser-L2-mass-GT.csv` | 5,038 |
| 게임 스레드 값(3단계) | `Full2traceser-L3-vatcull-N10000-P1-r1.utrace`(직렬) | `F2-traceser-L3-vatcull-GT.csv` | 6,412 |
| 렌더 스레드·GPU 값(2단계) | `Full2trace-L2-mass-N10000-P1-r1.utrace`(병렬) | `F2-trace-L2-mass-RT.csv`, `-GPU.csv`, `-GT.csv`(프레임 수) | 5,055 |
| 렌더 스레드·GPU 값(3단계) | `Full2trace-L3-vatcull-N10000-P1-r1.utrace`(병렬) | `F2-trace-L3-vatcull-RT.csv`, `-GPU.csv`, `-GT.csv`(프레임 수) | 6,423 |

내보낸 방법은 `Scripts/trace-export.ps1 -Command TimerStatistics -Extra "-threads=<스레드> -startTime=25 -endTime=75 -columns=*"`다. 표 전체와 단계별 해석은 [작업 기록의 트레이스 16개 절](../../Docs/Worklog/Q6.md#트레이스-16개사다리-여덟-단계-1만-p1-병렬직렬-한-회씩)에 있다. GPU 값은 큐 여럿의 합이라 패스 값을 더해도 GPU 프레임 시간이 되지 않는다. 트레이스를 켠 한 회라 `summary.csv`의 값과 직접 견주지 않는다.

## 측정 기록의 변경

- 2026-11-13(처음 씀): 트레이스 값은 730ff18에서 뜬 트레이스(`Full1trace*`)의 값이다. 최종 측정 빌드의 트레이스(`Full2trace-L2-mass-*`, `Full2trace-L3-vatcull-*`, 직렬 `Full2traceser-*`)는 `Saved/Traces/`에 있지만 내보내지 않았다. 내보내는 `Scripts/trace-export.ps1`이 Unreal Insights를 띄우고, 같은 PC에서 촬영이 도는 중이었다. 두 빌드 사이에 `Source/`와 `Config/`는 같고, 바뀐 것은 `Content/Lab/FX/`의 배경 무리 에셋 셋뿐이다(01번 글의 측정 기록과 같은 확인). 2·3단계는 배경 무리를 쓰지 않는다.
- 2026-11-13(트레이스 값 다시 뽑음): 처음 쓴 트레이스 값은 730ff18에서 뜬 트레이스(`Full1trace*`)의 값이었다. 최종 측정 빌드의 트레이스(`Full2trace-*`, `Full2traceser-*`)에서 같은 구간(25\~75초)·스레드·타이머·식으로 다시 뽑아 위 표와 `make-figures.py`의 상수를 바꿨다. 이전 값은 2.47 / 1.21, 2.61 / 1.35, 2.53 / 1.62, 1.41 / 0.83, 1.46 / 0.68, VAT 프로세서 0.451ms, 엔진 ISM 프로세서 0.569ms다. 병렬 트레이스의 프레임 수는 `FEngineLoop::Tick` 횟수이며 새 내보내기(`F2-trace-L2-mass-GT.csv`, `-L3-vatcull-GT.csv`)에서 읽었다. 같은 PC에서 옛 값을 같은 방법으로 다시 계산하면 옛 표와 일치했다.

## 화면의 HUD와 표의 차이

요약의 두 화면(`step2-L2-mass.webp`, `step3-L3-vatcull.webp`)은 측정 실행이 아니라 촬영 실행(`Pre5shot-L2-mass-N10000-P1-r1`, `Pre5shot-L3-vatcull-N10000-P1-r1`)에서 찍었다. HUD의 프레임 시간(2단계 11.1ms, 3단계 8.9ms)은 4K 영역 화면 녹화 부하가 섞여 표의 값(10.3, 7.91ms)보다 높다. 수치는 표의 값만 쓴다. 촬영 경위는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "촬영 준비"에 있다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/vat-summary.svg` | 요약 그림. 2단계와 3단계의 프레임·게임 스레드·렌더 스레드·GPU 시간 막대(1만 P1, 같은 축척)와 16.7ms 선 | "1만 P1의 네 값" 여덟 행 |
| `images/vat-principle.svg` | 원리 도식. 굽기(에디터) → 게임 스레드(`ULabVatISMProcessor`) → GPU(`MF_BoneAnimation`) 세 칸과, 2단계·3단계의 거리 구간 막대(0\~500m, 경계 30 / 80 / 200m) | "인스턴스 수와 거리" |
| `images/vat-problem.svg` | 문제 그림. 2단계의 GPU·렌더 스레드·게임 스레드 시간 중앙값 대 호드 총수(5천\~3만, 선형 축)와 16.7ms 선 | "2단계의 호드 총수별 값" 열두 행 |
| `images/vat-expected.svg` | 결과 그림. 트레이스 타이머 다섯 개의 2단계 값, 예상(× 0.4, 점선), 3단계 값. 아래 문장에 인스턴스 3,761 | "트레이스 값" 다섯 행, 인스턴스 행 |
| `images/vat-maxn.svg` | 결과 그림. 프레임 시간 느린 쪽 5% 경계 대 호드 총수(2단계 점선, 3단계 실선), 16.7ms 선, 2단계의 최대 개체 수 20,753 | "느린 쪽 5% 경계와 최대 개체 수" |
| `../../Docs/figures/steps/step2-L2-mass.webp`, `step3-L3-vatcull.webp` | 2·3단계 화면(HUD 켬) | 위 "화면의 HUD와 표의 차이" |

SVG 다섯 개는 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/03-ism-vat/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽고, 트레이스 값은 위 표의 타이머 합과 프레임 수를 스크립트의 상수로 둔다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다.

그림 글자는 소수 둘째 자리, 본문은 유효 숫자 세 자리다. 같은 값이다(엔진 ISM 프로세서 그림 0.57, 본문 0.573ms).

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다. 경로는 `Engine/` 아래다.

| 사실 | 위치 |
| --- | --- |
| Bone 모드는 바운드를 뼈 위치 범위(`BoneMinBBox`, `BoneSizeBBox`)로, Vertex 모드는 모든 프레임의 정점 범위(`VertexMinBBox`, `VertexSizeBBox`)로 넓힌다 | `Plugins/Experimental/AnimToTexture/Source/AnimToTextureEditor/Private/AnimToTextureBPLibrary.cpp:290`(Bone), `:245`(Vertex), 둘 다 `UAnimToTextureBPLibrary::AnimationToTexture`(`:33`) 안. 넓히는 함수 `SetBoundsExtensions` `:1108` |
| 표현 LOD 경계의 히스테리시스 기본 10% | `Plugins/Runtime/MassGameplay/Source/MassRepresentation/Public/MassRepresentationFragments.h:309` `BufferHysteresisOnDistancePercentage = 10.0f` |
| 엔진 ISM 갱신 프로세서는 게임 스레드 전용이고 보이는 개체가 있는 청크만 돈다 | `MassRepresentation/Private/MassUpdateISMProcessor.cpp:21` `bRequiresGameThreadExecution = true`, `:30` `SetChunkFilter(&FMassVisualizationChunkFragment::AreAnyEntitiesVisibleInChunk)` |
| 커스텀 데이터는 위치와 같은 순서·개수로 쌓여야 한다(플러시 때 개수 검사) | `MassRepresentation/Private/MassVisualizationComponent.cpp:566-571` `UMassVisualizationComponent::EndVisualChanges`. engine-notes.md 3절 |
| 병렬 Mass의 질의 대기(스핀) | `Source/Runtime/MassEntity/Private/MassProcessingQueueTypes.cpp:678` `WaitForQueryResources`, `Public/MassProcessingQueueTypes.h:760-778` `YieldThread`. engine-notes.md 11절(이 글은 줄을 다시 열지 않았다) |
| 엔진 Mass ISM의 그림자 기본값은 꺼짐 | `MassRepresentation/Public/MassRepresentationTypes.h:105` `bCastShadows = false`. 이 프로젝트는 2단계부터 켠다 |

이 동작(Bone 모드의 바운드)을 다룬 공개 이슈는 찾지 못했다. [references.md](../../Docs/Reference/references.md)의 포럼 목록과 [engine-notes.md](../../Docs/Reference/engine-notes.md) 3절에도 없다. Virtual Shadow Map이 바운드 안쪽만 무효화한다는 설명은 엔진 소스가 아니라 같은 조건의 창 여섯 개를 차례로 띄워 가른 결과다([Measurements/M3/shots/vat-shadow/README.md](../../Measurements/M3/shots/vat-shadow/README.md)). 해결은 `Scripts/bake-vat.py:232-245`(커밋 4f0d3fc)다. 해결 코드가 `Source/HordeOptLab/`가 아니라 굽기 스크립트에 있는 것은 바운드가 굽기 산출물(정적 메시 에셋)의 속성이기 때문이다.

### 본문에 넣은 발췌

엔진 코드는 3줄이다(`AnimToTextureBPLibrary.cpp:245`, `// ...`, `:290`). 남긴 줄은 소스를 그대로 옮겼다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| `BuildEntityConfig` 구간 | 11 | `LabCrowdSubsystem.cpp:125`, `// ...`(`:126` `MaxCounts`), `:127-130`, `// ...`(`:131` `LODMaxCount`), `:132`, `// ...`(`:133-136` `VatCullVis` 분기), `:137` |
| `ULabVatISMProcessor::Execute` | 15 | `LabVatProcessor.cpp:132-133`, `// ...`, `:147-149`(새 구조 분기 안), `// ...`, `:151-153`, `// ...`(`:154-155` 주석과 `Rate`), `:156-157`, `// ...`(`:158-170` 개발 확인 로그와 타이밍), `:171`. 들여쓰기는 줄였다 |
| AnimToTexture 바운드 | 3 | 위 엔진 코드 |
| `bake-vat.py` 바운드 수정 | 9 | `Scripts/bake-vat.py:232-233`, `:234`(줄 끝 주석을 뺐다), `:236`, `# ...`(`:235` 주석, `:237-241` 원래 상자 `base_min`·`base_max` 계산), `:242-245` |

## 경위의 근거

본문 "틀렸던 가설"의 앞 두 문단은 수치를 붙이지 않았다. 근거는 최종 측정과 조건이 다른 옛 묶음이라 비교에 쓰지 않는다.

- 표현만 VAT로 바꾼 실험(오브 시점 GPU 그대로, 게임 스레드 증가, 탑뷰 GPU 증가): M2 묶음 E2 (a), Test 패키지 c867446, 1종, 체력 100. [Worklog/M2.md](../../Docs/Worklog/M2.md) "2026-11-08 E2 측정"과 "STATUS.md에서 옮김". 최종 측정의 사다리에는 VAT만 바꾼 단계가 없다.
- 프로세서 안쪽을 나눠 잰 진단(반복 안 쓰기는 그대로, 늘어난 시간은 첫 청크 전 `WaitForQueryResources`, 직렬이면 사라짐): M3 진단 실행 `diag-vt*`(액터가 있는 단계, 20k P1, `-LabVatTiming`). [Worklog/M3.md](../../Docs/Worklog/M3.md) "2026-11-09 액터 쪽 비용 순위", [engine-notes.md](../../Docs/Reference/engine-notes.md) 11절.
- 본문은 기다림을 줄이려고 프래그먼트를 나눈 일(`FLabAnimStateFragment`, M3b 정비)을 적지 않았다. 그 전후 A/B(`m3qold` 대 정비 뒤, 20k P1)는 프레임 p50 17.01 대 17.77로 범위가 겹쳐 갈리지 않았다([Worklog/M3b.md](../../Docs/Worklog/M3b.md) 표 2). 3단계는 정비 뒤의 쿼리(`LabVatProcessor.cpp:53`)로 쟀다.
- "자동 재생은 클립을 되풀이한다": `Scripts/bake-vat.py:8-9` 주석과 [Worklog/M2.md](../../Docs/Worklog/M2.md) "설계 결정과 VAT 구현". 머티리얼 함수는 에셋이라 소스로 확인하지 않았다.
- 글 재료의 사실 확인: `Docs/post-review.md` "03 ISM + VAT"(글을 다 쓰면 지우는 문서라, 지운 뒤에는 git 이력에서 본다).

## 확인하지 않은 것

- VAT만의 비용과 200m 밖 지우기만의 이득. 최종 측정의 사다리가 둘을 한 단계로 묶었다. 탑뷰의 GPU 증가도 두 변수가 섞인 값이다.
- 예상과 실제의 차이 가운데 VAT가 더한 몫과 인스턴스 수에 비례하지 않는 몫. 트레이스는 단계마다 한 회다.
- 2단계가 실제로 올린 인스턴스 수. 엔진 프로세서라 세지 않았다. 청크 필터가 건너뛴 청크 수도 세지 않았다.
- `L3-vatcull` 10000 P1 r1의 게임 스레드 4.14ms가 다른 회차보다 긴 원인.
- 최종 측정 패키지에 든 VAT 메시의 바운드가 수정 뒤의 값인지. 굽기 산출물은 git 밖이고(`Content/Lab/VAT/Baked/`), `Scripts/bake-vat.py`의 기본 경로가 수정을 넣는다는 것만 확인했다.
- 3단계 화면에서 피격·죽기 동작이 보이는지. 움직이는 WebP를 열어 보지 않았다(브라우저 금지). 본문 이미지 설명은 "걷는다"까지만 적었다.
- 200m 경계에서 개체가 사라지는 모습이 화면에서 티 나는지. 본문은 화면 높이 0.4%(계산)까지만 적었다.
- SVG를 화면에 그려 보지 않았다. 글자 겹침은 좌표와 글자 수로만 확인했다.
