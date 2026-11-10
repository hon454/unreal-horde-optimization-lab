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

## M3b. 정비 + 새 기준선 — 큰 개체 추가

**목표:** 둘이다. (1) M3 뒤 점검(2026-11-09)에서 찾은 코드·측정 방법의 결함을 고친다. (2) 시즈 미니언 10%를 더해 "거리 외 중요도 기준"을 실험한다. 둘 다 모든 단계의 재측정을 요구하므로 한 마일스톤에서 한 번만 다시 잰다(2026-11-09 사용자 결정 A안. 대안 B "정비를 M3c로 분리"는 재측정이 두 번이라 버렸다).

**두 묶음.** 변경은 한 빌드에 넣고 측정을 둘로 나눠 비교마다 바뀌는 변수를 하나로 둔다.
- 묶음 1 "정비 기준선": 정비된 코드, 1종, 액터 콜리전 끔. M0\~M3 전 단계. 앵커는 M0 {1k, 5k}, M1\~M3 {1k, 5k, 20k, 50k}(Mass 단계의 60fps N을 다시 구한다) × 프리셋 2 × 3회.
- 묶음 2 "2종 시나리오": 같은 빌드, 같은 날, 2종. 묶음 1과 같은 조건. 비교 상대는 묶음 1.
- 수치로 효과를 남길 수정(쿼리 선언, ActorSync 필터 등)은 진단 변형으로 20k P1에서 묶음 1 기준점과 A/B를 한 번 잰다. 순수 버그(경합, 순서 의존)는 A/B 없이 고친다.
- 측정 방법 변경은 묶음 1부터 적용하고 [measurement.md](Guides/measurement.md)에 날짜와 함께 적는다([ADR-0002](Decisions/0002-freeze-measurement-infra.md)의 "바꾸면 그날 모든 단계를 다시 재고 적는다"). 글의 수치는 태그 값을 유지한다.
- 정비 항목은 하나씩 결정한다. 순서: 문서 정정 → 코드 수정 → 측정 방법 → 2종 시나리오 → M4 계획 재작성. 넣지 않는 항목은 [backlog.md](backlog.md)에 적는다.

**코드 정비(2026-11-09 사용자 결정 A안 "핵심 5").** 문서 정정은 f8702c2에서 끝냈다.
1. 표현 상태 분리: VAT 프로세서(`LabVatProcessor.cpp:39`)와 ActorSync가 로직 프래그먼트 `FLabMinionFragment`를 읽어 로직 프로세서와 쿼리 자원을 다툰다(20k P1 게임 스레드 스핀 대기 +2ms, `MassProcessingQueueTypes.h:776`). 상태(Walk/Hit/Dead)와 클립 시각을 표현 전용 프래그먼트로 나누고 로직만 쓴다. 저격 프로세서의 쿼리 밖 쓰기(`GetFragmentDataChecked`, `LabMassProcessors.cpp:234`)도 없앤다. 모든 접근을 쿼리에 선언한다([talk-notes.md](Reference/talk-notes.md) Shabel 12:32). 설계 전에 프로세서별 읽기·쓰기 표를 그린다. **A/B(`m3qold` vs 기준점, 20k P1 3회).**
2. 실행 순서 선언: Lab 로직 프로세서를 한 그룹으로 묶고 VAT·ActorSync에 `ExecuteAfter`를 선언한다. 지금은 State와 VAT 사이에 선언이 없어 솔버가 접근·등록 순서로 정한다(`MassProcessorDependencySolver.cpp:215-265`).
3. Hit → Dead 한 프레임 튐: State가 `StateTimeLeft`를 즉시 쓰고 태그는 지연해(`LabMassProcessors.cpp:294`) 그 프레임에 Hit 클립 시각이 음수가 된다. 시간과 태그를 같은 지연 명령에서 바꾼다. 1과 함께.
4. ActorSync 필터: 쿼리에 표현 필터가 없어 전 엔티티를 순회한다(`LabActorSyncProcessor.cpp:68,128`). 액터가 붙은 엔티티에 태그를 두고 거른다. **A/B(`m3syncall` vs 기준점, 20k P1 3회).**
5. M0 변형 `staticpawn`: CharacterMovement 틱 끔 + 애니 없음. 지금 `static`은 CharacterMovement를 유지해(`LabMinion.cpp:194`는 pawn만) "17배"가 컨테이너 전환만의 값이 아니다. 묶음 1에서 {1k, 5k} × P1 × 3회.
- 엔진 기본 프로세서 점검: 개발 빌드 Mass Debugger로 켜져 있는 프로세서 목록을 보고 쓰지 않는 것은 `DefaultMass.ini`에서 끈다(윤준기 16:10, talk-notes.md). 묶음 1 기준점에 포함.
- 같이 넣는 두 줄: `PendingSnipeTick`을 `std::atomic`으로(오브 틱과 프로세서가 잠금 없이 공유, `LabOrb.cpp:269`), `mass.commands.LockObserversDuringFlushing=1`(City Sample `DefaultEngine.ini:405`, 엔진 기본 0 `MassCommandBuffer.cpp:23`).
- 실험으로: E3b (8) 표현 LOD 값(아래).

