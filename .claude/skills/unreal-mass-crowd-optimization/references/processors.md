# Mass 프로세서를 짤 때

Mass 프로세서의 비용과 순서를 다루는 일곱 항목이다. Mass 고유의 프로파일링(직렬 트레이스, 한 스레드로 도는 엔진 프로세서 찾기)도 여기에 있다. 일반 프로파일링(병목, 기다림, Insights 내보내기)은 `unreal-perf-measurement`에 있다.

- 항목은 증상, 설명, 경고·주의, 절차, 코드, 결과의 순서다. **경고**는 어기면 결과가 틀려지는 것이다. **주의**는 시간을 버리거나 부작용이 있는 것이다.
- 결과의 수치는 `results.md`의 조건(UE 5.8.4, Ryzen 9 5950X와 RTX 4090, Test 구성 패키지, 1만 개체, 프레임 p95, 세 번 잰 중앙값)에서 잰 참고값이다. 트레이스 값은 같은 장면의 트레이스 실행에서 읽은 것이다.
- 코드의 게임 클래스 이름은 예시 이름이다. 엔진 소스 위치는 UE 5.8.4 기준이다. 줄임말: **ME** = `Engine/Source/Runtime/MassEntity/Private`, **MN** = `Engine/Plugins/AI/MassAI/Source/MassNavigation`, **MG** = `Engine/Plugins/Runtime/MassGameplay/Source`.

## 목차

1. 엔진 프로세서가 한 스레드로 도는지 먼저 본다
2. 개체마다 독립인 엔진 프로세서는 청크 병렬 사본으로 바꾼다
3. 게임 스레드 프로세서의 주 쿼리에 위치 쓰기를 선언하지 않는다
4. 프로세서 단위 서브시스템 선언이 다른 프로세서를 막는지 본다
5. 프로세서별 시간은 직렬 트레이스로 가른다
6. 틱 간격을 늘리기보다 큰 몫의 프로세서를 쿼리에서 뺀다
7. 결정적 해시로 대상을 고를 때는 `HashCombineFast`의 결과를 섞는다

## 프로세서 비용을 읽는 순서

1. 병목이 게임 스레드인지 확인한다(`unreal-perf-measurement`).
2. 프레임이 세 스레드보다 길면 게임 스레드가 작업자를 기다린 것이다. 트레이스에서 그 작업자를 찾는다.
3. 직렬 트레이스(5항목)로 프로세서별 시간을 읽는다.
4. 큰 몫의 엔진 프로세서가 `ForEachEntityChunk`인지 소스에서 본다(1항목).
5. 병렬 트레이스에서 기다림의 원인을 찾는다. 쿼리 충돌(3항목), 서브시스템 선언 충돌(4항목)이다.

## 1. 엔진 프로세서가 한 스레드로 도는지 먼저 본다

**증상**: 트레이스에서 작업자 스레드 하나가 프레임 대부분 동안 한 프로세서를 돈다. 게임 스레드는 그동안 기다린다.

**설명**
- 엔진 이동 회피(`UMassMovingAvoidanceProcessor`)는 `ForEachEntityChunk`로 청크를 한 스레드에서 차례로 돈다. 서 있는 회피(`UMassStandingAvoidanceProcessor`)도 같다(`MN/Private/MassAvoidanceProcessors.cpp`).
- 이동 회피 쿼리는 Medium·Low·Off LOD 태그가 있는 개체를 뺀다(`MassAvoidanceProcessors.cpp:301-303`). Simulation LOD를 켜지 않으면 이 태그가 없어 전 개체가 순회에 든다.
- 이웃 찾기는 장애물 격자(한 칸 2.5m)에서 탐지 거리 400cm 상자의 칸을 훑고 이웃을 최대 24개까지 모은다.
- 장애물 격자 갱신(`UMassNavigationObstacleGridProcessor`)도 격자의 `Move`가 스레드 안전하지 않아 병렬 순회를 하지 않는다(`MN/Private/MassNavigationProcessors.cpp`).
- 엔진 이동(`UMassApplyMovementProcessor`)과 방향 보간(`UMassSmoothOrientationProcessor`)도 병렬 순회가 아니다.

