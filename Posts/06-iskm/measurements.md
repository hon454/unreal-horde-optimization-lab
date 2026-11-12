# 06 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-12 20:34\~11-13 00:39). 경위는 [작업 기록의 재측정 절](../../Docs/Worklog/Q6.md#full2-재측정2026-11-12) |
| 빌드 | Test 패키지 08e45f2, CSV 메타데이터 `labbuild=08e45f24a5e5` |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, 모션 블러 엔진 기본(켬), `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1), 참고로 탑뷰(P2). 측정 창 60초, 세 회차 중앙값 |
| 사다리 조건 | `L5-m4offpool`(`-LabStage=M5 -LabVariant=m4offpool`), `L6-m5bounds`(`-LabStage=M5 -LabVariant=m5bounds`). 기대 메타데이터의 차이는 `LabTechIskm` 0 → 1, `LabTechFixedBounds` 0 → 1, `LabIskmBounds` 0 → 2(고정 바운드를 넣은 컴포넌트 수). 나머지 기법과 `LabActorCap` 500, `LabHydrateM` 30, `LabFarShadow` 1은 같다 |
| 빼 보기 조건 | `L8-q4final`(`-LabStage=M5 -LabVariant=q4final`), `A8-iskm`(+ `-LabIskm=0`, `LabTechIskm` 0, `LabIskmBounds` 0), `A8-bounds`(+ `-LabFixedBounds=0`, `LabTechFixedBounds` 0, `LabIskmBounds` 0). 인자와 기대값은 `Scripts/full-conditions.json` |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full2/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full2/runs.csv)(회차별 한 행) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full2/full-n-at-60fps.csv)(`Scripts/plot.py`가 측정 N 사이를 로그 보간) |
| 트레이스 | 본문 "결과"의 트레이스 문단 값은 아래 "트레이스 값"이다. 최종 측정 빌드의 병렬 트레이스 한 회씩이다 |

6단계의 구현(5단계와 다른 곳):

- 판정: `FLabConfig::UsesIskm()`(`Source/HordeOptLab/LabConfig.cpp:324-327`)이 M5 단계이고 변형이 `m4offmove`·`m4offpool`이 아니며 `-LabIskm=0`이 없을 때 참이다. `FLabConfig::IskmParams()`(`:329-336`)는 `BlendTime` 0, `AnimationMinScreenSize` −1, 그림자 켬을 기본으로 두고, 변형 `m5bounds`·`q2crowd`·`q4final`에서 `-LabFixedBounds=0`이 없으면 `bFixedBounds`를 켠다. 기본값은 `LabConfig.h:181-187`.
- 프로세서 등록: `FLabGameModule::StartupModule`(`Source/HordeOptLab/HordeOptLab.cpp:37-40`)이 `UsesIskm()`이면 `ULabVatISMProcessor` 대신 `ULabIskmProcessor`를 켠다. 엔진 `UMassUpdateInstancedSkinnedMeshProcessor`는 `Config/DefaultMass.ini:39`에서 꺼 둔다.
- 표현 설명자: `ULabCrowdSubsystem::BuildEntityConfig`(`Source/HordeOptLab/LabCrowdSubsystem.cpp:179-216`).
- 갱신: `ULabIskmProcessor`(`Source/HordeOptLab/LabIskmProcessor.cpp`). 자리와 조건 `:129-139`(게임 스레드, Representation 그룹, VisualizationProcessing 뒤), 실행 `:158-367`.
- 고정 바운드: `ULabCrowdSubsystem::ApplyFixedIskmBounds`(`LabCrowdSubsystem.cpp:381-402`). 오브 틱(`LabOrb.cpp:244`)이 부르는 `ExcludeVisualsFromLumen`(`LabCrowdSubsystem.cpp:404-446`)이 `:420-426`에서 ISKM 컴포넌트마다 부른다.
- 에셋: `Scripts/create-iskm.py`. 결과는 `/Game/Lab/ISKM/Baked/`(git 밖)이고 실행 로그는 [Worklog/M5.md](../../Docs/Worklog/M5.md) "ISKM 에셋 만들기"에 있다.
- 같은 것: 액터 구간 0\~30m(상한 500), 표현 LOD 거리 경계, 200m 밖 그리지 않음, Simulation LOD와 200m 밖 자체 이동, 엔티티 풀, 그림자 켬, 로직 프로세서, 시나리오.

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. "(그림)"은 본문 산문이 아니라 그림에만 쓴 값이다. 세 회차 열은 r1 / r2 / r3이다. "흔들림보다 크다"는 [measurement.md](../../Docs/Guides/measurement.md) "차이 판정"(변화가 두 조건의 범위 중 큰 쪽보다 크다)이다.

### 1만 P1의 네 값(요약 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 5단계 프레임 시간 7.65ms | 7.654 | `L5-m4offpool` 10000 P1 | `frame_p50_ms` | 7.646 / 7.654 / 7.695 |
| 5단계 게임 스레드 7.33ms | 7.333 | 같음 | `game_p50_ms` | 7.333 / 7.321 / 7.375 |
| 5단계 렌더 스레드 4.60ms(그림) | 4.601 | 같음 | `render_p50_ms` | 4.618 / 4.591 / 4.601 |
| 5단계 GPU 6.97ms | 6.974 | 같음 | `gpu_p50_ms` | 6.96 / 6.983 / 6.974 |
| 6단계 프레임 시간 7.51ms | 7.506 | `L6-m5bounds` 10000 P1 | `frame_p50_ms` | 7.498 / 7.506 / 7.524 |
| 6단계 게임 스레드 7.34ms | 7.344 | 같음 | `game_p50_ms` | 7.335 / 7.344 / 7.35 |
| 6단계 렌더 스레드 3.25ms(그림) | 3.247 | 같음 | `render_p50_ms` | 3.26 / 3.247 / 3.231 |
| 6단계 GPU 5.36ms | 5.356 | 같음 | `gpu_p50_ms` | 5.359 / 5.346 / 5.356 |
| "두 단계 모두 게임 스레드가 가장 느리다" | | 위 두 행 | `bottleneck` = `game` | |
| GPU −1.62ms | −1.618 | 계산 | 5.356 − 6.974. 범위 0.023 / 0.013보다 크다 | |
| GPU 23.2% 줄었다(제목 23%) | 23.20% | 계산 | 1.618 ÷ 6.974 | |
| 프레임 시간 −0.148ms | −0.148 | 계산 | 7.506 − 7.654. 범위 0.049 / 0.026보다 크다 | |
| 게임 스레드 +0.011ms, 흔들림보다 작다 | +0.011 | 계산 | 7.344 − 7.333. 범위 0.054 / 0.015보다 작다 | |
| 예상 상한 0.321ms | 0.321 | 계산 | 5단계 프레임 − 게임 스레드 = 7.654 − 7.333 | |

예상의 근거: 프레임 시간은 게임 스레드 시간 이상이다(측정의 정의, [measurement.md](../../Docs/Guides/measurement.md) "N@16.7ms" 문단의 병목 설명). 6단계 게임 스레드가 5단계와 같다면 프레임 시간은 5단계의 간격(0.321ms)보다 더 줄 수 없다. 6단계의 간격은 0.162ms(7.506 − 7.344)다.

### 5단계의 호드 총수별 값(문제 그림, 본문)

| 그림 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 게임 스레드 5천 4.38 | 4.383 | `L5-m4offpool` 5000 P1 | `game_p50_ms` | 4.34 / 4.415 / 4.383 |
| 게임 스레드 1만 7.33 | 7.333 | 10000 P1 | 같음 | 위 표 |
| 게임 스레드 2만 10.7 | 10.725 | 20000 P1 | 같음 | 10.637 / 10.787 / 10.725 |
| 게임 스레드 3만 13.8 | 13.843 | 30000 P1 | 같음 | 13.529 / 13.843 / 13.941 |
| GPU 5천 5.74 | 5.74 | 5000 P1 | `gpu_p50_ms` | 5.75 / 5.729 / 5.74 |
| GPU 1만 6.97 | 6.974 | 10000 P1 | 같음 | 위 표 |
| GPU 2만 9.67 | 9.667 | 20000 P1 | 같음 | 9.699 / 9.667 / 9.633 |
| GPU 3만 12.6 | 12.582 | 30000 P1 | 같음 | 12.558 / 12.664 / 12.582 |
| 렌더 스레드 5천 3.45 | 3.449 | 5000 P1 | `render_p50_ms` | 3.412 / 3.482 / 3.449 |
| 렌더 스레드 1만 4.60 | 4.601 | 10000 P1 | 같음 | 위 표 |
| 렌더 스레드 2만 6.88 | 6.882 | 20000 P1 | 같음 | 6.854 / 6.882 / 6.916 |
| 렌더 스레드 3만 8.53 | 8.528 | 30000 P1 | 같음 | 8.516 / 8.528 / 8.648 |
| GPU가 0.359ms 뒤 | 0.359 | 계산 | 7.333 − 6.974 | |
| 5천은 GPU가 병목, 1만부터 게임 스레드 | | 네 행 | `bottleneck` = `gpu`(5천), `game`(1만·2만·3만) | |
| 탑뷰 카메라에서 GPU가 병목, GPU 7.17ms | 7.172 | `L5-m4offpool` 10000 P2 | `gpu_p50_ms`, `bottleneck` = `gpu`(게임 스레드 3.127) | 7.18 / 7.167 / 7.172 |

### 6단계 − 5단계의 호드 총수별 변화(결과 그림, 본문)

값은 6단계 중앙값 − 5단계 중앙값이다. "판정"의 범위는 5단계 / 6단계 순서다.

| 본문·그림 값 | 5단계 → 6단계 | 열 | 차 | 판정(범위) |
| --- | --- | --- | --- | --- |
| (그림) 5천 프레임 −0.838 | 6.192 → 5.354 | `frame_p50_ms` | −0.838 | 크다(0.026 / 0.074) |
| (그림) 5천 게임 스레드 −0.014 | 4.383 → 4.369 | `game_p50_ms` | −0.014 | 작다(0.075 / 0.179) |
| (그림) 5천 GPU −0.875 | 5.74 → 4.865 | `gpu_p50_ms` | −0.875 | 크다(0.021 / 0.004) |
| 1만 프레임 −0.148 | 7.654 → 7.506 | `frame_p50_ms` | −0.148 | 크다(0.049 / 0.026) |
| 1만 게임 스레드 +0.011 | 7.333 → 7.344 | `game_p50_ms` | +0.011 | 작다(0.054 / 0.015) |
| 1만 GPU −1.62 | 6.974 → 5.356 | `gpu_p50_ms` | −1.618 | 크다(0.023 / 0.013) |
| (그림) 2만 프레임 +0.278 | 11.256 → 11.534 | `frame_p50_ms` | +0.278 | 작다(0.161 / 0.295) |
| 2만 게임 스레드 +0.545 | 10.725 → 11.27 | `game_p50_ms` | +0.545 | 크다(0.150 / 0.299) |
| (그림) 2만 GPU −3.29 | 9.667 → 6.379 | `gpu_p50_ms` | −3.288 | 크다(0.066 / 0.012) |
| 3만 프레임 +0.615 | 14.568 → 15.183 | `frame_p50_ms` | +0.615 | 크다(0.435 / 0.538) |
| 3만 게임 스레드 +1.13 | 13.843 → 14.972 | `game_p50_ms` | +1.129 | 크다(0.412 / 0.527) |
| 3만 GPU −4.97 | 12.582 → 7.615 | `gpu_p50_ms` | −4.967 | 크다(0.106 / 0.050) |

세 회차(5천 / 2만 / 3만. 1만은 "1만 P1의 네 값" 표):

| 조건 | 프레임 시간 | 게임 스레드 | GPU | 병목 |
| --- | --- | --- | --- | --- |
| `L5-m4offpool` 5000 P1 | 6.2 / 6.174 / 6.192 | 4.34 / 4.415 / 4.383 | 5.75 / 5.729 / 5.74 | `gpu` |
| `L5-m4offpool` 20000 P1 | 11.158 / 11.319 / 11.256 | 10.637 / 10.787 / 10.725 | 9.699 / 9.667 / 9.633 | `game` |
| `L5-m4offpool` 30000 P1 | 14.258 / 14.568 / 14.693 | 13.529 / 13.843 / 13.941 | 12.558 / 12.664 / 12.582 | `game` |
| `L6-m5bounds` 5000 P1 | 5.343 / 5.417 / 5.354 | 4.369 / 4.483 / 4.304 | 4.865 / 4.863 / 4.867 | `gpu` |
| `L6-m5bounds` 20000 P1 | 11.527 / 11.822 / 11.534 | 11.268 / 11.567 / 11.27 | 6.379 / 6.391 / 6.379 | `game` |
| `L6-m5bounds` 30000 P1 | 14.854 / 15.392 / 15.183 | 14.635 / 15.162 / 14.972 | 7.583 / 7.615 / 7.633 | `game` |

본문의 "6단계의 프레임 시간이 5단계보다 짧은 것은 호드 총수 1만까지다. 2만부터는 GPU 이점을 지우고, 3만에서는 넘어선다"는 위 표의 프레임 행이다: 5천·1만은 짧고 차이가 크다, 2만은 차이가 범위 안이다, 3만은 길고 차이가 크다.

### 최대 개체 수와 탑뷰(본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 6단계 최대 개체 수 29,799 | 29799 | `6 L6-m5bounds` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준은 30000, `above range`(`n_p50`) |
| 5단계는 3만에서도 한계에 닿지 않음 | 30000, `above range` | `5 L5-m4offpool` | 같은 파일 `n_p95`, `note_p95` | 3만 프레임 p95 16.298, 세 회차 최댓값 16.488도 16.7 안 |
| 6단계 3만의 프레임 p95(근거) | 16.756 | `L6-m5bounds` 30000 P1 | `frame_p95_ms` | 16.693 / 17.122 / 16.756 |
| 6단계 2만의 프레임 p95(근거) | 13.371 | `L6-m5bounds` 20000 P1 | `frame_p95_ms` | 13.371 / 13.58 / 13.243 |
| 탑뷰 1만 프레임 −2.35ms | −2.345 | 계산 | 5.166 − 7.511. 범위 0.016 / 0.036보다 크다 | |
| 탑뷰 5단계 7.51ms(측정 기록에만) | 7.511 | `L5-m4offpool` 10000 P2 | `frame_p50_ms` | 7.518 / 7.502 / 7.511 |
| 탑뷰 6단계 5.17ms(측정 기록에만) | 5.166 | `L6-m5bounds` 10000 P2 | `frame_p50_ms` | 5.149 / 5.166 / 5.185 |
| 탑뷰에서 GPU가 병목 | | 위 두 행 | `bottleneck` = `gpu`. GPU 7.172 → 4.742, 게임 스레드 3.127 → 3.077 | |

29,799는 `Scripts/plot.py`가 2만(13.371)과 3만(16.756) 사이를 로그 보간한 값이다. 본문은 줄이지 않고 그대로 썼다.

### 최종 구성에서 하나씩 뺀 값(빼 보기 그림, 본문, 틀렸던 가설)

| 본문·그림 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 최종 구성 프레임 7.78 | 7.783 | `L8-q4final` 10000 P1 | `frame_p50_ms` | 7.802 / 7.783 / 7.742 |
| (그림) 최종 구성 게임 스레드 7.65 | 7.648 | 같음 | `game_p50_ms` | 7.667 / 7.648 / 7.609 |
| (그림) 최종 구성 GPU 5.72 | 5.72 | 같음 | `gpu_p50_ms` | 5.72 / 5.715 / 5.723 |
| (그림) VAT 프레임 7.58 | 7.577 | `A8-iskm` 10000 P1 | `frame_p50_ms` | 7.542 / 7.577 / 7.625 |
| (그림) VAT 게임 스레드 7.32 | 7.321 | 같음 | `game_p50_ms` | 7.285 / 7.321 / 7.379 |
| (그림) VAT GPU 6.95 | 6.953 | 같음 | `gpu_p50_ms` | 6.961 / 6.953 / 6.904 |
| (그림) 기본 바운드 프레임 8.55 | 8.554 | `A8-bounds` 10000 P1 | `frame_p50_ms` | 8.49 / 8.554 / 8.562 |
| (그림) 기본 바운드 게임 스레드 7.95 | 7.947 | 같음 | `game_p50_ms` | 7.896 / 7.947 / 7.949 |
| (그림) 기본 바운드 GPU 5.74 | 5.736 | 같음 | `gpu_p50_ms` | 5.736 / 5.736 / 5.731 |
| VAT로 되돌리면 GPU +1.23ms | +1.233 | 계산 | 6.953 − 5.72. 범위 0.057 / 0.008보다 크다 | |
| 게임 스레드 −0.327ms | −0.327 | 계산 | 7.321 − 7.648. 범위 0.094 / 0.058보다 크다 | |
| 프레임 시간 −0.206ms | −0.206 | 계산 | 7.577 − 7.783. 범위 0.083 / 0.060보다 크다 | |
| 고정 바운드를 빼면 프레임 +0.771ms | +0.771 | 계산 | 8.554 − 7.783. 범위 0.072 / 0.060보다 크다 | |
| 그 가운데 게임 스레드 0.299ms | +0.299 | 계산 | 7.947 − 7.648. 범위 0.053 / 0.058보다 크다 | |
| 기본 바운드 ISKM은 VAT보다 GPU 1.22ms 짧다 | −1.217 | 계산 | 5.736 − 6.953(`A8-bounds` − `A8-iskm`) | |
| 기본 바운드 ISKM은 VAT보다 프레임 0.977ms 길다 | +0.977 | 계산 | 8.554 − 7.577(같음) | |
| 모든 행의 병목은 게임 스레드 | | 세 행 | `bottleneck` = `game` | |

참고 값(본문에 쓰지 않음): `A8-bounds` − `L8` GPU +0.016(범위 0.005 / 0.008), 렌더 스레드 +0.026. `A8-iskm` − `L8` 렌더 스레드 +0.383. 프레임 − 게임 스레드 간격은 `L8` 0.135, `A8-iskm` 0.256, `A8-bounds` 0.607이다. 본문은 고정 바운드를 뺀 증가분의 나머지(0.771 − 0.299 = 0.472ms)가 무엇인지 적지 않았다. 게임 스레드 시간에 들지 않는 프레임 끝의 일로 보이지만, 최종 측정의 트레이스로 확인하지 않았다.

`A8-iskm`과 `A8-bounds`를 견주는 것은 한 변수 비교다. 두 행은 최종 구성에서 기법을 하나씩 뺀 행이고, `A8-iskm`에서는 고정 바운드를 넣을 ISKM 컴포넌트가 없다(`LabIskmBounds` 0, `Docs/Worklog/Q3.md` "스위치" 표). 그래서 두 행의 차이는 40\~200m의 표현(VAT 대 엔진 기본 바운드의 ISKM) 하나다.

### 설정과 코드에서 온 값

| 본문 값 | 값 | 출처 |
| --- | --- | --- |
| 30\~200m(5·6단계 인스턴스 구간) | 3000 / 20000cm | `LabCrowdSubsystem.cpp:125` `Bands`(`HydrateCm` 30m, `RepOffCm()` 200m), `LabActorCap` 500 · `LabHydrateM` 30(`full-conditions.json` 기대값) |
| 최종 구성 40\~200m, 80m 밖 컴포넌트 둘 | 40m, 80m | [ADR-0010](../../Docs/Decisions/0010-final-representation-bands.md) 결정 1·2, `LabCrowdSubsystem.cpp:191-200`(설명자를 `High~Low`, `Low~Max` 둘로 나눔) |
| 블렌드 시간 0 | 0.f | `LabConfig.h:183` `BlendTime = 0.f`, `IskmParams()`가 `m5blend` 밖에서 바꾸지 않는다(`LabConfig.cpp:332`) |
| `AnimationMinScreenSize` −1 | −1.f | `LabConfig.h:184` |
| ASTPD의 세 클립(걷기·피격·죽기), 액터와 같은 AnimSequence | 0 / 1 / 2 | `Scripts/create-iskm.py:26-45` `UNITS`의 `anims`, 액터의 클립 `Config/DefaultGame.ini:24-26,34-36`, 클립 번호 = `ELabMinionState`(`LabIskmProcessor.cpp:233-237`) |
| 메시 LOD 다섯 개 | 5 | `create-iskm.py` 실행 로그 `lods=5 optimize_for_instancing=[True, True, True, True, True]`([Worklog/M5.md](../../Docs/Worklog/M5.md) "확인 실행") |
| VAT 메시는 메시 LOD 하나 | 1 | `Scripts/bake-vat.py:187` `convert_skeletal_mesh_to_static_mesh(..., 0)`, 엔진은 LOD 하나만 옮긴다(아래 엔진 소스) |
| 상자 가로세로 ±245m | 24500cm | `ApplyFixedIskmBounds`(`LabCrowdSubsystem.cpp:386`): `DrawExtentCm()`(200m, `LabConfig.cpp:348-351`) × 1.1 + `OrbViewBack`(15m, `LabConfig.h:154`) + 10m = 245m. 개발 실행 로그 `half_xy=24500`([Worklog/M5.md](../../Docs/Worklog/M5.md) "`m5bounds` 변형") |
| 상자 높이 −5\~+15m(측정 기록에만) | −500 / 1500cm | `LabCrowdSubsystem.cpp:387` |
| 히스테리시스 10% | 1.1 | 표현 LOD 경계 기본값 `MassRepresentationFragments.h:309`(03번 글 측정 기록) |
| 미니언은 Lumen 장면에서 빠지고 그림자는 그린다 | | `LabExcludeFromLumen`(`Source/HordeOptLab/LabMinion.h:39-42`)은 `SetAffectDynamicIndirectLighting(false)`만 한다. 그림자 켬은 `IskmParams().bCastShadows`(`LabConfig.h:185`)와 VAT 설명자(`LabCrowdSubsystem.cpp:86`) |

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 확인했다. 경로는 `Engine/` 아래다. 줄임말: **MR** = `Plugins/Runtime/MassGameplay/Source/MassRepresentation`, **ENG** = `Source/Runtime/Engine`.

| 사실 | 위치 |
| --- | --- |
| ISKM 컴포넌트 클래스 | `ENG/Classes/Components/InstancedSkinnedMeshComponent.h:58` `class UInstancedSkinnedMeshComponent : public USkinnedMeshComponent` |
| ISKM은 원본 메시의 LOD가 둘 이상이고 통합 본 맵이 있으면 GPU가 인스턴스마다 LOD를 고른다 | `ENG/Private/InstancedSkinnedMeshSceneProxy.cpp:76` `bUseGpuLodSelection = NumLODs > 1 && ... HasUnifiedBoneMap()`, `ENG/Private/InstancedSkinnedMeshSceneProxy.h:150-160` `GetDesiredLODLevel_RenderThread`. 설명자 기본값 `ENG/Public/InstancedSkinnedMeshSceneProxyDesc.h:43` `bUseGpuLodSelection = true` |
| 트랙 자동 재생 데이터(클립, 시작 위치, 재생 속도, 블렌드 시간, 반복) | `ENG/Classes/Animation/AnimSequenceTransformProviderData.h:104-127` `FAnimSequenceTrackAutoPlayData`. `BlendTime` 주석 "Use 0 for an instance switch" |
| ISKM은 BlendSpace 트랙도 있다 | 같은 파일 `:152-174` `FAnimSequenceTrackBlendSpaceData`, `:199-204` |
| 디스턴스 필드 표현의 기본값은 꺼짐, 정적 메시와 Nanite 프록시가 켠다 | `ENG/Private/PrimitiveSceneProxy.cpp:394` `bSupportsDistanceFieldRepresentation(false)`, `ENG/Private/StaticMeshSceneProxy.cpp:340`, `ENG/Private/Rendering/NaniteResources.cpp:1035`. 스켈레탈 메시·ISKM 프록시 파일에는 이 값을 켜는 줄이 없다(`Source/Runtime` 전체 검색) |
| 디스턴스 필드 장면에 들어가는 조건 | `ENG/Public/SceneManagement.h:1069-1082` `PrimitiveNeedsDistanceFieldSceneData`: (모든 프리미티브 추적 또는 동적 간접 그림자) && 디스턴스 필드 조명 영향 && 보임 && (동적 그림자 또는 동적 간접광). 모든 프리미티브 추적은 `ENG/Private/SceneManagement.cpp:71-76`과 `Config/DefaultEngine.ini:11` `r.GenerateMeshDistanceFields=True`. 추가는 `Source/Runtime/Renderer/Private/RendererScene.cpp:611` |
| 움직인 인스턴스의 디스턴스 필드 오브젝트 갱신 | `Source/Runtime/Renderer/Private/DistanceFieldObjectManagement.cpp:574-611`([engine-notes.md](../../Docs/Reference/engine-notes.md) 11절의 행, 이 글은 줄을 다시 열지 않았다) |
| 프레임 끝 렌더 데이터 전송과 바운드 갱신 | `ENG/Private/Components/InstancedSkinnedMeshComponent.cpp:468-503` `SendRenderInstanceData_Concurrent`. `:495` `FlushChanges`가 참이면 `:497` `UpdateBounds()`. `FlushChanges`는 바뀐 것이 없으면 그대로 돌아간다(`ENG/Private/InstancedStaticMesh/ISMInstanceDataManager.cpp:603-608`) |
| 고정 바운드가 있으면 인스턴스 반복 없이 그 상자 | `ENG/Private/Components/InstancedSkinnedMeshComponent.cpp:1249-1259` `CalcBounds`(`:1251-1254` 발췌, 없으면 `:1257` 도우미 호출) |
| 인스턴스마다 클립 바운드를 합친다 | `ENG/Public/InstancedSkinnedMeshComponentHelper.h:159` `CalcBounds`, `:202` `bFastBuild = false`, `:227-235` 반복(발췌). 애니메이션 바운드가 없으면 `:245-251`이 메시 바운드로 같은 반복을 한다 |
| 인스턴스별 컬링 바운드는 컴포넌트 상자를 쓰지 않는다 | `ENG/Public/InstancedSkinnedMeshComponentHelper.h:119-136`(인스턴스 로컬 바운드 = 클립 바운드 또는 메시 바운드) |
| 엔진은 트랙이 있으면 클립 번호가 다를 때만 다시 쓰고, 없으면 새로 잡는다 | `MR/Private/MassVisualizationComponent.cpp:761-775`(갱신, `:766-769`·`:773` 발췌), `:852-866`(추가). 둘 다 `UMassVisualizationComponent::EndVisualChanges`(`:529`) 안 |
| 갱신되지 않은 움직이는 인스턴스는 지우고 트랙은 둔다 | 같은 파일 `:812-827`(인스턴스 ID 맵만 지우고 `RemoveInstances`) |
| 트랙을 돌려받는 곳은 하나 | 같은 파일 `:1126` `ProcessRemovesForComponent`, `:1156` `DeallocateTrack`(발췌). 입력은 `EntitiesRequiringRemoval`이고 `:1536-1545` `FMassLODInstancedSkinnedMeshSignificanceRange::RemoveInstance`만 채운다 |
| `RemoveInstance`의 호출자는 정적 표현 경로뿐 | `MR/Private/MassStationaryISMSwitcherProcessor.cpp:30`, `MR/Private/MassStationaryISMRepresentationFragmentDestructor.cpp:28`(둘 다 `FMassStaticRepresentationTag` 요구). 호출 줄은 `MassStationaryISMSwitcherProcessor.cpp:105,127,238,264`, `MassStationaryISMRepresentationFragmentDestructor.cpp:68,91` |
| 트랙 제공자는 컴포넌트마다 따로 | 같은 파일 `:1323-1326`(`CreateAnimSequenceTransformProviderDataInstance`), `:1340` `SetHasPerInstancePrevTransforms(true)` |
| 엔진이 트랙의 재생 상태를 읽어 해시에 적는다(UAF 인계용 주석) | 같은 파일 `:777-787`(갱신), `:868-878`(추가). 주석 "Readback resolved animation state for UAF handoff", `ResolvedAnimStates.FindOrAddId` |
| AnimToTexture는 스켈레탈 메시의 LOD 하나만 정적 메시로 옮긴다 | `Plugins/Experimental/AnimToTexture/Source/AnimToTextureEditor/Private/AnimToTextureBPLibrary.cpp:851-868` "Copy Single LOD" |
| ASTPD의 쿡 데이터(애니메이션 바운드) | `ENG/Private/Animation/AnimSequenceTransformProviderData.cpp:716-735` `Serialize`, 쿡할 때 `CacheDerivedData`. 굽는 에디터 단계가 따로 없다는 근거 |

공개 이슈: 트랙 누수(첫째 빈틈)를 다룬 글이 Epic 개발자 포럼에 있다: https://forums.unrealengine.com/t/mass-crowds-using-skinnedmeshinstance-leak-gpu-memory-every-wave-of-spawned-and-killed-entities-permanently-grows-the-animation-track-pool-5-8/2742508 ("Mass crowds using SkinnedMeshInstance leak GPU memory", 5.8). 글의 내용은 읽지 않았고 해결 여부는 적지 않았다. 본문은 이 주소로 가는 링크 문장만 둔다. 클립 번호 조건(둘째 빈틈)과 모션 블러 번짐을 다룬 이슈 링크는 [references.md](../../Docs/Reference/references.md)와 [engine-notes.md](../../Docs/Reference/engine-notes.md)에 없고, 웹 검색은 하지 않았다. [engine-notes.md](../../Docs/Reference/engine-notes.md) 3절 "ISKM 트랙 풀 누수" 행의 파일 위치(`MassVisualizationComponent.cpp` 773·864·1156)는 이 글의 확인과 같다.

### 본문에 넣은 발췌

엔진 코드는 모두 25줄이다(30줄 한도). 남긴 줄은 소스를 그대로 옮기고 들여쓰기만 줄였다.

| 블록 | 줄 수 | 원본 줄 |
| --- | --- | --- |
| 엔진: 프레임 끝 바운드 갱신과 인스턴스 반복 | 13 | `InstancedSkinnedMeshComponent.cpp:495-497`, `// ...`, `InstancedSkinnedMeshComponentHelper.h:227-235` |
| 엔진: 고정 바운드 분기 | 4 | `InstancedSkinnedMeshComponent.cpp:1251-1254` |
| 엔진: 트랙 쓰기·잡기·돌려받기 | 8 | `MassVisualizationComponent.cpp:766-769`, `// ...`, `:773`, `// ...`, `:1156` |
| `BuildEntityConfig` ISKM 설명자 | 14 | `LabCrowdSubsystem.cpp:185-186`, `:187`(줄 끝 주석을 뺐다), `// ...`(`:188-202` 변환·그림자·화면 크기·80m 분리), `:203`, `// ...`(`:204` 닫는 괄호), `:205-212` |
| `ULabIskmProcessor::Execute` 애니 데이터 | 11 | `LabIskmProcessor.cpp:224`, `// ...`(`:225-234` 개발 확인과 주석), `:235-237`, `// ...`(`:238-239` 주석과 `Rate`), `:240-244` |
| `ApplyFixedIskmBounds` | 7 | `LabCrowdSubsystem.cpp:386-389`, `:391-393`(`:390` 주석 줄을 뺐다) |
| `SpawnEntities` 풀 재사용 | 8 | `LabCrowdSubsystem.cpp:257-264` |
| `ULabIskmProcessor::Execute` 트랙 다시 쓰기 | 10 | `LabIskmProcessor.cpp:253-257`, `// ...`(`:258-270` 개발 확인 로그), `:271-274` |

