# 현재 상태

마지막 갱신: 2026-11-07

## 단계

**M0 기준선.** 시나리오(오브·스포너·카메라·변형)와 측정 파이프라인(러너·Test 패키지·run-bench·summarize)이 돈다. 남은 것은 이펙트를 켠 채 E0를 다시 재는 일과 마무리다. 각 항목의 완료 조건은 [plan.md](plan.md) "M0"이다.

## 다음 할 일

- [ ] 1. [pc-specs.md](Reference/pc-specs.md) 나머지 열. CSV 메타데이터로 확인한 것: Ryzen 9 5950X, RTX 4090(드라이버 621.14), D3D12 SM6. RAM·해상도·`sg.*`가 남았다.
- [x] 2. 엔진 프로젝트, 모듈, 플러그인. `EngineAssociation` = `{DEC43C63-4155-0388-B4B3-6B830A2347BA}`.
- [x] 3. `build`, `run-dev`, `package`(Test, 첫 빌드 26분), `run-bench`. 시험 측정 1회로 패키지 → CSV → `runs.csv` 경로 확인.
- [x] 4. 미니언은 옮겼고(`Scripts/migrate-assets.py`) 늘어짐은 메시 콜리전을 꺼서 막았다. AnimBP `ABP_LabMinion`(Blend Poses by `ELabMinionState`: 걷기 루프 / 피격 / 죽기)은 2026-11-07 사용자가 에디터에서 그래프를 채웠다. 실행에서 `lab_assets animbp=1` 확인.
- [x] 5. 맵 `L_Bench`(Basic 템플릿, 바닥 540m), 오브(테슬라 코일 타워, 보라 범위 8m, 가중 피해, 번개가 범위 안 대상을 향함), 스포너, 카메라 P1 오브 뒤 회전 / P2 탑뷰 100m(P3는 ADR-0003으로 뺐다). 조명은 한낮과 해 질 녘 사이로 고정(`Scripts/create-bench-map.py`).
- [x] 6. 변형 `full`/`static`/`budget`/`uro`/`pawn`(CharacterMovement 틱 끔). `static`은 `Scripts/make-static-minion.py`가 만든 `/Game/Lab/Derived/SM_LabMinion`.
- [x] 7. 벤치마크 러너(`-LabBench`), `summarize.py`(표준 라이브러리만).
- [x] 8. 시나리오 고정값 확정(2026-11-07): R 500m, 피해 범위 8m, 저격 2초 5명, 밴드 30/80/200m, 2560x1440 Epic, 시드 20261106. 표는 [measurement.md](Guides/measurement.md) "확정값". `LODMaxCount`만 E0 뒤에 정한다.
- [x] 8b. 오브 이펙트를 측정 실행에서도 늘 그린다(2026-11-07 사용자 결정). 저격은 구체에서 대상까지 곧은 보라 선 하나(`/Game/Lab/FX/NS_LabBolt`, 만드는 법은 `Scripts/create-bolt-fx.md`). 번개 표현과 피격 이펙트는 시험 뒤 뺐다.
- [ ] 9. E0 다시 재기: 아래 표는 측정 실행에서 이펙트를 끈 빌드의 값이라 Test 패키지를 새로 만들어 전 조건(`Scripts/run-e0.ps1 -Tag fx`, pawn 포함 88회, 약 2시간)을 다시 잰다. 첫 측정 CSV는 `Saved/Profiling/CSV/superseded-nofx/`로 옮겼고, 새 값이 나오면 `Measurements/M0/`의 두 CSV를 새로 만든다(첫 값은 git 이력에 남는다). 그 뒤 대표 스크린샷(P1, N=5k), `plot.py`, `LODMaxCount` 결정.
- [ ] 10. README 결과 초판, 태그 `M0`, Worklog. ADR-0002 초안은 썼고 글 대기열(00, 01)은 이미 있다.

## M1 진행(M0 태그 전에 시작, 2026-11-07)

