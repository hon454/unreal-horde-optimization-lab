# Unreal 호드 최적화 실험실

Night of the Dead에서 측정 없이 액터로 했던 좀비 호드 최적화를, Unreal Engine 5.8 Mass Entity로 다시 지어 거리별로 비용을 덜어내며 단계마다 수치로 확인한 기록이다. 액터 군중에서 시작해 Mass 로직, ISM + VAT, 액터 하이드레이션과 애니메이션 예산, Simulation LOD, ISKM을 한 번에 하나씩 적용하고, 같은 시나리오를 같은 방법으로 다시 잰다.

- **대상**: 클라이언트 한 대에서 그리고 움직이는 군중의 비용. Game Thread, Render Thread, GPU, 메모리.
- **최적화**: Mass Entity(Fragment·Processor), Representation LOD(액터 / ISM+VAT / ISKM / 없음), 액터 하이드레이션과 풀링, Significance Manager → Animation Budget Allocator, URO, Simulation LOD(청크 단위 가변 틱), Niagara 배경 무리(엔티티 구동 입자).
- **측정 도구**: Test 구성 패키지에서 `csvprofile`과 Unreal Insights.

마일스톤 M0\~M5로 단계를 쌓은 뒤, 지금은 목표 사양(호드 총수 1만, 60fps) 안에서 거리별 품질을 확보하는 품질 계획 Q1\~Q6을 따른다. 가까이는 액터 하이드레이션, 멀리는 ISKM, 그리는 끝(200m) 밖은 Niagara 배경 무리로 그리고, 그렇게 번 프레임 시간을 액터 수와 전환 거리에 쓴다. 최종 구성의 기법은 모두 단계별 사다리와 "기법 하나를 뺀" 표에서 이점이 보여야 한다. 이 문서와 글의 수치는 총괄 묶음 `Full1`의 값이다.

