# 06 측정 기록

[글](README.md)의 수치, 그림의 값, 엔진 소스 위치, 화면의 출처를 적는다. 그림은 `py -3 -I Posts/06-engine-findings/make-figures.py`가 CSV에서 만든다.

## 묶음과 빌드

- 총괄 묶음 `Full4`. Test 패키지 de0e21b(태그 `v2-full4`, CSV `build` 열 `labbuild=de0e21bd64ac`)다. 호드 총수 1만, 이동 장면(`V0`), 오브 시점 카메라(`P1`), 워밍업 60초 + 측정 180초, 조건마다 3회다. 값은 세 실행의 중앙값이다. 파일은 [summary.csv](../../Measurements/Full4/summary.csv), [runs.csv](../../Measurements/Full4/runs.csv), [equivalence.csv](../../Measurements/Full4/equivalence.csv)다.
- 참고 묶음 `Pool2`. 같은 패키지 de0e21b다([batch.json](../../Measurements/Pool2/batch.json) `commit` de0e21bd64ac). 호드 총수 1만, `V0`, `P1`, 워밍업 60초 + 측정 960초다. 구성은 `s8`(풀 켬)과 `a8pool`(풀 끔)이고 **구성마다 1회**라 범위가 없다. 1분 구간 값은 [timeline.csv](../../Measurements/Pool2/timeline.csv)(`Scripts/pool-timeline.py`)다. 프레임 CSV는 `Saved/Profiling/CSV/Pool2-*.csv`(git 밖)다.
- 참고 묶음의 값은 총괄 묶음의 값과 한 그림에 잇지 않았다. 글 본문의 4분 실행 값은 `Full4`, 16분 실행 값은 `Pool2`다.
- "구별되지 않음"은 중앙값의 차이가 두 조건 범위 중 큰 쪽보다 작거나 0.5ms 아래인 경우다([measurement.md](../../Docs/Guides/measurement.md) "차이 판정").

## 본문의 수치와 출처

