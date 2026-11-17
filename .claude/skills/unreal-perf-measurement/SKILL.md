---
name: unreal-perf-measurement
description: >
  Unreal Engine 성능 측정과 비교 절차. 최적화 전후나 설정 A/B의 프레임 시간을 Test 구성 패키지에서 재고,
  한 번에 변수 하나, 반복과 버리는 첫 실행, 같은 조건 다시 재기(A/A)로 차이를 판정하고, 병목 스레드
  (게임·렌더·GPU)를 정하고, 빨라진 조건이 같은 일을 하는지(동등성) 확인한다. CsvProfiler CSV를 조건별로
  묶어 실행별 p50·p95, 조건별 중앙값과 범위, 병목, 기준 대비 판정을 Markdown 표로 내는 스크립트가 있다.
  "성능 재 줘", "최적화 효과 확인", "프레임 p95 비교", "CSV 프로파일 비교", "왜 프레임이 안 줄지"에 쓴다.
  Use for Unreal performance benchmarking, measuring frame time, CsvProfiler / csvprofile CSV,
  FrameTime GameThreadTime RenderThreadTime GPUTime, before/after or A/B comparison, A/A noise,
  bottleneck thread, Unreal Insights trace reading, Test build profiling, performance regression check.
  셰이더 컴파일·크래시 디버깅, 재지 않고 코드만 고치는 일, Unity·Godot에는 쓰지 않는다.
---

# Unreal 성능 측정과 비교

Unreal 프로젝트에서 "무엇이 더 빠른가"를 수치로 답하는 절차다. 재는 법, 차이를 판정하는 법, 병목을 읽는 법, 빨라진 조건이 같은 일을 하는지 보는 법을 다룬다.

**프로젝트에 측정 규약 문서가 있으면 그쪽을 따른다.** 이 스킬의 값(3회, 0.5ms, 2%)은 그런 문서가 없을 때의 기본값이다.

## 쓰는 말

- **조건**: 비교하는 구성 하나. 빌드, 장면, 설정 스위치가 정해진 것이다.
- **실행**: 조건 하나를 프로세스 하나로 한 번 잰 것. CSV 파일 하나가 나온다.
- **묶음**: 같은 빌드, 같은 PC, 같은 시간대에 잰 실행의 모음. 비교는 같은 묶음 안에서만 한다.
- **프레임 p50, 프레임 p95**: 측정 창에서 프레임 시간의 50번째, 95번째 백분위수. 판정 지표는 프레임 p95다.
- **게임 스레드 시간, 렌더 스레드 시간, GPU 시간**: 스레드별 시간. 중앙값(p50)만 쓴다.
- **병목**: 세 스레드 시간 가운데 가장 긴 쪽. 프레임 시간은 이 쪽이 정한다.
- **범위**: 한 조건의 실행 값 가운데 최댓값 − 최솟값.
- **구별되지 않음**: 중앙값의 차이가 두 조건 범위 중 큰 쪽보다 작다. 또는 실행 수가 모자라 0.5ms 아래 차이를 주장할 수 없다.
- **참고**: 바뀐 변수가 둘 이상인 비교, 또는 다른 묶음의 값과 견준 비교의 지위. 결론의 근거로 쓰지 않고 그래프에 잇지 않는다.
- **A/A**: 같은 조건 다시 재기. 묶음 끝에 기준 조건을 다시 재어 그 묶음의 잡음을 본다.

## 절차

위에서부터 한다. 각 단계의 자세한 내용은 가리키는 참조 파일에 있다.

1. **질문을 정한다.** 무엇과 무엇을 견주는지, 바뀌는 변수 하나, 판정 지표(프레임 p95)를 적는다. 바뀌는 변수가 둘 이상이면 그 비교는 "참고"다.
   - 자세히: `references/judging.md` "참고로 내리는 비교"
2. **측정 빌드를 고른다.** 수치는 Test 구성 패키지에서만 잰다. 에디터와 Development 빌드는 동작 확인과 디버그 화면에만 쓴다. Test에는 `stat` 그룹, `ProfileGPU`, 디버그 드로우가 없다. 세 스레드 시간과 CsvProfiler, 트레이스는 Test에서도 된다.
   - 자세히: `references/tracing.md` "Test 구성에서 되는 것"
