# 마일스톤 계획

M0\~M5가 필수다. 마일스톤마다 바꾸는 최적화 변수는 하나고, 끝에 측정과 태그가 있다. 글은 완료 조건이 아니다([AGENTS.md](../AGENTS.md) "진행 방식"). 미룬 주제는 [backlog.md](backlog.md)에 있다. 측정 조건과 지표는 [measurement.md](Guides/measurement.md)를 따른다.

## 공통 완료 조건

1. Development 빌드와 작은 규모 실행이 성공한다.
2. Test 패키지로 그 마일스톤의 측정 조건(N × 프리셋 × 3회)을 돌려 `Measurements/M<N>/summary.csv`, `runs.csv`, 대표 스크린샷(P1, N=5k)이 남는다.
3. 이전 마일스톤의 앵커 조건과 비교한 한 줄을 STATUS.md에 적는다. 바뀐 변수가 하나임을 확인한다.
4. `Docs/Worklog/M<N>.md`에 경위를 적고, `backlog.md` "글 대기열"에 글 항목을 더한다.
5. 태그 `M<N>`.

60fps 같은 성능 목표는 완료 조건이 아니라 "예상"이다. 하드웨어가 못 미쳐도 측정이 남으면 완료다.

## M0. 기준선 — 액터 군중

**목표:** 비교의 출발점. 측정 인프라를 만들고 동결한다.

**만드는 것**
- 엔진 프로젝트, 모듈, 플러그인, 스크립트(`build`, `run-dev`, `package`, `run-bench`, `summarize.py`, `plot.py`).
- Paragon Minions 근접 미니언 한 종류. 피직스 에셋 수정.
- 평지 맵, 오브, 스포너, 관전 카메라와 프리셋 2종, 벤치마크 러너. 오브와 저격 궤적은 구체와 디버그 라인이다.
- 미니언 `ACharacter` 변형 셋. 변형은 실행 인자 `-LabVariant=`로 고른다.
  - `full`: `ACharacter` + 스켈레탈 메시 + AnimBP(걷기·피격·죽기). 대표 기준선.
  - `static`: 같은 `ACharacter`에 `UStaticMeshComponent`, 애니메이션 없음. M1의 "애니 없는 ISM"과 컨테이너(액터 vs Mass)만 다르다.
  - `pawn`: `full`에서 CharacterMovement를 빼고 위치를 직접 옮긴다. 가장 가벼운 액터의 천장(2026-11-07 추가).
  - `budget`: `full` + `USkeletalMeshComponentBudgeted` + Significance Manager 거리 단계(25/50/100/250m) + `a.Budget.BudgetMs 2`. 액터 위에 올릴 수 있는 최적화를 다 올린 상태. 변형 `uro`(URO만)도 인자로 둔다.
- 오브: 반경 8m 피해 범위 안의 개체는 중심에 가까울수록 센 초당 피해를 받는다. 2초마다 화면 안의 개체를 거리 구간별로(30\~80m 1, 80\~200m 2, 200m\~ 2) 결정적 해시(스폰 일련번호 + 저격 틱)로 골라 쏜다(`LabSnipe.h`). 피격은 피격 애니 뒤 즉사다.
- 스포너: 시작 시 디스크 전체에 정상 상태 분포(반지름당 개체 수 일정, 밀도 ∝ 1/r)로 N개를 놓는다. 사망한 개체는 가장자리 링에서 다시 생성한다. 시드로 초기 위치와 재생성 순서를 재현한다.

**고정하는 것(사용자 확정 뒤 measurement.md "확정값")**
- 맵 반경 R, 이동 속도, 피해 범위와 피해량, 저격 간격·수·구간 배분, 프리셋 좌표.
- 거리 밴드 경계(초안 30 / 80 / 200m)와 `LODMaxCount`. 미니언 실제 크기와 프리셋으로 정한다. **M3이 아니라 여기서 동결한다.** 밴드가 바뀌면 M2의 ISM 개체 수가 달라져 비교가 끊긴다.
- 프리셋의 역할([ADR-0003](Decisions/0003-two-camera-presets.md)): P1 오브 시점 = 대표(오브 뒤 15m·높이 9m에서 오브 너머를 보며 1 rpm으로 돈다. 한 바퀴가 측정 60초다. 근거리 액터부터 지평선까지 한 화면), P2 탑뷰 = 원거리 밴드 대조군(높이 100m, 가장 가까운 개체도 근거리 밴드 밖).

**실험 E0**
- 변형 `full` / `static` / `budget` / `uro` × N {250, 500, 1k, 2k, 5k} × 프리셋 2. 어디서 16.7ms를 넘는가. `budget`과 `uro`는 "액터로 할 수 있는 것을 다 해도 어디서 멈추는가"의 답이고 M3의 대조군이다.
- 예상: `full`은 1k\~2k에서 Game Thread가 16.7ms를 넘는다.

