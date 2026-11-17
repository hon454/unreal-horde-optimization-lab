# 04 측정 기록: 표현을 더하는 단계

본문 [README.md](README.md)의 수치마다 정밀한 값과 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓴다. 차이(ms)는 소수 둘째 자리이고, CSV 글자에서 반올림한다(half up). 용어와 측정 방법은 [글 00의 측정 기록](../00-goals-and-method/measurements.md)과 같다.

## 빌드와 묶음

| 항목 | 값 |
| --- | --- |
| 묶음 | 총괄 묶음 `Full4`. 2026-11-16 14:50:12\~2026-11-17 08:52:12(`batch.json`) |
| 빌드 | Test 패키지 de0e21b(태그 `v2-full4`). CSV 메타데이터 `labbuild=de0e21bd64ac` |
| 조건 | 호드 총수 1만, 이동 장면(`V0`), 오브 시점 카메라(`P1`). 워밍업 60초 + 측정 180초, 조건마다 3회 |
| 라벨 | `Full4-V0-<구성>-N10000-P1-r<회차>` |
| 본문 조건의 회차 | `V0-s3`\~`V0-s8`, `V0-a8hold`, `V0-a8bud`, `V0-a8fix`, `V0-a8bnd`는 모두 r1, r2, r3이다(`summary.csv` `runs_used`, 느린 실행 없음) |
| 요약 파일 | [요약 표](../../Measurements/Full4/summary.csv), [실행별 값](../../Measurements/Full4/runs.csv), [동등성 검사](../../Measurements/Full4/equivalence.csv), [사다리 표](../../Docs/figures/full4/full-ladder.csv), [기법별 기여 표](../../Docs/figures/full4/full-ablation-moving.csv) |
| 단계 4, 6, 7의 정의 | `LabConfig.cpp:50-55`(de0e21b): 단계 4부터 `bScreenSigBudget`, 전환 대책 마스크 31, `RepHoldMs` 2,000. 단계 7부터 `bCrowdFx`. `LabConfig.h:407` `UsesVat()`(단계 4부터), `:424` `UsesIskm()`(단계 6부터) |

## 본문의 수치와 출처

조건은 따로 적지 않으면 위 "조건"이다. "세 실행" 열은 r1 / r2 / r3 순서다. 구별 판정은 [measurement.md](../../Docs/Guides/measurement.md) "차이 판정"이다(차이가 0.5ms 이상이고 두 조건의 범위 가운데 큰 쪽보다 클 때만 구별된다).

