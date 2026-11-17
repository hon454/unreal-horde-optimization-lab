# 개발 지침

UE5의 Mass로 수천\~수만 개체의 군중을 만들고 프로파일링할 때 쓰는 기술 팁 모음이다. 호드 1만이 맵 위를 원으로 도는 목표(오브)를 사방에서 쫓는 벤치마크를 액터에서 시작해 여덟 단계로 다시 만들며 얻었다. 엔진 동작은 UE 5.8.4 소스에서 확인했고, 값은 Test 구성 패키지에서 쟀다(측정 PC는 [pc-specs.md](../Reference/pc-specs.md)).

## 읽는 법

- 항목 제목은 할 일이다. 본문은 증상(이 팁이 필요한 때를 알아보는 법), 까닭(엔진이 왜 그렇게 동작하는가), 하는 일(바꾼 코드와 설정) 순서의 짧은 문단이다. 끝 줄에 근거 링크를 둔다.
- 값은 따로 적지 않으면 호드 1만, 이동 장면(목표가 2m/s로 원을 돈다), 오브 시점 카메라, 2560×1440에서 같은 조건을 세 번 잰 중앙값이다. 정확한 값은 [가장 최근 총괄 측정의 요약 표](../../Measurements/Full4/summary.csv)와 각 글의 측정 기록에 있다.
- 프레임 p95는 측정 창 프레임 시간의 95번째 백분위수(느린 쪽 5%의 경계)다. 두 조건의 중앙값 차이가 0.5ms 아래이거나 세 실행의 범위보다 작으면 "구별되지 않는다"고 적는다.
- 최종 구성은 모든 기법을 켠 구성이다. 카메라 35m 안은 하이드레이션(Hydration, 가까운 엔티티에만 진짜 액터를 붙이는 일) 액터, 35\~200m는 ISKM, 200m 밖은 Niagara 입자로 그린다. 프레임 p95는 14.2ms이고 병목은 게임 스레드다.
- 기법별 기여는 최종 구성에서 기법 하나만 끄고 같은 조건으로 잰 차이다. "끈 조건"은 이것을 가리킨다.
- 다른 프로젝트에 그대로 통하는지 확신하지 못한 것은 "이 프로젝트의 조건에서"라고 적었다. 소스로 원인을 찾지 못한 관측도 그렇다고 적었다.

## 목차

