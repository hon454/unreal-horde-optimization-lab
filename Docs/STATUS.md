# 현재 상태

마지막 갱신: 2026-11-12

## 단계

M0\~M5, M3b 완료(태그 `M0`\~`M5`, `M3b`). 품질 계획 Q1\~Q5 완료, Q6은 총괄 묶음 `Full1`의 측정까지 끝났다. 경위는 `Worklog/M<N>.md`, `Worklog/Q<N>.md`에 있다.

방향은 목표 사양(호드 총수 1만, 60fps) 안에서 거리별 품질을 확보하는 것이다([ADR-0009](Decisions/0009-flat-scenario-quality-by-distance.md)). 계획은 [plan.md](plan.md) "품질 계획"에 있다.

## 다음 할 일

- [ ] **Q6 마무리.** 측정·검사·요약·그래프는 끝났다([Worklog/Q6.md](Worklog/Q6.md) "총괄 묶음 Full1").
  - 문서를 `Full1` 값으로: ADR 0010\~0013 "결과" 덧붙임, [autonomous-decisions.md](Worklog/autonomous-decisions.md) 맨 위 요약, README·[measurement.md](Guides/measurement.md) 수치, [post-material.md](post-material.md) "08"의 잠정값.
  - 사다리 표와 빼 보기 표는 프레임 시간 옆에 Game p50, GPU p50을 둔다(Render는 측정 기록에만). 조건은 늘리지 않는다(2026-11-12 사용자 결정).
  - Development 전환 점수 묶음([ADR-0012](Decisions/0012-ablation-switches-and-quality-metrics.md)).
  - GIF 촬영(아래 "막힌 것"의 둘을 먼저 고친다).
- [ ] **글 00\~09.** [backlog.md](backlog.md) "글 대기열". 수치는 `Full1`에서만 만든다.

## 진행 중인 세션

- `docs-full1`(local_9a80969a): 문서를 `Full1` 값으로 맞추기(ADR 0010\~0013 "결과", autonomous-decisions 요약, README 표·그래프, measurement.md, post-material "08"). Sonnet medium. 시작함(2026-11-12).

세션을 띄우면 세션, 작업 단위, 모델과 추론 수준, 상태를 여기에 적고, 끝나서 확인한 세션은 보관 처리하고 지운다(AGENTS.md "작업 세션은 작게 나눠 띄우고").

## 지금 유효한 조건

근거는 [measurement.md](Guides/measurement.md) "확정값"과 "총괄 묶음 `Full1` 규약"이다.

- 조건 표는 `Scripts/full-conditions.json`의 21개다(사다리 `L1`\~`L8`, 빼 보기 `A8-*` 열 행, 하위 실험 셋). 탐색 점은 표에 넣지 않는다([ADR-0013](Decisions/0013-full-batch-runner-and-condition-table.md)).
- 최종 구성 `q4final`: 액터 0\~40m(상한 1,000), ISKM 40\~80m 그림자 켬, 80\~200m 그림자 끔, 200m 밖 엔티티 구동 입자([ADR-0010](Decisions/0010-final-representation-bands.md), [ADR-0011](Decisions/0011-entity-driven-crowd-particles.md)). 사다리 1\~7단계는 액터 0\~30m(상한 500), 그림자 켬.
- 미니언끼리 비켜 가기 끔, 하이드레이션 액터의 캡슐 콜리전 끔, 화면 크기 Budget 켬, 전환 대책 다섯 가지 켬. 모션 블러는 엔진 기본값이다([ADR-0007](Decisions/0007-keep-default-motion-blur.md)).
- 비교는 같은 묶음에서 잰 것끼리만 한다. 글과 README의 수치는 총괄 묶음에서만 만든다. 마일스톤 묶음과 Q2\~Q4 묶음의 값은 `Full1`과 견주지 않는다.
- 조건이 바뀌거나 묶음이 오염되면 `Full1`을 버리고 `Full2`로 전 조건을 다시 잰다. 같은 라벨과 묶음 이름은 다시 쓰지 않는다.

## 명령

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Stage M5 -Variant m5bounds
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -DryRun
py -3 -I Scripts/summarize.py --batch Full1
py -3 -I Scripts/plot.py --batch Full1
```

나머지는 [commands.md](Guides/commands.md).

## 측정 결과

**총괄 묶음 `Full1`**(Test 패키지 730ff18, 2026-11-12, 178회 실패 0, 동등성 54조건 통과, 스파이크 0건). 표는 `Measurements/Full1/summary.csv`, 그래프는 `Docs/figures/full1/`.

- 1만 P1 프레임 p50 / p95(ms): L1 441.5 / 466.4, L2 9.69 / 10.29, L3 7.71 / 8.19, L4 8.07 / 9.41, L5 7.51 / 8.83, L6 7.38 / 8.84, L7 12.51 / 13.03, L8 11.12 / 11.64. 최종 구성의 병목은 GPU다(Game p50 7.82, GPU p50 10.71).
- N@16.7ms(프레임 p95): L1 797, L2 20,250, L4 26,154, L7 13,808, L8 16,826. L3·L5·L6은 3만에서도 안쪽이다.
- 빼 보기(프레임 p50, L8 대비): `A8-crowdfx` −3.66, `A8-farshadow` +1.40, `A8-iskm` +1.28, `A8-farcull` +0.70(병목이 Game으로 옮겨 간다). 나머지 다섯과 `S8-col`은 ±0.1 안이고 Game에서 갈린다(`A8-hydrate` Game −4.07).
- 최종 구성의 대형 액터 애니 건너뜀 비율: 1만 0.003, 2만 0.637, 3만 0.644.
- `L2-mass` 2만은 세 회차 범위가 3.35ms다. r1만 렌더 스레드의 `PrepareDistanceFieldScene`이 15.2ms였다(다른 회차 8.6, 8.8). 스태틱 메시 ISM을 쓰는 2\~5단계에만 드는 비용이라 최종 구성과 무관하다. 직접 원인은 확정하지 못했다.
- 트레이스 16개의 단계별 비용 출처는 [Worklog/Q6.md](Worklog/Q6.md) "Full1 후속 분석"에 있다(L1은 이동 컴포넌트가 72%, L7의 증가는 GPU의 `BasePass`·`RenderVelocities`·`LumenSceneLighting`).

## 막힌 것

- 촬영 전에 고칠 것 둘(측정과 무관): 엔진의 "CsvProfiler Frame" 화면 메시지가 HUD 글자에 겹친다. GIF 하나가 41MB다(960x540 150프레임). 반드시 줄이고 이런 크기는 레포에 올리지 않는다(2026-11-12 사용자 지시).
- 독자용 HUD는 Test 패키지에서 8단계 화면만 확인했다(`Measurements/Q5/shots/hud-test-step8.jpg`).
- README의 수치 표와 `Docs/figures/n-at-60fps.svg`는 아직 마일스톤 묶음의 값이다.

## 사용자에게 요청한 일

- [autonomous-decisions.md](Worklog/autonomous-decisions.md)의 "사용자 판단 필요" 항목 검토(ADR 0010\~0013으로 옮긴 것은 승인됐다).