**완료:** 공통 조건 + ADR-0002 "측정 인프라 동결" 승인 + README "결과 한눈에 보기" 초판 + 글 대기열에 `00-testbed`, `01-baseline`.

## M1. Mass 코어 — 로직을 엔티티로

**목표:** 같은 이동·피해·사망 로직을 Fragment와 Processor로. 표현은 애니 없는 ISM(`UMassMovableVisualizationTrait`, 밴드 없이 모든 LOD가 같은 ISM). **비교 상대는 M0 `static`이다.**

**바꾸는 변수:** 컨테이너(액터 → Mass). 회피는 켜지 않는다(M0에도 없다).

**만드는 것**
- Fragment: `FLabTargetFragment`(목표 방향), `FLabHealthFragment`(float). 엔진 `FTransformFragment`, `FMassVelocityFragment`, `FMassMoveTargetFragment` 재사용. Tag: `FLabHitTag`, `FLabDeadTag`. Const Shared Fragment `FLabUnitTypeParams`(속도, 체력, 바운드 반경).
- Processor: `ULabApproachProcessor`(MoveTarget을 오브로), 엔진 스티어링, `ULabOrbContactProcessor`, `ULabOrbSnipeProcessor`, `ULabMinionStateProcessor`(피격·사망 타이머 → 지연 파괴 + 재생성). 피해 이벤트 엔티티와 `ULabDamageApplyProcessor`는 E1 (d)에서 더한다. CDO 자동 등록 프로세서 끄기.
- EntityConfig(실행 시 C++로 만든다, [ADR-0005](Decisions/0005-entity-config-in-code.md)), Spawner 연결(M0 스포너가 액터 대신 엔티티를 만든다. 분포·시드 로직은 같다).

**실험 E1**
- (a) M0 `static` vs M1: 컨테이너 전환의 Game Thread 차이. 대표 비교.
- (b) `mass.FullyParallel` 0 / 1.
- (c) 회피(`MassAvoidance` Trait) 끔 / 켬. **이 뒤 M2\~M5는 회피 끔으로 고정한다**(2026-11-08 사용자 결정. 처음 계획은 켬 고정이었으나 켜면 N=20k 프레임 16.0 → 57.7ms라 뒤 단계의 개선이 묻힌다). 가까운 개체만 비켜 가기는 M4에서 시험한다.
- (d) 저격 피해 전달: 직접 호출 vs 피해 이벤트 엔티티(수명 1프레임). 비치명 피해(체력 차감만)로 초당 1k / 10k. 사망·재생성 비용과 섞지 않는다.
- 부록(측정만, 글에는 한 줄): 프로세서 하나 vs 셋, 큰 프래그먼트 vs 작은 프래그먼트 셋. 2026-11-08 건너뜀(backlog.md).

**측정 조건:** N {1k, 2k, 5k, 20k, 50k}(1k·2k·5k는 E0 static과 겹치는 비교점). 예상: 50k가 로직만으로 16.7ms 안.

## M2. Representation LOD — ISM + VAT

**목표:** 먼 개체를 애니메이션이 있는 ISM으로. **비교 상대는 M1.**

**바꾸는 변수:** 표현(정지 ISM → VAT ISM). 대표 설정은 모든 거리(High / Medium / Low / Off)를 같은 VAT ISM으로 그린다. 바뀐 것이 "모든 조각상에 애니메이션이 붙었다" 하나다(2026-11-08 사용자 결정. 처음 계획은 High = None, Off = 없음이었으나 그러면 그리는 개체 수가 바뀌어 변수가 둘이 된다). High는 M3에서 액터로 바뀐다. 200m 밖을 지우는 효과는 E2 (c)에서 따로 재고 M3부터 쓸 방식을 그 결과로 정한다.

**만드는 것**
- AnimToTexture로 걷기·피격·죽기 베이크(Bone 모드 기본, Vertex는 메모리 비교용). VAT 머티리얼. 에디터 작업은 Python 커맨드렛 스크립트로 한다.
- `UMassUpdateISMProcessor`를 상속한 프로세서가 한 반복에서 위치(`UpdateISMTransform`)와 애니 정보(`AddBatchedCustomData`)를 쓴다. City Sample `MassCrowdUpdateISMVertexAnimationProcessor`와 같은 구조다. 엔진 프로세서와 이 프로세서 중 어느 쪽을 돌릴지는 시작 시 `-LabStage`로 정한다(M1은 엔진 것 그대로, M2부터 이 프로세서. CDO의 `SetShouldAutoRegisterWithGlobalList`, `MassProcessor.h:180-185`). 시점이 맞지 않으면 엔진 것을 ini에서 끄는 방식으로 물러난다(2026-11-08 사용자 결정).
- 저격으로 피격·사망 애니가 재생되고 끝나면 인스턴스가 제거되는지 확인.