발췌 줄에는 마일스톤 이름이 든 줄 끝 주석이 없다(`:181` 주석은 발췌 밖이다). `LabCrowdSubsystem.cpp:187`의 줄 끝 주석은 엔진 줄 번호(`:1323-1325`)만 가리켜 뺐다.

## 트레이스 값

본문 "결과"의 트레이스 문단 값이다. Insights 화면은 찍지 않고 명령줄 내보내기로 뽑았다.

| 항목 | 값 |
| --- | --- |
| 트레이스 | `Saved/Traces/Full2trace-L5-m4offpool-N10000-P1-r1.utrace`(5단계), `Full2trace-L6-m5bounds-N10000-P1-r1.utrace`(6단계). 병렬, 같은 패키지 08e45f2, 저장소 밖 |
| 내보낸 표 | `%TEMP%\horde-trace\F2-trace-L5-m4offpool-RT.csv`, `F2-trace-L6-m5bounds-RT.csv`(스레드 `RenderThread*`), `-GPU.csv`(`GPU0-Graphics0`, 5단계만 새로 뽑음), `-GT.csv`(프레임 수). `Scripts/trace-export.ps1 -Command TimerStatistics`, 구간 25\~75초, 저장소 밖 |
| 프레임 수 | 5단계 6,313, 6단계 6,515(`FEngineLoop::Tick` 횟수) |
| 식 | 타이머의 합(초) × 1000 ÷ 프레임 수 = 프레임당 ms (03번 글과 같다) |

