# 현재 상태

마지막 갱신: 2026-11-11

## 단계

M0\~M5, M3b 완료(태그 `M0`\~`M5`, `M3b`). 필수 마일스톤이 끝났다. 다음은 [backlog.md](backlog.md)의 순서다. 끝낸 단계의 경위는 `Worklog/M<N>.md`에 있다.

## 다음 할 일

- [x] **원거리 표현 결정(2026-11-11 사용자): `m5bounds`.** 다음 단계와 총괄 묶음의 대표 원거리 표현이다. 고정 상자는 오브가 움직이지 않아 가능한 것이라 카메라 추종 상자의 정지 사례로 읽는다(일반화는 backlog "선택 심화"). 근거는 [Worklog/M5.md](Worklog/M5.md) "고정 바운드 판단".
- [x] **고정 바운드 실험(1f5de2e, 묶음 39c1a0f).** 프레임 끝 `SendAllEndOfFrameUpdates`가 1.9ms에서 0.5ms로 줄었다. `m5bounds`의 N@16.7ms와 P2는 아직 재지 않았다.
- [x] **M5 Game +0.6ms 원인(트레이스, e09c355).** 엔진 `EndVisualChanges` +0.56\~0.75ms가 Game 증가분이고, 프레임 끝 `SendAllEndOfFrameUpdates` +1.0ms(거의 `ParallelFor.Wait`)가 프레임 − Game 간격과 크기가 맞는다. 하위 원인은 미확인이다. [Worklog/M5.md](Worklog/M5.md) "트레이스 검토".
- [x] **M5**(2026-11-11, 묶음 0a7b8bb, 패키지 40fc360). 설계 결정 9개는 [plan.md](plan.md) "M5"에 있다.
  - [x] 에셋 세션 `m5-assets`: `Scripts/create-iskm.py`(c573c0b). Python(A안)으로 끝났다. ISKM용 복제 메시 `SK_…ISKM`(`bOptimizeForInstancing`), ASTPD 2개, 머티리얼 2개. 애니메이션 바운드는 Test 패키지에서 런타임으로 확인한다. [Worklog/M5.md](Worklog/M5.md) "에셋 세션".
  - [x] 측정 러너 `Scripts/run-m5-all.ps1`(8fc271c, 42회, 약 90분). M5 단계는 `Lab/IskmInstances` 중앙값 > 0을 검사한다. Worklog/M5.md "측정 세션".
  - [x] 코드 세션 `m5-code`(dadc490): 엔티티 풀링, Lab ISKM 프로세서, `ELabStage::M5`와 변형 5개, 스탯 `Lab/IskmTracks`·`Lab/IskmInstances`. 풀링 켬과 끔에서 재생성 해시 12줄이 같다. 개발 20k P1에서 ISKM 인스턴스는 7,101개다. 원거리 스크린샷은 `Measurements/M5/shots/m5iskm_20k_p1_dev.jpg`. 벤치에 `-LabDuration`을 주면 측정 구간이 그 길이가 되고, 주지 않으면 60초 그대로다. Worklog/M5.md "코드 세션".
  - [x] 패키징(40fc360) → M5 묶음 42회(재시도 0) → 비교 → 로컬 태그 `M5`.
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
- 원거리 표현은 M5까지 Bone VAT ISM(`Scripts/bake-vat.py`), 이후 대표는 `m5bounds`(ISKM + 고정 바운드)다. M3부터 200m 밖은 Off(그리지 않음).
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

60fps를 지키는 최대 N(P1): M0 full 813, M0 static 1,196 → M1 20,577 → M2 20,518(같은 묶음의 M1 21,396) → M3 22,046(같은 묶음의 M2 + 200m 밖 Off 35,763). 이후 묶음의 표는 `Measurements/M<N>/summary.csv`와 `Worklog/M<N>.md`에 있다.

M5 묶음(2종 P1, N@16.7ms는 프레임 p95 기준): `m4offpool`(VAT, 기준점) 약 17,100, `m5iskm`(ISKM) 약 16,000.
- 20k P1에서 ISKM은 Render −3.2ms, GPU −3.0ms이지만 Game +0.77ms라 프레임 p50이 +1.7ms다.
- P2 20k에서는 프레임 p50이 −2.9ms다.
- E5 (b) 16분 실행: 풀링을 켜면 트랙이 20,050에서 멈춘다. 풀링을 끄면 124,840까지 늘고 GPU 메모리가 약 +1GB 는다.
- 엔티티 풀링 자체의 비용은 잡음 안이다(+0.06ms).

고정 바운드 묶음(2종 20k P1, 패키지 1d2ad79, 3회 중앙값, 이 묶음 안에서만 비교한다): 프레임 p50 / p95 / Game / Render / GPU가 `m4offpool` 14.83 / 16.89 / 13.86 / 7.04 / 9.44, `m5iskm` 16.34 / 18.00 / 14.45 / 3.92 / 6.49, `m5bounds` 14.47 / 16.11 / 13.85 / 3.82 / 6.46ms다.

## 막힌 것

- 없음. 러너가 게임 창을 앞으로 보내고 검사한다(eb16991). 측정 중에는 PC를 쓰지 않는다. 디스크의 Test 패키지는 1d2ad79(`m5bounds` 포함)이다.

## 사용자에게 요청한 일

- 에셋 세션(Unreal MCP)을 시작할 때 에디터를 띄워 달라. 벤치마크·패키징이 도는 동안은 에디터를 켜지 않는다.
