# 명령과 측정 절차

STATUS.md "명령"에는 매번 쓰는 것만 둔다. 여기에는 스크립트 사양, 인자, 측정 절차를 둔다. M0에서 스크립트를 만들면서 실제 명령으로 채운다. 그때까지는 사양이다.

## 스크립트 사양(M0 산출물)

모든 스크립트는 `Scripts/common.ps1`을 불러 엔진 경로를 얻는다. 엔진 경로는 저장소에 적지 않고 `.uproject`의 `EngineAssociation`을 레지스트리(`HKCU\Software\Epic Games\Unreal Engine\Builds`)에서 찾는다(DS 실험실 방식).

| 스크립트 | 하는 일 | 거부 조건 |
| --- | --- | --- |
| `build.ps1` | `HordeOptLabEditor Win64 Development` 빌드 | `UnrealEditor` 또는 `HordeOptLab` 프로세스가 떠 있으면 |
| `run-dev.ps1 -N -Preset -Seed [-Variant] [-Small]` | Development 에디터 빌드를 `-game`으로 실행. 개발·동작 확인·오버레이 스크린샷용. 수치는 쓰지 않는다 | |
| `package.ps1 [-Config Test]` | `RunUAT BuildCookRun` → `Packaged/<Config>/`. 끝나면 소요 시간을 출력 | 다른 실행이 떠 있으면 |
| `run-bench.ps1 -Label <라벨> -N <수> -Preset <P1\|P2> [-Variant] [-Seed] [-Runs 3] [-Trace]` | Test 패키지 실행 파일을 `-LabBench` 인자로 띄우고 종료를 기다린다. 실행마다 `Saved/Profiling/CSV/<라벨>-rN.csv`를 확인하고, 없으면 실패 | 같은 라벨의 CSV가 있으면. 다른 실행이 떠 있으면 |
| `summarize.py --milestone M<N>` | `Saved/Profiling/CSV/`의 라벨을 읽어 `Measurements/M<N>/runs.csv`에 행을 더하고 `summary.csv`를 다시 쓴다 | |
| `plot.py` | `Measurements/*/summary.csv`로 `Docs/figures/`의 그래프를 다시 그린다: 60fps 최대 N(대표), 고정 조건 Game/Render/GPU 스택, N 스케일링, 밴드 분포(M2+), 예산 사용(M3+) | |
| `capture-video.ps1` | 화면 녹화. 찍기 전 승인 필요(AGENTS.md) | 측정 실행이 떠 있으면 |

## 벤치마크 러너 인자(게임 측)

`-LabBench` 가 있으면 다음 순서로 동작하고 끝나면 종료한다.

1. 렌더 CVar 고정(measurement.md "렌더 고정")과 `-LabN= -LabPreset= -LabSeed= -LabVariant=` 적용. 모든 값을 로그 `lab_config`에 한 줄로 남긴다.
2. 스포너가 정상 상태 분포로 N개 배치. 카메라를 프리셋에 놓는다.
3. 15초 워밍업. 그 사이 N 유지 확인(사망·재생성이 동작하는지).
4. `csvprofile start`(압축 끔, 라벨을 파일 이름으로). 60초.
5. `csvprofile stop`. 프레임 수, N, 전환 수 등 요약을 로그 `lab_summary`에 남긴다. 조건이 깨졌으면(N 불일치 등) 종료 코드 1.
6. `-Trace`면 2\~5 동안 `-trace=cpu,gpu,frame,counters,mass`로 트레이스를 남긴다. 트레이스 실행의 수치는 비교에 쓰지 않는다(이벤트 기록 비용).

## 측정 절차

1. 측정 전에 잴 조건, 라벨, 걸리는 시간을 채팅에 알리고 답을 받는다.
2. `package.ps1`로 그 커밋의 Test 패키지를 만든다. 패키지 폴더에 커밋 해시를 남긴다.
3. 묶음의 맨 첫 실행 1회만 버린다(`run-bench.ps1`의 첫 조건만 `-r0`, 나머지는 `-SkipR0`). 그 뒤 조건별 3회.
4. 실행마다 로그의 `lab_config`와 `lab_summary`를 확인한다. N이 유지되지 않았거나 프레임 수가 비정상이면 그 실행을 버리고 라벨을 바꿔 다시 잰다.
5. `summarize.py` → `runs.csv`, `summary.csv`. 차이 판정은 measurement.md.
6. STATUS.md "측정 결과"에 묶음을 적는다. 마일스톤이 끝나면 Worklog로 옮긴다.

## 스크린샷·영상 실행

- 오버레이 스크린샷(`stat unit`, `a.Budget.Debug.Enabled 1`, 밴드 색 디버그 뷰)은 `run-dev.ps1`로 Development에서 찍는다. 그 실행의 수치는 쓰지 않는다.
- Test 패키지 스크린샷은 `stat unit`만 가능하다.
- 영상은 수치 실행에서 찍지 않는다. 라벨 `visualN`으로 따로 돌린다.

## Unreal MCP