| 본문 값 | 정밀한 값 | 조건 | 출처(열 또는 식) | 세 실행 |
| --- | --- | --- | --- | --- |
| 단계 3 프레임 p95 14.1ms | 14.148 | `V0-s3` | `frame_p95_ms` | 14.265 / 13.975 / 14.148 |
| 단계 3 여유 2.55ms | 2.552 | 계산 | 16.7 − 14.148 | |
| 단계 3 게임 스레드 11.4ms | 11.356 | `V0-s3` | `game_p50_ms` | 11.488 / 11.312 / 11.356 |
| 단계 3 GPU 7.07ms | 7.069 | `V0-s3` | `gpu_p50_ms` | 7.165 / 7.043 / 7.069 |
| 예상: GPU가 4.29ms 짧다 | 4.287 | 계산 | 11.356 − 7.069 | |
| 단계 4 +0.06ms | +0.057 | `V0-s4` − `V0-s3` | 14.205 − 14.148. 큰 쪽 범위 0.290 | 14.37 / 14.205 / 14.086 |
| 단계 5 +0.31ms | +0.311 | `V0-s5` − `V0-s4` | 14.516 − 14.205. 큰 쪽 범위 0.284 | 14.7 / 14.516 / 14.513 |
| 단계 6 +0.19ms | +0.192 | `V0-s6` − `V0-s5` | 14.708 − 14.516. 큰 쪽 범위 0.187 | 14.695 / 14.715 / 14.708 |
| 단계 7 +0.46ms, 범위를 넘는다 | +0.460 | `V0-s7` − `V0-s6` | 15.168 − 14.708. 큰 쪽 범위 0.335 | 15.426 / 15.168 / 15.091 |
| 단계 3 → 7 +1.02ms, 구별된다 | +1.020 | `V0-s7` − `V0-s3` | 15.168 − 14.148. 큰 쪽 범위 0.335 | 위 두 행 |
| 단계 7 프레임 p95 15.2ms | 15.168 | `V0-s7` | `frame_p95_ms` | 15.426 / 15.168 / 15.091 |
| 단계 7 여유 1.53ms | 1.532 | 계산 | 16.7 − 15.168 | |
| ISKM: GPU 7.15 → 5.60ms, −1.56ms | 7.154 → 5.599, −1.555 | `V0-s5`, `V0-s6` | `gpu_p50_ms`. 범위 0.058, 0.009 | 7.19 / 7.154 / 7.132; 5.594 / 5.599 / 5.603 |
| ISKM: 렌더 스레드 5.38 → 3.87ms | 5.377 → 3.865, −1.512 | `V0-s5`, `V0-s6` | `render_p50_ms`. 범위 0.089, 0.047 | 5.377 / 5.39 / 5.301; 3.824 / 3.865 / 3.871 |
| 배경 무리: GPU 6.65ms, +1.05ms | 6.648, +1.049 | `V0-s7` − `V0-s6` | `gpu_p50_ms`. 범위 0.033, 0.009 | 6.648 / 6.657 / 6.624 |
| 병목은 다섯 단계 모두 게임 스레드 | `game` | `V0-s3`\~`V0-s7` | `bottleneck` | |
| GPU는 게임 스레드보다 4ms 넘게 짧다 | 최소 4.287 | 계산 | `game_p50_ms` − `gpu_p50_ms`: 4.287, 4.683, 4.911, 6.711, 6.038 | |
| 게임 스레드 +1.33ms | 11.356 → 12.686, +1.330 | `V0-s3`, `V0-s7` | `game_p50_ms` | 12.931 / 12.686 / 12.686 |
| 단계 8이 0.94ms 되찾는다 | −0.937 | `V0-s8` − `V0-s7` | 14.231 − 15.168. 구별된다 | 14.426 / 14.035 / 14.231 |
| ISKM 고정 바운드를 끈 조건 +0.66ms | +0.656 | `V0-a8bnd` − `V0-s8` | 14.887 − 14.231. 큰 쪽 범위 0.391, 구별된다 | 14.999 / 14.682 / 14.887 |
| 왕복 전환 초당 4.34건 | 4.344 | `V0-a8hold` | `sw_round_all` | 4.5 / 4.228 / 4.344 |
| 모두 액터 경계에서 났다 | 4.344, 0, 0 | `V0-a8hold` | `sw_round_actor`, `sw_round_far`, `sw_round_shadow` | |
| 규칙을 켜면 0건 | 0 | `V0-s8` | `sw_round_all` | 0 / 0 / 0 |
| 최소 유지 시간을 끈 조건, 구별되지 않는다 | −0.210 | `V0-a8hold` − `V0-s8` | 14.021 − 14.231 | 14.1 / 13.901 / 14.021 |
| 단계 4 애니메이션 시간 4.03ms | 4.032 | `V0-s4` | `budgeted_anim_ms` | 4.04 / 4.032 / 4.014 |
| 소형 액터 7.02% | 0.0702 | `V0-s4` | `small_skipped_ratio` | 0.0712 / 0.0702 / 0.0693 |
| 최종 구성 애니메이션 시간 2.71ms | 2.708 | `V0-s8` | `budgeted_anim_ms` | 2.728 / 2.708 / 2.683 |
| 예산을 끈 조건, 구별되지 않는다 | −0.189 | `V0-a8bud` − `V0-s8` | 14.042 − 14.231 | 14.078 / 13.912 / 14.042 |
| 전환 대책을 끈 조건, 최종 구성과 같다 | 0.000 | `V0-a8fix` − `V0-s8` | 14.231 − 14.231 | 14.295 / 14.072 / 14.231 |

