# 현재 상태

마지막 갱신: 2026-11-13

## 단계

- 마일스톤 M0\~M5와 M3b는 끝났다. 태그는 `M0`\~`M5`, `M3b`다.
- 품질 계획 Q1\~Q5는 끝났다. Q6은 총괄 묶음 `Full2`의 측정까지 끝났다.
- 문서는 새 값으로 고쳤다(090f52c). 지금은 글을 쓴다.
- 방향은 목표 사양(호드 총수 1만, 60fps) 안에서 거리별 품질을 확보하는 것이다([ADR-0009](Decisions/0009-flat-scenario-quality-by-distance.md)).
- 계획은 [plan.md](plan.md) "품질 계획"에 있다. 경위는 `Worklog/M<N>.md`와 `Worklog/Q<N>.md`에 있다.
- 사용자는 2026-11-12에 남은 작업을 코디네이터에게 맡겼다. 코디네이터는 결정을 [autonomous-decisions.md](Worklog/autonomous-decisions.md) D20부터 적는다.

## 다음 할 일

순서대로 한다.

1. **7·8단계 화면을 다시 찍는다.** 촬영 묶음은 `Pre6`이다. 화면은 찍었다. 촬영 세션이 풀 끔 실행의 떨림을 확인하는 중이다.
2. **01번 글의 트레이스 값을 새 트레이스에서 다시 뽑는다.** 코디네이터가 촬영이 끝난 뒤에 한다. Insights 화면도 다시 찍는다.
3. **Test 패키지를 GitHub Releases에 올린다.** 패키지는 08e45f2다. 라이선스 판단은 [D25](Worklog/autonomous-decisions.md)에 있다. 촬영이 끝난 뒤에 한다.
4. **글 00\~09를 쓴다.** 대기열은 [backlog.md](backlog.md)에 있다. 나누는 법은 [D27](Worklog/autonomous-decisions.md)에 있다.
   - 시범 글 01은 [Posts/01-actor-baseline](../Posts/01-actor-baseline/README.md)에 있다.
   - 글 규칙은 [posting.md](Guides/posting.md), [ADR-0014](Decisions/0014-post-format.md), [ADR-0015](Decisions/0015-readme-figures.md)에 있다.
   - 글 재료의 사실 확인 결과는 [post-review.md](post-review.md)에 있다. 재료의 측정값은 옛 묶음의 값이다. 경위는 수치 없이 방향만 쓴다.
   - 엔진 문제를 쓸 때는 이슈 링크와 이 레포의 해결 코드 블록을 함께 보인다.
   - 독자용 문장은 묶음 이름 대신 "최종 측정"이라고 쓴다.
   - 글을 다 쓰면 한눈에 보는 사이트를 만든다.
5. **푸시한다.** 사용자가 2026-11-12에 푸시를 승인했다. 294a762까지 푸시했다. 촬영 중에는 푸시하지 않는다.

## 진행 중인 세션

- `post-00`, `post-02`, `post-03`: 글 00, 02, 03을 쓴다. Opus high. 진행 중.
- `pre6-capture`: 7·8단계 화면을 `Pre6`으로 다시 찍는다. 풀 끔 실행의 40\~80m 떨림을 화면으로 확인한다. Sonnet medium. 진행 중.
- 촬영이 끝날 때까지 다른 세션과 사용자는 에디터, 빌드, Chrome을 띄우지 않는다.

코디네이터는 세션을 띄우면 여기에 적는다. 끝난 세션은 결과를 확인하고 보관한 뒤 지운다.

## 지금 유효한 조건

근거는 [measurement.md](Guides/measurement.md) "확정값"과 "총괄 묶음 규약"이다.

- 조건 표는 `Scripts/full-conditions.json`의 21개다. 사다리 `L1`\~`L8`, 빼 보기 `A8-*` 열 행, 하위 실험 셋이다([ADR-0013](Decisions/0013-full-batch-runner-and-condition-table.md)).
- 총괄 묶음의 조건 표에는 조건을 더하지 않는다. 참고 묶음은 따로 둔다.
- 최종 구성 `q4final`의 표현은 거리 네 구간이다([ADR-0010](Decisions/0010-final-representation-bands.md), [ADR-0011](Decisions/0011-entity-driven-crowd-particles.md)).
  - 0\~40m: 액터(상한 1,000).
  - 40\~80m: ISKM, 그림자 켬.
  - 80\~200m: ISKM, 그림자 끔.
  - 200m 밖: 엔티티 구동 입자. 입자 메시는 원본 스켈레탈 LOD4로 구운 메시다(bf1a4c8).
- 사다리 1\~7단계는 액터 0\~30m(상한 500), 그림자 켬이다.
- 켠 것: 화면 크기 Budget, 전환 대책 다섯 가지.
- 끈 것: 미니언끼리 비켜 가기, 하이드레이션 액터의 캡슐 콜리전.
- 모션 블러는 엔진 기본값이다([ADR-0007](Decisions/0007-keep-default-motion-blur.md)).
- 비교는 같은 묶음에서 잰 값끼리만 한다. 글과 README의 수치는 총괄 묶음 `Full2`에서만 만든다.
- 조건이 바뀌거나 묶음이 오염되면 전 조건을 새 묶음으로 다시 잰다. 같은 라벨과 묶음 이름은 다시 쓰지 않는다.
- README와 글 본문에는 묶음 이름을 쓰지 않는다.
- 단계별 화면은 움직이는 WebP다(960폭, 15fps, 8초). 촬영 러너는 `Scripts/capture-full.ps1`이다. Chrome이 떠 있으면 러너가 거부한다.
- 새 체크아웃은 `Scripts/restore-assets.ps1`로 에셋을 복구한다([setup.md](Guides/setup.md)).

