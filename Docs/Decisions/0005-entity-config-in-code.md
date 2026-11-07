# ADR-0005 Mass EntityConfig를 실행 시 C++로 만든다

- 상태: 승인됨(2026-11-07 채팅에서 사용자가 결정)
- 날짜: 2026-11-07

## 맥락

M1부터 미니언은 Mass 엔티티다. 엔티티가 가질 Fragment와 붙을 Processor는 Trait 묶음인 EntityConfig가 정하고, 스포너는 그 설정에서 엔티티 템플릿을 만든다. 단계는 한 빌드 안의 실행 모드라([ADR-0004](0004-stages-as-run-modes.md)) M1\~M5마다 Trait 구성이 갈린다.

## 결정

EntityConfig 에셋을 두지 않고, 단계마다 실행 시 `UMassEntityConfigAsset`을 `NewObject`로 만들어 `AddTrait`로 Trait을 붙인다. 표현은 엔진 Trait(`UMassMovableVisualizationTrait` 등)을 그대로 쓴다.

엔진 소스에서 확인한 근거:
- `UMassEntityConfigAsset::AddTrait(TSubclassOf<UMassEntityTraitBase>)`는 공개 API다(`Engine/Plugins/Runtime/MassGameplay/Source/MassSpawner/Public/MassEntityConfigAsset.h:159`).
- 템플릿 ID는 설정의 `ConfigGuid`로 만들고(`MassEntityConfigAsset.cpp:137`), 소유 오브젝트가 있으면 GUID를 그 경로로 결정적으로 만든다(`:33`). 실행 시 만든 설정도 실행마다 같은 템플릿으로 잡힌다.

## 고려한 대안

| 대안 | 버린 이유 |
| --- | --- |
| EntityConfig 에셋을 Unreal MCP로 만든다 | 에디터에서 구성을 보고 검증할 수 있고 엔진 예제와 같지만, Trait을 바꿀 때마다 에디터를 거치고 바뀐 내용이 git diff로 읽히지 않는다 |
| Trait 없이 아키타입을 직접 만들고 ISM도 직접 그린다 | 엔진 표현 경로(Representation LOD, 액터 하이드레이션)를 버려 M2\~M3에서 다시 만들어야 한다 |

## 결과

- 2026-11-07 Test 패키지 빌드에서 확인: `UMassEntityConfigAsset::AddTrait(TSubclassOf)`는 `#if WITH_EDITOR` 안에 있어 패키지에서 쓸 수 없다. 런타임 API인 `FMassEntityConfig::AddTrait(UMassEntityTraitBase&)`(`MassEntityConfigAsset.h:61`)에 `NewObject`로 만든 Trait을 넘긴다(`LabCrowdSubsystem.cpp` `AddLabTrait`). 결정은 그대로다.
- 글에 Trait 구성 화면이 필요하면 같은 구성을 에셋으로 한 번 저장해 찍는다.
