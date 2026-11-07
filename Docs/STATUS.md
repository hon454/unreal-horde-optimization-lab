# 현재 상태

마지막 갱신: 2026-11-07

## 단계

**M0 기준선.** 시나리오(오브·스포너·카메라·변형)와 측정 파이프라인(러너·Test 패키지·run-bench·summarize)이 돈다. 남은 것은 E0 측정이다. 각 항목의 완료 조건은 [plan.md](plan.md) "M0"이다.

## 다음 할 일

- [ ] 1. [pc-specs.md](Reference/pc-specs.md) 나머지 열. CSV 메타데이터로 확인한 것: Ryzen 9 5950X, RTX 4090(드라이버 621.14), D3D12 SM6. RAM·해상도·`sg.*`가 남았다.
- [x] 2. 엔진 프로젝트, 모듈, 플러그인. `EngineAssociation` = `{DEC43C63-4155-0388-B4B3-6B830A2347BA}`.
- [x] 3. `build`, `run-dev`, `package`(Test, 첫 빌드 26분), `run-bench`. 시험 측정 1회로 패키지 → CSV → `runs.csv` 경로 확인.
- [x] 4. 미니언은 옮겼고(`Scripts/migrate-assets.py`) 늘어짐은 메시 콜리전을 꺼서 막았다. AnimBP `ABP_LabMinion`(Blend Poses by `ELabMinionState`: 걷기 루프 / 피격 / 죽기)은 2026-11-07 사용자가 에디터에서 그래프를 채웠다. 실행에서 `lab_assets animbp=1` 확인.
- [x] 5. 맵 `L_Bench`(Basic 템플릿, 바닥 540m), 오브(테슬라 코일 타워, 보라 범위 8m, 가중 피해, 번개가 범위 안 대상을 향함), 스포너, 카메라 P1 오브 뒤 회전 / P2 탑뷰 100m(P3는 ADR-0003으로 뺐다). 조명은 한낮과 해 질 녘 사이로 고정(`Scripts/create-bench-map.py`).
- [x] 6. 변형 `full`/`static`/`budget`/`uro`. `static`은 `Scripts/make-static-minion.py`가 만든 `/Game/Lab/Derived/SM_LabMinion`.
- [x] 7. 벤치마크 러너(`-LabBench`), `summarize.py`(표준 라이브러리만).
- [x] 8. 시나리오 고정값 확정(2026-11-07): R 500m, 피해 범위 8m, 저격 2초 5명, 밴드 30/80/200m, 2560x1440 Epic, 시드 20261106. 표는 [measurement.md](Guides/measurement.md) "확정값". `LODMaxCount`만 E0 뒤에 정한다.
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

- 없음.

## 사용자에게 요청한 일

- [ADR-0001](Decisions/0001-test-package-for-measurement.md) 승인 확인.
