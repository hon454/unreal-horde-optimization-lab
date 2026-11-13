# 문제 해결

빌드나 실행에서 이미 겪은 문제를 증상으로 찾는 문서다. 문제가 생겼을 때만 연다. 증상과 대처만 적고, 원인과 근거는 가리키는 문서에 있다. 새 문제를 겪으면 같은 형식으로 한 줄을 더한다.

| 증상 | 확인할 것과 대처 | 근거 |
| --- | --- | --- |
| `Scripts/common.ps1`이 엔진을 찾지 못한다 | 스크립트를 실행하는 프로세스에서 `reg query "HKCU\Software\Epic Games\Unreal Engine\Builds"`에 `.uproject`의 `EngineAssociation` 이름이 보이는지 확인한다. 없으면 사용자에게 알리고 승인을 받아 등록한다 | 다른 프로젝트에서 에이전트 프로세스의 레지스트리 값이 다르게 보인 적이 있다 |
| `common.ps1`이 "not a registered source build"를 던진다. `.uproject`의 `EngineAssociation`이 모르는 GUID로 바뀌어 있고 끝 개행이 사라졌다 | `git diff HordeOptLab.uproject`로 확인하고 커밋 값 `{DEC43C63-4155-0388-B4B3-6B830A2347BA}`(HKCU Builds의 `G:/Epic Games/UE_5.8.4`)로 되돌린다. 런처나 버전 선택기(우클릭 "Switch Unreal Engine version")가 다시 쓴 것이다. 되돌리기 전에 에디터 프로세스가 없는지 본다 | 2026-11-09 15:41, 선행 묶음 패키징 직전에 발생. 소스 빌드만 쓴다(AGENTS.md) |
| AssetHost 커맨드렛(`migrate-assets.py`)이 Python 출력 없이 종료 코드 1로 끝난다 | AssetHost `Saved/Logs/AssetHost.log`에 "The game module 'AssetHost' could not be found"가 있으면, AssetHost의 빈 C++ 모듈이 다른 엔진(소스 빌드)으로 컴파일된 것이다(`Binaries/Win64/UnrealEditor.modules`의 BuildId가 런처 엔진과 다름). 런처 엔진의 `Engine/Build/BatchFiles/Build.bat AssetHostEditor Win64 Development -project=<AssetHost.uproject>`로 다시 컴파일한다 | [Worklog/M3b.md](../Worklog/M3b.md) "2종(에셋)" |
| 빌드가 DLL 잠금으로 실패한다 | 에디터나 패키지 실행이 떠 있는지 확인하고 닫는다 | |
| Paragon 미니언이 배치 직후 래그돌처럼 늘어진다 | 피직스 에셋의 바디 Physics Type을 Default로 바꾼다 | [references.md](../Reference/references.md) 포럼 스레드 |
| Test 패키지에서 `stat anim`, `ProfileGPU`, `a.Budget.Debug`가 동작하지 않는다 | 정상이다. Test에서는 컴파일되지 않는다. Development(`run-dev.ps1`)로 찍는다 | [ADR-0001](../Decisions/0001-test-package-for-measurement.md) |
| Mass 하이드레이션 액터가 풀에 돌아간 뒤에도 애니메이션 예산을 쓴다 | `IMassActorPoolableInterface` 구현에서 중요도 0, 틱 끔을 확인한다 | [engine-notes.md](../Reference/engine-notes.md) "액터 풀링" |
| `run-dev.ps1 -Extra`에 준 인자 일부가 빠지거나 듣지 않는다 | 게임 인자 여럿은 한 문자열로 넘긴다: `-Extra "-LabStage=M3 -LabTypes=2"` | [Worklog/M3b.md](../Worklog/M3b.md) "2종(에셋)" |
| 개발 실행이 M0 액터로 돈다 | `-LabStage=`를 빠뜨렸다. 코드 기본값이 M0다. `-Extra "-LabStage=M<N>"`을 붙인다 | `Source/HordeOptLab/LabConfig.cpp` |
| 촬영한 GIF에 게임 창의 왼쪽 위 일부만 담기고 나머지가 검다(화면 배율 150%) | ffmpeg `gdigrab -i title=`이 DPI를 몰라 창 크기를 논리 픽셀로 읽는다. `capture-full.ps1`은 DPI 인식 좌표로 클라이언트 영역을 구해 `-i desktop`에 offset과 크기를 준다(624d6ae). 직접 ffmpeg를 부를 때도 같은 방식을 쓴다 | [Worklog/Q5.md](../Worklog/Q5.md) "후처리 스크립트 수정(2026-11-12)" |
| `run-full.ps1`가 "too few frames or unparsable check output"으로 실행을 버린다(액터 1만 조건) | 측정 구간의 프레임 수가 100 미만이다. 프레임당 400\~470ms면 10초 구간에 26\~30프레임이다. `-DurationOverride`는 예행 전용이고 정식 60초 구간에서 통과하는지는 아직 재지 않았다. 검사 값(100)은 러너 코드에 있다 | [Worklog/Q5.md](../Worklog/Q5.md) "예행 묶음 Reh2", `Scripts/run-full.ps1` |
| Unreal MCP `AddUserVariables`로 Niagara 배열(Vector 4 Array 등) 사용자 변수를 만들었는데 오류 없이 아무것도 생기지 않는다 | 엔진이 데이터 인터페이스 변수를 직접 넣는 경로를 막아 놓았다. 다이내믹 입력의 배열 입력을 `SetStackInputData`로 그 이름의 사용자 변수에 연결한다 | [engine-notes.md](../Reference/engine-notes.md) 24절, [Worklog/Q2.md](../Worklog/Q2.md) |
| Niagara 에디터에 "Disabling interpolated spawn because emitter flag and script type don't match" 경고가 남는다 | 동작은 같다. MCP로 이미터 값을 바꿀 때 `PostEditChange`가 불리지 않아 생긴다. 에디터에서 이미터를 한 번 다시 저장하면 없어진다고 Q2 세션이 적었다(저장 후 확인은 하지 않았다) | [engine-notes.md](../Reference/engine-notes.md) 24절 |
