# 액터 전환을 만들 때

하이드레이션 액터(가까운 개체에만 붙이는 진짜 액터)를 다루는 일곱 항목이다.

- 항목은 증상, 설명, 경고·주의, 절차, 코드, 결과의 순서다. **경고**는 어기면 결과가 틀려지는 것이다. **주의**는 시간을 버리거나 부작용이 있는 것이다.
- 결과의 수치는 `results.md`의 조건(UE 5.8.4, Ryzen 9 5950X와 RTX 4090, Test 구성 패키지, 1만 개체, 프레임 p95, 세 번 잰 중앙값)에서 잰 참고값이다. "끈 조건"은 최종 구성에서 그 기법만 끈 것이다.
- 코드의 게임 클래스 이름은 예시 이름이다. 엔진 소스 위치는 UE 5.8.4 기준이다. 줄임말 **MG** = `Engine/Plugins/Runtime/MassGameplay/Source`.

## 목차

1. 하이드레이션 액터의 몫부터 잰다
2. 액터 경계에는 최소 유지 시간을 둔다
3. 풀에 넣는 액터는 틱과 중요도와 콜리전을 끈다
4. 액터 메시에는 URO와 Budget Allocator 가운데 하나를 고른다
5. 액터 캡슐은 QueryOnly와 전용 트레이스 채널로 켠다
6. 액터 메시에 고정 바운드를 쓴다
7. 캡슐을 겨누는 라인 트레이스는 시작점을 정면 평면에서 비킨다

## 1. 하이드레이션 액터의 몫부터 잰다

**증상**: Mass로 옮긴 뒤에도 게임 스레드가 병목이다. 무엇이 큰지 모를 때 이 항목을 먼저 한다.

**설명**
- 하이드레이션 액터는 액터마다 고정비를 얹는다. 메시 틱과 애니메이션, 캡슐 바디의 이동, 엔티티 위치를 액터에 넣는 동기화다. 이 일은 액터마다 매 프레임 돈다.
- 표현 LOD의 액터 상한(`LODMaxCount`)이 걸리면 엔진은 액터 거리를 당긴다. 그때는 전환 거리보다 상한이 이 비용을 정한다.
- 상한이 걸렸는지는 공유 프래그먼트 `FMassVisualizationLODSharedFragment::bHasAdjustedDistancesFromCount`로 본다(`MG/MassRepresentation/Public/MassRepresentationFragments.h`).

**경고**: 하이드레이션을 끈 조건에서는 캡슐, Budget Allocator, 액터 메시 고정 바운드도 함께 빠진다. 이 조건의 값은 "참고"다.

**절차**
1. 같은 빌드에서 하이드레이션만 끈 조건을 실행 인자로 고를 수 있게 만든다.
2. 그 조건을 최종 구성과 같은 묶음에서 잰다.
3. 액터 평균 수와 상한이 걸린 프레임 비율을 실행 기록에 남긴다.

**결과**
- 참고 장면 최종 구성의 액터는 평균 578개였다.
- 하이드레이션을 끈 조건은 프레임 p95가 6.98ms 짧았다. 게임 스레드 시간은 11.9ms에서 5.23ms로 줄고 병목이 GPU(6.21ms)로 바뀌었다.
- 캡슐만 끈 조건도 1.86ms 짧았다.
- 가까운 개체의 캡슐이 기능에 필요해 액터를 남겼다. 대신 액터 하나의 비용을 5, 6, 4항목으로 줄였다.

## 2. 액터 경계에는 최소 유지 시간을 둔다

**증상**: 액터 경계 근처의 개체가 액터와 인스턴스 사이를 짧게 오간다. 전환 순간의 튐을 되풀이한다.

**설명**
- 엔진 표현 LOD의 거리 히스테리시스(기본 10%, `BufferHysteresisOnDistancePercentage`)는 한 단계 가까운 구간으로 들어올 때만 걸린다.
- 계산 반복이 High(0번)에서 멈춘다. 그래서 액터 경계에는 히스테리시스가 없다. 상한이 당긴 경계도 같은 함수를 쓴다(`MG/MassLOD/Public/MassLODCalculator.h` `ComputeLODFromSettings`).
- 화면 안팎 판정의 히스테리시스(`DistanceToFrustumHysteresis`)는 기본값이 0이다.
- 엔진에는 최소 유지 시간에 해당하는 설정이 없다. 표현 프로세서는 원하는 표현이 다르면 바로 바꾼다(`MG/MassRepresentation/Private/MassRepresentationProcessor.cpp`).

**절차**
1. 프로세서를 `Representation` 그룹에 두고 `LOD` 그룹 뒤에 둔다. 이번 프레임의 LOD가 정해진 뒤다.
2. 같은 프로세서를 `VisualizationProcessing` 그룹 앞에 둔다. 표현이 바뀌기 전이다.
3. 그 프로세서에서 2초 안에 바로 전 표현으로 돌아가려는 개체의 LOD를 되돌린다.