- 요약의 "1.56ms", "1.05ms", "1.02ms"는 위 행과 같다. "약 1ms"는 1.020을 정수로 줄인 말이다.
- "단계 3에서 7까지"는 단계 4, 5, 6, 7 네 단계에 걸친 비교다. 이웃한 두 단계는 묶음 하나씩만 다르지만, 이 비교는 네 묶음이 섞인다. 사다리 계획([V2-w14-plan.md](../../Docs/Worklog/V2-w14-plan.md) 결정 1, 3.1절)이 이 누적을 글의 결론으로 정했다. 본문은 단계 5의 몫이 섞인다고 적었다.
- 단계 4, 6, 7의 차이만 더하면 0.709ms이고 단계 5는 0.311ms다. 넷 모두 구별되지 않는 차이라 더한 값은 본문에 쓰지 않았다.
- `small_skipped_ratio`는 액터 애니메이션 예산이 갱신도 보간도 하지 않은 프레임의 비율이다(소형, [measurement.md](../../Docs/Guides/measurement.md) 67행). `budgeted_anim_ms`는 엔진 CSV 스탯 `AnimationBudget/GameThread/BudgetedAnimation`의 실행별 중앙값이다(`Scripts/summarize.py`, 글 05 측정 기록).
- 단계 4\~7의 동등성 검사는 모두 `ok`다. 초당 사망과 30m 안 개체 수는 기준과 0.1% 안이다(`equivalence.csv`: 단계 4 −0.05%, −0.08%, 단계 6 −0.01%, +0.04%, 단계 7 +0.01%, −0.03%).

## 설정값과 소스 사실

프로젝트 코드의 줄은 측정 패키지 커밋 de0e21b의 것이다. 지금 `v2`에서 줄이 다른 파일은 따로 적었다.