**실험 E2**
- (a) M1 vs M2: Render Thread, GPU, 드로우 콜. 대표 비교. M1을 같은 날 다시 잰다.
- (b) Bone vs Vertex VAT 텍스처 메모리.
- (c) 200m 밖 지우기: (1) 거리만으로 지우기(확정값 "200m\~ 없음") (2) 화면 밖일 때만 200m에서 지우기(`BaseLODDistance` 200m, `VisibleLODDistance`는 맵 끝까지). 대표(지우지 않음)와 비교해 M3부터 쓸 방식을 고른다.
- (d) 저격 빈도 초당 사망 0 / 1k에서 인스턴스 추가·제거 비용.

**측정 조건:** N {1k, 5k, 20k, 50k}.

## M3. 액터 하이드레이션 + 애니메이션 예산

**목표:** 가까운 개체를 액터 + AnimBP로 바꾸고 그 티어의 비용을 제한한다. 프로젝트의 중심. **비교 상대는 M2.**

**바꾸는 변수:** 근거리 표현(VAT ISM → HighResActor). 200m 밖은 Off(그리지 않음)로 둔다(E2 (c), 2026-11-08 사용자 결정). 비교 상대는 M2 + 200m 밖 Off이고 같은 묶음에서 다시 잰다. 예산 기법은 E3 안에서 계단으로 비교하고, **M3의 대표점은 (1) 하이드레이션만**이다.

**만드는 것**
- 단계: 액터(0\~30m, 최대 500) → VAT 인스턴스(30\~200m) → Off. 저품질 액터 단계는 두지 않는다(2026-11-08 사용자 결정. City Sample은 MetaHuman 한 명이 매우 비싸 저품질 액터를 끼우지만, 메시 하나짜리 미니언은 줄어드는 몫이 작고 액터 비용은 URO·Budget으로 같은 액터 안에서 줄인다. 근거는 E3 (5)로 잰다).
- 액터: M0 캐릭터에서 CharacterMovement 틱만 끈 것(E0 `pawn`과 같은 부품, 같은 AnimBP). 엔진 풀(`UMassActorSpawnerSubsystem`)을 쓴다.
- 동기화: 우리 프로세서 하나가 매 프레임 액터 위치, 애니 상태, 죽기 진홍색을 쓴다(엔진은 액터를 붙이는 순간에만 위치를 맞춘다, `MassRepresentationProcessor.cpp:272`).
- 전환 시 VAT 재생 시각 ↔ AnimBP 시각 동기화: AnimBP 시퀀스 플레이어의 Start Position을 `ULabMinionAnimInstance::StartTime`에 묶었다(Unreal MCP, `Scripts/bind-anim-start.md`).
- 액터 머티리얼은 VAT 인스턴스와 같은 단순 머티리얼(기본 색·노멀, 죽기 진홍색)로 맞춰 전환 때 색이 튀지 않게 한다.
- 액터 메시를 `USkeletalMeshComponentBudgeted`로. 중요도는 `1 - LODSignificance / 3`으로 변환해 `SetComponentSignificance`에 넘긴다(Mass는 0이 High, Budget은 큰 값이 중요. engine-notes.md).
- 풀에 돌아간 액터는 `IMassActorPoolableInterface`에서 중요도 0, 틱 끔. Mass 풀링은 액터를 숨기기만 한다(engine-notes.md).

**실험 E3(계단, 조건 하나씩 더함)**
1. 하이드레이션만. 전환 횟수/초, 전환 히치, 팝핑 유무(영상).
2. + URO.
3. URO 대신 Budget Allocator(`BudgetMs` 2). Significance 소스 (a) 거리 단계(M0 `budget`과 같음) vs (b) Mass `LODSignificance` 변환값.
4. 풀 크기별 스폰 비용(부록).
5. 저품질 액터 단계 비교(5k P1, 참고): 15\~30m를 가벼운 액터로 둔 구성 vs 두지 않은 대표 구성. 글에서 City Sample과 다른 선택의 근거로 쓴다(2026-11-08 사용자 요청).

**측정 조건:** N {1k, 5k, 20k, 50k}. P1 오브 시점의 근거리(피해 범위 주변)가 이 마일스톤의 주 관찰점이다.

## M3b. 새 기준선 — 큰 개체 추가

**목표:** 시즈 미니언 10%를 더해 "거리 외 중요도 기준"을 실험한다. 시나리오 변경이므로 **M0\~M3 태그를 모두 다시 잰다**(앵커 N {1k, 5k} × 프리셋 2 × 3회).

