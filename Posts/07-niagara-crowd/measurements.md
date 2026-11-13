# 07 측정 기록

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 최종 측정 `Full3`(2026-11-13 16:52:49\~21:00:00, 178회, 실패 0). 경위는 [작업 기록의 Full3 절](../../Docs/Worklog/Q7.md#2026-11-13-측정-full3) |
| 빌드 | Test 패키지 df41c13, CSV 메타데이터 `labbuild=df41c131c650` |
| 조건을 바꾼 이유 | 애니메이션 예산을 2ms에서 4ms로 올렸고([ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md)), 오브의 저격 시점(`SnipeView`) 경합을 고쳤다(62b6a42). 그래서 전 조건을 앞 묶음 `Full2`(08e45f2)에서 `Full3`으로 다시 쟀다 |
| 재측정 | `Full3-A8-farcull-N10000-P1-r2`를 `r5`로 다시 쟀다. 그래서 `A8-farcull`의 세 회차는 r1 / r3 / r5다 |
| 조건 | 2560x1440 Epic, `r.VSync=0`, `t.MaxFPS=0`, Lumen·VSM 켬, `mass.FullyParallel=true`. 2종(소형 90%, 대형 10%), 체력 60\~240(시드 20261106). 오브 시점(P1). 측정 창 60초, 세 회차 중앙값 |
| 단계와 변형 | `L6-m5bounds`(`-LabStage=M5 -LabVariant=m5bounds`), `L7-q2crowd`(`-LabStage=M5 -LabVariant=q2crowd`), `L8-q4final`(`-LabStage=M5 -LabVariant=q4final`) |
| 빼 보기 | `A8-crowdfx`(`-LabCrowdFx=0`), `A8-farcull`(`-LabFarCull=0`). 둘 다 `L8-q4final`에 인자 하나를 더한다 |
| 기대 메타데이터 | `Scripts/full-conditions.json`. `L6`과 `L7`은 `LabCrowdFx`·`LabCrowdFxActive`·`LabTechCrowdFx`만 0 → 1로 다르다. 둘 다 `LabFarShadow=1`, `LabActorCap=500`, `LabHydrateM=30` |
| 요약 파일 | [최종 측정의 요약 표](../../Measurements/Full3/summary.csv)(세 회차 중앙값과 범위), [실행별 값](../../Measurements/Full3/runs.csv)(회차별 한 행), [빼 보기 표](../../Docs/figures/full3/full-ablation.csv) |
| 60fps를 지키는 최대 개체 수(`N@16.7ms`) | [최대 개체 수 CSV](../../Docs/figures/full3/full-n-at-60fps.csv)(`Scripts/plot.py`가 측정 N 사이를 로그 보간) |
| 트레이스 | 앞 묶음 `Full2` 패키지(08e45f2)의 병렬 트레이스 한 회씩. `Full3`으로 다시 뽑지 않았다. 아래 "트레이스 값" |

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

`summary.csv`는 행을 `variant,n,preset`으로 찾는다. 범위는 세 회차의 최솟값\~최댓값이다. "(그림)"은 그림에만 쓴 값이다. 세 회차 열은 `Full3/runs.csv`의 r1 / r2 / r3이다(`A8-farcull`만 r1 / r3 / r5). 별말이 없으면 P1이다.

### 1만에서 6단계와 7단계(요약 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| 6단계 프레임 시간 8.45ms | 8.452 | `L6-m5bounds` 10000 | `frame_p50_ms` | 8.452 / 8.459 / 8.417 |
| 6단계 게임 스레드 8.29ms | 8.292 | 같음 | `game_p50_ms` | 8.292 / 8.298 / 8.254 |
| (그림) 6단계 렌더 스레드 3.25ms | 3.247 | 같음 | `render_p50_ms` | 3.247 / 3.246 / 3.255 |
| 6단계 GPU 5.37ms | 5.373 | 같음 | `gpu_p50_ms` | 5.373 / 5.365 / 5.385 |
| 7단계 프레임 시간 8.73ms | 8.728 | `L7-q2crowd` 10000 | `frame_p50_ms` | 8.728 / 8.687 / 8.762 |
| (그림) 7단계 게임 스레드 8.57ms | 8.569 | 같음 | `game_p50_ms` | 8.569 / 8.525 / 8.589 |
| (그림) 7단계 렌더 스레드 3.36ms | 3.355 | 같음 | `render_p50_ms` | 3.401 / 3.355 / 3.352 |
| (그림) 7단계 GPU 6.06ms | 6.061 | 같음 | `gpu_p50_ms` | 6.12 / 6.025 / 6.061 |
| 병목은 두 단계 모두 게임 스레드 | `game` | 위 두 조건 | `bottleneck` | 세 회차 모두 `game` |
| 프레임 시간 +0.276ms(제목, 한 문장으로) | 0.276 | 계산 | 8.728 − 8.452. 범위 0.042 / 0.075보다 크다 | |
| 게임 스레드 +0.277ms | 0.277 | 계산 | 8.569 − 8.292. 범위 0.044 / 0.064보다 크다 | |
| GPU +0.688ms | 0.688 | 계산 | 6.061 − 5.373. 범위 0.020 / 0.095보다 크다 | |
| (참고) 렌더 스레드 +0.108ms | 0.108 | 계산 | 3.355 − 3.247 | |

### 6단계가 그리는 것과 GPU의 여유(문제 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처 | 세 회차 |
| --- | --- | --- | --- | --- |
| 액터 373 | 372.81 | `L6-m5bounds` 10000 | `actors_mean`(`Lab/Actors` 평균) | 372.81 / 372.82 / 372.81 |
| ISKM 3,401 | 3,400.98 | 같음 | `ism_band0`+`1`+`2`+`3` = 214.99 + 1,024.53 + 2,161.46 + 0 | 밴드0 214.99 / 214.97 / 215.01, 밴드1 1,024.51 / 1,024.62 / 1,024.53, 밴드2 2,161.46 / 2,161.27 / 2,161.86 |
| 그리지 않음 6,226 | 6,226.21 | 계산 | 10,000 − 372.81 − 3,400.98 | |
| "절반 넘게" | 62.3% | 계산 | 6,226.21 ÷ 10,000 | |
| GPU의 여유 2.92ms | 2.919 | 계산 | 6단계 `game_p50_ms` − `gpu_p50_ms` = 8.292 − 5.373 | |

`ism_band0`\~`3`은 `ULabIskmProcessor`가 그린 ISKM 인스턴스를 카메라 수평 거리 0\~30 / 30\~80 / 80\~200 / 200m 밖으로 센 프레임 평균이다(`LabIskmProcessor.cpp:314-315,331-334`, `Scripts/summarize.py:102,198-200`). 풀 밖 엔티티는 모두 액터, ISKM, 표현 없음 가운데 하나라서 나머지를 "그리지 않음"으로 계산했다(같은 셈이 맞는다는 확인은 [Worklog/Q2.md](../../Docs/Worklog/Q2.md) "Q2b" 개발 실행 표의 "원소 + 피격·사망 + 그린 수 = 10,000"). 그리지 않음에는 그 거리에서 피격·사망으로 멈춘 몇 개가 들어 있다. 7단계의 같은 계산은 6,221.37이다(373.41 + 3,405.22).

### 호드 총수별 6단계와 7단계(결과 그림, 본문)

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차(6단계 / 7단계의 범위) |
| --- | --- | --- | --- | --- |
| 5천 프레임 +0.349 | 5.678 − 5.329 | 5000 | `frame_p50_ms` | 범위 0.170 / 0.122 |
| (그림) 5천 게임 스레드 +0.076, 흔들림 안 | 4.339 − 4.263 | 5000 | `game_p50_ms` | 범위 0.141 / 0.135 |
| (그림) 5천 GPU +0.364 | 5.209 − 4.845 | 5000 | `gpu_p50_ms` | 범위 0.076 / 0.064 |
| 5천은 GPU가 병목 | `gpu` | 5000 | `bottleneck` | 세 회차 모두 |
| (그림) 1만 +0.276 / +0.277 / +0.688 | 위 표 | 10000 | 위 표 | 위 표 |
| (그림) 2만 프레임 +0.529 | 13.977 − 13.448 | 20000 | `frame_p50_ms` | 범위 0.062 / 0.053 |
| (그림) 2만 게임 스레드 +0.536 | 13.726 − 13.19 | 20000 | `game_p50_ms` | 범위 0.065 / 0.052 |
| (그림) 2만 GPU +1.34 | 7.785 − 6.441 | 20000 | `gpu_p50_ms` | 범위 0.009 / 0.033 |
| (그림) 3만 프레임 +0.817 | 17.264 − 16.447 | 30000 | `frame_p50_ms` | 범위 0.614 / 0.534 |
| 3만 게임 스레드 +0.798 | 17.048 − 16.25 | 30000 | `game_p50_ms` | 범위 0.613 / 0.527 |
| 3만 GPU +1.77 | 9.413 − 7.64 | 30000 | `gpu_p50_ms` | 범위 0.110 / 0.075 |
| 최대 개체 수 약 2만 4천 → 약 2만 2천 | 23774 / 22033 | `6 L6-m5bounds`, `7 L7-q2crowd` | `full-n-at-60fps.csv` `n_p95` | 중앙값 기준은 30000 `above range` / 27984 |
| (참고) 프레임 시간 느린 쪽 5% 경계 | 5천 6.197 → 6.534, 1만 10.143 → 10.572, 2만 15.287 → 15.939, 3만 18.601 → 19.127 | 각 N | `frame_p95_ms` | |

회차별 값(6단계 ; 7단계): 5천 프레임 5.366 / 5.329 / 5.196 ; 5.678 / 5.753 / 5.631. 5천 게임 스레드 4.331 / 4.263 / 4.19 ; 4.339 / 4.417 / 4.282. 5천 GPU 4.862 / 4.845 / 4.786 ; 5.209 / 5.245 / 5.181. 2만 프레임 13.476 / 13.448 / 13.414 ; 14.024 / 13.977 / 13.971. 2만 게임 스레드 13.213 / 13.19 / 13.148 ; 13.764 / 13.726 / 13.712. 2만 GPU 6.434 / 6.441 / 6.443 ; 7.801 / 7.768 / 7.785. 3만 프레임 16.695 / 16.447 / 16.081 ; 17.644 / 17.264 / 17.11. 3만 게임 스레드 16.487 / 16.25 / 15.874 ; 17.429 / 17.048 / 16.902. 3만 GPU 7.537 / 7.647 / 7.64 ; 9.444 / 9.413 / 9.369.

3만의 프레임과 게임 스레드 변화는 범위보다 크지만 1.5배 안쪽이다. 5천의 게임 스레드 변화는 두 범위보다 작아 구별되지 않는다. 최대 개체 수의 감소는 2만 프레임 p95의 차(+0.652)가 두 범위(0.129 / 0.360)보다 커서 구별된다. 본문의 개체 수는 두 자리로 줄였다(23,774 → 약 2만 4천, 22,033 → 약 2만 2천).

### 최종 구성에서 하나씩 바꾼 측정(빼 보기 그림, 본문)

기준은 `L8-q4final` 10000 P1(r1\~r3)이다. 묶음 끝 A/A(`L8-q4final-aa`, r11\~r13)는 쓰지 않았다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열) | 세 회차 |
| --- | --- | --- | --- | --- |
| (그림) 최종 구성 프레임 9.75ms | 9.746 | `L8-q4final` | `frame_p50_ms` | 9.707 / 9.763 / 9.746 |
| (그림) 최종 구성 게임 스레드 9.59ms | 9.592 | 같음 | `game_p50_ms` | 9.559 / 9.616 / 9.592 |
| (그림) 최종 구성 GPU 5.75ms | 5.752 | 같음 | `gpu_p50_ms` | 5.752 / 5.757 / 5.746 |
| 배경 무리를 빼면 프레임 −0.398ms | 9.348 − 9.746 | `A8-crowdfx` | `frame_p50_ms` | 9.476 / 9.268 / 9.348. 범위 0.208 / 0.056보다 크다 |
| GPU −0.465ms | 5.287 − 5.752 | 같음 | `gpu_p50_ms` | 5.292 / 5.254 / 5.287. 범위 0.038 / 0.011보다 크다 |
| 게임 스레드 −0.385ms | 9.207 − 9.592 | 같음 | `game_p50_ms` | 9.328 / 9.118 / 9.207. 범위 0.210 / 0.057보다 크다 |
| ISKM으로 그리면 프레임 +4ms | 13.75 − 9.746 = 4.004 | `A8-farcull` | `frame_p50_ms` | 13.75 / 13.525 / 13.82(r1 / r3 / r5) |
| 게임 스레드 +3.95ms | 13.544 − 9.592 = 3.952 | 같음 | `game_p50_ms` | 13.544 / 13.33 / 13.6 |
| GPU 차이 0.026ms, 범위 안 | 5.778 − 5.752 = 0.026 | 같음 | `gpu_p50_ms` | 5.788 / 5.778 / 5.761. 범위 0.027 / 0.011 안 |
| ISKM 5,984개 더 | 5,983.57 | 같음 | `ism_band3`(200m 밖 ISKM, `L8`은 0) | 5,983.57 / 5,986.25 / 5,983.18 |
| (참고) ISKM 인스턴스 3,324 → 9,543 | | `L8`, `A8-farcull` | `iskm_instances_p50` | |
| 두 행 모두 병목 게임 스레드 | `game` | 위 세 조건 | `bottleneck` | |

