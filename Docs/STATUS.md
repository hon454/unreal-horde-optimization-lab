# 현재 상태

마지막 갱신: 2026-11-10

## 단계

M0\~M3, M3b 완료(태그 `M0`\~`M3`, `M3b`). 다음은 M4([plan.md](plan.md) "M4"). 끝낸 단계의 경위는 `Worklog/M<N>.md`.

## 다음 할 일

2026-11-09 점검 결정 1\~5·8·9는 끝났다([plan.md](plan.md) "M3b"·"M4", [measurement.md](Guides/measurement.md)). 결정 6(오브 중심 바닥 띠)·7(장애물 + 플로우 필드를 M5 뒤 첫 마일스톤으로, 퀄리티 앞)도 끝났다([backlog.md](backlog.md)). 아래 1\~4는 코디네이터 세션이 작업 세션에 나눠 맡긴다(AGENTS.md "여러 세션이 한 체크아웃을 쓴다").

- [x] 1\~4. 코드 정비(210bbac·4f6db5b), 측정 방법(af4be11·d761020·spikes.py), 선행 묶음·잡음 진단·코드 A/B(결정 4: 선언 제거, LockObservers 비용 없음), 결정 1 B안(`m3mass` = 15/30m, 5k 캡처 근거), 2종 코드·에셋(ee1a17d·8475be6·245cc35), `-LabBandFloor`(6ad7f7e). 경위·수치는 [Worklog/M3b.md](Worklog/M3b.md)와 해설 페이지 https://claude.ai/artifact/QzpD3b9rV8QC529CR4oZQN.
- [x] 5. 최종 패키지(6a98c46) 하나로 묶음 1·2를 같은 밤에 돌렸다(2026-11-09 23:28\~11-10 06:03, 294회, 실패 0, 주기 스파이크 0, A/A 차 0.04ms, `Scripts/run-m3b-all.ps1`, 라벨 태그 `fin`). 결과는 [Worklog/M3b.md](Worklog/M3b.md) "묶음 1·2 결과"와 `Measurements/M3b/summary.csv`(final=1 행), 해설 https://claude.ai/artifact/YHpunRXyhdetQGBcKMbZPk. N@16.7ms(p95, P1) 1종 M1 16,963 / M2 14,085 / M3 14,204, 2종 M3 13,513(2종 비용 20k P1 +0.56ms). Budget 변형 m3sig 16.06 vs 기준 19.58(−3.5ms). 결정 1(15/30m)은 옛 30/30m보다 +0.7ms, 결정 4(선언 없음)는 선언 있음보다 −0.7ms.
- [x] 6. **재측정하지 않고 M3b를 닫았다(2026-11-10 사용자 결정, 같은 날의 결정 A를 대체).** 묶음 5는 게임 창이 뒤에 있어 Game 스레드가 약 2ms(12%) 느리게 찍힌 "뒤 창 묶음"이다(294회 중 7개만 앞 창, Worklog "페이즈 첫 실행 효과"). 묶음 안의 상대 비교는 유효하고, 절댓값(N@16.7ms 등)은 README와 글에 싣지 않는다. 뒤 창 손실은 Mass 20k 한 조건에서만 쟀다(액터 단계에서 같은 크기인지는 추정). 러너는 고쳤다(eb16991: 게임 창을 앞으로 보내고 측정 60초를 검사, 아니면 FAIL 뒤 한 번 재시도, `fgtest` 3회 fg=30/30). 결정 B1: 대표값은 `m3mass` 유지, Budget 채택은 M4 뒤 작은 마일스톤(backlog). 결정 C1: M4부터 2종 기본.
- [ ] 7. **M4.** (a) 코드 세션이 먼저 코드 기본값을 `-LabTypes=2`로 바꾼다(`-Small` 1종·2종 종료 코드 0 확인). (b) M4 구현([plan.md](plan.md) "M4"). (c) M4 묶음은 고친 러너로 돌고 기준점(`m3mass` 2종)을 같은 묶음에 넣어 잰다. 전 단계 사다리의 앞 창 값은 총괄 묶음에서 만든다. 글 05의 수치는 앞 창 묶음이 생긴 뒤 싣는다. 측정을 세 층(개발 확인 / 마일스톤 묶음 / 총괄 묶음)으로 나누는 안은 사용자 결정 대기다.

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

60fps를 지키는 최대 N(P1): M0 full 813, M0 static 1,196 → M1 20,577 → M2 20,518(같은 묶음의 M1 21,396) → M3 22,046(같은 묶음의 M2 + 200m 밖 Off 35,763). 표와 하위 실험은 `Measurements/M<N>/summary.csv`와 `Worklog/M<N>.md`. M3b(뒤 창 묶음, 상대 비교만): N@16.7ms(p95, P1) 1종 M1 16,963 / M2 14,085 / M3 14,204, 2종 M3 13,513.

## 막힌 것

- 없음. 게임 창이 뒤에 있으면 Game 스레드가 약 2ms 느리다. 러너가 앞 창을 보장하고 검사한다(eb16991). 측정 중에는 PC를 쓰지 않는다.

## 세션 인계(2026-11-10)

코디네이터 세션을 compact하고 M3b 작업 세션을 모두 보관했다. 다음 세션은 이 파일만 읽고 이어 간다: (1) 코드 세션 새로 시작 → 다음 할 일 7 (a)(b). (2) 측정 세션은 M4 묶음 때 새로 시작한다. (3) Niagara 원거리 표현 논의는 토론 1까지 했고(엔진에 Mass→Niagara 공식 경로 없음, MetaHuman Crowd·City Sample도 Niagara로 개체를 그리지 않음) 토론 2\~4가 남았다. 미푸시 커밋이 쌓여 있다. 푸시는 사용자가 "푸시"라고 할 때.

## 사용자에게 요청한 일

- 2종 구현 때: 시즈 에셋 이전은 런처 엔진 커맨드렛으로 돌린다(commands.md "에셋 마이그레이션"). AnimBP Start Position 바인딩과 바닥 띠 머티리얼은 Unreal MCP가 필요하다. MCP 서버는 에디터를 켜면 자동으로 뜨므로(2026-11-09 사용자), 에셋 세션을 시작할 때 에디터를 띄워 달라. 선행 묶음(패키징·벤치마크)이 도는 동안은 에디터를 켜지 않는다.