**바꾸는 것:** 개체 종류 1 → 2(소형 90 / 대형 10). 행동은 같고 크기만 다르다. `FLabUnitTypeParams` 값이 달라 아키타입이 갈린다. VAT 텍스처, ISM 컴포넌트, 액터 풀이 종류별로 생긴다.

**함께 바꾸는 설정:** 하이드레이션 액터의 캡슐 콜리전을 끈다(E3 (6) `m3nocol`, 20k P1 프레임 15.58 → 14.12ms, 판정은 Mass가 가져 캡슐을 쓰는 곳이 없다. 2026-11-09 사용자 결정). 재측정 표의 M3 칸이 개체 종류 하나만 다르도록 "1종 + 콜리전 끔" M3를 앵커(1k·5k × P1·P2 × 3회)에서 같이 잰다.

**실험 E3b**
- 재측정 표(1종 vs 2종, 마일스톤별).
- E3(3)에 (c) 크기 보정 추가: 대형은 바운드 반경 비례로 중요도 가산. 예산이 모자랄 때 대형이 끝까지 매 프레임 갱신되는지.

## M4. Simulation LOD

**목표:** 로직도 거리별로 가볍게. **비교 상대는 M3b 대표점.**

**바꾸는 변수:** `UMassSimulationLODTrait`의 `TickRates[High/Medium/Low/Off]`. `UMassMovingAvoidanceProcessor`는 High LOD만 처리하므로 먼 개체의 회피는 자동으로 꺼진다(engine-notes.md). 시나리오는 바꾸지 않는다(넉다운은 backlog).

**실험 E4**
- (a) 끔 / 켬의 Game Thread, 프로세서별 ms.
- (b) 먼 개체 움직임의 끊김(영상, 틱 간격별).
- (c) 가까운 개체만 비켜 가기(충돌 회피): 회피 Trait을 켜고 Simulation LOD로 먼 개체의 회피가 빠지게 해(위의 High LOD 조건) E1 (c)의 전체 회피(N=20k 프레임 57.7ms)와 비교한다. M2\~M3에서는 회피를 끈 채로 둔다(2026-11-08 추가).

**선택 M4b. StateTree + Signal:** 접근 → 피격 → 사망 전이를 태그 상태(M1 방식) 대신 StateTree로. 피해 적용 프로세서가 Signal로 깨운다. (a) 저격 1회당 깨어나는 엔티티 수 (b) 태그 vs StateTree 비용. 글 하나 분량이면 하고, 아니면 backlog.

**측정 조건:** N {1k, 5k, 20k, 50k}.

## M5. ISKM

**목표:** 원거리 표현을 ISKM + `AnimSequenceTransformProviderData`로 바꿔 VAT와 비교. **비교 상대는 M4.**

**바꾸는 변수:** 원거리 표현(VAT ISM → ISKM).

**만드는 것**
- `FMassSkinnedMeshInstanceVisualizationMeshDesc`, `UMassConsumeInstancedSkinnedMeshAnimationProcessor` 경로. MetaHuman Crowd Sample 설정 참고(복사 금지).
- **사망 시 엔티티를 파괴하지 않고 재활용한다**(링으로 옮겨 체력 복원). ISKM 트랙 풀 해제 경로가 동적 개체에서 막혀 있다(engine-notes.md). M1부터 재활용으로 짜면 여기서 바꿀 것이 없다.
- ISKM ↔ 액터 전환 동기화.

**실험 E5**
- (a) VAT vs ISKM: GPU ms, 메모리, 드로우 콜, 전환 품질.
- (b) 생성·소멸 10만 회 뒤 GPU 메모리(재활용으로 누수가 막혔는지).
- 선택: N 100k.

**완료:** 공통 조건. 이 뒤 최신 빌드에서 전 단계를 한 날 연달아 재서(ADR-0004) README "결과 한눈에 보기"를 확정한다.

## 리스크

- **ISKM(M5):** 5.8.4에서도 트랙 풀 누수가 남아 있다. 재활용으로 우회하고, 막히면 VAT 결과로 마무리하고 ISKM은 기록만 남긴다.
- **Budget + 풀링(M3):** 숨긴 액터가 예산을 계속 쓴다. 풀 반환 시 중요도 0으로. 첫날 확인.
- **Paragon Minions:** 스켈레톤이 종류별로 다르고 피직스 에셋 기본값으로 늘어진다. 안 되면 UE5 Manny + 리타겟 애니로 대체.
- **Unreal MCP:** Experimental. 막히면 Python Editor Script, 그다음 사용자.
- **범위 확장 유혹:** 새 아이디어는 backlog.md에만.