| 본문 값 | 정밀한 값(5단계 / 6단계) | 타이머 | 합(초) |
| --- | --- | --- | --- |
| `PrepareDistanceFieldScene` 1.41 / 0.0368ms | 1.4124 / 0.0368 | 렌더 스레드 `FSceneRenderer::PrepareDistanceFieldScene` Excl(횟수 6,312 / 6,514) | 8.916218 / 0.239721 |
| `GlobalDistanceFieldUpdate` 0.302 / 0.003ms | 0.3022 / 0.0026 | GPU `GlobalDistanceFieldUpdate` Excl(횟수 6,312 / 135) | 1.907774 / 0.016782 |

참고(본문에 쓰지 않았다): `PrepareDistanceFieldScene` Incl 1.9154 / 0.0600ms. GPU `ShadowDepths` Excl 1.5667 / 1.0911, `BasePass` 0.7552 / 0.2481, `RenderVelocities(%s)` 0.5843 / 0.1851ms. 트레이스를 켠 한 회라 `summary.csv`의 값과 직접 견주지 않는다. GPU 값은 큐 여럿의 합이라 더해도 GPU 프레임 시간이 되지 않는다.

게임 스레드 쪽 직렬 트레이스(`Full2traceser-L5-*`, `Full2traceser-L6-*`)의 `LabVatISMProcessor`·`LabIskmProcessor` 값은 뽑지 않았다. 본문 "남은 비용"은 소스 확인만 적었다.