**절차**
1. 엔진 프로세서를 쓰기 전에 소스에서 `ForEachEntityChunk`인지 `ParallelForEachEntityChunk`인지 본다.
2. 엔진 회피 대신 위치를 직접 미는 밀어내기 프로세서를 짠다. 겹친 두 개체의 위치를 서로 반대로 민다.
3. 밀어내기 프로세서의 이웃 계산을 128개씩 묶어 `ParallelFor`로 나눈다.
4. 모든 개체가 같은 위치 사본(`Pos`)을 읽고 새 배열(`Next`)에 쓰게 한다. 그래서 결과가 스레드 수와 무관하다.
5. 프로세서의 자리는 엔진 이동(`UMassApplyMovementProcessor`) 뒤로 둔다. 그 클래스는 `MinimalAPI`라 `StaticClass()->GetFName()`으로 순서를 적을 수 있다.

```cpp
// 예시 이름: FSeparationSolver::Solve
ParallelFor(NumBatches, [this, &Params, Num](int32 Batch)
{
	for (int32 I = Batch * BatchSize; I < FMath::Min(Num, (Batch + 1) * BatchSize); ++I)
	{ const FVector2f Delta = Resolve(I, Params); Next[I] = Pos[I] + Delta; } // 모두 Pos를 읽고 Next에 쓴다
}, Flags);
```

**결과**
- 엔진 회피를 그대로 쓴 구성에서 회피는 작업자 하나에서 프레임마다 24.7ms를 돌았다.
- 직렬 트레이스에서 그 회피는 Mass 프로세서 시간의 80.7%(23.5ms)였다.
- 최종 구성에서 밀어내기 프로세서를 엔진 회피로 되돌린 조건은 프레임 p95가 8.57ms 길었다.

## 2. 개체마다 독립인 엔진 프로세서는 청크 병렬 사본으로 바꾼다

**증상**: 한 스레드로 도는 엔진 프로세서가 위치를 쓰는 사슬 앞에 있다. 그 프로세서가 뒤 프로세서와 게임 스레드를 늦춘다.

**설명**
- 엔진 방향 보간의 두 쿼리(HighRes, LowRes)는 `ForEachEntityChunk`로 돈다(`MN/Private/MassSmoothOrientationProcessors.cpp:28-41,117-134`).
- 방향 보간은 개체마다 이동 목표와 바라는 속도를 읽고 자기 회전과 `DesiredFacing`만 쓴다. 위치를 읽지도 쓰지도 않는다.
- 쿼리가 `private`이고 클래스가 `MinimalAPI`라 상속으로 순회만 바꿀 수 없다(`MN/Public/SmoothOrientation/MassSmoothOrientationProcessors.h:14-29`).
- 처리 큐는 `ParallelForEachEntityChunk`의 청크를 하나씩 `UE::Tasks`로 띄운다. 그래서 게임 스레드 프로세서여도 청크는 작업자로 간다(`ME/MassEntityQuery.cpp:530-647`).
- 수학 함수(`GetYawFromDirection`, `LerpAngle`, `ExponentialSmoothingAngle`)는 헤더 인라인이다(`MN/Public/MassNavigationUtils.h`). 그래서 게임 모듈로 옮긴 사본이 같은 결과를 낸다.

**주의**: `UMassProcessor::SetShouldAutoRegisterWithGlobalList`는 에디터 빌드에서 그 값을 `DefaultMass.ini`에 써 버린다. 자동 등록은 CDO의 `bAutoRegisterWithProcessingPhases`를 리플렉션으로 바꿔 끈다.

**절차**
1. 같은 쿼리와 식을 가진 사본을 게임 모듈에 둔다.
2. 사본의 두 쿼리를 `ParallelForEachEntityChunk`로 돌린다.
3. 게임 모듈 시작 때 엔진 프로세서 CDO의 `bAutoRegisterWithProcessingPhases`를 리플렉션으로 거짓으로 둔다.
4. 사본과 엔진의 결과가 같은지 Development 빌드에서 한 번 견준다.

```cpp
// 예시 이름: UParallelSmoothOrientationProcessor::Execute(엔진은 같은 자리가 ForEachEntityChunk다)
HighResEntityQuery.ParallelForEachEntityChunk(Context, HighRes);
LowResEntityQuery_Conditional.ParallelForEachEntityChunk(Context, LowRes);
```

