# Mass 군중 최적화와 프로파일링 팁

UE5의 Mass로 수천\~수만 개체의 군중을 만들고 프로파일링할 때 쓰는 기술 팁 모음이다. 이 프로젝트는 벤치마크 하나를 액터에서 시작해 여덟 단계로 다시 만들며 이 팁을 얻었다. 그 벤치마크에서는 1만 개체의 호드가 맵 위를 원으로 도는 목표(오브)를 사방에서 쫓는다. 엔진 동작은 UE 5.8.4 소스에서 확인했다. 값은 Test 구성 패키지에서 쟀다. 측정 PC는 [pc-specs.md](../Reference/pc-specs.md)에 있다.

## 읽는 법

- 항목은 증상, 설명, 절차, 결과의 순서다. **경고**는 어기면 결과가 틀려지는 것이다. **주의**는 시간을 버리거나 부작용이 있는 것이다.
- 따로 적지 않은 값은 호드 총수 1만, 이동 장면, 오브 시점 카메라, 2560×1440에서 세 번 잰 중앙값이다. 값의 출처는 [글 목록](../../Posts/README.md)의 측정 기록이다.
- 프레임 p95는 측정 창 프레임 시간의 95번째 백분위수다.
- "구별되지 않는다"는 두 조건의 차이가 0.5ms 아래이거나 세 실행의 범위보다 작다는 뜻이다.
- 하이드레이션(Hydration)은 가까운 엔티티에만 진짜 액터를 붙이는 일이다.
- 최종 구성은 모든 기법을 켠 구성이다. 35m 안은 하이드레이션 액터, 35\~200m는 ISKM, 200m 밖은 Niagara 입자로 그린다.
- "끈 조건"은 최종 구성에서 기법 하나만 끄고 같은 조건으로 잰 것이다.

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

**증상**: Mass로 옮긴 뒤에도 게임 스레드가 병목이다. 무엇이 큰지 모를 때 이 항목을 먼저 한다.

**설명**
- 하이드레이션 액터는 액터마다 고정비를 얹는다.
- 그 고정비는 메시 틱과 애니메이션, 캡슐 바디의 이동, 엔티티 위치를 액터에 넣는 동기화다.
- 이 일은 액터마다 매 프레임 돈다.
- 표현 LOD의 액터 상한이 걸리면 엔진은 액터 거리를 당긴다.
- 그때는 전환 거리보다 상한이 이 비용을 정한다.
- Animation Budget Allocator는 액터 메시의 애니메이션에 4ms 예산을 둔 것이다.

**경고**: 하이드레이션을 끈 조건에서는 캡슐, Animation Budget Allocator, 액터 메시 고정 바운드도 함께 빠진다. 이 조건의 값은 참고 값이다.

**절차**
1. 같은 빌드에서 하이드레이션만 끈 조건을 재 둔다.

**결과**
- 최종 구성의 액터는 평균 578개다.
- 최종 구성에서 하이드레이션을 끈 조건은 프레임 p95가 6.98ms 짧다.
- 그 조건의 게임 스레드 시간은 11.9ms에서 5.23ms로 준다.
- 그 조건의 병목은 GPU(6.21ms)로 바뀐다.
- 캡슐만 끈 조건도 프레임 p95가 1.86ms 짧다.
- 이 프로젝트는 가까운 개체의 캡슐이 필요한 기능이라 액터를 남겼다.
- 대신 이 프로젝트는 액터 하나의 비용을 줄였다. 그 방법은 아래의 캡슐, 액터 메시 고정 바운드, Budget Allocator 항목에 있다.

### 액터 경계에는 최소 유지 시간을 둔다

**증상**: 액터 경계 근처의 개체가 액터와 인스턴스 사이를 짧게 오간다. 그 개체는 전환 순간의 튐을 되풀이한다.

**설명**
- 엔진 표현 LOD의 거리 히스테리시스(기본 10%)는 한 단계 가까운 구간으로 들어올 때만 걸린다.
- 엔진의 계산 반복은 High(0번)에서 멈춘다.
- 그래서 액터 경계에는 이 히스테리시스가 없다.
- 상한이 당긴 경계도 같은 함수를 쓴다.
- 화면 안팎 판정의 히스테리시스(`DistanceToFrustumHysteresis`)는 기본값이 0이다.
- 엔진에는 최소 유지 시간에 해당하는 설정이 없다.

**절차**
1. Lab 프로세서를 이번 프레임의 LOD가 정해진 뒤에 둔다.
2. 그 프로세서를 표현이 바뀌기 전에 둔다.
3. 그 프로세서에서 2초 안에 바로 전 표현으로 돌아가려는 개체의 LOD를 되돌린다.

`Source/HordeOptLab/LabMassProcessors.cpp`, `ULabRepHoldProcessor::ULabRepHoldProcessor`

```cpp
ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::LOD);
ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::VisualizationProcessing);
```

**결과**
- 왕복 전환은 2초 안에 같은 경계를 반대로 되넘은 수다.
- 최종 구성에서 이 규칙만 끈 조건은 왕복 전환이 초당 4.34건이다.
- 그 왕복 전환은 모두 액터 경계에서 난다.
- 이 규칙을 켜면 왕복 전환은 0건이다.
- 프레임 p95는 구별되지 않는다.

### 풀에 넣는 액터는 틱과 중요도와 콜리전을 끈다

**증상**: Mass 액터 풀로 돌려보낸 액터가 숨은 채 틱한다. 또는 마지막 자리에 남은 캡슐이 트레이스에 맞는다.

**설명**
- `UMassActorSpawnerSubsystem`의 풀은 액터를 숨기기만 한다.
- 컴포넌트 등록, 틱, 콜리전은 그대로 남는다.
- `USkeletalMeshComponentBudgeted`는 Budget Allocator에 등록된 채 남는다.
- 엔진은 풀에서 꺼낸 액터의 숨김을 풀고 엔티티 변환(스케일 1)으로 옮긴다.
- 엔진은 그다음 `PrepareForGame`을 부른다. 엔진은 `BeginPlay`를 다시 부르지 않는다.

**절차**
1. `IMassActorPoolableInterface`의 `PrepareForPooling`에서 메시 틱, 중요도, 액터 콜리전을 끈다.
2. `PrepareForGame`에서 셋을 되돌린다.
3. `PrepareForGame`에서 액터에 넣어 둔 스케일을 다시 넣는다.

`Source/HordeOptLab/LabMinion.cpp`, `ALabMinionMassActor`와 하위 클래스의 `PrepareForPooling_Implementation`(둘을 합쳐 줄였다)

```cpp
GetMesh()->SetComponentTickEnabled(false);
SetActorEnableCollision(false); // 엔진 풀은 콜리전을 그대로 둔다
CastChecked<USkeletalMeshComponentBudgeted>(GetMesh())->SetComponentSignificance(0.f);
```

**결과**: 이 처리만 끈 조건은 재지 않았다.

### 액터 메시에는 URO와 Budget Allocator 가운데 하나를 고른다

**증상**: Budget Allocator를 켠 액터에 URO(Update Rate Optimization)도 걸려 있다고 본다. 그 가정으로 결과를 읽으면 틀린다.

**설명**
- Budget Allocator는 켜져 있으면 등록하는 메시의 `bEnableUpdateRateOptimizations`를 끄고 갱신을 직접 맡는다.
- Budget Allocator는 꺼지면 URO를 다시 켠다.
- Budget Allocator는 CVar `a.Budget.Enabled`(기본 0)와 월드의 `SetEnabled`가 모두 참일 때만 켜진다.
- 엔진은 Budget Allocator를 월드마다 만든다. `SetEnabled`의 처음 값은 거짓이다.
- 스켈레탈 메시의 URO 기본값은 끔이다.
- Budget Allocator는 수요가 예산 안이면 모든 메시를 매 프레임 갱신한다. 그때 Budget Allocator는 일을 줄이지 않는다.