## 그림

| 파일 | 내용 | 값의 출처 |
| --- | --- | --- |
| `images/iskm-summary.svg` | 요약 그림. 5단계와 6단계의 프레임·게임 스레드·렌더 스레드·GPU 시간 막대(1만 P1, 같은 축척)와 16.7ms 선 | "1만 P1의 네 값" 여덟 행 |
| `images/iskm-principle.svg` | 원리 도식. VAT와 ISKM의 두 길(에디터·에셋 → 게임 스레드 → GPU)과 ISKM의 프레임 끝 바운드 칸 | "설정과 코드에서 온 값", "엔진 소스" |
| `images/iskm-problem.svg` | 문제 그림. 5단계의 게임 스레드·GPU·렌더 스레드 시간 중앙값 대 호드 총수(5천\~3만, 선형 축)와 16.7ms 선 | "5단계의 호드 총수별 값" 열두 행 |
| `images/iskm-delta.svg` | 결과 그림. 호드 총수마다 6단계 − 5단계의 프레임·게임 스레드·GPU 변화 막대. 흔들림보다 작은 칸은 점선 | "6단계 − 5단계의 호드 총수별 변화" 열두 행 |
| `images/iskm-ablation.svg` | 결과 그림. 최종 구성의 40\~200m를 VAT, 기본 바운드 ISKM, 고정 바운드 ISKM으로 그린 프레임·게임 스레드·GPU 시간과 아래 두 문장 | "최종 구성에서 하나씩 뺀 값" |
| `../../Docs/figures/steps/step5-L5-m4offpool.webp`, `step6-L6-m5bounds.webp` | 5·6단계 화면(HUD 켬) | 아래 "화면의 HUD와 표의 차이" |

