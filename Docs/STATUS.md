# 현재 상태

마지막 갱신: 2026-11-09

## 단계

M0\~M3 완료(태그 `M0`\~`M3`). 다음은 M3b 정비 + 새 기준선([plan.md](plan.md) "M3b"). 끝낸 단계의 경위는 `Worklog/M<N>.md`.

## 다음 할 일

2026-11-09 점검 결정 1\~5·8·9는 끝났다([plan.md](plan.md) "M3b"·"M4", [measurement.md](Guides/measurement.md)). 결정 6(오브 중심 바닥 띠)·7(장애물 + 플로우 필드를 M5 뒤 첫 마일스톤으로, 퀄리티 앞)도 끝났다([backlog.md](backlog.md)). 아래 1\~4는 코디네이터 세션이 작업 세션에 나눠 맡긴다(AGENTS.md "여러 세션이 한 체크아웃을 쓴다").

- [ ] 1. M3b 코드 정비(plan.md "코드 정비" 핵심 5 + 두 줄 + 기본 프로세서 점검). 먼저 프로세서별 읽기·쓰기 표를 그린다.
- [x] 2. 측정 방법 구현(측정 세션, af4be11): summarize.py p95·`bottleneck`·`build` 열, M0\~M3 재요약(기존 값 차이 0), plot.py, `Scripts/spikes.py`, `Scripts/run-m3b.ps1 -DryRun`, pc-specs.md. 남은 것은 코드 세션에 달렸다: 변형 이름 ↔ `-LabStage` 매핑, 직렬 트레이스 인자(`-LabFullyParallel=0`), CSV 메타데이터 키(`LabBuild`·`LabCVars`·`LabDriver` 가정). 결과: p95 N@16.7ms P1은 M0 full 667 / M1 15,498 / M2 16,460 / M3 18,475 / M3 m2off 30,812(p50 정의 813 / 20,577 / 20,518 / 22,046 / 35,763). 7.0초 주기 스파이크는 M0·M1·M2 묶음(11-08 이전)에 있고 M3 묶음(11-09)에는 없다. `massav`의 2.0초 주기는 저격 간격과 같아 시나리오 효과다.
- [ ] 3. 2종: 시즈 에셋 이전(`migrate-assets.py siege`), AnimBP·Start Position 바인딩(MCP), VAT 베이크, 예약 발 저격, 화면 크기 중요도. 같은 에디터 작업으로 바닥 띠 머티리얼 `M_LabBandFloor` + `-LabBandFloor`(결정 6).
- [ ] 4. 묶음 1(E3b (8) 18회 → 대표값 결정 → 전수 + A/A + 트레이스), 묶음 2.

## 지금 유효한 조건

M2\~M5 공통이고 근거는 [measurement.md](Guides/measurement.md) "확정값"이다.

- 미니언끼리 비켜 가기 끔(M4에서 가까운 개체만 다시 시험). 미니언 표현은 Lumen 장면 밖, 그림자 켬.
- 60fps 판정(N@16.7ms)은 프레임·Game·Render·GPU p50 중 최댓값.
- 원거리 표현은 Bone VAT ISM(`Scripts/bake-vat.py`). M3부터 200m 밖은 Off(그리지 않음).
- 0\~30m는 액터(상한 500), 전환 대책 다섯 가지 전부 켬(`-LabHydrateFixMask` 기본 31).
- 비교는 같은 묶음에서 잰 것끼리만 한다. 같은 M1이 묶음에 따라 20k 16.0 / 15.2ms로 달랐다. M0·M1·M2 묶음(11-08 이전)에는 7.0초 주기 스파이크가 있었고 M3 묶음(11-09)에는 없었다(원인 미상, 환경 추정. `Scripts/spikes.py`로 확인).
- M3b부터 대표 지표는 프레임 p95 N@16.7ms(p50 N·병목 병기), 하위 실험 앵커는 20k P1, Mass 단계 앵커에 30k 추가, 묶음 끝 A/A, 트레이스 병렬+직렬.
- M3b 저격: 대형은 예약 발(틱 4의 배수 200m 밖 1발, 나머지 2 80\~200m 1발)에서만. 중요도는 화면 크기(반경 ÷ 거리).

## 명령

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Extra "-LabStage=M3"
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 2000 -Preset P1 -Duration 13 -ShotAt 10 -Extra "-LabStage=M3"
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-bench.ps1 -Milestone M3 -N 5000 -Preset P1 -Variant mass
py -3 Scripts/summarize.py --milestone M3
```

나머지는 [commands.md](Guides/commands.md).

## 측정 결과

60fps를 지키는 최대 N(P1): M0 full 813, M0 static 1,196 → M1 20,577 → M2 20,518(같은 묶음의 M1 21,396) → M3 22,046(같은 묶음의 M2 + 200m 밖 Off 35,763). 표와 하위 실험은 `Measurements/M<N>/summary.csv`와 `Worklog/M<N>.md`.

## 막힌 것

- 없음.

## 사용자에게 요청한 일

- 2종 구현 때: 시즈 에셋 이전은 런처 엔진 커맨드렛으로 돌린다(commands.md "에셋 마이그레이션"). AnimBP Start Position 바인딩은 Unreal MCP가 필요하니 그때 에디터를 띄워 달라(이번 세션은 MCP 연결 실패).