- 서버는 에디터가 뜨면 자동으로 열린다: `Config/DefaultEditorPerProjectUserSettings.ini`의 `bAutoStartServer=True`, `ServerPortNumber=8001`(DS 실험실의 기본 8000과 겹치지 않게). 수동은 콘솔 `ModelContextProtocol.StartServer`.
- 클라이언트 설정: 프로젝트 루트 `.mcp.json`(gitignore)에 `"unreal-mcp": {"type": "http", "url": "http://127.0.0.1:8001/mcp"}`. 새 체크아웃이면 에디터 콘솔 `ModelContextProtocol.GenerateClientConfig ClaudeCode`로 다시 만든다. Claude Code는 에디터를 띄운 뒤 시작해야 툴이 잡힌다.
- 2026-11-06 확인: `-nullrhi`로 띄운 에디터가 약 15초 뒤 `initialize`에 응답했다.
- 툴 호출은 게임 스레드에서 직렬. 겹쳐 부르지 않는다. 대량 변경 전후 저장. 새 `UFUNCTION`은 에디터 재시작.
- Claude Code는 `claude --add-dir ..\unreal-horde-references`로 시작한다(`.claude/settings.json`의 `additionalDirectories`에도 있다).

## 에셋 마이그레이션

Fab 원본 팩은 `../unreal-horde-references/AssetHost/`(런처 5.8 엔진 프로젝트)에 받고, 쓰는 것만 `Scripts/migrate-assets.py`로 옮긴다. AssetHost를 연 엔진(런처 `G:\Epic Games\UE_5.8`)의 커맨드렛으로 돌린다. `LAB_MIGRATE_DRY=1`이면 의존성 목록과 크기만 출력한다.

```bash
LAB_MIGRATE_SET=minion LAB_MIGRATE_DRY=0 LAB_MIGRATE_DEST="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Content" "G:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "G:/Epic Games/UE_5.8.4/Games/unreal-horde-references/AssetHost/AssetHost.uproject" -run=pythonscript -script="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Scripts/migrate-assets.py" -unattended -nosplash -nullrhi -stdout
```

2026-11-06 실행: 51개 패키지, 원본 기준 282.7MB(옮긴 뒤 `Content/ParagonMinions/` 244MB). 소프트 참조로 Dusk 메시와 Super 텍스처 일부가 함께 온다.

`orbfx` 세트(`LAB_MIGRATE_SET=orbfx`)는 오브의 타워·구체·이펙트다. 2026-11-06에 AssetHost 커맨드렛이 Python 플러그인을 올리기 전에 로그 없이 종료 코드 1로 끝나는 일이 있었다. 이때 `MI_Mesh_Overlay_TeslaCoil`과 부모 `M_Mesh_Overlay`는 같은 `/Game` 경로로 `.uasset`을 직접 복사했다. 의존성(MPC, `MF_ExposureCompensation`, 노이즈 텍스처)은 이미 옮겨져 있었다.

## 파생 에셋과 맵

- `Scripts/make-static-minion.py`: `static` 변형의 `/Game/Lab/Derived/SM_LabMinion`을 만든다(gitignore). 커맨드렛에 `-AllowCommandletRendering`이 필요하다. 새 체크아웃에서는 에셋 이전 뒤 한 번 돌린다.
- `Scripts/create-bench-map.py`: `L_Bench`를 엔진 Basic 템플릿으로 다시 만든다(바닥을 맵 반경 + 20m로 키우고 VolumetricCloud 삭제). 기존 맵이 있으면 `.umap` 파일을 먼저 지운다(스크립트의 `delete_asset`이 커맨드렛에서 듣지 않았다).

```bash
"G:/Epic Games/UE_5.8.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/HordeOptLab.uproject" -run=pythonscript -script="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Scripts/make-static-minion.py" -unattended -nosplash -AllowCommandletRendering -stdout
```

## 벤치마크 실행(구현됨)

- `package.ps1`이 `Packaged/Test/Windows/`를 만들고 `Packaged/Test/commit.txt`에 커밋 해시(작업 트리가 더러우면 `(dirty)`)를 남긴다. 쿡은 `L_Bench`와, 코드가 경로로 부르는 `/Game/ParagonMinions`·`/Game/NiagaraExamples`·`/Game/Lab` 폴더다(`Config/DefaultGame.ini`).
- `run-bench.ps1 -Milestone M0 -N <n> -Preset <P1|P2> -Variant <v> [-Runs 3] [-Tag x] [-ResX -ResY]`: r0 + r1\~rRuns. 실행마다 CSV를 패키지 `Saved/Profiling/CSV/`에서 프로젝트 `Saved/Profiling/CSV/`로 옮기고 `Saved/Lab/<라벨>.summary.txt`를 출력한다. 해상도 기본값 1920x1080 창은 M0 8번에서 확정한다.
- `run-e1.ps1 [-Tag x]`: M1 E1 (a) 묶음(mass N {1k, 2k, 5k, 20k, 50k} x P1·P2 x 3회, 첫 조건만 r0). M1부터 변형 이름은 라벨용 `mass`다. 개발 실행은 `run-dev.ps1 -Extra "-LabStage=M1"`.
- `summarize.py`는 `py -3`(3.9)로 돌린다. pandas 없이 표준 라이브러리만 쓴다.

## Unreal MCP를 HTTP로 직접 부르기

이 세션처럼 MCP 클라이언트가 붙어 있지 않을 때는 에디터를 `-nullrhi`로 띄운 뒤 JSON-RPC로 부른다. `initialize` 응답 헤더의 `Mcp-Session-Id`를 이후 요청에 붙인다.

```bash
curl -s -X POST http://127.0.0.1:8001/mcp -H "Content-Type: application/json" -H "Accept: application/json, text/event-stream" -H "Mcp-Session-Id: <id>" -d '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"list_toolsets","arguments":{}}}'
```