| 본문의 말 | 값 | 출처 | 비고 |
| --- | --- | --- | --- |
| 구간 경계 35m, 80m, 200m | 3,500, 8,000, 20,000cm | `LabCrowdSubsystem.cpp:135` `Bands`, `LabConfig.h:19` `DefaultHydrateM`, `:219` `FarM` | 지금 `v2`에서는 208행이다(W14-4가 색 코드를 더했다) |
| 액터 상한 1,000 | 1,000 | `LabConfig.h:18` `DefaultActorCap`, `LabCrowdSubsystem.cpp:136` `MaxCounts` | [ADR-0021](../../Docs/Decisions/0021-unified-actor-cap-and-hydrate-distance.md) |
| 200m 밖은 그리지 않는다 | Off → None | `LabCrowdSubsystem.cpp:143` | |
| 예산 4ms | 4 | `LabConfig.h:121` `BudgetMs`, `LabGameMode.cpp:383-391` `ApplyBudgetCVars` | [ADR-0016](../../Docs/Decisions/0016-animation-budget-value.md), `summary.csv` `budget_ms` 4 |
| 액터 메시를 `USkeletalMeshComponentBudgeted`로 | | `LabMinion.cpp:366` `ALabMinionMassActorBudget` 생성자의 `SetDefaultSubobjectClass` | |
| 화면 크기를 중요도로 | 바운드 반경 ÷ 카메라 거리 | `LabActorSyncProcessor.cpp:306` `SetComponentSignificance` | |
| 최소 유지 시간 2초 | 2,000ms | `LabConfig.h:23` `DefaultRepHoldMs`, `summary.csv` `rep_hold_ms` 2000 | 끈 조건은 0 |
| 전환 대책 다섯 | 마스크 31 | `LabConfig.cpp:51`, [용어집](../../Docs/Guides/glossary.md) 1.3절 | 다섯의 목록은 용어집에 있다 |
| VAT 프로세서가 프레임 번호를 넘긴다 | `Frame`, `PrevFrame` | `LabVatProcessor.cpp:130-131`, `:145` `AddBatchedCustomData`, `LabVatProcessor.h:53-57` | 게임 스레드: 부모 `UMassUpdateISMProcessor`가 게임 스레드 전용([engine-notes.md](../../Docs/Reference/engine-notes.md) 3절) |
| AnimToTexture로 뼈의 위치와 회전을 굽는다 | 텍스처 `BonePosition`, `BoneRotation`, `BoneWeight` | `Scripts/bake-vat.py:200`, `:209` | |
| ISKM으로 바꾼다 | `SkinnedMeshInstance` | `LabCrowdSubsystem.cpp:210-216`(de0e21b) | |
| 상태가 바뀐 개체의 트랙만 다시 쓴다 | | [code-tour.md](../../Docs/Guides/code-tour.md) "ISKM 갱신", `LabIskmProcessor.cpp` | |
| ISKM은 AnimBP가 없다 | | [engine-notes.md](../../Docs/Reference/engine-notes.md) 3절 "ISKM 블렌딩" | |
| 엔진은 상자가 없으면 인스턴스를 모두 돈다 | `bFastBuild = false` | `Engine/Source/Runtime/Engine/Public/InstancedSkinnedMeshComponentHelper.h:202`, `:227-235`. 상자가 있으면 `InstancedSkinnedMeshComponent.cpp:1249-1253` `CalcBounds`가 바로 돌려준다 | 2026-11-17 직접 열어 확인 |
| 상자가 카메라를 따라간다 | 10m | `LabConfig.h:25` `DefaultBoundsStepM`, [code-tour.md](../../Docs/Guides/code-tour.md) "ISKM 갱신" | |
| 엔진 문제 셋과 우회 | | [글 06](../06-engine-findings/README.md) 1\~3절 | |
| 입자에 넘기는 원소 | (X, Y, 시각, 위상) | `LabCrowdFxProcessor.cpp:179`, `LabConfig.h:53-58` | |
| 원소마다 그 프레임에만 입자를 만든다 | 수명 0.001초, 보간 스폰 끔 | `Scripts/create-crowd-fx.md` 5절, [engine-notes.md](../../Docs/Reference/engine-notes.md) 24절 | |
| 메시는 LOD4로 구운 VAT 메시 | 소형 삼각형 586 | `Scripts/create-crowd-fx.md` 6절 | 본문은 삼각형 수를 쓰지 않았다 |
| 커스텀 데이터 0번은 입자 번호 | | `Engine/Plugins/FX/Niagara/Shaders/Private/NiagaraUpdateMeshGPUSceneInstances.usf:208`, `Engine/Plugins/FX/Niagara/Source/Niagara/Private/NiagaraRendererMeshes.cpp:1130` | 2026-11-17 직접 열어 확인. engine-notes 17절 |
| 걷기 프레임은 Dynamic Parameter로 | 1번 | `Scripts/create-crowd-fx.md` 2, 5절 | |
| 200m 밖 엔티티는 1초마다 옮겨진다 | 1.0초 | `LabConfig.h:73` `FLabSimLod::TickLow`(단계 5부터) | `summary.csv` `sim_lod` 메타데이터 "80/200m tick=0/0.50/1.00" |
| 맵 밖(맵 중심 500\~1,000m) | 50,000, 100,000cm | `LabConfig.h:96` `MapRadius`, `:50` `FLabCrowdFx::OuterRadius` | [ADR-0020](../../Docs/Decisions/0020-far-stream-respawn-handoff.md) |
| 경계의 여유 10% | 0.1 | `MassLODCalculator.h:271` `BufferHysteresisOnDistanceRatio`, `MassRepresentationFragments.h:309` `BufferHysteresisOnDistancePercentage` 10 | |
| 200m 경계는 188m, 80m 경계는 75.5m | 188, 75.5 | 계산 | 80 + 0.9 × 120, 35 + 0.9 × 45. `Lerp(D[i], D[i+1], 1 − 0.1)` |
| 왕복 전환의 정의 | 2초 | [measurement.md](../../Docs/Guides/measurement.md) 63행 `sw_round*` | |

