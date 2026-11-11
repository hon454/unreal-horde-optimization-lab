# 현재 상태

마지막 갱신: 2026-11-11

## 단계

M0\~M5, M3b 완료(태그 `M0`\~`M5`, `M3b`). 필수 마일스톤이 끝났다. 끝낸 단계의 경위는 `Worklog/M<N>.md`에 있다.

**게임 장면에서 목표 사양을 맞추는 것으로 방향을 바꿨다(2026-11-11 사용자 결정).** 순서는 게임 조건 구현 → 총괄 묶음 `Full1` → 글이다. 같은 날 앞서 정한 "평지 총괄 묶음 먼저 공개"를 대체한다. 근거는 [Worklog/M5.md](Worklog/M5.md) "방향 전환: 게임 장면과 목표 사양".

## 다음 할 일

- [ ] **새 계획 짜기.** 아래 결정으로 plan.md의 마일스톤, backlog.md의 글 대기열, 측정 목록을 다시 짜서 사용자 승인을 받는다.
  - 목표 사양은 호드 총수 1만, 60fps, 게임 조건 켬이다. 게임 조건은 장애물 + 플로우 필드, 비켜 가기, 콜리전, 부위 판정, 래그돌이다. 1만이 맞는 수준인지는 최종 측정 때 한 번 더 재서 확정한다.
  - 주 지표는 1만에서의 프레임 시간이다. 60fps를 지키는 최대 N은 보조다.
  - 전 단계를 게임 장면에서 잰다. 장애물 + 플로우 필드와 비켜 가기는 전 단계 공통이다. 콜리전·부위 판정·래그돌은 액터가 있는 단계에만 켜고, 액터가 없는 단계는 표에 "조건 미충족"으로 적는다.
  - 단계는 일곱이다: 액터 기준선 → 로직을 Mass로 → ISM + VAT → 근거리 액터 + 애니 예산 → Simulation LOD → ISKM → 화질 되사기.
  - Simulation LOD와 ISKM은 최종 구성에서 이득이 나게 만든다. 그러려고 프로젝트를 고쳐도 된다.
  - 액터 상한, 액터로 바뀌는 거리, 동시 래그돌 수는 화질 되사기 단계에서 재서 60fps를 지키는 가장 높은 조합으로 정한다.
  - 글은 목표 사양 → 프로파일 → 가장 큰 비용 → 고친 것 → 1만에서 다시 잰 값 순서로 쓴다. 하위 실험은 글의 주장을 받치는 것만 잰다.
  - 공개 자료에 GIF와 HUD(프레임 시간, 개체 수, 단계 이름)를 넣는다. Test 패키지에 인자로 켜는 HUD를 넣고, 측정 실행은 끄고 촬영 실행은 같은 빌드·같은 조건에서 따로 돌린다.

- [ ] **총괄 묶음 준비(2026-11-11 사용자 결정).** 근거는 [Worklog/M5.md](Worklog/M5.md) "총괄 묶음 전 점검".
  - 묶음 이름은 `Full1`이다. 라벨은 `Full1-<단계>-<변형>-N<수>-<프리셋>-r<회차>`, 폴더는 `Measurements/Full1/`. 조건이 바뀌어 다시 재면 `Full2`다. `summarize.py`·`plot.py`·`spikes.py`가 이 라벨을 읽게 한다.
  - 전 단계를 2종으로 잰다(`-Types 2` 명시).
  - M3은 `mass`(Budget 켬)가 대표이고 전 구간을 잰다. `m3mass`(Budget 끔)는 20k P1 한 점만 잰다.
  - CSV 메타데이터에 `LabStage`, `LabTypes`, `LabBudget`, `LabPool`, `LabIskmBounds`(실제로 적용된 개수)를 더하고 러너가 실행마다 기대값과 대조한다.
  - 실패한 실행은 이유를 기록하고 다음으로 넘어간다. 묶음 끝에서 실패분만 새 회차 번호로 한 번 다시 잰다. 실패한 라벨은 다시 쓰지 않고 요약에서 뺀다. 같은 조건이 두 번 실패하면 그 조건은 포기하고 보고한다. 자식 프로세스의 stderr 한 줄로 묶음이 멈추지 않게 한다.
  - 묶음 전에 Chrome을 닫는다(7초 주기 스파이크의 원인, [Worklog/M5.md](Worklog/M5.md) "7초 주기 스파이크 조사"). `Full1` 러너는 시작할 때 `chrome.exe`가 떠 있으면 멈춘다(`run-bench.ps1`은 고치지 않는다). 묶음 뒤 `spikes.py`로 확인하고 주기 판정이 나면 measurement.md "묶음 검사"대로 한다.
  - `spikes.py`는 `Full1` 라벨만 훑고 트레이스 실행은 따로 센다.
  - `plot.py`는 `Measurements/Full1/`만 읽고 그릴 계열(단계별 대표 하나씩)을 스크립트 위쪽 표에 적는다. 옛 그림은 총괄 묶음 뒤에 같은 파일 이름으로 덮는다.
  - 남은 것: 측정 목록(새 계획의 단계에서 뽑는다), 코드·러너 구현, 다시 패키징. 단계 이름과 변형은 새 계획에 맞춰 다시 정한다.
- [ ] **게임 조건 구현.** 장애물 + 플로우 필드(액터 기준선도 같은 방향장을 읽는다), 비켜 가기, 판정용 피직스 애셋, 래그돌, Test 패키지 HUD. 순서와 범위는 새 계획에서 정한다.
- [ ] **총괄 묶음 `Full1`(게임 장면).** 측정 목록은 글이 쓰는 점에서 뽑는다(ADR-0006).

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

- 없음. 러너가 게임 창을 앞으로 보내고 검사한다. 측정 중에는 PC를 쓰지 않는다. 디스크의 Test 패키지는 1d2ad79(`m5bounds` 포함)이고 Budget 기본 채택(3f9a885)보다 앞이라, 총괄 묶음 전에 다시 패키징한다.

## 사용자에게 요청한 일

- 에셋 세션(Unreal MCP)을 시작할 때 에디터를 띄워 달라. 벤치마크·패키징이 도는 동안은 에디터를 켜지 않는다.