| 본문의 수치 | 값 | 출처 | 계산 또는 비고 |
| --- | --- | --- | --- |
| 16분 실행, 풀 끔의 트랙 "12만 개를 넘는다", 120,428개 | 120,428 | `Pool2-V0-a8pool-N10000-P1-r1` 16분 `iskm_tracks_end` | 참고 묶음, 1회 |
| 4분 실행 끝 트랙, 풀 켬 20,215개 | 20,214.72 | `Full4` `V0-s8` `iskm_tracks_last` | 측정 창 마지막 10% 프레임 평균, 세 실행 중앙값 |
| 4분 실행 끝 트랙, 풀 끔 30,797개 | 30,797.07 | `Full4` `V0-a8pool` `iskm_tracks_last` | 같다 |
| 4분 실행 = 워밍업 + 측정 | 60 + 180초 | `Full4` 조건 표 | 계산 240초 |
| 풀 켬·끔의 프레임 p95 구별되지 않음 | 14.231, 14.240 | `Full4` `V0-s8`, `V0-a8pool` `frame_p95_ms` | 차 +0.009ms, 0.5ms 아래 |
| 16분 실행 | 측정 960초 | `Pool2` `batch.json` `measure_sec` | 계산 16분 |
| 풀 끔 GPU 메모리 3,370MB → 5,105MB | 3,369.6 → 5,105.3 | `Pool2` `a8pool` 1분, 16분 `gpu_mem_mean` | 정수로 반올림 |
| 풀 켬 16분 끝 트랙 21,149개 | 21,149 | `Pool2-V0-s8-N10000-P1-r1` 16분 `iskm_tracks_end` | 참고 묶음, 1회 |
| 풀 켬 트랙이 3분 뒤 2만 남짓에서 분당 70개쯤 는다 | 3분 20,232, 16분 21,149 | `Pool2-V0-s8-N10000-P1-r1` `iskm_tracks_end` | (21,149 − 20,232) ÷ 13 = 70.5 |
| 풀 켬 GPU 메모리가 평평하다 | 3,345.9\~3,375.0 | `Pool2` `s8` 1\~16분 `gpu_mem_mean` | 최솟값과 최댓값 |
| 프레임 p95는 12분까지 같은 구간끼리 차이가 0.5ms 아래다 | 차 −0.287\~+0.434 | `Pool2` 1\~12분 `frame_p95`(끔 − 켬) | 모두 0.5ms 아래 |
| 끝의 네 구간에서 0.62\~1.14ms 높다 | +1.138, +0.730, +0.621, +0.766 | `Pool2` 13\~16분 `frame_p95`(끔 − 켬) | 1회씩이라 범위 없음 |
| 엔티티 풀은 단계 5부터 | `Step >= 5` | `LabConfig.cpp:54`(de0e21b) | 소스 확인 |
| ISKM은 단계 6부터, 카메라 35\~200m | `Step >= 6`, 35m, 200m | `LabConfig.h:424`, `:19` `DefaultHydrateM`, `:219` `FarM` | 소스 확인(de0e21b) |
| 트랙 예약 용량 = 개체 수 × 1.1 + 64 | 소형 9,964, 대형 1,164 | `LabIskmProcessor.cpp:309-311` | `ceil(9000 × 1.1) + 64`, float32 곱 |
| 200m 밖 피격 개체의 컴포넌트 용량 64 | 64 | `LabIskmProcessor.cpp:408-409` | 소스 확인 |
| 예약한 용량의 합 22,384 | 22,384 | 아래 "트랙 예약 용량의 합" | 계산 |
| 트랙 예약을 끈 조건 구별되지 않음 | 14.239, 14.231 | `Full4` `V0-a8res`, `V0-s8` `frame_p95_ms` | 차 +0.008ms, 0.5ms 아래 |
| 200m 밖(저격 구간, 피격 ISKM) | 200m | `LabConfig.h:219` `FarM`(de0e21b) | 표현 LOD Off 거리 |
| 확인 트레이스 2초마다 | 2초 | 저격 간격, [measurement.md](../../Docs/Guides/measurement.md) "확정값" 저격 행 | 저격 틱마다 쏜다(`LabOrb.cpp:340-344`) |
| 정지 장면 빗나감 약 1.7% | 63 / 3,780 = 1.667% | `Full4` `equivalence.csv` 정지 14조건 | 아래 "캡슐 빗나감" |
| 이동 장면 한 번도 빗나가지 않음 | 0 / 8,086 | `Full4` `equivalence.csv` 이동 25조건 | 아래 "캡슐 빗나감" |
| 시나리오 카드의 2m/s, 호드 1만 | 2m/s, 10,000 | [measurement.md](../../Docs/Guides/measurement.md) "시나리오" | 설정값 |
| UE 5.8.4 | 5.8.4 | [pc-specs.md](../../Docs/Reference/pc-specs.md) | 소스 빌드 |

- 본문의 "개발 빌드 진단에서 이 프로세서의 실행 시간은 거의 0이 되었다"는 방향만 쓴 문장이다. 값은 아래 "본문에 수치로 쓰지 않은 근거"에 있다.
- "16분"은 측정 창의 길이 960초이고 워밍업 60초는 따로 더한다(`Pool2`). "4분"은 워밍업 60초와 측정 180초를 합친 240초다(`Full4`). 그림의 가로축은 측정을 시작한 뒤의 분이다(워밍업 60초 제외).

## 세 실행의 값(`Full4`)

호드 총수 1만, 이동 장면, 오브 시점 카메라. 라벨은 `Full4-V0-<구성>-N10000-P1-r<회차>`다.

| 실행 | `iskm_tracks_last` | 프레임 p95(ms) | 비고 |
| --- | --- | --- | --- |
| `s8` r1 / r2 / r3 | 20,214.72 / 20,216.61 / 20,212.92 | 14.426 / 14.035 / 14.231 | 중앙값 20,214.72, 14.231 |
| `a8pool` r1 / r2 / r3 | 30,797.03 / 30,807.31 / 30,797.07 | 14.369 / 14.088 / 14.240 | 중앙값 30,797.07, 14.240 |
| `a8res` r1 / r2 / r3 | 20,207.53 / 20,211.88 / 20,214.14 | 14.436 / 14.160 / 14.239 | 중앙값 20,211.88, 14.239 |

- `iskm_tracks_last`는 CSV 스탯 `Lab/IskmTracks`의 측정 창 뒤 10% 프레임 평균이다(`Scripts/summarize.py:149-150, 276-278`). `Lab/IskmTracks`는 ISKM 컴포넌트마다 트랙 맵의 크기를 더한 값이다(`LabIskmProcessor.cpp:193-231` `LabCountIskm`). 트랙 풀의 끝 번호가 아니다.
- `summary.csv`의 `runs_used`는 세 조건 모두 `r1 r2 r3`다.

