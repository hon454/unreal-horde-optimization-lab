# 현재 상태

마지막 갱신: 2026-11-06

## 단계

**계획 확정. 저장소에 코드와 엔진 프로젝트가 아직 없다.** 다음은 M0 기준선이다([plan.md](plan.md) "M0").

- 계획 리뷰(2026-11-06)에서 바꾼 것은 plan.md와 measurement.md에 반영됐다: 초기 스폰을 정상 상태 분포로, M0에 변형 셋(full / static / budget), M1은 회피 없이 시작, 거리 밴드는 M0에서 동결, 앵커 N {1k, 5k}, 3회 범위 보고, 수치는 Test 패키지에서만.
- 엔진 소스에서 확인한 사실은 [engine-notes.md](Reference/engine-notes.md)에 있다. 계획 문서가 틀렸던 것(Mass 코어 이동 시점, `mass.FullyParallel`, `LODSignificance` 0\~3, ISKM 블렌딩)은 이미 고쳤다.

## 다음 할 일

M0의 작업이다. 순서대로 한다. 각 항목의 완료 조건은 plan.md "M0"에 있다.

1. 저장소·참고 디렉터리 상태 확인. 엔진 소스 빌드의 태그를 `Engine/Build/Build.version`에서 읽어 [pc-specs.md](Reference/pc-specs.md)에 적는다. 5.8.4가 아니면 engine-notes.md의 줄 번호를 그 태그로 다시 확인한다.
2. `HordeOptLab.uproject`(`EngineAssociation` = 이 PC에 등록된 소스 빌드 이름), `Source/HordeOptLab/`(모듈, `HordeOptLab.Target.cs`, `HordeOptLabEditor.Target.cs`), `Config/`. 플러그인: MassEntity 계열(`MassGameplay`, `MassAI`), `AnimationBudgetAllocator`, `SignificanceManager`, `AnimToTexture`, `ModelContextProtocol`, `AllToolsets`(에디터만).
3. `Scripts/common.ps1`, `build.ps1`, `run-dev.ps1`(Development `-game`), `package.ps1`(Test, Win64), `run-bench.ps1`(패키지 실행 + 라벨 + 반복). 스크립트 사양은 [commands.md](Guides/commands.md).
4. Paragon Minions 임포트(근접 미니언 한 종류), 피직스 에셋 수정(바디 Physics Type → Default), 스켈레톤·애니 확인.
5. 평지 맵(`Content/Lab/Maps/L_Bench`), 오브(`ALabOrb`: 근접 소멸 반경 + 주기 저격, 구체 + 디버그 라인), 스포너(`ALabCrowdSpawner`: 정상 상태 분포 초기 배치 + 가장자리 재생성), 관전 카메라 + 프리셋 3종.
6. 미니언 `ACharacter` 세 변형과 `-LabVariant=full|static|budget` 인자.
7. 벤치마크 러너(`-LabBench -LabN= -LabPreset= -LabSeed= -LabLabel=`): CVar 고정 → 워밍업 30초 → `csvprofile start` → 60초 → `csvprofile stop` → 메타데이터 로그 → 종료. `Scripts/summarize.py`가 `Saved/Profiling/CSV/`를 읽어 `Measurements/M0/runs.csv`·`summary.csv`를 쓴다.
8. 거리 밴드 수치와 시나리오 고정값(이동 속도, 소멸 반경, 저격 간격·대상 수, 맵 반경, 프리셋 좌표)을 실측해 표로 보고하고 사용자 확정을 받아 measurement.md "확정값"에 적는다.
9. E0 측정(Test 패키지): 변형 3 × N {250, 500, 1k, 2k, 5k} × 프리셋 3 × 3회. 대표 스크린샷. `Scripts/plot.py` 초판.
10. ADR-0002 "측정 인프라 동결" 초안, README "결과 한눈에 보기" 초판, 태그 `M0`, Worklog, backlog "글 대기열"에 `00-testbed`, `01-baseline` 추가.

## 명령

M0에서 스크립트를 만들면 채운다. 그때까지는 [commands.md](Guides/commands.md)의 사양을 따른다.

## 측정 결과

진행 중인 묶음이 없다.

## 막힌 것

- 없음.

## 사용자에게 요청한 일

- `../unreal-horde-references/` 구성: Fab에서 **Paragon: Minions**, **City Sample**, **MetaHuman Crowd Sample**을 받아 두고, 엔진 소스 빌드 `Engine/`에 심볼릭 링크 `UnrealEngine/`를 만든다. 경로는 [references.md](Reference/references.md) "샘플 프로젝트".
- PC에 Python 3와 `matplotlib`, `pandas` 설치(`Scripts/summarize.py`, `plot.py`).
- [ADR-0001](Decisions/0001-test-package-for-measurement.md) 승인 확인(채팅에서 결정했고 문서로 옮겼다).