3. **장면을 고정한다.** 시드, 개체 수, 카메라 경로, 렌더 CVar(`r.VSync 0`, `t.MaxFPS 0`, 고정 창 해상도, `r.ScreenPercentage 100`, `r.DynamicRes.OperationMode 0`, `sg.*`)를 고정한다. 적용한 값은 CSV 메타데이터에 남긴다.
   - 자세히: `references/protocol.md` "조건 고정"
4. **캡처를 정한다.** CsvProfiler를 압축 없이(`csv.CompressionMode 0`) 워밍업 뒤에 시작한다. 장면이 주기를 가지면 측정 창은 그 한 주기다.
   - 자세히: `references/protocol.md` "캡처"
5. **재는 동안 다른 일을 하지 않는다.** 빌드, 에디터, 쿡, 브라우저, 녹화 도구를 닫는다. 시작 전에 CPU 사용률을 본다. 같은 PC에서 다른 사람이나 다른 에이전트 세션이 빌드하면 그 묶음은 버린다.
   - 자세히: `references/protocol.md` "재는 동안의 PC"
6. **반복하고 순서를 섞는다.** 조건마다 프로세스를 새로 띄워 3회 잰다. 묶음의 맨 첫 실행 하나는 버린다. 회차를 바깥 루프로 두고 짝수 회차는 조건 순서를 거꾸로 돈다. 묶음 끝에 기준 조건을 3회 다시 잰다(A/A).
   - 자세히: `references/protocol.md` "반복과 순서"
7. **실패한 실행을 버린다.** 프레임 수 100 미만, 메타데이터 불일치, 개체 수 유지 실패, 앞 창이 게임이 아님이 실패다. 종료 코드 0이어도 실패일 수 있다. 버린 실행은 새 회차 번호로 한 번 다시 잰다.
   - 자세히: `references/protocol.md` "실패한 실행", `references/troubleshooting.md`
8. **요약하고 판정한다.** `scripts/compare_csv.py`로 실행별·조건별 표와 판정을 낸다(아래 "비교 스크립트"). 중앙값의 차이가 두 조건 범위 중 큰 쪽보다 커야 "차이 있음"이다.
   - 자세히: `references/judging.md` "차이 판정"
9. **병목을 읽는다.** 세 스레드 시간 가운데 가장 긴 쪽이 병목이다. 병목이 아닌 쪽이 준 것은 프레임 이득이 아니다. 프레임이 세 스레드보다 길면 게임 스레드가 작업자 스레드를 기다린 것이다.
   - 자세히: `references/judging.md` "병목", `references/tracing.md` "프레임이 세 스레드보다 길 때"
10. **같은 일을 하는지 본다.** 빨라진 조건이 일을 덜 해서 빨라졌는지 일의 양 카운터로 확인한다. 일이 다르면 그 프레임 시간은 비용 비교에 쓰지 않는다.
    - 자세히: `references/equivalence.md`
11. **기여를 가른다.** 여러 기법을 쌓았으면 최종 구성에서 하나만 끈 조건을 같은 빌드에서 잰다. 하나를 끄면 다른 것이 함께 빠지는 조건은 "참고"다.
    - 자세히: `references/judging.md` "기법별 기여"
12. **기록한다.** 빌드 해시, 실행 이름, 세 실행의 값, 판정, 측정 PC, 엔진 버전을 남긴다. 측정 장치(스포너, 시드, 카메라, CSV 열, 조건 정의)를 바꾸면 모든 조건을 새 묶음으로 다시 잰다.
    - 자세히: `references/protocol.md` "기록과 측정 장치 동결"

## 비교 스크립트

`scripts/compare_csv.py`는 CsvProfiler가 압축 없이 쓴 CSV를 조건별로 묶어 표와 판정을 낸다. 표준 라이브러리만 쓴다(Python 3.9 이상). 스크립트를 읽지 말고 실행한다. 사용법은 `--help`에 있다.

```bash
python scripts/compare_csv.py runs/baseline runs/optimized --baseline baseline
```

Windows에서는 `py -3 -I scripts/compare_csv.py ...`로도 부른다.

- 조건 하나 = 폴더 하나다. 폴더 이름이 조건 이름이고 안의 `*.csv` 하나가 실행 하나다.
- 한 폴더에 실행이 섞여 있으면 `--cond NAME=GLOB`을 여러 번 준다. 폴더 입력과 섞어 쓸 수 있다.

