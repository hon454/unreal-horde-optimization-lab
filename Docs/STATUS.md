# 현재 상태

마지막 갱신: 2026-11-13

## 단계

- 마일스톤 M0\~M5, M3b와 품질 계획 Q1\~Q6은 끝났다. 총괄 묶음은 `Full2`다.
- 글 00\~09의 초안, Releases, 사이트는 끝났다. 경위는 [Worklog/Posts.md](Worklog/Posts.md)에 있다.
- 지금은 사용자 검토에서 나온 수정을 한다.
- 방향은 목표 사양(호드 총수 1만, 60fps) 안에서 거리별 품질을 확보하는 것이다([ADR-0009](Decisions/0009-flat-scenario-quality-by-distance.md)).
- 계획은 [plan.md](plan.md) "품질 계획"에 있다. 경위는 `Worklog/M<N>.md`와 `Worklog/Q<N>.md`에 있다.
- 코디네이터가 묻지 않고 내린 결정은 [autonomous-decisions.md](Worklog/autonomous-decisions.md) D20\~D32에 있다.

## 다음 할 일

사용자가 2026-11-13에 정했다. Animation Budget Allocator의 예산 값을 측정으로 다시 정한다. 오브 `SnipeView` 경합을 고친다. 전 조건을 새 총괄 묶음 `Full3`으로 다시 잰다. 경위는 [Worklog/Q7.md](Worklog/Q7.md)에 적는다.

1. 끝났다(62b6a42).
2. 측정 세션이 새 Test 패키지로 참고 묶음 `Budget1`을 잰다(최종 구성, 예산 끔·1·2·3·4·6ms, 호드 1만·2만, P1, 세 번씩 36회).
3. 코디네이터가 멈춘다. 채팅에 그림, 캡처, 추천 값, ADR 초안을 낸다. 사용자가 값을 고른다.
4. 고른 값이 2ms와 다르면 코드 세션이 기본값을 바꾼다. 측정 세션이 `Full3`을 잰다.
5. 글 세션이 글 04에 예산을 정한 절을 쓴다. 글 세션이 공개 문서의 수치를 `Full3` 값으로 바꾼다.
6. 코디네이터가 멈춘다. 사용자가 글과 사이트를 검토한다.
7. 사용자가 글 검토를 끝내면 코디네이터가 `Docs/post-review.md`를 지운다.

코디네이터는 첫 Test 패키지 측정(`Budget1`)을 시작하기 전에 사용자의 허락을 한 번 받는다. 그 뒤의 측정과 촬영은 허락 없이 시작한다. 사용자가 그때부터 측정 PC를 비운다.

예산 값의 선택 기준은 둘이다. 호드 1만에서 액터 구간의 건너뜀이 예산을 끈 실행과 구별되지 않는다. 호드 2만에서 프레임 p95가 16.7ms 안에 남는다. 두 기준을 만족하는 가장 작은 값을 고른다.

범위 피해의 시간값 변경과 가변 틱의 `-FixedSeed`는 새 빌드에 넣지 않는다. 코드 세션이 Development 실행으로 영향 크기만 잰다.

코디네이터는 커밋한 뒤 main을 푸시한다([AGENTS.md](../AGENTS.md) "진행 방식").

2026-11-13 검토에서 글과 README의 틀린 서술을 고쳤다. 고친 사실은 [engine-notes.md](Reference/engine-notes.md) 28절에 있다.

사이트 주소는 <https://hon454.github.io/unreal-horde-optimization-lab/>다. 글이나 CSV가 바뀌면 `py -3 -I Scripts/build-site.py`를 다시 돌리고 `gh-pages` 브랜치에 다시 올린다.

끝난 묶음의 러너는 `Scripts/Archive/`에 있다. 그 자리에서는 돌지 않는다([Scripts/README.md](../Scripts/README.md)).

그림과 사이트는 4·7·8단계를 번 시간을 품질에 쓰는 단계로 표시한다(빗금). 묶음 제목은 쓰지 않는다.

공개 글은 여유 기준 13.3ms를 쓰지 않는다. 기준은 16.7ms 하나이고, 남는 시간은 잰 비용과 견주어 적는다([plan.md](plan.md) "목표 사양").

글 02\~07에는 Unreal Insights 스크린샷이 있다([Worklog/Posts.md](Worklog/Posts.md) "Insights 스크린샷"). 트레이스 값을 새로 쓰는 글에는 같은 방법으로 스크린샷을 넣는다.

## 진행 중인 세션

| 세션 | 작업 단위 | 모델 | 상태 |
| --- | --- | --- | --- |
| `budget-measure` | Test 패키징, `Budget1` 러너 준비와 측정 | Sonnet medium | 준비 중. 측정은 사용자 허락 뒤 |

코드 세션 `budget-code`는 끝났다(62b6a42, [Worklog/Q7.md](Worklog/Q7.md) "코드"). 벤치 끝의 종료 코드는 늘 0이다. 러너는 실패를 `LabOk`와 CSV 유무로 판정한다.

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
- Test 패키지 08e45f2는 Releases에 있다(태그 `test-package-08e45f2`, [D30](Worklog/autonomous-decisions.md)).
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

- **글 00\~09 초안과 사이트의 검토.** 글은 `Posts/`에 있다. 사이트 주소는 위 "다음 할 일"에 있다.
- **[autonomous-decisions.md](Worklog/autonomous-decisions.md)의 D20\~D32 검토.** "사용자가 볼 것" 줄이 판단이 필요한 곳이다.
- **`.claude/settings.json`의 수정.** 없는 스크립트(`capture-video.ps1`, `run-e0.ps1`)와 `python` 항목을 지우고 `git add -A` 거부 규칙을 더한다. 코디네이터는 이 파일을 고치지 않는다.
- **`Packaged/Release/` 삭제의 허락.** 릴리스 스테이징 폴더다(약 1.7GB, git 밖). 코디네이터가 작업을 모두 끝낸 뒤에 묻는다.
