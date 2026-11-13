# 현재 상태

마지막 갱신: 2026-11-13

## 단계

- 마일스톤 M0\~M5, M3b와 품질 계획 Q1\~Q6은 끝났다. 총괄 묶음은 `Full3`이다.
- 글 00\~09의 초안, Releases, 사이트는 끝났다. 경위는 [Worklog/Posts.md](Worklog/Posts.md)에 있다.
- 지금은 사용자 검토에서 나온 수정을 한다.
- 방향은 목표 사양(호드 총수 1만, 60fps) 안에서 거리별 품질을 확보하는 것이다([ADR-0009](Decisions/0009-flat-scenario-quality-by-distance.md)).
- 계획은 [plan.md](plan.md) "품질 계획"에 있다. 경위는 `Worklog/M<N>.md`와 `Worklog/Q<N>.md`에 있다.
- 코디네이터가 묻지 않고 내린 결정은 [autonomous-decisions.md](Worklog/autonomous-decisions.md) D20\~D32에 있다.

## 다음 할 일

사용자가 2026-11-13에 정했다. Animation Budget Allocator의 예산 값을 측정으로 다시 정한다. 오브 `SnipeView` 경합을 고친다. 전 조건을 새 총괄 묶음 `Full3`으로 다시 잰다. 경위는 [Worklog/Q7.md](Worklog/Q7.md)에 적는다.

1\~6. 끝났다. 예산은 4ms다([ADR-0016](Decisions/0016-animation-budget-value.md)). 공개 문서와 사이트는 `Full3` 값이다.
7. 끝났다. 사용자가 사이트를 검토했다. 코디네이터가 `Docs/post-review.md`를 지웠다.
8. 끝났다(83ffcbd). 변형 `manager`의 인자는 `-LabStage=M1 -LabVariant=manager`다([Worklog/Q8.md](Worklog/Q8.md)).
9. 끝났다(47394de). 참고 묶음 `Mgr1`은 18회이고 실패는 0회다. 값은 아래 "측정 결과"에 있다.
10. 측정 세션이 `manager`와 2단계의 트레이스 `MgrT1`을 뽑는다. 측정 세션이 Game 차이의 출처를 적는다.
11. 글 세션이 글 02에 결과와 시각 자료를 더한다. 사용자가 2026-11-13에 정했다.

사용자가 측정을 허락했다. 코디네이터는 뒤의 측정과 촬영을 허락 없이 시작한다.

코디네이터는 커밋한 뒤 main을 푸시한다([AGENTS.md](../AGENTS.md) "진행 방식").

사이트 주소는 <https://hon454.github.io/unreal-horde-optimization-lab/>다. 글이나 CSV가 바뀌면 `py -3 -I Scripts/build-site.py`를 다시 돌리고 `gh-pages` 브랜치에 다시 올린다.

끝난 묶음의 러너는 `Scripts/Archive/`에 있다. 그 자리에서는 돌지 않는다([Scripts/README.md](../Scripts/README.md)).

그림과 사이트는 4·7·8단계를 번 시간을 품질에 쓰는 단계로 표시한다(빗금). 묶음 제목은 쓰지 않는다.

공개 글은 여유 기준 13.3ms를 쓰지 않는다. 기준은 16.7ms 하나이고, 남는 시간은 잰 비용과 견주어 적는다([plan.md](plan.md) "목표 사양").

글 01\~07에는 Unreal Insights 스크린샷이 있다([Worklog/Posts.md](Worklog/Posts.md) "Insights 스크린샷"). 트레이스 값을 새로 쓰는 글에는 같은 방법으로 스크린샷을 넣는다.

## 진행 중인 세션

| 세션 | 작업 단위 | 모델 | 상태 |
| --- | --- | --- | --- |
| `mgr1` | 트레이스 `MgrT1` 6회, 타이머별 차이 | Sonnet medium | 시작함 |

벤치 끝의 종료 코드는 늘 0이다. 러너는 실패를 `LabOk`와 CSV 유무로 판정한다([Worklog/Q7.md](Worklog/Q7.md) "종료 코드").

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
- Animation Budget Allocator의 예산은 4ms다([ADR-0016](Decisions/0016-animation-budget-value.md)).
- 끈 것: 미니언끼리 비켜 가기, 하이드레이션 액터의 캡슐 콜리전.
- 모션 블러는 엔진 기본값이다([ADR-0007](Decisions/0007-keep-default-motion-blur.md)).
- 비교는 같은 묶음에서 잰 값끼리만 한다. 글과 README의 수치는 총괄 묶음 `Full3`에서만 만든다.
- 조건이 바뀌거나 묶음이 오염되면 전 조건을 새 묶음으로 다시 잰다. 같은 라벨과 묶음 이름은 다시 쓰지 않는다.
- README와 글 본문에는 묶음 이름을 쓰지 않는다.
- 단계별 화면은 움직이는 WebP다(960폭, 15fps, 8초). 촬영 러너는 `Scripts/capture-full.ps1`이다. Chrome이 떠 있으면 러너가 거부한다.
- `Full3`의 Test 패키지는 df41c13이다. Releases에는 `Full2`의 패키지 08e45f2가 있다(태그 `test-package-08e45f2`, [D30](Worklog/autonomous-decisions.md)).
- 새 체크아웃은 `Scripts/restore-assets.ps1`로 에셋을 복구한다([setup.md](Guides/setup.md)).