- [x] `-LabStage=M1`: 같은 스포너·오브에서 미니언을 Mass 엔티티로 만든다. EntityConfig는 실행 시 C++([ADR-0005](Decisions/0005-entity-config-in-code.md)). 프로세서 `ULabApproachProcessor`(엔진 조향 앞), `ULabOrbContactProcessor`, `ULabOrbSnipeProcessor`, `ULabMinionStateProcessor`(지연 파괴 + 재생성). 표현은 `UMassMovableVisualizationTrait`의 ISM 하나(모든 LOD), `Config/DefaultMass.ini`로 LOD·표현 프로세서를 켠다.
- [x] 로직 일치: N=300 개발 실행 25초에서 처치·재생성·저격 수가 M0 static과 같다. 프레임이 100ms를 넘으면 엔진 이동이 시간을 0.1초로 잘라 느려진다([engine-notes.md](Reference/engine-notes.md) 11절).
- [x] GPU 병목: 움직이는 ISM 2,000개가 Lumen 장면에 들어가 GPU 29ms였다. 모든 단계에서 미니언 표현을 Lumen 장면에서 빼고(`LabExcludeFromLumen`) Mass ISM 그림자를 켰다(2026-11-07 사용자 결정, measurement.md 확정값). 시험 수치는 engine-notes.md 11절. E0 재측정은 이 조건과 화면 안 구간별 저격을 넣은 패키지(96f8202)로 한다.
- [ ] `ULabDamageApplyProcessor`(피해 이벤트 엔티티)는 E1 (d)에서 더한다. 지금 저격은 직접 태그를 붙인다.
- [x] `Scripts/run-e1.ps1`: E1 (a) mass N {1k, 2k, 5k, 20k, 50k} x P1·P2 x 3회(31회, 약 45분). 50k 시험(Test 패키지, 비교용 아님): 프레임 p50 40ms, Game 34ms라 이동 시간 상한(100ms) 안이다.
- [ ] E1 (a) 측정, (b) `mass.FullyParallel`, (c) 회피, (d) 피해 전달.

## 명령

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 2000 -Preset P1 -Duration 13 -ShotAt 10
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 2000 -Preset P1 -Duration 13 -ShotAt 10 -Extra "-LabStage=M1"
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-bench.ps1 -Milestone M0 -N 1000 -Preset P1 -Variant full
py -3 Scripts/summarize.py --milestone M0
```

나머지(에셋 이전, 정적 메시, 맵 생성, MCP)는 [commands.md](Guides/commands.md).

## 측정 결과

E0 첫 측정(M0, Test 패키지, 2560x1440 Epic, 2026-11-07, 측정 실행에서 저격 이펙트를 끈 빌드, `Measurements/M0/summary.csv`). 다시 재면 바뀐다. Game Thread p50(ms)이 병목이고 Render·GPU는 N=5k에서도 16ms 안팎이다.

| 변형 | P1 N250 | 500 | 1k | 2k | 5k | N@16.7ms(P1, 로그 보간) |
| --- | --- | --- | --- | --- | --- | --- |
| full | 5.3 | 9.8 | 18.2 | 38.0 | 150.0 | 약 880 |
| static | 3.6 | 6.7 | 12.5 | 24.6 | 84.4 | 약 1,270 |
| budget(v2) | | | 15.9 | 29.0 | 109.9 | 약 1,050 |
| uro | | | 16.7 | 34.1 | 124.1 | 약 1,000 |
| pawn(v2) | | | 13.4 | 26.5 | 69.3 | 약 1,190 |

P2는 full·static 모두 P1보다 0.2\~1.7ms 낮다(N=5k full만 18ms 차이). 첫 묶음의 budget 9회는 할당기가 꺼져 있어 무효(`Saved/Profiling/CSV/invalid/`), v2로 다시 쟀다.

## 막힌 것

- 없음.

## 사용자에게 요청한 일

- [ADR-0001](Decisions/0001-test-package-for-measurement.md) 승인 확인.
- [ADR-0002](Decisions/0002-freeze-measurement-infra.md) 승인(측정 인프라 동결).
- 밤에 직접 시작: `powershell -ExecutionPolicy Bypass -File Scripts/run-e0.ps1 -Tag fx` 뒤 `powershell -ExecutionPolicy Bypass -File Scripts/run-e1.ps1`(Test 패키지 96f8202, 합쳐 약 2시간 45분).
