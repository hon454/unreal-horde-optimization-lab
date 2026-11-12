# Q5 독자용 HUD

`-LabHud`의 HUD를 GIF를 보는 독자가 "지금 무엇을 재고 있는지" 바로 알도록 고친다(2026-11-12 사용자 결정, 경위는 [Q5.md](Q5.md) "HUD"). HUD를 끈 실행의 동작·비용, CSV 스키마·메타데이터, 러너는 그대로다.

## 진행

- 조사: 끝. 표현별 수는 기존 집계를 쓴다. 액터 = `ULabCrowdSubsystem::ActorCount`(M0은 액터 전부), 인스턴스 = VAT·ISKM 프로세서의 밴드 집계(`Lab/IsmBand*`) 합(M1은 엔진 ISM 프로세서라 미니언 총수), 입자 = `CrowdRecsLast`(`Lab/CrowdRecs`). 다음: 구현.
- 구현·빌드: 끝. `LabHud.cpp`를 다시 짰고 `Scripts/build.ps1`이 성공했다. 다음: 실행 확인.
- 실행 확인: 끝. 다섯 구성(1·3·6·8단계, 빼 보기 `crowdfx=0`)의 HUD를 2560x1440 스크린샷과 960px 사본으로 직접 봤다. HUD 없는 `-Small -Stage M5 -Variant q4final`은 `hud=0`, 종료 코드 0이다. 다음: 커밋.

## 모양

위에서부터 다음 순서다(1만 P1 최종 구성, Development 에디터 `-game`, 참고 값).

```
STEP 8 / 8   Final: quality by distance
+ actors within 40 m (cap 1,000), far shadows off

12.2 ms   target 16.7 ms (60 fps)
Game 11.3    Render 12.0    GPU 11.7 ms

Horde 10,000   distance from camera
  Actors        472   0–40 m
  Instances   3,345   40–200 m  · ISKM
  Particles   6,186   200–500 m  · Niagara
  Not drawn       0   past 200 m, hit or dying
+ backdrop particles 500–1,000 m (scenery, not counted)
[프레임 시간 그래프, 16.7 ms 선]
frame time, last 5 s
q4final · M5 · P1 · 2 types · Development build
```

- 프레임 시간 큰 글자의 색은 그래프 막대와 같은 규칙이다: 16.7ms 이하 초록, 33.3ms 이하 주황, 그 위 빨강.
- 거리 구간은 카메라 거리다. 표현 LOD가 카메라 기준이라(`LabCrowdSubsystem.cpp` 밴드 설정) 그 사실을 "distance from camera"로 적었다. 경계는 설정값에서 읽는다: 전환 거리 `HydrateM`, 그리는 끝 `FarM`, 맵 반경 `FLabScenario::MapRadius`, 배경 바깥 반경 `FLabCrowdFx::OuterRadius`, 배경 안쪽 반경 `CrowdFxInnerCm()`.
- 줄 구성은 그 실행의 실제 구성을 따른다. M0은 `Actors · all distances` 한 줄, M1은 `Instances · all distances · static mesh`, 액터가 없는 구성(M2, 빼 보기 `hydrate=0`)은 Actors 줄이 없고, 엔티티 구동 입자가 없는 구성은 Particles 줄이 없다. 인스턴스의 방식은 `static mesh`·`VAT`·`ISKM` 중 하나다(`UsesIskm()`). 그리는 끝이 있는 구성(`CullsFar()`)에는 `Not drawn` 줄이 있다.
- `Not drawn` = 총수 − 액터 − 인스턴스 − 입자(0 아래면 0). 7단계 전에는 200m 밖 전부, 7·8단계에서는 200m 밖의 멈춘 개체(피격·사망. 엔티티 구동 입자는 걷는 개체만 그린다, `LabCrowdFxProcessor.cpp`)다. 액터 준비를 기다리는 프레임에는 같은 개체가 인스턴스와 액터로 함께 세져 합이 총수를 몇 개 넘을 수 있다(8단계 472 + 3,345 + 6,186 = 10,003). 그래서 0 아래는 0으로 둔다.
- 맨 아래 작은 줄에 변형·단계·프리셋·종류 수·빌드 구성과 기본값과 다르게 준 인자(`LabOverrides`)를 둔다. 내부 변형 이름은 이 줄에만 나온다.