```cpp
// 게임 모듈 StartupModule: 엔진 프로세서의 자동 등록 끄기.
// Mass가 월드를 시작하며 프로세서 목록을 만들기 전에 바꿔야 해서 모듈 시작 때 한다.
// SetShouldAutoRegisterWithGlobalList(MassProcessor.h:180-185)는 에디터에서 ini에 쓰므로 CDO 값만 바꾼다.
UMassProcessor* CDO = GetMutableDefault<UMassSmoothOrientationProcessor>();
const FBoolProperty* Prop = CastField<FBoolProperty>(UMassProcessor::StaticClass()->FindPropertyByName(TEXT("bAutoRegisterWithProcessingPhases")));
check(Prop);
Prop->SetPropertyValue_InContainer(CDO, false);
```

**결과**
- 사본의 결과는 엔진과 비트 단위로 같았다.
- 직렬 트레이스에서 방향 보간은 프레임당 0.82ms에서 0.22ms가 됐다.

## 3. 게임 스레드 프로세서의 주 쿼리에 위치 쓰기를 선언하지 않는다

**증상**: 게임 스레드의 가벼운 프로세서가 하는 일보다 오래 걸린다. 트레이스에서 그 프로세서는 위치를 쓰는 다른 프로세서가 끝나는 순간에 함께 끝난다.

**설명**
- `mass.ForceInlineProcessorExecution`(기본 참)에서 한 쿼리의 직접 쓰기와 다른 쿼리의 직접 읽기가 겹치면, 엔진은 아키타입이 겹치는지 보지 않고 두 쿼리를 전역 충돌(`ChunkGlobalContentionIndex`)로 묶는다(`ME/MassProcessingQueueTypes.cpp:26,31-33,331-340`).
- 묶인 쿼리는 시작하기 전에 상대가 끝나기를 기다린다. 그 기다림은 자체 타이머 없이 기다린 프로세서의 실행 시간에 잡힌다.
- ReadWrite 선언은 읽기와 쓰기 양쪽에 든다.
- 실행 중 쿼리 순서는 자유이고 엔진은 돌리지 않은 쿼리를 검사하지 않는다(`ME/MassProcessingQueueTypes.cpp:547-559`). 그래서 드물게 쓰는 프래그먼트는 따로 된 쿼리로 뺄 수 있다.

**절차**
1. 드물게 쓰는 프래그먼트(위치 같은)는 따로 된 쿼리로 뺀다.
2. 쓸 개체가 있는 프레임에만 그 쿼리를 돌린다.

```cpp
// 예시 이름: UFarHitIskmProcessor::ConfigureQueries와 Execute
EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadWrite); // 주 쿼리에는 위치가 없다
CatchUpQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);            // 위치는 따라잡기 쿼리만
if (CatchUps.Num() > 0) { CatchUpQuery.ForEachEntityChunk(Context, /* ... */); }
```

**결과**: 화면 끝 밖의 피격 개체를 ISKM으로 올리는 프로세서를 이렇게 고쳤다. Development 빌드 진단에서 그 프로세서의 실행 시간이 거의 0이 됐다.

## 4. 프로세서 단위 서브시스템 선언이 다른 프로세서를 막는지 본다

**증상**: 게임 스레드 러너가 고를 프로세서가 없어 노는 빈틈이 생긴다. 그 빈틈의 끝은 작업자의 다른 프로세서가 끝나는 때와 맞는다.

**설명**
- 실행 큐는 `ProcessorRequirements`에 선언한 서브시스템 요구를 그대로 담는다. 한 프로세서가 쓰고 다른 프로세서가 읽거나 쓰면 두 프로세서를 서로 막는 쌍으로 둔다(`ME/MassProcessingQueueTypes.cpp:137-138,203-214,376-382`).
- `TMassExternalSubsystemTraits::ThreadSafeWrite`는 의존성 해결기만 읽는다. 그래서 이 충돌을 풀지 못한다(`ME/MassProcessorDependencySolver.cpp:1078`).
- 러너는 프로세서를 고를 때 충돌 칸을 잡고 쿼리를 도는 동안만 놓는다. 그래서 쿼리 밖에서 오래 일하는 프로세서는 상대를 끝까지 막는다(`ME/MassProcessingQueue.cpp:369-380`).
- 쿼리에 선언한 서브시스템은 그 쿼리의 데이터 접근에만 든다(`bAggregateRequirements` 기본 거짓, `Engine/Source/Runtime/MassEntity/Public/MassProcessor.h:261-264`).
- 게임 스레드 러너는 고를 프로세서가 없으면 `YieldThread` 루프로 기다린다(`ME/MassProcessingQueue.cpp:592-617`).

