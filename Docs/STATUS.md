# 현재 상태

마지막 갱신: 2026-11-07

## 단계

**M0 완료(태그 `M0`, 2026-11-08).** 측정 인프라 동결([ADR-0002](Decisions/0002-freeze-measurement-infra.md)). **M1 진행 중**: E1 (a) 끝, (b)\~(d) 남음. 각 항목의 완료 조건은 [plan.md](plan.md) "M0"이다.

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
- [x] 9. E0 다시 재기(2026-11-08, 아래 표), `plot.py`, 대표 스크린샷 `Measurements/M0/shots/full-N5000-P1.png`.
- [x] 9b. 문서를 코드에 맞췄다(plan.md M0 오브·M1 프로세서·측정 N, measurement.md 시나리오, ADR-0002 결과 절).
- [x] 10. README 결과 초판, 대표 스크린샷, Worklog, ADR-0002 승인, 태그 `M0`.

## M1 진행(M0 태그 전에 시작, 2026-11-07)

- [x] `-LabStage=M1`: 같은 스포너·오브에서 미니언을 Mass 엔티티로 만든다. EntityConfig는 실행 시 C++([ADR-0005](Decisions/0005-entity-config-in-code.md)). 프로세서 `ULabApproachProcessor`(엔진 조향 앞), `ULabOrbContactProcessor`, `ULabOrbSnipeProcessor`, `ULabMinionStateProcessor`(지연 파괴 + 재생성). 표현은 `UMassMovableVisualizationTrait`의 ISM 하나(모든 LOD), `Config/DefaultMass.ini`로 LOD·표현 프로세서를 켠다.
- [x] 로직 일치: N=300 개발 실행 25초에서 처치·재생성·저격 수가 M0 static과 같다. 프레임이 100ms를 넘으면 엔진 이동이 시간을 0.1초로 잘라 느려진다([engine-notes.md](Reference/engine-notes.md) 11절).
- [x] GPU 병목: 움직이는 ISM 2,000개가 Lumen 장면에 들어가 GPU 29ms였다. 모든 단계에서 미니언 표현을 Lumen 장면에서 빼고(`LabExcludeFromLumen`) Mass ISM 그림자를 켰다(2026-11-07 사용자 결정, measurement.md 확정값). 시험 수치는 engine-notes.md 11절. E0 재측정은 이 조건과 화면 안 구간별 저격을 넣은 패키지(96f8202)로 한다.
- [ ] `ULabDamageApplyProcessor`(피해 이벤트 엔티티)는 E1 (d)에서 더한다. 지금 저격은 직접 태그를 붙인다.
- [x] `Scripts/run-e1.ps1`: E1 (a) mass N {1k, 2k, 5k, 20k, 50k} x P1·P2 x 3회(31회, 약 45분). 50k 시험(Test 패키지, 비교용 아님): 프레임 p50 40ms, Game 34ms라 이동 시간 상한(100ms) 안이다.
- [x] E1 (a) 측정(2026-11-08, 아래 표).
- [x] E1 (b)(c)(d) 측정(2026-11-08, P1, 프레임 p50 ms, `Measurements/M1/summary.csv`·`runs.csv`).
  - (b) 병렬 처리 끔(`massst`): 5k 8.1 / 20k 16.1 / 50k 36.6, 켬(`mass`) 8.0 / 16.0 / 39.2. 차이가 없다. M1의 이득은 멀티스레드가 아니라 데이터 배치에서 온다.
  - (c) 미니언끼리 비켜 가기 켬(`massav`): 5k 13.2 / 20k 57.7 / 50k 202.4. 일이 워커 스레드에서 돌아 Game Thread 시간에는 안 보이고(`EventWait/PrePhysics` 20k 46ms) 프레임이 3.6배 길어진다. 50k는 프레임이 100ms를 넘어 이동 시간 상한으로 시나리오가 달라졌다(재생성 8,489 대 18,365) → 참고.
  - (d) 피해 적용 방식(20k, 초당 1천·1만 비치명 피해): 바로 깎기 15.5 / 15.8, 이벤트로 모아 깎기 16.0 / 15.7. 구별되지 않는다.
- [x] 결정(2026-11-08): M2\~M5는 비켜 가기를 끈다(M4에서 가까운 개체만 시험). N@16.7ms 판정에 프레임 시간을 넣는다.
- [ ] 태그 `M1`, Worklog M1, 글 대기열(02 이미 있음).

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

Test 패키지 96f8202(M0, M1 (a))·ecbef15(M1 (b)\~(d)), 2560x1440 Epic, P1, 프레임 p50(ms). N@16.7ms는 프레임·Game·Render·GPU p50 최댓값 기준 로그 보간(2026-11-08부터 프레임 포함, `Scripts/plot.py`). 원자료는 `Measurements/M0/`, `Measurements/M1/`.

| 단계·변형 | 1k | 2k | 5k | 20k | 50k | N@16.7ms |
| --- | --- | --- | --- | --- | --- | --- |
| M0 full | 19.5 | 40.6 | 156.9 | | | 813 |
| M0 static | 13.1 | 27.0 | 104.0 | | | 1,196 |
| M0 budget | 17.4 | 32.3 | 118.3 | | | 1k 미만 |
| M0 uro | 18.6 | 37.5 | 132.8 | | | 1k 미만 |
| M0 pawn | 14.3 | 28.3 | 75.3 | | | 1,128 |
| M1 mass | 5.9 | 6.5 | 8.0 | 16.0 | 39.2 | 20,577 |
| M1 massst | | | 8.1 | 16.1 | 36.6 | 20,530 |
| M1 massav | | | 13.2 | 57.7 | 202.4(참고) | 5,578 |

- E1 (a): N@16.7ms 1,196 → 20,577(약 17배). (b) 병렬 처리는 차이가 없다. (c) 비켜 가기는 20k에서 프레임 3.6배라 M2\~M5는 끈다. (d) 20k에서 바로 깎기 15.5 / 15.8, 이벤트로 모아 깎기 16.0 / 15.7(초당 1천 / 1만)로 구별되지 않는다.
- Game Thread p50만 보면 M0이 5\~8% 낮게 나온다. 이펙트를 끈 첫 측정은 git 이력(`a4e1a8c` 이전)에 있다.

## 막힌 것

- 없음.

## 사용자에게 요청한 일

- [ADR-0001](Decisions/0001-test-package-for-measurement.md) 승인 확인.
