# 현재 상태

마지막 갱신: 2026-11-17

## 단계

- 작업 브랜치는 main이다. `v2`의 작업은 main에 병합됐다(a7ac37d). 브랜치 `v2`는 없다.
- 릴리스 태그는 `v2`다(a7ac37d). 릴리스의 패키지는 de0e21b다. 이전 판은 태그 `v1`(62146e2)에 있다.
- 시나리오와 시뮬레이션을 다시 만든 경위는 [ADR-0017](Decisions/0017-roaming-orb-and-equal-function-ladder.md)과 [Worklog/V2.md](Worklog/V2.md)에 있다.
- 총괄 묶음은 `Full4`다(패키지 de0e21b, 태그 `v2-full4`). W1\~W14는 끝났다.
- 사이트는 `gh-pages`에 올렸다(85328bc). 주소는 <https://hon454.github.io/unreal-horde-optimization-lab/>다.
- 용어의 기준은 [glossary.md](Guides/glossary.md)다.
- 코디네이터가 묻지 않고 내린 결정은 [autonomous-decisions.md](Worklog/autonomous-decisions.md)에 있다.
- `v1`의 조건과 측정 결과는 태그 `v1`의 이 문서에 있다.

## 다음 할 일

1. 두 스킬은 커밋됐다(`.claude/skills/`, 94d35ce, f5d69e1, 436b884). 경위는 [Worklog/Skills.md](Worklog/Skills.md)에 있다.
2. 불림 시험과 재시험은 끝났다. 13개 모두 맞는 스킬이 불렸다. 보완(5d5d22a) 뒤 재시험한 네 질문도 내용이 맞다([Worklog/Skills.md](Worklog/Skills.md) "재시험").
3. 측정 스킬이 영어 질문에 한국어로 답한 일이 한 번 남았다. 더 고칠지는 사용자가 정한다.
4. 공개 자료를 고치면 사용자 승인을 받아 `v2.1`처럼 태그를 달아 릴리스한다.
5. 사이트에 보이는 것을 고치면 `Scripts/build-site.py`를 다시 돌린다. `Site/assets/`는 `.gitignore`에 있어 `gh-pages`에 따로 넣는다.
6. `gh-pages`는 강제 푸시 없이 올린다. 방법은 [Worklog/V2.md](Worklog/V2.md) "W14-20 병합과 릴리스" 절에 있다.

끝난 W14 작업 단위(커밋):

| 단위 | 내용 | 커밋 |
| --- | --- | --- |
| W14-G | 용어집 | 2bce803 |
| W14-P | 엔티티 풀 긴 실행 러너와 참고 묶음 `Pool2` | fefc6a6, 5d88cf9 |
| W14-1 | 묶음 그림 | 36cceb7 |
| W14-2 | README 그림과 사이트 데이터 스크립트 | 163336b |
| W14-3 | 단계 화면과 대표 프레임 | 2d6e77a |
| W14-4 | `-LabRepTint`와 소스 주석의 용어 | c12daf7 |
| W14-5 | 입자의 디버그용 색 사본(`Content/LabDebug/FX/`) | 894a637 |
| W14-6 | 표현별 색 정지 화면 5장 | 93ed08d |
| W14-7 | 가이드와 참고 문서의 용어 | d556417 |
| W14-8 | 팁 문서 `mass-crowd-tips.md`(31항목, 매뉴얼 문체) | 1c26c8d, 7583990, 6a1c599 |
| W14-9\~16 | 글 00\~07 초안 | 1ebab61, daf8da4, 4306930, 16fe248, b0dc6f4, 4340647, 583a770, 086c465 |
| 글 맞추기 | 글 00\~07 사이의 표기, 링크, 그림 색 | 604bb28 |
| W14-17 | README, 글 목록, `v1` 글 폴더 여덟 삭제 | b8be175, 9f1b955 |
| W14-18 | `Site/`를 새 데이터 형식에 맞춤 | b088d95 |

다음 총괄 묶음 전에 할 일:

- 저격 캡슐 트레이스의 시작점을 고친다(engine-notes.md 53절).
- 표현 프로세서와 밀어내기 프로세서의 순서 문제(D60)를 고칠지 사용자가 정한다.
- 패키지를 만든 뒤 `Saved/Cooked/`에 `Content/LabDebug/`가 없는지 확인한다.

