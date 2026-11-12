# 현재 상태

마지막 갱신: 2026-11-12

## 단계

- 마일스톤 M0\~M5와 M3b는 끝났다. 태그는 `M0`\~`M5`, `M3b`다.
- 품질 계획 Q1\~Q5는 끝났다. Q6은 총괄 묶음 `Full1`의 측정까지 끝났다.
- 지금은 평가를 반영하고 글을 쓴다.
- 방향은 목표 사양(호드 총수 1만, 60fps) 안에서 거리별 품질을 확보하는 것이다([ADR-0009](Decisions/0009-flat-scenario-quality-by-distance.md)).
- 계획은 [plan.md](plan.md) "품질 계획"에 있다. 경위는 `Worklog/M<N>.md`와 `Worklog/Q<N>.md`에 있다.

## 다음 할 일

순서대로 한다.

1. **입자 메시를 고친다.** 세션 `crowdfx-lowpoly`가 한다.
   - 원인: 200m 밖 입자는 LOD가 없는 약 1만 삼각형 VAT 메시를 두 번 그린다([Worklog/Q6.md](Worklog/Q6.md) "입자 GPU 비용 조사").
   - 수정: 원본 스켈레탈 메시의 낮은 LOD로 입자 전용 VAT 메시를 굽는다.
2. **끝났다.** 사용자가 경계 화질 비교를 보고 LOD4를 승인했다(2026-11-12). 비교 시트는 `Measurements/CrowdFx2/shots/`에 있다.
3. **새 Test 패키지를 만들고 `Full2`로 전 조건을 다시 잰다.** 같은 날 같은 패키지에서 아래 두 묶음도 잰다.
   - 참고 묶음: 해상도를 낮춰 게임 스레드를 병목으로 만든다. 조건은 8단계와 빼 보기 열 행이다(11조건 × 3회). 이 묶음 안에서만 비교한다. 낮추는 방법과 값은 측정 세션이 제안한다.
   - 엔티티 풀: 16분 실행을 두 번 한다(풀 켬, 풀 끔). ISKM 트랙 수와 GPU 메모리를 시간에 따라 기록한다.
4. **새 값으로 문서를 고친다.** 대상은 README 그림, 01번 글의 수치, 7·8단계 화면이다.
5. **에셋 복구 스크립트를 실제로 돌려 검증한다.** 1번이 끝난 뒤에 한다. 사용자가 범위를 정했다(2026-11-12): `Content/ParagonMinions`와 `Content/NiagaraExamples`도 백업 폴더로 옮기고 팩 이전부터 돌린다. 세션 `asset-restore-guide`가 스크립트와 가이드를 쓰고 있다.
6. **Test 패키지를 GitHub Releases에 올린다.** 3번의 패키지를 쓴다. 조건은 "사용자에게 요청한 일"에 있다.
7. **글 00\~09를 쓴다.** 대기열은 [backlog.md](backlog.md)에 있다.
   - 시범 글 01은 [Posts/01-actor-baseline](../Posts/01-actor-baseline/README.md)에 있다. 사용자의 두 번째 검토를 기다린다.
   - 글 규칙은 [posting.md](Guides/posting.md), [ADR-0014](Decisions/0014-post-format.md), [ADR-0015](Decisions/0015-readme-figures.md)에 있다.
   - 글 재료의 사실 확인 결과는 [post-review.md](post-review.md)에 있다. 재료의 측정값은 옛 묶음의 값이다. 경위는 수치 없이 방향만 쓴다.
   - 남은 질문 넷은 [Worklog/Posts.md](Worklog/Posts.md) "코디네이터용"에 있다.
   - 글을 다 쓰면 한눈에 보는 사이트를 만든다.

끝난 평가 항목의 경위는 [Worklog/Posts.md](Worklog/Posts.md) "평가 반영 가운데 끝난 것"에 있다.

## 진행 중인 세션

- `crowdfx-lowpoly`: 입자 전용 저폴리 VAT 메시를 만든다. Opus high. 굽기 경로(`bake-vat.py --lod N`)는 끝났다. LOD3·4 굽기도 끝났다(소형 9,786 → 1,222 / 586 삼각형). 세션은 LOD4를 골랐다(소형 586, 대형 621 삼각형). 지금은 `NS_LabCrowd`의 네 이미터에 적용한다. **에디터가 떠 있다. 이 세션만 에디터, 빌드, Development 실행을 쓴다.** 끝나면 코디네이터가 에디터를 닫는다.
- `asset-restore-guide`: `Scripts/restore-assets.ps1`과 `Docs/Guides/setup.md`를 썼다(c518699, e3e6024). Opus high. 실제 복구 실행은 검증하지 않았다. 검증은 `crowdfx-lowpoly`가 끝난 뒤에 한다.

코디네이터는 세션을 띄우면 여기에 적는다. 끝난 세션은 결과를 확인하고 보관한 뒤 지운다.

## 지금 유효한 조건

근거는 [measurement.md](Guides/measurement.md) "확정값"과 "총괄 묶음 `Full1` 규약"이다.