## 명령

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Stage M5 -Variant q4final
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -DryRun
py -3 -I Scripts/summarize.py --batch Full2
py -3 -I Scripts/plot.py --batch Full2
py -3 -I Scripts/plot-readme.py
```

나머지 명령은 [commands.md](Guides/commands.md)에 있다.

## 측정 결과

총괄 묶음 `Full2`의 값이다([Worklog/Q6.md](Worklog/Q6.md) "Full2 재측정").

- 빌드는 Test 패키지 08e45f2다. 측정일은 2026-11-12\~13이다. 178회 가운데 실패는 0회다.
- 표는 `Measurements/Full2/summary.csv`에 있다. 그래프는 `Docs/figures/full2/`에 있다.
- 약어: P1은 오브 시점 카메라다. p50은 중앙값이다. p95는 느린 쪽 5% 경계다. Game은 게임 스레드다.

호드 1만, P1의 값(ms):

| 조건 | 프레임 p50 | 프레임 p95 | Game p50 | GPU p50 |
| --- | --- | --- | --- | --- |
| `L1` | 439.96 | 463.58 | 424.87 | 50.23 |
| `L2` | 10.31 | 10.93 | 6.30 | 10.02 |
| `L3` | 7.91 | 8.47 | 3.21 | 7.54 |
| `L4` | 8.16 | 9.60 | 7.87 | 6.94 |
| `L5` | 7.65 | 9.12 | 7.33 | 6.97 |
| `L6` | 7.51 | 9.19 | 7.34 | 5.36 |
| `L7` | 7.79 | 9.34 | 7.63 | 6.10 |
| `L8` | 7.78 | 9.42 | 7.65 | 5.72 |

- `L4`\~`L8`의 병목은 Game이다. `Full1`에서 `L7`과 `L8`의 병목은 GPU였다.
- 60fps를 지키는 최대 개체 수(프레임 p95 기준): `L1` 774, `L2` 20,753, `L4` 25,636, `L6` 29,799, `L7` 26,587, `L8` 25,958. `L3`과 `L5`는 3만에서도 16.7ms 안이다.
- 빼 보기의 프레임 p50 변화(`L8` 대비, ms): `A8-farcull` +4.30, `S8-col` +2.01, `A8-budget` +1.75, `A8-hydrate` −1.59, `A8-simlod` +1.04, `A8-bounds` +0.77, `A8-crowdfx` −0.27, `A8-iskm` −0.21.
- `A8-fix`, `A8-pool`, `A8-farshadow`는 프레임 시간이 세 실행 범위 안이다.
- `A8-iskm`은 GPU p50이 +1.23ms이고 Game p50이 −0.33ms다. ISKM의 이점은 GPU 시간으로 적는다.
- `A8-farshadow`는 GPU p50만 +0.39ms다.
- `L8`의 대형 액터 애니메이션 건너뜀 비율: 1만 0.207, 2만 0.573, 3만 0.557. 1만의 값은 `Full1`의 0.003보다 크다. 원인은 확인하지 않았다.
- `L1`\~`L6`은 `Full1`보다 프레임 p50이 0.1\~0.6ms 길다. 원인은 확인하지 않았다.

참고 묶음 `Ref1`(1280×720, 빼 보기 열 행과 `L8`, [D21](Worklog/autonomous-decisions.md)):

- 모든 행의 병목이 Game이다. 이 묶음 안에서만 비교한다.
- `A8-hydrate`는 프레임 p50이 −3.73ms다. 나머지 행은 `Full2`와 0.2ms 안에서 같다.

엔티티 풀 묶음 `Pool1`(16분 실행 두 번):

| 항목 | 풀 켬 | 풀 끔 |
| --- | --- | --- |
| ISKM 트랙 수(16분) | 20,053 | 119,473 |
| GPU 메모리(1분 → 16분, MB) | 3,339 → 3,342 | 3,310 → 5,063 |
| 프레임 p50(1분 → 16분, ms) | 7.81 → 8.08 | 7.74 → 9.64 |

- 60초 실행에서는 `A8-pool`과 `L8`이 구별되지 않는다.

Development 전환 점수는 `Full1` 때의 참고값이다(`Measurements/Full1/transition-dev/runs.csv`).

- 튐 평균: 최종 구성 4.63, VAT 5.22, 전환 대책 끔 7.46, 둘 다 끔 7.55.
- ISKM과 VAT의 차이는 시드 범위 안이다.

## 막힌 것

없다.

## 사용자에게 요청한 일

- **01번 글의 두 번째 검토.**
- **[autonomous-decisions.md](Worklog/autonomous-decisions.md)의 D20 뒤 결정과 "사용자 판단 필요" 항목 검토.**