지켜야 할 것:

- 측정 전에 Chrome과 Edge를 닫는다. 러너가 쓰는 `progress.log`에 `tail -F`를 걸지 않는다.
- `Packaged/Test`는 de0e21b다. 다시 만들지 않는다.
- 공개 글의 기준은 16.7ms 하나다. 단계 3\~8은 호드 1만의 프레임 p95와 16.7ms까지의 여유로 적는다. 최대 N은 단계 1과 2에만 쓴다.
- 예외로 최종 구성의 최대 N "약 1만 2천"을 글 07과 README 요약에 한 문장으로 적는다(사용자 결정, 2026-11-17). 글 07과 README에 반영했다.
- 최종 구성의 여유는 2.47ms로 쓴다(사용자 결정, 2026-11-17).
- 구조 그림의 화면은 `Measurements/Full4/shots/reptint-s8-moving-t75.jpg`다(사용자 결정, 2026-11-17).
- 글 05는 80m 밖 그림자 끄기를 한 문단으로 줄인다. 개념 절과 코드 블록을 뺀다(사용자 결정, 2026-11-17, b5db295).
- 글 04는 "표현 셋을 약 1ms에 샀다"를 "단계 3에서 7까지 1.02ms 늘었다"로 쓴다(사용자 결정, 2026-11-17, b5db295). 글 07도 같다.
- 검토 결과를 공개 문서에 반영하는 기준(사용자 결정, 2026-11-17): 독자가 사실을 잘못 알게 되는 곳만 본문에서 고친다. 더 정밀하게 쓸 수 있을 뿐인 것은 측정 기록에 둔다. 고칠 때는 문장을 늘리지 않는다.
- "단계 3에서 7까지 +1.02ms, 구별된다"는 그대로 둔다. 단서 문장을 붙이지 않는다(사용자 결정, 2026-11-17).
- 뒤의 릴리스는 `v2.1`처럼 태그를 단다. 태그 `v1`과 `v2`의 자료는 지우거나 옮기지 않는다(사용자 결정, 2026-11-17).
- 트레이스 값을 새로 쓰는 글에는 Unreal Insights 스크린샷을 넣는다. 글 01, 02, 03의 스크린샷은 코디네이터가 찍어 넣었다(2026-11-17).
- 하위 에이전트 여럿이 `Worklog/V2.md`를 함께 고치면 커밋이 섞인다. 브리프에 커밋 직전의 `git diff` 확인을 넣는다.

## 진행 중인 세션

| 세션 | 작업 단위 | 모델 | 상태 |
| --- | --- | --- | --- |

코디네이터는 세션을 띄우면 여기에 적는다. 끝난 세션은 결과를 확인하고 보관한 뒤 지운다.

## 지금 유효한 조건

근거는 [measurement.md](Guides/measurement.md) "확정값"과 "총괄 묶음 규약", [v2-spec.md](v2-spec.md)다.

- 시나리오: 호드 1만, 오브는 반지름 57.3m 궤도를 2m/s로 돈다. 카메라는 P1(오브 시점)이다.
- 측정: 워밍업 60초, 측정 180초, 조건마다 3회의 중앙값이다. 정지 장면은 궤도 속도 0이다.
- 판정: 단계 8의 프레임 p95 중앙값 + 0.6ms가 16.7ms 이하다.
- 차이 판정: 프레임 p95 차이가 0.5ms 아래거나 범위 안이면 "구별되지 않는다"로만 쓴다.
- 조건 표는 `Scripts/full-conditions.json`이다. 본 실행은 207회다. 총괄 묶음의 조건 표에는 조건을 더하지 않는다.
- 최종 구성(단계 8)의 표현은 거리 네 구간이다([ADR-0021](Decisions/0021-unified-actor-cap-and-hydrate-distance.md)).
  - 0\~35m: 액터(상한 1,000).
  - 35\~80m: ISKM, 그림자 켬.
  - 80\~200m: ISKM, 그림자 끔.
  - 200m 밖: 배경 무리 입자.