**E3b (8) 표현 LOD 값(2026-11-09 사용자 결정 A안 "먼저 재고 결정").** `BaseLODDistance = VisibleLODDistance = 30m`, 히스테리시스 10%라(`LabCrowdSubsystem.cpp:96-97`, 엔진 기본) 레퍼런스 셋(엔진 기본, City Sample, MetaHuman Crowd `MetaHumanMassCrowdVisualizationTrait.cpp:34-49`)과 모두 다르다. P1은 30m 원의 절반이 카메라 뒤인데도 액터가 되고, 상한 배분은 거리 버킷 순이라 카메라 뒤 10m가 화면 안 25m보다 먼저 자리를 가져간다(`MassLODCalculator.h` `AdjustDistancesFromCountForRuntimeData`).
- 변형: `m3vis0`(Base High 0, Visible 30m), `m3vis15`(Base 15m, Visible 30m, MetaHuman 비율), `m3hys20`(30/30m, 히스테리시스 20%). 기준점은 `mass`. 기준점도 같은 섞기에 넣는다(3 변형 + 기준점 × 20k·5k × 3회 = 24회. 2026-11-09, 같은 묶음 안에서 실행 단위로 섞어야 A/B다).
- 조건: 20k·5k P1 × 3회, 실행 단위로 섞음. 지표: 프레임 p50·p95, Game, 액터 수, 전환 수/초(`Lab/ActorRebinds`를 summary 열로), 화면 안 액터 수(새 스탯).
- 화질: 개발 빌드 `m3vis0`로 카메라가 도는 60초 연속 캡처. 돌아볼 때 액터가 채워지는 프레임 수(`Scripts/transition-score.py`). 상한은 스폰 시간 분할 1.5ms/틱.
- 순서: **묶음 1은 이 24회로 시작한다.** 결과로 M3b 대표값(Base, 히스테리시스)을 그 자리에서 정하고, 그다음 전수 매트릭스를 돌린다. 대표값 변경이 재측정을 다시 부르지 않게 하기 위해서다. 묶음 1 ↔ 묶음 2 비교(종류 수)는 영향 없다.
- 2026-11-09 선행 묶음 뒤 결정(사용자): 대표값은 **`m3vis15`(화면 밖 15m, 화면 안 30m, MetaHuman Crowd와 같은 1:2)**. 조건부: **5k** 빠른 회전(90도 0.5초) 캡처로 `m3vis0`·`m3vis15`를 같은 방식으로 찍어 확인한다(2026-11-09 재검토. 20k 캡처에서는 `m3vis15`가 12회 중 12회 회복 미달이었지만, 세 변형 모두 액터 482\~493으로 상한 500이 꽉 찬 상태라 원인은 "상한이 차면 링 액터가 화면 안 액터를 뺏는 것"이고 설정 자체가 아니다. 5k는 상한이 남아 링이 등 뒤 액터 약 58개를 Game 약 1.05ms로 유지한다. 그래서 20k 결과는 대표값을 뒤집는 근거로 쓰지 않고 글 재료(상한 상향의 동기)로 쓴다. 5k에서 `m3vis15`의 회복이 `m3vis0`보다 빠르거나 같으면 확정, 5k에서도 미달이면 `m3vis0`. **5k 캡처 결과(2026-11-09, Worklog M3b "빠른 회전" 5k): `m3vis15` near_vat 최댓값 중앙값 21 / `m3vis0` 40, near_actors 최솟값 중앙값 119 / 99, 미달 0/12 둘 다. 확정.** 옛 M3 값(Base 30m)은 변형 `m3vis30`으로 남겨 24회의 기준점 자리에 둔다). 차이가 없어도 유지하되 글에 "화질로는 가르지 못했고 현업 모양(근접 링 + 시야 기준)을 따랐다"고 쓴다. `m3vis0`는 "가장 싸게 보이는 설정"으로 대조군에 남긴다. 근거: 1rpm 캡처에서 화면 안 30m 액터 중앙값 m3vis0 482 / 기준 211(상한 500), 눈에 띈 전환 둘 다 0%; 이 실험실의 판정은 Mass 쪽이라 "등 뒤 적 판정"은 근거가 아니고, 근거는 빠른 회전 화질과 현업 모양이다. 선행 묶음의 20k 프레임 수치는 환경 오염(Worklog "잡음 진단")으로 비교에 쓰지 않는다.
- 미룬 변형: `m3visoff`(Base Off 80m, 화면 밖은 80m부터 안 그림). 대표점 결정이 하나 더 늘어 M4 뒤 후보로 둔다.
**측정 방법(2026-11-09 사용자 결정 D안).** 묶음 1부터 적용. 규약 본문은 [measurement.md](Guides/measurement.md).
1. 앵커: M1\~M3 {1k, 5k, 20k, 30k, 50k} × P1·P2, M0 {1k, 5k} × P1·P2(budget·uro·pawn·staticpawn은 P1).
2. 대표 지표 N@16.7ms를 프레임 p95로. p50 N과 병목 자원을 병기. M0\~M3은 원본 CSV에서 다시 요약(`Scripts/summarize.py`, `plot.py` 수정).
3. 하위 실험 앵커 5k → 20k P1.
4. `Scripts/spikes.py`로 묶음마다 주기 스파이크 검사. 히치 정규화 없음.
5. CSV 메타데이터에 빌드 커밋 해시, 고정 CVar 값, GPU 드라이버 버전. `runs.csv` `build` 열. [pc-specs.md](Reference/pc-specs.md) 빈칸 채움.
6. 묶음 끝에 기준점 A/A 3회. A/B 하위 실험은 실행 단위로 섞음.
7. Insights 트레이스: 20k P1 단계별 병렬 1 + 직렬 1(`Scripts/run-bench.ps1 -Trace`).
8. 메모리 지표는 유지(`PhysicalUsedMB`는 프로세스 작업 집합, `CsvProfiler.cpp:4174`). GPU 메모리는 M5 `memreport`.
- 묶음 크기(추천 반영): 조건 약 50 × 3회 + 30k 18회 + A/A 3회 + 트레이스 8회 ≈ 170회, 약 4.2시간. 묶음 2도 같다.