**절차**
1. 트레이스에서 러너의 빈틈(`MassProcessingQueue Main-Thread Runner Task`의 Excl)을 찾는다.
2. 그 빈틈의 끝에 끝난 프로세서를 맞춰 본다.
3. 서브시스템 쓰기는 꼭 필요한 프로세서에만 프로세서 단위로 선언한다.
4. 선언을 뺄 수 없으면 앞쪽 사슬을 줄인다(2항목).

```cpp
// 두 프로세서의 ConfigureQueries. UCrowdSubsystem은 예시 이름
// UPushApartProcessor: 확인 로그에 쓰는 값을 읽는다
ProcessorRequirements.AddSubsystemRequirement<UCrowdSubsystem>(EMassFragmentAccess::ReadOnly);
// UFarHitIskmProcessor: 위와 서로 막는 쌍이 된다
ProcessorRequirements.AddSubsystemRequirement<UCrowdSubsystem>(EMassFragmentAccess::ReadWrite);
```

**결과**
- 참고 장면의 트레이스에서 이 두 선언이 밀어내기 프로세서와 피격 ISKM 프로세서를 서로 막았다.
- 읽기 선언을 빼면 표현이 밀기 전 위치를 읽는 프레임이 늘었다. 그래서 선언을 두고 앞쪽 사슬(방향 보간)을 병렬 사본으로 줄였다.

## 5. 프로세서별 시간은 직렬 트레이스로 가른다

**증상**: 병렬 트레이스에서 어떤 프로세서의 구간이 프레임 대부분에 걸친다. 그래서 그 프로세서가 비싸 보인다.

**설명**
- 병렬 처리 큐에서 프로세서가 쿼리 자원을 기다린 시간은 그 프로세서의 실행 시간에 잡힌다. 기다림 자체의 타이머는 없다.
- 게임 스레드에서 기다리면 스핀(`YieldThread` 루프)이다. 그래서 그 시간은 게임 스레드 시간에도 든다.
- `mass.FullyParallel`이 0이면 Mass 프로세서는 게임 스레드에서 차례로 돈다(`Engine/Source/Runtime/MassEntity/Private/MassProcessingPhaseManager.cpp:31,36,576`).
- 프로세서 안의 `ParallelFor`와 `ParallelForEachEntityChunk`는 직렬에서도 작업자로 나뉜다. 쿼리의 병렬 순회까지 끄는 값은 `mass.AllowQueryParallelFor`(기본 참)로 따로다(`ME/MassEntityQuery.cpp:26-31`).
- Mass 프로세서의 CSV 스탯은 `MassProcessorTask`다(`ME/MassProcessor.cpp:80`).

**경고**: 직렬 트레이스는 원인을 가르는 수단이다. 병렬에서 기다린 시간도 그 실행의 실제 비용이다. 직렬 실행의 프레임 시간은 비교에 쓰지 않는다.

**절차**
1. `mass.FullyParallel`을 0으로 두고 실행한다. 실행 인자나 시작 코드로 넣는다.
2. 그 실행에서 트레이스(Unreal Insights)를 한 번 더 뜬다. 측정 실행과 따로 돈다.
3. 직렬 트레이스에서 프로세서별 시간을 읽는다. 프레임당 시간 = 구간 합 ÷ 프레임 수다.

```cpp
// 트레이스 실행에서만
IConsoleManager::Get().FindConsoleVariable(TEXT("mass.FullyParallel"))->Set(0, ECVF_SetByCode);
```

**결과**
- 엔진 회피를 쓴 구성의 병렬 트레이스 한 프레임에서 엔진 방향 보간의 구간은 0.69ms부터 28.8ms까지 걸쳐 있었다.
- 직렬 트레이스에서 같은 프로세서는 프레임당 0.82ms였다.

## 6. 틱 간격을 늘리기보다 큰 몫의 프로세서를 쿼리에서 뺀다

**증상**: Simulation LOD로 먼 개체의 틱 간격을 늘렸다. 그런데 프레임이 줄지 않는다.