## 문구 표

`LabHud.cpp`의 `StepText`·`OverrideText`에 있다. 근거는 `Scripts/full-conditions.json`의 `note`와 [plan.md](../plan.md) "품질 계획"의 사다리다. 고칠 문구가 있으면 이 표와 코드를 같이 고친다.

| 단계 | 첫 줄(머리 + 제목) | 둘째 줄(앞 단계에서 바꾼 것) |
| --- | --- | --- |
| 1 | STEP 1 / 8 · All actors (baseline) | every minion is a skeletal-mesh actor |
| 1b | STEP 1b · Actors + Budget Allocator | + USkeletalMeshComponentBudgeted, 2 ms budget |
| 1c | STEP 1c · Actors + URO | + Update Rate Optimization on every actor |
| 2 | STEP 2 / 8 · Logic in Mass, static instances | + Mass entities drawn as instanced meshes, no animation |
| 3 | STEP 3 / 8 · Animated instances, cull past 200 m | + vertex animation (VAT), nothing drawn past 200 m |
| 4 | STEP 4 / 8 · Actors up close, animation budget | + actors within 30 m, Animation Budget Allocator |
| 5 | STEP 5 / 8 · Simulation LOD, entity pool | + far minions update less often, entities reused |
| 6 | STEP 6 / 8 · Skinned instances (ISKM) | + ISKM replaces VAT, fixed bounds |
| 7 | STEP 7 / 8 · Niagara crowd past 200 m | + particles where minions are not drawn |
| 8 | STEP 8 / 8 · Final: quality by distance | + actors within 40 m (cap 1,000), far shadows off |

200m·30m·40m·1,000은 설정값(`FarM`, `HydrateM`, `ActorCap`)에서 채운다. 사다리 밖 구성(`LabLadder` `-`)은 머리 없이 "단계 / 변형"을 제목으로 쓴다.

빼 보기와 하위 실험(8단계 + `LabOverrides`)은 머리를 `ABLATION`(빼기만 있을 때) 또는 `SIDE TEST`로 바꾸고, 제목을 "Final minus: <뺀 것>"(더한 것은 "Final plus: …"), 둘째 줄을 "= step 8, but <바뀐 점>"으로 쓴다.

| 인자(`LabOverrides`) | 뺀 것(제목) | 바뀐 점(둘째 줄) |
| --- | --- | --- |
| `hydrate=0` | near actors | minions within 40 m stay instances |
| `screensig_budget=0` | animation budget | every actor animates every frame |
| `fixmask=0` | hand-off fixes | no pop-in fixes between actor and instance |
| `pool=0` | entity pool | dead entities are destroyed and re-created |
| `simlod=0` | Simulation LOD | far minions update every frame |
| `iskm=0` | ISKM | instances go back to VAT |
| `fixed_bounds=0` | fixed bounds | instance bounds are recomputed every frame |
| `far_cull=0` | 200 m cull | instances are drawn to the map edge |
| `crowdfx=0` | Niagara crowd | nothing is drawn past 200 m |
| `far_shadow=1` | far shadow cut | instances past 80 m cast shadows again |
| `actor_collision=1`(더하기) | actor collision | near actors keep their capsule collision |
| `hydrate_m=<m>`, `actor_cap=<n>` | actors within <m> m, actor cap <n> | 같음 |

표에 없는 인자는 토큰을 그대로 적는다.

## 배경 입자

500\~1,000m의 독립 입자는 세지 않고 한 줄 "+ backdrop particles 500–1,000 m (scenery, not counted)"로만 적는다. 이 입자는 엔티티가 아니라 수평선을 채우는 장식이고 수는 스폰율 × 수명의 계산값(`CrowdFxParticles()`)이라, Horde 아래에 수를 두면 총수 1만과 더해 읽히기 때문이다. 줄이 없으면 독자가 지평선의 무리를 실제 개체로 오해하므로 구간과 "not counted"는 남긴다. 옛 방식(`-LabCrowdFxEntity=0`)이면 안쪽 반경이 172.8m로 바뀌어 그 값이 나온다.

## 크기