형제 프로젝트 [UE5 Dedicated Server 최적화 실험실](https://github.com/hon454/ue-dedicated-server-optimization-lab)이 서버가 보내는 비용을 다루고, 이 프로젝트는 클라이언트가 그리고 움직이는 비용을 다룬다.

## 시나리오

플레이어는 움직이지 않는다. 맵 중앙의 오브를 향해 개체가 사방에서 걸어오고, 오브는 가까운 개체를 지우고 먼 개체를 주기적으로 쏜다. 모든 거리에서 생성·피격·사망·표현 전환이 계속 일어나게 하는 벤치마크 장치다. 개체는 Epic의 무료 에셋 Paragon: Minions의 근접 미니언(소형 90%)과 시즈 미니언(대형 10%) 두 종류이고, 체력은 개체마다 60\~240 사이에서 시드로 정한다. 카메라는 고정 프리셋 두 개(오브 뒤에서 바깥을 보며 도는 오브 시점, 탑뷰)로 잰다.

## 결과 한눈에 보기

![사다리 여덟 단계: 목표 총수 1만의 프레임 시간](Docs/figures/full1/full-ladder-P1.svg)

최종 측정의 값이다. 여덟 단계와 빼 보기 조건을 Test 패키지 하나(730ff18)에서 하루에 연달아 쟀다(2026-11-12). 호드 총수 1만, 오브 시점(P1), 2560x1440 Epic, 3회 중앙값, 단위 ms다(`Measurements/Full1/summary.csv`). 사다리는 단계마다 기술 하나를 더한다. 병목은 L1·L4\~L6이 Game, L2·L3·L7·L8이 GPU다.

| 단계 | 도입한 기술 | 프레임 p50 | 프레임 p95 | Game p50 | GPU p50 | N@16.7ms |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | 액터 군중(`ACharacter` + AnimBP) | 441.46 | 466.35 | 426.93 | 50.52 | 797 |
| L2 | Mass 로직 + 애니 없는 ISM | 9.69 | 10.29 | 6.08 | 9.41 | 20,250 |
| L3 | ISM + VAT, 200m 밖 그리지 않음 | 7.71 | 8.19 | 2.91 | 7.37 | 3만 안쪽 |
| L4 | 30m 안 액터 하이드레이션(상한 500) + 애니메이션 예산 | 8.07 | 9.41 | 7.77 | 6.81 | 26,154 |
| L5 | Simulation LOD + 엔티티 풀 | 7.51 | 8.83 | 7.20 | 6.83 | 3만 안쪽 |
| L6 | ISKM + 고정 바운드 | 7.38 | 8.84 | 7.20 | 5.28 | 3만 안쪽 |
| L7 | Niagara 배경 무리(200m 밖, 엔티티 위치 구동 입자) | 12.51 | 13.03 | 7.88 | 12.11 | 13,808 |
| L8 | 품질 올리기: 액터 0\~40m(상한 1,000), 80m 밖 그림자 끔 | 11.12 | 11.64 | 7.82 | 10.71 | 16,826 |

N@16.7ms는 프레임 p95가 16.7ms 이하인 최대 개체 수다(`Docs/figures/full1/full-n-at-60fps.csv`의 보간값). L3·L5·L6은 3만에서도 16.7ms 안이라 값을 적지 않았다. L2의 2만 P1은 세 회차가 17.98 / 14.63 / 15.71ms로 범위가 3.35ms다(다른 조건은 0.4ms 안). 첫 회차에서 렌더 스레드의 `PrepareDistanceFieldScene`이 6.5ms 길었기 때문이고, L2의 N@16.7ms는 이 흔들림을 안고 있다([Worklog/Q6.md](Docs/Worklog/Q6.md)의 후속 분석 절).

L7·L8은 앞 단계에서 번 프레임 시간을 배경 무리와 액터 수·전환 거리에 쓰는 품질 단계다. L8의 프레임 p95 11.64ms는 여유 기준 13.3ms(16.7ms의 80%) 안이다.

### 빼 보기: 최종 구성(L8)에서 기법 하나를 뺀 값

같은 조건(1만 P1)에서 `L8`의 기법을 하나씩 끈 실행이다. 병목이 GPU라 Game만 달라지는 행은 프레임 시간에 드러나지 않는다.

| 뺀 것 | 프레임 p50 | 프레임 p95 | Game p50 | GPU p50 |
| --- | --- | --- | --- | --- |
| 없음(L8) | 11.12 | 11.64 | 7.82 | 10.71 |
| 액터 하이드레이션 | 11.08 | 11.54 | 3.74 | 10.74 |
| 애니메이션 예산 | 11.17 | 11.68 | 8.64 | 10.75 |
| 전환 대책 | 11.15 | 11.65 | 7.78 | 10.72 |
| 엔티티 풀 | 11.14 | 11.65 | 7.81 | 10.72 |
| Simulation LOD | 11.15 | 11.71 | 8.45 | 10.73 |
| ISKM(VAT로 되돌림) | 12.40 | 12.83 | 7.80 | 12.00 |
| 고정 바운드 | 11.13 | 11.63 | 7.99 | 10.71 |
| 200m 밖을 입자 대신 ISKM으로 그림 | 11.82 | 13.29 | 11.60 | 7.70 |
| Niagara 배경 무리 | 7.46 | 8.90 | 7.32 | 5.19 |
| 80m 밖 그림자 끔을 되돌림(켬) | 12.52 | 13.01 | 8.08 | 12.12 |

배경 무리를 빼면 프레임 p50이 3.66ms 줄고(GPU p50 −5.53ms) 게임 스레드는 0.49ms만 줄어, 입자의 비용은 GPU에 실린다. 입자 대신 ISKM으로 그리면 병목이 Game으로 옮겨 가고 프레임 p95가 1.66ms 늘어난다. ISKM 끔(+1.28ms)과 80m 밖 그림자 켬(+1.40ms)도 GPU에서 난다. 하이드레이션(Game p50 −4.07ms), 애니메이션 예산(+0.82ms), Simulation LOD(+0.64ms)는 프레임 시간으로 구별되지 않지만 Game에서 갈린다.

![오브 시점, 액터 5,000개](Measurements/M0/shots/full-N5000-P1.png)

아래 단계 표는 마일스톤의 경위이고, 위 사다리는 같은 기법을 한 빌드의 실행 모드로 다시 쌓아 잰 것이다.

| 단계 | 바꾼 것 | 태그 |
| --- | --- | --- |
| M0 기준선 | `ACharacter` + AnimBP 군중. 변형: 애니 없는 액터, CharacterMovement 틱 끔, URO, Animation Budget Allocator | `M0` |
| M1 Mass 코어 | 이동·피해·사망 로직을 Fragment와 Processor로. 표현은 애니 없는 ISM | `M1` |
| M2 ISM + VAT | 먼 개체를 AnimToTexture로 구운 애니메이션이 있는 ISM으로 | `M2` |
| M3 하이드레이션 + 예산 | 가까운 개체에 액터를 붙이고, 그 애니메이션 비용을 URO / Budget Allocator로 제한 | `M3` |
| M3b 새 기준선 | 큰 개체(시즈 미니언) 10%를 더하고 M0\~M3를 다시 잰다. 크기 보정 중요도 | `M3b` |
| M4 Simulation LOD | 먼 개체의 로직 틱 간격을 청크 단위로 늘린다 | `M4` |
| M5 ISKM | 먼 개체 표현을 ISKM + AnimSequenceTransformProvider로 바꿔 VAT와 비교 | `M5` |

## 글

| # | 제목 | 알게 된 것 |
| --- | --- | --- |
| | (작성 중) | |

## 측정 방법

- 측정 PC는 Ryzen 9 5950X, RTX 4090, 64GB, Windows 11, UE 5.8.4 소스 빌드다. 자세한 값은 [Docs/Reference/pc-specs.md](Docs/Reference/pc-specs.md)에 있다.
- 고정 시드로 초기 조건을 재현한다. 개체는 정상 상태 분포(밀도 ∝ 1/r)로 미리 놓고 15초 뒤 60초를 잰다. 궤적은 비결정적이라 3회 실행의 중앙값과 범위를 함께 적는다.
- 마일스톤 묶음에서는 개체 수 N 1k와 5k를 모든 단계에서 재고 단계마다 범위를 넓혔다. 총괄 묶음은 목표 총수 1만에서 사다리 전 단계(오브 시점·탑뷰)와 빼 보기 표를 재고, 보조로 단계별 최대 N을 잰다.
- 수치는 Test 구성 패키지에서 `csvprofile`로 얻는다. Development 빌드의 수치는 비교에 쓰지 않는다.
- 중앙값의 차이가 두 구성의 범위 중 큰 쪽보다 작으면 "구별되지 않았다"고 적는다.

규약 전체는 [Docs/Guides/measurement.md](Docs/Guides/measurement.md)에 있다.

## 측정의 한계

- PC 한 대, 해상도와 스케일러빌리티를 고정한 Test 패키지다. 다른 하드웨어의 절댓값은 다르다.
- Paragon 미니언은 실제 게임 캐릭터보다 가볍다. 상대 변화를 본다.
- Mass의 병렬 처리와 가변 프레임 시간 때문에 같은 시드도 궤적이 다르다. 범위를 함께 본다.

## 실행

단계는 한 빌드 안의 실행 모드다(`-LabStage=M0`\~`M5`). Test 패키지를 만들고 묶음 스크립트로 잰다.

```powershell
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -DryRun
powershell -ExecutionPolicy Bypass -File Scripts/run-e0.ps1
powershell -ExecutionPolicy Bypass -File Scripts/run-e1.ps1
py -3 Scripts/summarize.py --milestone M0
py -3 Scripts/plot.py
```

Fab 에셋은 저장소에 없다. 준비와 나머지 명령은 [Docs/Guides/commands.md](Docs/Guides/commands.md)에 있다.

## 구조

| 경로 | 내용 |
| --- | --- |
| `Source/HordeOptLab/` | C++ 모듈. 클래스 접두사 `Lab` |
| `Content/Lab/` | 직접 만든 콘텐츠. Fab 에셋은 커밋하지 않는다 |
| `Scripts/` | 빌드, 패키징, 벤치마크, 그래프 |
| `Docs/` | 계획, 결정 기록, 측정 규약, 엔진 소스 확인 기록 |
| `Posts/` | 글과 측정 기록 |
| `Measurements/M<N>/` | 마일스톤별 수치와 대표 스크린샷 |