[액터 전환을 만들 때](#액터-전환을-만들-때)

- [하이드레이션 액터의 몫부터 잰다](#하이드레이션-액터의-몫부터-잰다)
- [액터 경계에는 최소 유지 시간을 둔다](#액터-경계에는-최소-유지-시간을-둔다)
- [풀에 넣는 액터는 틱과 중요도와 콜리전을 끈다](#풀에-넣는-액터는-틱과-중요도와-콜리전을-끈다)
- [액터 메시에는 URO와 Budget Allocator 가운데 하나를 고른다](#액터-메시에는-uro와-budget-allocator-가운데-하나를-고른다)
- [액터 캡슐은 QueryOnly와 전용 트레이스 채널로 켠다](#액터-캡슐은-queryonly와-전용-트레이스-채널로-켠다)
- [액터 메시에 고정 바운드를 쓴다](#액터-메시에-고정-바운드를-쓴다)
- [캡슐을 겨누는 라인 트레이스는 시작점을 정면 평면에서 비킨다](#캡슐을-겨누는-라인-트레이스는-시작점을-정면-평면에서-비킨다)

[인스턴스와 입자를 쓸 때](#인스턴스와-입자를-쓸-때)

- [먼 개체는 화면 밖 여부보다 거리로 먼저 지운다](#먼-개체는-화면-밖-여부보다-거리로-먼저-지운다)
- [움직이는 인스턴스는 Lumen 장면에서 뺀다](#움직이는-인스턴스는-lumen-장면에서-뺀다)
- [Mass 인스턴스의 그림자는 설명자에서 켠다](#mass-인스턴스의-그림자는-설명자에서-켠다)
- [ISKM 컴포넌트에 카메라를 따라가는 고정 바운드를 넣는다](#iskm-컴포넌트에-카메라를-따라가는-고정-바운드를-넣는다)
- [움직이는 ISKM 엔티티는 파괴하지 말고 풀로 다시 쓴다](#움직이는-iskm-엔티티는-파괴하지-말고-풀로-다시-쓴다)
- [ISKM 트랙 풀은 처음에 용량까지 늘려 둔다](#iskm-트랙-풀은-처음에-용량까지-늘려-둔다)
- [ISKM에는 가산 클립 대신 가산을 끈 복제본을 넣는다](#iskm에는-가산-클립-대신-가산을-끈-복제본을-넣는다)
- [AnimToTexture Bone 방식 메시의 바운드는 정점 범위로 넓힌다](#animtotexture-bone-방식-메시의-바운드는-정점-범위로-넓힌다)
- [Niagara 메시 입자의 VAT 프레임은 Dynamic Parameter로 넘긴다](#niagara-메시-입자의-vat-프레임은-dynamic-parameter로-넘긴다)

[Mass 프로세서를 짤 때](#mass-프로세서를-짤-때)

- [엔진 프로세서가 한 스레드로 도는지 먼저 본다](#엔진-프로세서가-한-스레드로-도는지-먼저-본다)
- [개체마다 독립인 엔진 프로세서는 청크 병렬 사본으로 바꾼다](#개체마다-독립인-엔진-프로세서는-청크-병렬-사본으로-바꾼다)
- [게임 스레드 프로세서의 주 쿼리에 위치 쓰기를 선언하지 않는다](#게임-스레드-프로세서의-주-쿼리에-위치-쓰기를-선언하지-않는다)
- [프로세서 단위 서브시스템 선언이 다른 프로세서를 막는지 본다](#프로세서-단위-서브시스템-선언이-다른-프로세서를-막는지-본다)
- [프로세서별 시간은 직렬 트레이스로 가른다](#프로세서별-시간은-직렬-트레이스로-가른다)
- [틱 간격을 늘리기보다 큰 몫의 프로세서를 쿼리에서 뺀다](#틱-간격을-늘리기보다-큰-몫의-프로세서를-쿼리에서-뺀다)
- [결정적 해시로 대상을 고를 때는 `HashCombineFast`의 결과를 섞는다](#결정적-해시로-대상을-고를-때는-hashcombinefast의-결과를-섞는다)

[프로파일링할 때](#프로파일링할-때)

- [프레임 p95와 세 스레드 시간을 함께 보고 병목을 정한다](#프레임-p95와-세-스레드-시간을-함께-보고-병목을-정한다)
- [병목이 아닌 스레드가 준 것은 프레임 이득으로 세지 않는다](#병목이-아닌-스레드가-준-것은-프레임-이득으로-세지-않는다)
- [게임 스레드 시간의 차이만으로 비용을 말하지 않는다](#게임-스레드-시간의-차이만으로-비용을-말하지-않는다)
- [프레임이 세 스레드보다 길면 게임 스레드가 기다린 작업자를 찾는다](#프레임이-세-스레드보다-길면-게임-스레드가-기다린-작업자를-찾는다)
- [액터 군중은 불린 횟수와 한 번의 시간으로 나눠 본다](#액터-군중은-불린-횟수와-한-번의-시간으로-나눠-본다)
- [Insights의 Timers 합계는 스레드를 골라 읽고 불린 횟수로 나눈다](#insights의-timers-합계는-스레드를-골라-읽고-불린-횟수로-나눈다)
- [기법 하나만 끄고 재서 기여를 가른다](#기법-하나만-끄고-재서-기여를-가른다)
- [빨라진 조건이 같은 일을 하는지 본다](#빨라진-조건이-같은-일을-하는지-본다)

## 액터 전환을 만들 때

### 하이드레이션 액터의 몫부터 잰다

Mass로 옮긴 뒤에도 게임 스레드가 병목이고 무엇이 큰지 모를 때 먼저 할 일이다. 하이드레이션 액터는 액터마다 고정비를 얹는다. 메시 틱과 애니메이션, 캡슐 바디의 이동, 엔티티 위치를 액터에 넣는 동기화가 액터마다 매 프레임 돈다. 표현 LOD의 액터 상한이 걸리면 엔진이 액터 거리를 당기므로, 그때는 전환 거리보다 상한이 이 비용을 정한다.

같은 빌드에서 하이드레이션만 끈 조건을 재 둔다. 최종 구성(액터 평균 578개)에서 하이드레이션을 끄면 프레임 p95가 6.98ms 짧다. 게임 스레드가 11.9ms에서 5.23ms로 줄어 병목이 GPU(6.21ms)로 바뀐다. 이 조건은 캡슐, 액터 애니메이션 예산(Animation Budget Allocator로 액터 메시의 애니메이션에 4ms 예산을 둔 것), 액터 메시 고정 바운드도 함께 빠지는 참고 값이다. 캡슐만 끈 조건도 1.86ms 짧다. 이 프로젝트는 가까운 개체의 캡슐이 필요한 기능이라 액터를 남기고 액터 하나의 비용을 줄였다(아래 캡슐, 고정 바운드, Budget Allocator 항목).

근거: [글 05 "기법별 기여"](../../Posts/05-cost-cuts-and-contributions/README.md), [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md), [engine-notes 19절, 43절](../Reference/engine-notes.md)(상한이 거리를 당기는 방식, 액터 동기화 경로), [`LabActorSyncProcessor.cpp`](../../Source/HordeOptLab/LabActorSyncProcessor.cpp)

### 액터 경계에는 최소 유지 시간을 둔다

액터 경계 근처의 개체가 액터와 인스턴스 사이를 짧게 오가며 전환 순간의 튐을 되풀이한다. 엔진 표현 LOD의 거리 히스테리시스(기본 10%)는 한 단계 가까운 구간으로 들어올 때만 걸리고, 계산 반복이 High(0번)에서 멈춰 액터 경계에는 없다. 상한이 당긴 경계도 같은 함수를 쓴다. 화면 안팎 판정의 히스테리시스(`DistanceToFrustumHysteresis`)는 기본값이 0이고, 최소 유지 시간에 해당하는 엔진 설정은 없다.

Lab 프로세서를 이번 프레임의 LOD가 정해진 뒤, 표현이 바뀌기 전 자리에 두고, 2초 안에 바로 전 표현으로 돌아가려는 개체의 LOD를 되돌린다. 최종 구성에서 이 규칙만 끄면 왕복 전환(2초 안에 같은 경계를 반대로 되넘은 수)이 초당 4.34건 생기고 모두 액터 경계에서 난다. 켜면 0건이고 프레임 p95는 구별되지 않는다.

`Source/HordeOptLab/LabMassProcessors.cpp`, `ULabRepHoldProcessor::ULabRepHoldProcessor`

```cpp
ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::LOD);
ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::VisualizationProcessing);
```

근거: [engine-notes 40절](../Reference/engine-notes.md), [글 04 "바꾼 것"](../../Posts/04-quality-by-distance/README.md), [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md), [`LabMassProcessors.cpp`](../../Source/HordeOptLab/LabMassProcessors.cpp)

### 풀에 넣는 액터는 틱과 중요도와 콜리전을 끈다

Mass 액터 풀로 돌려보낸 액터가 숨은 채 틱하거나, 마지막 자리에 남은 캡슐이 트레이스에 맞는다. `UMassActorSpawnerSubsystem`의 풀은 액터를 숨기기만 한다. 컴포넌트 등록, 틱, 콜리전은 그대로이고 `USkeletalMeshComponentBudgeted`는 Budget Allocator에 등록된 채 남는다. 꺼낼 때는 숨김을 풀고 엔티티 변환(스케일 1)으로 옮긴 뒤 `PrepareForGame`을 부르며, `BeginPlay`는 다시 부르지 않는다.

`IMassActorPoolableInterface`의 `PrepareForPooling`에서 메시 틱, 중요도, 액터 콜리전을 끄고 `PrepareForGame`에서 되돌린다. 액터에 넣어 둔 스케일도 `PrepareForGame`에서 다시 넣는다. 이 처리만 끈 조건은 재지 않았다.

`Source/HordeOptLab/LabMinion.cpp`, `ALabMinionMassActor`와 하위 클래스의 `PrepareForPooling_Implementation`(둘을 합쳐 줄였다)

```cpp
GetMesh()->SetComponentTickEnabled(false);
SetActorEnableCollision(false); // 엔진 풀은 콜리전을 그대로 둔다
CastChecked<USkeletalMeshComponentBudgeted>(GetMesh())->SetComponentSignificance(0.f);
```

근거: [engine-notes 4절, 5절](../Reference/engine-notes.md)("풀에서 꺼낸 액터의 변환"), [engine-notes 38절](../Reference/engine-notes.md)("Mass 액터 풀의 콜리전"), [`LabMinion.cpp`](../../Source/HordeOptLab/LabMinion.cpp)

### 액터 메시에는 URO와 Budget Allocator 가운데 하나를 고른다

Animation Budget Allocator를 켠 액터에 URO(Update Rate Optimization)도 함께 걸려 있다고 보면 결과를 잘못 읽는다. Budget Allocator가 켜져 있으면 등록하는 메시의 `bEnableUpdateRateOptimizations`를 끄고 갱신을 직접 맡고, 꺼지면 URO를 다시 켠다. 켜짐은 CVar `a.Budget.Enabled`(기본 0)와 월드마다 만드는 Budget Allocator의 `SetEnabled`(처음 값 거짓)가 모두 참이어야 한다. 스켈레탈 메시의 URO 기본값도 끔이다.

Budget Allocator는 수요가 예산 안이면 모든 메시를 매 프레임 갱신해 일을 줄이지 않는다. 모든 개체가 액터인 군중에 예산 4ms를 건 조건은 호드 2천(애니메이션 4.17ms)에서만 프레임 p95를 41.3ms에서 36.9ms로 줄였다. 1천(3.71ms)에서는 0.52ms 길었다. 메시 클래스를 `USkeletalMeshComponentBudgeted`로 바꾸고, CVar 둘과 월드의 `SetEnabled`를 함께 넣고, 실제로 켜졌는지 실행 기록에 남긴다.

`Source/HordeOptLab/LabGameMode.cpp`(줄였다)

```cpp
LabSetCVar(TEXT("a.Budget.Enabled"), TEXT("1"));
LabSetCVar(TEXT("a.Budget.BudgetMs"), TEXT("4"));
IAnimationBudgetAllocator::Get(GetWorld())->SetEnabled(true); // CVar만으로는 켜지지 않는다
```

근거: [engine-notes 52절, 18절, 29절](../Reference/engine-notes.md), [글 01 "Budget Allocator만 켠 액터"](../../Posts/01-actor-baseline/README.md), [글 01 측정 기록](../../Posts/01-actor-baseline/measurements.md), [`LabGameMode.cpp`](../../Source/HordeOptLab/LabGameMode.cpp), [`LabMinion.cpp`](../../Source/HordeOptLab/LabMinion.cpp) `ALabMinionBudget`

### 액터 캡슐은 QueryOnly와 전용 트레이스 채널로 켠다

가까운 개체에 판정용 캡슐이 필요하면 캡슐이 액터 비용의 큰 몫이 된다. `ACharacter` 캡슐은 `Pawn` 프로필(QueryAndPhysics)에 오버랩과 물리 볼륨 갱신이 켜져 있다. 피직스 상태가 있는 캡슐은 옮길 때마다 바디 변환을 피직스 씬에 보내고(`SendPhysicsTransform` → `SetGlobalPose`), QueryOnly도 바디가 있어 이 일을 한다. `NoCollision`만 바디를 만들지 않는다.

오버랩과 물리 볼륨 갱신을 끄고, 모든 채널을 무시로 둔 뒤 전용 트레이스 채널만 막게 하고, QueryOnly로 바꾼다. 채널은 `Config/DefaultEngine.ini`에 기본 응답 무시(`ECC_GameTraceChannel1`, `DefaultResponse=ECR_Ignore`)로 만든다. 그래도 최종 구성에서 캡슐만 끈 조건은 프레임 p95가 1.86ms 짧아 바디 자체가 비용이다. QueryOnly 바디를 가속 구조의 따로 된 칸에 넣는 CVar `p.Chaos.AccelerationStructureIsolateQueryOnlyObjects`도 있지만 최종 측정에는 넣지 않았다.

`Source/HordeOptLab/LabMinion.cpp`, `ALabMinionMassActor::BeginPlay`(캡슐을 켜는 갈래만 남겨 줄였다)

```cpp
Capsule->SetShouldUpdatePhysicsVolume(false);
Capsule->SetGenerateOverlapEvents(false);
FCollisionResponseContainer Responses(ECR_Ignore);
Responses.SetResponse(LabSnipeTraceChannel, ECR_Block); // 전용 트레이스 채널만 막는다
Capsule->SetCollisionResponseToChannels(Responses);
Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
```

근거: [engine-notes 38절, 44절](../Reference/engine-notes.md), [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md), [`LabMinion.cpp`](../../Source/HordeOptLab/LabMinion.cpp), [`DefaultEngine.ini`](../../Config/DefaultEngine.ini)

### 액터 메시에 고정 바운드를 쓴다

액터 수가 늘수록 Budget Allocator가 재는 액터 애니메이션 시간이 는다. 엔진은 애니메이션을 갱신할 때마다(`FinalizeAnimationUpdate`) 캐시한 바운드를 지우고 다시 셈한다. 기본값에서는 피직스 에셋의 바디마다 뼈 변환으로 상자를 셈한다. `bComponentUseFixedSkelBounds`를 켜면 메시 에셋의 바운드에 루트 뼈 이동만 더한다.

하이드레이션 액터의 메시에서 고정 바운드를 켜고 `BoundsScale`을 1.1로 둔다. 메시 에셋의 바운드가 피직스 에셋으로 셈한 바운드보다 작아서, 화면 컬링과 메시 LOD 고르기가 달라지지 않게 키운 것이다. 최종 구성에서 이것만 끄면 프레임 p95가 0.98ms 길고, 액터 메시의 애니메이션 시간이 2.71ms에서 3.63ms로 는다.

`Source/HordeOptLab/LabMinion.cpp`, `ALabMinionMassActor::BeginPlay`

```cpp
SkelMesh->bComponentUseFixedSkelBounds = FLabConfig::Get().bFixedSkelBounds;
if (FLabConfig::Get().bFixedSkelBounds)
{
	SkelMesh->SetBoundsScale(FLabQuality::FixedSkelBoundsScale); // 1.1
}
```

근거: [engine-notes 44절](../Reference/engine-notes.md), [글 05 "단계 8"](../../Posts/05-cost-cuts-and-contributions/README.md), [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md), [`LabMinion.cpp`](../../Source/HordeOptLab/LabMinion.cpp)

### 캡슐을 겨누는 라인 트레이스는 시작점을 정면 평면에서 비킨다

캡슐 중심을 겨눈 라인 트레이스가 일정 비율로 캡슐을 놓친다. 이 프로젝트의 조건에서는 대상이 시작점을 바라볼 때, 곧 시작점이 캡슐의 정면 평면(정면 축과 연직축이 이루는 평면) 위에 있을 때 놓쳤다. 목표가 서 있는 장면에서 3,780회 중 63회(1.67%)다. 빗나간 순간에도 같은 자리의 작은 구 겹침 질의는 캡슐을 찾았고, 브로드페이즈를 거치지 않는 `LineTraceComponent`도 놓쳤다. 캡슐 레이캐스트(`Chaos::Raycasts::RayCapsule`) 안의 어느 분기인지는 찾지 못했다.

시작점이나 끝점을 정면 평면에서 몇 cm 비킨다. 개발 빌드 진단에서 시작점을 옆으로 1.7cm 비키자 빗나감이 0회였다. 이 프로젝트는 이 트레이스를 확인에만 써서 아직 고치지 않고 빗나감을 보고만 한다.

근거: [engine-notes 53절](../Reference/engine-notes.md), [글 06 5절](../../Posts/06-engine-findings/README.md), [글 06 측정 기록](../../Posts/06-engine-findings/measurements.md), [`LabOrb.cpp`](../../Source/HordeOptLab/LabOrb.cpp) `ALabOrb::TraceSnipeCapsules`

## 인스턴스와 입자를 쓸 때

### 먼 개체는 화면 밖 여부보다 거리로 먼저 지운다

먼 개체를 인스턴스로 그리는 비용이 화면에 보이는 수가 아니라 표현을 가진 개체 수를 따라 는다. 엔진 표현 프로세서는 표현이 없음(None)이 아닌 개체를 화면 밖이어도 `CanBeSeen`으로 둔다. 그래서 인스턴스 갱신 프로세서의 청크 필터(`AreAnyEntitiesVisibleInChunk`)가 그 청크를 거르지 않는다. 반대로 표현이 None인 개체는 `CulledByDistance` 청크로 모이고, 표현 프로세서는 그 청크를 0.5초(± 10%)마다만 다시 본다.

표현 LOD의 Off 구간에 `EMassRepresentationType::None`을 주고 Off 거리를 둔다. 최종 구성에서 200m 밖 지우기만 끄면 200m 밖 개체가 맵 끝까지 ISKM으로 그려진다. 그러면 프레임 p95가 3.54ms, 게임 스레드가 3.23ms 길어진다. 화면 밖만 지우는 변형은 앞선 시험에서 이득이 없었다(원인은 가르지 않았다). Off에서 들어오는 개체는 히스테리시스 때문에 80\~200m 구간의 90% 지점(188m)에서, 청크 갱신 간격만큼 늦게 그려진다.

`Source/HordeOptLab/LabCrowdSubsystem.cpp`, `ULabCrowdSubsystem::BuildEntityConfig`

```cpp
const float Bands[EMassLOD::Max] = { 0.f, HydrateCm, 8000.f, FLabConfig::Get().RepOffCm() }; // 0, 35, 80, 200m
// ...
Visual->Params.LODRepresentation[EMassLOD::Off] = EMassRepresentationType::None;
```

근거: [engine-notes 17절, 21절, 31절, 33절, 45절](../Reference/engine-notes.md), [글 05 "기법별 기여"](../../Posts/05-cost-cuts-and-contributions/README.md), [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md), [measurement.md](measurement.md) "확정값" 200m 밖 행

### 움직이는 인스턴스는 Lumen 장면에서 뺀다

움직이는 정지 메시 인스턴스 수천 개를 그리면 GPU 시간이 크게 는다. 소프트웨어 Lumen은 `AffectsDynamicIndirectLighting()`이 참이고 거리 필드를 가진 프리미티브를 장면에 넣는다. 거리 필드는 스태틱 메시, Nanite, 스플라인만 가진다. 인스턴스가 움직인 프리미티브는 매 프레임 Lumen 장면 갱신 목록에 든다. 스켈레탈 메시는 원래 Lumen 장면 밖이라, 액터 군중에서는 드러나지 않다가 인스턴스로 바꿀 때 나타난다.

Mass 비주얼라이저가 만든 ISM·ISKM 컴포넌트마다 `SetAffectDynamicIndirectLighting(false)`를 부른다. 컴포넌트는 처음 쓰일 때 늦게 생기므로 매 틱 비주얼라이저의 컴포넌트를 훑어 아직 고치지 않은 것만 고친다. 개체는 계속 Lumen의 간접광을 받고 그림자를 그린다. 총괄 측정 밖의 초기 시험(Test 패키지, 움직이는 ISM 2,000개)에서 GPU 시간이 29.0ms에서 6.0ms로 줄었다. 어느 Lumen 패스가 시간을 썼는지는 GPU 트레이스로 보지 않았다.

`Source/HordeOptLab/LabCrowdSubsystem.cpp`, `ULabCrowdSubsystem::ExcludeVisualsFromLumen`(줄였다)

```cpp
for (TActorIterator<AMassVisualizer> It(GetWorld()); It; ++It) // 컴포넌트는 늦게 생기므로 매 틱 훑는다
{
	// ... ISM·ISKM 컴포넌트 가운데 bAffectDynamicIndirectLighting이 아직 켜진 것만
	LabExcludeFromLumen(*Component); // SetAffectDynamicIndirectLighting(false)
}
```

근거: [engine-notes 11절, 33절](../Reference/engine-notes.md), [measurement.md](measurement.md) "확정값" 미니언과 Lumen 행, [`LabCrowdSubsystem.cpp`](../../Source/HordeOptLab/LabCrowdSubsystem.cpp), [`LabMinion.h`](../../Source/HordeOptLab/LabMinion.h)

### Mass 인스턴스의 그림자는 설명자에서 켠다

하이드레이션 액터는 그림자를 그리는데 인스턴스로 바뀐 개체는 그림자가 없다. 엔진의 ISM 설명자(`FMassStaticMeshInstanceVisualizationMeshDesc`)와 ISKM 설명자(`FMassSkinnedMeshInstanceVisualizationMeshDesc`)의 `bCastShadows` 기본값은 거짓이다. 이 값은 엔진이 설명자마다 만드는 컴포넌트의 `SetCastShadow`로만 넘어가고, 인스턴스 단위 그림자 플래그는 없다. `bCastShadows`가 설명자 해시에 들어서 그림자만 다른 두 설명자는 컴포넌트가 둘이 된다.

설명자를 만들 때 `bCastShadows`를 참으로 둔다. 그림자 비용은 GPU에 실린다. 최종 구성에서 80m 밖 그림자를 다시 켠 조건은 GPU 시간이 0.48ms 길고, 병목이 게임 스레드라 프레임 p95는 구별되지 않았다.

`Source/HordeOptLab/LabCrowdSubsystem.cpp`, `ULabCrowdSubsystem::BuildEntityConfig`

```cpp
MeshDesc.bCastShadows = true; // 엔진 기본값은 false(MassRepresentationTypes.h:105)
SkinnedDesc.bCastShadows = FLabIskmParams::bCastShadows; // true. 엔진 기본값은 false(MassSkinnedMeshRepresentationTypes.h:79)
```

근거: [engine-notes 22절](../Reference/engine-notes.md), 엔진 소스 `MassRepresentation/Public/MassRepresentationTypes.h:105`와 `MassSkinnedMeshRepresentationTypes.h:79`, [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md), [`LabCrowdSubsystem.cpp`](../../Source/HordeOptLab/LabCrowdSubsystem.cpp)

### ISKM 컴포넌트에 카메라를 따라가는 고정 바운드를 넣는다

ISKM 인스턴스가 매 프레임 움직이면 컴포넌트 바운드 계산이 인스턴스 수를 따라 는다. ISKM 컴포넌트는 인스턴스가 바뀐 프레임마다 `UpdateBounds()`를 부른다. 바운드 계산은 `bFastBuild = false`로 박혀 있어 인스턴스마다 애니메이션 바운드를 변환해 합친다. `SetPrimitiveBoundsOverride(FBox)`로 상자를 넣으면 이 반복 없이 그 상자를 돌려준다. 인스턴스 컬링과 GPU LOD 선택은 인스턴스 데이터로 하므로 상자는 컴포넌트 단위 컬링에만 쓰인다.

카메라 둘레에 상자를 넣고, 카메라가 정한 거리보다 멀리 움직이면 상자를 옮겨 다시 넣는다. 인스턴스가 매 프레임 움직이는 컴포넌트는 다음 갱신 때 새 바운드가 렌더 스레드로 가서, 렌더 상태를 다시 만들 필요가 없다. 최종 구성에서 이 상자만 끄면 프레임 p95가 0.66ms 길다.

`Source/HordeOptLab/LabCrowdSubsystem.cpp`, `ULabCrowdSubsystem::ApplyFixedIskmBounds`

```cpp
const FBox Fixed(IskmBoundsCenter + FVector(-HalfXY, -HalfXY, -500.f), IskmBoundsCenter + FVector(HalfXY, HalfXY, 1500.f));
Component.SetPrimitiveBoundsOverride(Fixed); // 상자를 옮긴 틱에도 다시 넣는다
Component.UpdateBounds();
```

근거: [engine-notes 15절, 41절](../Reference/engine-notes.md), [글 04 "단계 6"](../../Posts/04-quality-by-distance/README.md), [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md), [`LabCrowdSubsystem.cpp`](../../Source/HordeOptLab/LabCrowdSubsystem.cpp)

### 움직이는 ISKM 엔티티는 파괴하지 말고 풀로 다시 쓴다

개체가 죽고 다시 생기는 동안 ISKM 트랙 수와 GPU 메모리가 실행 내내 는다. Mass는 엔티티가 ISKM 컴포넌트에 처음 들 때 엔티티 핸들을 키로 트랙을 잡는다. 트랙을 돌려받는 `DeallocateTrack`은 `FMassStaticRepresentationTag`를 요구하는 정지 표현 경로에서만 불려, 움직이는 엔티티를 파괴하면 트랙이 활성인 채 남는다. 렌더러는 남은 트랙도 매 프레임 평가한다. 5.8.4에도 남아 있고 [Epic 개발자 포럼](https://forums.unrealengine.com/t/mass-crowds-using-skinnedmeshinstance-leak-gpu-memory-every-wave-of-spawned-and-killed-entities-permanently-grows-the-animation-track-pool-5-8/2742508)에 같은 보고가 있다.

죽은 엔티티를 종류별 풀에 넣었다가 재생성 때 꺼내 위치, 체력, 상태를 다시 넣는다. 핸들이 같아 엔진이 옛 트랙을 다시 쓴다. 엔진은 트랙이 있으면 클립 번호가 바뀔 때만 재생 데이터를 쓰므로, 상태나 시작 시각이 바뀐 프레임에 `SetAutoPlayData`를 직접 부른다. 240초 실행 끝의 트랙은 풀을 켜면 20,215개, 끄면 30,797개였고 프레임 p95는 구별되지 않았다. 960초씩 돌린 참고 측정에서는 풀을 끈 쪽이 트랙 120,428개, GPU 메모리 5,110MB(켠 쪽 3,370MB)가 됐고 13분 구간부터 프레임 p95가 0.6ms 넘게 높았다.

`Source/HordeOptLab/LabCrowdSubsystem.cpp`, `ULabCrowdSubsystem::SpawnEntities`(줄였다)

```cpp
const int32 NumReuse = bPool ? FMath::Min(EntityPool[Type].Num(), Indices.Num()) : 0;
for (int32 k = 0; k < NumReuse; ++k)
{
	const FMassEntityHandle Entity = EntityPool[Type].Pop(EAllowShrinking::No);
	ReinitPooledEntity(EntityManager, Entity, Locations[Indices[k]], Serials[Indices[k]]);
}
```

근거: [engine-notes 3절](../Reference/engine-notes.md)("ISKM 트랙 풀 누수"), [engine-notes 15절](../Reference/engine-notes.md), [글 05 "엔티티 풀"](../../Posts/05-cost-cuts-and-contributions/README.md), [글 06 1절](../../Posts/06-engine-findings/README.md), [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md), [`LabCrowdSubsystem.cpp`](../../Source/HordeOptLab/LabCrowdSubsystem.cpp)

### ISKM 트랙 풀은 처음에 용량까지 늘려 둔다

ISKM으로 그린 같은 종류의 개체 전체가 가끔 한 프레임 세로로 길게 끌린다. 트랙 수가 느는 동안에만 나고, 모든 개체가 트랙을 가지면 멈춘다. 트랙 수가 바뀌면 렌더러가 그 컴포넌트의 뼈대 버퍼(트랙 수 × 뼈 수 × 2)를 옛 자리 해제, 새 자리 할당 순으로 다시 잡는다. `FSpanAllocator`는 맞는 빈 구간이 없으면 해제한 구간을 이웃과 합쳐 옛 자리와 겹치는 자리를 줄 수 있고, 옛 내용을 옮기는 셰이더는 같은 버퍼를 병렬로 읽고 쓴다. 그래서 이전 프레임 자세 칸이 깨지고 모션 블러가 틀린 속도를 그린다.

컴포넌트가 처음 쓰이는 프레임에 트랙을 용량(종류의 개체 수 × 1.1 + 64)만큼 할당했다가 모두 해제한다. 트랙 풀의 끝 번호가 남아 트랙 수가 다시 바뀌지 않고, 엔진의 다음 할당은 빈 번호를 다시 쓴다. 속도 버퍼를 찍은 개발 빌드 900프레임 캡처에서 튄 프레임이 42개에서 0개가 됐고, 이 처리만 끈 조건의 프레임 p95는 구별되지 않았다. 트랙이 용량을 넘으면 다시 생기므로 위의 엔티티 풀과 함께 쓴다.

`Source/HordeOptLab/LabIskmProcessor.cpp`, `LabReserveTracks`

```cpp
TArray<int32> Temp;
while (int32(Provider->GetUniqueAnimationCount()) < Capacity) { Temp.Add(Provider->AllocateTrack()); }
for (const int32 TrackIndex : Temp) { Provider->DeallocateTrack(TrackIndex); } // 끝 번호는 남는다
```

근거: [engine-notes 42절](../Reference/engine-notes.md), [글 06 2절](../../Posts/06-engine-findings/README.md), [글 06 측정 기록](../../Posts/06-engine-findings/measurements.md), [`LabIskmProcessor.cpp`](../../Source/HordeOptLab/LabIskmProcessor.cpp)

### ISKM에는 가산 클립 대신 가산을 끈 복제본을 넣는다

ISKM 개체가 피격 클립을 재생하는 동안 사라지거나 누운 모양이 된다. ISKM 트랙 제공자(`UAnimSequenceTransformProviderData`)의 렌더러 경로는 압축을 푼 지역 자세를 그대로 뼈 변환으로 쓰고, 기준 자세를 읽는 코드가 없다. 가산 클립이면 이 값은 기준 자세와의 차이라 뼈가 원점 근처로 모인다. 액터의 단일 노드 재생과 AnimToTexture 굽기는 기준 자세에 차이를 더하므로, 같은 클립이 액터와 VAT 인스턴스에서는 맞다.

에셋 스크립트에서 가산 클립을 복제해 가산 종류를 `AAT_None`으로 두고, 그 복제본을 트랙 제공자에 넣는다. 엔진은 원본 데이터를 늘 전체 자세로 두고 가산 값은 압축할 때 만든다. 그래서 복제본은 기준 자세에 차이를 더한 전체 자세로 다시 압축된다.

`Scripts/create-iskm.py`, `provider_clip`(줄였다)

```python
if kind == unreal.AdditiveAnimationType.AAT_NONE:
    return anim
full = eal.duplicate_asset(src_path, path)
full.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)  # PostEditChange가 다시 압축한다
```

근거: [engine-notes 46절](../Reference/engine-notes.md), [글 06 3절](../../Posts/06-engine-findings/README.md), [`create-iskm.py`](../../Scripts/create-iskm.py)

### AnimToTexture Bone 방식 메시의 바운드는 정점 범위로 넓힌다

VAT 인스턴스가 죽기 애니메이션을 재생하는 동안 무기 끝 같은 먼 정점의 그림자가 바닥에 남는다. AnimToTexture의 Bone 모드는 스태틱 메시 바운드를 뼈 위치 범위로만 넓히고, Vertex 모드는 모든 프레임의 정점 범위로 넓힌다. Virtual Shadow Map은 물체 바운드 안쪽의 페이지만 무효화하므로, 바운드 밖으로 나간 정점의 그림자가 캐시에 남는다. 셰이더로 정점을 옮기는 기법(VAT, WPO)은 바운드를 직접 챙겨야 한다.

같은 메시를 Vertex 모드로 먼저 구워 정점 범위(`VertexMinBBox`, `VertexSizeBBox`)를 얻는다. 그 범위까지 Bone 메시의 `PositiveBoundsExtension`·`NegativeBoundsExtension`을 넓힌다. 그림자 캐시를 매 프레임 무효화하는 설정(`ShadowCacheInvalidationBehavior::Always`)도 그림자를 지웠지만 비용 때문에 쓰지 않았다.

`Scripts/bake-vat.py`(줄였다)

```python
vmin, vsize = VERTEX_BBOX[0]  # 먼저 구운 Vertex 모드의 정점 범위
new_max = unreal.Vector(max(base_max.x, vmin.x + vsize.x), max(base_max.y, vmin.y + vsize.y), max(base_max.z, vmin.z + vsize.z))
sm.set_editor_property("positive_bounds_extension", unreal.Vector(new_max.x - base_max.x, new_max.y - base_max.y, new_max.z - base_max.z))
# negative_bounds_extension도 min으로 같은 식을 쓴다
```

근거: [engine-notes 3절](../Reference/engine-notes.md)("AnimToTexture 바운드"), [post-material.md](../post-material.md) "죽는 VAT 인스턴스의 칼날 그림자", [전후 화면](../../Measurements/M3/shots/vat-shadow/README.md), [`bake-vat.py`](../../Scripts/bake-vat.py)

### Niagara 메시 입자의 VAT 프레임은 Dynamic Parameter로 넘긴다

ISM에서 맞게 걷던 VAT 머티리얼을 Niagara 메시 입자에 쓰면 프레임이 엉뚱하게 나온다. Niagara 메시 렌더러는 GPU Scene 경로에서 인스턴스 커스텀 데이터 float 하나를 잡고, 그 0번에 입자 인덱스의 비트를 쓴다. 그래서 `PerInstanceCustomData`로 프레임을 읽는 VAT 머티리얼은 Niagara에서 입자 인덱스를 읽는다.

AnimToTexture의 머티리얼 함수 `GetFrameSwitch`에서 정적 스위치 `AutoPlay`와 `UseDynamicParameters`를 켠다. 그러면 프레임을 Dynamic Material Parameter 1번(TimeOffset, Playrate, StartFrame, EndFrame)에서 읽는다. 이미터에는 `DynamicMaterialParameters` 모듈을 더하고, 쓰기 인덱스 0을 끄고 1을 Float로 둔 뒤 네 값을 넣는다. Playrate 0과 StartFrame을 죽기 클립 안에 둔 화면에서 모든 입자가 같은 쓰러진 자세로 멈춰, 네 값이 제자리에 들어간 것을 확인했다.

`Scripts/create-crowd-fx.md`(설정)

```text
MI 정적 스위치: AutoPlay = true, UseDynamicParameters = true
DynamicMaterialParameters: Write Parameter Index 0 = Off, Write Parameter Index 1 = Float
Index 1 Param 1..4 = TimeOffset, Playrate, StartFrame, EndFrame
```

근거: [engine-notes 17절](../Reference/engine-notes.md), [글 04 "Niagara 메시 입자"](../../Posts/04-quality-by-distance/README.md), [`create-crowd-fx.md`](../../Scripts/create-crowd-fx.md)

## Mass 프로세서를 짤 때

### 엔진 프로세서가 한 스레드로 도는지 먼저 본다

트레이스에서 작업자 스레드 하나가 프레임 대부분 동안 한 프로세서를 돌고, 게임 스레드는 그동안 기다린다. 엔진 이동 회피(`UMassMovingAvoidanceProcessor`)와 서 있는 회피는 `ForEachEntityChunk`로 청크를 한 스레드에서 차례로 돈다. 이동 회피 쿼리는 Medium·Low·Off LOD 태그가 있는 개체를 빼는데, Simulation LOD를 켜지 않으면 이 태그가 없어 전 개체가 순회에 든다. 엔진 이동(`UMassApplyMovementProcessor`)과 방향 보간(`UMassSmoothOrientationProcessor`)도 병렬 순회가 아니다.

엔진 프로세서를 쓰기 전에 소스에서 `ForEachEntityChunk`인지 `ParallelForEachEntityChunk`인지 본다. 엔진 회피를 그대로 쓴 단계에서 회피는 작업자 하나에서 프레임마다 24.7ms를 돌았고, 직렬 트레이스에서 Mass 프로세서 시간의 80.7%(23.5ms)였다. 이 프로젝트는 위치를 직접 미는 분리 프로세서를 짜서 바꿨다. 이웃 계산을 128개씩 묶어 `ParallelFor`로 나누고, 모든 개체가 같은 위치 사본을 읽어 스레드 수와 무관하게 결과가 같다. 최종 구성에서 이것을 엔진 회피로 되돌린 조건은 프레임 p95가 8.57ms 길다.

`Source/HordeOptLab/LabSeparation.h`, `FLabSeparationSolver::Solve`(줄였다)

```cpp
ParallelFor(NumBatches, [this, &Params, Num](int32 Batch)
{
	for (int32 I = Batch * BatchSize; I < FMath::Min(Num, (Batch + 1) * BatchSize); ++I)
	{ const FVector2f Delta = Resolve(I, Params); Next[I] = Pos[I] + Delta; } // 모두 Pos를 읽고 Next에 쓴다
}, Flags);
```

근거: [engine-notes 36절, 37절](../Reference/engine-notes.md), [글 02 "게임 스레드는 작업자 스레드 하나의 엔진 회피를 기다린다"](../../Posts/02-mass-engine-path/README.md), [글 03](../../Posts/03-separation/README.md), [글 03 측정 기록](../../Posts/03-separation/measurements.md), [`LabSeparation.h`](../../Source/HordeOptLab/LabSeparation.h)

### 개체마다 독립인 엔진 프로세서는 청크 병렬 사본으로 바꾼다

한 스레드로 도는 엔진 프로세서가 위치를 쓰는 사슬 앞에 있어 뒤 프로세서와 게임 스레드를 늦춘다. 엔진 방향 보간의 두 쿼리는 `ForEachEntityChunk`이고, 개체마다 이동 목표와 바라는 속도를 읽어 자기 회전과 `DesiredFacing`만 쓴다. 쿼리가 `private`이고 클래스가 `MinimalAPI`라 상속으로 순회만 바꿀 수 없다. 처리 큐에서 `ParallelForEachEntityChunk`의 청크는 하나씩 `UE::Tasks`로 띄워져, 게임 스레드 프로세서여도 작업자로 간다.

같은 쿼리와 식을 가진 사본을 게임 모듈에 두고 엔진 프로세서의 자동 등록을 끈다. 수학 함수(`LerpAngle`, `ExponentialSmoothingAngle`)가 헤더 인라인이라 사본의 결과는 엔진과 비트 단위로 같았다. 직렬 트레이스에서 방향 보간은 프레임당 0.82ms에서 0.22ms가 됐다. 자동 등록은 CDO의 `bAutoRegisterWithProcessingPhases`를 게임 모듈 시작 때 리플렉션으로 바꾼다. 엔진의 `SetShouldAutoRegisterWithGlobalList`는 에디터 빌드에서 그 값을 `DefaultMass.ini`에 써 버린다.

`Source/HordeOptLab/LabSmoothOrientation.cpp`, `ULabSmoothOrientationProcessor::Execute`(엔진은 같은 자리가 `ForEachEntityChunk`다)

```cpp
HighResEntityQuery.ParallelForEachEntityChunk(Context, HighRes);
LowResEntityQuery_Conditional.ParallelForEachEntityChunk(Context, LowRes);
```

근거: [engine-notes 48절, 49절](../Reference/engine-notes.md), 엔진 소스 `MassEntity/Private/MassProcessor.cpp:255-270`, [글 03 "바꾼 것"](../../Posts/03-separation/README.md), [글 03 측정 기록](../../Posts/03-separation/measurements.md), [`LabSmoothOrientation.cpp`](../../Source/HordeOptLab/LabSmoothOrientation.cpp), [`HordeOptLab.cpp`](../../Source/HordeOptLab/HordeOptLab.cpp) `LabSetAutoRegister`

### 게임 스레드 프로세서의 주 쿼리에 위치 쓰기를 선언하지 않는다

게임 스레드의 가벼운 프로세서가 하는 일보다 오래 걸리고, 트레이스에서 위치를 쓰는 다른 프로세서가 끝나는 순간에 함께 끝난다. `mass.ForceInlineProcessorExecution`(기본 참)에서는 한 쿼리의 직접 쓰기와 다른 쿼리의 직접 읽기가 겹치면, 아키타입이 겹치는지 보지 않고 두 쿼리를 전역 충돌로 묶는다. 묶인 쿼리는 시작하기 전에 상대가 끝나기를 기다린다. 그 시간은 기다린 프로세서의 실행 시간에 잡히고 자체 타이머가 없다. ReadWrite 선언은 읽기와 쓰기 양쪽에 든다.

드물게 쓰는 프래그먼트는 따로 된 쿼리로 빼고, 쓸 개체가 있는 프레임에만 그 쿼리를 돌린다. 실행 중 쿼리 순서는 자유이고 돌리지 않은 쿼리는 검사하지 않는다. 그리는 끝 밖의 피격 개체를 ISKM으로 올리는 프로세서를 이렇게 고치자, 개발 빌드 진단에서 그 실행 시간이 거의 0이 됐다.

`Source/HordeOptLab/LabMassProcessors.cpp`, `ULabFarHitIskmProcessor::ConfigureQueries`와 `Execute`(줄였다)

```cpp
EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadWrite); // 주 쿼리에는 위치가 없다
CatchUpQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);            // 위치는 따라잡기 쿼리만
if (CatchUps.Num() > 0) { CatchUpQuery.ForEachEntityChunk(Context, /* ... */); }
```

근거: [engine-notes 47절](../Reference/engine-notes.md), [글 06 4절](../../Posts/06-engine-findings/README.md), [글 06 측정 기록](../../Posts/06-engine-findings/measurements.md), [`LabMassProcessors.cpp`](../../Source/HordeOptLab/LabMassProcessors.cpp)

### 프로세서 단위 서브시스템 선언이 다른 프로세서를 막는지 본다

게임 스레드 러너가 고를 프로세서가 없어 노는 빈틈이 생기고, 그 끝이 작업자의 다른 프로세서가 끝나는 때와 맞는다. 실행 큐는 `ProcessorRequirements`에 선언한 서브시스템 요구를 그대로 담고, 한쪽이 쓰고 다른 쪽이 읽거나 쓰면 두 프로세서를 서로 막는 쌍으로 둔다. `TMassExternalSubsystemTraits::ThreadSafeWrite`는 의존성 해결기만 읽어 이 충돌을 풀지 못한다. 러너는 프로세서를 고를 때 충돌 칸을 잡고 쿼리를 도는 동안만 놓으므로, 쿼리 밖에서 오래 일하는 프로세서는 상대를 끝까지 막는다.

트레이스에서 러너의 빈틈(`MassProcessingQueue Main-Thread Runner Task`의 Excl)과 그 끝에 끝난 프로세서를 맞춰 본다. 서브시스템 쓰기는 꼭 필요한 프로세서에만 프로세서 단위로 선언한다. 쿼리에 선언한 서브시스템은 그 쿼리의 데이터 접근에만 든다(`bAggregateRequirements` 기본 거짓). 이 프로젝트의 트레이스에서는 아래 두 선언이 분리 프로세서와 피격 ISKM 프로세서를 서로 막았다. 읽기 선언을 빼면 표현이 밀기 전 위치를 읽는 프레임이 늘어 선언은 두었고, 대신 앞쪽 사슬을 줄였다(위 방향 보간 사본).

`Source/HordeOptLab/LabMassProcessors.cpp`, 두 프로세서의 `ConfigureQueries`

```cpp
// ULabSeparationProcessor: 개발 확인 로그에 쓰는 값을 읽는다
ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadOnly);
// ULabFarHitIskmProcessor: 위와 서로 막는 쌍이 된다
ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadWrite);
```

근거: [engine-notes 35절, 47절, 48절](../Reference/engine-notes.md), [글 03 측정 기록](../../Posts/03-separation/measurements.md) "표현과 분리의 순서", [`LabMassProcessors.cpp`](../../Source/HordeOptLab/LabMassProcessors.cpp)

### 프로세서별 시간은 직렬 트레이스로 가른다

병렬 트레이스에서 어떤 프로세서의 구간이 프레임 대부분에 걸쳐 그 프로세서가 비싸 보인다. 병렬 처리 큐에서 프로세서가 쿼리 자원을 기다린 시간은 그 프로세서의 실행 시간에 잡히고, 기다림 자체의 타이머는 없다. 게임 스레드에서 기다리면 스핀(`YieldThread` 루프)이라 게임 스레드 시간에도 든다.

`mass.FullyParallel`을 0으로 둔 실행에서 트레이스를 한 번 더 떠, Mass 프로세서를 게임 스레드에서 차례로 돌린다. 엔진 회피를 쓴 단계의 병렬 트레이스 한 프레임에서 엔진 방향 보간의 구간은 0.69ms부터 28.8ms까지 걸쳐 있었다. 직렬 트레이스에서 같은 프로세서는 프레임당 0.82ms였다. 직렬은 원인을 가르는 수단이고, 병렬에서 기다린 시간도 그 실행의 실제 비용이다. 프로세서 안의 `ParallelFor`는 직렬에서도 작업자로 나뉜다.

`Source/HordeOptLab/LabGameMode.cpp`(실행 인자 `-LabFullyParallel=0`일 때)

```cpp
IConsoleManager::Get().FindConsoleVariable(TEXT("mass.FullyParallel"))->Set(0, ECVF_SetByCode);
```

근거: [engine-notes 11절, 13절, 47절](../Reference/engine-notes.md), [measurement.md](measurement.md) "산출물" 트레이스 행, [글 02 측정 기록](../../Posts/02-mass-engine-path/measurements.md) "한 프레임", [글 03 측정 기록](../../Posts/03-separation/measurements.md) "트레이스 값", [`LabGameMode.cpp`](../../Source/HordeOptLab/LabGameMode.cpp)

### 틱 간격을 늘리기보다 큰 몫의 프로세서를 쿼리에서 뺀다

Simulation LOD로 먼 개체의 틱 간격을 늘렸는데 프레임이 줄지 않는다. 엔진 가변 틱은 청크 필터 `ShouldTickChunkThisFrame`과 누적 델타를 쓰는 프로세서만 건너뛴다. 엔진 이동·조향은 Off를 뺀 개체를 매 프레임 돈다. 또 엔티티가 드나든 청크는 수정 번호가 바뀌어 간격과 무관하게 다음 프레임에 돈다. 죽고 다시 생기며 태그가 바뀌는 호드에서 설정한 0.5초 간격의 실제 주기는 0.13초였다.

엔진 이동·조향 넷을 한꺼번에 빼는 태그는 `FMassOffLODTag`뿐이다. 먼 개체를 Simulation LOD의 Off에 두고, Off 개체를 옮기는 일은 직접 짠 프로세서가 1초마다 한다. 엔진의 `UMassOffLODNavigationProcessor`는 Off 개체를 이동 목표로 순간 이동시키므로 등록을 끈다. 가변 틱 필터를 읽는 프로세서는 `ExecuteAfter(ProcessorGroupNames::LOD)`를 적어야 주기를 놓치지 않는다. 앞선 시험에서는 Off로 엔진 이동을 뺀 변형만 프레임이 줄었다. 최종 구성에서는 Simulation LOD를 끈 조건이 0.24ms 짧아 구별되지 않았다.

`Source/HordeOptLab/LabMassTraits.cpp`, `ULabSimulationLODTrait::ULabSimulationLODTrait`(줄였다)

```cpp
bEnableVariableTicking = true; // LOD 태그도 함께 붙는다
VariableTickParams.TickRates[EMassLOD::Medium] = FLabSimLod::TickMedium; // 0.5초
VariableTickParams.TickRates[EMassLOD::Low] = FLabSimLod::TickLow;       // 1초
Params.LODDistance[EMassLOD::Off] = FLabSimLod::LowDistance;             // 200m 밖은 Off
VariableTickParams.TickRates[EMassLOD::Off] = FLabSimLod::TickLow;
```

근거: [engine-notes 5절](../Reference/engine-notes.md), [글 05 "단계 5"](../../Posts/05-cost-cuts-and-contributions/README.md), [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md), 실제 주기는 [요약 표](../../Measurements/Full4/summary.csv)의 `real_period_medium`(평균 프레임 시간 × 밴드 개체 수 ÷ 그 프레임에 돈 개체 수), [`LabMassTraits.cpp`](../../Source/HordeOptLab/LabMassTraits.cpp)

### 결정적 해시로 대상을 고를 때는 `HashCombineFast`의 결과를 섞는다

일련번호와 틱으로 해시를 만들어 가장 작은 값의 개체를 고르는데, 늘 작은 일련번호만 뽑힌다. `HashCombineFast(A, B)`는 `A ^ (B + 0x9e3779b9 + (A << 6) + (A >> 2))`이고 `GetTypeHash(uint32)`는 값을 그대로 돌려준다. 그래서 틱은 모든 후보에 같은 수를 더할 뿐 순서를 바꾸지 못한다. 같은 식을 Python으로 흉내 내 후보 2,000개, 틱 200개로 돌리면 200번 모두 가장 작은 일련번호가 뽑힌다.

입력과 결과를 `MurmurFinalize32`로 섞는다. 같은 흉내에서 200번에 서로 다른 대상 189개가 뽑히고, 뽑힌 순위의 평균이 1,043이다(고르게 뽑으면 1,000).

`Source/HordeOptLab/LabSnipe.h`, `LabSnipeHash`

```cpp
inline uint32 LabSnipeHash(uint32 Serial, uint32 SnipeTick)
{
	return MurmurFinalize32(HashCombineFast(MurmurFinalize32(Serial), SnipeTick));
}
```

근거: [engine-notes 34절, 37절](../Reference/engine-notes.md)(`MurmurFinalize32`의 위치), [`LabSnipe.h`](../../Source/HordeOptLab/LabSnipe.h)

## 프로파일링할 때

대규모 군중의 프레임을 읽을 때 이 프로젝트가 따른 순서다. 위에서부터 보고, 앞에서 답이 나오면 다음으로 내려간다.

1. [프레임 p95와 세 스레드 시간을 함께 보고 병목을 정한다](#프레임-p95와-세-스레드-시간을-함께-보고-병목을-정한다).
2. [병목이 아닌 스레드가 준 것은 프레임 이득으로 세지 않는다](#병목이-아닌-스레드가-준-것은-프레임-이득으로-세지-않는다).
3. [게임 스레드 시간의 차이만으로 비용을 말하지 않는다](#게임-스레드-시간의-차이만으로-비용을-말하지-않는다).
4. [프레임이 세 스레드보다 길면 게임 스레드가 기다린 작업자를 찾는다](#프레임이-세-스레드보다-길면-게임-스레드가-기다린-작업자를-찾는다).
5. 트레이스를 연다. [액터 군중은 불린 횟수와 한 번의 시간으로 나눠 본다](#액터-군중은-불린-횟수와-한-번의-시간으로-나눠-본다). [Insights의 Timers 합계는 스레드를 골라 읽고 불린 횟수로 나눈다](#insights의-timers-합계는-스레드를-골라-읽고-불린-횟수로-나눈다).
6. Mass 프로세서로 내려간다. [프로세서별 시간은 직렬 트레이스로 가른다](#프로세서별-시간은-직렬-트레이스로-가른다). [엔진 프로세서가 한 스레드로 도는지 먼저 본다](#엔진-프로세서가-한-스레드로-도는지-먼저-본다).
7. [기법 하나만 끄고 재서 기여를 가른다](#기법-하나만-끄고-재서-기여를-가른다).
8. [빨라진 조건이 같은 일을 하는지 본다](#빨라진-조건이-같은-일을-하는지-본다).

### 프레임 p95와 세 스레드 시간을 함께 보고 병목을 정한다

기법 하나를 넣고 어느 값이 움직였는지 볼 때 첫 질문은 어느 스레드가 프레임을 정하는가다. 게임 스레드, 렌더 스레드, GPU는 서로 다른 프레임을 겹쳐 처리하고 앞서 나갈 수 있는 프레임 수에 한도가 있다. 그래서 프레임 시간은 셋 가운데 가장 느린 쪽(병목)에 맞춰진다. 엔진은 게임 스레드 시간에서 기다린 시간을 빼므로 프레임 시간은 병목보다 조금 길다.

CSV 프로파일러의 `FrameTime`, `GameThreadTime`, `RenderThreadTime`, `GPUTime`을 한 표에 둔다. 판정은 프레임 p95로 하고, 스레드 시간은 기다림이 섞여 중앙값만 쓴다. 최종 구성은 프레임 p95 14.2ms, 게임 스레드 11.9ms, 렌더 스레드 4.12ms, GPU 6.16ms로 병목이 게임 스레드다. 세 스레드 값의 원천(`GGameThreadTime`, `GRenderThreadTime`, `RHIGetGPUFrameCycles()`)은 `STATS` 없이 쓰여 Test 구성의 HUD에서도 같은 값을 읽을 수 있다.

`Engine/Source/Runtime/Engine/Private/UnrealClient.cpp`, `FViewport::Draw`

```cpp
GGameThreadTime = (ThreadTime > GameThread.Waits) ? (ThreadTime - GameThread.Waits) : ThreadTime;
```

근거: [글 00 "가장 느린 스레드가 프레임 시간을 정한다"](../../Posts/00-goals-and-method/README.md), [engine-notes 20절](../Reference/engine-notes.md)("HUD의 시간 값 출처"), [measurement.md](measurement.md) "지표"

### 병목이 아닌 스레드가 준 것은 프레임 이득으로 세지 않는다

GPU 시간이 크게 줄었는데 프레임 시간은 그대로다. 병목이 게임 스레드면 GPU와 렌더 스레드는 남는 시간을 기다리며 보내므로, 그 쪽을 줄여도 프레임은 줄지 않는다. 35\~200m의 VAT 인스턴스를 ISKM으로 바꾼 단계에서 GPU 시간은 1.56ms, 렌더 스레드는 1.51ms 줄었다. 그러나 프레임 p95는 0.19ms 늘어 구별되지 않았다. 최종 구성에서 80m 밖 그림자를 다시 켠 조건도 GPU만 0.48ms 길었다.

결과에는 GPU 시간을 프레임 시간과 함께 적고 "프레임에는 드러나지 않았다"로 쓴다. 그 몫은 사라진 것이 아니라 병목이 바뀔 때까지의 여유다. 하이드레이션을 끈 조건처럼 게임 스레드가 GPU 아래로 내려가면 그때부터 GPU가 프레임을 정한다.

근거: [글 04 "결과"](../../Posts/04-quality-by-distance/README.md), [글 04 측정 기록](../../Posts/04-quality-by-distance/measurements.md), [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md)

### 게임 스레드 시간의 차이만으로 비용을 말하지 않는다

게임 스레드 시간이 늘었는데 프레임은 같거나, 프레임이 늘었는데 게임 스레드 시간은 그대로다. 게임 스레드 시간은 기다림을 빼기도 넣기도 한다. 작업자의 태스크를 기다린 이벤트 대기는 엔진이 뺀다. 병렬 Mass에서 쿼리 자원을 기다린 시간은 기다린 프로세서의 실행 시간이 되고, 게임 스레드에서 기다리면 스핀이라 게임 스레드 시간에 든다.

그래서 기다림이 찍히는 자리가 옮겨 가면 일이 같아도 게임 스레드 시간이 바뀐다. 위치 쓰기 선언 때문에 게임 스레드의 가벼운 프로세서가 작업자의 분리 프로세서를 기다린 시간은 그 프로세서의 비용으로 보였다. 탑뷰 카메라로 잰 앞선 시험에서는 게임 스레드 시간만 늘고 프레임이 같았는데, 작업 대기가 표현 LOD 프로세서 안으로 옮겨 간 것이었다. 스레드 시간의 차이는 프레임 시간과 병렬 트레이스로 확인한 뒤에 비용이라고 쓴다.

근거: [measurement.md](measurement.md) "함정", [engine-notes 11절, 47절](../Reference/engine-notes.md)("병렬 Mass의 게임 스레드 스핀 대기", "Game Thread 시간과 워커 대기"), [글 06 4절](../../Posts/06-engine-findings/README.md), [글 00 측정 방법](../../Posts/00-goals-and-method/README.md)

### 프레임이 세 스레드보다 길면 게임 스레드가 기다린 작업자를 찾는다

프레임 시간이 게임 스레드, 렌더 스레드, GPU 어느 것보다 길다. 엔진 회피를 그대로 쓴 단계에서 프레임 p50은 34.1ms인데 게임 스레드는 18.0ms, GPU는 12.2ms, 렌더 스레드는 7.29ms였다. 모자란 몫은 게임 스레드가 작업자 스레드를 기다린 시간이다. Test 구성에서 이 기다림은 유휴 시간으로 세여 게임 스레드 시간에서 빠진다.

프레임 CSV에서 게임 스레드의 `EventWait/*` 열을 더한다. 이 단계는 프레임마다 16.2ms를 기다렸고, 그중 14.5ms가 Mass 처리 단계가 도는 틱 그룹의 `EventWait/PrePhysics`였다. 게임 스레드 시간에 기다림을 더하면 프레임과 0.03ms 안에서 맞는다. `GameThreadTime_CriticalPath`가 프레임 시간과 같다면 그 기다림이 임계 경로에 있다는 뜻이다. 그다음 트레이스에서 게임 스레드의 `WaitForTasks`가 끝나는 순간에 끝나는 작업자 이벤트를 찾는다. 이 단계에서는 작업자 하나의 엔진 회피(한 프레임에서 25.6ms)였다.

근거: [글 02 "프레임이 세 스레드 시간보다 길다"](../../Posts/02-mass-engine-path/README.md), [글 02 측정 기록](../../Posts/02-mass-engine-path/measurements.md) "프레임 CSV의 기다림 열"과 "엔진 소스", [engine-notes 11절](../Reference/engine-notes.md)("Game Thread 시간과 워커 대기")

### 액터 군중은 불린 횟수와 한 번의 시간으로 나눠 본다

액터가 많은 장면의 게임 스레드 트레이스에서 한두 컴포넌트의 틱이 대부분을 차지한다. 액터 군중의 비용은 개체마다 컴포넌트가 차례로 틱하는 일의 합이라, 타이머의 합계는 한 번의 비용 × 불린 횟수다. 횟수가 개체 수를 따르는지, 한 번의 비용이 개체 수에 따라 변하는지를 따로 본다.

모든 개체가 `ACharacter`인 호드 1만의 트레이스에서 `UCharacterMovementComponent_TickComponent`는 프레임마다 9,983번 불려 게임 스레드 프레임의 76.4%(411ms)를 썼다. 한 번에 약 41µs(411ms ÷ 9,983)다. 개체당 게임 스레드 시간은 호드 2천까지 18µs 안팎이었고 1만에서 58.1µs였다. 프레임이 50ms를 넘으면 CharacterMovement가 시간을 50ms씩 최대 여덟 번 나눠 계산하고, 엔진은 월드 델타를 0.4초(`MaxUndilatedFrameTime`)로 자른다. 증가는 이 하위 스텝과 같은 쪽이지만, 하위 스텝을 끈 실행으로 가르지는 않았다.

근거: [글 01 "문제"](../../Posts/01-actor-baseline/README.md), [글 01 측정 기록](../../Posts/01-actor-baseline/measurements.md) "트레이스", [engine-notes 11절](../Reference/engine-notes.md)("CharacterMovement 하위 스텝", "월드 델타의 상한 0.4초")

### Insights의 Timers 합계는 스레드를 골라 읽고 불린 횟수로 나눈다

Unreal Insights의 Timers 목록에서 고른 구간의 합계가 프레임 수 × 프레임 시간보다 크다. Timers 목록은 모든 스레드의 같은 이름 타이머를 합쳐 보인다. 엔진 회피를 쓴 단계의 50초 구간에서 `WaitForTasks`는 목록에 60,369번, 1분 42.7초로 나왔다. 게임 스레드만의 값은 22.96초(프레임당 15.1ms)였다.

스레드별 값은 Insights의 명령줄 내보내기로 얻는다. `-threads=GameThread`로 걸러도 GPU 타이머와 `Frame` 행이 섞여 나오니 버린다. 프레임당 값은 구간 합 ÷ 구간 안 `FEngineLoop::Tick`의 Count, 한 번의 시간은 구간 합 ÷ 그 타이머의 Count로 셈한다. 직렬 트레이스의 엔진 회피는 28.92초 ÷ 1,232번 = 23.5ms였다. `UnrealInsights.exe`는 소스 빌드에 없어 한 번 빌드한다(`Build.bat UnrealInsights Win64 Development`).

`Scripts/trace-export.ps1`이 만드는 명령줄(줄였다)

```text
UnrealInsights.exe -OpenTraceFile=<trace.utrace> -NoUI -AutoQuit
  -ExecOnAnalysisCompleteCmd="TimingInsights.ExportTimerStatistics <out.csv> -threads=GameThread -startTime=70 -endTime=120 -columns=*"
```

근거: [글 02 측정 기록](../../Posts/02-mass-engine-path/measurements.md) "Insights 스크린샷"과 "트레이스에서 읽은 값", [글 03 측정 기록](../../Posts/03-separation/measurements.md) "Insights 화면", [글 01 측정 기록](../../Posts/01-actor-baseline/measurements.md) "트레이스", [`trace-export.ps1`](../../Scripts/trace-export.ps1)

### 기법 하나만 끄고 재서 기여를 가른다

단계마다 기법을 더하며 재면 단계 사이의 차이가 작아 구별되지 않는다. 사다리의 차이는 더한 순서에 묶이고, 한 단계에 변수가 둘 이상이면 몫을 나눌 수 없다. 먼 개체의 표현을 더한 세 단계는 하나씩 +0.06, +0.19, +0.46ms로 모두 구별되지 않았다. 그러나 그 사이의 단계까지 모아 견주면 +1.02ms로 구별됐다.

최종 구성에서 기법 하나만 끈 조건을 같은 빌드, 같은 장면에서 재 차이를 본다. 하나를 끄면 다른 것이 함께 빠지는 조건은 참고로 적는다. 예를 들어 하이드레이션을 끄면 캡슐과 액터 애니메이션 예산도 빠진다. 최종 구성에서 열일곱 조건 가운데 일곱이 구별됐다. 200m 밖 지우기 +3.54ms, 액터 메시 고정 바운드 +0.98ms, ISKM 고정 바운드 +0.66ms가 시간을 줄이는 쪽이었다.

근거: [글 04 "결과"](../../Posts/04-quality-by-distance/README.md), [글 05 "기법별 기여"](../../Posts/05-cost-cuts-and-contributions/README.md), [글 05 측정 기록](../../Posts/05-cost-cuts-and-contributions/measurements.md) "그림 2"

### 빨라진 조건이 같은 일을 하는지 본다

어떤 설정을 바꾸자 프레임이 짧아졌다면, 비용이 아니라 일이 준 것일 수 있다. 최종 구성에서 겹침 방지를 끈 조건은 프레임 p95가 0.93ms 짧지만 겹친 쌍(몸통이 겹친 개체 쌍의 수)이 7배였다. 프레임이 아주 길면 엔진도 일을 덜 한다. 월드 델타는 0.4초로, Mass 힘·이동 프로세서의 델타는 0.1초로 잘려 개체가 실제보다 덜 움직인다. 모든 개체가 액터인 호드 1만은 프레임이 0.4초를 넘어 초당 사망이 다른 단계보다 45% 적었다.

프레임 시간과 함께 일의 양을 세는 값(초당 사망, 목표 둘레의 개체 수, 겹친 쌍)을 남기고 조건끼리 견준다. 일이 다르면 그 조건의 프레임 시간을 비용 비교에 쓰지 않고 값만 보고한다.

근거: [글 00 "모든 단계가 같은 일 넷을 한다"](../../Posts/00-goals-and-method/README.md), [글 01 "문제"](../../Posts/01-actor-baseline/README.md), [글 03 "결과"](../../Posts/03-separation/README.md), [engine-notes 11절](../Reference/engine-notes.md)("이동 프로세서의 시간 상한", "월드 델타의 상한 0.4초")