```cpp
// 예시 이름: URepHoldProcessor의 생성자
ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Representation;
ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::LOD);
ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::VisualizationProcessing);
```

**결과**
- 왕복 전환은 2초 안에 같은 경계를 반대로 되넘은 수다.
- 이 규칙만 끈 조건은 왕복 전환이 초당 4.34건이었다. 모두 액터 경계에서 났다.
- 이 규칙을 켜면 왕복 전환은 0건이었다. 프레임 p95는 구별되지 않았다.

## 3. 풀에 넣는 액터는 틱과 중요도와 콜리전을 끈다

**증상**: Mass 액터 풀로 돌려보낸 액터가 숨은 채 틱한다. 또는 마지막 자리에 남은 캡슐이 트레이스에 맞는다.

**설명**
- `UMassActorSpawnerSubsystem`의 풀은 액터를 숨기기만 한다. 컴포넌트 등록, 틱, 콜리전은 그대로 남는다.
- `USkeletalMeshComponentBudgeted`는 Budget Allocator에 등록된 채 남는다.
- 엔진은 풀에서 꺼낸 액터의 숨김을 풀고 엔티티 변환(스케일 1)으로 옮긴 뒤 `PrepareForGame`을 부른다. `BeginPlay`는 다시 부르지 않는다(`MG/MassActors/Private/MassActorSpawnerSubsystem.cpp`).

**절차**
1. `IMassActorPoolableInterface`의 `PrepareForPooling`에서 메시 틱, 중요도, 액터 콜리전을 끈다.
2. `PrepareForGame`에서 셋을 되돌린다.
3. `PrepareForGame`에서 액터 스케일을 다시 넣는다.

```cpp
// 예시 이름: ACrowdMemberActor::PrepareForPooling_Implementation
GetMesh()->SetComponentTickEnabled(false);
SetActorEnableCollision(false); // 엔진 풀은 콜리전을 그대로 둔다
CastChecked<USkeletalMeshComponentBudgeted>(GetMesh())->SetComponentSignificance(0.f);
```

**결과**: 이 처리만 끈 조건은 재지 않았다. 숨은 액터의 틱과 트레이스 적중으로 확인한다.

## 4. 액터 메시에는 URO와 Budget Allocator 가운데 하나를 고른다

**증상**: Budget Allocator를 켠 액터에 URO(Update Rate Optimization)도 걸려 있다고 본다. 그 가정으로 결과를 읽으면 틀린다.

**설명**
- Budget Allocator는 켜져 있으면 등록하는 메시의 `bEnableUpdateRateOptimizations`를 끄고 갱신을 직접 맡는다. 꺼지면 URO를 다시 켠다.
- Budget Allocator는 CVar `a.Budget.Enabled`(기본 0)와 월드의 `SetEnabled`가 모두 참일 때만 켜진다. 엔진은 월드마다 만들고 `SetEnabled`의 처음 값은 거짓이다.
- 등록은 `USkeletalMeshComponentBudgeted`만 받는다. ISKM은 예산 밖이다(`Engine/Plugins/Runtime/AnimationBudgetAllocator/Source/AnimationBudgetAllocator/Public/IAnimationBudgetAllocator.h`).
- 예산 기본값은 1ms다(`AnimationBudgetAllocatorParameters.h` `BudgetInMs`).
- 수요가 예산 안이면 모든 메시를 매 프레임 갱신한다. 그때는 일을 줄이지 않는다. 초과분이 0보다 클 때만 매 프레임 갱신하는 몫을 줄인다(`AnimationBudgetAllocator.cpp`).
- 중요도는 큰 값이 중요하다. Mass의 `LODSignificance`(0이 가장 높음, 3이 Off)를 넘길 때는 `1 - LODSignificance / 3`처럼 바꾼다.
- 스켈레탈 메시의 URO 기본값은 끔이다.

**경고**: CVar만 넣으면 Budget Allocator는 켜지지 않는다.

**절차**
1. 액터 메시의 클래스를 `USkeletalMeshComponentBudgeted`로 바꾼다.
2. CVar `a.Budget.Enabled`와 `a.Budget.BudgetMs`를 넣는다.
3. 같은 자리에서 월드의 Budget Allocator에 `SetEnabled(true)`를 부른다.
4. Budget Allocator가 실제로 켜졌는지 실행 기록에 남긴다.

```cpp
// 게임 모드나 월드 서브시스템의 시작 자리
IConsoleManager::Get().FindConsoleVariable(TEXT("a.Budget.Enabled"))->Set(1, ECVF_SetByCode);
IConsoleManager::Get().FindConsoleVariable(TEXT("a.Budget.BudgetMs"))->Set(4.f, ECVF_SetByCode);
IAnimationBudgetAllocator::Get(GetWorld())->SetEnabled(true); // CVar만으로는 켜지지 않는다
```

**결과**
- 모든 개체가 액터인 군중에 예산 4ms를 걸었다.
- 2천(애니메이션 4.17ms)에서는 프레임 p95가 41.3ms에서 36.9ms로 줄었다.
- 1천(애니메이션 3.71ms)에서는 0.52ms 길었다. 예산을 넘는 수요가 있을 때만 줄인다.