**경고**: CVar만 넣으면 Budget Allocator는 켜지지 않는다.

**절차**
1. 액터 메시의 클래스를 `USkeletalMeshComponentBudgeted`로 바꾼다.
2. CVar `a.Budget.Enabled`와 `a.Budget.BudgetMs`를 넣는다.
3. 같은 자리에서 월드의 Budget Allocator에 `SetEnabled(true)`를 부른다.
4. Budget Allocator가 실제로 켜졌는지 실행 기록에 남긴다.

`Source/HordeOptLab/LabGameMode.cpp`(줄였다)

```cpp
LabSetCVar(TEXT("a.Budget.Enabled"), TEXT("1"));
LabSetCVar(TEXT("a.Budget.BudgetMs"), TEXT("4"));
IAnimationBudgetAllocator::Get(GetWorld())->SetEnabled(true); // CVar만으로는 켜지지 않는다
```

**결과**
- 모든 개체가 액터인 군중에 예산 4ms를 건 조건을 쟀다.
- 호드 총수 2천(애니메이션 4.17ms)에서는 프레임 p95가 41.3ms에서 36.9ms로 줄었다.
- 호드 총수 1천(애니메이션 3.71ms)에서는 프레임 p95가 0.52ms 길었다.
- 프레임 p95가 준 것은 호드 총수 2천에서만이다.

### 액터 캡슐은 QueryOnly와 전용 트레이스 채널로 켠다

**증상**: 가까운 개체에 판정용 캡슐이 필요하다. 그 캡슐이 액터 비용의 큰 몫이 된다.

**설명**
- `ACharacter`의 캡슐은 `Pawn` 프로필(QueryAndPhysics)을 쓴다.
- 그 캡슐은 오버랩과 물리 볼륨 갱신이 켜져 있다.
- 피직스 상태가 있는 캡슐은 옮길 때마다 바디 변환을 피직스 씬에 보낸다(`SendPhysicsTransform` → `SetGlobalPose`).
- QueryOnly 캡슐도 바디가 있어 이 일을 한다.
- `NoCollision` 캡슐만 바디를 만들지 않는다.

**절차**
1. `Config/DefaultEngine.ini`에 기본 응답이 무시인 전용 트레이스 채널을 만든다(`ECC_GameTraceChannel1`, `DefaultResponse=ECR_Ignore`).
2. 캡슐의 오버랩과 물리 볼륨 갱신을 끈다.
3. 캡슐의 모든 채널 응답을 무시로 둔다.
4. 전용 트레이스 채널만 막음으로 둔다.
5. 캡슐을 QueryOnly로 바꾼다.

`Source/HordeOptLab/LabMinion.cpp`, `ALabMinionMassActor::BeginPlay`(캡슐을 켜는 갈래만 남겨 줄였다)

```cpp
Capsule->SetShouldUpdatePhysicsVolume(false);
Capsule->SetGenerateOverlapEvents(false);
FCollisionResponseContainer Responses(ECR_Ignore);
Responses.SetResponse(LabSnipeTraceChannel, ECR_Block); // 전용 트레이스 채널만 막는다
Capsule->SetCollisionResponseToChannels(Responses);
Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
```

**결과**
- 이 설정을 해도 최종 구성에서 캡슐만 끈 조건은 프레임 p95가 1.86ms 짧다.
- 그래서 남은 비용은 바디 자체다.
- CVar `p.Chaos.AccelerationStructureIsolateQueryOnlyObjects`는 QueryOnly 바디를 가속 구조의 따로 된 칸에 넣는다.
- 이 CVar는 최종 측정에 넣지 않았다.

### 액터 메시에 고정 바운드를 쓴다

**증상**: 액터 수가 늘면 Budget Allocator가 재는 액터 메시의 애니메이션 시간이 는다.

**설명**
- 엔진은 애니메이션을 갱신할 때마다 캐시한 바운드를 지우고 다시 셈한다(`FinalizeAnimationUpdate`).
- 기본값에서는 엔진이 피직스 에셋의 바디마다 뼈 변환으로 상자를 셈한다.
- `bComponentUseFixedSkelBounds`를 켜면 엔진은 메시 에셋의 바운드에 루트 뼈의 이동만 더한다.

**주의**: 메시 에셋의 바운드가 피직스 에셋으로 셈한 바운드보다 작으면 화면 컬링과 메시 LOD 고르기가 달라진다. 이 프로젝트의 메시는 메시 에셋의 바운드가 더 작다.

**절차**
1. 하이드레이션 액터 메시의 `bComponentUseFixedSkelBounds`를 켠다.
2. `BoundsScale`을 1.1로 둔다.

`Source/HordeOptLab/LabMinion.cpp`, `ALabMinionMassActor::BeginPlay`

```cpp
SkelMesh->bComponentUseFixedSkelBounds = FLabConfig::Get().bFixedSkelBounds;
if (FLabConfig::Get().bFixedSkelBounds)
{
	SkelMesh->SetBoundsScale(FLabQuality::FixedSkelBoundsScale); // 1.1
}
```

**결과**
- 최종 구성에서 액터 메시 고정 바운드만 끈 조건은 프레임 p95가 0.98ms 길다.
- 그 조건에서 액터 메시의 애니메이션 시간은 2.71ms에서 3.63ms로 는다.

### 캡슐을 겨누는 라인 트레이스는 시작점을 정면 평면에서 비킨다

**증상**: 캡슐 중심을 겨눈 라인 트레이스가 일정 비율로 캡슐을 놓친다.

**설명**
- 이 프로젝트의 조건에서는 대상이 시작점을 바라볼 때 트레이스가 캡슐을 놓쳤다.
- 그때 시작점은 캡슐의 정면 평면 위에 있다. 정면 평면은 정면 축과 연직축이 이루는 평면이다.
- 빗나간 순간에도 같은 자리의 작은 구 겹침 질의는 캡슐을 찾았다.
- 브로드페이즈를 거치지 않는 `LineTraceComponent`도 캡슐을 놓쳤다.
- 캡슐 레이캐스트(`Chaos::Raycasts::RayCapsule`) 안의 어느 분기가 원인인지는 찾지 못했다.

**절차**
1. 시작점이나 끝점을 정면 평면에서 몇 cm 비킨다.

**결과**
- 목표가 서 있는 정지 장면에서 트레이스는 3,780회 중 63회(1.67%) 캡슐을 놓쳤다.
- 개발 빌드 진단에서 시작점을 옆으로 1.7cm 비키자 빗나감이 0회였다.
- 이 프로젝트는 이 트레이스를 확인에만 쓴다. 그래서 아직 고치지 않고 빗나감을 보고만 한다.

## 인스턴스와 입자를 쓸 때

### 먼 개체는 화면 밖 여부보다 거리로 먼저 지운다

**증상**: 먼 개체를 인스턴스로 그리는 비용이 화면에 보이는 수를 따르지 않는다. 그 비용은 표현을 가진 개체 수를 따라 는다.

**설명**
- 엔진 표현 프로세서는 표현이 없음(None)이 아닌 개체를 화면 밖이어도 `CanBeSeen`으로 둔다.
- 그래서 인스턴스 갱신 프로세서의 청크 필터(`AreAnyEntitiesVisibleInChunk`)는 그 청크를 거르지 않는다.
- 표현이 None인 개체는 `CulledByDistance` 청크로 모인다.
- 표현 프로세서는 그 청크를 0.5초(± 10%)마다만 다시 본다.

**주의**: 엔진은 Off에서 들어오는 개체를 80\~200m 구간의 90% 지점(188m)에서 그린다. 원인은 히스테리시스다. 엔진은 그 개체를 청크 갱신 간격만큼 늦게 그린다.

