# 현재 상태

마지막 갱신: 2026-11-07

## 단계

**M0 기준선.** 시나리오(오브·스포너·카메라·변형)와 측정 파이프라인(러너·Test 패키지·run-bench·summarize)이 돈다. 남은 것은 AnimBP, 고정값 확정, E0 측정이다. 각 항목의 완료 조건은 [plan.md](plan.md) "M0"이다.

## 다음 할 일

- [ ] 1. [pc-specs.md](Reference/pc-specs.md) 나머지 열. CSV 메타데이터로 확인한 것: Ryzen 9 5950X, RTX 4090(드라이버 621.14), D3D12 SM6. RAM·해상도·`sg.*`가 남았다.
- [x] 2. 엔진 프로젝트, 모듈, 플러그인. `EngineAssociation` = `{DEC43C63-4155-0388-B4B3-6B830A2347BA}`.
- [x] 3. `build`, `run-dev`, `package`(Test, 첫 빌드 26분), `run-bench`. 시험 측정 1회로 패키지 → CSV → `runs.csv` 경로 확인.
- [ ] 4. 미니언은 옮겼고(`Scripts/migrate-assets.py`) 늘어짐은 메시 콜리전을 꺼서 막았다. **AnimBP `ABP_LabMinion`이 없다**(사용자에게 요청한 일). 그때까지 `full`은 싱글 노드 임시 경로로 돈다.
- [x] 5. 맵 `L_Bench`(Basic 템플릿, 바닥 540m), 오브(테슬라 코일 타워, 보라 범위 6m, 가중 피해), 스포너, 카메라 P1 오브 뒤 회전 / P2 탑뷰 100m / P3 오브 뒤 고정.
- [x] 6. 변형 `full`/`static`/`budget`/`uro`. `static`은 `Scripts/make-static-minion.py`가 만든 `/Game/Lab/Derived/SM_LabMinion`.
- [x] 7. 벤치마크 러너(`-LabBench`), `summarize.py`(표준 라이브러리만).
- [ ] 8. 시나리오 고정값 실측 보고와 사용자 확정. 초안은 `Source/HordeOptLab/LabConfig.h` `FLabScenario`. **맵 반경 결정 대기**(250m 유지 vs 500m 추천).
- [ ] 9. E0 측정, 대표 스크린샷, `plot.py`.
- [ ] 10. ADR-0002 초안, README 결과 초판, 태그 `M0`, Worklog, 글 대기열.

## 명령

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 2000 -Preset P1 -Duration 13 -ShotAt 10
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-bench.ps1 -Milestone M0 -N 1000 -Preset P1 -Variant full
py -3 Scripts/summarize.py --milestone M0
```

나머지(에셋 이전, 정적 메시, 맵 생성, MCP)는 [commands.md](Guides/commands.md).

## 측정 결과

진행 중인 묶음이 없다. 시험 1회(N=500 P1 full, 싱글 노드 애니, 1920x1080 창): Game p50 13.7ms, 프레임 p50 14.5ms. 비교에 쓰지 않는다.

## 막힌 것

- AnimBP: Unreal MCP 툴셋(`editor_toolset` BlueprintTools의 그래프 DSL)과 Python에 애님 그래프 편집이 없다(engine-notes.md 10절). 에디터 작업이 필요하다.

## 사용자에게 요청한 일

- **AnimBP 만들기**(약 5분): 에디터에서 `/Game/Lab/Anim/ABP_LabMinion` 생성. 스켈레톤 `Minion_Lane_Core_Skeleton`, 부모 클래스 `LabMinionAnimInstance`. AnimGraph에 `Blend Poses by ELabMinionState` 노드 하나, Active Enum Value에 변수 `State`, 핀 Walk = `NonCombat_JogFwd`(루프), Hit = `HitReact_Front`(루프 끔), Dead = `Death_A`(루프 끔), 블렌드 시간 0.15. 컴파일·저장. 스포너가 이 경로가 있으면 자동으로 쓴다.
- 맵 반경 결정: 250m 유지 또는 500m(추천, 밴드 비율 표는 채팅).
- GitHub 푸시 확인(로컬 커밋만 쌓여 있다).
- [ADR-0001](Decisions/0001-test-package-for-measurement.md) 승인 확인.
