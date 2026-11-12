# 현재 상태

마지막 갱신: 2026-11-12

## 단계

M0\~M5, M3b 완료(태그 `M0`\~`M5`, `M3b`). 품질 계획 Q1\~Q5 완료, Q6은 총괄 묶음 `Full1`의 측정까지 끝났다. 경위는 `Worklog/M<N>.md`, `Worklog/Q<N>.md`에 있다.

방향은 목표 사양(호드 총수 1만, 60fps) 안에서 거리별 품질을 확보하는 것이다([ADR-0009](Decisions/0009-flat-scenario-quality-by-distance.md)). 계획은 [plan.md](plan.md) "품질 계획"에 있다.

## 다음 할 일

- [ ] **Q6 마무리.** 측정·검사·요약·그래프는 끝났다([Worklog/Q6.md](Worklog/Q6.md) "총괄 묶음 Full1").
  - 문서는 `Full1` 값으로 맞췄다(README, ADR 0010\~0013 "결과", autonomous-decisions 요약, measurement.md, post-material "08". 9be3056). 표는 프레임 시간 옆에 Game p50, GPU p50을 두고 조건은 늘리지 않는다(2026-11-12 사용자 결정). [post-material.md](post-material.md)의 옛 묶음 값은 글을 쓸 때 그 절만 바꾼다. 글 재료는 그대로 쓸 수 없어 글 쓰기 전에 한 번 정리한다(2026-11-12 사용자 결정). README와 글 본문에는 `Full1` 같은 묶음 이름을 쓰지 않는다(같은 날).
  - Development 전환 점수 묶음도 끝났다(아래 "측정 결과").
  - 사다리 여덟 단계의 움직이는 WebP를 `Docs/figures/steps/step<N>-<조건>.webp`에 넣었다(촬영 `Pre5`, 약 51MB, Git LFS. 2026-11-12 사용자가 보고 여덟 개 모두 넣기로 결정). README와 글에는 아직 잇지 않았다. 다시 찍으면 `Pre6`부터 쓴다.
- [ ] **글 00\~09.** [backlog.md](backlog.md) "글 대기열". 수치는 `Full1`에서만 만든다. 글 재료 46개 항목의 사실 확인과 선별, 핵심 인사이트 열 개는 [post-review.md](post-review.md)에 있다(2026-11-12). 재료의 측정값은 거의 전부 옛 묶음 값이고, 경위의 비교 여럿은 `Full1`에 대응 조건이 없다. 경위는 수치 없이 방향만 쓴다(2026-11-12 사용자 결정, [posting.md](Guides/posting.md)). 시범 글 01([Posts/01-actor-baseline](../Posts/01-actor-baseline/README.md), 70b3075)을 사용자가 검토했고(2026-11-12), 의견을 글 규칙에 일반화했다(posting.md, b5d1c7b: "배경 지식" 산문 절, Mermaid 대신 SVG, 약어 풀기, 스레드 셋 함께, 표 제목, 코드 발췌). 그 뒤 글 형식이 [ADR-0014](Decisions/0014-post-format.md)로 정리됐다(본문에 표 없음, 수치는 `summary.csv`에서 스크립트로 만든 SVG). 01을 그 규칙으로 고쳤고(b5bff0a, 87e1d9a) 사용자의 두 번째 검토를 기다린다. 남은 질문(그림 스크립트 위치, 트레이스 값 CSV, 측정 기록의 약어, 기준선 글의 결과 그림)은 [Worklog/Posts.md](Worklog/Posts.md) "코디네이터용"에 있다. README는 사용자 검토(2026-11-12)에 따라 다른 프로젝트·앞선 게임의 이름과 마일스톤 표를 뺐고, 수치 표는 그림 네 개로 바꿨다(ADR-0015, 305f43a. `Scripts/plot-readme.py`가 `Docs/figures/readme/`에 만든다). "시나리오" 절도 같은 스크립트의 그림으로 바꿨다(`scenario.svg`, bdd3fbe). 글을 다 쓰면 한눈에 보는 사이트를 만든다([backlog.md](backlog.md)).