## 트랙 예약 용량의 합

- 단계 8은 종류마다 ISKM 컴포넌트가 셋이다. 그림자를 켠 `[High, Low)`, 그림자를 끈 `[Low, Off)`, 그리는 끝 밖 피격 개체의 `[Off, Max)`다(`LabCrowdSubsystem.cpp:175-208`, de0e21b의 줄). 컴포넌트마다 트랙 제공자가 따로다.
- 용량은 앞의 둘이 종류의 개체 수 × 1.1 + 64, 셋째가 64다(`LabIskmProcessor.cpp:305-311, 409`). 1만이면 대형이 `(N + 9) / 10` = 1,000, 소형이 9,000이다.
- 합: 2 × 9,964 + 2 × 1,164 + 2 × 64 = 22,384.
- 트랙 맵의 항목은 모두 활성 트랙이고 번호가 서로 다르다. 그래서 한 컴포넌트의 항목 수가 용량을 넘으면 그 컴포넌트의 끝 번호도 용량을 넘는다. `a8pool`의 30,797은 22,384보다 크므로 적어도 한 컴포넌트는 예약 뒤에 끝 번호가 늘었다. 컴포넌트별 값은 CSV에 없다.

## 캡슐 빗나감(`Full4` `equivalence.csv`)

- 정지 장면: `speed` `V1`, N 1만, `capsule_ok` `report`인 14조건이다. `V1-s8`과 `a8bnd`, `a8bud`, `a8far`, `a8fix`, `a8fsb`, `a8fx`, `a8hold`, `a8iskm`, `a8pool`, `a8res`, `a8sep`, `a8shd`, `a8sim`이다. 조건마다 트레이스 270회다. 빗나감은 차례로 4, 4, 4, 4, 6, 5, 4, 6, 7, 5, 3, 8, 1, 2회이고 합 63회다.
- 이동 장면: `speed` `V0`, N 1만, `capsule_ok`가 `n/a`가 아니고 트레이스가 있는 25조건이다. 캡슐 콜리전을 끈 `V0-a8cap`은 뺐다. 트레이스 합 8,086회, 빗나감 0회다.
- 뺀 조건: `a8cap`(캡슐 콜리전 끔, 트레이스가 모두 빗나간다), `a8hyd`(액터가 없어 트레이스 0회), `V0-s8` 탑뷰(트레이스 0회).
- 그림의 1.67%는 63 ÷ 3,780 = 1.667%를 세 자리로 적은 것이다. 본문은 "약 1.7%"로 적었다.
- 정지 장면의 빗나감은 동등성 검사에서 보고만 한다([measurement.md](../../Docs/Guides/measurement.md) "실패 등급"). 이 묶음의 정지 장면 수치는 이 글에서 쓰지 않았다.

## 그림의 값

### [pool-long-run.svg](images/pool-long-run.svg)

참고 묶음 `Pool2`, 호드 총수 1만, 이동 장면, 오브 시점 카메라, 구성마다 1회. 그림에 적은 값은 1분과 16분 구간이다. 아래는 16구간 전체다.