SVG 다섯 개는 [make-figures.py](make-figures.py)가 만든다(저장소 루트에서 `py -3 -I Posts/06-iskm/make-figures.py`). 측정값은 `summary.csv`와 `full-n-at-60fps.csv`에서 읽는다. 결과 그림의 점선 판정도 스크립트가 `summary.csv`의 `_min`·`_max` 열로 계산한다. 트레이스 값은 쓰지 않는다. 폭 800 기준으로 글자는 16px 이상이고, 자기 흰 배경을 가진다.

## 화면의 HUD와 표의 차이

요약의 두 화면은 측정 실행이 아니라 촬영 실행(`Pre5shot-L5-m4offpool-N10000-P1-r1`, `Pre5shot-L6-m5bounds-N10000-P1-r1`)에서 찍었다. 촬영 패키지는 최종 측정 이전의 것이다. 1\~6단계에는 그 뒤 바뀐 200m 밖 입자 메시가 없어 다시 찍지 않았다([autonomous-decisions.md](../../Docs/Worklog/autonomous-decisions.md) D24). HUD의 프레임 시간(두 단계 모두 9.6ms)은 4K 영역 화면 녹화 부하가 섞여 표의 값(7.65, 7.51ms)보다 높다. 수치는 표의 값만 쓴다. 촬영 경위는 [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "촬영 준비"에 있다.

## 경위의 근거

본문 "틀렸던 가설"과 "화면" 소절의 경위는 수치를 붙이지 않았다. 근거는 최종 측정과 조건이 다른 묶음이라 비교에 쓰지 않는다.

- 바운드를 고정하기 전의 측정(GPU·렌더 스레드는 줄고 프레임 시간은 늘었다, 게임 스레드도 늘었다): M5 묶음 42회, Test 패키지 40fc360, 20k P1. [Worklog/M5.md](../../Docs/Worklog/M5.md) "2단계: 묶음 실행 결과"와 "코디네이터 판단과 비교".
- 늘어난 시간의 큰 몫이 프레임 끝 렌더 상태 전송(`UWorld_SendAllEndOfFrameUpdates`)의 `ParallelFor.Wait`였다: 같은 패키지의 20k P1 병렬·직렬 트레이스. [Worklog/M5.md](../../Docs/Worklog/M5.md) "트레이스 분석", "트레이스 검토".
- 상자를 미리 주자 그 기다림이 거의 사라지고 프레임 시간이 VAT 기준점보다 짧아졌다: 고정 바운드 묶음(Test 패키지 1d2ad79, 20k P1, `m4offpool-bnd`·`m5iskm-bnd`·`m5bounds-bnd`). [Worklog/M5.md](../../Docs/Worklog/M5.md) "고정 바운드 실험 결과", "고정 바운드 판단".
- 그 묶음의 20k에서는 고정 바운드 ISKM의 게임 스레드가 VAT와 같았다. 최종 측정의 2만에서는 6단계 게임 스레드가 +0.545ms다(위 표). 두 묶음의 차이(액터 동기화 Budget, 체력 분산 등 그 뒤에 바뀐 조건)는 가르지 않았다. 본문은 최종 측정의 값으로 "1만까지"라고 썼다.
- 액터 전환의 티 나는 정도(시드 사이 흔들림 안): Development 전환 점수 묶음(소스는 730ff18과 같다, 1만 P1, 시드 셋), [Measurements/Full1/transition-dev/runs.csv](../../Measurements/Full1/transition-dev/runs.csv), [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "Development 전환 점수 묶음" 읽기 2. 앞선 1회 측정은 [Worklog/Q3.md](../../Docs/Worklog/Q3.md) "전환 점수". 최종 측정 빌드로는 다시 재지 않았다.
- 모션 블러 번짐: Development 빌드 20k P1 연속 캡처. [Worklog/M5.md](../../Docs/Worklog/M5.md) "미니언 깜빡임 진단"(모션 블러를 끄면 튀는 프레임 0개, 속도 버퍼, `PrevTransform` 확인, VAT 비교). 캡처 [flicker_m5bounds_f3892_sheet.jpg](../../Measurements/M5/shots/flicker_m5bounds_f3892_sheet.jpg), [flicker_m5bounds_velocity_f3943.jpg](../../Measurements/M5/shots/flicker_m5bounds_velocity_f3943.jpg), [flicker_m4offpool_f3577.jpg](../../Measurements/M5/shots/flicker_m4offpool_f3577.jpg). 결정은 [ADR-0007](../../Docs/Decisions/0007-keep-default-motion-blur.md). 그 캡처에서 번짐은 37\~66초 게임 시각에만 났고 이유는 모른다.
- 트랙 누수가 실제로 쌓이는 것(풀을 끄면 16분 동안 트랙과 GPU 메모리가 계속 는다): 엔티티 풀 묶음 `Pool1`. STATUS.md "측정 결과"와 05번 글의 몫이라 이 글은 수치를 쓰지 않았다.
- 글 재료의 사실 확인: `Docs/post-review.md` "07 ISKM vs VAT"와 핵심 인사이트 5(글을 다 쓰면 지우는 문서라, 지운 뒤에는 git 이력에서 본다). 그 문서의 수치는 옛 묶음 값이라 쓰지 않았다.

## 확인하지 않은 것

- 게임 스레드가 늘어난 곳이 어느 타이머인지. 렌더 스레드의 디스턴스 필드 값은 위 "트레이스 값"에 있지만, 게임 스레드 쪽(직렬 트레이스)은 뽑지 않았다. 본문은 엔진 소스로 확인한 경로만 적었다.
- 고정 바운드를 뺀 증가분 가운데 게임 스레드 밖의 몫(0.771 − 0.299)이 프레임 끝 바운드 계산인지. `A8-bounds` 트레이스가 없다.
- 2만에서 옛 고정 바운드 묶음과 최종 측정의 6단계 게임 스레드가 갈린 이유.
- 최종 측정 패키지의 ASTPD에 애니메이션 바운드가 있는지. Test 구성은 로그가 없다(`lab_iskm … bounds=`가 남지 않는다). 없으면 `CalcBounds`는 메시 바운드로 같은 반복을 하므로 본문의 설명(인스턴스 반복)은 그대로다.
- 모션 블러 번짐의 엔진 안 원인, 37\~66초에만 나는 이유, 최종 측정 빌드에서도 나는지.
- 5·6단계 화면에서 ISKM과 VAT의 모습 차이(메시 LOD, 스키닝). 움직이는 WebP를 열어 보지 않았다(브라우저 금지). 본문 이미지 설명은 "그린다"까지만 적었다.
- SVG를 화면에 그려 보지 않았다. 글자 겹침은 좌표와 글자 수로만 확인했다.