## 5. 액터 캡슐은 QueryOnly와 전용 트레이스 채널로 켠다

**증상**: 가까운 개체에 판정용 캡슐이 필요하다. 그 캡슐이 액터 비용의 큰 몫이 된다.

**설명**
- `ACharacter`의 캡슐은 `Pawn` 프로필(QueryAndPhysics)을 쓰고 오버랩과 물리 볼륨 갱신이 켜져 있다.
- 피직스 상태가 있는 캡슐은 옮길 때마다 바디 변환을 피직스 씬에 보낸다(`SendPhysicsTransform` → `SetGlobalPose`).
- QueryOnly 캡슐도 바디가 있어 이 일을 한다. `NoCollision` 캡슐만 바디를 만들지 않는다.
- 군중끼리 겹치지 않게 하는 일은 Mass 쪽(밀어내기 프로세서)에서 한다. 캡슐은 판정 트레이스에만 쓴다.

**절차**
1. `Config/DefaultEngine.ini`에 기본 응답이 무시인 전용 트레이스 채널을 만든다(`ECC_GameTraceChannel1`, `DefaultResponse=ECR_Ignore`).
2. 캡슐의 오버랩과 물리 볼륨 갱신을 끈다.
3. 캡슐의 모든 채널 응답을 무시로 두고 전용 트레이스 채널만 막음으로 둔다.
4. 캡슐을 QueryOnly로 바꾼다.

```cpp
// 예시 이름: ACrowdMemberActor::BeginPlay
Capsule->SetShouldUpdatePhysicsVolume(false);
Capsule->SetGenerateOverlapEvents(false);
FCollisionResponseContainer Responses(ECR_Ignore);
Responses.SetResponse(ECC_GameTraceChannel1, ECR_Block); // 전용 트레이스 채널만 막는다
Capsule->SetCollisionResponseToChannels(Responses);
Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
```

**결과**
- 이 설정을 해도 캡슐만 끈 조건은 프레임 p95가 1.86ms 짧았다. 남은 비용은 바디 자체다.
- CVar `p.Chaos.AccelerationStructureIsolateQueryOnlyObjects`는 QueryOnly 바디를 가속 구조의 따로 된 칸에 넣는다. 이 CVar는 재지 않은 후보다.

## 6. 액터 메시에 고정 바운드를 쓴다

**증상**: 액터 수가 늘면 Budget Allocator가 재는 액터 메시의 애니메이션 시간이 는다.

**설명**
- 엔진은 애니메이션을 갱신할 때마다 캐시한 바운드를 지우고 다시 셈한다(`FinalizeAnimationUpdate`).
- 기본값에서는 피직스 에셋의 바디마다 뼈 변환으로 상자를 셈한다.
- `bComponentUseFixedSkelBounds`를 켜면 메시 에셋의 바운드에 루트 뼈의 이동만 더한다.

**주의**: 메시 에셋의 바운드가 피직스 에셋으로 셈한 바운드보다 작으면 화면 컬링과 메시 LOD 고르기가 달라진다. 두 바운드를 먼저 견준다.

**절차**
1. 하이드레이션 액터 메시의 `bComponentUseFixedSkelBounds`를 켠다.
2. `BoundsScale`을 1.1로 둔다.

```cpp
// 예시 이름: ACrowdMemberActor::BeginPlay
SkelMesh->bComponentUseFixedSkelBounds = true;
SkelMesh->SetBoundsScale(1.1f);
```

**결과**
- 액터 메시 고정 바운드만 끈 조건은 프레임 p95가 0.98ms 길었다.
- 그 조건에서 액터 메시의 애니메이션 시간은 2.71ms에서 3.63ms로 늘었다.

## 7. 캡슐을 겨누는 라인 트레이스는 시작점을 정면 평면에서 비킨다

**증상**: 캡슐 중심을 겨눈 라인 트레이스가 일정 비율로 캡슐을 놓친다.

**설명**
- 대상이 시작점을 바라볼 때 트레이스가 캡슐을 놓쳤다. 그때 시작점은 캡슐의 정면 평면(정면 축과 연직축이 이루는 평면) 위에 있다.
- 빗나간 순간에도 같은 자리의 작은 구 겹침 질의는 캡슐을 찾았다. 브로드페이즈를 거치지 않는 `LineTraceComponent`도 놓쳤다.
- 캡슐 레이캐스트(`Chaos::Raycasts::RayCapsule`) 안의 어느 분기가 원인인지는 원인 미확정이다.

**절차**
1. 시작점이나 끝점을 정면 평면에서 몇 cm 비킨다.

**결과**
- 목표가 서 있는 장면에서 트레이스는 3,780회 중 63회(1.67%) 캡슐을 놓쳤다.
- Development 빌드 진단에서 시작점을 옆으로 1.7cm 비키자 빗나감이 0회였다.