- 조건 표는 `Scripts/full-conditions.json`의 21개다. 사다리 `L1`\~`L8`, 빼 보기 `A8-*` 열 행, 하위 실험 셋이다([ADR-0013](Decisions/0013-full-batch-runner-and-condition-table.md)).
- 총괄 묶음의 조건 표에는 조건을 더하지 않는다. 참고 묶음은 따로 둔다.
- 최종 구성 `q4final`의 표현은 거리 네 구간이다([ADR-0010](Decisions/0010-final-representation-bands.md), [ADR-0011](Decisions/0011-entity-driven-crowd-particles.md)).
  - 0\~40m: 액터(상한 1,000).
  - 40\~80m: ISKM, 그림자 켬.
  - 80\~200m: ISKM, 그림자 끔.
  - 200m 밖: 엔티티 구동 입자.
- 사다리 1\~7단계는 액터 0\~30m(상한 500), 그림자 켬이다.
- 켠 것: 화면 크기 Budget, 전환 대책 다섯 가지.
- 끈 것: 미니언끼리 비켜 가기, 하이드레이션 액터의 캡슐 콜리전.
- 모션 블러는 엔진 기본값이다([ADR-0007](Decisions/0007-keep-default-motion-blur.md)).
- 비교는 같은 묶음에서 잰 값끼리만 한다. 글과 README의 수치는 총괄 묶음에서만 만든다.
- 조건이 바뀌거나 묶음이 오염되면 전 조건을 새 묶음으로 다시 잰다. 같은 라벨과 묶음 이름은 다시 쓰지 않는다.
- README와 글 본문에는 묶음 이름을 쓰지 않는다.
- 단계별 화면은 움직이는 WebP다(960폭, 15fps, 8초). 촬영 러너는 `Scripts/capture-full.ps1`이다. Chrome이 떠 있으면 러너가 거부한다. 다음 촬영 묶음은 `Pre6`이다.

## 명령

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Stage M5 -Variant q4final
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -DryRun
py -3 -I Scripts/summarize.py --batch Full1
py -3 -I Scripts/plot.py --batch Full1
py -3 -I Scripts/plot-readme.py
```

나머지 명령은 [commands.md](Guides/commands.md)에 있다.

## 측정 결과

총괄 묶음 `Full1`의 값이다. 입자 메시를 고치면 `Full2`의 값으로 바꾼다.

- 빌드는 Test 패키지 730ff18이다. 측정일은 2026-11-12다. 178회 가운데 실패는 0회다.
- 표는 `Measurements/Full1/summary.csv`에 있다. 그래프는 `Docs/figures/full1/`에 있다.
- 약어: P1은 오브 시점 카메라다. p50은 중앙값이다. p95는 느린 쪽 5% 경계다.

호드 1만, P1의 프레임 시간(ms):

| 조건 | p50 | p95 |
| --- | --- | --- |
| `L1` | 441.5 | 466.4 |
| `L2` | 9.69 | 10.29 |
| `L3` | 7.71 | 8.19 |
| `L4` | 8.07 | 9.41 |
| `L5` | 7.51 | 8.83 |
| `L6` | 7.38 | 8.84 |
| `L7` | 12.51 | 13.03 |
| `L8` | 11.12 | 11.64 |

- `L8`의 병목은 GPU다. 게임 스레드 p50은 7.82ms이고 GPU p50은 10.71ms다.
- 60fps를 지키는 최대 개체 수(프레임 p95 기준): `L1` 797, `L2` 20,250, `L4` 26,154, `L7` 13,808, `L8` 16,826. `L3`, `L5`, `L6`은 3만에서도 16.7ms 안이다.
- 빼 보기의 프레임 p50 변화(`L8` 대비, ms): `A8-crowdfx` −3.66, `A8-farshadow` +1.40, `A8-iskm` +1.28, `A8-farcull` +0.70.
- 나머지 빼 보기 다섯 행과 `S8-col`은 프레임 시간이 ±0.1ms 안이다. 차이는 게임 스레드에 나온다(`A8-hydrate` −4.07ms).
- `L8`의 대형 액터 애니메이션 건너뜀 비율: 1만 0.003, 2만 0.637, 3만 0.644.
- `L2` 2만은 세 실행의 범위가 3.35ms다. 원인은 확정하지 못했다. 글에는 범위와 원인 후보를 함께 적는다([Worklog/Q6.md](Worklog/Q6.md) "Full1 후속 분석").
- Development 전환 점수는 참고값이다(`Measurements/Full1/transition-dev/runs.csv`).
  - 튐 평균: 최종 구성 4.63, VAT 5.22, 전환 대책 끔 7.46, 둘 다 끔 7.55.
  - 전환 대책의 이점은 세 시드 모두에서 보인다.
  - ISKM과 VAT의 차이는 시드 범위 안이다. ISKM의 이점은 프레임 시간으로 적는다.

## 막힌 것

없다.

## 사용자에게 요청한 일

- **Releases 올리기 전의 OK.** 코디네이터가 Epic 콘텐츠 라이선스 원문을 확인한다. 그 뒤 파일 목록과 태그 이름을 사용자에게 보인다. 사용자가 OK하면 올린다.
- **01번 글의 두 번째 검토.**
- **[autonomous-decisions.md](Worklog/autonomous-decisions.md)의 "사용자 판단 필요" 항목 검토.**
