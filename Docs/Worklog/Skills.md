# 스킬 작업 기록

이 레포의 노하우를 Claude Code 스킬 두 개(측정 절차, 군중 최적화)로 만드는 작업의 기록이다. 결정은 [autonomous-decisions.md](autonomous-decisions.md) "사용자 확인(2026-11-18, 스킬)"에 있고, 계획안은 [Skills-plan.md](Skills-plan.md)다.

## 2026-11-18 스킬 계획안 작성

- 조사: 성공. 결정 절, [V2-w14-plan.md](V2-w14-plan.md) 13절, `mass-crowd-tips.md` 31항목, `measurement.md`(벤치마크 조건, 지표, 차이 판정, 총괄 묶음 규약, 묶음 판정), `troubleshooting.md`, `glossary.md` 1\~2절, `v2-plan.md` 사다리, `Measurements/Full4/summary.csv`, `summarize.py`의 CSV 읽기와 percentile, `equivalence-check.py` 머리 주석, `engine-notes.md` 2·6·7절을 읽었다. 다음: 작성.
- 작성: 성공. [Skills-plan.md](Skills-plan.md)에 두 스킬의 파일 구성, description 초안, 본문 개요, 참조 파일, 비교 스크립트 명세, 출처 표, 넣지 않는 것과 걷어 낼 이름, 완료 확인 질문 13개, 사용자 판단 아홉 항목을 썼다. 사다리 참고값과 기법별 기여 일곱은 `Measurements/Full4/summary.csv`에서 다시 셈해 맞췄다. 스크립트 검사(7.3절)의 기대값은 같은 파일의 `V0-s8`, `V0-a8far`, `V0-a8fx`, `V0-s8-aa`다. `Saved/Profiling/CSV/`에 `Full4-*` 196개가 있음을 확인했다. 다음: 커밋.
- 커밋: 성공(508e04f). 다음: 사용자가 계획안 8절의 항목을 정한다.
- 남긴 것: `AGENTS.md` "문서 지도"의 Worklog 목록에 두 파일을 더하는 일은 코디네이터 몫이다(이 세션의 파일 소유 밖).

## 군중 스킬

2026-11-18 작업 세션이 `.claude/skills/unreal-mass-crowd-optimization/`을 만든다(계획안 1, 4\~8절).

- 조사: 성공. 계획안 전체, 결정 절, `mass-crowd-tips.md` 31항목(작업 트리의 "밀어내기" 표기), `engine-notes.md` 1\~5·13·34\~37·40·47\~49절, `glossary.md` 1절과 조건 식별자 표, `Measurements/Full4/summary.csv`의 사다리 여덟 행과 기법별 기여 행을 읽었다. 계획안 4.4·4.7절의 수치가 `summary.csv`와 맞는다(최종 구성 14.231, 범위 14.035\~14.426). 용어 "분리 프로세서", "직접 짠 분리"는 사용자 결정(2026-11-18)대로 "밀어내기 프로세서", "자체 밀어내기"로 쓴다. 다음: 작성.
- 작성: 성공. `SKILL.md`(130줄: 범위, 말, 절차 7단계, 병목별 다음 행동 표, 적용 순서 여덟 칸, 함정 열, 참조 안내)와 `references/` 여섯(`ladder.md` 114줄, `actors.md` 193줄, `instances.md` 260줄, `processors.md` 232줄, `troubleshooting.md` 43줄, `results.md` 82줄)을 썼다. 코드 예시는 역할 이름과 "예시 이름" 주석으로 바꾸고 설정값은 값으로 풀었다. CDO 자동 등록 끄기 코드는 `HordeOptLab.cpp` 23\~28행과 맞췄다. 래그돌 행은 `troubleshooting.md` 11행대로 "Physics Type을 Default로"만 적었다. `results.md`의 사다리 여덟 행, 기법별 기여 일곱, 구별되지 않은 열 이름, 액터 군중 개체 수, 엔티티 풀 긴 실행 값은 `Full4/summary.csv`와 `mass-crowd-tips.md`에서 옮겼다. 다음: 내용 검사.
- 내용 검사(계획안 7.2절): 성공. 6.2절 정규식 0건. 더 넓힌 검사(`Snipe`, `Minion`, 레포 경로, `Fab`, 쓰지 않는 말)는 `Engine/Source/...` 경로와 "직접 짠 프로세서"만 걸렸고 레포 이름이 아니다. description 722자, `~` 없음. `SKILL.md`가 가리키는 참조 여섯이 모두 있다. 수치가 있는 파일마다 머리에 `results.md` 조건 표시가 있다. `check-tilde.py`는 git이 추적하는 파일만 보므로 일곱 파일을 인자로 돌려 0건을 확인했다. 다음: 커밋.
- 커밋: 성공(94d35ce). 스킬 폴더의 일곱 파일만 넣었다. 이 Worklog는 코디네이터가 커밋한다. 푸시하지 않았다. 다음: 코디네이터가 7.1절 질문을 새 세션에서 던진다.
- 용어 확인: 성공(436b884). 코디네이터의 용어 변경 알림을 받고 스킬 폴더에서 "분리"를 찾았다. 본문은 이미 새 말이었다. 계획안 1.3절 표에서 온 예시 이름 `FSeparationSolver`만 `FPushApartSolver`로 바꿔 `UPushApartProcessor`와 맞췄다.

