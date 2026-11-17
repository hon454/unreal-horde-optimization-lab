# 스킬 작업 기록

이 레포의 노하우를 Claude Code 스킬 두 개(측정 절차, 군중 최적화)로 만드는 작업의 기록이다. 결정은 [autonomous-decisions.md](autonomous-decisions.md) "사용자 확인(2026-11-18, 스킬)"에 있고, 계획안은 [Skills-plan.md](Skills-plan.md)다.

## 2026-11-18 스킬 계획안 작성

- 조사: 성공. 결정 절, [V2-w14-plan.md](V2-w14-plan.md) 13절, `mass-crowd-tips.md` 31항목, `measurement.md`(벤치마크 조건, 지표, 차이 판정, 총괄 묶음 규약, 묶음 판정), `troubleshooting.md`, `glossary.md` 1\~2절, `v2-plan.md` 사다리, `Measurements/Full4/summary.csv`, `summarize.py`의 CSV 읽기와 percentile, `equivalence-check.py` 머리 주석, `engine-notes.md` 2·6·7절을 읽었다. 다음: 작성.
- 작성: 성공. [Skills-plan.md](Skills-plan.md)에 두 스킬의 파일 구성, description 초안, 본문 개요, 참조 파일, 비교 스크립트 명세, 출처 표, 넣지 않는 것과 걷어 낼 이름, 완료 확인 질문 13개, 사용자 판단 아홉 항목을 썼다. 사다리 참고값과 기법별 기여 일곱은 `Measurements/Full4/summary.csv`에서 다시 셈해 맞췄다. 스크립트 검사(7.3절)의 기대값은 같은 파일의 `V0-s8`, `V0-a8far`, `V0-a8fx`, `V0-s8-aa`다. `Saved/Profiling/CSV/`에 `Full4-*` 196개가 있음을 확인했다. 다음: 커밋.
- 남긴 것: `AGENTS.md` "문서 지도"의 Worklog 목록에 두 파일을 더하는 일은 코디네이터 몫이다(이 세션의 파일 소유 밖).
