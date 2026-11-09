# 문제 해결

빌드나 실행에서 이미 겪은 문제를 증상으로 찾는 문서다. 문제가 생겼을 때만 연다. 증상과 대처만 적고, 원인과 근거는 가리키는 문서에 있다. 새 문제를 겪으면 같은 형식으로 한 줄을 더한다.

| 증상 | 확인할 것과 대처 | 근거 |
| --- | --- | --- |
| `Scripts/common.ps1`이 엔진을 찾지 못한다 | 스크립트를 실행하는 프로세스에서 `reg query "HKCU\Software\Epic Games\Unreal Engine\Builds"`에 `.uproject`의 `EngineAssociation` 이름이 보이는지 확인한다. 없으면 사용자에게 알리고 승인을 받아 등록한다 | DS 실험실에서 에이전트 프로세스의 레지스트리 값이 다르게 보인 적이 있다 |
| `common.ps1`이 "not a registered source build"를 던진다. `.uproject`의 `EngineAssociation`이 모르는 GUID로 바뀌어 있고 끝 개행이 사라졌다 | `git diff HordeOptLab.uproject`로 확인하고 커밋 값 `{DEC43C63-4155-0388-B4B3-6B830A2347BA}`(HKCU Builds의 `G:/Epic Games/UE_5.8.4`)로 되돌린다. 런처나 버전 선택기(우클릭 "Switch Unreal Engine version")가 다시 쓴 것이다. 되돌리기 전에 에디터 프로세스가 없는지 본다 | 2026-11-09 15:41, 선행 묶음 패키징 직전에 발생. 소스 빌드만 쓴다(AGENTS.md) |
| 빌드가 DLL 잠금으로 실패한다 | 에디터나 패키지 실행이 떠 있는지 확인하고 닫는다 | |
| Paragon 미니언이 배치 직후 래그돌처럼 늘어진다 | 피직스 에셋의 바디 Physics Type을 Default로 바꾼다 | [references.md](../Reference/references.md) 포럼 스레드 |
| Test 패키지에서 `stat anim`, `ProfileGPU`, `a.Budget.Debug`가 동작하지 않는다 | 정상이다. Test에서는 컴파일되지 않는다. Development(`run-dev.ps1`)로 찍는다 | [ADR-0001](../Decisions/0001-test-package-for-measurement.md) |
| Mass 하이드레이션 액터가 풀에 돌아간 뒤에도 애니메이션 예산을 쓴다 | `IMassActorPoolableInterface` 구현에서 중요도 0, 틱 끔을 확인한다 | [engine-notes.md](../Reference/engine-notes.md) "액터 풀링" |