- 확인만: 걷기 루프가 `Fmod(Local, Length)`로 End + 0.99 프레임까지 가 다음 클립 첫 행이 섞일 수 있다(`LabVatProcessor.cpp:26`, 가설). 개발 빌드에서 시간을 멈춰 확인하고 사실이면 고친다.

**바꾸는 것:** 개체 종류 1 → 2(소형 90 / 대형 10). 행동은 같고 크기만 다르다. `FLabUnitTypeParams` 값이 달라 아키타입이 갈린다. VAT 텍스처, ISM 컴포넌트, 액터 풀이 종류별로 생긴다.

**함께 바꾸는 설정:** 하이드레이션 액터의 캡슐 콜리전을 끈다(E3 (6) `m3nocol`, 20k P1 프레임 15.58 → 14.12ms, 판정은 Mass가 가져 캡슐을 쓰는 곳이 없다. 2026-11-09 사용자 결정). 재측정 표의 M3 칸이 개체 종류 하나만 다르도록 "1종 + 콜리전 끔" M3를 앵커(1k·5k × P1·P2 × 3회)에서 같이 잰다.

**저격 대상 규칙(2종 시나리오의 일부, 2026-11-09 사용자 결정 E안):** 저격 수와 간격(2초에 5명), 구간 배분(1 / 2 / 2)은 그대로다. **대형은 예약된 발에서만 쏘고 나머지 발은 소형만 고른다.** 저격 틱이 4의 배수면 200m 밖 구간의 1발, 4로 나눈 나머지가 2면 80\~200m 구간의 1발이 대형 전용이다(해시 최소 대형). 홀수 틱은 예약이 없다. 예약 구간 화면 안에 대형이 없으면 그 발은 소형으로 떨어진다. 틱 번호는 누적 시간으로 정해져 시드와 시각만으로 재현된다.
- 왜 이 규칙인가: 비율이 틀어지는 조건은 "저격 사망 중 대형 몫 s ≠ 10%" 하나다(정상 상태 `0.1 × (N/163 + 2.5) = L/163 + 2.5 s`, 수명 163초 = 490m ÷ 3m/s). 예약 발은 틱당 평균 0.5발 = 5발의 10%라 모든 N에서 L = 0.1 N이다. 버린 안: 구간당 대형 1발(s ≈ 0.6, 1k에서 대형 고갈), 반경 가중치(s ≈ 0.18, 1k에서 −33%), 가중치 + 비율 유지 재생성(비율은 맞지만 종류 순서가 사망 순서에 의존해 시드 재현성을 잃음), 10틱 중 1틱(드리프트 0이지만 대형 피격이 20초에 한 번).
- 의도: 대형 피격·사망이 4초마다 한 번, 항상 중·원거리에서 일어난다. 1종에는 대형이 없어 예약 발이 소형으로 떨어지므로 지금 규칙과 같고, 재측정 표의 변수는 "2종" 하나다. 촬영용 저격 인자는 넣지 않고 글을 쓸 때 다시 본다(backlog).

