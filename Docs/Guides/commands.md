# 명령과 측정 절차

STATUS.md "명령"에는 매번 쓰는 것만 둔다. 여기에는 스크립트 사양, 인자, 측정 절차를 둔다. M0에서 스크립트를 만들면서 실제 명령으로 채운다. 그때까지는 사양이다.

## 스크립트 사양(M0 산출물)

모든 스크립트는 `Scripts/common.ps1`을 불러 엔진 경로를 얻는다. 엔진 경로는 저장소에 적지 않고 `.uproject`의 `EngineAssociation`을 레지스트리(`HKCU\Software\Epic Games\Unreal Engine\Builds`)에서 찾는다(DS 실험실 방식).

| 스크립트 | 하는 일 | 거부 조건 |
| --- | --- | --- |
| `build.ps1` | `HordeOptLabEditor Win64 Development` 빌드 | `UnrealEditor` 또는 `HordeOptLab` 프로세스가 떠 있으면 |
| `run-dev.ps1 -N -Preset -Seed [-Variant] [-Small]` | Development 에디터 빌드를 `-game`으로 실행. 개발·동작 확인·오버레이 스크린샷용. 수치는 쓰지 않는다 | |
| `package.ps1 [-Config Test]` | `RunUAT BuildCookRun` → `Packaged/<Config>/`. 끝나면 소요 시간을 출력 | 다른 실행이 떠 있으면 |
| `run-bench.ps1 -Label <라벨> -N <수> -Preset <P1\|P2\|P3> [-Variant] [-Seed] [-Runs 3] [-Trace]` | Test 패키지 실행 파일을 `-LabBench` 인자로 띄우고 종료를 기다린다. 실행마다 `Saved/Profiling/CSV/<라벨>-rN.csv`를 확인하고, 없으면 실패 | 같은 라벨의 CSV가 있으면. 다른 실행이 떠 있으면 |
| `summarize.py --milestone M<N>` | `Saved/Profiling/CSV/`의 라벨을 읽어 `Measurements/M<N>/runs.csv`에 행을 더하고 `summary.csv`를 다시 쓴다 | |
| `plot.py` | `Measurements/*/summary.csv`로 `Docs/figures/`의 그래프를 다시 그린다: 60fps 최대 N(대표), 고정 조건 Game/Render/GPU 스택, N 스케일링, 밴드 분포(M2+), 예산 사용(M3+) | |
| `capture-video.ps1` | 화면 녹화. 찍기 전 승인 필요(AGENTS.md) | 측정 실행이 떠 있으면 |

## 벤치마크 러너 인자(게임 측)

`-LabBench` 가 있으면 다음 순서로 동작하고 끝나면 종료한다.

1. 렌더 CVar 고정(measurement.md "렌더 고정")과 `-LabN= -LabPreset= -LabSeed= -LabVariant=` 적용. 모든 값을 로그 `lab_config`에 한 줄로 남긴다.
2. 스포너가 정상 상태 분포로 N개 배치. 카메라를 프리셋에 놓는다.
3. 30초 워밍업. 그 사이 N 유지 확인(사망·재생성이 동작하는지).
4. `csvprofile start`(압축 끔, 라벨을 파일 이름으로). 60초.
5. `csvprofile stop`. 프레임 수, N, 전환 수 등 요약을 로그 `lab_summary`에 남긴다. 조건이 깨졌으면(N 불일치 등) 종료 코드 1.
6. `-Trace`면 2\~5 동안 `-trace=cpu,gpu,frame,counters,mass`로 트레이스를 남긴다. 트레이스 실행의 수치는 비교에 쓰지 않는다(이벤트 기록 비용).

## 측정 절차

1. 측정 전에 잴 조건, 라벨, 걸리는 시간을 채팅에 알리고 답을 받는다.
2. `package.ps1`로 그 커밋의 Test 패키지를 만든다. 패키지 폴더에 커밋 해시를 남긴다.
3. 묶음의 첫 실행(`-r0`)은 버린다. 그 뒤 조건별 3회.
4. 실행마다 로그의 `lab_config`와 `lab_summary`를 확인한다. N이 유지되지 않았거나 프레임 수가 비정상이면 그 실행을 버리고 라벨을 바꿔 다시 잰다.
5. `summarize.py` → `runs.csv`, `summary.csv`. 차이 판정은 measurement.md.
6. STATUS.md "측정 결과"에 묶음을 적는다. 마일스톤이 끝나면 Worklog로 옮긴다.

## 스크린샷·영상 실행

- 오버레이 스크린샷(`stat unit`, `a.Budget.Debug.Enabled 1`, 밴드 색 디버그 뷰)은 `run-dev.ps1`로 Development에서 찍는다. 그 실행의 수치는 쓰지 않는다.
- Test 패키지 스크린샷은 `stat unit`만 가능하다.
- 영상은 수치 실행에서 찍지 않는다. 라벨 `visualN`으로 따로 돌린다.

## Unreal MCP

- 에디터 콘솔: `ModelContextProtocol.GenerateClientConfig ClaudeCode` → 프로젝트 루트에 `.mcp.json`(gitignore). `ModelContextProtocol.StartServer`로 시작.
- 툴 호출은 게임 스레드에서 직렬. 겹쳐 부르지 않는다. 대량 변경 전후 저장. 새 `UFUNCTION`은 에디터 재시작.
- Claude Code는 `claude --add-dir ..\unreal-horde-references`로 시작한다(`.claude/settings.json`의 `additionalDirectories`에도 있다).