2560x1440 창을 960px GIF로 줄이면 0.375배다. 배치 배율 S는 화면 높이 / 1080이라 1440에서 1.33이다. 글꼴 크기(1080 기준 pt): 머리 14 굵게, 제목 19 굵게, 둘째 줄 15, 프레임 시간 27 굵게, 본문 15, 캡션 13. 본문은 1440에서 20pt(약 27px)이고 960px GIF에서 약 10px다. 처음 캡션 12pt는 960px 사본에서 겨우 읽혀 13pt로 올렸다. 판은 1440 창에서 약 650x640px(가로 25%, 세로 44%)이고 대부분 하늘 위에 놓인다.

## 집계와 비용

- 새 집계는 하나다. VAT·ISKM 프로세서가 이미 세는 밴드 집계 넷의 합을 `ULabCrowdSubsystem::HudInstances`에 넣는다. HUD가 보일 때만 `ALabHud`가 `bHudCounts`를 켜고(그리기마다, F1 토글로 끄면 끔), 꺼져 있으면 프로세서는 그 값을 쓰지 않는다. 측정 실행(`LabHud` 0)에서 더해진 것은 원자 불 값 읽기 하나다.
- CSV 스탯·메타데이터 키와 값, `lab_summary`, `Ladder()`(`LabLadder` 값)는 바꾸지 않았다. 사다리 이름(`FLabConfig::Ladder().Name`)은 HUD에서 더 쓰지 않는다(문구는 `LabHud.cpp`의 표).

## 확인

Development 에디터 `-game`, 2560x1440, P1, `-Duration 13 -ShotAt 10 -LabHud`. 값은 참고다.

| 구성 | N | HUD 프레임 | Actors | Instances | Particles | Not drawn | 종료 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1단계 `M0 full` | 2,000 | 77.8 ms(빨강) | 2,000 all | — | — | — | 0 |
| 3단계 `M2 vatcull` | 10,000 | 8.5 ms | — | 3,801 0–200 VAT | — | 6,199 | 0 |
| 6단계 `M5 m5bounds` | 10,000 | 10.5 ms | 381 0–30 | 3,419 30–200 ISKM | — | 6,200 | 0 |
| 8단계 `M5 q4final` | 10,000 | 12.2 ms | 472 0–40 | 3,345 40–200 ISKM | 6,186 200–500 | 0 | 0 |
| 빼 보기 `q4final -LabCrowdFx=0` | 10,000 | 10.8 ms | 471 0–40 | 3,339 40–200 ISKM | — | 6,190 | 0 |

- 각 구성의 합이 총수와 맞는다(8단계는 위 "모양"의 겹침 3개). 6단계 액터 381은 같은 실행의 `lab_summary actors=353`(끝 프레임)과 같은 범위다.
- 빼 보기는 머리 `ABLATION`, 제목 "Final minus: Niagara crowd", 둘째 줄 "= step 8, but nothing is drawn past 200 m", 맨 아래 `crowdfx=0`이 나왔다. `-Extra "-LabHud","-LabCrowdFx=0"`처럼 배열로 넘기면 쉼표로 붙어 `-LabHud`가 먹지 않으므로 한 문자열 `-Extra "-LabHud -LabCrowdFx=0"`으로 준다.
- 대표 스크린샷: [8단계 원본](../../Measurements/Q5/shots/hud-reader-q4final-N10000-P1.jpg), [8단계 960px](../../Measurements/Q5/shots/hud-reader-q4final-N10000-P1-960.jpg), [빼 보기 crowdfx=0](../../Measurements/Q5/shots/hud-reader-crowdfx0-N10000-P1.jpg).

## 확인하지 못한 것

- Test 패키지에서 보지 않았다(이 세션은 패키징하지 않는다). 빌드 이름 줄은 Test에서 "Test build"가 된다(`UE_BUILD_TEST`). 재패키징 뒤 촬영 한 장으로 확인한다.
- `Scripts/ablation-check.py --shot` 대조는 돌리지 않았다. 대조하는 메타데이터(`LabHud`, `LabShotAt`과 조건 표의 키)를 만드는 코드는 바꾸지 않았다.
- 2·4·5·7단계와 나머지 빼 보기 행의 화면은 보지 않았다. 문구와 줄 구성은 같은 코드 경로다.