## 명령

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Stage M5 -Variant q4final
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -DryRun
py -3 -I Scripts/summarize.py --batch Full3
py -3 -I Scripts/plot.py --batch Full3
py -3 -I Scripts/plot-readme.py
```

나머지 명령은 [commands.md](Guides/commands.md)에 있다.

## 측정 결과

총괄 묶음 `Full3`의 값이다([Worklog/Q7.md](Worklog/Q7.md) "측정: Full3").

- 빌드는 Test 패키지 df41c13이다. 측정일은 2026-11-13이다. 178회 가운데 실패는 0회다.
- 표는 `Measurements/Full3/summary.csv`에 있다. 그래프는 `Docs/figures/full3/`에 있다.
- 약어: P1은 오브 시점 카메라다. p50은 중앙값이다. p95는 느린 쪽 5% 경계다. Game은 게임 스레드다.
- `Full2`와 달라진 것은 둘이다. 예산이 2ms에서 4ms로 올랐다. 오브 `SnipeView` 경합을 고쳤다(62b6a42).

호드 1만, P1의 값(ms):

| 조건 | 프레임 p50 | 프레임 p95 | Game p50 | GPU p50 |
| --- | --- | --- | --- | --- |
| `L1` | 444.28 | 469.66 | 429.44 | 50.17 |
| `L2` | 10.19 | 10.84 | 6.24 | 9.90 |
| `L3` | 7.91 | 8.48 | 4.16 | 7.53 |
| `L4` | 9.42 | 10.98 | 9.14 | 6.93 |
| `L5` | 8.57 | 10.34 | 8.25 | 6.97 |
| `L6` | 8.45 | 10.14 | 8.29 | 5.37 |
| `L7` | 8.73 | 10.57 | 8.57 | 6.06 |
| `L8` | 9.75 | 11.49 | 9.59 | 5.75 |

- `L4`\~`L8`의 병목은 Game이다.
- `L1`\~`L3`의 프레임 p50은 `Full2`와 세 실행 범위 안에서 같다. `L4`\~`L8`은 0.9\~2ms 길다.
- 60fps를 지키는 최대 개체 수(프레임 p95 기준): `L1` 766, `L2` 19,554, `L4` 19,952, `L5` 22,857, `L6` 23,774, `L7` 22,033, `L8` 20,037. `L3`은 3만에서도 16.7ms 안이다.
- `L8`의 2만 P1 프레임 p95는 16.68ms다. 16.7ms와의 여유는 0.02ms다.
- `L2`의 최대 개체 수는 `Full2`보다 약 1,200 적다. 원인은 확인하지 않았다.
- 빼 보기의 프레임 p50 변화(`L8` 대비, ms): `A8-farcull` +4.00, `A8-hydrate` −3.57, `S8-col` +1.60, `A8-simlod` +0.75, `A8-bounds` +0.54, `A8-crowdfx` −0.40, `A8-budget` −0.29, `A8-iskm` −0.13, `A8-farshadow` +0.13.
- `A8-fix`와 `A8-pool`은 프레임 시간이 세 실행 범위 안이다.
- `A8-iskm`은 GPU p50이 +1.13ms다. ISKM의 이점은 GPU 시간으로 적는다.
- `L8`의 건너뜀 비율(갱신도 보간도 하지 않은 프레임): 1만은 대형 0, 소형 0이다. 2만은 대형 0.004, 소형 0.42다. 3만은 대형 0, 소형 0.48이다.

참고 묶음 `Ref1`(1280×720, 빼 보기 열 행과 `L8`, [D21](Worklog/autonomous-decisions.md)):

- 모든 행의 병목이 Game이다. 이 묶음 안에서만 비교한다.
- `A8-hydrate`는 프레임 p50이 −3.73ms다. `Full2` 때의 참고값이다.

참고 묶음 `Mgr1`(Test 패키지 28346dc, P1, [Worklog/Q8.md](Worklog/Q8.md) "측정: Mgr1"):

| 조건 | Game p50 1만 | Game p50 2만 | 프레임 p50 1만 | 프레임 p50 2만 |
| --- | --- | --- | --- | --- |
| `manager` | 5.27 | 9.89 | 10.14 | 14.98 |
| 2단계 | 6.22 | 12.18 | 9.89 | 15.09 |
| 2단계 직렬 | 6.46 | 12.87 | 10.00 | 14.83 |

- 세 조건의 병목은 GPU다. 프레임 p50의 차이는 세 실행 범위 안이다.
- 초당 사망 수와 살아 있는 총수는 세 조건이 같다.

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

- **[autonomous-decisions.md](Worklog/autonomous-decisions.md)의 D20\~D32 검토.** "사용자가 볼 것" 줄이 판단이 필요한 곳이다.
- **`.claude/settings.json`의 수정.** 없는 스크립트(`capture-video.ps1`, `run-e0.ps1`)와 `python` 항목을 지우고 `git add -A` 거부 규칙을 더한다. 코디네이터는 이 파일을 고치지 않는다.
- **`Packaged/Release/` 삭제의 허락.** 릴리스 스테이징 폴더다(약 1.7GB, git 밖). 코디네이터가 작업을 모두 끝낸 뒤에 묻는다.