| 분 | ISKM 트랙 수(켬 / 끔) | GPU 메모리 MB(켬 / 끔) | 프레임 p95 ms(켬 / 끔) | p95 차(끔 − 켬) |
| --- | --- | --- | --- | --- |
| 1 | 17,211 / 17,711 | 3,371.0 / 3,369.6 | 14.102 / 14.035 | −0.067 |
| 2 | 20,048 / 24,971 | 3,360.7 / 3,417.9 | 14.793 / 14.506 | −0.287 |
| 3 | 20,232 / 31,940 | 3,373.4 / 3,618.5 | 14.632 / 14.384 | −0.248 |
| 4 | 20,338 / 38,420 | 3,375.0 / 3,795.3 | 14.740 / 14.722 | −0.018 |
| 5 | 20,408 / 44,972 | 3,345.9 / 3,888.8 | 13.857 / 13.658 | −0.199 |
| 6 | 20,476 / 51,673 | 3,351.9 / 3,918.2 | 14.314 / 14.339 | +0.025 |
| 7 | 20,549 / 58,752 | 3,363.6 / 3,912.8 | 14.307 / 14.413 | +0.106 |
| 8 | 20,615 / 66,071 | 3,363.4 / 3,858.8 | 15.124 / 15.062 | −0.062 |
| 9 | 20,685 / 72,937 | 3,365.5 / 3,817.3 | 14.632 / 14.852 | +0.220 |
| 10 | 20,747 / 79,468 | 3,371.5 / 3,823.3 | 14.668 / 15.051 | +0.383 |
| 11 | 20,816 / 86,049 | 3,348.9 / 3,796.8 | 13.844 / 14.116 | +0.272 |
| 12 | 20,887 / 92,756 | 3,360.4 / 4,690.8 | 14.095 / 14.529 | +0.434 |
| 13 | 20,950 / 99,901 | 3,371.5 / 5,104.5 | 14.559 / 15.697 | +1.138 |
| 14 | 21,019 / 107,148 | 3,363.4 / 5,102.9 | 14.815 / 15.545 | +0.730 |
| 15 | 21,084 / 113,948 | 3,374.3 / 5,106.2 | 14.794 / 15.415 | +0.621 |
| 16 | 21,149 / 120,428 | 3,370.6 / 5,105.3 | 14.468 / 15.234 | +0.766 |

| 그림의 글자 | CSV 값 | 반올림 |
| --- | --- | --- |
| 트랙 1분 17,211 / 17,711, 16분 21,149 / 120,428 | 같다 | 개수라 줄이지 않는다 |
| GPU 메모리 1분 3,371 / 3,370 | 3,371.0 / 3,369.6 | 정수로 반올림 |
| GPU 메모리 16분 3,371 / 5,105 | 3,370.6 / 5,105.3 | 정수로 반올림 |
| 프레임 p95 1분 14.1 / 14.0 | 14.102 / 14.035 | 세 자리, 나란히 적는 값이라 14.0의 0을 남긴다 |
| 프레임 p95 16분 14.5 / 15.2 | 14.468 / 15.234 | 세 자리 |

- 트랙 수는 구간 끝 값(`iskm_tracks_end`), GPU 메모리는 구간 평균(`gpu_mem_mean`, CSV 스탯 `GPUMem/LocalUsedMB`), 프레임 p95는 구간의 프레임 시간 95번째 백분위수(`frame_p95`)다.
- 풀 끔의 트랙은 분당 약 6,850개씩 는다((120,428 − 17,711) ÷ 15 = 6,847.8). 풀 켬은 3분 뒤부터 분당 약 70개다((21,149 − 20,232) ÷ 13 = 70.5). 풀 켬의 값만 본문과 대체 텍스트에 "분당 70개쯤"으로 썼다.

### [capsule-plane.svg](images/capsule-plane.svg)

| 그림의 글자 | 값 | 출처 |
| --- | --- | --- |
| 정지 장면 3,780회 중 63회(1.67%) | 63 / 3,780 | 위 "캡슐 빗나감" |
| 이동 장면 8,086회 중 0회 | 0 / 8,086 | 위 "캡슐 빗나감" |

- 그림의 기하(시작점, 캡슐, 정면 평면, 이동 장면의 회전 24도)는 설명을 위한 것이고 잰 값이 아니다. 아래 줄의 겹침 질의 문장은 개발 빌드 진단이다(아래 "본문에 수치로 쓰지 않은 근거").

### 값이 없는 도식

- [findings-map.svg](images/findings-map.svg), [entity-pool.svg](images/entity-pool.svg), [iskm-realloc.svg](images/iskm-realloc.svg), [query-wait.svg](images/query-wait.svg)는 개념도다. `iskm-realloc.svg`의 칸 수(옛 5칸, 새 6칸)와 `query-wait.svg`의 막대 길이는 설명을 위한 값이다.
- `entity-pool.svg`는 글 05(`Posts/05-cost-cuts-and-contributions/`)도 쓴다. 파일은 이 폴더의 `images/entity-pool.svg` 하나다.

## 화면

모두 개발 빌드(Development, `-game`) 연속 캡처에서 고른 대표 프레임이다. 측정값이 아니고 화면의 모양만 보인다. 빌드는 측정 패키지 de0e21b보다 앞이다.

