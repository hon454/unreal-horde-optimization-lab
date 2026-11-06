# 현재 상태

마지막 갱신: 2026-11-06

## 단계

**M0 기준선.** 저장소에 코드와 엔진 프로젝트가 아직 없다. 작업 목록과 각 항목의 완료 조건은 [plan.md](plan.md) "M0"이고, 아래는 그 순서와 진행 상황만 적는다.

## 다음 할 일

- [ ] 1. 엔진 태그를 [pc-specs.md](Reference/pc-specs.md)에 적는다. 소스 빌드 `G:\Epic Games\UE_5.8.4`, `Build.version` 5.8.4로 확인했다. CPU·GPU 등 나머지 열이 남았다.
- [x] 2. `HordeOptLab.uproject`, 모듈, `Config/`, 플러그인. 에디터 빌드 성공. `EngineAssociation`은 레지스트리 GUID `{DEC43C63-4155-0388-B4B3-6B830A2347BA}`(같은 경로의 GUID가 둘 있어 앞의 것).
- [ ] 3. `Scripts/` 실행·패키지·벤치 스크립트([commands.md](Guides/commands.md) 사양). `common.ps1`, `build.ps1`은 있다.
- [ ] 4. 근접 미니언은 옮겼다(`Scripts/migrate-minions.py`, 메시·스켈레톤·피직스 에셋·걷기/피격/죽기 애니, 소스 에디터에서 로드 확인). 남은 것: 늘어짐 대처. 피직스 에셋 바디 타입은 Python으로 못 바꾼다(engine-notes.md 9절). 메시 콜리전을 꺼서 바디를 만들지 않는 코드 쪽 대처를 먼저 시도한다.
- [ ] 5. 맵, 오브, 스포너, 카메라 프리셋.
- [ ] 6. 미니언 변형과 `-LabVariant=`.
- [ ] 7. 벤치마크 러너와 `summarize.py`.
- [ ] 8. 거리 밴드·시나리오 고정값 실측 보고와 사용자 확정.
- [ ] 9. E0 측정, 대표 스크린샷, `plot.py`.
- [ ] 10. ADR-0002 초안, README 결과 초판, 태그 `M0`, Worklog, 글 대기열.

## 명령

M0에서 스크립트를 만들면 채운다. 그때까지는 [commands.md](Guides/commands.md)의 사양을 따른다.

## 측정 결과

진행 중인 묶음이 없다.

## 막힌 것

- 없음.

## 사용자에게 요청한 일

- PC에 Python 3와 `matplotlib`, `pandas` 설치(`Scripts/summarize.py`, `plot.py`). 2026-11-06 확인 때 `python`이 정상 응답하지 않았다.
- [ADR-0001](Decisions/0001-test-package-for-measurement.md) 승인 확인(채팅에서 결정했고 문서로 옮겼다).
