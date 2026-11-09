# 현재 상태

마지막 갱신: 2026-11-09

## 단계

M0\~M3 완료(태그 `M0`\~`M3`). 다음은 M3b 새 기준선([plan.md](plan.md) "M3b"). 끝낸 단계의 경위는 `Worklog/M<N>.md`.

## 다음 할 일

- [ ] 1. M3b 계획과 설계 선택지를 추천과 함께 보여 주고 승인받기. 정해진 것: 액터 콜리전 끔, 저격은 대형 우선(구간당 규칙은 계획에서), [plan.md](plan.md) "M3b".
- [ ] 2. [pc-specs.md](Reference/pc-specs.md) 나머지 열(RAM, 해상도, `sg.*`).

## 지금 유효한 조건

M2\~M5 공통이고 근거는 [measurement.md](Guides/measurement.md) "확정값"이다.

- 미니언끼리 비켜 가기 끔(M4에서 가까운 개체만 다시 시험). 미니언 표현은 Lumen 장면 밖, 그림자 켬.
- 60fps 판정(N@16.7ms)은 프레임·Game·Render·GPU p50 중 최댓값.
- 원거리 표현은 Bone VAT ISM(`Scripts/bake-vat.py`). M3부터 200m 밖은 Off(그리지 않음).
- 0\~30m는 액터(상한 500), 전환 대책 다섯 가지 전부 켬(`-LabHydrateFixMask` 기본 31).
- 비교는 같은 묶음에서 잰 것끼리만 한다. 같은 M1이 묶음에 따라 20k 16.0 / 15.2ms로 달랐다.

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

- 없음.