| 파일 | 조건 | 보이는 것 | 근거 |
| --- | --- | --- | --- |
| [p1-reserve0-f4469-4471.jpg](../../Measurements/V2W9Flash/shots/p1-reserve0-f4469-4471.jpg) | 1만, 최종 구성, 오브 시점, `-LabIskmTrackReserve=0`, 프레임 4,469\~4,471 | 가운데 프레임에서 소형 ISKM 전체가 세로로 끌린다 | [V2.md](../../Docs/Worklog/V2.md) "2026-11-15 W9 ISKM 번쩍임" |
| [p1-reserve1-f4469-4471.jpg](../../Measurements/V2W9Flash/shots/p1-reserve1-f4469-4471.jpg) | 같은 조건, 트랙 예약 켬(기본) | 같은 프레임 번호에서 끌림이 없다 | 같다 |
| [iskm-forceseq1-f3839-before-after.jpg](../../Measurements/V2M6Fix/shots/iskm-forceseq1-f3839-before-after.jpg) | `-LabIskmForceSeq=1 -LabHydrate=0 -LabFarM=400 -LabCrowdFx=0`, 프레임 3,839 | 왼쪽 고치기 전(소형 없음, 대형 누움), 오른쪽 고친 뒤 | [V2.md](../../Docs/Worklog/V2.md) "M6 조사: 먼 거리 ISKM의 피격·사망 클립" |

- 번쩍임 화면의 조건: 1280×720 창, `-benchmark -fps=60`, 시드 20261106, 궤도 57.3m·2m/s. 화면 행 200\~420을 잘라 세 프레임을 위아래로 이었다. 고른 기준은 띠 휘도 튐이 가장 큰 프레임이다(V2.md 같은 절).

## 엔진 소스 위치

소스 빌드 `Engine/` 기준이다(UE 5.8.4). 2026-11-17에 이 글을 쓰면서 아래 줄을 직접 열어 확인했다.

### 1. 트랙 누수