`A8-farcull`(`-LabFarCull=0`)은 표현 LOD의 Off 거리와 ISKM 바운드 범위만 바꾼다(Simulation LOD의 Off 거리는 그대로). 엔티티 구동 입자의 원소는 0이고, 500m 밖 독립 입자는 그대로 그린다([Worklog/Q2.md](../../Docs/Worklog/Q2.md) "Q2c" 진행 마지막 항, `LabConfig.h:295,340`, `LabConfig.cpp:348-351`). 그래서 본문은 "ISKM(500m까지)"와 "500m 밖은 이 구성에서도 입자"로 적었다.

"최종 구성은 입자의 그림자를 끄므로 GPU가 덜 주는 것과 방향이 맞는다": 최종 구성은 `LabFarShadow=0`이라 `SetupCrowdFx`가 컴포넌트 그림자를 끈다(`LabGameMode.cpp:311`). 7단계 트레이스의 그림자 깊이 패스 증가(아래 0.252ms, 앞 빌드 08e45f2의 트레이스)를 6→7단계 GPU 증가 0.688ms에서 빼면 0.436ms이고, `A8-crowdfx`의 0.465ms와 비슷하다. 트레이스와 `summary.csv`를 섞은 다른 실행끼리의 비교라 본문에는 수치로 쓰지 않았다.