**절차**
1. 표현 LOD의 Off 구간에 `EMassRepresentationType::None`을 준다.
2. Off 거리를 둔다.

`Source/HordeOptLab/LabCrowdSubsystem.cpp`, `ULabCrowdSubsystem::BuildEntityConfig`

```cpp
const float Bands[EMassLOD::Max] = { 0.f, HydrateCm, 8000.f, FLabConfig::Get().RepOffCm() }; // 0, 35, 80, 200m
// ...
Visual->Params.LODRepresentation[EMassLOD::Off] = EMassRepresentationType::None;
```

**결과**
- 최종 구성에서 200m 밖을 그리지 않는 일만 끈 조건을 쟀다.
- 그 조건에서 엔진은 200m 밖 개체를 맵 끝까지 ISKM으로 그린다.
- 그 조건의 프레임 p95는 3.54ms 길어진다.
- 그 조건의 게임 스레드 시간은 3.23ms 길어진다.
- 화면 밖만 지우는 변형은 앞선 시험에서 이득이 없었다. 그 원인은 가르지 않았다.

### 움직이는 인스턴스는 Lumen 장면에서 뺀다

**증상**: 움직이는 정지 메시 인스턴스 수천 개를 그리면 GPU 시간이 크게 는다.

**설명**
- 소프트웨어 Lumen은 `AffectsDynamicIndirectLighting()`이 참이고 거리 필드를 가진 프리미티브를 장면에 넣는다.
- 거리 필드는 정지 메시, Nanite, 스플라인만 가진다.
- 인스턴스가 움직인 프리미티브는 매 프레임 Lumen 장면 갱신 목록에 든다.
- 스켈레탈 메시는 원래 Lumen 장면 밖이다.
- 그래서 이 비용은 액터 군중에서는 드러나지 않다가 인스턴스로 바꿀 때 나타난다.

**주의**: 비주얼라이저는 컴포넌트를 처음 쓰일 때 늦게 만든다. 그래서 아래 절차는 컴포넌트를 매 틱 훑는다.

**절차**
1. Mass 비주얼라이저가 만든 ISM·ISKM 컴포넌트마다 `SetAffectDynamicIndirectLighting(false)`를 부른다.
2. 매 틱 비주얼라이저의 컴포넌트를 훑는다.
3. 아직 고치지 않은 컴포넌트만 고친다.

`Source/HordeOptLab/LabCrowdSubsystem.cpp`, `ULabCrowdSubsystem::ExcludeVisualsFromLumen`(줄였다)

```cpp
for (TActorIterator<AMassVisualizer> It(GetWorld()); It; ++It) // 컴포넌트는 늦게 생기므로 매 틱 훑는다
{
	// ... ISM·ISKM 컴포넌트 가운데 bAffectDynamicIndirectLighting이 아직 켜진 것만
	LabExcludeFromLumen(*Component); // SetAffectDynamicIndirectLighting(false)
}
```

**결과**
- 개체는 계속 Lumen의 간접광을 받는다.
- 개체는 계속 그림자를 그린다.
- 총괄 측정 밖의 초기 시험에서 GPU 시간이 29.0ms에서 6.0ms로 줄었다. 그 시험은 Test 패키지에서 움직이는 ISM 2,000개로 했다.
- 어느 Lumen 패스가 시간을 썼는지는 GPU 트레이스로 보지 않았다.

### Mass 인스턴스의 그림자는 설명자에서 켠다

**증상**: 하이드레이션 액터는 그림자를 그린다. 인스턴스로 바뀐 개체는 그림자가 없다.

**설명**
- 엔진의 ISM 설명자(`FMassStaticMeshInstanceVisualizationMeshDesc`)는 `bCastShadows`의 기본값이 거짓이다.
- 엔진의 ISKM 설명자(`FMassSkinnedMeshInstanceVisualizationMeshDesc`)도 `bCastShadows`의 기본값이 거짓이다.
- 엔진은 이 값을 설명자마다 만드는 컴포넌트의 `SetCastShadow`로만 넘긴다.
- 인스턴스 단위 그림자 플래그는 없다.
- `bCastShadows`는 설명자 해시에 든다. 그래서 그림자만 다른 두 설명자는 컴포넌트가 둘이 된다.
- 그림자 비용은 GPU에 실린다.

**절차**
1. 설명자를 만들 때 `bCastShadows`를 참으로 둔다.

`Source/HordeOptLab/LabCrowdSubsystem.cpp`, `ULabCrowdSubsystem::BuildEntityConfig`

```cpp
MeshDesc.bCastShadows = true; // 엔진 기본값은 false(MassRepresentationTypes.h:105)
SkinnedDesc.bCastShadows = FLabIskmParams::bCastShadows; // true. 엔진 기본값은 false(MassSkinnedMeshRepresentationTypes.h:79)
```

**결과**
- 최종 구성에서 80m 밖 그림자를 다시 켠 조건은 GPU 시간이 0.48ms 길다.
- 병목이 게임 스레드라 프레임 p95는 구별되지 않았다.

### ISKM 컴포넌트에 카메라를 따라가는 고정 바운드를 넣는다

**증상**: ISKM 인스턴스가 매 프레임 움직이면 컴포넌트 바운드 계산이 인스턴스 수를 따라 는다.

**설명**
- ISKM 컴포넌트는 인스턴스가 바뀐 프레임마다 `UpdateBounds()`를 부른다.
- 바운드 계산은 `bFastBuild = false`로 박혀 있다. 그래서 엔진은 인스턴스마다 애니메이션 바운드를 변환해 합친다.
- `SetPrimitiveBoundsOverride(FBox)`로 상자를 넣으면 엔진은 이 반복 없이 그 상자를 돌려준다.
- 인스턴스 컬링과 GPU LOD 선택은 인스턴스 데이터로 한다. 그래서 상자는 컴포넌트 단위 컬링에만 쓰인다.
- 인스턴스가 매 프레임 움직이는 컴포넌트에서는 엔진이 다음 갱신 때 새 바운드를 렌더 스레드로 보낸다.
- 그래서 렌더 상태를 다시 만들 필요가 없다.

**절차**
1. 카메라 둘레에 상자를 넣는다.
2. 카메라가 정한 거리보다 멀리 움직이면 상자를 옮긴다.
3. 옮긴 상자를 다시 넣는다.

`Source/HordeOptLab/LabCrowdSubsystem.cpp`, `ULabCrowdSubsystem::ApplyFixedIskmBounds`

```cpp
const FBox Fixed(IskmBoundsCenter + FVector(-HalfXY, -HalfXY, -500.f), IskmBoundsCenter + FVector(HalfXY, HalfXY, 1500.f));
Component.SetPrimitiveBoundsOverride(Fixed); // 상자를 옮긴 틱에도 다시 넣는다
Component.UpdateBounds();
```

**결과**: 최종 구성에서 ISKM 고정 바운드만 끈 조건은 프레임 p95가 0.66ms 길다.

### 움직이는 ISKM 엔티티는 파괴하지 말고 풀로 다시 쓴다

**증상**: 개체가 죽고 다시 생기는 동안 ISKM 트랙 수와 GPU 메모리가 실행 내내 는다.

