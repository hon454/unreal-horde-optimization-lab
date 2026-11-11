# 현재 상태

마지막 갱신: 2026-11-11

## 단계

M0\~M5, M3b 완료(태그 `M0`\~`M5`, `M3b`). 필수 마일스톤이 끝났다. 끝낸 단계의 경위는 `Worklog/M<N>.md`에 있다.

**글 공개가 우선이다(2026-11-11 사용자 결정).** 평지 총괄 묶음 → 글 00\~08 → 장애물 + 플로우 필드 기준선. 글 수치는 평지 시나리오 값으로 남긴다([backlog.md](backlog.md) "글 대기열").

## 다음 할 일

- [ ] **평지 총괄 묶음.** 측정 목록은 글 00\~08이 쓰는 점에서 뽑는다(ADR-0006). `m5bounds`의 N@16.7ms와 P2도 여기서 나온다. 돌리기 전에 아래 여덟 가지를 정한다(2026-11-11 전체 점검에서 나왔다. 근거는 [Worklog/M5.md](Worklog/M5.md) "총괄 묶음 전 점검").
  1. **7초 주기 스파이크.** 고정 바운드 묶음 9회 중 5회가 주기 스파이크 판정이다(`m4offpool` r2·r3, `m5iskm` r1\~r3, `m5bounds` 없음). 원인을 모른다. 확인 실행을 먼저 할지 정한다.
  2. **전 단계 러너.** 모든 단계를 한 번에 도는 러너가 없다. `run-m5-all.ps1`을 일반화한다. 라벨 접두사와 폴더(`summarize.py`·`plot.py`는 `M<숫자>`만 읽는다), M3의 대표 변형(`m3mass`는 Budget 끔), `-Types 2` 명시를 정한다.
  3. **CSV 메타데이터.** 단계·Budget·풀링 여부가 CSV에 없다. `m1mass`\~`m4mass`가 모두 `mass`로 찍힌다.
  4. **묶음 러너의 중단.** 자식 프로세스가 stderr에 한 줄만 써도 묶음이 멈춘다.
  5. **재시도 규칙.** 러너마다 다르다(같은 라벨, r+10, 태그).
  6. **`spikes.py`의 범위.** 접두사로 누적해 훑고 트레이스 실행도 센다.
  7. **`plot.py`.** 지금 돌리면 43계열이 나온다. `Measurements/figures/`는 M0\~M3 때 것이다.
  8. **ISKM 바운드 고정의 직접 확인.** Test 패키지에서 `bounds_override=1`을 본 적이 없다(프레임 끝 시간으로 추정했다).

## 앞 단계에서 남은 제약

- M3b 묶음은 게임 창이 뒤에 있던 "뒤 창 묶음"이라 절댓값을 README와 글에 싣지 않는다(상대 비교만 유효, [Worklog/M3b.md](Worklog/M3b.md)). 대표 변형은 `m3mass`, M4부터 2종이 기본이다.
- M4: Simulation LOD는 프레임 이득이 없다(`m4mass` − `m3mass`가 잡음 안). 엔진 이동·조향 3.3ms가 매 프레임 그대로 든다([Worklog/M4.md](Worklog/M4.md)).
- E4 (d): `m4offmove`가 `m4mass`보다 2종 P1 20k 프레임 p50 −1.23ms라서 M5의 기준점이다. 밴드 경계가 달라(188 / 200m) 이 비교는 "참고"다.
- 그리는 끝은 200m다(`-LabFarM` 기본값, 2026-11-11 사용자 결정). 빈 수평선은 [backlog.md](backlog.md) "선택 심화".
- 대표 원거리 표현은 `m5bounds`다(2026-11-11 사용자 결정). 고정 상자는 오브가 움직이지 않아 가능한 것이라 카메라 추종 상자의 정지 사례로 읽는다([Worklog/M5.md](Worklog/M5.md) "고정 바운드 판단").
- 화면 크기 Budget은 M3·M4·M5에서 기본으로 켠다(`-LabScreenSigBudget=`). `m3mass`·`m3uro`·`m3budgetdist`·`m3budgetsig`와 M3 하위 실험 변형은 끈다. 그래서 총괄 묶음의 `m4offpool`·`m5bounds`는 Budget이 들어간 값이고 마일스톤 묶음의 같은 이름과 뜻이 다르다([Worklog/M5.md](Worklog/M5.md) "Budget 기본 채택").
- 모션 블러는 기본값으로 둔다([ADR-0007](Decisions/0007-keep-default-motion-blur.md)). 일부 ISKM 인스턴스의 세로 줄무늬는 고치지 않는다.

