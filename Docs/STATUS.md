# 현재 상태

마지막 갱신: 2026-11-10

## 단계

M0\~M3 완료(태그 `M0`\~`M3`). 다음은 M3b 정비 + 새 기준선([plan.md](plan.md) "M3b"). 끝낸 단계의 경위는 `Worklog/M<N>.md`.

## 다음 할 일

2026-11-09 점검 결정 1\~5·8·9는 끝났다([plan.md](plan.md) "M3b"·"M4", [measurement.md](Guides/measurement.md)). 결정 6(오브 중심 바닥 띠)·7(장애물 + 플로우 필드를 M5 뒤 첫 마일스톤으로, 퀄리티 앞)도 끝났다([backlog.md](backlog.md)). 아래 1\~4는 코디네이터 세션이 작업 세션에 나눠 맡긴다(AGENTS.md "여러 세션이 한 체크아웃을 쓴다").

- [x] 1. M3b 코드 정비(코드 세션, 210bbac): 핵심 5 + 두 줄 + 엔진 프로세서 8개 끔 + CSV 메타데이터 + E3b (8) 변형 + 별칭 + `-LabFullyParallel=0`. 빌드 성공, `-Small` 전 단계·새 변형 종료 코드 0. 걷기 루프 가설은 사실 아님(Nearest 필터). 설계·표·끈 목록은 [Worklog/M3b.md](Worklog/M3b.md).
- [x] 2. 측정 방법 구현(측정 세션, af4be11·d761020): summarize.py p95·`bottleneck`·`build` 열, M0\~M3 재요약(기존 값 차이 0), plot.py, `Scripts/spikes.py`, `Scripts/run-m3b.ps1 -DryRun`(E3b 24회, 기준점 포함), pc-specs.md. 병목 열은 M0가 game, M1 이후는 대체로 gpu다. 남은 것은 코드 세션에 달렸다: 변형 이름 ↔ `-LabStage` 매핑, 직렬 트레이스 인자(`-LabFullyParallel=0`), CSV 메타데이터 키(`LabBuild`·`LabCVars`·`LabDriver` 가정). 결과: p95 N@16.7ms P1은 M0 full 667 / M1 15,498 / M2 16,460 / M3 18,475 / M3 m2off 30,812(p50 정의 813 / 20,577 / 20,518 / 22,046 / 35,763). 7.0초 주기 스파이크는 M0·M1·M2 묶음(11-08 이전)에 있고 M3 묶음(11-09)에는 없다. `massav`의 2.0초 주기는 저격 간격과 같아 시나리오 효과다.
- [x] 3. 선행 묶음(측정 세션, a0b9771): E3b 24회 + A/B 9회, 실패 0, 주기 스파이크 0. 결과는 [Worklog/M3b.md](Worklog/M3b.md) "선행 묶음", 해설 페이지 https://claude.ai/artifact/QzpD3b9rV8QC529CR4oZQN. m3vis0가 성능 최선(5k 액터 262→114, Game −1.7ms). **문제:** 같은 m3mass 20k P1이 실행마다 16.0/16.2 vs 17.8\~18.5 두 수준으로 갈리고 M3 묶음(15.5)보다 느리다. A/B 판정 불가.
- [x] 3a. 잡음 진단(400bfa6·38948a6): 어피니티는 파싱 오류로 안 걸렸지만(고침) 같은 조건 12회가 15.87\~16.33(σ 0.2)으로 조밀했다. 선행 묶음(15:48\~16:33)의 두 수준(16/18)은 그 시간대만의 환경 요인이다(원인 확인: 그 시간대에 사용자가 MCP 확인을 위해 에디터를 켰다. measurement.md "측정 중 PC 상태"에 기록). 선행 묶음의 20k 프레임 수치는 비교에 쓰지 않는다. 깨끗한 기준 16.2는 M3 묶음 15.5보다 +0.7ms(코드 A/B 대상).
- [x] 3c. D안 코드 A/B(6d040cd·96b8fdf): 20k P1 프레임 p50 기준 16.12 / m3lock0 16.27 / **m3noorder 15.52**(세 쌍 모두 0.4\~0.7 빠름, M3 묶음 15.47 수준). 느려진 0.6ms는 정비 2번의 `ExecuteAfter("LabLogic")`이고 LockObservers=1은 비용 없음. 시작 A/A 16.03 = 묶음 안 기준(수준 안정). **결정 4 확정(2026-11-09): 선언 제거, `m3noorder` 삭제, A/B용 `m3order` 추가.** 코드 세션 진행 중.
- [x] 3b. E3b 화질 캡처(8eb80e1): 1rpm 60초, 화면 안 30m 액터 중앙값 m3vis0 482 / mass 211, 눈에 띈 전환 0%. **대표값 결정: `m3vis15`(조건부, plan.md E3b (8))**. 빠른 회전 캡처(a3a35bd, 90도 0.5초 × 12회): 회전 뒤 화면 안 VAT 잔여 최댓값 중앙값 m3vis0 160 / m3vis15 250 / mass 402, 100 아래 회복은 m3vis0만(14프레임). **상한 500이 찬 20k에서 등 뒤 링은 손해**. **재검토(2026-11-09, 포크 세션): B안 `m3vis15` 유지.** 20k 미달은 상한이 찬 탓이라 근거에서 뺐고, 조건을 5k 캡처로 바꿨다(plan.md E3b (8)).
- [x] 3d. 5k 빠른 회전 캡처(코드 세션, 4f6db5b 빌드): `m3vis15` near_vat 최댓값 중앙값 21 / `m3vis0` 40, near_actors 최솟값 중앙값 119 / 99, 미달 0/12 둘 다(Worklog "빠른 회전" 5k). **B안 확정, 구현 끝(7f0236f): `m3mass` 기본값 15/30m(`m3vis15`는 별칭), 옛 30/30m은 `m3vis30`(E3b 24회의 기준점), `m3vis0` 대조군.** M2 이하 코드는 안 닿음, `-Small` 네 변형 종료 코드 0. 결정 4는 4f6db5b(선언 제거 + `m3order`). 측정 스크립트는 기준점 `m3vis30`·A/B `m3order`로 고치는 중(측정 세션).
- [x] 4a. 2종 코드(코드 세션, ee1a17d): `-LabTypes=2`(일련번호 10의 배수가 대형), 종류별 아키타입·VAT·ISM·액터 풀, 저격 E안, `m3sig` + 대형 틱 건너뜀 스탯. 에셋 경로는 `DefaultGame.ini` `[LabUnitAssets]`. 1종 `-Small` 결과 불변. 대형 에셋이 없으면 소형 대체(`LabTypesAssets=substituted`, 측정 금지). 필요한 에셋 표는 [Worklog/M3b.md](Worklog/M3b.md) "2종(코드)".
- [x] 4b. 2종 에셋(에셋 세션, 8475be6·245cc35): 시즈 11개 이전, `ABP_LabMinionLarge`, 정적 메시, VAT(86프레임), 액터 머티리얼, ini Large 키, `LargeBoundRadius=47`(measurement.md 확정값), 바닥 띠 `M_LabBandFloor`(파라미터 `BandCenter`, `BandRadius1~3`). 측정 스크립트도 끝(0a47b9f: `m3sig`, `-Types 2`, `-Phase types2`, summarize 열). `-LabBandFloor` 스위치(6ad7f7e, 개발 전용, P2 끔, `lab.BandFloor` 메타데이터). 2종 스모크 `large_assets=ok` 종료 코드 0(ff77de6), 스크린샷 `Saved/Screenshots/Lab/m3b_types2_m3mass_n5000_p1.png`.
- [x] 5. 최종 패키지(6a98c46) 하나로 묶음 1·2를 같은 밤에 돌렸다(2026-11-09 23:28\~11-10 06:03, 294회, 실패 0, 주기 스파이크 0, A/A 차 0.04ms, `Scripts/run-m3b-all.ps1`, 라벨 태그 `fin`). 결과는 [Worklog/M3b.md](Worklog/M3b.md) "묶음 1·2 결과"와 `Measurements/M3b/summary.csv`(final=1 행), 해설 https://claude.ai/artifact/YHpunRXyhdetQGBcKMbZPk. N@16.7ms(p95, P1) 1종 M1 16,963 / M2 14,085 / M3 14,204, 2종 M3 13,513(2종 비용 20k P1 +0.56ms). Budget 변형 m3sig 16.06 vs 기준 19.58(−3.5ms). 결정 1(15/30m)은 옛 30/30m보다 +0.7ms, 결정 4(선언 없음)는 선언 있음보다 −0.7ms.
- [ ] 6. **막힌 것 → 결정 대기:** 게임 창이 백그라운드이면 Game 스레드가 약 2ms(12%) 느리다(진단 2026-11-10, Worklog "페이즈 첫 실행 효과"). 묶음 294회 중 페이즈 첫 실행 7개만 포그라운드였고 나머지는 Claude 앱 창이 앞에 있었다. 묶음 안 비교는 유효하나 절대값이 낮다. 사용자 결정: (A) 러너에 포그라운드 보장 + 프로브 검사를 넣고 294회 재측정(추천) / (B) 이대로 태그. 그다음 (B) m3sig 대표값 여부, (C) M4부터 2종 기본 여부. 결정 뒤 STATUS 측정 결과·Worklog·README 갱신과 태그 `M3b`.

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

- 벤치마크 중 게임 창의 포그라운드 여부가 Game 스레드를 약 2ms 바꾼다(다음 할 일 6). 측정 중에는 다른 창을 앞에 두지 않는다. 러너 수정은 사용자 결정 뒤.

## 사용자에게 요청한 일

- 2종 구현 때: 시즈 에셋 이전은 런처 엔진 커맨드렛으로 돌린다(commands.md "에셋 마이그레이션"). AnimBP Start Position 바인딩과 바닥 띠 머티리얼은 Unreal MCP가 필요하다. MCP 서버는 에디터를 켜면 자동으로 뜨므로(2026-11-09 사용자), 에셋 세션을 시작할 때 에디터를 띄워 달라. 선행 묶음(패키징·벤치마크)이 도는 동안은 에디터를 켜지 않는다.
