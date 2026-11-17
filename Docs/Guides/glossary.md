# 용어집

작업 세션과 글 세션이 글, README, 가이드를 쓰기 전에 읽는 작업용 가이드다. 독자용 풀이가 아니다. 독자에게 용어를 풀어 주는 일은 글 00이 맡는다.

- 근거가 있는 말만 적는다. 출처는 괄호에 둔다(파일, 사용자 결정 날짜). 근거를 찾지 못한 것은 "확인하지 못함"으로 적는다.
- 출처 문서와 이 문서가 어긋나면 출처 문서를 먼저 본다. 단 2절의 치환은 이 문서가 기준이다([V2-w14-plan.md](../Worklog/V2-w14-plan.md) 결정 8).
- 용어를 정한 사용자 결정은 [autonomous-decisions.md](../Worklog/autonomous-decisions.md) 끝의 "사용자 확인(2026-11-17)" 두 절과 [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 2.2절에 있다.
- 이 문서에서 공개 문서는 README, 글 본문, 사이트다(4절의 규칙이 적용된다). 가이드와 계획 문서도 같은 말을 쓴다.

## 1. 쓰는 말

"처음 나올 때" 열은 그 글(또는 README)에서 처음 쓸 때의 처리다. 새 용어는 한 문장으로 정의한다는 기본 규칙이 있다([posting.md](posting.md) 56행).

### 1.1 지표와 판정

| 용어 | 한 줄 뜻 | 처음 나올 때 |
| --- | --- | --- |
| 프레임 p50, 프레임 p95 | 측정 창에서 프레임 시간의 50번째, 95번째 백분위수. 표의 값은 같은 조건 세 실행의 중앙값이다(`frame_p50_ms`, `frame_p95_ms`) | 풀어 쓰지 않고 이 이름 그대로 쓴다. 글 00이 한 번 정의한다. 다른 글과 README의 괄호 정의 한 번은 계획안의 최소안이고 사용자 확정은 확인하지 못함(사용자 지시 2026-11-17, [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 0절) |
| 게임 스레드 시간, 렌더 스레드 시간, GPU 시간 | 스레드별 시간. p50만 쓴다(`game_p50_ms`, `render_p50_ms`, `gpu_p50_ms`) | "Game p50"을 쓰지 않고 "게임 스레드 시간(중앙값)"으로 쓴다. 셋을 함께 보인다([posting.md](posting.md) 57\~58행, [measurement.md](measurement.md) "지표") |
| 병목 | 게임·렌더·GPU 시간 중 가장 긴 쪽. 프레임 시간은 이 쪽이 정한다 | 그 글에서 처음 나올 때 한 문장([posting.md](posting.md) 58행) |
| 호드 총수 | 시나리오가 늘 유지하는 개체 수 N. 목표 총수는 1만이다 | "호드 총수 1만, 오브 시점 카메라"처럼 문장으로 쓴다([posting.md](posting.md) 59행) |
| 목표 판정 | 단계 8 이동 장면의 프레임 p95 중앙값 + 0.6ms ≤ 16.7ms | 한 문장 정의([ADR-0019](../Decisions/0019-step8-actor-cap-distance-and-fixed-skel-bounds.md) 결정 3, [measurement.md](measurement.md) "지표") |
| 16.7ms까지의 여유 | 16.7ms에서 프레임 p95를 뺀 값. 단계 3\~8에 쓴다. 예: "1만에서 프레임 p95 14.2ms, 16.7ms까지 여유 2.47ms" | 그대로 쓴다. 최대 N은 단계 1과 2, 그리고 최종 구성의 한 문장("약 1만 2천")에만 쓴다(사용자 결정, [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 결정 5) |
| 최대 N | 프레임 p95가 16.7ms를 넘기 직전의 호드 총수. 잰 총수 사이를 로그 보간하고 기능 미달 점은 뺀다. 단계 1과 2에만 쓴다 | 한 문장 정의. 단계 3\~8은 최대 N 대신 1만 기준의 프레임 p95와 16.7ms까지의 여유로 적는다. 값은 측정 기록의 표에 둔다([V2-w14-plan.md](../Worklog/V2-w14-plan.md) 결정 5) |
| 구별되지 않음 | 중앙값의 차이가 두 조건 범위(최댓값 − 최솟값) 중 큰 쪽보다 작거나 0.5ms 아래다. "구별되지 않았다"로 쓴다 | 한 문장 정의. 이 묶음에는 5회 반복이 없어 0.5ms 아래 차이는 범위를 넘어도 이렇게 쓴다([measurement.md](measurement.md) "차이 판정") |
| 기능 미달 | `UNMET`. 호드 총수가 1만이 아닌 점이 같은 일을 하지 못한 것. 그 점만 표시하고 최대 N 보간에서 뺀다 | 한 문장 정의. 총수 1만에서 같은 실패는 `FAIL`(묶음 무효)이라 공개 문서에 나오지 않는다([measurement.md](measurement.md) "실패 등급") |
| 겹친 쌍 | 몸통이 겹친 개체 쌍의 수. 0.5초마다 센 값의 평균(`overlap_mean`). 깊게 겹친 쌍은 `overlap_deep_mean` | 한 문장 정의. 프레임 시간이 다른 조건끼리 견주지 않는다([measurement.md](measurement.md) "실패 등급", [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 11절 17번) |
| 동등성 검사 | 단계 사이가 같은 일을 하는지 보는 검사: 살아 있는 총수, 초당 사망, 오브 30m 안 개체 수, 겹친 쌍, 캡슐 빗나감, 왕복 전환 | 글 00이 한 문단으로 정의한다([V2-w14-plan.md](../Worklog/V2-w14-plan.md) 3.2절, [measurement.md](measurement.md) "총괄 묶음 규약" 동등성 행) |
| 왕복 전환 | 2초 안에 같은 경계를 반대로 되넘은 수(초당). `sw_round*` | 한 문장 정의([measurement.md](measurement.md) 지표 표) |
| 같은 조건 다시 재기 | 묶음 끝에 단계 8 이동 장면을 회차 11\~13으로 다시 잰 것. 묶음 안 잡음의 기준이다. 글과 README의 이름이다. 가이드와 러너는 A/A라고 부른다 | 한 문장 정의([V2-w14-plan.md](../Worklog/V2-w14-plan.md) 0절, [measurement.md](measurement.md) "벤치마크 조건" 반복 행) |
| 참고 | 바뀐 변수가 둘 이상인 비교, 또는 총괄 묶음 밖에서 잰 값의 지위. 그래프에 잇지 않는다 | "참고"로 적고 섞인 변수를 함께 적는다([measurement.md](measurement.md) "차이 판정", "총괄 묶음 규약" 끝) |
| 측정 창, 워밍업 | 이동 장면은 워밍업 60초 + 측정 180초, 정지 장면은 15초 + 180초, 트레이스는 측정 60초 | 문장으로 쓴다([measurement.md](measurement.md) "벤치마크 조건" 시간 행) |
| 측정 기록 | `Posts/NN-이름/measurements.md`. 본문 수치의 값, 출처, 계산식, 확인하지 않은 것을 둔다 | 글 끝에서 링크([posting.md](posting.md) "본문과 측정 기록") |

### 1.2 장면과 시나리오

| 용어 | 한 줄 뜻 | 처음 나올 때 |
| --- | --- | --- |
| 이동 장면 | 오브가 반지름 57.3m, 높이 6m의 원을 2m/s로 돈다. 한 바퀴가 180초다. 라벨 `V0`. 가이드는 "이동 조건"이라고도 쓴다 | 한 문장. 정지 장면과 값을 직접 견주지 않는다([V2-w14-plan.md](../Worklog/V2-w14-plan.md) 0절, [measurement.md](measurement.md) "시나리오", "차이 판정") |
| 정지 장면 | 오브가 원점에 서 있다. 라벨 `V1`. 가이드는 "정지 조건"이라고도 쓴다. 단계 8과 기법별 기여에만 붙는다 | 같다 |
| 오브 시점 카메라, 탑뷰 | 오브를 따라가는 카메라(`P1`)와 오브 100m 위의 수직 카메라(`P2`). `P1`, `P2` 약어를 본문에 쓰지 않는다. 탑뷰는 단계 8 이동 장면 한 점만 참고로 잰다 | 문장으로 쓴다([posting.md](posting.md) 57, 59행, [measurement.md](measurement.md) "확정값" P2 행) |
| 오브 | 개체가 쫓는 구체. 반경 8m 안의 개체에 피해를 주고 저격으로 먼 개체를 맞힌다 | 영어 병기: 오브(Orb)([posting.md](posting.md) 53행, [measurement.md](measurement.md) "시나리오") |
| 저격 | 2초마다 5명. 화면 안의 산 개체를 거리 구간별로 결정적 해시로 골라 죽인다 | 한 문장([measurement.md](measurement.md) "확정값") |
| 소형, 대형 | 근접 미니언(90%)과 시즈 미니언(10%) | 한 문장([measurement.md](measurement.md) "시나리오") |
| 맵 밖 무리 | 맵 중심 500\~1,000m에서 코드가 굴리는 기록 N개. 재생성 자리와 배경 무리가 쓴다 | 한 문장([measurement.md](measurement.md) "확정값" 맵 반경 행, [code-tour.md](code-tour.md)) |

### 1.3 단계와 구성

| 용어 | 한 줄 뜻 | 처음 나올 때 |
| --- | --- | --- |
| 사다리, 단계 | 단계 1\~8. 한 단계는 앞 단계에서 한 묶음만 바꾸고 모든 단계가 같은 일을 한다. 실행 인자는 `-LabStage=1`\~`8`이다 | 한 문장. "같은 일 넷"(README 초안)과 "기능 넷"(사양)이 둘 다 쓰인다. 하나로 정하지 않았다([v2-spec.md](../v2-spec.md) 3절, [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 4.1절) |
| 표현을 더하는 단계 | 단계 4, 6, 7. 거리별 표현(VAT 인스턴스, ISKM, 배경 무리)을 더한다 | 한 문장([V2-w14-plan.md](../Worklog/V2-w14-plan.md) 0절, [v2-plan.md](../v2-plan.md) "사다리") |
| 비용을 줄이는 단계 | 단계 2, 3, 5, 8. 성격은 의도다. 단계 5는 잰 값에서 프레임 p95가 +0.31ms였고 구별되지 않았다 | 한 문장([V2-w14-plan.md](../Worklog/V2-w14-plan.md) 3.1절) |
| 출발점 | 단계 1. 성격은 둘 어디에도 넣지 않는다 | 확인하지 못함(공개 문서의 이름을 정한 기록을 찾지 못했다). 가이드는 "출발점"이다([v2-plan.md](../v2-plan.md) "사다리") |
| 최종 구성 | 단계 8(`-LabStage=8`, 기본값). 기법별 기여의 기준이다 | 한 문장([code-tour.md](code-tour.md) 머리, [measurement.md](measurement.md)) |
| 기법별 기여 | 최종 구성에서 기법 하나만 끄고 같은 조건으로 잰 차이. 단계 8과의 차이다. 제목은 "기법별 기여(최종 구성에서 하나씩 껐을 때)", 조건은 "\~를 끈 조건"이다 | 처음 쓰는 곳(README, 글 00)에서 한 문장으로 방법을 적는다. 그 문장에 조건 표의 이름 `ablation`을 적어 잇는다([V2-w14-plan.md](../Worklog/V2-w14-plan.md) 결정 7, 7.2절) |
| 하위 실험 | 기법을 끈 것이 아니라 바꾼 조건(`s8av`, `s1opt`). 조건 표에 이름이 있는 것만 총괄 묶음에서 잰다 | 한 문장([measurement.md](measurement.md) "벤치마크 조건") |
| 분리 프로세서, 엔진 회피 | 겹침 방지의 구현 둘. 단계 2는 엔진 회피(`-LabAvoid=1`), 단계 3부터 Lab 분리 프로세서(`ULabSeparationProcessor`)다. 겹침 방지는 기능 이름이다 | 한 문장([measurement.md](measurement.md) "확정값" 비켜 가기 행, [code-tour.md](code-tour.md)) |
| 방향 보간 병렬 사본 | 엔진 `UMassSmoothOrientationProcessor`와 같은 계산을 하는 Lab 프로세서(`ULabSmoothOrientationProcessor`). 단계 3부터 쓴다 | 한 문장([code-tour.md](code-tour.md) 선언 순서 표) |
| 액터 상한, 전환 거리 | 하이드레이션 액터 수의 상한 1,000과 액터로 바뀌는 카메라 거리 35m. 단계 2\~8이 같다 | 한 문장([ADR-0021](../Decisions/0021-unified-actor-cap-and-hydrate-distance.md)) |
| 최소 유지 시간 | 한 번 바뀐 개체가 2초 안에 바로 전의 표현으로 되돌아가지 않게 붙잡는다. 단계 4부터 | 한 문장([code-tour.md](code-tour.md) 68행) |
| 전환 대책 | 액터와 인스턴스 사이를 오갈 때 증상을 막는 다섯 가지(`-LabHydrateFixMask`, 단계 4부터 전부 켠다. 기본 31, 끄면 0). 비트 순서로 다섯은 (1) AnimBP 시작 시각 맞추기, (2) 액터의 포즈가 준비될 때까지 인스턴스를 그리고 되돌아갈 때 액터를 한 프레임 더 남기기, (3) 그동안 액터를 숨길 때 액터 숨김 대신 메인·깊이 패스·그림자에서만 빼기, (4) 엔진 이동(`TeleportActor`)에 캡슐 반높이 더하기, (5) 풀에 넣을 때 사망 진홍색 지우기다. 액터에 `FLabActorTag`를 붙이는 일은 이 다섯에 들지 않고 단계 2부터 늘 한다 | 한 문장([commands.md](commands.md) 87행, 다섯의 근거는 `Source/HordeOptLab/LabConfig.h` 446\~453행 `EHydrateFix`와 `LabConfig.cpp` 51행, 쓰는 곳은 `LabActorSyncProcessor.cpp`, `LabMinion.cpp`, `LabCrowdSubsystem.cpp` 128\~129행. 2026-11-17 소스 확인) |

### 1.4 기법과 표현

| 용어 | 한 줄 뜻 | 처음 나올 때 |
| --- | --- | --- |
| 하이드레이션 | 가까운 엔티티에만 진짜 액터를 붙이는 일. 단계 2부터이고 단계 1은 모든 개체가 액터다 | 영어 병기: 하이드레이션(Hydration, 가까운 엔티티에만 진짜 액터를 붙이는 일)([posting.md](posting.md) 53행) |
| 표현 LOD | Representation LOD. 무엇으로 그리나(액터, ISKM, 없음). "LOD"는 늘 수식한다 | 한 문장. Simulation LOD(얼마나 자주 계산하나), 메시 LOD(폴리곤)와 구별한다([posting.md](posting.md) 62행) |
| VAT 인스턴스 | VAT(Vertex Animation Texture)로 애니메이션을 재생하는 인스턴스드 메시. 단계 4\~5의 액터 밖 표현이고 `a8iskm`이 ISKM을 이것으로 되돌린다. 단계 2\~3의 정지 메시 인스턴스는 VAT 인스턴스가 아니다 | VAT를 한 문장으로 정의한다("애니메이션을 텍스처에 구워 정점 셰이더로 재생", [README.md](../../README.md) 5행). 이 낱말로만 부른다([code-tour.md](code-tour.md) 146행, [commands.md](commands.md) 28, 36, 92행) |
| ISKM | Instanced Skinned Mesh. GPU에서 스키닝하는 인스턴스 스켈레탈 메시. 단계 6부터 35\~200m의 표현이다 | 한 문장([README.md](../../README.md) 5행, [code-tour.md](code-tour.md) 요약) |
| 액터 애니메이션 예산 | 하이드레이션 액터의 메시를 `USkeletalMeshComponentBudgeted`로 바꾸고 Animation Budget Allocator의 예산을 4ms로 둔 것. 단계 4부터 | 한 일은 바꾼 것으로 쓴다([AGENTS.md](../../AGENTS.md) "문서와 글", [ADR-0016](../Decisions/0016-animation-budget-value.md), [commands.md](commands.md) 73, 200행) |
| Simulation LOD | 먼 개체의 로직을 드물게 돌리고 200m 밖은 자체 이동으로 대신한다. 단계 5부터 | 한 문장. 틱 간격이 아니라 프로세서를 건너뛴다([code-tour.md](code-tour.md) 62, 67행) |
| 엔티티 풀 | 죽은 엔티티를 파괴하지 않고 종류별 풀에 넣었다가 재생성 때 다시 쓴다. 움직이는 ISKM 엔티티를 파괴하면 트랙이 남는 엔진 문제의 우회다. 하이드레이션 액터 풀(engine-notes 4절)과 다르다. 단계 5부터 | 한 문장. 설명, 시각 자료, 문제가 있는 엔진 코드를 함께 싣는 것을 고려한다(사용자 요청 2026-11-17, [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 결정 2, [code-tour.md](code-tour.md) 57행, [engine-notes.md](../Reference/engine-notes.md) 3절) |
| 고정 바운드 | 두 가지다. ISKM 고정 바운드(`-LabFixedBounds`, 단계 6부터)는 ISKM 컴포넌트에 고정 상자를 `SetPrimitiveBoundsOverride`로 넣는다. 상자는 카메라를 따라 옮겨진다(`-LabBoundsStepM`). 액터 메시 고정 바운드(`-LabFixedSkelBounds`, 단계 8)는 하이드레이션 액터 메시의 `bComponentUseFixedSkelBounds`를 켜서 애니가 갱신될 때마다 피직스 에셋 바디로 바운드를 다시 셈하지 않고 메시 에셋의 고정 바운드를 쓰게 하고, `BoundsScale` 1.1을 곱한다. 글에서는 앞에 어느 쪽인지 붙인다 | 한 문장([commands.md](commands.md) 59, 67, 92, 101행, [measurement.md](measurement.md) 188행. 구분의 근거는 `LabCrowdSubsystem.cpp` 458\~473행 `ApplyFixedIskmBounds`, `LabMinion.cpp` 270\~277행, `LabConfig.h` 384\~392행. 2026-11-17 소스 확인) |
| 배경 무리 | 카메라에서 200m 밖의 개체를 Niagara 메시 입자로 그리는 것(단계 7). 200\~500m는 그려지지 않는 실제 엔티티의 자리에 입자가 걷고, 500m 밖은 맵 밖 무리의 기록을 그린다 | 한 문장([code-tour.md](code-tour.md) 87행, [plan.md](../plan.md) 267행). 호드 총수에 입자를 넣는지는 확인하지 못함 |

### 1.5 묶음과 조건

| 용어 | 한 줄 뜻 | 처음 나올 때 |
| --- | --- | --- |
| 총괄 묶음 | 글과 README의 수치를 만드는 묶음. 조건 표 전체를 잰다. 지금은 `Full4`(패키지 de0e21b, 태그 `v2-full4`)이다 | 공개 문서에서는 이름을 쓰지 않고 풀어 쓴다(4절). 측정 기록에는 이름과 빌드 해시를 쓴다([measurement.md](measurement.md) "총괄 묶음 규약") |
| 참고 묶음 | 총괄 묶음의 조건 표에 없는 조건으로 잰 묶음. 총괄 묶음의 값과 한 표나 그림에 잇지 않는다. README 그림에는 넣지 않되, 엔티티 풀의 긴 실행만 따로 된 그림 하나로 "참고" 표시를 달아 싣는다(사용자 결정 2) | 측정 기록에서만 쓴다([measurement.md](measurement.md) "참고 묶음 규약", [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 결정 2) |
| 작은 묶음, 개발 확인 | 측정 세 층의 둘이다. 작은 묶음은 작업 단위의 설계 근거(조건마다 3회), 개발 확인은 Development 실행이고 비교에 쓰지 않는다 | 공개 문서에 쓰지 않는다([ADR-0006](../Decisions/0006-three-measurement-tiers.md), [measurement.md](measurement.md) 31행) |
| 조건, 구성, 실행 | 조건은 점 하나(구성, 호드 총수, 장면, 카메라)다. 구성은 스위치 한 벌이다(`s1`\~`s8`, `a8*`). 실행은 조건을 한 번 잰 것이고 조건마다 3회 잰다 | 본문에서는 "조건", "세 번 잰 중앙값"으로 쓴다([measurement.md](measurement.md) "벤치마크 조건", [full-conditions.json](../../Scripts/full-conditions.json)) |
| 느린 실행 | 게임·렌더 스레드 중앙값이 그 조건 원 실행 중앙값보다 함께 2% 넘게 긴 실행. 새 회차로 한 번 다시 잰다 | 측정 기록에서만 쓴다([measurement.md](measurement.md) "묶음 판정") |

## 2. 쓰지 않는 말과 바꿔 쓸 말

이 절은 금지어를 싣는다. 옛 낱말이 이 문서에 나오는 것은 괜찮다.

### 2.1 치환표

| 쓰지 않는 말 | 바꿔 쓸 말 | 출처와 주의 |
| --- | --- | --- |
| 할당기 | Animation Budget Allocator(처음) 또는 Budget Allocator | 사용자 2026-11-17([autonomous-decisions.md](../Worklog/autonomous-decisions.md) 끝). 한 일은 바꾼 것으로 쓴다(AGENTS.md). **`FSpanAllocator`는 Budget Allocator가 아니다.** 엔진의 GPU 버퍼 구간 관리자다([engine-notes.md](../Reference/engine-notes.md) 42절, [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 7.1절). `FSpanAllocator`를 가리키는 옛 낱말은 `FSpanAllocator`로 쓴다 |
| 빼 보기 | 기법별 기여 | 제목은 "기법별 기여(최종 구성에서 하나씩 껐을 때)", 조건은 "\~를 끈 조건"이다. 처음 쓰는 곳에서 방법을 한 문장으로 적는다(사용자 2026-11-17, 결정 7). `a8shd`는 끈 조건이 아니라 80m 밖 그림자를 다시 켠 조건이다 |
| 품질 단계 | 표현을 더하는 단계 | 단계 4, 6, 7(사용자 2026-11-17) |
| 버는 단계 | 비용을 줄이는 단계 | 단계 2, 3, 5, 8(사용자 2026-11-17) |
| 조각상 | VAT 인스턴스 | 사용자 지시. 이 저장소의 추적 문서에서 옛 낱말이 쓰인 곳은 찾지 못했다(`git grep` 0건) |
| 품질 올리기(단계 8의 이름) | 남은 비용 줄이기 | 사용자가 2026-11-16에 바꿨다([v2-plan.md](../v2-plan.md) 28행). 같은 이름의 옛 작업 단위 W11은 그대로 둔다 |
| N일차, N주 안에 | 쓰지 않는다 | 일정 표현은 어느 문서에도 쓰지 않는다([AGENTS.md](../../AGENTS.md) "문서와 글"). 대체어는 정한 것이 없다 |

### 2.2 측정 약어

| 쓰지 않는 말 | 바꿔 쓸 말 | 출처와 주의 |
| --- | --- | --- |
| Game p50 | 게임 스레드 시간(중앙값) | [posting.md](posting.md) 57행 |
| `P1`, `P2` | 오브 시점 카메라, 탑뷰 | [posting.md](posting.md) 57행 |
| `N@16.7ms` | 60fps를 지키는 최대 개체 수 | [posting.md](posting.md) 57행. 이 표기는 GitHub가 메일 주소로 보고 링크를 건다. 꼭 써야 하면 코드 스팬에 넣는다. 본문 용어는 "최대 N"이다(1.1절) |
| 프레임 시간(느린 쪽 5% 경계) | 프레임 p95. 프레임 p50, 프레임 p95를 그대로 쓴다 | posting.md 57행이 아직 옛 규칙이다. 사용자 지시 2026-11-17이 이긴다. 고치는 일은 W14-7이 한다 |

### 2.3 적용 범위

| 바꾸는 곳(사람이 읽는 곳) | 두는 곳 |
| --- | --- |
| 글, README, `Docs/Guides/`, 계획과 사양, STATUS, [engine-notes.md](../Reference/engine-notes.md), [post-material.md](../post-material.md), 그림 제목, 스크립트와 소스의 주석 | Worklog, 승인된 ADR, 조건 표의 식별자(`ablation`, `a8*`, `full-conditions.json`의 키와 값), 그림 파일 이름(`full-ablation-*.svg`) |

- 출처: [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 결정 7, 8, 7.2절, 사용자 선택 2026-11-17.
- `full-conditions.json`은 설명 글만 바꾼다. 키와 값은 둔다([V2-w14-plan.md](../Worklog/V2-w14-plan.md) 7.2절).

## 3. 번역하지 않는 이름

정의한 뒤에는 그 이름으로만 쓴다. 한글로 옮겨 적은 영어 낱말은 처음 나올 때 괄호에 영어를 함께 적는다([posting.md](posting.md) 52\~53행). Animation Budget Allocator는 처음에 전체 이름을 쓰고 이후 Budget Allocator로 줄여 쓸 수 있다.

### 3.1 확정된 목록

| 이름 | 출처 |
| --- | --- |
| Mass, Representation LOD, ISM, VAT, ISKM, Animation Budget Allocator, URO | [AGENTS.md](../../AGENTS.md) "문서와 글" |
| Fragment, Processor, Archetype, Chunk, Simulation LOD, Significance | [posting.md](posting.md) 52행(위 일곱과 겹치는 이름은 같다) |

### 3.2 문서에서 쓰는 이름

`README.md`, `Docs/Guides/`, `Docs/plan.md`, `Docs/v2-plan.md`, `Docs/v2-spec.md`, `Docs/Reference/engine-notes.md`, `Docs/post-material.md`, `AGENTS.md`에서 `git grep`으로 센 파일 수다(2026-11-17). 목록에 없는 이름이라 번역하지 않는 쪽을 따른다. 확정은 아니다.

| 이름 | 파일 수 | 이름 | 파일 수 |
| --- | --- | --- | --- |
| Niagara | 9 | Mass Entity | 2 |
| Chaos | 3 | EntityConfig | 4 |
| Unreal Insights(`Insights`) | 4 | Trait | 4 |
| Lumen | 4 | Observer | 3 |
| AnimToTexture | 5 | ParallelFor | 2 |
| CharacterMovement | 5 | PSO | 1 |
| ASTPD | 3 | FSpanAllocator | 1 |

## 4. 공개 문서에 쓰지 않는 것

공개 문서는 README, 글 본문, 사이트다. 측정 기록(`measurements.md`)은 예외라 라벨과 빌드 해시를 쓴다([AGENTS.md](../../AGENTS.md) "측정과 비교", [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 12.1절 "글의 완료 확인").

| 쓰지 않는 것 | 예 | 대신 | 출처 |
| --- | --- | --- | --- |
| 묶음 이름 | `Full1`\~`Full4`, `Pool1`, `Ref1` | 측정 조건을 풀어 쓴다("호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값") | [AGENTS.md](../../AGENTS.md), [posting.md](posting.md) 74행 |
| 조건 라벨, 식별자 | `L1`\~`L8`, `A8-`, `V0-s8`, `a8hyd`, `s8av` | 5절의 문서의 말 | [AGENTS.md](../../AGENTS.md), [posting.md](posting.md) 87행 |
| 마일스톤 이름 | `M0`\~`M5`, `M3b`, `Q1`\~`Q8`, `W14` | 쓰지 않는다 | [AGENTS.md](../../AGENTS.md) |
| 다른 프로젝트 이름 | 참고 프로젝트(City Sample, MetaHuman Crowd Sample), 앞선 게임 | 쓰지 않는다 | [AGENTS.md](../../AGENTS.md), [posting.md](posting.md) 89행 |
| 작업 경위, 사족 | 커밋하지 않은 것, 이름 규칙, 쓰지 않은 값, 같은 말의 되풀이, 촬영과 측정의 사정, 일정 표현 | 측정 기록이나 Worklog에 둔다 | [AGENTS.md](../../AGENTS.md), [posting.md](posting.md) 60, 90행 |
| 면접, 회사 이름, 질문 번호 | | 동기는 "앞 프로젝트에서 측정하지 못했던 것"으로 쓴다 | [posting.md](posting.md) 64행 |
| 옛 판 | `v1`의 수치와 단계 | README "측정 방법" 끝에 한 문장만 둔다. "이전 판은 태그 `v1`에 있고 겹침 방지와 캡슐이 없어 수치를 잇지 않는다" | [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 2.1절 |
| PARAGON 상표 | 프로젝트 이름과 제목의 PARAGON | 쓰지 않는다. 범위는 프로젝트 이름과 제목이다 | [AGENTS.md](../../AGENTS.md) "구현" |

## 5. 문서의 말과 식별자의 대응

원본은 [full-conditions.json](../../Scripts/full-conditions.json)이다. 뜻은 조건 표, [v2-plan.md](../v2-plan.md), [measurement.md](measurement.md), [commands.md](commands.md), [code-tour.md](code-tour.md)에서 읽었다. `Source/`의 코드는 전환 대책, 고정 바운드, `-LabFarCull`만 읽었다(2026-11-17). 확인하지 못한 것은 그렇게 적었다.

### 5.1 단계(`kind` `step`)

단계 이름은 [v2-plan.md](../v2-plan.md) "사다리"의 것이다. json의 `title`은 영어다.

| 식별자 | 문서의 말 | 성격 | json `role` |
| --- | --- | --- | --- |
| `s1` | 단계 1 액터 기준선 | 출발점 | `start` |
| `s2` | 단계 2 Mass, 엔진 기본 경로 | 비용을 줄이는 단계 | `earn` |
| `s3` | 단계 3 직접 짠 분리(분리 프로세서, 방향 보간 병렬 사본) | 비용을 줄이는 단계 | `earn` |
| `s4` | 단계 4 애니메이션 표현(VAT 인스턴스, 액터 애니메이션 예산, 최소 유지 시간) | 표현을 더하는 단계 | `quality` |
| `s5` | 단계 5 Simulation LOD + 엔티티 풀 | 비용을 줄이는 단계 | `earn` |
| `s6` | 단계 6 ISKM | 표현을 더하는 단계 | `quality` |
| `s7` | 단계 7 배경 무리 | 표현을 더하는 단계 | `quality` |
| `s8` | 단계 8 남은 비용 줄이기 = 최종 구성 | 비용을 줄이는 단계 | `earn` |

- 단계 3의 이름은 "직접 짠 분리"로 쓴다([v2-plan.md](../v2-plan.md), [V2-w14-plan.md](../Worklog/V2-w14-plan.md) 3.2절). 조건 표의 영어 이름은 "Own separation logic"이다.

### 5.2 구성(`kind` `ablation`, `sub`, `extra`)

`ablation` 열일곱 개는 모두 기준이 `s8`이다. 문서의 말은 "\~를 끈 조건"이다(`a8shd`만 "다시 켠 조건").

| 식별자 | 문서의 말 | 인자 |
| --- | --- | --- |
| `a8hyd` | 하이드레이션을 끈 조건. 액터가 없어 캡슐, 액터 애니메이션 예산, 액터 메시 고정 바운드도 함께 빠지고 전 구간이 ISKM이 된다 | `-LabHydrate=0` |
| `a8bud` | 액터 애니메이션 예산을 끈 조건 | `-LabScreenSigBudget=0` |
| `a8fix` | 전환 대책을 끈 조건 | `-LabHydrateFixMask=0` |
| `a8hold` | 최소 유지 시간을 끈 조건 | `-LabRepHold=0` |
| `a8pool` | 엔티티 풀을 끈 조건 | `-LabEntityPool=0` |
| `a8sim` | Simulation LOD를 끈 조건. 200m 밖 자체 이동도 함께 빠진다 | `-LabSimLod=0` |
| `a8iskm` | ISKM을 끈 조건. VAT 인스턴스로 되돌린다. ISKM 고정 바운드, 트랙 예약, 장면 규칙 `-LabFarHitIskm`도 함께 빠진다 | `-LabIskm=0` |
| `a8bnd` | ISKM 고정 바운드를 끈 조건 | `-LabFixedBounds=0` |
| `a8far` | 200m 밖을 그리지 않는 일을 끈 조건. 표현 LOD의 Off 거리가 200m에서 1,000m(맵 반경의 두 배)로 옮겨져 맵 안 개체(카메라에서 최대 515m)가 Off에 들지 않는다. 그래서 200m 밖 엔티티가 ISKM(Low, 그림자 끔)으로 맵 끝까지 그려진다. 입자는 켠 채지만 맵 안에는 표현 None 엔티티가 없어 입자는 맵 밖 무리의 기록(500\~1,000m)만 그린다. Simulation LOD의 200m 경계는 그대로다(`LabConfig.h` 354, 410\~418행, `LabCrowdSubsystem.cpp` 135행, `LabCrowdFxProcessor.cpp` 221행. 2026-11-17 소스 확인) | `-LabFarCull=0` |
| `a8fx` | 배경 무리를 끈 조건. 장면 규칙 `-LabFarHitIskm`과 맵 밖 기록의 그리기도 함께 빠진다 | `-LabCrowdFx=0` |
| `a8sep` | 겹침 방지(분리 프로세서)를 끈 조건 | `-LabSeparate=0` |
| `a8cap` | 액터 캡슐을 끈 조건 | `-LabActorCollision=0` |
| `a8shd` | 80m 밖 그림자를 다시 켠 조건. 끈 조건이 아니다 | `-LabFarShadow=1` |
| `a8fsb` | 액터 메시 고정 바운드를 끈 조건. 피직스 에셋 바운드로 돌아간다 | `-LabFixedSkelBounds=0` |
| `a8res` | ISKM 트랙 예약을 끈 조건 | `-LabIskmTrackReserve=0` |
| `a8fxf` | 입자가 오브를 따라가는 일(외삽 목표를 매 프레임 갱신)을 끈 조건 | `-LabFxOrbFollow=0` |
| `a8stp` | ISKM 바운드 상자가 카메라를 따라가는 일을 끈 조건 | `-LabBoundsStepM=0` |
| `s8av` | 하위 실험. 분리 프로세서 대신 엔진 회피를 쓴 조건. 끈 조건이 아니라 바꾼 조건이다 | `-LabAvoid=1 -LabSeparate=0` |
| `s1opt` | 하위 실험. 단계 1 액터에 Animation Budget Allocator(4ms)를 더한 조건 | `-LabStage=1 -LabActorOpt=1` |
| `s8iso` | 후보. 단계 8에 `-LabQueryIsolate=1`(Chaos CVar `p.Chaos.AccelerationStructureIsolateQueryOnlyObjects` 1)을 더한 조건. 기본 묶음에 없고 `Full4`의 `summary.csv`에 행이 없다 | `-LabQueryIsolate=1` |

- 근거: [commands.md](commands.md) 84\~103행, [measurement.md](measurement.md) 149\~150, 185\~190행, [code-tour.md](code-tour.md) 87행.
- `a8hyd`, `a8iskm`, `a8fx`, `a8pool`처럼 다른 것이 함께 빠지는 행은 "참고"로 적고 함께 빠지는 것을 적는다([measurement.md](measurement.md) 185\~190행).
- 전체 18행(`ablation` 17 + `s8av`)의 설명을 쓰는 일은 W14의 작업이다([V2-w14-plan.md](../Worklog/V2-w14-plan.md) 4.3절).

### 5.3 묶음 종류(`groups`)

| 식별자 | 문서의 말 | 내용 |
| --- | --- | --- |
| `ladder` | 사다리, 여덟 단계 | 8단계 × 1만 × 이동 장면 × 3회 |
| `ablation` | 기법별 기여 | 열일곱 행 × 1만 × 이동 장면 × 3회. 기준은 `s8` |
| `still` | 정지 장면의 기법별 기여 | `s8`과 열다섯 행(`a8fxf`, `a8stp` 없음) × 정지 장면 × 3회 |
| `sub` | 하위 실험 | `s8av` × 이동 장면 × 3회 |
| `topview` | 탑뷰 참고 한 점 | `s8` × 탑뷰 × 이동 장면 × 3회. 사다리와 그림에 넣지 않는다 |
| `maxn`, `maxn_mid`, `maxn_low` | 최대 N 보조 점 | 단계 2는 2,500과 5천, 단계 3\~8은 1.25만과 2만. 공개 문서의 최대 N은 단계 1과 2에만 쓴다 |
| `actor` | 액터 보조 점 | 단계 1과 `s1opt` × {500, 1천, 2천}. 단계 1의 최대 N을 만든다 |
| `aa` | 같은 조건 다시 재기 | `s8` × 이동 장면, 회차 11\~13 |
| `trace`, `traceser` | 트레이스(병렬, 직렬) | 측정 60초. 수치는 비교에 쓰지 않는다. `traceser`는 `-LabFullyParallel=0` |
| `extra` | 후보 | `s8iso`. `-Only extra`로만 돈다 |
| `capture`(`groups`가 아닌 `capture` 블록) | 단계 화면 | 단계 1\~8을 HUD를 켜고 촬영한다. 요약에 넣지 않는다 |

- 단계 1의 최대 N이 `actor` 점에서 나온다는 것은 [measurement.md](measurement.md) 26행으로 읽었다. 값이 어느 구성(`s1`과 `s1opt`)에서 나오는지는 확인하지 못함.

### 5.4 라벨

- 형식: `<묶음>-<V0|V1>-<구성>-N<N>-P<1|2>-r<회차>`. `V0` 이동 장면, `V1` 정지 장면, `P1` 오브 시점 카메라, `P2` 탑뷰([measurement.md](measurement.md) "총괄 묶음 규약" 라벨 행).
- 회차: 같은 조건의 반복 번호다. 11\~13은 같은 조건 다시 재기(`-aa`), 4\~6은 실패 재측정, 7\~9는 느린 실행 재측정, 21 이상은 두 번째 재측정이다.
- 접두사: `<묶음>trace-`(병렬 트레이스), `<묶음>traceser-`(직렬 트레이스), `<묶음>shot-`(촬영).
- 요약 행 이름: `V0-s8-aa`는 같은 조건 다시 재기, `V0-s8av`는 하위 실험이다(`summary.csv`).
- 라벨과 식별자는 측정 기록에만 쓴다(4절).
