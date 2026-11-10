# 현재 상태

마지막 갱신: 2026-11-11

## 단계

M0\~M4, M3b 완료(태그 `M0`\~`M4`, `M3b`). 지금은 M5([plan.md](plan.md) "M5")다. 끝낸 단계의 경위는 `Worklog/M<N>.md`.

## 다음 할 일

- [ ] **M5.** 비교 기준은 `m4offmove`다(2026-11-10 사용자 결정, [plan.md](plan.md) "M5"). M5 단계는 `m4offmove`의 구성(200m 밖 Off 밴드 + Lab 이동 프로세서) 위에 원거리 표현만 VAT ISM → ISKM으로 바꾼다. 같은 묶음에서 `m4offmove`를 기준점으로 함께 잰다.
- [ ] **움직이는 화면 검토.** 피격 멈춤, 30m 전환, `m4offmove`의 200m 경계 끊김. 탑뷰 카메라에서 짙은 회색 대형이 그림자와 섞이는 문제. Niagara 원거리 표현 논의(토론 2\~4).

## 앞 단계에서 남은 제약

- M3b 묶음은 게임 창이 뒤에 있던 "뒤 창 묶음"이라 절댓값을 README와 글에 싣지 않는다(상대 비교만 유효, [Worklog/M3b.md](Worklog/M3b.md)). 대표 변형은 `m3mass`, M4부터 2종이 기본이다. Budget 채택은 backlog의 작은 마일스톤이다.
- M4: Simulation LOD는 프레임 이득이 없다(`m4mass` − `m3mass`가 잡음 안). 엔진 이동·조향 3.3ms가 매 프레임 그대로 든다([Worklog/M4.md](Worklog/M4.md)).
- E4 (d): `m4offmove`가 `m4mass`보다 2종 P1 20k 프레임 p50 −1.23ms라서 M5의 기준점이다. 밴드 경계가 달라(188 / 200m) 이 비교는 "참고"다.
- 그리는 끝은 200m다(`-LabFarM` 기본값, 2026-11-11 사용자 결정). 빈 수평선은 [backlog.md](backlog.md) "선택 심화".

## 지금 유효한 조건

M2\~M5 공통이고 근거는 [measurement.md](Guides/measurement.md) "확정값"이다.

- 미니언끼리 비켜 가기 끔. 미니언 표현은 Lumen 장면 밖, 그림자 켬.
- 60fps 판정(N@16.7ms)은 프레임·Game·Render·GPU p50 중 최댓값.
- 원거리 표현은 Bone VAT ISM(`Scripts/bake-vat.py`). M3부터 200m 밖은 Off(그리지 않음).
- 0\~30m는 액터(상한 500), 전환 대책 다섯 가지 전부 켬(`-LabHydrateFixMask` 기본 31).
- 비교는 같은 묶음에서 잰 것끼리만 한다. 주기 스파이크는 `Scripts/spikes.py`로 확인한다.
- 대표 지표는 프레임 p95 N@16.7ms(p50 N·병목 병기), 하위 실험 앵커는 20k P1, Mass 단계 앵커에 30k 추가, 묶음 끝 A/A, 트레이스 병렬+직렬.
- 측정은 세 층이다(ADR-0006): 개발 확인(20k P1, 비교에 쓰지 않음), 마일스톤 묶음(새 단계와 기준점만), 총괄 묶음(전 단계와 글이 쓰는 모든 점). 글과 README의 수치는 총괄 묶음에서만 만든다.
- 저격: 대형은 예약 발(틱 4의 배수 200m 밖 1발, 나머지 2 80\~200m 1발)에서만. 중요도는 화면 크기(반경 ÷ 거리).

## 명령

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Extra "-LabStage=M4"
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 2000 -Preset P1 -Duration 13 -ShotAt 10 -Extra "-LabStage=M4"
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-bench.ps1 -Milestone M3 -N 5000 -Preset P1 -Variant mass
py -3 Scripts/summarize.py --milestone M3
```

나머지는 [commands.md](Guides/commands.md).

## 측정 결과

60fps를 지키는 최대 N(P1): M0 full 813, M0 static 1,196 → M1 20,577 → M2 20,518(같은 묶음의 M1 21,396) → M3 22,046(같은 묶음의 M2 + 200m 밖 Off 35,763). 이후 묶음의 표는 `Measurements/M<N>/summary.csv`와 `Worklog/M<N>.md`.

## 막힌 것

- 없음. 러너가 게임 창을 앞으로 보내고 검사한다(eb16991). 측정 중에는 PC를 쓰지 않는다. 디스크의 Test 패키지는 2ed86b9다.

## 사용자에게 요청한 일

- 에셋 세션(Unreal MCP)을 시작할 때 에디터를 띄워 달라. 벤치마크·패키징이 도는 동안은 에디터를 켜지 않는다.