- 단계 2\~8은 액터 상한 1,000과 전환 거리 35m을 쓴다.
- 기본값: Mass Avoidance 끔, 밀어내기 프로세서 켬, 액터 캡슐 켬, 최소 유지 시간 2,000ms.
- Animation Budget Allocator의 예산은 4ms다([ADR-0016](Decisions/0016-animation-budget-value.md)).
- 비교는 같은 묶음에서 잰 값끼리만 한다. 글과 README의 수치는 `Full4`에서만 만든다. `Pool2`는 "참고"로만 싣는다.
- README와 글 본문에는 묶음 이름과 조건 라벨을 쓰지 않는다.
- 단계 화면은 움직이는 WebP다(960×540, 15fps, 8초, `Docs/figures/steps/`).
- Releases에는 `v1` 시절의 패키지 08e45f2가 있다. 병합 때 de0e21b를 올린다(사용자 결정 9).
- 새 체크아웃은 `Scripts/restore-assets.ps1`로 에셋을 복구한다([setup.md](Guides/setup.md)).

## 명령

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Stage 8
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -Batch Full5 -DryRun
py -3 -I Scripts/summarize.py --batch Full4
py -3 -I Scripts/plot.py --batch Full4
py -3 -I Scripts/plot-readme.py
py -3 -I Scripts/build-site.py
py -3 -I Scripts/check-tilde.py
```

나머지 명령은 [commands.md](Guides/commands.md)에 있다.

## 측정 결과

총괄 묶음 `Full4`의 값이다([Worklog/V2.md](Worklog/V2.md) "총괄 묶음 Full4").

- 빌드는 Test 패키지 de0e21b다. 측정일은 2026-11-16\~17이다. 본 실행 207회가 모두 있다.
- 표는 `Measurements/Full4/summary.csv`에 있다. 그래프는 `Docs/figures/full4/`와 `Docs/figures/readme/`에 있다.
- 동등성 검사는 실패 0이다. 기능 미달은 6건이다(호드 2만, 단계 3\~8).
- 같은 조건을 다시 잰 차이는 프레임 p95 0.08ms다.

호드 1만, 이동 장면, P1의 값(ms):

| 단계 | 프레임 p50 | 프레임 p95 |
| --- | --- | --- |
| 1 액터 | 597.11 | 665.25 |
| 2 Mass, 엔진 기본 경로 | 34.06 | 36.79 |
| 3 자체 밀어내기 | 12.41 | 14.15 |
| 4 애니메이션 표현 | 12.86 | 14.21 |
| 5 Simulation LOD + 엔티티 풀 | 13.09 | 14.52 |
| 6 ISKM | 13.20 | 14.71 |
| 7 배경 무리 | 13.56 | 15.17 |
| 8 남은 비용 줄이기 | 12.77 | 14.23 |

- 단계 8의 판정값은 14.23 + 0.6 = 14.83ms다. 16.7ms까지의 여유는 2.47ms다.
- 단계 4\~7은 앞 단계와 구별되지 않는다. 단계 3에서 7까지는 +1.02ms다.
- 최대 N은 단계 1이 781, 단계 2가 4,102다.
- 단계 2는 게임 스레드가 엔진 회피를 16.2ms 기다린다. 이 시간은 게임 스레드 시간에 들지 않는다(글 02).
- 기법별 기여에서 구별된 조건(프레임 p95 차이, ms): `s8av` +8.57, `a8hyd` −6.98, `a8far` +3.54, `a8cap` −1.86, `a8fsb` +0.98, `a8sep` −0.93, `a8iskm` −0.67, `a8bnd` +0.66.
- 나머지 열 조건은 구별되지 않는다.
- 정지 장면의 단계 8은 프레임 p95 11.46ms다. 이동 장면과 직접 견주지 않는다.

참고 묶음 `Pool2`(960초, 조건마다 1회):

| 항목 | 풀 켬 | 풀 끔 |
| --- | --- | --- |
| ISKM 트랙 수(16분) | 21,149 | 120,428 |
| GPU 메모리(16분, MB) | 3,371 | 5,105 |
| 프레임 p95(16분, ms) | 14.47 | 15.23 |

## 막힌 것

없다.

## 사용자에게 요청한 일

없다.