```bash
python scripts/compare_csv.py --cond base="Saved/Profiling/CSV/Base-r[1-3].csv" \
  --cond opt="Saved/Profiling/CSV/Opt-r[1-3].csv" --cond aa="Saved/Profiling/CSV/Base-r1[1-3].csv" \
  --baseline base --aa aa --out compare.md
```

| 인자 | 뜻 |
| --- | --- |
| `--baseline NAME` | 기준 조건. 판정 표는 모든 조건을 이 조건과 견준다 |
| `--aa NAME` | A/A 조건. 기준과의 프레임 p95 중앙값 차이가 0.5ms 이하인지 본다 |
| `--exclude GLOB` | 버릴 실행(묶음의 첫 실행 등). 버린 파일은 출력에 이름이 남는다 |
| `--skip-seconds S`, `--skip-frames K` | 캡처가 워밍업을 포함했을 때 앞부분을 버린다. 둘 중 하나만 준다 |
| `--extra-col NAME` | 더 볼 열(`RHIThreadTime`, `GameThreadTime_CriticalPath`, 사용자 스탯)의 p50 |
| `--meta-key KEY` | 조건 사이에 같아야 할 메타데이터 키(빌드 해시 등). 다르면 주의 줄에 적는다 |
| `--min-frames`, `--min-diff-ms`, `--slow-pct` | 실패 문턱(100), 0.5ms 규칙, 느린 실행 문턱(2%)을 바꾼다 |
| `--out FILE` | 표준 출력과 함께 파일에도 쓴다 |

출력은 실행 표, 조건 표, 판정 표, A/A 줄, 주의 줄이다. 판정 규칙은 `references/judging.md` "차이 판정"과 같다. 종료 코드는 0이 정상, 2가 입력 오류(폴더 없음, 기준 조건 없음, 읽을 실행 0)다. 실패한 실행이 있어도 0이다.

결과를 사용자에게 전할 때는 판정 표의 판정 칸을 그대로 쓴다. "구별되지 않음"인 차이를 "약간 빨라졌다"로 바꾸지 않는다.

## 참조 파일

| 파일 | 이럴 때 연다 |
| --- | --- |
| `references/protocol.md` | 측정을 설계할 때. 조건 고정, 캡처 코드, 반복과 순서, 실패한 실행, 재는 동안의 PC, 기록할 메타데이터 |
| `references/judging.md` | 결과를 해석할 때. 차이 판정, A/A, 느린 실행, 병목, 게임 스레드 시간 읽기, 기법별 기여, 목표 판정식 |
| `references/equivalence.md` | 빨라진 조건이 정말 같은 일을 하는지 볼 때. 절차와 호드 벤치마크의 다섯 항목 예시 |
| `references/tracing.md` | 원인을 트레이스로 가를 때. Test 구성에서 되는 것, 프레임이 세 스레드보다 길 때, Insights 내보내기, 액터가 많은 장면 |
| `references/troubleshooting.md` | 측정이나 패키징이 이상할 때. 증상과 대처 |

## 예시 수치의 조건

참조 파일의 예시 수치(ms, 배수, 비율)는 모두 아래 조건에서 잰 것이다. 수치 옆의 "(참고값)"이 이 조건을 뜻한다. 다른 하드웨어와 장면에서는 방향만 참고한다.

| 항목 | 값 |
| --- | --- |
| 엔진 | UE 5.8.4 소스 빌드 |
| PC | AMD Ryzen 9 5950X(16코어), NVIDIA RTX 4090 24GB, DDR4 64GB, Windows 11 |
| 빌드 | Test 구성 Win64 패키지, 2560×1440 창, VSync 끔, 동적 해상도 끔 |
| 장면 | 평지에서 1만 개체의 호드가 원 궤도를 도는 목표를 사방에서 쫓는다. 카메라는 목표를 따라간다 |
| 측정 | 워밍업 60초 + 측정 180초, 같은 빌드에서 세 번 잰 중앙값 |

## 다른 스킬

Mass Entity 군중이면 프로세서별 시간을 가르는 절차(직렬 트레이스, 한 스레드로 도는 엔진 프로세서 찾기)는 `unreal-mass-crowd-optimization` 스킬의 `references/processors.md`를 본다. 그 스킬은 기법을 넣은 뒤의 측정, 판정, 동등성 검사를 이 스킬로 넘긴다.