**설명**
- Mass는 엔티티가 ISKM 컴포넌트에 처음 들 때 엔티티 핸들을 키로 트랙을 잡는다.
- Mass는 트랙을 돌려받는 `DeallocateTrack`을 정지 표현 경로에서만 부른다. 그 경로는 `FMassStaticRepresentationTag`를 요구한다.
- 그래서 움직이는 엔티티를 파괴하면 트랙이 활성인 채 남는다.
- 렌더러는 남은 트랙도 매 프레임 평가한다.
- 이 문제는 5.8.4에도 남아 있다. [Epic 개발자 포럼](https://forums.unrealengine.com/t/mass-crowds-using-skinnedmeshinstance-leak-gpu-memory-every-wave-of-spawned-and-killed-entities-permanently-grows-the-animation-track-pool-5-8/2742508)에 같은 보고가 있다.
- 풀에서 꺼낸 엔티티는 핸들이 같다. 그래서 엔진은 옛 트랙을 다시 쓴다.

**경고**: 엔진은 트랙이 있으면 클립 번호가 바뀔 때만 재생 데이터를 쓴다.

**절차**
1. 죽은 엔티티를 파괴하지 않고 종류별 풀에 넣는다.
2. 재생성 때 풀에서 엔티티를 꺼낸다.
3. 꺼낸 엔티티에 위치, 체력, 상태를 다시 넣는다.
4. 상태나 시작 시각이 바뀐 프레임에 `SetAutoPlayData`를 직접 부른다.

`Source/HordeOptLab/LabCrowdSubsystem.cpp`, `ULabCrowdSubsystem::SpawnEntities`(줄였다)

```cpp
const int32 NumReuse = bPool ? FMath::Min(EntityPool[Type].Num(), Indices.Num()) : 0;
for (int32 k = 0; k < NumReuse; ++k)
{
	const FMassEntityHandle Entity = EntityPool[Type].Pop(EAllowShrinking::No);
	ReinitPooledEntity(EntityManager, Entity, Locations[Indices[k]], Serials[Indices[k]]);
}
```

**결과**
- 240초 실행 끝의 트랙은 엔티티 풀을 켜면 20,215개, 끄면 30,797개였다.
- 그 실행의 프레임 p95는 구별되지 않았다.
- 960초씩 돌린 참고 측정에서 엔티티 풀을 끈 쪽의 트랙은 120,428개가 됐다.
- 그 참고 측정의 GPU 메모리는 엔티티 풀을 끈 쪽이 5,110MB, 켠 쪽이 3,370MB였다.
- 엔티티 풀을 끈 쪽은 13분 구간부터 프레임 p95가 0.6ms 넘게 높았다.

### ISKM 트랙 풀은 처음에 용량까지 늘려 둔다

**증상**: ISKM으로 그린 같은 종류의 개체 전체가 가끔 한 프레임 세로로 길게 끌린다. 이 일은 트랙 수가 느는 동안에만 난다.

**설명**
- 모든 개체가 트랙을 가지면 이 일은 멈춘다.
- 트랙 수가 바뀌면 렌더러는 그 컴포넌트의 뼈대 버퍼(트랙 수 × 뼈 수 × 2)를 다시 잡는다.
- 렌더러는 옛 자리를 먼저 해제하고 새 자리를 할당한다.
- `FSpanAllocator`는 맞는 빈 구간이 없으면 해제한 구간을 이웃과 합친다. 그래서 옛 자리와 겹치는 자리를 줄 수 있다.
- 옛 내용을 옮기는 셰이더는 같은 버퍼를 병렬로 읽고 쓴다.
- 그래서 이전 프레임 자세 칸이 깨진다. 모션 블러는 그 칸으로 틀린 속도를 그린다.

**주의**: 트랙이 용량을 넘으면 이 일은 다시 생긴다.

**절차**
1. 컴포넌트가 처음 쓰이는 프레임에 트랙을 용량(종류의 개체 수 × 1.1 + 64)만큼 할당한다.
2. 할당한 트랙을 모두 해제한다.
3. 위 항목의 엔티티 풀을 함께 쓴다.

`Source/HordeOptLab/LabIskmProcessor.cpp`, `LabReserveTracks`

```cpp
TArray<int32> Temp;
while (int32(Provider->GetUniqueAnimationCount()) < Capacity) { Temp.Add(Provider->AllocateTrack()); }
for (const int32 TrackIndex : Temp) { Provider->DeallocateTrack(TrackIndex); } // 끝 번호는 남는다
```

**결과**
- 트랙 풀의 끝 번호가 남아 트랙 수는 다시 바뀌지 않는다.
- 엔진의 다음 할당은 빈 번호를 다시 쓴다.
- 속도 버퍼를 찍은 개발 빌드 900프레임 캡처에서 튄 프레임이 42개에서 0개가 됐다.
- 이 처리만 끈 조건의 프레임 p95는 구별되지 않았다.

### ISKM에는 가산 클립 대신 가산을 끈 복제본을 넣는다

**증상**: ISKM 개체가 피격 클립을 재생하는 동안 사라지거나 누운 모양이 된다.

**설명**
- ISKM 트랙 제공자(`UAnimSequenceTransformProviderData`)의 렌더러 경로는 압축을 푼 지역 자세를 그대로 뼈 변환으로 쓴다.
- 그 경로에는 기준 자세를 읽는 코드가 없다.
- 가산 클립이면 이 값은 기준 자세와의 차이다. 그래서 뼈가 원점 근처로 모인다.
- 액터의 단일 노드 재생과 AnimToTexture 굽기는 기준 자세에 차이를 더한다. 그래서 같은 클립이 액터와 VAT 인스턴스에서는 맞다.
- 엔진은 원본 데이터를 늘 전체 자세로 둔다.
- 엔진은 가산 값을 압축할 때 만든다.

**절차**
1. 에셋 스크립트에서 가산 클립을 복제한다.
2. 복제본의 가산 종류를 `AAT_None`으로 둔다.
3. 그 복제본을 트랙 제공자에 넣는다.

`Scripts/create-iskm.py`, `provider_clip`(줄였다)

```python
if kind == unreal.AdditiveAnimationType.AAT_NONE:
    return anim
full = eal.duplicate_asset(src_path, path)
full.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)  # PostEditChange가 다시 압축한다
```

**결과**: 엔진은 복제본을 기준 자세에 차이를 더한 전체 자세로 다시 압축한다.

### AnimToTexture Bone 방식 메시의 바운드는 정점 범위로 넓힌다

**증상**: VAT 인스턴스가 죽기 애니메이션을 재생하는 동안 무기 끝 같은 먼 정점의 그림자가 바닥에 남는다.

**설명**
- AnimToTexture의 Bone 모드는 정지 메시 바운드를 뼈 위치 범위로만 넓힌다.
- AnimToTexture의 Vertex 모드는 정지 메시 바운드를 모든 프레임의 정점 범위로 넓힌다.
- Virtual Shadow Map은 물체 바운드 안쪽의 페이지만 무효화한다.
- 그래서 바운드 밖으로 나간 정점의 그림자가 캐시에 남는다.
- 셰이더로 정점을 옮기는 기법(VAT, WPO)은 바운드를 직접 챙겨야 한다.

**주의**: 그림자 캐시를 매 프레임 무효화하는 설정(`ShadowCacheInvalidationBehavior::Always`)도 그림자를 지웠다. 이 프로젝트는 비용 때문에 이 설정을 쓰지 않았다.

**절차**
1. 같은 메시를 Vertex 모드로 먼저 굽는다.
2. 구운 결과에서 정점 범위(`VertexMinBBox`, `VertexSizeBBox`)를 얻는다.
3. Bone 메시의 `PositiveBoundsExtension`·`NegativeBoundsExtension`을 그 범위까지 넓힌다.

`Scripts/bake-vat.py`(줄였다)

```python
vmin, vsize = VERTEX_BBOX[0]  # 먼저 구운 Vertex 모드의 정점 범위
new_max = unreal.Vector(max(base_max.x, vmin.x + vsize.x), max(base_max.y, vmin.y + vsize.y), max(base_max.z, vmin.z + vsize.z))
sm.set_editor_property("positive_bounds_extension", unreal.Vector(new_max.x - base_max.x, new_max.y - base_max.y, new_max.z - base_max.z))
# negative_bounds_extension도 min으로 같은 식을 쓴다
```

### Niagara 메시 입자의 VAT 프레임은 Dynamic Parameter로 넘긴다

**증상**: ISM에서 맞게 걷던 VAT 머티리얼을 Niagara 메시 입자에 쓰면 프레임이 엉뚱하게 나온다.

**설명**
- Niagara 메시 렌더러는 GPU Scene 경로에서 인스턴스 커스텀 데이터 float 하나를 잡는다.
- Niagara 메시 렌더러는 그 0번에 입자 인덱스의 비트를 쓴다.
- 그래서 `PerInstanceCustomData`로 프레임을 읽는 VAT 머티리얼은 Niagara에서 입자 인덱스를 읽는다.
- 정적 스위치 `AutoPlay`와 `UseDynamicParameters`를 켜면 `GetFrameSwitch`는 프레임을 Dynamic Material Parameter 1번에서 읽는다.
- 1번의 네 값은 TimeOffset, Playrate, StartFrame, EndFrame이다.

**절차**
1. AnimToTexture의 머티리얼 함수 `GetFrameSwitch`에서 정적 스위치 `AutoPlay`와 `UseDynamicParameters`를 켠다.
2. 이미터에 `DynamicMaterialParameters` 모듈을 더한다.
3. 그 모듈의 쓰기 인덱스 0을 끈다.
4. 쓰기 인덱스 1을 Float로 둔다.
5. 인덱스 1에 네 값을 넣는다.

`Scripts/create-crowd-fx.md`(설정)

```text
MI 정적 스위치: AutoPlay = true, UseDynamicParameters = true
DynamicMaterialParameters: Write Parameter Index 0 = Off, Write Parameter Index 1 = Float
Index 1 Param 1..4 = TimeOffset, Playrate, StartFrame, EndFrame
```

**결과**
- Playrate를 0으로, StartFrame을 죽기 클립 안에 둔 화면에서 모든 입자가 같은 쓰러진 자세로 멈췄다.
- 이 화면으로 네 값이 제자리에 들어간 것을 확인했다.

## Mass 프로세서를 짤 때

### 엔진 프로세서가 한 스레드로 도는지 먼저 본다

**증상**: 트레이스에서 작업자 스레드 하나가 프레임 대부분 동안 한 프로세서를 돈다. 게임 스레드는 그동안 기다린다.

**설명**
- 엔진 이동 회피(`UMassMovingAvoidanceProcessor`)는 `ForEachEntityChunk`로 청크를 한 스레드에서 차례로 돈다.
- 엔진의 서 있는 회피도 같다.
- 이동 회피 쿼리는 Medium·Low·Off LOD 태그가 있는 개체를 뺀다.
- Simulation LOD를 켜지 않으면 이 태그가 없다. 그래서 전 개체가 순회에 든다.
- 엔진 이동(`UMassApplyMovementProcessor`)과 방향 보간(`UMassSmoothOrientationProcessor`)도 병렬 순회가 아니다.

**절차**
1. 엔진 프로세서를 쓰기 전에 소스에서 `ForEachEntityChunk`인지 `ParallelForEachEntityChunk`인지 본다.
2. 엔진 회피 대신 위치를 직접 미는 밀어내기 프로세서를 짠다.
3. 밀어내기 프로세서의 이웃 계산을 128개씩 묶어 `ParallelFor`로 나눈다.
4. 모든 개체가 같은 위치 사본을 읽게 한다.

`Source/HordeOptLab/LabSeparation.h`, `FLabSeparationSolver::Solve`(줄였다)

```cpp
ParallelFor(NumBatches, [this, &Params, Num](int32 Batch)
{
	for (int32 I = Batch * BatchSize; I < FMath::Min(Num, (Batch + 1) * BatchSize); ++I)
	{ const FVector2f Delta = Resolve(I, Params); Next[I] = Pos[I] + Delta; } // 모두 Pos를 읽고 Next에 쓴다
}, Flags);
```

**결과**
- 엔진 회피를 그대로 쓴 단계에서 회피는 작업자 하나에서 프레임마다 24.7ms를 돌았다.
- 직렬 트레이스에서 그 회피는 Mass 프로세서 시간의 80.7%(23.5ms)였다.
- 밀어내기 프로세서의 결과는 스레드 수와 무관하게 같다.
- 최종 구성에서 밀어내기 프로세서를 엔진 회피로 되돌린 조건은 프레임 p95가 8.57ms 길다.

### 개체마다 독립인 엔진 프로세서는 청크 병렬 사본으로 바꾼다

**증상**: 한 스레드로 도는 엔진 프로세서가 위치를 쓰는 사슬 앞에 있다. 그 프로세서가 뒤 프로세서와 게임 스레드를 늦춘다.

**설명**
- 엔진 방향 보간의 두 쿼리는 `ForEachEntityChunk`로 돈다.
- 방향 보간은 개체마다 이동 목표와 바라는 속도를 읽고 자기 회전과 `DesiredFacing`만 쓴다.
- 쿼리가 `private`이고 클래스가 `MinimalAPI`라 상속으로 순회만 바꿀 수 없다.
- 처리 큐는 `ParallelForEachEntityChunk`의 청크를 하나씩 `UE::Tasks`로 띄운다.
- 그래서 게임 스레드 프로세서여도 그 청크는 작업자로 간다.
- 수학 함수(`LerpAngle`, `ExponentialSmoothingAngle`)는 헤더 인라인이다.

**주의**: 엔진의 `SetShouldAutoRegisterWithGlobalList`는 에디터 빌드에서 그 값을 `DefaultMass.ini`에 써 버린다.

**절차**
1. 같은 쿼리와 식을 가진 사본을 게임 모듈에 둔다.
2. 사본의 두 쿼리를 `ParallelForEachEntityChunk`로 돌린다.
3. 게임 모듈 시작 때 엔진 프로세서 CDO의 `bAutoRegisterWithProcessingPhases`를 리플렉션으로 바꿔 자동 등록을 끈다.

`Source/HordeOptLab/LabSmoothOrientation.cpp`, `ULabSmoothOrientationProcessor::Execute`(엔진은 같은 자리가 `ForEachEntityChunk`다)

```cpp
HighResEntityQuery.ParallelForEachEntityChunk(Context, HighRes);
LowResEntityQuery_Conditional.ParallelForEachEntityChunk(Context, LowRes);
```

**결과**
- 사본의 결과는 엔진과 비트 단위로 같았다.
- 직렬 트레이스에서 방향 보간은 프레임당 0.82ms에서 0.22ms가 됐다.

### 게임 스레드 프로세서의 주 쿼리에 위치 쓰기를 선언하지 않는다

**증상**: 게임 스레드의 가벼운 프로세서가 하는 일보다 오래 걸린다. 트레이스에서 그 프로세서는 위치를 쓰는 다른 프로세서가 끝나는 순간에 함께 끝난다.

**설명**
- `mass.ForceInlineProcessorExecution`(기본 참)에서 한 쿼리의 직접 쓰기와 다른 쿼리의 직접 읽기가 겹치면, 엔진은 아키타입이 겹치는지 보지 않고 두 쿼리를 전역 충돌로 묶는다.
- 묶인 쿼리는 시작하기 전에 상대가 끝나기를 기다린다.
- 그 기다림은 자체 타이머 없이 기다린 프로세서의 실행 시간에 잡힌다.
- ReadWrite 선언은 읽기와 쓰기 양쪽에 든다.
- 실행 중 쿼리 순서는 자유다.
- 엔진은 돌리지 않은 쿼리를 검사하지 않는다.

**절차**
1. 드물게 쓰는 프래그먼트는 따로 된 쿼리로 뺀다.
2. 쓸 개체가 있는 프레임에만 그 쿼리를 돌린다.

`Source/HordeOptLab/LabMassProcessors.cpp`, `ULabFarHitIskmProcessor::ConfigureQueries`와 `Execute`(줄였다)

```cpp
EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadWrite); // 주 쿼리에는 위치가 없다
CatchUpQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);            // 위치는 따라잡기 쿼리만
if (CatchUps.Num() > 0) { CatchUpQuery.ForEachEntityChunk(Context, /* ... */); }
```

**결과**: 그리는 끝 밖의 피격 개체를 ISKM으로 올리는 프로세서를 이렇게 고쳤다. 개발 빌드 진단에서 그 프로세서의 실행 시간이 거의 0이 됐다.

### 프로세서 단위 서브시스템 선언이 다른 프로세서를 막는지 본다

**증상**: 게임 스레드 러너가 고를 프로세서가 없어 노는 빈틈이 생긴다. 그 빈틈의 끝은 작업자의 다른 프로세서가 끝나는 때와 맞는다.

**설명**
- 실행 큐는 `ProcessorRequirements`에 선언한 서브시스템 요구를 그대로 담는다.
- 한 프로세서가 쓰고 다른 프로세서가 읽거나 쓰면, 실행 큐는 두 프로세서를 서로 막는 쌍으로 둔다.
- `TMassExternalSubsystemTraits::ThreadSafeWrite`는 의존성 해결기만 읽는다. 그래서 이 충돌을 풀지 못한다.
- 러너는 프로세서를 고를 때 충돌 칸을 잡고 쿼리를 도는 동안만 놓는다.
- 그래서 쿼리 밖에서 오래 일하는 프로세서는 상대를 끝까지 막는다.
- 쿼리에 선언한 서브시스템은 그 쿼리의 데이터 접근에만 든다(`bAggregateRequirements` 기본 거짓).

**절차**
1. 트레이스에서 러너의 빈틈(`MassProcessingQueue Main-Thread Runner Task`의 Excl)을 찾는다.
2. 그 빈틈의 끝에 끝난 프로세서를 맞춰 본다.
3. 서브시스템 쓰기는 꼭 필요한 프로세서에만 프로세서 단위로 선언한다.

`Source/HordeOptLab/LabMassProcessors.cpp`, 두 프로세서의 `ConfigureQueries`

```cpp
// ULabSeparationProcessor: 개발 확인 로그에 쓰는 값을 읽는다
ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadOnly);
// ULabFarHitIskmProcessor: 위와 서로 막는 쌍이 된다
ProcessorRequirements.AddSubsystemRequirement<ULabCrowdSubsystem>(EMassFragmentAccess::ReadWrite);
```

**결과**
- 이 프로젝트의 트레이스에서는 코드의 두 선언이 밀어내기 프로세서와 피격 ISKM 프로세서를 서로 막았다.
- 읽기 선언을 빼면 표현이 밀기 전 위치를 읽는 프레임이 늘었다.
- 그래서 이 프로젝트는 선언을 두었다.
- 대신 이 프로젝트는 앞쪽 사슬을 줄였다. 방법은 위 항목의 방향 보간 병렬 사본이다.

### 프로세서별 시간은 직렬 트레이스로 가른다

**증상**: 병렬 트레이스에서 어떤 프로세서의 구간이 프레임 대부분에 걸친다. 그래서 그 프로세서가 비싸 보인다.

**설명**
- 병렬 처리 큐에서 프로세서가 쿼리 자원을 기다린 시간은 그 프로세서의 실행 시간에 잡힌다.
- 기다림 자체의 타이머는 없다.
- 게임 스레드에서 기다리면 스핀(`YieldThread` 루프)이다. 그래서 그 시간은 게임 스레드 시간에도 든다.
- `mass.FullyParallel`이 0이면 Mass 프로세서는 게임 스레드에서 차례로 돈다.
- 프로세서 안의 `ParallelFor`는 직렬에서도 작업자로 나뉜다.

**경고**: 직렬 트레이스는 원인을 가르는 수단이다. 병렬에서 기다린 시간도 그 실행의 실제 비용이다.

**절차**
1. `mass.FullyParallel`을 0으로 두고 실행한다.
2. 그 실행에서 트레이스를 한 번 더 뜬다.
3. 직렬 트레이스에서 프로세서별 시간을 읽는다.

`Source/HordeOptLab/LabGameMode.cpp`(실행 인자 `-LabFullyParallel=0`일 때)

```cpp
IConsoleManager::Get().FindConsoleVariable(TEXT("mass.FullyParallel"))->Set(0, ECVF_SetByCode);
```

**결과**
- 엔진 회피를 쓴 단계의 병렬 트레이스 한 프레임에서 엔진 방향 보간의 구간은 0.69ms부터 28.8ms까지 걸쳐 있었다.
- 직렬 트레이스에서 같은 프로세서는 프레임당 0.82ms였다.

### 틱 간격을 늘리기보다 큰 몫의 프로세서를 쿼리에서 뺀다

**증상**: Simulation LOD로 먼 개체의 틱 간격을 늘렸다. 그런데 프레임이 줄지 않는다.

**설명**
- 엔진 가변 틱은 청크 필터 `ShouldTickChunkThisFrame`과 누적 델타를 쓰는 프로세서만 건너뛴다.
- 엔진 이동·조향은 Off를 뺀 개체를 매 프레임 돈다.
- 엔티티가 드나든 청크는 수정 번호가 바뀐다. 그래서 그 청크는 간격과 무관하게 다음 프레임에 돈다.
- 엔진 이동·조향 넷을 한꺼번에 빼는 태그는 `FMassOffLODTag`뿐이다.
- 엔진의 `UMassOffLODNavigationProcessor`는 Off 개체를 이동 목표로 순간 이동시킨다.

**경고**: 가변 틱 필터를 읽는 프로세서는 `ExecuteAfter(ProcessorGroupNames::LOD)`를 적어야 한다. 적지 않으면 그 프로세서는 주기를 놓친다.

**절차**
1. 먼 개체를 Simulation LOD의 Off에 둔다.
2. 직접 짠 프로세서로 Off 개체를 1초마다 옮긴다.
3. 엔진의 `UMassOffLODNavigationProcessor`의 등록을 끈다.

`Source/HordeOptLab/LabMassTraits.cpp`, `ULabSimulationLODTrait::ULabSimulationLODTrait`(줄였다)

```cpp
bEnableVariableTicking = true; // LOD 태그도 함께 붙는다
VariableTickParams.TickRates[EMassLOD::Medium] = FLabSimLod::TickMedium; // 0.5초
VariableTickParams.TickRates[EMassLOD::Low] = FLabSimLod::TickLow;       // 1초
Params.LODDistance[EMassLOD::Off] = FLabSimLod::LowDistance;             // 200m 밖은 Off
VariableTickParams.TickRates[EMassLOD::Off] = FLabSimLod::TickLow;
```

**결과**
- 죽고 다시 생기며 태그가 바뀌는 호드에서 설정한 0.5초 간격의 실제 주기는 0.13초였다.
- 실제 주기는 평균 프레임 시간 × 밴드 개체 수 ÷ 그 프레임에 돈 개체 수로 셈했다.
- 앞선 시험에서는 Off로 엔진 이동을 뺀 변형만 프레임이 줄었다.
- 최종 구성에서 Simulation LOD를 끈 조건은 0.24ms 짧았다. 이 차이는 구별되지 않는다.

### 결정적 해시로 대상을 고를 때는 `HashCombineFast`의 결과를 섞는다

**증상**: 일련번호와 틱으로 해시를 만들어 가장 작은 값의 개체를 고른다. 그런데 그 코드는 늘 작은 일련번호만 뽑는다.

**설명**
- `HashCombineFast(A, B)`는 `A ^ (B + 0x9e3779b9 + (A << 6) + (A >> 2))`다.
- `GetTypeHash(uint32)`는 값을 그대로 돌려준다.
- 그래서 틱은 모든 후보에 같은 수를 더할 뿐 순서를 바꾸지 못한다.

**절차**
1. 입력과 결과를 `MurmurFinalize32`로 섞는다.

`Source/HordeOptLab/LabSnipe.h`, `LabSnipeHash`

```cpp
inline uint32 LabSnipeHash(uint32 Serial, uint32 SnipeTick)
{
	return MurmurFinalize32(HashCombineFast(MurmurFinalize32(Serial), SnipeTick));
}
```

**결과**
- 같은 식을 Python으로 흉내 내 후보 2,000개, 틱 200개로 돌렸다.
- 섞기 전의 흉내는 200번 모두 가장 작은 일련번호를 뽑았다.
- 섞은 뒤의 흉내는 200번에 서로 다른 대상 189개를 뽑았다.
- 뽑힌 순위의 평균은 1,043이다. 고르게 뽑으면 1,000이다.

## 프로파일링할 때

아래는 대규모 군중의 프레임을 읽을 때 이 프로젝트가 따른 순서다. 위에서부터 본다. 앞에서 답이 나오면 다음으로 내려간다.

1. [프레임 p95와 세 스레드 시간을 함께 보고 병목을 정한다](#프레임-p95와-세-스레드-시간을-함께-보고-병목을-정한다).
2. [병목이 아닌 스레드가 준 것은 프레임 이득으로 세지 않는다](#병목이-아닌-스레드가-준-것은-프레임-이득으로-세지-않는다).
3. [게임 스레드 시간의 차이만으로 비용을 말하지 않는다](#게임-스레드-시간의-차이만으로-비용을-말하지-않는다).
4. [프레임이 세 스레드보다 길면 게임 스레드가 기다린 작업자를 찾는다](#프레임이-세-스레드보다-길면-게임-스레드가-기다린-작업자를-찾는다).
5. 트레이스를 연다. [액터 군중은 불린 횟수와 한 번의 시간으로 나눠 본다](#액터-군중은-불린-횟수와-한-번의-시간으로-나눠-본다). [Insights의 Timers 합계는 스레드를 골라 읽고 불린 횟수로 나눈다](#insights의-timers-합계는-스레드를-골라-읽고-불린-횟수로-나눈다).
6. Mass 프로세서로 내려간다. [프로세서별 시간은 직렬 트레이스로 가른다](#프로세서별-시간은-직렬-트레이스로-가른다). [엔진 프로세서가 한 스레드로 도는지 먼저 본다](#엔진-프로세서가-한-스레드로-도는지-먼저-본다).
7. [기법 하나만 끄고 재서 기여를 가른다](#기법-하나만-끄고-재서-기여를-가른다).
8. [빨라진 조건이 같은 일을 하는지 본다](#빨라진-조건이-같은-일을-하는지-본다).

### 프레임 p95와 세 스레드 시간을 함께 보고 병목을 정한다

**증상**: 기법 하나를 넣고 어느 값이 움직였는지 본다. 첫 질문은 어느 스레드가 프레임을 정하는가다.

**설명**
- 게임 스레드, 렌더 스레드, GPU는 서로 다른 프레임을 겹쳐 처리한다.
- 앞서 나갈 수 있는 프레임 수에는 한도가 있다.
- 그래서 프레임 시간은 셋 가운데 가장 느린 쪽에 맞춰진다. 이 쪽이 병목이다.
- 엔진은 게임 스레드 시간에서 기다린 시간을 뺀다. 그래서 프레임 시간은 병목보다 조금 길다.
- 세 스레드 값의 원천(`GGameThreadTime`, `GRenderThreadTime`, `RHIGetGPUFrameCycles()`)은 `STATS` 없이 쓰인다.
- 그래서 Test 구성의 HUD에서도 같은 값을 읽을 수 있다.

**절차**
1. CSV 프로파일러의 `FrameTime`, `GameThreadTime`, `RenderThreadTime`, `GPUTime`을 한 표에 둔다.
2. 판정은 프레임 p95로 한다.
3. 스레드 시간은 기다림이 섞이므로 중앙값만 쓴다.
4. 세 스레드 시간 가운데 가장 긴 쪽을 병목으로 정한다.

`Engine/Source/Runtime/Engine/Private/UnrealClient.cpp`, `FViewport::Draw`

```cpp
GGameThreadTime = (ThreadTime > GameThread.Waits) ? (ThreadTime - GameThread.Waits) : ThreadTime;
```

**결과**
- 최종 구성의 프레임 p95는 14.2ms다.
- 최종 구성의 게임 스레드 시간은 11.9ms다.
- 최종 구성의 렌더 스레드 시간은 4.12ms다.
- 최종 구성의 GPU 시간은 6.16ms다.
- 그래서 최종 구성의 병목은 게임 스레드다.

### 병목이 아닌 스레드가 준 것은 프레임 이득으로 세지 않는다

**증상**: GPU 시간이 크게 줄었다. 그런데 프레임 시간은 그대로다.

**설명**
- 병목이 게임 스레드면 GPU와 렌더 스레드는 남는 시간을 기다리며 보낸다.
- 그래서 그 쪽을 줄여도 프레임은 줄지 않는다.
- 줄인 몫은 사라지지 않는다. 그 몫은 병목이 바뀔 때까지의 여유다.
- 게임 스레드가 GPU 아래로 내려가면 그때부터 GPU가 프레임을 정한다. 하이드레이션을 끈 조건이 그 예다.

**절차**
1. 두 조건의 병목 스레드를 먼저 정한다.
2. 결과에 GPU 시간을 프레임 시간과 함께 적는다.
3. 프레임이 줄지 않았으면 "프레임에는 드러나지 않았다"로 쓴다.

**결과**
- 35\~200m의 VAT 인스턴스를 ISKM으로 바꾼 단계에서 GPU 시간은 1.56ms 줄었다.
- 같은 단계에서 렌더 스레드 시간은 1.51ms 줄었다.
- 그러나 그 단계의 프레임 p95는 0.19ms 늘어 구별되지 않았다.
- 최종 구성에서 80m 밖 그림자를 다시 켠 조건도 GPU 시간만 0.48ms 길었다.

### 게임 스레드 시간의 차이만으로 비용을 말하지 않는다

**증상**: 게임 스레드 시간이 늘었는데 프레임은 같다. 또는 프레임이 늘었는데 게임 스레드 시간은 그대로다.

**설명**
- 게임 스레드 시간은 기다림을 빼기도 하고 넣기도 한다.
- 엔진은 작업자의 태스크를 기다린 이벤트 대기를 게임 스레드 시간에서 뺀다.
- 병렬 Mass에서 쿼리 자원을 기다린 시간은 기다린 프로세서의 실행 시간이 된다.
- 게임 스레드에서 기다리면 스핀이다. 그래서 그 시간은 게임 스레드 시간에 든다.
- 그래서 기다림이 찍히는 자리가 옮겨 가면 일이 같아도 게임 스레드 시간이 바뀐다.

**절차**
1. 게임 스레드 시간의 차이를 프레임 시간과 맞춰 본다.
2. 병렬 트레이스에서 기다림이 어느 프로세서의 시간에 찍혔는지 본다.
3. 두 확인이 끝난 뒤에 스레드 시간의 차이를 비용이라고 쓴다.

**결과**
- 위치 쓰기 선언 때문에 게임 스레드의 가벼운 프로세서가 작업자의 밀어내기 프로세서를 기다렸다.
- 그 기다린 시간은 그 가벼운 프로세서의 비용으로 보였다.
- 탑뷰 카메라로 잰 앞선 시험에서는 게임 스레드 시간만 늘고 프레임이 같았다.
- 그 시험에서는 작업 대기가 표현 LOD 프로세서 안으로 옮겨 가 있었다.

### 프레임이 세 스레드보다 길면 게임 스레드가 기다린 작업자를 찾는다

**증상**: 프레임 시간이 게임 스레드, 렌더 스레드, GPU 어느 것보다 길다.

**설명**
- 모자란 몫은 게임 스레드가 작업자 스레드를 기다린 시간이다.
- Test 구성은 이 기다림을 유휴 시간으로 센다. 그래서 이 기다림은 게임 스레드 시간에서 빠진다.
- `GameThreadTime_CriticalPath`가 프레임 시간과 같으면 그 기다림은 임계 경로에 있다.

**절차**
1. 프레임 CSV에서 게임 스레드의 `EventWait/*` 열을 더한다.
2. 게임 스레드 시간에 그 합을 더해 프레임 시간과 맞춰 본다.
3. `GameThreadTime_CriticalPath`를 프레임 시간과 견준다.
4. 트레이스에서 게임 스레드의 `WaitForTasks`가 끝나는 순간을 찾는다.
5. 그 순간에 끝나는 작업자 이벤트를 찾는다.

**결과**
- 엔진 회피를 그대로 쓴 단계의 값은 다음과 같았다.
  - 프레임 p50: 34.1ms
  - 게임 스레드 시간: 18.0ms
  - GPU 시간: 12.2ms
  - 렌더 스레드 시간: 7.29ms
- 그 단계의 게임 스레드는 프레임마다 16.2ms를 기다렸다.
- 그중 14.5ms는 `EventWait/PrePhysics`였다. 이 틱 그룹에서 Mass 처리 페이즈가 돈다.
- 게임 스레드 시간에 기다림을 더하면 프레임 시간과 0.03ms 안에서 맞았다.
- 게임 스레드가 기다린 작업자 이벤트는 작업자 하나의 엔진 회피였다. 그 회피는 한 프레임에서 25.6ms였다.

### 액터 군중은 불린 횟수와 한 번의 시간으로 나눠 본다

**증상**: 액터가 많은 장면의 게임 스레드 트레이스에서 한두 컴포넌트의 틱이 대부분을 차지한다.

**설명**
- 액터 군중의 비용은 개체마다 컴포넌트가 차례로 틱하는 일의 합이다.
- 그래서 타이머의 합계는 한 번의 비용 × 불린 횟수다.
- 프레임이 50ms를 넘으면 CharacterMovement는 시간을 50ms씩 최대 여덟 번의 하위 스텝으로 나눠 계산한다.
- 엔진은 월드 델타를 0.4초(`MaxUndilatedFrameTime`)로 자른다.

**절차**
1. 트레이스에서 큰 타이머의 합계와 불린 횟수를 읽는다.
2. 불린 횟수가 개체 수를 따르는지 본다.
3. 한 번의 비용이 개체 수에 따라 변하는지 따로 본다.

**결과**
- 모든 개체가 `ACharacter`인 호드 총수 1만의 트레이스를 읽었다.
- `UCharacterMovementComponent_TickComponent`는 프레임마다 9,983번 불렸다.
- 그 타이머는 게임 스레드 프레임의 76.4%(411ms)를 썼다.
- 한 번의 시간은 약 41µs(411ms ÷ 9,983)다.
- 개체당 게임 스레드 시간은 호드 총수 2천까지 18µs 안팎이었다.
- 개체당 게임 스레드 시간은 호드 총수 1만에서 58.1µs였다.
- 이 증가는 하위 스텝과 같은 쪽이다. 그러나 하위 스텝을 끈 실행으로 가르지는 않았다.

### Insights의 Timers 합계는 스레드를 골라 읽고 불린 횟수로 나눈다

**증상**: Unreal Insights의 Timers 목록에서 고른 구간의 합계가 프레임 수 × 프레임 시간보다 크다.

**설명**
- Timers 목록은 모든 스레드의 같은 이름 타이머를 합쳐 보인다.
- 명령줄 내보내기를 `-threads=GameThread`로 걸러도 GPU 타이머와 `Frame` 행이 섞여 나온다.

**주의**: 소스 빌드에는 `UnrealInsights.exe`가 없다.

**절차**
1. `UnrealInsights.exe`를 한 번 빌드한다(`Build.bat UnrealInsights Win64 Development`).
2. Insights의 명령줄 내보내기로 스레드별 값을 얻는다.
3. 내보낸 값에서 GPU 타이머와 `Frame` 행을 버린다.
4. 프레임당 값은 구간 합을 구간 안 `FEngineLoop::Tick`의 Count로 나눠 셈한다.
5. 한 번의 시간은 구간 합을 그 타이머의 Count로 나눠 셈한다.

`Scripts/trace-export.ps1`이 만드는 명령줄(줄였다)

```text
UnrealInsights.exe -OpenTraceFile=<trace.utrace> -NoUI -AutoQuit
  -ExecOnAnalysisCompleteCmd="TimingInsights.ExportTimerStatistics <out.csv> -threads=GameThread -startTime=70 -endTime=120 -columns=*"
```

**결과**
- 엔진 회피를 쓴 단계의 50초 구간에서 `WaitForTasks`는 목록에 60,369번, 1분 42.7초로 나왔다.
- 같은 구간의 게임 스레드만의 값은 22.96초(프레임당 15.1ms)였다.
- 직렬 트레이스의 엔진 회피는 28.92초 ÷ 1,232번 = 23.5ms였다.

### 기법 하나만 끄고 재서 기여를 가른다

**증상**: 단계마다 기법을 더하며 잰다. 그런데 단계 사이의 차이가 작아 구별되지 않는다.

**설명**
- 사다리의 차이는 기법을 더한 순서에 묶인다.
- 한 단계에 변수가 둘 이상이면 몫을 나눌 수 없다.

**경고**: 하나를 끄면 다른 것이 함께 빠지는 조건이 있다. 하이드레이션을 끄면 캡슐과 Animation Budget Allocator도 빠진다.

**절차**
1. 최종 구성에서 기법 하나만 끈 조건을 같은 빌드, 같은 장면에서 잰다.
2. 최종 구성과 그 조건의 차이를 본다.
3. 다른 것이 함께 빠지는 조건은 참고로 적는다.

**결과**
- 먼 개체의 표현을 더한 세 단계는 하나씩 +0.06, +0.19, +0.46ms였다. 셋 모두 구별되지 않았다.
- 그 사이의 단계까지 모아 견주면 +1.02ms로 구별됐다.
- 최종 구성에서 열일곱 조건 가운데 일곱이 구별됐다.
- 다음 셋은 시간을 줄이는 쪽이었다.
  - 200m 밖을 그리지 않는 일: +3.54ms
  - 액터 메시 고정 바운드: +0.98ms
  - ISKM 고정 바운드: +0.66ms

### 빨라진 조건이 같은 일을 하는지 본다

**증상**: 어떤 설정을 바꾸자 프레임이 짧아졌다. 그 차이는 비용이 줄어서가 아니라 일이 줄어서 생겼을 수 있다.

**설명**
- 프레임이 아주 길면 엔진도 일을 덜 한다.
- 엔진은 월드 델타를 0.4초로 자른다.
- 엔진은 Mass 힘·이동 프로세서의 델타를 0.1초로 자른다.
- 그래서 개체가 실제보다 덜 움직인다.
- 겹친 쌍은 몸통이 겹친 개체 쌍의 수다.

**절차**
1. 프레임 시간과 함께 일의 양을 세는 값(초당 사망, 목표 둘레의 개체 수, 겹친 쌍)을 남긴다.
2. 그 값을 조건끼리 견준다.
3. 일이 다르면 그 조건의 프레임 시간을 비용 비교에 쓰지 않는다.
4. 일이 다른 조건은 값만 보고한다.

**결과**
- 최종 구성에서 겹침 방지를 끈 조건은 프레임 p95가 0.93ms 짧다.
- 그러나 그 조건의 겹친 쌍은 7배였다.
- 모든 개체가 액터인 호드 총수 1만은 프레임이 0.4초를 넘었다.
- 그래서 그 조건의 초당 사망은 다른 단계보다 45% 적었다.