## 그림의 값

`py -3 -I Posts/04-quality-by-distance/make-figures.py`가 SVG 둘을 만든다. 환경 변수 `LAB_FIG_CHECK=1`을 주면 그림에 쓴 글자를 찍는다. 폭 800 기준으로 글자는 16px 이상이고 자기 흰 배경을 가진다.

### `images/steps-3-to-7.svg`

호드 총수 1만, 이동 장면, 오브 시점 카메라, 세 번 잰 중앙값(ms)이다. 프레임은 p95, 스레드는 p50이다. 그림의 프레임 p95 점에는 세 실행의 범위를 세로선으로 그렸다.

| 단계 | 프레임 p95(범위) | 게임 스레드 | 렌더 스레드 | GPU |
| --- | --- | --- | --- | --- |
| 단계 3 `V0-s3` | 14.148(13.975\~14.265) | 11.356 | 5.669 | 7.069 |
| 단계 4 `V0-s4` | 14.205(14.086\~14.37) | 11.825 | 5.626 | 7.142 |
| 단계 5 `V0-s5` | 14.516(14.513\~14.7) | 12.065 | 5.377 | 7.154 |
| 단계 6 `V0-s6` | 14.708(14.695\~14.715) | 12.31 | 3.865 | 5.599 |
| 단계 7 `V0-s7` | 15.168(15.091\~15.426) | 12.686 | 3.981 | 6.648 |

- 그림의 회색 차이 +0.06, +0.31, +0.19, +0.46과 빨간 괄호 +1.02는 위 "본문의 수치와 출처"의 행이다. 스크립트가 구별 판정을 다시 셈하고, 본문과 다르면 멈춘다.
- 그림의 "GPU −1.56"과 "GPU +1.05"도 같은 표의 행이다.
- 노란 칸은 표현을 더하는 단계(4, 6, 7)다. `full-ladder.csv`의 `kind`가 `quality`인 행과 같다.
- 그림에 적은 값 27개(프레임 p95 5, 스레드 15, 차이 7)를 `summary.csv`와 대조해 어긋남이 없었다(스크래치 스크립트, 2026-11-17). 범위의 끝 10개는 글자 없이 선으로만 그렸다. `full-ladder.csv`의 같은 열도 `summary.csv`와 같다.

### `images/distance-bands.svg`

측정값이 없는 도식이다. 칸의 값은 위 "설정값과 소스 사실"의 행이다.

| 그린 것 | 값 | 출처 |
| --- | --- | --- |
| 구간 0\~35m, 35\~80m, 80\~200m | 35, 80, 200m | `Bands`, `DefaultHydrateM`, `FarM` |
| 맵 끝과 바깥 끝 | 맵 중심 500m, 1,000m | `MapRadius`, `FLabCrowdFx::OuterRadius` |
| 여유 없음, 75.5m에서, 188m에서 | 10% | `ComputeLODFromSettings`, 계산 |
| 액터 상한 1,000, 최소 유지 시간 2초 | | `DefaultActorCap`, `DefaultRepHoldMs` |
| 색 | 선형 RGB (0.9, 0.1, 0.1), (0.1, 0.3, 0.9), (0.2, 0.8, 0.9), (0.95, 0.8, 0.1) | `LabRepTint.cpp:25-29`(지금 `v2`, W14-4가 더한 개발 전용 코드). 스크립트가 sRGB로 바꿔 칠한다 |
| 어느 단계가 더했나 | | [v2-plan.md](../../Docs/v2-plan.md) "사다리", [code-tour.md](../../Docs/Guides/code-tour.md) |