- [ ] **평가 반영(2026-11-12).** 받은 평가 여덟 항목을 사용자와 하나씩 정한다. (1) 200m 밖 입자의 GPU 비용은 Development에서 원인부터 조사한다(세션 `crowdfx-gpu-cause`). 수정과 전 조건 재측정은 결과를 보고 따로 정한다. (2) 해상도를 낮춰 게임 스레드가 병목이 되게 한 참고 묶음(8단계와 빼 보기 열 행, 11조건 × 3회)을 따로 잰다. 그 묶음 안에서만 비교하고 사다리와 잇지 않는다. 낮추는 방법과 값은 측정 세션이 제안하고, 측정은 (1)의 결과가 나온 뒤에 한다. "조건은 늘리지 않는다"는 앞선 결정은 총괄 묶음의 조건 표에만 남는다. (3) AI 협업 흔적은 그대로 둔다(README 단락과 소스 주석 정리 없음). 대신 AGENTS.md를 주제와 역할별로 다시 짰다(aaef5b8. 규칙 내용은 그대로, 지운 결정 날짜는 [Decisions/README.md](Decisions/README.md) "작업 방식의 결정"). (4) 엔티티 풀은 최종 빌드에서 16분 실행 두 번(풀 켬, 풀 끔)을 다시 재서 시간에 따른 ISKM 트랙 수와 GPU 메모리를 기록한다. (2)의 참고 묶음과 같은 날 같은 패키지에서 잰다.

## 진행 중인 세션

- `readme-resources-fig`: README에 단계별 프로세스 메모리, GPU 메모리, 드로우 콜 그림을 더한다(`Docs/figures/readme/resources.svg`. 평가 5번, 2026-11-12 사용자 결정). Opus high. 진행 중.
- `readme-run-prereqs`: README "실행" 절 앞에 전제 세 줄(소스 빌드 엔진, Fab 에셋, 에셋 이전과 베이크)을 넣는다(평가 5번, 2026-11-12 사용자 결정). Sonnet medium. 진행 중.
- `crowdfx-gpu-cause`: 최종 구성에서 200m 밖 입자의 GPU 비용 원인을 Development에서 가린다(조사만, 수정·재측정 없음. 2026-11-12 사용자 결정). 결과는 [Worklog/Q6.md](Worklog/Q6.md) "입자 GPU 비용 조사". Opus high. 조사 단계 끝, Development 트레이스 실행 중(시험 스위치와 `Content/LabTmp/`는 임시이고 끝나면 지운다). 이 세션이 Development 빌드와 `-game` 실행을 쓴다.

세션을 띄우면 세션, 작업 단위, 모델과 추론 수준, 상태를 여기에 적고, 끝나서 확인한 세션은 보관 처리하고 지운다(AGENTS.md "여러 세션"의 코디네이터 항목).

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
- `L2-mass` 2만은 세 회차 범위가 3.35ms다. r1만 렌더 스레드의 `PrepareDistanceFieldScene`이 15.2ms였다(다른 회차 8.6, 8.8). 스태틱 메시 ISM을 쓰는 2\~5단계에만 드는 비용이라 최종 구성과 무관하다. 직접 원인은 확정하지 못했고 더 파지 않는다. 글에는 범위와 원인 후보를 함께 적는다(2026-11-12 사용자 결정).
- 트레이스 16개의 단계별 비용 출처는 [Worklog/Q6.md](Worklog/Q6.md) "Full1 후속 분석"에 있다(L1은 이동 컴포넌트가 72%, L7의 증가는 GPU의 `BasePass`·`RenderVelocities`·`LumenSceneLighting`).

- Development 전환 점수(참고, 화면 캡처 기반, `q4final` 1만 P1, 시드 셋의 중앙값. `Measurements/Full1/transition-dev/runs.csv`): 튐 평균이 최종 구성 4.63, VAT 5.22, 전환 대책 끔 7.46, 둘 다 끔 7.55다. 전환 대책의 이점은 세 시드 모두 보인다(눈에 띈 전환 3\~5% 대 9\~11%). ISKM 대 VAT의 전환 차이는 시드 범위 안이라 ISKM의 이점은 프레임으로 적는다. 엔티티 풀(`A8-pool`)의 이점은 어느 지표에서도 보이지 않았다.

## 막힌 것

- 단계별 움직이는 그림은 움직이는 WebP 960폭, 15fps, 8초다(2026-11-12 사용자가 표본을 보고 결정). 촬영 러너는 `Scripts/capture-full.ps1`이고 Chrome이 떠 있으면 거부한다. 42MB GIF는 레포에 올리지 않는다.

## 사용자에게 요청한 일

- [autonomous-decisions.md](Worklog/autonomous-decisions.md)의 "사용자 판단 필요" 항목 검토(ADR 0010\~0013으로 옮긴 것은 승인됐다).