## 지금 유효한 조건

M2\~M5 공통이고 근거는 [measurement.md](Guides/measurement.md) "확정값"이다.

- 미니언끼리 비켜 가기 끔. 미니언 표현은 Lumen 장면 밖, 그림자 켬.
- 원거리 표현은 M5까지 Bone VAT ISM(`Scripts/bake-vat.py`), 이후 대표는 `m5bounds`(ISKM + 고정 바운드)다. M3부터 200m 밖은 Off(그리지 않음).
- 0\~30m는 액터(상한 500), 전환 대책 다섯 가지 전부 켬(`-LabHydrateFixMask` 기본 31).
- 비교는 같은 묶음에서 잰 것끼리만 한다. 주기 스파이크는 `Scripts/spikes.py`로 확인한다.
- 대표 지표는 프레임 p95 N@16.7ms다. p50 기준 N(프레임·Game·Render·GPU p50 중 최댓값이 16.7ms)과 병목을 함께 적는다. 하위 실험 앵커는 20k P1, Mass 단계 앵커에 30k 추가, 묶음 끝 A/A, 트레이스 병렬+직렬.
- 측정은 세 층이다(ADR-0006): 개발 확인(20k P1, 비교에 쓰지 않음), 마일스톤 묶음(새 단계와 기준점만), 총괄 묶음(전 단계와 글이 쓰는 모든 점). 글과 README의 수치는 총괄 묶음에서만 만든다.
- 저격: 대형은 예약 발(틱 4의 배수 200m 밖 1발, 나머지 2 80\~200m 1발)에서만. 중요도는 화면 크기(반경 ÷ 거리).

## 명령

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Stage M5 -Variant m5bounds
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 2000 -Preset P1 -Duration 13 -ShotAt 10 -Stage M5 -Variant m5bounds
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-bench.ps1 -Milestone M5 -Variant m5bounds -N 20000 -Preset P1 -Types 2 -Runs 1 -Tag chk
py -3 Scripts/summarize.py --milestone M5
```

`run-bench.ps1`의 `-Tag`는 라벨에 붙는다. 같은 라벨을 다시 쓰지 않으므로 실행마다 새 태그를 준다. 나머지는 [commands.md](Guides/commands.md).

## 측정 결과

마일스톤 묶음의 값이다. 글과 README의 수치는 총괄 묶음에서 다시 만든다.

- 60fps를 지키는 최대 N(P1): M0 full 813, M0 static 1,196 → M1 20,577 → M2 20,518(같은 묶음의 M1 21,396) → M3 22,046(같은 묶음의 M2 + 200m 밖 Off 35,763).
- M5 묶음(2종 P1, 프레임 p95 기준 N@16.7ms): `m4offpool`(VAT, 기준점) 약 17,100, `m5iskm`(ISKM) 약 16,000.
- 고정 바운드 묶음(2종 20k P1): `m5bounds`가 `m4offpool`보다 프레임 p50 −0.36ms, p95 −0.77ms다. 이 묶음 안에서만 비교한다.

묶음별 표는 `Measurements/M<N>/summary.csv`와 `Worklog/M<N>.md`에 있다.

## 막힌 것

- 없음. 러너가 게임 창을 앞으로 보내고 검사한다. 측정 중에는 PC를 쓰지 않는다. 디스크의 Test 패키지는 1d2ad79(`m5bounds` 포함)이고 Budget 기본 채택(3f9a885)보다 앞이라, 총괄 묶음 전에 다시 패키징한다.

## 사용자에게 요청한 일

- 총괄 묶음 전 결정 여덟 가지("다음 할 일").
- 에셋 세션(Unreal MCP)을 시작할 때 에디터를 띄워 달라. 벤치마크·패키징이 도는 동안은 에디터를 켜지 않는다.