"6단계에서 7단계로 갈 때보다 조금 더 준다": `A8-crowdfx`의 프레임 −0.398, 게임 스레드 −0.385와 6→7단계의 +0.276, +0.277을 견준 것이다. 다른 두 쌍의 차이라 차이 판정을 하지 않았다. 앞 측정에서는 두 쌍이 −0.268 / −0.271과 +0.279 / +0.284로 같은 크기였다.

"GPU의 차이는 범위 안이라 입자와 같다": 앞 측정에서는 +0.038로 범위(0.017) 밖이었다. `Full3`에서는 `A8-farcull`의 범위가 0.027로 넓어 차이 0.026이 그 안에 든다.

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
- "메시를 바꾼 뒤 최종 구성의 GPU 시간이 크게 줄어 병목이 GPU에서 게임 스레드로 옮겨 갔다": 옛 총괄 묶음(Test 패키지 730ff18)의 최종 구성 병목은 GPU였고, 앞 최종 측정(`Full2`, 08e45f2)과 지금 최종 측정(`Full3`, df41c13)에서는 게임 스레드다. 두 빌드 사이의 차이는 `Content/Lab/FX/`의 배경 무리 에셋 셋뿐이다(커밋 bf1a4c8, 01번 글의 측정 기록과 같은 확인). 옛 값은 [ADR-0011](../../Docs/Decisions/0011-entity-driven-crowd-particles.md) "결과"와 Q6.md "Full2 재측정"의 사다리 표 괄호에 있다. 본문에는 옛 값을 쓰지 않았다.