| 확인한 것 | 파일과 줄 | 심볼 |
| --- | --- | --- |
| 핸들이 맵에 없으면 트랙 할당(갱신 경로) | `Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassVisualizationComponent.cpp:761, 773` | `UMassVisualizationComponent::EndVisualChanges` |
| 같은 일(추가 경로) | 같은 파일 `:852, 864` | 같다 |
| 갱신되지 않은 엔티티는 인스턴스만 지움 | 같은 파일 `:812-827` | 같다 |
| 트랙이 있으면 클립 번호가 다를 때만 다시 씀 | 같은 파일 `:763-770, 854-861` | 같다 |
| 트랙을 돌려받는 곳 | 같은 파일 `:1126-1169`(`:1156` `DeallocateTrack`) | `ProcessRemovesForComponent` |
| 돌려받을 엔티티를 넣는 곳 | 같은 파일 `:1537-1546` | `FMassLODInstancedSkinnedMeshSignificanceRange::RemoveInstance` |
| 그 호출자(정지 표현) | `MassStationaryISMSwitcherProcessor.cpp:127, 238, 264`(쿼리 `:30`), `MassStationaryISMRepresentationFragmentDestructor.cpp:91`(쿼리 `:28`) | `FMassStaticRepresentationTag` 요구 |
| 그 호출자(정지한 인스턴스 액터) | `Plugins/Runtime/InstancedActors/Source/InstancedActors/Private/InstancedActorsVisualizationSwitcherProcessor.cpp:112`, 같은 폴더 `InstancedActorsRepresentationActorManagement.cpp:217-218` | Mass 군중 경로 밖 |
| 할당은 가장 낮은 빈 번호, 없으면 끝에 붙임 | `Source/Runtime/Engine/Classes/Animation/AnimTrackPool.h:102-120, 161-172, 274-277` | `AllocateTrack`, `DeallocateTrack`, `GetNumTracks` |
| 트랙 풀은 트랙 제공자 인스턴스의 멤버 | `Source/Runtime/Engine/Classes/Animation/AnimSequenceTransformProviderData.h:1038, 1137` | `UAnimSequenceTransformProviderDataInstance`, `TrackPools` |
| 인스턴스는 컴포넌트를 주인으로 만든다 | `Source/Runtime/Engine/Private/Animation/AnimSequenceTransformProviderData.cpp:1305-1312` | `CreateAnimSequenceTransformProviderDataInstance` |
| 활성 트랙은 매 프레임 평가 | `Source/Runtime/Renderer/Private/Skinning/AnimSequenceTransformProvider.cpp:1456-1491` | `EnumerateActiveTracks` |
| 뼈대 버퍼 크기 = 트랙 수 × 뼈 수 × 2 | `Source/Runtime/Renderer/Private/Skinning/SkinningSceneExtension.cpp:1336` | `AllocSpaceForPrimitive` |
| 알려진 문제 | [Epic 개발자 포럼](https://forums.unrealengine.com/t/mass-crowds-using-skinnedmeshinstance-leak-gpu-memory-every-wave-of-spawned-and-killed-entities-permanently-grows-the-animation-track-pool-5-8/2742508) | [references.md](../../Docs/Reference/references.md) 49행 |

### 2. 트랙 수가 늘 때의 번쩍임

| 확인한 것 | 파일과 줄 | 심볼 |
| --- | --- | --- |
| 트랙 수 = 트랙 풀 끝 번호 | `Source/Runtime/Engine/Private/Animation/AnimSequenceTransformProviderData.cpp:1341-1344` | `GetUniqueAnimationCount` |
| 트랙 수가 바뀌면 프록시에 알림 | 같은 파일 `:278-284` | `SetUniqueAnimationCount` |
| 인스턴스의 뼈대 자리 = 트랙 번호 × 2 | 같은 파일 `:1346-1354` | `GetSkinningDataOffset` |
| 트랙 수가 바뀐 컴포넌트를 할당 목록에 넣음 | `SkinningSceneExtension.cpp:1124-1135` | `FSkinningSceneExtension::FUpdater::PostMeshUpdate` |
| 옛 자리 해제 뒤 새 자리 할당 | `SkinningSceneExtension.cpp:1336-1352` | 같은 함수의 `AllocSpaceForPrimitive`(`:1220`) |
| 옛 → 새 복사 헤더 | `SkinningSceneExtension.cpp:1857-1866` | `FSkinningSceneExtension::FUpdater::PreGPUSceneUpdate` |
| 조각 모음과 모바일만 스테이징 버퍼, 그 밖은 같은 버퍼 | `SkinningSceneExtension.cpp:1908-1929` | 같다 |
| 셰이더의 가정과 같은 UAV 읽고 쓰기 | `Shaders/Private/Skinning/CopyPreviousTransforms.usf:11-12, 51-56` | `CopyPreviousTransformsCS` |
| 맞는 빈 구간이 없으면 합친 뒤 다시 찾음 | `Source/Runtime/Engine/Public/SpanAllocator.h:23-31, 63-69` | `FSpanAllocator::Allocate`, `Free` |
| 이웃 구간 합치기, 첫 맞춤 | `Source/Runtime/Engine/Private/SpanAllocator.cpp:58-122, 124-138` | `Consolidate`, `SearchFreeList` |

알려진 문제로 올라온 이슈는 찾지 못했다([engine-notes.md](../../Docs/Reference/engine-notes.md) 42절, 2026-11-15 검색). 이 글을 쓰며 다시 검색하지는 않았다. 그래서 본문은 "2026-11-15에 찾아본 범위에서"로 쓴다.

### 3. 가산 클립

| 확인한 것 | 파일과 줄 | 심볼 |
| --- | --- | --- |
| 압축 해제 문맥에 가산 여부를 넘김 | `Source/Runtime/Renderer/Private/Skinning/AnimSequenceTransformProvider.cpp:1082-1093` | `FAnimSequenceTransformProvider::UploadSequences` |
| 기준 포즈에서 시작해 풀린 자세를 그대로 씀 | 같은 파일 `:1095-1096, 1109, 1120-1121` | 같다 |
| 액터 단일 노드 재생은 기준 자세에 더함 | `Source/Runtime/Engine/Private/Animation/AnimSingleNodeInstanceProxy.cpp:331-352` | `AccumulateAdditivePose` |
| 원본 데이터는 가산이 아님 | `Source/Runtime/Engine/Private/Animation/AnimSequence.cpp:1842` | "Raw data is never additive" |

### 4. 쿼리 쓰기 선언의 기다림

| 확인한 것 | 파일과 줄 | 심볼 |
| --- | --- | --- |
| `mass.ForceInlineProcessorExecution` 기본 참, 읽기 전용 | `Source/Runtime/MassEntity/Private/MassProcessingQueueTypes.cpp:26, 31-33` | `bForceInlineProcessorExecution` |
| 직접 쓰기와 직접 읽기가 겹치면 전역 충돌 | 같은 파일 `:331-340` | `FQueuedProcessor::InitCrossChunkWriteContention` |
| 아키타입을 거르지 않고 모든 쿼리 쌍에 적용 | `MassProcessingQueue.cpp:861-873` | 쿼리 쌍 반복 |
| `ReadWrite`는 쓰기와 읽기 양쪽에 듦 | `MassProcessingQueueTypes.cpp:412-431` | `FDataAccess::SetFragmentWriteRequirements` |
| 쿼리 시작 전에 전역 칸을 잡음 | 같은 파일 `:640-644, 678-691` | `FProcessorWorker::WaitForQueryResources` |

### 5. Chaos 라인 트레이스

| 확인한 것 | 파일과 줄 | 심볼 |
| --- | --- | --- |
| 바디에 바로 쏘는 경로 | `Source/Runtime/Engine/Private/PhysicsEngine/Experimental/PhysInterface_Chaos.cpp:905-984` | `FPhysInterface_Chaos::LineTrace_Geom` |
| 그 경로의 캡슐 레이캐스트 | `Source/Runtime/Experimental/ChaosCore/Private/Chaos/Raycasts.cpp:181-396` | `Chaos::Raycasts::RayCapsule` |

원인 분기는 엔진 소스로 확인하지 못했다. 위 두 줄은 경로의 위치만이다([engine-notes.md](../../Docs/Reference/engine-notes.md) 53절).

## 엔진 코드 발췌의 줄 수

| 문제 | 발췌 | 줄 수(`// ...` 포함) | 코드 줄 |
| --- | --- | --- | --- |
| 1 | `MassVisualizationComponent.cpp` `EndVisualChanges` | 8 | 5 |
| 2 | `SkinningSceneExtension.cpp` `AllocSpaceForPrimitive` + `CopyPreviousTransforms.usf` | 5 + 1 | 3 + 1 |
| 3 | `AnimSequenceTransformProvider.cpp` `UploadSequences` | 4 | 3 |
| 4 | `MassProcessingQueueTypes.cpp` `InitCrossChunkWriteContention` | 7 | 7 |
| 합 | | 25 | 19 |

posting.md의 상한(글 하나에 엔진 코드 30줄)보다 작다. 문제 5는 원인 코드를 확인하지 못해 발췌하지 않았다.

## Lab 코드 위치

아래 줄 번호는 측정 빌드 de0e21b의 것이다(`git show de0e21b:<파일>`). `LabCrowdSubsystem.cpp`는 그 뒤 c12daf7에서 바뀌었다.

| 발췌와 문장 | 파일과 줄 | 심볼 |
| --- | --- | --- |
| 풀에서 꺼내 다시 쓰기(발췌) | `LabCrowdSubsystem.cpp:264-275` | `ULabCrowdSubsystem::SpawnEntities` |
| 다시 넣는 값(위치, 일련번호, 체력, 걷기 상태, 속도 0) | `LabCrowdSubsystem.cpp:331-349` | `ReinitPooledEntity` |
| 풀에 넣고 맵 밖에 세우기 | `LabCrowdSubsystem.cpp:321-329, 351-391` | `LabPoolParkLocation`, `RecycleAndRespawn` |
| 파괴 대신 풀로 | `LabMassProcessors.cpp:1463-1481` | `ULabMinionStateProcessor::Execute` |
| 상태나 시작 시각이 바뀌면 트랙에 직접 씀 | `LabIskmProcessor.cpp:35-60, 412, 431-449` | `LabWriteTrack`, `ULabIskmProcessor::Execute` |
| 트랙 풀 미리 늘리기(발췌) | `LabIskmProcessor.cpp:71-100`(발췌 `:89-97`) | `LabReserveTracks` |
| 가산 클립의 비가산 복제(발췌) | `Scripts/create-iskm.py:163-188`(발췌 `:174-175, 179, 182`) | `provider_clip` |
| 주 쿼리에서 위치를 빼고 따라잡기 쿼리로(발췌) | `LabMassProcessors.cpp:1180-1203, 1299-1302` | `ULabFarHitIskmProcessor` |
| 분리 프로세서도 위치를 읽기·쓰기로 선언 | `LabMassProcessors.cpp:766-769` | `ULabSeparationProcessor::ConfigureQueries` |
| 확인 트레이스의 시작점과 끝점 | `LabOrb.cpp:378-405`(`:384`, `:391`) | `ALabOrb::TraceCapsule` |

## 본문에 수치로 쓰지 않은 근거

총괄 묶음에 대응 조건이 없어 본문에는 방향만 썼다([posting.md](../../Docs/Guides/posting.md) "경위에는 수치를 쓰지 않는다").

| 본문의 문장 | 값 | 출처 | 지위 |
| --- | --- | --- | --- |
| 번쩍임이 고친 뒤 사라졌다 | 튄 프레임 42 → 0(확대 속도), 18 → 0(오브 시점 색) | V2.md "2026-11-15 W9 ISKM 번쩍임" | 개발 빌드, 900장 캡처 |
| 피격 프로세서의 실행 시간이 거의 0이 되었다 | 0.889 → 0.029ms(`exec_ms`) | V2.md "M6 코드 비용 줄이기" `devm6costpre`, `devm6costpost` | 개발 빌드, 1회씩 |
| 같은 수정의 Test 값 | A 위에 B를 켠 프레임 p95 +0.484 → +0.075ms | V2.md "묶음 `V2M6Cost`"(`V2M6Split`과 견줌) | 다른 묶음끼리라 참고 |
| 트레이스에서 분리 프로세서와 함께 끝났다 | 세 프레임 0.872, 0.768, 0.794ms | V2.md "M6 코드 비용 줄이기" "몫 1의 원인", `V2M6Trace-V0-s8-N10000-P1-r1` | Test 트레이스, 다른 묶음 |
| 겹침 질의는 캡슐을 찾았다, 시작점을 비키면 빗나감이 없었다 | 6,690 / 6,690, 1.7cm 비킨 시작점 0회 | V2.md "2026-11-17 Full4 캡슐 빗나감 진단" | 개발 빌드 |
| 바디에 바로 쏜 트레이스도 놓쳤다 | 6,690회 중 4,522회(68%) | 같은 절 | 개발 빌드 |

## 확인하지 않은 것

- 엔티티 풀을 끈 조건에서 번쩍임이 화면에 다시 보이는지. 트랙이 예약 용량의 합을 넘는다는 것은 계산으로 보였다. 컴포넌트별 트랙 수와 넘는 시각은 CSV에 없다.
- 16분 실행의 풀 끔에서 12분 구간에 GPU 메모리가 계단처럼 뛰는 까닭(트랙 버퍼의 재할당으로 추정, 엔진 코드로 확인하지 않았다).
- 풀 켬에서도 트랙이 분당 약 70개씩 느는 까닭. 풀이 비어 새로 만든 엔티티의 트랙으로 추정한다.
- 쿼리 기다림을 고친 효과는 총괄 묶음의 조건으로 재지 않았다. 다음 표현 프로세서가 같은 이유로 분리 프로세서를 기다리는 몫은 남아 있을 수 있다(V2.md "M6 코드 비용 줄이기"의 추정).
- 가산 클립을 압축 코덱이 풀 때 차이 값을 내놓는다는 것은 화면과 [engine-notes.md](../../Docs/Reference/engine-notes.md) 46절에 기댔다. 코덱 내부(`DecompressPose`)는 따라가지 않았다.
- 번쩍임 프레임에 `FSpanAllocator`가 실제로 겹친 자리를 준 것을 GPU 캡처로 보지는 않았다. 튄 프레임이 모두 트랙 수가 바뀐 프레임이었고, 트랙 수를 고정하자 사라진 것으로 판정했다(V2.md W9 절).
- Chaos `RayCapsule` 안에서 놓치는 분기. 정지 장면 빗나감의 시작점이 실제로 캡슐 로컬 Y ≈ 0인지는 직접 찍지 않았다.
- 포럼 글의 내용은 이 글을 쓰며 다시 읽지 않았다. 링크는 [references.md](../../Docs/Reference/references.md)의 것이다.
- 화면 세 장은 측정 패키지 de0e21b보다 앞선 개발 빌드다. 같은 코드 경로가 de0e21b에 남아 있다는 것은 위 Lab 코드 위치로 확인했다.
- 본문이 가리키는 글 00, 04, 05, 07의 폴더(`00-goals-and-method`, `04-quality-by-distance`, `05-cost-cuts-and-contributions`, `07-summary`)는 모두 있다.