**2종 구성의 사실.** 시즈 에셋은 `../unreal-horde-references/AssetHost/.../Down_Minions/Meshes/Minion_Lane_Siege_Dawn`에 있고 `Scripts/migrate-assets.py`에 `siege` 세트를 더해 옮긴다. 스켈레톤이 달라(`Minion_Lane_Siege_Skeleton`) AnimBP를 하나 더 만들고 Start Position 바인딩을 다시 한다(Unreal MCP, `Scripts/bind-anim-start.md`). 피직스 에셋은 소형과 같은 수정. VAT는 종류별로 굽는다. ISM 설명자, 액터 클래스, 액터 풀이 종류별로 생긴다. `FLabUnitTypeParams`는 속도·체력 같고 `BoundRadius`만 다르다(메시 바운드로 잰 뒤 확정값에 적음). 아키타입이 둘로 갈린다(Mass Debugger 아키타입 창 스크린샷 후보). 초기 배치와 재생성은 일련번호 10의 배수가 대형(시드로 재현).

**실험 E3b**
- 재측정 표(1종 vs 2종, 마일스톤별).
- E3(3)에 (c) 크기 보정 추가(2026-11-09 사용자 결정): 중요도를 **화면 크기(바운드 반경 ÷ 카메라 거리)**로 바꾼다. 현업 Significance Manager의 기본형이고 크기와 거리를 한 수로 합친다. 비교 상대는 (3a) 거리 단계. 20k P1 3회, 지표는 예산이 모자랄 때 대형 액터가 매 프레임 갱신되는 비율(새 스탯: 대형 액터의 틱 건너뜀 수). 버린 안: 거리 단계 × 반경비(임의 계수).

## M4. Simulation LOD

**목표:** 로직도 거리별로 가볍게. **비교 상대는 M3b 대표점(2종, `m3mass`).** M4부터 기본 시나리오는 2종이다(`-LabTypes=2`가 코드 기본값. M3b 재측정이 끝난 뒤 바꾼다. 2026-11-10 사용자 결정 C1. 2종 비용은 M3 20k P1 +0.56ms, Worklog M3b "묶음 1·2 결과"). 2026-11-09에 엔진 소스로 전제를 확인하고 다시 썼다(사용자 결정 A안 "엔진 LOD + 자체 틱". 사실의 출처는 [engine-notes.md](Reference/engine-notes.md) 5절).