## 화면 근거

- 요약의 두 화면(`step6-L6-m5bounds.webp`, `step7-L7-q2crowd.webp`)은 측정 실행이 아니라 촬영 실행에서 찍었다. 촬영 실행은 `Full3shot-L6-m5bounds-N10000-P1-r1`, `Full3shot-L7-q2crowd-N10000-P1-r1`이다([Measurements/Full3/captures.csv](../../Measurements/Full3/captures.csv), 최종 측정과 같은 패키지 df41c13). HUD의 수치는 화면 녹화 부하가 섞여 표보다 높다. 수치는 표의 값만 쓴다.
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
| 본문의 나눗셈 | "BasePass는 1.617초를 6,515로 나눈 0.248ms에서 3.110초를 6,330으로 나눈 0.491ms": 1.616661 × 1000 ÷ 6,515 = 0.2481, 3.110305 × 1000 ÷ 6,330 = 0.4914. 화면 표시값 1.62초로 나누면 0.2487이라 0.249가 되어 표의 0.248과 어긋난다. 그래서 본문은 내보낸 합을 넷째 자리까지 적는다 |
| 바뀐 것 | 처음 계획은 프레임 하나의 GPU 트랙이었다. 본문 값이 구간 평균이라 구간 합계 화면으로 바꿨다 |
| 구간 | Timing 뷰의 시간 눈금자를 끌어 25초부터 75초까지(길이 표시 50s)를 골랐다. 손으로 고른 구간이라 내보내기의 창과 프레임이 한두 개 다를 수 있다. 화면의 합계는 표시 자릿수까지 내보낸 값과 같다 |
| 가공 | 화면 캡처 둘(왼쪽 절반, 오른쪽 절반)을 이어 붙였다. 트레이스 파일 이름이 보이는 제목 줄은 잘라 냈다. 빨간 상자와 단계 이름을 그렸다. 두 트레이스를 견주는 그림은 위아래로 붙였다 |

## 측정 기록의 변경

- 2026-11-13: 수치를 총괄 묶음 `Full2`(08e45f2)에서 `Full3`(df41c13)으로 바꿨다. 애니메이션 예산 2ms → 4ms(ADR-0016)와 `SnipeView` 경합 수정(62b6a42)으로 전 조건을 다시 쟀기 때문이다. ISKM으로 그린 빼 보기의 GPU 차이가 범위 안이 되어 그 문장을 고쳤다. BasePass 나눗셈의 반올림을 내보낸 합으로 맞췄다. 트레이스 값과 Insights 화면은 앞 빌드의 것 그대로다.