**설명**
- 가변 틱은 기본으로 꺼져 있다(`bEnableVariableTicking = false`). 켜면 LOD 태그도 붙는다(`MG/MassLOD/Private/MassLODTrait.cpp:128-133`).
- 엔진 가변 틱은 청크 필터 `ShouldTickChunkThisFrame`과 누적 델타를 쓰는 프로세서만 건너뛴다. 엔진에서 그런 프로세서는 Off 내비게이션, 방향 보간, SimpleMovement뿐이다.
- 엔진 이동·조향은 Off를 뺀 개체를 매 프레임 돈다. 넷을 한꺼번에 빼는 태그는 `FMassOffLODTag`뿐이다.
- 엔티티가 드나든 청크는 수정 번호가 바뀐다. 그래서 그 청크는 간격과 무관하게 다음 프레임에 돈다(`MG/MassLOD/Public/MassLODTickRateController.h:127-154`). 태그가 자주 바뀌는 군중(피격, 사망, LOD 전환)에서는 실제 주기가 설정보다 짧다.
- 엔진의 `UMassOffLODNavigationProcessor`는 Off 개체를 이동 목표의 중심으로 순간 이동시킨다(`MN/Private/MassNavigationProcessors.cpp:31-38`). 목표를 쫓는 군중이면 먼 개체가 목표로 튄다.
- `UMassSimulationLODTrait`의 `Params`, `bEnableVariableTicking`, `VariableTickParams`는 `protected`다. 코드로 넣으려면 자식 클래스를 만든다.

**경고**: 가변 틱 필터를 읽는 프로세서는 `ExecuteAfter(ProcessorGroupNames::LOD)`를 적어야 한다. "이 프레임에 돈다"는 값은 LOD 그룹의 프로세서가 쓰고 다음에 돌 때 지운다. 적지 않으면 순서가 프레임마다 바뀌어 그 프로세서가 주기를 놓친다(`MG/MassLOD/Private/MassSimulationLOD.cpp:76-77`).

**절차**
1. 먼 개체를 Simulation LOD의 Off에 둔다.
2. 직접 짠 프로세서로 Off 개체를 1초마다 옮긴다. 흐른 시간은 자체 시각으로 센다. 엔진의 누적 델타는 한 주기 늦다.
3. 엔진의 `UMassOffLODNavigationProcessor`의 등록을 끈다(2항목의 CDO 방법).

```cpp
// 예시 이름: UCrowdSimulationLODTrait(UMassSimulationLODTrait의 자식) 생성자
bEnableVariableTicking = true; // LOD 태그도 함께 붙는다
VariableTickParams.TickRates[EMassLOD::Medium] = 0.5f;
VariableTickParams.TickRates[EMassLOD::Low] = 1.f;
Params.LODDistance[EMassLOD::Off] = 20000.f; // 200m 밖은 Off
VariableTickParams.TickRates[EMassLOD::Off] = 1.f;
```

**결과**
- 죽고 다시 생기며 태그가 바뀌는 군중에서 설정한 0.5초 간격의 실제 주기는 0.13초였다. 실제 주기 = 평균 프레임 시간 × 밴드 개체 수 ÷ 그 프레임에 돈 개체 수로 셈했다.
- 앞선 시험에서는 Off로 엔진 이동을 뺀 변형만 프레임이 줄었다.
- 최종 구성에서 Simulation LOD를 끈 조건은 0.24ms 짧았다. 구별되지 않는다.

## 7. 결정적 해시로 대상을 고를 때는 `HashCombineFast`의 결과를 섞는다

**증상**: 일련번호와 틱으로 해시를 만들어 가장 작은 값의 개체를 고른다. 그런데 그 코드는 늘 작은 일련번호만 뽑는다.

**설명**
- `HashCombineFast(A, B)`는 `A ^ (B + 0x9e3779b9 + (A << 6) + (A >> 2))`다. `GetTypeHash(uint32)`는 값을 그대로 돌려준다(`Engine/Source/Runtime/Core/Public/Templates/TypeHash.h`).
- 그래서 틱은 모든 후보에 같은 수를 더할 뿐 순서를 바꾸지 못한다.

**절차**
1. 입력과 결과를 같은 파일의 `MurmurFinalize32`로 섞는다.

```cpp
// 예시 이름: PickHash
inline uint32 PickHash(uint32 Serial, uint32 Tick)
{
	return MurmurFinalize32(HashCombineFast(MurmurFinalize32(Serial), Tick));
}
```

**결과**
- 같은 식을 Python으로 흉내 내 후보 2,000개, 틱 200개로 돌렸다.
- 섞기 전에는 200번 모두 가장 작은 일련번호를 뽑았다.
- 섞은 뒤에는 200번에 서로 다른 대상 189개를 뽑았다. 뽑힌 순위의 평균은 1,043이다. 고르게 뽑으면 1,000이다.
