# 증상별 원인과 고칠 것

Mass 군중에서 겪는 증상을 원인과 고칠 것으로 잇는 표다. 자세한 설명, 코드, 결과는 "자세히" 칸의 참조 파일 항목에 있다. 수치는 `results.md`의 조건에서 잰 참고값이다.

## 성능

| 증상 | 원인 | 고칠 것 | 자세히 |
| --- | --- | --- | --- |
| Mass로 옮겼는데 게임 스레드가 그대로다 | 하이드레이션 액터의 고정비(메시 틱, 애니메이션, 캡슐 바디 이동, 위치 동기화). 액터 상한이 걸리면 상한이 비용을 정한다 | 하이드레이션만 끈 조건으로 몫을 잰다. 액터 하나의 비용을 줄인다(캡슐, 메시 고정 바운드, Budget Allocator) | `actors.md` 1 |
| 작업자 하나가 프레임 내내 한 프로세서를 돈다 | `ForEachEntityChunk`로 도는 엔진 프로세서(회피, 방향 보간, 이동) | 병렬 밀어내기 프로세서, `ParallelForEachEntityChunk` 사본 | `processors.md` 1, 2 |
| 엔진 회피가 모든 개체를 돈다 | Simulation LOD 태그가 없어 Medium·Low·Off 제외가 걸리지 않는다 | 가변 틱이나 `bSetLODTags`를 켠다. 또는 회피를 대체한다 | `processors.md` 1 |
| 게임 스레드의 가벼운 프로세서가 오래 걸린다 | 주 쿼리의 위치 ReadWrite가 전역 충돌로 묶여 상대를 기다린다 | 드문 쓰기를 따로 된 쿼리로 빼고 필요한 프레임에만 돌린다 | `processors.md` 3 |
| 게임 스레드 러너가 논다 | 프로세서 단위 서브시스템 쓰기 선언이 다른 프로세서와 서로 막는 쌍이 된다. `ThreadSafeWrite`는 풀지 못한다 | 꼭 필요한 프로세서에만 선언한다. 못 빼면 앞쪽 사슬을 줄인다 | `processors.md` 4 |
| 병렬 트레이스에서 한 프로세서가 비싸 보인다 | 기다린 시간이 실행 시간에 잡힌다 | `mass.FullyParallel 0` 직렬 트레이스로 프로세서별 시간을 읽는다 | `processors.md` 5 |
| Simulation LOD를 켜도 프레임이 그대로다 | 가변 틱은 청크 필터를 읽는 일부 프로세서만 건너뛴다. 엔진 이동·조향은 Off만 뺀다 | 먼 개체를 Off로 빼고 직접 옮긴다. `UMassOffLODNavigationProcessor`를 끈다 | `processors.md` 6 |
| 설정한 틱 간격보다 실제 주기가 짧다 | 엔티티가 드나든 청크는 다음 프레임에 돈다 | 실제 주기를 셈해 본다. 태그 변경을 줄이거나 Off로 뺀다 | `processors.md` 6 |
| 가변 틱 프로세서가 주기를 놓친다 | LOD 그룹 뒤에 돈다고 적지 않았다 | `ExecuteAfter(ProcessorGroupNames::LOD)` | `processors.md` 6 |
| 엔진 프로세서를 끈 설정이 ini에 남는다 | `SetShouldAutoRegisterWithGlobalList`가 에디터에서 `DefaultMass.ini`에 쓴다 | CDO의 `bAutoRegisterWithProcessingPhases`를 리플렉션으로 바꾼다 | `processors.md` 2 |
| 인스턴스가 늘자 GPU가 크게 는다 | 움직이는 인스턴스가 소프트웨어 Lumen 장면을 매 프레임 갱신한다 | 비주얼라이저 컴포넌트마다 `SetAffectDynamicIndirectLighting(false)` | `instances.md` 2 |
| 먼 개체의 비용이 화면에 보이는 수를 따르지 않는다 | 표현이 None이 아니면 화면 밖이어도 `CanBeSeen`이다 | 먼 구간을 `EMassRepresentationType::None`으로 둔다 | `instances.md` 1 |
| ISKM 렌더 스레드 비용이 인스턴스 수를 따라 는다 | 인스턴스가 바뀐 프레임마다 `UpdateBounds()`가 인스턴스마다 돈다 | 카메라를 따라가는 고정 바운드(`SetPrimitiveBoundsOverride`) | `instances.md` 4 |
| 액터 메시 애니메이션 시간이 액터 수보다 빨리 는다 | 애니메이션 갱신마다 피직스 에셋 바디로 바운드를 다시 셈한다 | `bComponentUseFixedSkelBounds`, `BoundsScale` 1.1 | `actors.md` 6 |
| Budget Allocator를 켜도 프레임이 그대로다 | 수요가 예산 안이면 모든 메시를 매 프레임 갱신한다 | 수요와 예산을 견준다. 예산을 넘는 개체 수에서만 효과가 있다 | `actors.md` 4 |