- 눈금 간격은 거리에 비례하지 않는다. 0\~200m는 카메라 거리, 맵 끝부터는 맵 중심 거리다. 카메라는 맵 중심 둘레의 오브를 따라가므로 두 기준이 같지 않다. 그림 안에 이 말을 적었다.
- 80\~200m의 그림자 끔은 단계 8의 것이다(`LabConfig.cpp:58` `bFarShadow = Step != 8`). 단계 6·7은 35\~200m가 모두 그림자 켬이다.

## 화면

| 본문의 화면 | 파일 | 찍은 실행 | 비고 |
| --- | --- | --- | --- |
| 최종 구성의 표현별 색(구조 그림) | [reptint-s8-moving-t75.jpg](../../Measurements/Full4/shots/reptint-s8-moving-t75.jpg) | Development 빌드 a4138de, 단계 8, 1만, 오브 시점 카메라, 2m/s, 75초. `-LabRepTint`. 로그 `LabDev-s8-w146s1b-N10000-P1.log` | [V2.md](../../Docs/Worklog/V2.md) "2026-11-17 W14-6 표현별 색 정지 화면" |
| 단계 5의 표현별 색 | [reptint-s5-vat.jpg](../../Measurements/Full4/shots/reptint-s5-vat.jpg) | 같은 빌드, 단계 5, 같은 인자, 75초. 로그 `LabDev-s5-w146s4-N10000-P1.log` | 같은 절 |
| 단계 7 화면(요약) | `Docs/figures/steps/step7-crowd.webp` | 촬영 실행 `Full4shot-V0-s7-N10000-P1-r1`(960×540, 15fps, 8초) | `Saved/Capture/Full4/`의 원본과 SHA-256 앞 12자 5b7908b4e4c8이 같다 |
| 단계 4의 움직이는 화면(링크) | `Docs/figures/steps/step4-vat-budget.webp` | `Full4shot-V0-s4-N10000-P1-r1` | SHA-256 앞 12자 c96f1b0f5d79 |

- 표현별 색은 머티리얼 파라미터 `UnitTint`만 바꾼 개발 전용 기능이다. Test 빌드에는 코드가 없다. 화면은 측정값이 아니고, 본문은 화면의 숫자를 인용하지 않는다.
- 단계 7 화면은 HUD를 켜고 녹화와 함께 돈 실행이라 HUD의 숫자는 측정값이 아니다([captures.csv](../../Measurements/Full4/captures.csv)).
- 구조 그림의 다른 후보는 `reptint-s8-moving-t135.jpg`(같은 인자, 135초), `reptint-s8-still.jpg`(정지 장면), `reptint-s8-topdown.jpg`(탑뷰)다. t135는 파랑 띠가 넓지만 카메라 가까운 오른쪽 아래에 파랑 ISKM이 섞여 있다. 정지 장면은 빨강이 화면 아래 절반을 덮는다. 탑뷰는 파랑 구간과 입자가 화면에 없다. 그래서 t75를 기본으로 두었다. 최종 선택은 사용자가 한다.
- 단계 4의 화면 자리에 단계 5의 표현별 색 화면을 실었다. 단계 4와 5는 표현 설명자가 같다(단계 5가 더한 것은 Simulation LOD와 엔티티 풀이다, `LabConfig.cpp:54`). 단계 4의 표현별 색 화면은 찍지 않았다.

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`에서 직접 열어 확인했다(2026-11-17). 줄임말 **ML** = `Engine/Plugins/Runtime/MassGameplay/Source/MassLOD`.

| 사실 | 위치 |
| --- | --- |
| `ComputeLODFromSettings`의 반복은 `LODDistIdx > 0`에서 멈춘다 | `ML/Public/MassLODCalculator.h:868` |
| 한 칸 가까워질 때만 여유를 둔다 | 같은 파일 `:873` `(PrevLOD - LODDistIdx) == 1` |
| 여유 거리는 `Lerp(D[i], D[i+1], 1 − 비율)` | 같은 파일 `:876` |
| 그 거리보다 멀면 옛 LOD를 남긴다 | 같은 파일 `:877-880` |
| 비율 기본값 0.1 | 같은 파일 `:271` |
| ISKM 바운드 계산의 인스턴스 반복 | `Engine/Source/Runtime/Engine/Public/InstancedSkinnedMeshComponentHelper.h:202`, `:227-235` |
| 바운드 상자가 있으면 그 값을 돌려준다 | `Engine/Source/Runtime/Engine/Private/Components/InstancedSkinnedMeshComponent.cpp:1249-1253` |
| Niagara 메시 입자의 커스텀 데이터 0번은 입자 번호 | `Engine/Plugins/FX/Niagara/Shaders/Private/NiagaraUpdateMeshGPUSceneInstances.usf:208` |

본문의 엔진 코드 발췌는 하나이고 13줄이다(868, 869, 870, 871, `// ...`, 873, 874, `// ...`, 876, 877, 878, 879, 880행). 글 하나의 엔진 코드 합계 30줄 제한 안이다. 남긴 줄은 소스 그대로이고 앞쪽 들여쓰기만 줄였다.