**엔진이 해 주는 것과 해 주지 않는 것.** `UMassSimulationLODTrait`은 거리 등급(High/Medium/Low/Off), LOD 태그, 청크 단위 틱 결정(`ShouldTickChunkThisFrame`), 누적 델타(`FMassSimulationVariableTickFragment::DeltaTime`)를 준다. 그 틀을 읽는 엔진 프로세서는 Off 내비게이션, 부드러운 회전, SimpleMovement뿐이다. 엔진 이동·조향·회피는 Off만 제외하고 매 프레임 프레임 델타로 돈다. 가변 틱과 태그는 기본으로 꺼져 있고, 기본 상한(100/200/300)이면 20k 중 약 19,400이 Off가 되며, **Off 개체는 매 틱 `MoveTarget.Center`로 스냅된다**(우리 Center는 오브라 켜는 순간 전부 순간이동). City Sample도 틱 절약을 엔진에 맡기지 않고 자기 프로세서가 청크 LOD를 읽어 간격을 정한다(`MassZoneGraphAnnotationFragments.cpp:34-48`).

**바꾸는 변수:** Lab 로직 프로세서의 틱 간격(`TickRates[High/Medium/Low]`). 시나리오는 바꾸지 않는다(넉다운은 backlog).

**만드는 것**
- Trait: `bEnableVariableTicking` 켬, `LODMaxCount` 무제한, 거리는 표현 밴드와 독립(초안 High 0\~80m / Medium 80\~200m / Low 200m\~, **Off 없음**). Off를 두지 않아 스냅과 이동 제외가 발동하지 않는다.
- Lab 로직 프로세서 넷(접근, 범위 피해, 저격, 상태)에 청크 필터 `ShouldTickChunkThisFrame`를 걸고 델타를 누적 델타로 바꾼다. 범위 피해(초당 피해 × 델타)와 상태 타이머가 건너뛴 시간을 보상한다. 표현(VAT·ActorSync)은 M3b 정비로 로직 프래그먼트를 읽지 않으므로 로직이 건너뛰어도 재생이 멈추지 않는다. 저격 프로세서는 틱 간격이 저격 간격(2초)보다 짧아야 한다.
- 엔진 이동·조향은 매 프레임 그대로 둔다. 이 비용은 M3b 트레이스(20k P1 병렬·직렬)로 먼저 보고, 크면 E4 (d)로 Low 밴드 이동을 우리 프로세서로 바꾼다(`FMassCustomMovementTag`, 누적 델타로 직진).

**실험 E4**
- (a) 끔 / 켬의 Game Thread, 프로세서별 ms(트레이스).
- (b) 틱 간격 스윕: Medium 0.25 / 0.5초, Low 0.5 / 1.0초. 먼 개체가 1초에 한 번 3m씩 뛰면 200m 밖(화면 높이 0.4%)에서도 보일 수 있다. 영상으로 확인한다.
- (c) 가까운 개체만 비켜 가기(충돌 회피): 회피 Trait을 켠다. 회피 프로세서는 High 태그만 처리하므로(`MassAvoidanceProcessors.cpp:301-303`) 80m 안만 회피한다. E1 (c)의 전체 회피(N=20k 프레임 57.7ms)와 비교한다. M2\~M3b는 회피 끔.
- (d) 선택: Low 밴드 이동을 우리 프로세서로. M3b 트레이스에서 이동·조향이 Game Thread의 큰 몫이면 한다.

**글 맥락(06):** Shabel은 틱 제한을 "OOP의 흔한 회피책"이라 했다(talk-notes.md 07:27). 글에서는 "쓰기 패턴 정리(M3b)가 먼저, 틱 간격은 그 뒤"로 쓴다.

**선택 M4b. StateTree + Signal:** 접근 → 피격 → 사망 전이를 태그 상태(M1 방식) 대신 StateTree로. 피해 적용 프로세서가 Signal로 깨운다. (a) 저격 1회당 깨어나는 엔티티 수 (b) 태그 vs StateTree 비용. 글 하나 분량이면 하고, 아니면 backlog. 강연 근거는 윤준기 21:33뿐이라 "선택" 유지(talk-notes.md).

**측정 조건:** N {1k, 5k, 20k, 30k, 50k}.

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
