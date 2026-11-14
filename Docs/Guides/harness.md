# 에이전트 하네스(Claude Code 훅)

[AGENTS.md](../../AGENTS.md)의 규칙 중 기계로 판정할 수 있는 것은 Claude Code 훅이 강제한다. 에이전트가 규칙을 기억하는지에 기대지 않고, 도구 호출 앞뒤에서 스크립트가 검사한다.

- 등록: [.claude/settings.json](../../.claude/settings.json)의 `hooks`
- 스크립트: [.claude/hooks/](../../.claude/hooks/)(Python, `py -3`로 실행)
- 공용 함수: `.claude/hooks/_lab.py`

## 훅 목록

| 훅 | 이벤트 | 강제하는 규칙 | 위반 시 |
| --- | --- | --- | --- |
| `commit_gate.py` | PreToolUse, `Bash`·`PowerShell`의 `git commit` | Conventional Commits 제목과 허용 type, 커밋 금지 경로, 이미지·에셋의 Git LFS, `.md`의 `\~` | 커밋을 막는다 |
| `bench_guard.py` | PreToolUse, `Bash`·`PowerShell` | 벤치마크 중 빌드·패키징·에디터·Development 실행·두 번째 벤치마크 금지, 빌드 동시 실행 금지 | 명령을 막는다 |
| `doc_check.py` | PostToolUse, `Edit`·`MultiEdit`·`Write` | `.md`의 `\~`, STATUS.md 10KB, 공개 문서의 묶음 이름·조건 라벨·일정 표현, `Scripts/*.ps1`의 UTF-8 BOM | 문제를 Claude에 돌려준다. BOM은 직접 붙인다 |
| `session_brief.py` | SessionStart | "세션을 시작하면" 1단계 | STATUS.md 발췌와 실행 중인 프로세스를 맥락에 넣는다 |

## 판정 기준

- 커밋 제목: `type(scope): summary`. type은 `feat`, `fix`, `docs`, `build`, `refactor`, `chore`, `bench`다. summary는 소문자나 숫자로 시작하고 마침표로 끝나지 않는다.
- 커밋 금지 경로: `Saved/`, `Binaries/`, `Intermediate/`, `DerivedDataCache/`, `Packaged/`, `Content/ParagonMinions/`.
- 커밋 관문은 스테이지된 파일과 같은 명령 줄의 `git add`가 더하는 파일을 함께 본다.
- 벤치마크 판정: `HordeOptLab-Win64-Test.exe` 프로세스가 있으면 벤치마크 중이다.
- 빌드 판정: `UnrealBuildTool.exe`나 `AutomationTool.exe` 프로세스가 있으면 빌드 중이다.
- 공개 문서: 루트 `README.md`와 `Posts/**/README.md`다. 묶음 이름은 `Measurements/`의 폴더 이름에서 읽는다. 코드 스팬, 코드 블록, 링크 대상은 검사하지 않는다.

## 우회

- 벤치마크 중에 일부러 명령을 돌리려면 명령에 `LAB_ALLOW_CONCURRENT=1`을 넣는다.
- 다른 훅은 우회하지 않는다. 판정이 틀렸으면 스크립트를 고친다.

## 직접 돌려 보기

훅은 stdin으로 JSON 하나를 받는다. 종료 코드 0은 통과, 2는 차단이다.

```bash
echo '{"tool_input":{"command":"git commit -m \"Update stuff\""}}' | py -3 .claude/hooks/commit_gate.py
```

```bash
echo '{"source":"startup"}' | py -3 .claude/hooks/session_brief.py
```

## 한계

- 커밋 관문은 `-F 파일`로 준 메시지의 제목을 검사하지 않는다.
- 문서 검사는 편집 뒤에 돈다. 편집을 되돌리지 않고 Claude가 고치게 한다.
- 프로세스 판정은 Windows `tasklist`를 쓴다.
