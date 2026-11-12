# 코드 투어 작업 기록

최종 구성의 한 프레임을 코드에서 읽어 `Docs/Guides/code-tour.md`와 흐름 그림으로 만든 기록이다.

## 2026-11-12 최종 구성 코드 투어 (코드만 읽음, 실행 없음)

### 단계

- 조사: 성공. `Source/HordeOptLab/` 전 파일(`LabCrowdFxProcessor.cpp`·`LabGameMode.cpp`는 `git show HEAD:`), `Config/DefaultMass.ini`, 엔진 소스의 프로세서 생성자·Trait·그룹 이름을 읽었다. 다음: 문서 쓰기.
- 문서: 성공. `Docs/Guides/code-tour.md`(요약과 읽는 순서, 한 프레임의 흐름, Fragment와 Tag, 인자에서 설정까지, 돌지 않는 코드). 다음: 그림.
- 그림: 성공. `Scripts/plot-code-tour.py` → `Docs/figures/code-tour/frame-flow.svg`(800×954, 글자 16px 이상, 스크립트·외부 글꼴 없음). Edge 헤드리스로 밝은 바탕과 `#0d1117` 바탕에 렌더링해 겹침·잘림이 없음을 봤다. 확인용 PNG: 세션 스크래치 패드의 `frame-flow-light.png`, `frame-flow-dark.png`(저장소 밖, 커밋하지 않음). 다음: README·Scripts/README·engine-notes 27절.
- 확인: 성공. 네 문서의 상대 링크가 모두 실제 파일을 가리킨다. 문서의 클래스 이름은 `git grep HEAD -- Source` 또는 엔진 헤더에서 모두 찾았고, 함수·멤버 이름 37개도 `Source`에 있다.

### 확인한 것과 출처

- 실행 인자 → 설정: `LabConfig.cpp` `FLabConfig::Get()`(변형 기본값, 인자 우선, `Overrides`), 판정 함수 `UsesIskm`·`UsesSimLod`·`SimLod`·`UsesHydration`·`CullsFar`·`IskmParams`·`UsesCrowdFxEntity`.
- 프로세서 등록: `HordeOptLab.cpp` `FLabGameModule::StartupModule`(CDO의 `bAutoRegisterWithProcessingPhases`), `Config/DefaultMass.ini`(엔진 LOD 수집·표현 LOD·표현 켬, 엔진 프로세서 여덟 끔), 관찰자는 `ULabOffMoveEnterObserver` 생성자.
- 그룹과 선후: 각 Lab 프로세서 생성자와 `LabRunAfterSimLod`, 엔진 프로세서 생성자(engine-notes 27절에 더함). 게임 스레드 전용은 `ULabIskmProcessor`, `ULabCrowdFxGatherProcessor`, `ULabActorSyncProcessor` 셋(`bRequiresGameThreadExecution = true`).
- 엔티티 구성: `ULabCrowdSubsystem::BuildEntityConfig`(Trait 일곱, 표현 LOD 밴드 0/40/80/200m와 상한 1,000/5,000, 원거리 그림자 설명자 나누기, ISKM 설명자), `ULabMinionTrait::BuildTemplate`, 엔진 Trait의 `BuildTemplate`.
- `LabCrowdFxProcessor.cpp`·`LabGameMode.cpp`는 다른 세션의 시험 변경을 피해 `git show HEAD:`로 읽었다(HEAD d58fb96 이후 `Source/` 변경 없음).

### 순서가 정해져 있지 않은 곳

- 두 LOD 계산(`UMassSimulationLODProcessor`, `UMassVisualizationLODProcessor`) 사이.
- Movement 그룹 안(`UMassApplyMovementProcessor`, `UMassSmoothOrientationProcessor`, `ULabOffMoveProcessor`).
- LabLogic 그룹과 표현 프로세서(`UMassVisualizationProcessor`, `ULabIskmProcessor`, `ULabCrowdFxGatherProcessor`, `ULabActorSyncProcessor`) 사이. 프래그먼트 접근 겹침은 청크 단위 충돌뿐이다(engine-notes 13절).
- `ULabIskmProcessor`와 `ULabCrowdFxGatherProcessor` 사이.
- `ULabNearOrbCountProcessor`(선언 없음).
- 액터 틱(`ALabOrb`, `ALabCameraRig`, `ALabGameMode`, 모두 기본 `TG_PrePhysics`)과 Mass PrePhysics 틱 함수 사이.

### 확인 못 한 것

- 최종 구성의 실제 처리 그래프("flat processing graph" 로그)는 보지 않았다. 실행을 하지 않는 작업이다. 쿼리 기반 가지치기(engine-notes 27절)로 빠지는 프로세서가 있는지(예: `ULabDamageApplyProcessor`)는 로그로만 확정할 수 있다. 문서는 "등록되지만 일하지 않는다"로 적었다.
- 지연 명령 버퍼가 PrePhysics 안에서 몇 번, 언제 비워지는지는 엔진 소스에서 따라가지 않았다. 문서는 "명령 버퍼를 비울 때"로만 적었다.
- `UMassVisualizationProcessor`·`UMassVisualizationLODProcessor`가 쿼리 요구(서브시스템)로 게임 스레드에 묶이는지는 보지 않았다. 생성자에는 게임 스레드 선언이 없다.
- `UMassActorSpawnerSubsystem`이 액터를 스폰하는 틱 시점은 보지 않았다.

### 문서 지도에 더할 행(코디네이터용)

`| 최종 구성에서 한 프레임에 도는 코드, 프로세서 순서, 엔티티 구성 | [code-tour.md](Docs/Guides/code-tour.md). 흐름 그림은 Scripts/plot-code-tour.py |`