## 화면과 메모리

| 증상 | 원인 | 고칠 것 | 자세히 |
| --- | --- | --- | --- |
| 액터 경계에서 개체가 깜빡인다(액터 ↔ 인스턴스 왕복) | 엔진 표현 LOD의 히스테리시스가 액터 경계에 없다 | 최소 유지 시간 프로세서(`LOD` 뒤, `VisualizationProcessing` 앞) | `actors.md` 2 |
| 먼 개체가 경계 안쪽에서 늦게 나타난다 | Off 청크 재검사 0.5초와 히스테리시스(경계의 90% 지점) | 알려진 동작으로 둔다 | `instances.md` 1 |
| 인스턴스로 바뀐 개체에 그림자가 없다 | 설명자 `bCastShadows` 기본 false | 설명자에서 켠다. 먼 구간만 끄려면 중요도 범위로 설명자를 나눈다 | `instances.md` 3 |
| ISKM 트랙과 GPU 메모리가 계속 는다 | 움직이는 엔티티 파괴 때 `DeallocateTrack`이 불리지 않는다(5.8.4에도 있음, 포럼 보고 있음) | 엔티티 풀로 다시 쓴다. 상태가 바뀐 프레임에 `SetAutoPlayData`를 직접 부른다 | `instances.md` 5 |
| 풀에서 꺼낸 ISKM 개체의 클립이 처음부터 재생되지 않는다 | 트랙이 있으면 클립 번호가 바뀔 때만 재생 데이터를 쓴다 | `SetAutoPlayData`를 직접 부른다 | `instances.md` 5 |
| ISKM 개체 전체가 한 프레임 세로로 끌린다 | 트랙 수 변화 때 뼈대 버퍼 재할당이 옛 자리와 겹친다 | 처음에 용량까지 트랙을 할당한 뒤 해제한다 | `instances.md` 6 |
| ISKM 개체가 피격 클립에서 눕거나 사라진다 | 가산 클립을 트랙 제공자가 기준 자세 없이 그대로 쓴다 | 가산 종류를 `AAT_None`으로 둔 복제본 | `instances.md` 7 |
| VAT 개체의 그림자가 바닥에 남는다 | AnimToTexture Bone 모드 바운드가 정점 범위보다 작아 VSM이 페이지를 무효화하지 않는다 | Vertex 모드 정점 범위로 바운드 확장 | `instances.md` 8 |
| Niagara 입자의 VAT 프레임이 엉뚱하다 | 커스텀 데이터 0번이 입자 인덱스다 | Dynamic Material Parameter 1번으로 넘긴다 | `instances.md` 9 |
| Budget Allocator가 켜지지 않는다 | CVar만 넣었다 | 월드의 `SetEnabled(true)` | `actors.md` 4 |
| 풀에 돌아간 액터가 틱하거나 트레이스에 맞는다 | 엔진 액터 풀은 숨기기만 한다 | `PrepareForPooling`에서 틱·중요도·콜리전을 끄고 `PrepareForGame`에서 되돌린다 | `actors.md` 3 |
| 풀에서 꺼낸 액터의 크기가 1로 돌아간다 | 엔진이 엔티티 변환(스케일 1)으로 옮기고 `BeginPlay`를 다시 부르지 않는다 | `PrepareForGame`에서 스케일을 다시 넣는다 | `actors.md` 3 |
| 해시로 고르면 작은 일련번호만 뽑힌다 | `HashCombineFast`가 순서를 바꾸지 못한다 | `MurmurFinalize32`로 섞는다 | `processors.md` 7 |
| 캡슐 라인 트레이스가 일정 비율로 놓친다 | 시작점이 캡슐의 정면 평면 위에 있다(원인 분기는 원인 미확정) | 시작점이나 끝점을 몇 cm 비킨다 | `actors.md` 7 |
| 마켓 캐릭터가 배치 직후 래그돌처럼 늘어진다 | 피직스 에셋 바디의 Physics Type 설정 | 피직스 에셋 편집기에서 바디의 Physics Type을 Default로 바꾼다 | 에디터 작업 |
