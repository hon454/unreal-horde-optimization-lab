# 07 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정(2026-11-12 20:34\~11-13 00:39). 경위는 [작업 기록의 재측정 절](../../Docs/Worklog/Q6.md#full2-재측정2026-11-12) |
| 빌드 | Test 패키지 08e45f2, CSV 메타데이터 `labbuild=08e45f24a5e5` |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L6-m5bounds`(`-LabStage=M5 -LabVariant=m5bounds`), `L7-q2crowd`(`-LabStage=M5 -LabVariant=q2crowd`), `L8-q4final`(`-LabStage=M5 -LabVariant=q4final`) |
| 빼 보기 | `A8-crowdfx`(`-LabCrowdFx=0`), `A8-farcull`(`-LabFarCull=0`). 둘 다 `L8-q4final`에 인자 하나를 더한다 |
| 기대 메타데이터 | `Scripts/full-conditions.json`. `L6`과 `L7`은 `LabCrowdFx`·`LabCrowdFxActive`·`LabTechCrowdFx`만 0 → 1로 다르다. 둘 다 `LabFarShadow=1`, `LabActorCap=500`, `LabHydrateM=30` |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full2/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full2/runs.csv)(회차별 한 행), [빼 보기 표](../../Docs/figures/full2/full-ablation.csv) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full2/full-n-at-60fps.csv)(`Scripts/plot.py`가 측정 N 사이를 로그 보간) |
| 트레이스 | 같은 패키지의 병렬 트레이스 한 회씩. 아래 "트레이스 값" |

## 7단계의 구현(6단계와 다른 곳)

- 스위치: `FLabConfig::bCrowdFx`가 변형 `q2crowd`·`q4final`에서 참이고 `-LabCrowdFx=`가 이긴다(`Source/HordeOptLab/LabConfig.cpp:140-146`). 변형 `q2crowd`는 "표현·풀링·Budget은 m5bounds와 같다"(`LabConfig.h:66`). 엔티티 구동(`bCrowdFxEntity`)은 기본 참(`LabConfig.h:259`).
- 프로세서 등록: `UsesCrowdFxEntity()`이고 M1 이상이면 게임 모듈이 `ULabCrowdFxGatherProcessor`를 켠다(`HordeOptLab.cpp:50-51`).
- 모으기 프로세서: Representation 그룹, `VisualizationProcessing` 뒤, 게임 스레드(`LabCrowdFxProcessor.cpp:24-26`). 쿼리는 미니언 태그 있음, 풀 태그 없음(`:31-38`). 표현 None이고 상태 Walk인 엔티티만 원소 (X, Y, T0, −StateStartTime)로 담는다(`:89-107`). T0은 Off 아키타입이면 `LastMoveTime`, 아니면 지금(`:86`, `:101-106`). 배열 넣기(`:119-126`), 지금 시각(`:127-130`). CSV 스탯 `Lab/CrowdRecs` 등(`:131-136`).
- 시스템 켜기: `ALabGameMode::SetupCrowdFx`(`LabGameMode.cpp:274-353`). 오브 위치에 자동 활성화 없이 스폰(`:298-299`), 그림자(`:308-312`), 독립 입자의 스폰율·수명·속도(`:313-317`), 사용자 변수 확인과 `LabOrbPos`(`:319-347`), 활성화(`:349`).
- 독립 입자: 초당 생성 = N ÷ 맵 반경 × 이동 속도(`LabConfig.cpp:436-438`), 대형 몫 10%(`:441-449`), 안쪽 반경 = 맵 반경 500m(`:457-461`), 수명 = (1,000m − 500m) ÷ 3m/s(`:466-469`), 정상 상태 수 = 생성 × 수명(`:471-475`). 바깥 반경 1,000m(`LabConfig.h:102`).
- 에셋(`Content/Lab/FX/NS_LabCrowd`, 만드는 순서는 [create-crowd-fx.md](../../Scripts/create-crowd-fx.md)): 엔티티 구동 이미터 `SmallEnt`·`LargeEnt`는 GPU, 월드 공간, 보간 스폰 끔(5절 3-2), 메시 렌더러 그림자 켬·카메라 거리 컬링 175\~2,000m·모션 벡터 Approximate(5절 3-3), `SpawnPerFrame` 수 = 배열 길이(5절 3-4), 수명 0.001초·원소 선택·위치 식·동적 머티리얼 파라미터(5절 3-5). 독립 이미터 `Small`·`Large`는 1,000m 고리에서 스폰, 렌더러 그림자 끔(3절).
- 입자 메시: 네 이미터 모두 `SM_LabMinionVAT_Bone_Lod4`(대형 `SM_LabMinionLargeVAT_Bone_Lod4`)와 그 머티리얼 인스턴스(create-crowd-fx.md 2·3·5·6절). 근거리 VAT와 ISKM은 LOD0 산출물을 그대로 쓴다(6절).
- 같은 것: 표현 LOD 구간(0/30/80/200m), 액터 상한 500, 80m 밖 그림자 켬, Simulation LOD와 Off 이동, 엔티티 풀, 고정 바운드, 시나리오.

## 본문의 수치와 출처

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. "(그림)"은 그림에만 쓴 값이다. 세 회차 열은 r1 / r2 / r3이다. 별말이 없으면 P1이다.

### 1만에서 6단계와 7단계(요약 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 6단계 프레임 시간 7.51ms | 7.506 | `L6-m5bounds` 10000 | `frame_p50_ms` | 7.498 / 7.506 / 7.524 |
| 6단계 게임 스레드 7.34ms | 7.344 | 같음 | `game_p50_ms` | 7.335 / 7.344 / 7.35 |
| (그림) 6단계 렌더 스레드 3.25ms | 3.247 | 같음 | `render_p50_ms` | 3.26 / 3.247 / 3.231 |
| 6단계 GPU 5.36ms | 5.356 | 같음 | `gpu_p50_ms` | 5.359 / 5.346 / 5.356 |
| 7단계 프레임 시간 7.79ms | 7.785 | `L7-q2crowd` 10000 | `frame_p50_ms` | 7.741 / 7.785 / 7.803 |
| (그림) 7단계 게임 스레드 7.63ms | 7.628 | 같음 | `game_p50_ms` | 7.596 / 7.628 / 7.638 |
| (그림) 7단계 렌더 스레드 3.36ms | 3.36 | 같음 | `render_p50_ms` | 3.357 / 3.36 / 3.373 |
| (그림) 7단계 GPU 6.10ms | 6.096 | 같음 | `gpu_p50_ms` | 6.096 / 6.072 / 6.129 |
| 병목은 두 단계 모두 게임 스레드 | `game` | 위 두 조건 | `bottleneck` | 세 회차 모두 `game` |
| 프레임 시간 +0.279ms | 0.279 | 계산 | 7.785 − 7.506. 범위 0.026 / 0.062보다 크다 | |
| 게임 스레드 +0.284ms | 0.284 | 계산 | 7.628 − 7.344. 범위 0.015 / 0.042보다 크다 | |
| GPU +0.740ms | 0.740 | 계산 | 6.096 − 5.356. 범위 0.013 / 0.057보다 크다 | |
| (참고) 렌더 스레드 +0.113ms | 0.113 | 계산 | 3.36 − 3.247 | |

### 6단계가 그리는 것과 GPU의 여유(문제 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처 | 세 회차 |
| --- | --- | --- | --- | --- |
| 액터 373 | 373.27 | `L6-m5bounds` 10000 | `actors_mean`(`Lab/Actors` 평균) | 373.26 / 373.28 / 373.27 |
| ISKM 3,401 | 3,400.63 | 같음 | `ism_band0`+`1`+`2`+`3` = 214.66 + 1,024.41 + 2,161.56 + 0 | 밴드별 회차 값은 `runs.csv` |
| 그리지 않음 6,226 | 6,226.10 | 계산 | 10,000 − 373.27 − 3,400.63 | |
| "절반 넘게" | 62.3% | 계산 | 6,226.10 ÷ 10,000 | |
| GPU의 여유 1.99ms | 1.988 | 계산 | 6단계 `game_p50_ms` − `gpu_p50_ms` = 7.344 − 5.356 | |

`ism_band0`\~`3`은 `ULabIskmProcessor`가 그린 ISKM 인스턴스를 카메라 수평 거리 0\~30 / 30\~80 / 80\~200 / 200m 밖으로 센 프레임 평균이다(`LabIskmProcessor.cpp:314-315,331-334`, `Scripts/summarize.py:102,198-200`). 풀 밖 엔티티는 모두 액터, ISKM, 표현 없음 가운데 하나라서 나머지를 "그리지 않음"으로 계산했다(같은 셈이 맞는다는 확인은 [Worklog/Q2.md](../../Docs/Worklog/Q2.md) "Q2b" 개발 실행 표의 "원소 + 피격·사망 + 그린 수 = 10,000"). 그리지 않음에는 그 거리에서 피격·사망으로 멈춘 몇 개가 들어 있다. 7단계의 같은 계산은 6,221.79다(373.79 + 3,404.42).

### 호드 총수별 6단계와 7단계(결과 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차(6단계 / 7단계의 범위) |
| --- | --- | --- | --- | --- |
| (그림) 5천 프레임 +0.410 | 5.764 − 5.354 | 5000 | `frame_p50_ms` | 범위 0.074 / 0.030 |
| (그림) 5천 게임 스레드 +0.083, 흔들림 안 | 4.452 − 4.369 | 5000 | `game_p50_ms` | 범위 0.179 / 0.022 |
| (그림) 5천 GPU +0.381 | 5.246 − 4.865 | 5000 | `gpu_p50_ms` | 범위 0.004 / 0.033 |
| 5천은 GPU가 병목 | `gpu` | 5000 | `bottleneck` | 세 회차 모두 |
| (그림) 1만 +0.279 / +0.284 / +0.740 | 위 표 | 10000 | 위 표 | 위 표 |
| (그림) 2만 프레임 +0.663 | 12.197 − 11.534 | 20000 | `frame_p50_ms` | 범위 0.295 / 0.139 |
| (그림) 2만 게임 스레드 +0.677 | 11.947 − 11.27 | 20000 | `game_p50_ms` | 범위 0.299 / 0.125 |
| (그림) 2만 GPU +1.33 | 7.713 − 6.379 | 20000 | `gpu_p50_ms` | 범위 0.012 / 0.065 |
| (그림) 3만 프레임 +0.799 | 15.982 − 15.183 | 30000 | `frame_p50_ms` | 범위 0.538 / 0.376 |
| 3만 게임 스레드 +0.785 | 15.757 − 14.972 | 30000 | `game_p50_ms` | 범위 0.527 / 0.374 |
| 3만 GPU +1.94 | 9.551 − 7.615 | 30000 | `gpu_p50_ms` | 범위 0.050 / 0.062 |
| 최대 개체 수 29,799 → 26,587 | 29799 / 26587 | `6 L6-m5bounds`, `7 L7-q2crowd` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준은 둘 다 30000 `above range` |
| (참고) 프레임 시간 느린 쪽 5% 경계 | 5천 6.395 → 6.596, 1만 9.191 → 9.336, 2만 13.371 → 13.987, 3만 16.756 → 17.851 | 각 N | `frame_p95_ms` | |

회차별 값(6단계 / 7단계): 5천 프레임 5.343 / 5.417 / 5.354, 5.741 / 5.764 / 5.771. 2만 프레임 11.527 / 11.822 / 11.534, 12.069 / 12.197 / 12.208. 3만 프레임 14.854 / 15.392 / 15.183, 16.118 / 15.742 / 15.982. 3만 게임 스레드 14.635 / 15.162 / 14.972, 15.892 / 15.518 / 15.757. 3만 GPU 7.583 / 7.615 / 7.633, 9.494 / 9.551 / 9.556.

2만·3만의 프레임과 게임 스레드 변화는 범위보다 크지만 2배 안쪽이다. 5천의 게임 스레드 변화는 6단계의 범위(r2가 길다)보다 작아 구별되지 않는다.

### 최종 구성에서 하나씩 바꾼 측정(빼 보기 그림, 본문)

기준은 `L8-q4final` 10000 P1(r1\~r3)이다. 묶음 끝 A/A(`L8-q4final-aa`, r11\~r13)는 쓰지 않았다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 최종 구성 프레임 7.78ms | 7.783 | `L8-q4final` | `frame_p50_ms` | 7.802 / 7.783 / 7.742 |
| (그림) 최종 구성 게임 스레드 7.65ms | 7.648 | 같음 | `game_p50_ms` | 7.667 / 7.648 / 7.609 |
| (그림) 최종 구성 GPU 5.72ms | 5.72 | 같음 | `gpu_p50_ms` | 5.72 / 5.715 / 5.723 |
| 배경 무리를 빼면 프레임 −0.268ms | 7.515 − 7.783 | `A8-crowdfx` | `frame_p50_ms` | 7.515 / 7.509 / 7.626. 범위 0.117 / 0.060보다 크다 |
| GPU −0.467ms | 5.253 − 5.72 | 같음 | `gpu_p50_ms` | 5.244 / 5.268 / 5.253 |
| 게임 스레드 −0.271ms | 7.377 − 7.648 | 같음 | `game_p50_ms` | 7.377 / 7.368 / 7.484 |
| ISKM으로 그리면 프레임 +4.30ms | 12.087 − 7.783 = 4.304 | `A8-farcull` | `frame_p50_ms` | 12.058 / 12.087 / 12.153 |
| 게임 스레드 +4.24ms | 11.883 − 7.648 = 4.235 | 같음 | `game_p50_ms` | 11.851 / 11.883 / 11.947 |
| GPU +0.038ms | 5.758 − 5.72 | 같음 | `gpu_p50_ms` | 5.758 / 5.749 / 5.766. 범위 0.017 / 0.008보다 크다 |
| ISKM 5,986개 더 | 5,985.78 | 같음 | `ism_band3`(200m 밖 ISKM, `L8`은 0) | 5,986.13 / 5,985.78 / 5,985.21 |
| (참고) ISKM 인스턴스 3,322 → 9,542 | | `L8`, `A8-farcull` | `iskm_instances_p50` | |
| 두 행 모두 병목 게임 스레드 | `game` | 위 세 조건 | `bottleneck` | |

`A8-farcull`(`-LabFarCull=0`)은 표현 LOD의 Off 거리와 ISKM 바운드 범위만 바꾼다(Simulation LOD의 Off 거리는 그대로). 엔티티 구동 입자의 원소는 0이고, 500m 밖 독립 입자는 그대로 그린다([Worklog/Q2.md](../../Docs/Worklog/Q2.md) "Q2c" 진행 마지막 항, `LabConfig.h:295,340`, `LabConfig.cpp:348-351`). 그래서 본문은 "ISKM(500m까지)"와 "500m 밖은 이 구성에서도 입자"로 적었다.

"최종 구성은 입자의 그림자를 끄므로 GPU가 덜 주는 것과 방향이 맞는다": 최종 구성은 `LabFarShadow=0`이라 `SetupCrowdFx`가 컴포넌트 그림자를 끈다(`LabGameMode.cpp:311`). 7단계 트레이스의 그림자 깊이 패스 증가(아래 0.252ms)와 6→7단계 GPU 증가 0.740ms의 차 0.488ms가 `A8-crowdfx`의 0.467ms와 비슷하다. 다른 실행끼리의 비교라 본문에는 수치로 쓰지 않았다.

### 트레이스 값(결과 본문)

트레이스는 한 회씩이고, 트레이스를 켠 실행이라 `summary.csv`의 값과 직접 견주지 않는다. 구간은 실행 시작 뒤 25\~75초다. 내보낸 방법은 `Scripts/trace-export.ps1 -Command TimerStatistics -Extra "-threads=<스레드> -startTime=25 -endTime=75 -columns=*"`다. 값은 `Incl`(초)을 창 안 프레임 수로 나눈 프레임당 ms다. GPU 타이머는 큐 여럿에 걸쳐 겹치므로 패스 값을 더해도 GPU 시간이 되지 않는다.

| 쓰인 곳 | 파일(`Saved/Traces/`, 저장소 밖) | 내보낸 표(`%TEMP%\horde-trace\`, 저장소 밖) | 창 안 프레임 수 |
| --- | --- | --- | --- |
| 6단계 GPU | `Full2trace-L6-m5bounds-N10000-P1-r1.utrace`(병렬) | `F2-trace-L6-m5bounds-GPU.csv`(`GPU0-Graphics0`) | 6,515(`BasePass` 횟수) |
| 7단계 GPU | `Full2trace-L7-q2crowd-N10000-P1-r1.utrace`(병렬) | `F2-trace-L7-q2crowd-GPU.csv` | 6,330 |
| 6단계 게임 스레드 | 6단계와 같은 파일 | `F2-trace-L6-m5bounds-GT.csv`(`GameThread`) | 6,515(`FEngineLoop::Tick` 횟수) |
| 7단계 게임 스레드 | 7단계와 같은 파일 | `F2-trace-L7-q2crowd-GT.csv` | 6,331 |

| 본문 값 | 타이머 | 6단계(초, 프레임당 ms) | 7단계(초, 프레임당 ms) | 차 |
| --- | --- | --- | --- | --- |
| 모으기 프로세서 0.199ms | `LabCrowdFxGatherProcessor_*` | 없음 | 1.258121, 0.199 | |
| BasePass +0.243ms | `BasePass` | 1.616661, 0.248 | 3.110305, 0.491 | 0.243 |
| 속도 패스 +0.243ms | `RenderVelocities(%s)` | 1.205691, 0.185 | 2.709501, 0.428 | 0.243 |
| 그림자 깊이 +0.252ms | `ShadowDepths` | 7.247518, 1.112 | 8.638312, 1.365 | 0.252 |
| Lumen 화면 프로브 수집 +0.420ms | `LumenScreenProbeGather`(큰 행) | 14.013078, 2.151 | 16.272711, 2.571 | 0.420 |

- 같은 내보내기의 다른 차(참고): `FEngineLoop::Tick` 7.674 → 7.897ms(+0.223), `FMassProcessingPhase::ExecuteTick EMassProcessingPhase::PrePhysics` 2.228 → 2.418ms, `FXSystemPreRender`(GPU) 0.472 → 0.309ms. 게임 스레드 내보내기에도 GPU 타이머가 섞여 나온다(스레드 필터의 동작은 보지 않았다).
- `LumenScreenProbeGather`는 같은 이름의 행이 둘이다. 본문 값은 큰 행(6단계 14.01초)이다. 작은 행은 0.03초 남짓으로 두 단계가 같다.
- `RenderVelocities(%s)`는 호출이 프레임보다 많아(7,465 / 7,254) `BasePass` 횟수로 나눴다.

## 설정값과 계산값

| 본문 값 | 값 | 근거 |
| --- | --- | --- |
| 카메라 188m에서 ISKM이 된다 | 188 | 계산: `Lerp(80m, 200m, 1 − 0.1)`. 엔진 `MassLODCalculator.h:873-881`, 표현 히스테리시스 기본 10%(`MassRepresentationFragments.h:309` `BufferHysteresisOnDistancePercentage = 10.0f`), 프로젝트는 바꾸지 않는다(`LabConfig.h:104-107` 주석). [engine-notes.md](../../Docs/Reference/engine-notes.md) 17절 |
| 맵 밖 500\~1,000m | 50,000 / 100,000cm | `FLabScenario::MapRadius`(`LabConfig.h:140`), `FLabCrowdFx::OuterRadius`(`LabConfig.h:102`) |
| 엔티티는 오브 500m 안에만 있다 | | 초기 배치 반지름 [8m, 500m], 재생성 링 [490m, 500m](`LabCrowdSpawner.cpp`, code-tour.md "시작") |
| 1만에서 독립 입자 1만 개 | 10,000 | 계산: 초당 생성 10,000 ÷ 50,000cm × 300cm/s = 60, 수명 (100,000 − 50,000) ÷ 300 = 166.67초, 60 × 166.67 = 10,000(`LabConfig.cpp:436-475`). 개발 로그 `lab_crowdfx particles=10000`(Q6.md "입자 GPU 비용 조사") |
| 수명 0.001초, 보간 스폰 끔 | 0.001 | `Scripts/create-crowd-fx.md` 5절 3-2, 3-5. 보간 스폰을 끄면 새 입자는 생긴 프레임에 갱신을 돌지 않는다(`NiagaraCommon.h:230-243`, engine-notes 24절) |
| 입자의 위치 식 | | `Scripts/create-crowd-fx.md` 5절 3-5(71행) |
| LOD0 메시 소형 삼각형 9,786 | 9,786(대형 10,379) | 커맨들릿 `get_num_triangles`([Worklog/Q6.md](../../Docs/Worklog/Q6.md) "입자 GPU 비용 조사"의 "입자가 그려지는 방식"), create-crowd-fx.md 6절 |
| LOD4 소형 586, 대형 621 | 586 / 621 | 굽기 로그 `LABVAT Bone_Lod4 ... tris=586`, 대형 `tris=621`(Q6.md "입자 전용 저폴리 VAT 메시", create-crowd-fx.md 6절) |
| 그림자 기본값 꺼짐 | `CastShadow = false` | `Engine/Source/Runtime/Engine/Private/Components/PrimitiveComponent.cpp:341`. `UNiagaraComponent`는 바꾸지 않고, 시스템 덮어쓰기가 꺼져 있으면 컴포넌트 값이 쓰인다(`NiagaraComponent.cpp:4678`, engine-notes 23절) |
| 메시 렌더러가 커스텀 데이터 0번에 입자 번호를 쓴다 | | `NiagaraRendererMeshes.cpp:1130,1207,1225`, `NiagaraUpdateMeshGPUSceneInstances.usf:208`(engine-notes 17절) |
| 동적 머티리얼 파라미터 네 칸 | TimeOffset, Playrate, StartFrame, EndFrame | AnimToTexture `GetFrameSwitch`(engine-notes 17절), create-crowd-fx.md 2절, 5절 3-5. 첫 칸 = `Particles.LabRec.w`(걷기 위상) |
| 독립 입자의 생성 흐름이 맵 가장자리 흐름과 같다 | | 설계 근거: [Worklog/Q2.md](../../Docs/Worklog/Q2.md) "장면 수치", "Q2b" "500m 경계의 밀도"(반지름 방향 밀도 N ÷ R) |

## 입자 메시와 첫 구현의 경위

본문 "바꾼 것"의 두 이야기(엔티티와 무관한 입자로 시작한 첫 구현, LOD0 메시에서 LOD4 메시로)는 수치 없이 방향만 썼다. 근거는 최종 측정과 조건이 다른 옛 기록이다.

- 첫 구현(오브 둘레의 독립 입자만)과 경계 문제: [ADR-0011](../../Docs/Decisions/0011-entity-driven-crowd-particles.md) "맥락"과 "고려한 대안", [Worklog/Q2.md](../../Docs/Worklog/Q2.md) "3단위"·"Q2b".
- 입자 GPU 비용의 원인(정점 비용, 모션 벡터·Lumen은 원인이 아님, 감축 LOD는 모양이 깨짐): [Worklog/Q6.md](../../Docs/Worklog/Q6.md) "입자 GPU 비용 조사"(Development, 한 회씩). 감축 LOD의 깨짐은 `Measurements/CrowdFx1/shots/horizon-compare.jpg` 셋째 줄.
- LOD4를 고른 근거와 화면: Q6.md "입자 전용 저폴리 VAT 메시". 본문의 지평선 비교 그림은 그 절의 `Measurements/CrowdFx2/shots/lod-compare-horizon.jpg`(Development `-game`, 같은 시드 20초, 지평선 띠 2배)다. 오른쪽 위 글자와 왼쪽 눈금은 그 비교 때 넣었다.
- "메시를 바꾼 뒤 최종 구성의 GPU 시간이 크게 줄어 병목이 GPU에서 게임 스레드로 옮겨 갔다": 옛 총괄 묶음(Test 패키지 730ff18)의 최종 구성 병목은 GPU였고, 최종 측정(08e45f2)에서는 게임 스레드다. 두 빌드 사이의 차이는 `Content/Lab/FX/`의 배경 무리 에셋 셋뿐이다(커밋 bf1a4c8, 01번 글의 측정 기록과 같은 확인). 옛 값은 [ADR-0011](../../Docs/Decisions/0011-entity-driven-crowd-particles.md) "결과"와 Q6.md "Full2 재측정"의 사다리 표 괄호에 있다. 본문에는 옛 값을 쓰지 않았다.

## 화면 근거

- 요약의 두 화면(`step6-L6-m5bounds.webp`, `step7-L7-q2crowd.webp`)은 측정 실행이 아니라 촬영 실행에서 찍었다. 7단계는 새 패키지로 다시 찍은 `Pre6shot-L7-q2crowd-N10000-P1-r1`이다([Worklog/Q6.md](../../Docs/Worklog/Q6.md) "Pre6 촬영"). HUD의 프레임 시간(6단계 7.5ms, 7단계 8.1ms, 첫 프레임에서 읽음)은 화면 녹화 부하가 섞여 표의 값과 다르다. 수치는 표의 값만 쓴다.
- "경계에서 빈 띠나 겹친 띠가 생기지 않는다": Development 정지 카메라 연속 캡처의 행 밝기와 확대 화면([Worklog/Q2.md](../../Docs/Worklog/Q2.md) "Q2b" 에셋 단위 진행의 화면 확인 (b), `Measurements/Q2/shots/q2b-boundary-zoom.jpg`, `q2b-static-entity.jpg`). 같은 방향으로 걷는 모습은 같은 절 (d)(`q2b-dir-zoom.jpg`). 새 패키지의 입자가 지평선까지 깨짐 없이 그려지는 것은 [particle-check-q4final-t20.jpg](../../Measurements/Full2/shots/particle-check-q4final-t20.jpg)(최종 구성, 묶음 밖 확인 실행)로 봤다.
- "저격당한 개체는 배열에서 빠져 사라진다": 모으기 프로세서가 상태 Walk만 담는다(`LabCrowdFxProcessor.cpp:95-99`). 피격·사망 개체는 멈춘다(Q2.md "Q2b" "확인한 것"의 피격·사망 항).

## Insights 화면

본문의 Insights 스크린샷이다. 2026-11-13에 찍었다. 값의 계산은 위 트레이스 표가 근거이고, 화면은 같은 트레이스에서 같은 타이머의 구간 합계를 보인다.

| 항목 | 내용 |
| --- | --- |
| 파일 | `images/insights-gpu-passes.jpg`. 위가 6단계, 아래가 7단계다 |
| 트레이스 | `Full2trace-L6-m5bounds-N10000-P1-r1`, `Full2trace-L7-q2crowd-N10000-P1-r1`(병렬) |
| 거른 말 | 거르지 않고 GPU 타이머만 |
| 표시한 행(Incl) | `LumenScreenProbeGather` 14.01s / 16.27s, `ShadowDepths` 7.25s / 8.64s, `BasePass` 1.62s / 3.11s, `RenderVelocities(%s)` 1.21s / 2.71s. Count 6,515 / 6,330 |
| 화면에 없는 값 | 게임 스레드의 모으기 프로세서 0.199ms는 찍지 않았다 |
| 바뀐 것 | 처음 계획은 프레임 하나의 GPU 트랙이었다. 본문 값이 구간 평균이라 구간 합계 화면으로 바꿨다 |
| 구간 | Timing 뷰의 시간 눈금자를 끌어 25초부터 75초까지(길이 표시 50s)를 골랐다. 손으로 고른 구간이라 내보내기의 창과 프레임이 한두 개 다를 수 있다. 화면의 합계는 표시 자릿수까지 내보낸 값과 같다 |
| 가공 | 화면 캡처 둘(왼쪽 절반, 오른쪽 절반)을 이어 붙였다. 트레이스 파일 이름이 보이는 제목 줄은 잘라 냈다. 빨간 상자와 단계 이름을 그렸다. 두 트레이스를 견주는 그림은 위아래로 붙였다 |
