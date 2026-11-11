# 현재 상태

마지막 갱신: 2026-11-11

## 단계

M0\~M5, M3b 완료(태그 `M0`\~`M5`, `M3b`). 필수 마일스톤이 끝났다. 끝낸 단계의 경위는 `Worklog/M<N>.md`에 있다.

**목표 사양 안에서 거리별 품질을 확보하는 것으로 방향을 정했다(2026-11-11 사용자 결정).** 장면은 옛 평지 시나리오다. 같은 날 앞서 세운 게임 장면 계획(건물, 방향장, 래그돌, 부위 판정)은 버렸다. 순서는 Q1\~Q4 구현 → 측정 준비 → 총괄 묶음 `Full1` → 글이다. 근거는 [Worklog/M5.md](Worklog/M5.md) "방향 재조정: 평지와 거리별 품질".

## 다음 할 일

계획은 [plan.md](plan.md) "품질 계획"에 있다. 목표 사양은 호드 총수 1만(잠정)·60fps이고, 주 지표는 목표 총수에서의 프레임 시간이다. 최종 구성의 기법은 모두 사다리와 빼 보기 표에서 이점이 보여야 한다.

- [ ] **Q1 체력 분산.** 개체마다 시드로 체력을 다르게 준다. 값을 표로 보고해 확정받는다.
- [ ] **Q2 Niagara 배경 무리.** 200m 밖을 독립된 입자 무리로 채운다(총수에 넣지 않는다). 에디터 작업이 필요하다.
- [ ] **Q3 빼 보기 스위치와 전환 점수.** 빼 보기 표 아홉 조합을 인자로 고르게 하고, ISKM과 VAT의 전환 품질을 비교할 수 있게 한다.
- [ ] **Q4 품질 올리기.** 액터 상한·전환 거리를 재서 최종 구성을 정한다. 목표 총수 후보도 잰다.
- [ ] **Q5 측정 준비.** `Full1` 러너와 메타데이터, 스크립트의 라벨 지원, Test 패키지 HUD, ffmpeg 자동 촬영. 다시 패키징.
- [ ] **Q6 총괄 묶음 `Full1`** → 글 00\~09([backlog.md](backlog.md) "글 대기열").

## 진행 중인 세션

없음. 세션을 띄우면 세션, 작업 단위, 모델과 추론 수준, 상태를 여기에 적는다(AGENTS.md "작업 세션은 작게 나눠 띄우고").

## 앞 단계에서 남은 제약

- M3b 묶음은 게임 창이 뒤에 있던 "뒤 창 묶음"이라 절댓값을 README와 글에 싣지 않는다(상대 비교만 유효, [Worklog/M3b.md](Worklog/M3b.md)). 대표 변형은 `m3mass`, M4부터 2종이 기본이다.
- M4: Simulation LOD는 프레임 이득이 없다(`m4mass` − `m3mass`가 잡음 안). 엔진 이동·조향 3.3ms가 매 프레임 그대로 든다([Worklog/M4.md](Worklog/M4.md)).
- E4 (d): `m4offmove`가 `m4mass`보다 2종 P1 20k 프레임 p50 −1.23ms라서 M5의 기준점이다. 밴드 경계가 달라(188 / 200m) 이 비교는 "참고"다.
- 그리는 끝은 200m다(`-LabFarM` 기본값, 2026-11-11 사용자 결정). 빈 수평선은 Q2 Niagara 배경 무리로 채운다([plan.md](plan.md)).
- 대표 원거리 표현은 `m5bounds`다(2026-11-11 사용자 결정). 고정 상자는 오브가 움직이지 않아 가능한 것이라 카메라 추종 상자의 정지 사례로 읽는다([Worklog/M5.md](Worklog/M5.md) "고정 바운드 판단").
- 화면 크기 Budget은 M3·M4·M5에서 기본으로 켠다(`-LabScreenSigBudget=`). `m3mass`·`m3uro`·`m3budgetdist`·`m3budgetsig`와 M3 하위 실험 변형은 끈다. 그래서 총괄 묶음의 `m4offpool`·`m5bounds`는 Budget이 들어간 값이고 마일스톤 묶음의 같은 이름과 뜻이 다르다([Worklog/M5.md](Worklog/M5.md) "Budget 기본 채택").
- 모션 블러는 기본값으로 둔다([ADR-0007](Decisions/0007-keep-default-motion-blur.md)). 일부 ISKM 인스턴스의 세로 줄무늬는 고치지 않는다.

## 지금 유효한 조건

M2\~M5 공통이고 근거는 [measurement.md](Guides/measurement.md) "확정값"이다.

- 미니언끼리 비켜 가기 끔. 미니언 표현은 Lumen 장면 밖, 그림자 켬.
- 원거리 표현은 M5까지 Bone VAT ISM(`Scripts/bake-vat.py`), 이후 대표는 `m5bounds`(ISKM + 고정 바운드)다. M3부터 200m 밖은 Off(그리지 않음).
- 0\~30m는 액터(상한 500), 전환 대책 다섯 가지 전부 켬(`-LabHydrateFixMask` 기본 31).
- 비교는 같은 묶음에서 잰 것끼리만 한다. 주기 스파이크는 `Scripts/spikes.py`로 확인한다.
- 마일스톤 묶음의 대표 지표는 프레임 p95 N@16.7ms다(총괄 묶음부터는 보조 지표). p50 기준 N(프레임·Game·Render·GPU p50 중 최댓값이 16.7ms)과 병목을 함께 적는다. 하위 실험 앵커는 20k P1, Mass 단계 앵커에 30k 추가, 묶음 끝 A/A, 트레이스 병렬+직렬.
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

- `Binaries/`가 버린 G1 코드로 빌드돼 있다. 다음 개발 실행 전에 다시 빌드한다. Unreal MCP가 연결돼 있지 않아 Q2의 에디터 작업은 에디터를 띄운 뒤에 한다.
- 러너가 게임 창을 앞으로 보내고 검사한다. 측정 중에는 PC를 쓰지 않는다. 디스크의 Test 패키지는 1d2ad79(`m5bounds` 포함)이고 Budget 기본 채택(3f9a885)보다 앞이라, 총괄 묶음 전에 다시 패키징한다.

## 사용자에게 요청한 일

- [ADR-0009](Decisions/0009-flat-scenario-quality-by-distance.md) 승인.
- Q1 체력 분산의 값 확정(코디네이터가 채팅에 낸 제안).
- 에셋 세션(Unreal MCP)을 시작할 때 에디터를 띄워 달라. 벤치마크·패키징이 도는 동안은 에디터를 켜지 않는다.