## 프로젝트 코드 발췌의 원본

| 발췌 | 위치(de0e21b) | 지금 `v2` |
| --- | --- | --- |
| `BuildEntityConfig`의 구간 4줄 | `Source/HordeOptLab/LabCrowdSubsystem.cpp:135-136`, `:143`. 136행 끝의 주석은 지웠다 | 208-209, 216행. 내용은 같다 |
| `ULabRepHoldProcessor::Execute` 14줄 | `Source/HordeOptLab/LabMassProcessors.cpp:1111`, `:1121-1132`. 줄인 곳은 1112\~1120, 1126, 1130행 | 같다 |
| `ULabCrowdFxGatherProcessor::Execute` 11줄 | `Source/HordeOptLab/LabCrowdFxProcessor.cpp:141-145`, `:172`, `:179`, `:242` | 같다 |

`git diff v2-full4 HEAD`로 보면 위 셋 가운데 바뀐 것은 `LabCrowdSubsystem.cpp`의 표현별 색 코드(개발 전용, `#if !UE_BUILD_TEST`)뿐이다. 발췌한 줄의 내용은 같다.

## 확인하지 않은 것

- 단계 3에서 7까지 는 게임 스레드 시간 1.33ms가 어느 프로세서의 몫인지. 단계 4\~7의 트레이스를 이 글에서 열지 않았다. 본문은 원인을 적지 않았다.
- ISKM이 렌더 스레드 시간을 1.51ms 줄인 까닭. VAT 인스턴스의 커스텀 데이터 갱신과 ISKM 갱신의 렌더 스레드 경로를 견주지 않았다. 본문은 잰 값만 적었다.
- ISKM 고정 바운드 상자를 끈 조건이 0.66ms 긴 것이 인스턴스 반복 때문인지. 엔진 코드의 사실과 잰 값을 나란히 적었을 뿐 트레이스로 잇지 않았다.
- 단계 4의 VAT 인스턴스, 액터 애니메이션 예산, 최소 유지 시간, 전환 대책 각각의 몫을 단계 4에서 가르는 조건. 최종 구성에서 끈 조건만 있다. VAT 인스턴스 자체를 끈 조건은 없다(ISKM을 끈 조건은 VAT 인스턴스로 되돌리지만 다른 것이 함께 빠지는 참고 값이다).
- 단계 4에서 예산을 끈 조건. 그래서 단계 4의 소형 건너뜀 7.02%가 프레임 시간을 얼마나 지켰는지는 모른다.
- 전환 대책의 화면 효과. 이 묶음에는 화면 튐 지표가 없다.
- 표현별 색 화면을 어두운 바탕과 본문 폭에서 읽기 쉬운지. 그림 SVG는 Edge 헤드리스로 PNG를 만들어 겹침을 봤다(밝은 바탕만).
- 링크한 글 05와 06은 있다. 글 07(`07-summary`)은 아직 없어 이 글에서 링크하지 않았다.