## 측정 스킬

2026-11-18 작업 세션이 `.claude/skills/unreal-perf-measurement/`를 만든다(계획안 1\~3, 5\~8절).

- 조사: 성공. 계획안 전체, 결정 절, `measurement.md`, `mass-crowd-tips.md` "프로파일링할 때" 아홉 항목, `troubleshooting.md`, `engine-notes.md` 6·7·30·50절, `glossary.md` 1·2절, `summarize.py`의 `read_profile`·`column`·`percentile`·`bottleneck`·`summarize_run`, `batch_rules.py` 느린 실행 규칙, `equivalence-check.py` 머리 주석을 읽었다. 용어 "분리"는 사용자 결정(2026-11-18)대로 "밀어내기"로 쓴다. 검사 기대값: `Full4` `V0-a8far`는 `runs_used`가 r2 r3 r7이다(r1은 느린 실행으로 대체). 다음: 작성.
- 작성: 성공. `SKILL.md`(절차 12단계, 비교 스크립트 사용법, 참조 안내, 예시 수치의 조건), `references/` 다섯(`protocol.md`, `judging.md`, `equivalence.md`, `tracing.md`, `troubleshooting.md`), `scripts/compare_csv.py`(계획안 3절 명세)를 썼다. 예시 수치에는 "(참고값)" 표시를 달고 `SKILL.md` "예시 수치의 조건" 표를 가리켰다. 1만이 아닌 예시(2만, 탑뷰)는 표시 옆에 조건을 더 적었다. 다음: 스크립트 검사.
- 스크립트 검사(계획안 7.3절): 성공. `Saved/Profiling/CSV/Full4-*`를 `--cond` 글롭과 스크래치 폴더 복사본(폴더 입력) 두 방식으로 돌렸다.
  - 기준 `V0-s8` r1\~r3, 조건 `V0-a8far` r2·r3·r7(`summary.csv`의 `runs_used`), `V0-a8fx` r1\~r3, A/A r11\~r13.
  - 조건 표의 프레임 p95 중앙값이 `summary.csv`와 소수 셋째 자리까지 같다: 14.231, 17.767, 13.922, 14.310. 프레임 p50, p99, 세 스레드 p50, 범위도 같다.
  - 판정: `V0-a8far` +3.536ms, 문턱 0.391, "차이 있음(느려짐)". `V0-a8fx` −0.309ms, 문턱 0.391, "구별되지 않음". A/A +0.079ms, 통과.
  - `V0-a8far` r1을 넣으면 그 실행에 "느린 실행 후보"가 붙는다. 러너가 대체한 실행과 같다.
  - `--skip-frames 100`은 r1의 프레임 수를 13,915에서 13,815로 줄인다. `--skip-seconds 10`은 799프레임을 버리고 측정 창 170.0초를 적는다.
  - 빈 폴더, 없는 기준, 없는 폴더, `--skip-*` 둘 다는 종료 코드 2다. `--baseline` 없음은 argparse가 2로 끝낸다.
  - 0.5ms 규칙과 5회 규칙, 실행 부족은 합성 값으로 `judge()`를 불러 확인했다.
  - `.csv.bin`이 있는 폴더는 주의 줄을 낸다. 없는 `--extra-col`은 빈칸과 주의 줄이다.
  - 다음: 내용 검사.
- 내용 검사(계획안 7.2절): 성공. 6.2절 정규식 0건. 더 넓힌 검사(`Lab` 낱말, 계획 식별자, 레포 경로, 태그, "분리", 쓰지 않는 말, "단계 N")도 0건이다. frontmatter에 `name`과 `description`이 있고 description은 683자, `~`가 없다. `SKILL.md`가 가리키는 참조 다섯과 스크립트가 있다. 군중 스킬의 `references/processors.md`도 있다. 수치가 있는 참조 파일마다 머리 줄이 `SKILL.md` "예시 수치의 조건"을 가리키고 수치 옆에 "(참고값)"이 있다. `check-tilde.py`는 인자가 없으면 git이 추적하는 `*.md`만 본다. 그래서 여섯 파일을 인자로 돌려 0건을 확인했다. 커밋한 뒤에는 인자 없이도 이 파일들을 본다. 분량: `SKILL.md` 121줄, 참조 합 448줄, 스크립트 391줄. 다음: 커밋.
- 커밋: 성공(f5d69e1). `.claude/skills/unreal-perf-measurement/` 일곱 파일만 들어갔다(`git diff --cached --stat`, 경로를 준 커밋). 이 절(`Skills.md`)은 커밋하지 않았다. 다음: 코디네이터가 `Skills.md`를 커밋하고 푸시한다. 두 스킬이 모두 끝나면 계획안 7.1절의 질문을 던진다.
