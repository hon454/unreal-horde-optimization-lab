# 스킬 계획안: 측정 절차와 Mass 군중 최적화

이 레포의 노하우, 트러블슈팅, 측정 결과를 Claude Code 스킬 두 개로 만드는 계획안이다. 사용자가 이 문서를 승인하면 스킬 하나당 작업 세션 하나가 이 문서대로 만든다. 이 문서는 Worklog라 레포의 이름을 쓴다. 다만 2\~4절의 description 초안, 본문 개요, 예문은 스킬에 그대로 들어갈 글이라 레포의 이름을 쓰지 않는다.

- 정해진 것: [autonomous-decisions.md](autonomous-decisions.md) "사용자 확인(2026-11-18, 스킬)"(5fb32d2). 배경은 [V2-w14-plan.md](V2-w14-plan.md) 13절이다.
- 작업 기록: [Skills.md](Skills.md).
- 모든 원본의 기준 커밋은 HEAD 5fb32d2다.

## 0. 요약

| 항목 | 측정 스킬 | 군중 스킬 |
| --- | --- | --- |
| 폴더(추천, 8절 항목 1) | `.claude/skills/unreal-perf-measurement/` | `.claude/skills/mass-crowd-optimization/` |
| 하는 일 | Unreal 성능을 Test 패키지에서 재고, 조건끼리 차이를 판정하고, 병목 스레드를 정하고, 빨라진 조건이 같은 일을 하는지 본다 | Mass 군중에 기법을 어떤 순서로 넣는지, 기법마다 언제 효과가 있고 어떻게 확인하는지, 함정과 증상별 고칠 코드 |
| 파일 | `SKILL.md`, `references/` 다섯, `scripts/compare_csv.py` | `SKILL.md`, `references/` 여섯 |
| 분량(가늠) | `SKILL.md` 약 170줄, 참조 합 약 650줄, 스크립트 약 350줄 | `SKILL.md` 약 200줄, 참조 합 약 1,100줄 |
| 넘김 | 군중 스킬의 Mass 고유 트레이스 절차(직렬 트레이스)를 가리킨다 | 적용 뒤 확인(측정, 판정, 동등성)을 측정 스킬로 넘긴다 |
| 훅 | 없다 | 없다 |

## 1. 공통 설계

### 1.1 단계적 공개

`anthropic-skills:skill-creator`의 세 층을 따른다.

1. 메타데이터(`name`, `description`)는 늘 맥락에 있다. 그래서 description이 불릴지를 정한다. 무엇을 하는지와 언제 쓰는지를 함께 쓰고, 트리거 낱말을 넉넉히 넣는다. 1,024자 안에 둔다.
2. `SKILL.md` 본문은 스킬이 불릴 때 읽힌다. 절차의 뼈대만 두고 500줄 안에 둔다(이 계획은 200줄 안).
3. `references/`는 본문이 "이럴 때 이 파일을 연다"고 가리킬 때만 읽힌다. 300줄을 넘는 참조 파일은 머리에 목차를 둔다. `scripts/`는 읽지 않고 실행한다.

### 1.2 언어와 용어

- 본문은 한국어다. description에는 한국어 문장과 영어 트리거 키워드를 함께 넣는다.
- 엔진의 기법·클래스·함수 이름은 번역하지 않는다(Mass, Representation LOD, ISM, VAT, ISKM, Animation Budget Allocator, URO, Fragment, Processor, Chunk, Simulation LOD).
- 용어는 [glossary.md](../Guides/glossary.md)를 따른다. 쓰는 말: 프레임 p50·p95, 게임 스레드 시간(중앙값), 병목, 구별되지 않음, 기법별 기여, "\~를 끈 조건", 표현을 더하는 단계, 비용을 줄이는 단계, 드로우 콜, Budget Allocator. 쓰지 않는 말: 할당기, 빼 보기, 품질 단계, 버는 단계, Game p50.
- `SKILL.md`의 YAML description에는 `~`를 쓰지 않는다("수천에서 수만"처럼 쓴다). 본문에서 범위는 `\~`로 쓴다. 레포의 `check-tilde.py`가 `.claude/` 아래도 보는지는 만드는 세션이 확인한다.

### 1.3 이 레포 이름을 걷어 내는 규칙

스킬 본문에는 이 레포의 파일, 커밋, 묶음·조건·단계 번호, `Lab` 접두사를 쓰지 않는다(결정 5). 목록은 6.2절에 있다. 바꾸는 규칙은 아래다(8절 항목 3에서 사용자가 확정한다).

| 이 레포의 것 | 스킬에서 |
| --- | --- |
| `ULabRepHoldProcessor`, `FLabSeparationSolver`, `ULabSmoothOrientationProcessor` 같은 클래스 | 접두사를 뺀 역할 이름(`URepHoldProcessor`, `FSeparationSolver`, `UParallelSmoothOrientationProcessor`)과 "예시 이름" 주석 |
| `FLabConfig::Get().bFixedSkelBounds`, `FLabQuality::FixedSkelBoundsScale` 같은 설정 | 값을 바로 쓴다(`true`, `1.1f`) |
| 코드 블록 위의 `Source/HordeOptLab/...` 줄 | 지운다. 엔진 소스 위치(`Engine/Source/...`)는 남긴다 |
| 오브, 저격 | 목표(개체가 쫓는 것), 원거리 피격 선택 |
| 근접 미니언, 시즈 미니언 | 소형 개체, 대형 개체 |
| 단계 1\~8 | 기법의 이름(액터 군중, Mass 엔진 기본 경로, 직접 짠 분리 ...) |
| `a8far` 같은 조건 | "\~를 끈 조건" |
| `Full4`, 회차 번호, 라벨 | "같은 빌드에서 세 번 잰 중앙값" |

### 1.4 레포 규칙과의 관계

이 레포 안에서도 두 스킬이 불릴 수 있다. 그때는 `AGENTS.md`와 레포 가이드가 이긴다. 두 `SKILL.md`의 첫 절에 "프로젝트에 측정 규약 문서가 있으면 그쪽을 따른다"는 한 줄을 둔다(8절 항목 8).

## 2. 측정 스킬

### 2.1 파일 구성

```text
.claude/skills/unreal-perf-measurement/
├── SKILL.md                     약 170줄. 절차 12단계와 참조 안내
├── references/
│   ├── protocol.md              약 180줄. 조건 고정, 캡처, 반복과 순서, 실패 실행, 측정 중 PC, 기록할 메타데이터
│   ├── judging.md               약 150줄. 차이 판정, A/A, 느린 실행, 병목, 스레드 시간 읽기, 목표 판정식
│   ├── equivalence.md           약 120줄. 동등성 검사 절차와 호드 벤치마크의 다섯 항목 예시
│   ├── tracing.md               약 140줄. Test 구성에서 되는 것, 프레임이 세 스레드보다 길 때, Insights 내보내기, 불린 횟수
│   └── troubleshooting.md       약 60줄. 측정과 패키징에서 겪는 증상과 대처
└── scripts/
    └── compare_csv.py           약 350줄. 3절의 명세
```

### 2.2 description 초안

```yaml
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
```

- 불리는 때: Unreal에서 무엇이 더 빠른지 재거나, CSV를 비교하거나, 프레임이 왜 안 주는지 묻거나, 군중 스킬이 확인을 넘길 때.
- 불리지 않는 때: 성능과 무관한 빌드·크래시, 측정 없이 코드 리뷰만 하는 일, 다른 엔진.

### 2.3 SKILL.md 본문 개요(절차 12단계)

각 단계는 한두 문장과 "자세히: `references/<파일>`" 한 줄이다.

1. **질문을 정한다.** 무엇과 무엇을 견주는지, 바뀌는 변수 하나, 판정 지표(프레임 p95)를 적는다. 변수가 둘 이상이면 그 비교는 "참고"다.
2. **측정 빌드를 고른다.** 수치는 Test 구성 패키지에서만 잰다. 에디터와 Development 빌드는 동작 확인과 디버그 화면에만 쓴다. Test에서는 `stat` 그룹, `ProfileGPU`, 디버그 드로우가 없다(→ `tracing.md`).
3. **장면을 고정한다.** 시드, 개체 수, 카메라 경로, 렌더 CVar(`r.VSync 0`, `t.MaxFPS 0`, 고정 창 해상도, `r.ScreenPercentage 100`, `r.DynamicRes.OperationMode 0`, `sg.*`)를 고정하고 CSV 메타데이터에 남긴다(→ `protocol.md`).
4. **캡처를 정한다.** CsvProfiler를 압축 없이(`csv.CompressionMode 0`) 워밍업 뒤에 시작한다. 측정 창은 장면의 한 주기다(→ `protocol.md`).
5. **재는 동안 다른 일을 하지 않는다.** 빌드, 에디터, 쿡, 브라우저, 녹화 도구를 닫는다. 시작 전 CPU 사용률을 본다. 같은 PC에서 다른 세션이나 사람이 빌드하면 그 묶음은 버린다.
6. **반복하고 순서를 섞는다.** 조건마다 프로세스를 새로 띄워 3회 잰다. 묶음의 맨 첫 실행 하나는 버린다. 회차를 바깥 루프로 두고 짝수 회차는 거꾸로 돈다. 묶음 끝에 기준 조건을 3회 다시 잰다(A/A).
7. **실패한 실행을 버린다.** 프레임 수 100 미만, 메타데이터 불일치, 개체 수 유지 실패, 앞 창이 게임이 아님. 종료 코드 0이어도 실패일 수 있다. 버린 실행은 새 회차 번호로 한 번 다시 잰다.
8. **요약하고 판정한다.** `scripts/compare_csv.py`로 실행별·조건별 표와 판정을 낸다. 중앙값의 차이가 두 조건 범위 중 큰 쪽보다 커야 "차이"다(→ `judging.md`).
9. **병목을 읽는다.** 세 스레드 시간 가운데 가장 긴 쪽이 병목이다. 병목이 아닌 쪽이 준 것은 프레임 이득이 아니다. 프레임이 세 스레드보다 길면 게임 스레드가 작업자를 기다린 것이다(→ `judging.md`, `tracing.md`).
10. **같은 일을 하는지 본다.** 빨라진 조건이 일을 덜 해서 빨라졌는지 일의 양 카운터로 확인한다(→ `equivalence.md`).
11. **기여를 가른다.** 여러 기법을 쌓았으면 최종 구성에서 하나만 끈 조건을 같은 빌드에서 잰다. 다른 것이 함께 빠지는 조건은 "참고"다.
12. **기록한다.** 빌드 해시, 실행 이름, 세 실행의 값, 판정, 측정 PC, 엔진 버전을 남긴다. 측정 장치(스포너, 시드, 카메라, CSV 열, 조건)를 바꾸면 모든 조건을 새로 잰다.

본문 끝에 "Mass 군중이면 프로세서별 시간은 `mass-crowd-optimization`의 `references/processors.md`(직렬 트레이스)를 본다"를 둔다.

### 2.4 참조 파일의 내용

**`protocol.md`** (약 180줄)
- 측정 세 층: 개발 확인(Development, 비교에 쓰지 않음), 작은 묶음(설계 근거, 조건마다 3회), 최종 묶음(공개 수치).
- 조건 고정 표: 빌드, 카메라, 시간(워밍업 + 측정. 주기 장면이면 측정 창 = 한 주기, 워밍업 = 주기의 3분의 1 이상), 반복, 렌더 고정, 시드, 개체 수, 측정 중 PC, CPU 클럭 기록. 각 행에 "왜"를 한 줄 붙인다(이 레포의 이유 칸을 일반화).
- 캡처 코드 예시: `csv.CompressionMode 0`, 워밍업 뒤 `FCsvProfiler::Get()->BeginCapture(-1, FString(), Label)`, 끝에 `EndCapture`. 콘솔로 할 때 `csvprofile start`/`stop`. 메타데이터는 `CSV_METADATA`로 빌드 해시, 적용한 CVar, 드라이버, 기법 스위치를 남긴다.
- 셰이더·PSO: 쿡 때 컴파일, `r.PSOPrecaching 1`, 묶음 첫 실행을 버리는 이유(드라이버 PSO 캐시).
- 모션 블러 같은 엔진 기본값은 바꾸지 않고 그대로 적는다.
- 실패 실행의 정의와 다시 재기 규칙, 연속 3회 실패면 묶음을 멈춘다.
- 결정적 시나리오의 한계: 가변 프레임 시간과 병렬 처리 때문에 궤적은 재현되지 않는다.
- 측정 장치 동결: 동결 대상 목록과 바꿨을 때 할 일.

**`judging.md`** (약 150줄)
- 지표: 프레임 p50·p95·p99, 스레드 시간은 p50만(병렬 실행의 스레드 시간에는 기다림이 섞인다), 히치(50ms 초과, p50의 2배 초과), 드로우 콜.
- 차이 판정: 중앙값 차이 > 두 조건 범위 중 큰 쪽. 0.5ms 아래의 차이를 주장하려면 두 조건을 5회로 다시 잰다. 3회 묶음의 0.5ms 아래 차이는 범위를 넘어도 "구별되지 않았다".
- "참고": 변수가 둘 이상, 다른 묶음, 장면이 다른 두 조건(이동과 정지)은 직접 견주지 않는다.
- A/A: 기준 조건을 묶음 끝에 3회. p95 중앙값 차이가 0.5ms를 넘으면 묶음을 "참고"로 내린다.
- 느린 실행: 게임 스레드와 렌더 스레드 p50이 그 조건 중앙값보다 함께 2% 넘게 길면 표시하고 한 번 다시 잰다. 시간 표류의 흔적(CPU 클럭)과 함께 본다.
- 병목: 세 스레드 p50 중 최댓값. 엔진이 게임 스레드 시간에서 기다림을 빼므로 프레임은 병목보다 조금 길다(`UnrealClient.cpp` `FViewport::Draw`의 식).
- 병목이 아닌 쪽의 감소는 "프레임에는 드러나지 않았다"로 쓴다. 줄인 몫은 병목이 바뀔 때까지의 여유다.
- 게임 스레드 시간의 차이만으로 비용을 말하지 않는다. 기다림이 찍히는 자리가 옮겨 가면 일이 같아도 값이 바뀐다.
- 목표 판정식의 꼴: p95 중앙값 + (실행 간 최대 편차) ≤ 16.7ms. 실행 하나하나도 16.7ms 이하인지 따로 센다.
- 최대 개체 수: 16.7ms를 사이에 두는 두 개체 수 사이의 로그 보간. 기능 미달 점은 뺀다.

**`equivalence.md`** (약 120줄)
- 왜: 프레임이 길면 엔진이 델타를 자른다(월드 0.4초 `MaxUndilatedFrameTime`, Mass 힘·이동 0.1초). 그래서 느린 조건은 일을 덜 한다. 프레임마다 한 번 고치는 일(분리 같은)은 프레임 시간에 따라 결과가 바뀐다.
- 절차(5.3절의 일곱 단계).
- 예시: 목표를 쫓는 호드 벤치마크의 다섯 항목(5.3절 표).
- 등급: 묶음 무효(측정 자체가 틀림), 기능 미달(그 점만 표시), 보고만(정의상 값이 다른 조건).
- 사례: 겹침 방지를 끈 조건이 프레임 p95 0.93ms 짧지만 겹친 쌍이 7배라 비용 비교에서 뺐다. 모든 개체가 액터인 1만은 프레임이 0.4초를 넘어 초당 사망이 45% 적었다.

**`tracing.md`** (약 140줄)
- 트레이스는 원인을 가르는 수단이다. 수치는 비교에 쓰지 않는다. 트레이스 실행은 측정 실행과 따로 돈다.
- Test 구성 매크로 표: `STATS` 0, `CSV_PROFILER` 1, `UE_TRACE_ENABLED` 1, `HAS_GPU_STATS` 1, `WITH_PROFILEGPU` 0, 디버그 드로우 0, `-statnamedevents`.
- 엔진 CSV 기본 열: `GameThreadTime`, `RenderThreadTime`, `GPUTime`, `RHIThreadTime`, `GameThreadTime_CriticalPath`, `RenderThreadTime_CriticalPath`.
- 프레임이 세 스레드보다 길 때(5단계): `EventWait/*` 열 합, 게임 스레드 시간 + 합 ≈ 프레임, `GameThreadTime_CriticalPath`, 트레이스의 `WaitForTasks` 끝, 그 순간에 끝나는 작업자 이벤트.
- Insights Timers 합계는 모든 스레드를 합친다. 명령줄 내보내기(`-ExecOnAnalysisCompleteCmd="TimingInsights.ExportTimerStatistics ... -threads=GameThread"`), GPU 타이머와 `Frame` 행 버리기, 프레임당 = 구간 합 ÷ `FEngineLoop::Tick` Count, 한 번 = 구간 합 ÷ 그 타이머 Count. 소스 빌드에는 `UnrealInsights.exe`가 없어 한 번 빌드한다.
- 액터 군중: 합계 = 한 번의 비용 × 불린 횟수. 50ms 넘는 프레임에서 CharacterMovement 하위 스텝.
- Mass면 군중 스킬의 직렬 트레이스로 넘긴다.

**`troubleshooting.md`** (약 60줄, 증상 → 대처)
- Test에서 `stat anim`, `ProfileGPU`, `a.Budget.Debug`가 안 된다 → 정상. Development로 찍는다.
- 벤치가 실패했는데 프로세스 종료 코드가 0이다 → Windows 비강제 종료 코드가 닿지 않는다. 실패는 CSV 메타데이터의 성공 표시와 CSV 유무로 본다.
- 쿡이나 에디터 실행 뒤 `Config/DefaultEngine.ini`가 바뀌어 빌드가 dirty가 된다 → `AndroidFileServer` 플러그인이 토큰을 쓴 것이다. `.uproject`에서 끈다. 토큰 절은 커밋하지 않는다.
- 빌드가 DLL 잠금으로 실패한다 → 게임·에디터 프로세스를 닫는다.
- 주기적 스파이크(예: 7초 주기)가 p95를 흔든다 → 브라우저 같은 백그라운드 프로세스를 닫는다. 중앙값 2배 초과 프레임의 주기성을 본다.
- 가끔 한 실행이 p95 약 0.5ms 길다 → 원인 미확정. CPU 클럭을 실행마다 남겨 가린다. 느린 실행 규칙으로 다시 잰다.
- 프레임 수가 너무 적어 실행이 버려진다 → 프레임이 400ms를 넘는 조건은 측정 창을 늘리거나 그 조건을 "참고"로 둔다.

### 2.5 절차 항목의 내용(브리프 3번)

| 항목 | 스킬에 쓸 내용 | 자리 |
| --- | --- | --- |
| Test 패키지에서만 재기 | Development는 비교에 쓰지 않는다. Test는 STATS가 없어 측정 오버헤드가 작다. 세 스레드 시간의 원천(`GGameThreadTime`, `GRenderThreadTime`, `RHIGetGPUFrameCycles()`)은 STATS 없이 쓰여 Test에서도 같다 | `SKILL.md` 2단계, `tracing.md` |
| 한 번에 변수 하나 | 한 비교에서 바꾸는 변수는 하나다. 둘 이상이면 "참고"로 적고 그래프에 잇지 않는다. 사다리처럼 쌓으면 기여는 최종 구성에서 하나씩 끈 조건으로 가른다 | `SKILL.md` 1·11단계, `judging.md` |
| 반복과 워밍업, 버리는 실행 | 조건마다 3회, 프로세스 새로. 묶음 첫 실행 1회 버림. 워밍업은 캡처하지 않는다. 회차 바깥 루프, 짝수 회차 역순 | `SKILL.md` 4·6단계, `protocol.md` |
| A/A | 묶음 끝에 기준 조건 3회. p95 중앙값 차이 0.5ms 초과면 묶음을 "참고"로 | `SKILL.md` 6단계, `judging.md` |
| 차이 판정 | 중앙값 차이 > 범위 중 큰 쪽. 0.5ms 아래는 5회. 스크립트가 같은 규칙을 쓴다 | `SKILL.md` 8단계, `judging.md`, 3절 |
| 동등성 검사 | 5.3절 | `equivalence.md` |
| 병목 스레드 | 세 스레드 p50 최댓값. 비병목 감소, 기다림이 찍히는 자리, 프레임 > 세 스레드 | `SKILL.md` 9단계, `judging.md`, `tracing.md` |
| 재는 동안 빌드·에디터를 켜지 않는다 | 절차의 한 단계(5단계)로 둔다. 훅은 넣지 않는다(결정 2). 러너가 있으면 시작 전에 빌드 도구·에디터·브라우저 프로세스를 찾아 멈추게 하라고 권한다 | `SKILL.md` 5단계, `protocol.md` |

### 2.6 동등성 검사 절차와 예시

`equivalence.md`에 들어갈 절차다.

1. 기법이 바꿀 수 있는 "일의 양"을 적는다. 개체 수, 이동량, 상호작용 수, 화면 안 개체 수, 기능의 결과(맞음·빗나감)다.
2. 각 양을 세는 카운터를 게임 코드에 둔다. 프레임마다 바뀌는 값은 CSV 사용자 스탯(`CSV_CUSTOM_STAT`), 실행 하나의 합계는 CSV 메타데이터(`CSV_METADATA`)로 남긴다.
3. 기준 조건의 실행 간 범위로 허용 폭을 정한다. 실행 간 ±0.4%면 ±1%처럼 범위의 두세 배로 둔다.
4. 정의상 값이 달라지는 조건(엔진 회피처럼 무리 모양이 바뀌는 구현, 기능을 끈 조건)은 "보고만"으로 표시한다.
5. 어긴 조건의 프레임 시간은 비용 비교에 쓰지 않고 값만 보고한다.
6. 프레임 시간이 크게 다른 두 조건은 고정 시간 간격 실행(`-benchmark -fps=` 같은)으로 카운터가 같아지는지 확인한다.
7. 기준 개체 수가 아닌 점에는 밀도를 반영한 기준(쌍의 수는 개체 수의 제곱에 비례)을 더한다.

예시(스킬 본문에 그대로 넣는다. 호드가 원 궤도를 도는 목표를 사방에서 쫓는 벤치마크, 1만 개체):

| 항목 | 기준 | 어기면 |
| --- | --- | --- |
| 살아 있는 총수 | 측정 내내 최소 = 최대 = N | 측정 자체가 틀림(묶음 무효) |
| 초당 사망, 목표 30m 안 평균 개체 수 | 같은 개체 수·장면의 기준 조건 중앙값의 ±1%. 실행 간 범위는 ±0.4%였다 | 기능 차이. 1만에서는 묶음 무효, 다른 개체 수에서는 그 점만 기능 미달 |
| 겹친 쌍(몸통이 겹친 쌍, 0.5초마다 센 평균) | 기준 조건 중앙값의 0.5배\~2배, ±1쌍 허용. 구현 사이 차이는 1.4배, 무너진 경우는 3배 이상이었다. 다른 개체 수에서는 2 × 기준 × (N ÷ 1만)² 이하 | 같다 |
| 판정용 캡슐 트레이스 | 트레이스 수 > 0, 적중 + 막힘 = 트레이스 수(빗나감 0) | 트레이스 0은 묶음 무효. 빗나감은 기능 차이 |
| 표현 왕복 전환(2초 안에 같은 경계를 되넘은 수) | 최소 유지 시간을 쓰는 조건은 초당 0.5 이하 | 기능 차이 |

## 3. 비교 스크립트 `scripts/compare_csv.py` 명세

### 3.1 입력

- 실행: `python scripts/compare_csv.py <조건 폴더> [<조건 폴더> ...] --baseline <이름>`. Windows에서는 `py -3 -I`로도 부른다.
- 조건 하나 = 폴더 하나다. 폴더 이름이 조건 이름이고, 안의 `*.csv` 하나가 실행 하나다.
- 보조 입력: `--cond NAME=GLOB`(여러 번). 한 폴더에 모든 실행이 섞여 있을 때 쓴다. 폴더 입력과 섞어 쓸 수 있다.
- `--exclude GLOB`: 버릴 실행(묶음의 첫 실행 등). 버린 파일은 출력에 이름을 적는다.
- `--aa NAME`: A/A 조건. `--baseline`과의 p95 중앙값 차이를 따로 보고한다.
- CSV 형식: CsvProfiler가 압축 없이 쓴 파일. 첫 열이 `EVENTS`인 행이 헤더이고, 캡처 중 열이 늘면 마지막 헤더가 전체 열 목록이다. `[키],값` 쌍의 행은 메타데이터다. 둘째 열이 숫자가 아닌 행은 버린다(`summarize.py` `read_profile`과 같은 규칙). `.csv.bin`은 읽지 않고 `csv.CompressionMode 0`을 쓰라고 알린다.

### 3.2 워밍업

- 기본은 캡처가 워밍업 뒤에 시작했다고 보고 모든 프레임을 쓴다(이 레포의 방식).
- `--skip-seconds S`: `FrameTime` 누적이 S초가 될 때까지의 프레임을 버린다.
- `--skip-frames K`: 앞 K프레임을 버린다. 둘을 함께 주면 오류다.
- 버린 프레임 수와 남은 측정 창 길이(초)를 실행 표에 적는다.

### 3.3 열

| 출력 이름 | CSV 열 | 실행마다 셈 |
| --- | --- | --- |
| 프레임 | `FrameTime` | p50, p95, p99, 50ms 초과 수, p50 2배 초과 수 |
| 게임 스레드 | `GameThreadTime` | p50 |
| 렌더 스레드 | `RenderThreadTime` | p50 |
| GPU | `GPUTime` | p50 |
| 더할 열 | `--extra-col NAME`(여러 번, 예: `RHIThreadTime`, `GameThreadTime_CriticalPath`, 사용자 스탯) | p50 |

- 백분위수는 선형 보간이다(`summarize.py` `percentile`과 같다. 이 레포의 `summary.csv`와 숫자가 맞아야 한다).
- `FrameTime`이 없으면 그 실행은 실패다. 다른 열이 없으면 빈칸으로 두고 알린다.
- 실행 실패: 프레임 100 미만(`--min-frames`로 바꾼다). 실패한 실행은 요약에서 빼고 표에 이유를 적는다.

### 3.4 출력(Markdown, 표준 출력. `--out FILE`이면 파일에도)

1. **실행 표**: 조건, 파일, 프레임 수, 측정 창(초), 프레임 p50·p95·p99, 세 스레드 p50, 병목, 표시(실패, 느린 실행 후보, 세 스레드보다 긴 프레임).
2. **조건 표**: 조건, 실행 수, 각 지표의 중앙값과 [최소, 최대], 병목(중앙값 기준).
3. **판정 표**: 기준과 각 조건의 프레임 p95·p50, 세 스레드 p50의 차이(조건 − 기준), 문턱(두 범위 중 큰 쪽), 판정, 병목의 변화.
4. **A/A 줄**(있을 때): 차이와 0.5ms 기준의 통과 여부.
5. **주의 줄**: 실행 수 3 미만, 섞인 메타데이터(빌드 해시 `--meta-key`가 조건 사이에 다름), 버린 실행.

### 3.5 판정 규칙

- 차이 = 조건 중앙값 − 기준 중앙값.
- 문턱 = max(기준 범위, 조건 범위). 범위는 최댓값 − 최솟값이다.
- |차이| > 문턱이고 |차이| ≥ 0.5ms면 "차이 있음(빨라짐/느려짐)".
- |차이| > 문턱이지만 |차이| < 0.5ms면 두 조건이 모두 5회 이상일 때만 "차이 있음", 아니면 "구별되지 않음(0.5ms 아래, 5회 필요)".
- 그 밖은 "구별되지 않음".
- 실행 수가 2 이하면 판정하지 않고 "실행 부족"이라 쓴다.
- 판정 지표는 프레임 p95다. 스레드 시간의 차이는 판정 없이 값만 적고, 병목이 아닌 스레드의 감소에는 "프레임 이득 아님" 표시를 단다.
- 느린 실행 후보: 한 실행의 게임 스레드와 렌더 스레드 p50이 그 조건 중앙값보다 함께 2% 넘게 길다. 표시만 하고 요약에서 빼지 않는다.
- 세 스레드보다 긴 프레임: 프레임 p50 > 1.1 × max(세 스레드 p50). `tracing.md`의 절차를 가리킨다.
- `--min-diff-ms`(기본 0.5)와 `--slow-pct`(기본 2)로 값을 바꾼다.

### 3.6 그 밖

- 표준 라이브러리만(`argparse`, `csv`, `glob`, `pathlib`, `statistics`). Python 3.9 이상.
- 종료 코드: 0 정상, 2 입력 오류(폴더 없음, 기준 조건 없음, 읽을 실행 0). 실패한 실행이 있어도 0이다.
- 약 350줄. 머리 주석에 사용법과 판정 규칙을 적는다.
- 만드는 세션의 확인(7.3절): 이 레포의 프레임 CSV로 돌려 `summary.csv`와 숫자를 맞춘다.

## 4. 군중 스킬

### 4.1 파일 구성

```text
.claude/skills/mass-crowd-optimization/
├── SKILL.md                     약 200줄. 범위, 진단 흐름, 적용 순서 표, 참조 안내
└── references/
    ├── ladder.md                약 200줄. 적용 순서 여덟 칸: 기법, 효과가 나는 때, 확인, 참고값
    ├── actors.md                약 200줄. 액터 전환 일곱 항목
    ├── instances.md             약 250줄. 인스턴스와 입자 아홉 항목(목차 포함)
    ├── processors.md            약 230줄. Mass 프로세서 일곱 항목과 직렬 트레이스, 단일 스레드 엔진 프로세서
    ├── troubleshooting.md       약 120줄. 증상 → 원인 → 고칠 코드 표
    └── results.md               약 90줄. 참고값과 해석 조건
```

항목 형식은 [mass-crowd-tips.md](../Guides/mass-crowd-tips.md)의 칸(증상, 설명, 경고·주의, 절차, 코드, 결과)을 그대로 쓴다. 결과 칸의 수치는 `results.md`의 조건을 따른다고 한 줄로 가리킨다.

### 4.2 description 초안

```yaml
name: mass-crowd-optimization
description: >
  UE5 Mass Entity로 수천에서 수만 개체의 군중(호드, 좀비 떼, RTS 유닛, 군중 NPC)을 만들거나 빠르게 할 때의
  적용 순서, 기법마다 효과가 나는 때와 확인 방법, 함정, 증상별 원인과 고칠 코드. 액터 군중에서 Mass로 옮기기,
  하이드레이션(가까운 개체만 액터), Representation LOD, ISM·VAT·ISKM, Niagara 원거리 군중, Simulation LOD,
  엔티티 풀, Animation Budget Allocator와 URO, 엔진 회피 대체, 프로세서 병렬화와 쿼리 충돌을 다룬다.
  Use for Mass Entity crowd optimization, large crowd or horde performance, MassRepresentation, actor hydration,
  instanced skinned mesh (ISKM), vertex animation texture (VAT, AnimToTexture), MassAvoidance slow,
  ParallelForEachEntityChunk, Mass processor dependencies, entity pooling, ISKM track leak, crowd LOD.
  적용 뒤의 측정과 판정은 unreal-perf-measurement 스킬을 따른다.
  개체 수가 적은 일반 AI, Behavior Tree, GAS·전투·네트워크 복제, Unity DOTS에는 쓰지 않는다.
```

- 불리는 때: Mass 군중을 만들거나 느릴 때, 군중 표현(인스턴스, VAT, ISKM, 입자)을 고를 때, 군중의 증상(사라짐, 끌림, 메모리 증가, 깜빡임)을 고칠 때.
- 불리지 않는 때: 개체 수가 적은 AI 로직, 게임플레이 기능, 다른 엔진.

### 4.3 SKILL.md 본문 개요

1. **범위를 확인한다.** 개체 수, 군중이 하는 기능(쫓기, 겹치지 않기, 가까운 개체의 판정, 피해와 사망), 목표 프레임, 카메라 거리 분포를 적는다. 기능을 바꾸는 최적화는 비교에서 뺀다.
2. **기준선을 잰다.** 지금 구현을 Test 패키지에서 잰다. 절차는 `unreal-perf-measurement`를 따른다. 동등성 카운터(총수, 사망, 목표 둘레 개체 수, 겹친 쌍)를 이때 넣는다.
3. **병목을 정한다.** 병목별 다음 행동 표를 둔다.

   | 병목 | 먼저 볼 것 | 참조 |
   | --- | --- | --- |
   | 게임 스레드, 액터 군중 | 컴포넌트 틱의 불린 횟수 × 한 번의 시간 → Mass로 옮긴다 | `ladder.md` 1\~2칸 |
   | 게임 스레드, Mass | 한 스레드로 도는 엔진 프로세서(회피), 액터 수, 직렬 트레이스의 프로세서별 시간 | `processors.md`, `actors.md` |
   | 게임 스레드가 작업자를 기다림 | 쿼리 충돌, 서브시스템 선언 충돌 | `processors.md` |
   | GPU | 움직이는 인스턴스의 Lumen, 먼 개체 거리로 지우기, 원거리 그림자 | `instances.md` |
   | 렌더 스레드 | ISKM 바운드 계산, 컴포넌트 수 | `instances.md` |

4. **적용 순서를 따른다.** 4.4절의 표를 그대로 둔다. 한 번에 한 칸만 넣고 재고 다음 칸으로 간다.
5. **넣을 때마다 확인한다.** 측정 스킬의 판정, 동등성, 병목 변화를 본다. 표현을 더하는 칸은 "프레임이 늘지 않았다"가 목표다.
6. **기여를 가른다.** 마지막에 최종 구성에서 기법 하나씩 끈 조건을 잰다(측정 스킬 11단계).
7. **함정 목록**(한 줄씩 열 개): 4.5절의 함정 칸에서 고른다.
8. **증상이 있으면** `troubleshooting.md`를 연다.

### 4.4 적용 순서(사다리)

이 레포의 단계 순서를 기본으로 둔다(8절 항목 4). 이유: 큰 구조 변화(액터 → Mass)가 뒤의 모든 기법의 전제이고, 다음으로 큰 몫이 한 스레드로 도는 엔진 프로세서였다. 표현을 더하는 칸은 비용이 늘지 않는지 보며 넣고, 액터 하나의 비용은 마지막에 줄였다. 참고값은 1만 개체, 목표가 원을 도는 장면, 목표를 따라가는 카메라에서 잰 프레임 p95다(조건은 4.7절).

| # | 칸 | 넣는 것 | 성격 | 참고값(프레임 p95) |
| --- | --- | --- | --- | --- |
| 1 | 액터 군중(기준선) | 캡슐과 이동 컴포넌트가 있는 액터 | 출발점 | 1천 19.96ms, 2천 41.3ms, 1만 665ms |
| 2 | Mass, 엔진 기본 경로 | 로직을 엔티티로. 가까운 개체(35m)만 하이드레이션 액터(상한 1,000), 나머지 인스턴스, 먼 개체(200m 밖)는 None. 움직이는 인스턴스를 Lumen 장면에서 뺀다 | 비용을 줄이는 단계 | 1만 36.8ms(게임 스레드 병목) |
| 3 | 단일 스레드 엔진 프로세서 대체 | 엔진 회피를 병렬 분리 프로세서로, 방향 보간을 병렬 사본으로 | 비용을 줄이는 단계 | 14.1ms. 분리를 엔진 회피로 되돌리면 +8.57ms |
| 4 | 애니메이션 표현 | VAT 인스턴스(바운드 넓히기), 액터 메시에 Budget Allocator, 최소 유지 시간과 전환 대책 | 표현을 더하는 단계 | 14.2ms(+0.06, 구별되지 않음) |
| 5 | 먼 개체 로직 줄이기 | Simulation LOD(먼 개체를 Off로 빼고 직접 옮김), 엔티티 풀 | 비용을 줄이는 단계 | 14.5ms(+0.31, 구별되지 않음) |
| 6 | ISKM | 중거리를 ISKM으로. 카메라를 따라가는 고정 바운드, 트랙 예약, 가산을 끈 클립 복제본 | 표현을 더하는 단계 | 14.7ms(+0.19, 구별되지 않음). GPU −1.56ms, 렌더 −1.51ms |
| 7 | 원거리 배경 무리 | 200m 밖을 Niagara 메시 입자로(VAT 프레임은 Dynamic Parameter) | 표현을 더하는 단계 | 15.2ms(+0.46, 구별되지 않음) |
| 8 | 남은 비용 줄이기 | 80m 밖 인스턴스 그림자 끔, 액터 메시 고정 바운드. 캡슐은 QueryOnly와 전용 트레이스 채널 | 비용을 줄이는 단계 | 14.2ms(−0.94). 게임 스레드 11.9, 렌더 4.12, GPU 6.16ms |

`ladder.md`에는 칸마다 "효과가 나는 때", "확인 방법", "넘어가도 되는 때"를 둔다. 예: 3칸은 트레이스에서 작업자 하나가 프레임 대부분 동안 회피를 돌 때만 효과가 있다. 5칸의 Simulation LOD는 틱 간격만 늘려서는 효과가 없고 Off로 빼야 한다. ISKM을 쓰면 엔티티 풀은 선택이 아니다.

### 4.5 기법마다 효과가 나는 때, 확인, 함정

`ladder.md`와 각 참조 파일이 나눠 담는다.

| 기법 | 효과가 나는 때 | 확인 | 함정 |
| --- | --- | --- | --- |
| 하이드레이션과 액터 상한 | 가까운 개체에 진짜 액터(캡슐, 애니)가 필요할 때. 액터 하나가 메시 틱, 애니, 캡슐 바디 이동, 위치 동기화를 매 프레임 얹는다 | 하이드레이션만 끈 조건(참고: −6.98ms, 병목이 GPU로) | 상한이 걸리면 전환 거리보다 상한이 비용을 정한다. 끈 조건은 캡슐·Budget Allocator·고정 바운드도 빠져 "참고"다 |
| 먼 개체 거리로 지우기 | 먼 개체가 많을 때. 표현이 None이 아니면 화면 밖이어도 `CanBeSeen`이라 갱신된다 | Off 구간을 끈 조건(참고: +3.54ms) | Off에서 들어오는 개체는 히스테리시스로 경계의 90% 지점에서 늦게 그려진다 |
| 움직이는 인스턴스 Lumen 제외 | 소프트웨어 Lumen 장면에서 움직이는 정지 메시 인스턴스가 수천일 때 | GPU 시간(초기 시험: 움직이는 ISM 2,000에서 29.0 → 6.0ms) | 비주얼라이저가 컴포넌트를 늦게 만들어 매 틱 훑어야 한다 |
| 인스턴스 그림자 | 인스턴스로 바뀐 개체에 그림자가 필요할 때 | 설명자 `bCastShadows`(기본 false). 원거리 그림자를 다시 켠 조건은 GPU +0.48ms | 그림자만 다른 설명자는 컴포넌트가 둘이 된다 |
| 엔진 회피 대체 | `UMassMovingAvoidanceProcessor`가 `ForEachEntityChunk`로 한 스레드에서 돌 때 | 직렬 트레이스에서 회피 몫(참고: 80.7%, 23.5ms) | Simulation LOD 태그가 없으면 전 개체가 순회에 든다 |
| 방향 보간 병렬 사본 | 한 스레드 엔진 프로세서가 위치 쓰기 사슬 앞에 있을 때 | 결과 비트 일치, 직렬 트레이스 0.82 → 0.22ms | `SetShouldAutoRegisterWithGlobalList`는 에디터에서 ini에 값을 써 버린다. CDO 리플렉션으로 끈다 |
| 쿼리의 위치 쓰기 분리 | 게임 스레드의 가벼운 프로세서가 다른 프로세서와 함께 끝날 때 | 그 프로세서 실행 시간이 거의 0으로 | `mass.ForceInlineProcessorExecution`에서 직접 쓰기·읽기는 아키타입과 무관하게 전역 충돌이다 |
| 서브시스템 선언 | 러너가 노는 빈틈의 끝이 다른 프로세서의 끝과 맞을 때 | `MassProcessingQueue Main-Thread Runner Task`의 Excl | `ThreadSafeWrite`는 이 충돌을 풀지 못한다 |
| VAT 인스턴스 | 중거리에 애니가 필요하고 GPU 여유가 있을 때 | 프레임이 늘지 않음 | AnimToTexture Bone 모드 바운드는 뼈 범위뿐이라 VSM 그림자가 남는다. Vertex 모드 정점 범위로 넓힌다 |
| Budget Allocator와 URO | 액터 애니 수요가 예산을 넘을 때만. 예산 안이면 모든 메시를 매 프레임 갱신한다 | 모든 개체가 액터: 2천 41.3 → 36.9ms, 1천은 +0.52ms | CVar만으로는 안 켜진다. 월드의 `SetEnabled(true)`가 필요하다. 켜지면 등록 메시의 URO를 끈다 |
| 최소 유지 시간 | 액터 경계에서 왕복 전환이 있을 때(엔진 히스테리시스는 액터 경계에 걸리지 않는다) | 왕복 전환 초당 4.34 → 0, 프레임 구별되지 않음 | 프로세서를 LOD 뒤, 표현 바뀌기 전에 둔다 |
| Simulation LOD | 먼 개체를 Off로 엔진 이동·조향에서 뺄 때 | 실제 틱 주기(설정 0.5초가 실제 0.13초였다) | 가변 틱은 청크 필터를 읽는 프로세서만 건너뛴다. 드나든 청크는 다음 프레임에 돈다. `ExecuteAfter(LOD)`가 없으면 주기를 놓친다. 끈 조건 −0.24ms, 구별되지 않음 |
| 엔티티 풀 | ISKM 엔티티가 죽고 다시 생길 때 | 트랙 수, GPU 메모리(960초: 풀 끔 120,428 트랙, 5,110MB 대 풀 켬 3,370MB) | 트랙이 있으면 클립 번호가 바뀔 때만 재생 데이터를 쓴다. `SetAutoPlayData`를 직접 부른다 |
| ISKM 고정 바운드 | 인스턴스가 매 프레임 움직일 때(`UpdateBounds`가 인스턴스마다 돈다) | 끈 조건 +0.66ms | 상자를 옮긴 틱에 다시 넣는다 |
| ISKM 트랙 예약 | 트랙 수가 느는 동안 한 프레임 세로로 끌릴 때 | 속도 버퍼 캡처의 튄 프레임 42 → 0 | 용량을 넘으면 다시 생긴다 |
| ISKM 가산 클립 | 피격 클립 동안 사라지거나 누울 때 | 화면 | 트랙 제공자 경로는 기준 자세를 읽지 않는다 |
| Niagara VAT | 원거리를 입자로 그릴 때 | 정지 프레임 화면 | 메시 렌더러가 커스텀 데이터 0번에 입자 인덱스를 쓴다 |
| 캡슐 QueryOnly와 전용 채널 | 가까운 개체에 판정 캡슐이 필요할 때 | 캡슐을 끈 조건(참고: −1.86ms, 남은 비용은 바디) | QueryOnly도 바디가 있어 옮길 때마다 피직스 씬에 보낸다 |
| 액터 메시 고정 바운드 | 액터 수가 많고 애니 갱신마다 바운드를 다시 셀 때 | 끈 조건 +0.98ms, 애니 2.71 → 3.63ms | 에셋 바운드가 피직스 에셋 바운드보다 작으면 컬링·메시 LOD가 바뀐다 |
| 풀 액터 정리 | Mass 액터 풀을 쓸 때 | 숨은 액터의 틱, 트레이스 적중 | 엔진 풀은 숨기기만 한다. `PrepareForPooling`에서 틱·중요도·콜리전을 끈다 |
| 결정적 해시 | 일련번호와 틱으로 대상을 고를 때 | 뽑힌 순위의 평균 | `HashCombineFast`는 섞지 않는다. `MurmurFinalize32`로 섞는다 |
| 캡슐 라인 트레이스 | 캡슐 중심을 겨눌 때 | 빗나감 수 | 시작점이 캡슐의 정면 평면 위면 놓친다(1.67%). 몇 cm 비킨다 |

### 4.6 트러블슈팅(`troubleshooting.md`)

증상 → 원인 → 고칠 것 → 자세히(참조 파일 항목). 행은 아래 스물이다.

| 증상 | 원인 | 고칠 것 |
| --- | --- | --- |
| Mass로 옮겼는데 게임 스레드가 그대로다 | 하이드레이션 액터의 고정비, 액터 상한 | 액터 수와 하이드레이션을 끈 조건으로 몫을 잰다 |
| 작업자 하나가 프레임 내내 한 프로세서를 돈다 | `ForEachEntityChunk` 엔진 프로세서(회피, 방향 보간) | 병렬 분리 프로세서, `ParallelForEachEntityChunk` 사본 |
| 가벼운 게임 스레드 프로세서가 오래 걸린다 | 주 쿼리의 위치 ReadWrite가 전역 충돌 | 드문 쓰기를 따로 된 쿼리로 |
| 게임 스레드 러너가 논다 | 프로세서 단위 서브시스템 쓰기 선언 | 꼭 필요한 프로세서에만 선언 |
| 병렬 트레이스에서 한 프로세서가 비싸 보인다 | 기다린 시간이 실행 시간에 잡힌다 | `mass.FullyParallel 0` 직렬 트레이스 |
| Simulation LOD를 켜도 프레임이 그대로다 | 가변 틱은 일부 프로세서만 건너뛴다 | Off로 빼고 직접 옮기기, `UMassOffLODNavigationProcessor` 끄기 |
| 가변 틱 프로세서가 주기를 놓친다 | LOD 뒤에 돌지 않는다 | `ExecuteAfter(ProcessorGroupNames::LOD)` |
| 엔진 프로세서를 끈 설정이 ini에 남는다 | `SetShouldAutoRegisterWithGlobalList`가 에디터에서 저장 | CDO의 `bAutoRegisterWithProcessingPhases`를 리플렉션으로 |
| 액터 경계에서 개체가 깜빡인다 | 액터 경계에 히스테리시스가 없다 | 최소 유지 시간 프로세서 |
| 먼 개체가 경계 안쪽에서 늦게 나타난다 | Off 청크 재검사 0.5초와 히스테리시스 | 알려진 동작으로 둔다 |
| 인스턴스가 늘자 GPU가 크게 는다 | 소프트웨어 Lumen 장면 갱신 | `SetAffectDynamicIndirectLighting(false)` |
| 인스턴스로 바뀐 개체에 그림자가 없다 | 설명자 `bCastShadows` 기본 false | 설명자에서 켠다 |
| ISKM 트랙과 GPU 메모리가 계속 는다 | 움직이는 엔티티 파괴 때 `DeallocateTrack`이 불리지 않는다(5.8.4, 포럼 링크) | 엔티티 풀, `SetAutoPlayData` |
| ISKM 개체 전체가 한 프레임 세로로 끌린다 | 트랙 수 변화 때 뼈대 버퍼 재할당이 겹친다 | 처음에 용량까지 트랙 할당 후 해제 |
| ISKM 개체가 피격 클립에서 눕거나 사라진다 | 가산 클립을 트랙 제공자가 그대로 쓴다 | `AAT_None` 복제본 |
| VAT 개체의 그림자가 바닥에 남는다 | Bone 모드 바운드가 정점 범위보다 작다 | Vertex 모드 범위로 바운드 확장 |
| Niagara 입자의 VAT 프레임이 엉뚱하다 | 커스텀 데이터 0번이 입자 인덱스 | Dynamic Parameter 1번 |
| Budget Allocator가 켜지지 않는다 | CVar만 넣었다 | 월드 `SetEnabled(true)` |
| 풀에 돌아간 액터가 틱하거나 트레이스에 맞는다 | 엔진 풀은 숨기기만 한다 | `PrepareForPooling`/`PrepareForGame` |
| 해시로 고르면 작은 일련번호만 뽑힌다 | `HashCombineFast`가 순서를 못 바꾼다 | `MurmurFinalize32` |

더해 둘: 캡슐 라인 트레이스가 일정 비율로 놓친다(시작점 비키기). 캐릭터가 배치 직후 래그돌처럼 늘어진다(피직스 에셋 바디의 Physics Type을 Default로. 브랜드 이름은 쓰지 않는다).

### 4.7 참고값과 해석 조건(`results.md`)

`results.md` 머리에 아래 조건을 둔다. 스킬 본문의 모든 수치는 이 조건이다. 다른 하드웨어와 장면에서는 방향만 참고하라고 적는다.

| 조건 | 값 |
| --- | --- |
| 엔진 | UE 5.8.4 소스 빌드 |
| PC | AMD Ryzen 9 5950X(16코어), NVIDIA RTX 4090 24GB, DDR4 64GB(2666MT/s), Windows 11 |
| 빌드 | Test 구성 Win64 패키지 |
| 화면 | 2560×1440 창, VSync 끔, 동적 해상도 끔, Lumen·VSM·TSR 켬, 모션 블러 기본 |
| 장면 | 반경 500m 평지, 장애물 없음. 개체가 원 궤도(반지름 57.3m, 2m/s)를 도는 목표를 쫓는다. 소형 90%, 대형 10%. 가까운 개체는 피해로, 먼 개체는 원거리 선택으로 죽고 가장자리에서 다시 생긴다. 총수 유지 |
| 카메라 | 목표를 뒤 15m, 높이 9m에서 따라간다 |
| 개체 수 | 1만(따로 적지 않으면) |
| 측정 | 워밍업 60초 + 측정 180초, 세 번 잰 중앙값 |
| 지표 | 프레임 p95(판정), 스레드 시간 p50 |
| 표현 거리 | 0\~35m 액터, 35\~200m ISKM, 200m 밖 Niagara 입자 |

표 둘을 싣는다.

1. 적용 순서 여덟 칸의 프레임 p50·p95, 세 스레드 p50, 병목(4.4절의 값).
2. 최종 구성에서 하나씩 끈 조건 열일곱 가운데 구별된 일곱(하이드레이션 −6.98 참고, 200m 밖 지우기 +3.54, 캡슐 −1.86, 액터 메시 고정 바운드 +0.98, 겹침 방지 −0.93 참고, ISKM −0.67, ISKM 고정 바운드 +0.66)과 나머지 열은 "구별되지 않음"으로 한 줄. 최종 구성 기준은 14.23ms, 범위 14.04\~14.43이다.
3. 액터 군중 최대 개체 수의 감(500 10.8ms, 1천 20.0ms, 2천 41.3ms)과 최종 구성 최대 개체 수(약 1만 2천).

### 4.8 참조 파일의 내용

- **`ladder.md`**: 4.4절의 여덟 칸. 칸마다 넣는 것, 효과가 나는 때, 확인 방법(측정 스킬의 어느 단계), 넘어가도 되는 때, 참고값. 끝에 "기능 넷을 지키는가"(총수, 겹침, 판정, 피해) 점검표.
- **`actors.md`**: 하이드레이션 몫 재기, 최소 유지 시간, 풀 액터 정리, URO와 Budget Allocator, 캡슐 QueryOnly와 전용 채널, 액터 메시 고정 바운드, 캡슐 라인 트레이스. 일곱 항목.
- **`instances.md`**: 거리로 지우기, Lumen 제외, 설명자 그림자, ISKM 고정 바운드, 엔티티 풀, 트랙 예약, 가산 복제본, AnimToTexture 바운드, Niagara Dynamic Parameter. 아홉 항목. 머리에 목차.
- **`processors.md`**: 한 스레드 엔진 프로세서 찾기, 병렬 사본, 위치 쓰기 쿼리 분리, 서브시스템 선언, 직렬 트레이스, Simulation LOD는 쿼리에서 빼기, 결정적 해시. 일곱 항목. 엔진 소스 위치(`MassProcessingPhaseManager.cpp`의 `mass.FullyParallel` 등)를 남긴다.
- **`troubleshooting.md`**: 4.6절의 표.
- **`results.md`**: 4.7절.

## 5. 출처 표

모든 원본의 커밋은 HEAD 5fb32d2다.

| 스킬 파일 | 원본 문서(절) |
| --- | --- |
| `unreal-perf-measurement/SKILL.md` | [measurement.md](../Guides/measurement.md) "벤치마크 조건", "차이 판정", "함정"; [AGENTS.md](../../AGENTS.md) "측정과 비교"; [ADR-0001](../Decisions/0001-test-package-for-measurement.md), [ADR-0002](../Decisions/0002-freeze-measurement-infra.md), [ADR-0006](../Decisions/0006-three-measurement-tiers.md); [mass-crowd-tips.md](../Guides/mass-crowd-tips.md) "프로파일링할 때" 머리의 순서 |
| `.../references/protocol.md` | measurement.md "시나리오"(시드와 한계), "벤치마크 조건" 표, "산출물", "함정", "측정 인프라 동결"; [commands.md](../Guides/commands.md) 119\~120행(csvprofile 순서); [engine-notes.md](../Reference/engine-notes.md) 6절(CSV 압축); [ADR-0007](../Decisions/0007-keep-default-motion-blur.md); `Source/HordeOptLab/LabGameMode.cpp` 440, 534행(압축 끔, `BeginCapture`) |
| `.../references/judging.md` | measurement.md "지표", "차이 판정", "묶음 판정"(A/A, 단계 8의 모든 실행, 느린 실행, 연속 실패), "함정"; mass-crowd-tips.md "프레임 p95와 세 스레드 시간을 함께 보고 병목을 정한다", "병목이 아닌 스레드가 준 것은...", "게임 스레드 시간의 차이만으로..."; [glossary.md](../Guides/glossary.md) 1.1절; [ADR-0019](../Decisions/0019-step8-actor-cap-distance-and-fixed-skel-bounds.md) 결정 3(목표 판정식) |
| `.../references/equivalence.md` | measurement.md "총괄 묶음 규약" 동등성 행, 겹친 쌍 기준, 총수 사이 기준, 실패 등급; `Scripts/equivalence-check.py` 머리 주석; mass-crowd-tips.md "빨라진 조건이 같은 일을 하는지 본다", "기법 하나만 끄고 재서 기여를 가른다" |
| `.../references/tracing.md` | mass-crowd-tips.md "프레임이 세 스레드보다 길면...", "액터 군중은 불린 횟수와...", "Insights의 Timers 합계는..."; engine-notes.md 6절, 7절; measurement.md "산출물" 트레이스 행 |
| `.../references/troubleshooting.md` | [troubleshooting.md](../Guides/troubleshooting.md)의 Test 스탯, 종료 코드, 쿡 ini 변경, DLL 잠금, 프레임 수 행; engine-notes.md 30절, 50절; measurement.md "벤치마크 조건" 측정 중 행(브라우저 스파이크), "함정"(0.5ms 긴 실행) |
| `.../scripts/compare_csv.py` | `Scripts/summarize.py` `read_profile`, `column`, `percentile`, `bottleneck`, `summarize_run`; measurement.md "차이 판정", "묶음 판정" 느린 실행 행; engine-notes.md 6절(엔진 CSV 기본 스탯) |
| `mass-crowd-optimization/SKILL.md` | [v2-plan.md](../v2-plan.md) "사다리"; mass-crowd-tips.md "읽는 법", 목차, "프로파일링할 때" 머리; [code-tour.md](../Guides/code-tour.md) "요약과 읽는 순서"; measurement.md "시나리오"(기능 넷) |
| `.../references/ladder.md` | v2-plan.md "사다리"; `Measurements/Full4/summary.csv`의 사다리 행(`V0-s1`\~`V0-s8`, 1만, P1); glossary.md 1.3, 1.4절; measurement.md "함정"(단계 2의 뭉침) |
| `.../references/actors.md` | mass-crowd-tips.md "액터 전환을 만들 때" 일곱 항목; engine-notes.md 4, 19, 28, 29, 38, 40, 44, 52, 53절 |
| `.../references/instances.md` | mass-crowd-tips.md "인스턴스와 입자를 쓸 때" 아홉 항목; engine-notes.md 17, 22, 41, 42, 46절 |
| `.../references/processors.md` | mass-crowd-tips.md "Mass 프로세서를 짤 때" 일곱 항목; engine-notes.md 2, 5, 13, 34\~37, 47\~49절 |
| `.../references/troubleshooting.md` | mass-crowd-tips.md 각 항목의 증상 칸; troubleshooting.md의 풀 액터 Budget Allocator 행, 래그돌 행 |
| `.../references/results.md` | `Measurements/Full4/summary.csv`(패키지 de0e21b, 태그 `v2-full4`); [pc-specs.md](../Reference/pc-specs.md); measurement.md "벤치마크 조건", "확정값"; `Measurements/Pool2/`(엔티티 풀 긴 실행) |

## 6. 스킬에 넣지 않는 것

### 6.1 넣지 않는 것과 이유

| 넣지 않는 것 | 이유 |
| --- | --- |
| 러너(`run-full.ps1`, `run-pool.ps1`), 조건 표(`full-conditions.json`), 라벨과 회차 번호 규칙, `batch_rules.py` | 이 레포의 게임 인자와 폴더 구조에 묶여 있다(결정 3의 버린 대안). 절차의 원리(반복, 순서 섞기, A/A, 재측정)만 넣는다 |
| `equivalence-check.py`, `batch-check.py`, `spikes.py`, `ablation-check.py` | 게임의 메타데이터 키와 조건 표를 읽는다. 동등성은 절차와 예시로, 스파이크는 규칙 한 줄로 쓴다 |
| `bench_guard.py`와 다른 훅 | 결정 2. 여러 세션이 한 체크아웃을 쓰는 운영 방식에서 나왔다 |
| 여러 세션 운영, 코디네이터, Worklog·STATUS 규칙 | 세 번째 후보(운영 방식 스킬)의 몫이다 |
| 글·README·사이트·그림 규칙(posting.md, ADR-0014, 0015), 물결표 규칙 | 공개 문서 작성 규칙이다 |
| 시나리오 고정값의 세부(저격 2초 5명, 피해 반경과 값, 재생성 자리, 맵 밖 무리) | 이 벤치마크의 설계다. 참고값의 해석 조건(4.7절)만 넣는다 |
| HUD, 촬영(`capture-full.ps1`, ffmpeg DPI), 대표 프레임 고르기 | 측정이 아니라 글 재료다 |
| AnimToTexture 굽기 스크립트 전체, ISKM 에셋 스크립트 전체, Niagara MCP 우회 | 에셋 경로와 MCP에 묶여 있다. 설정 단계와 핵심 줄만 넣는다 |
| 엔진 레지스트리 GUID, 런처 엔진과 소스 빌드 섞기, AssetHost 커맨드렛 | 이 PC의 설치 문제다 |
| `v1`의 수치와 묶음(`Full1`\~`Full3`, `Ref1`, `Pool1`) | `v2` 값과 섞지 않는다 |
| 원인을 가르지 않은 관찰의 해석(Lumen 패스별 시간, 화면 밖만 지우는 변형이 이득이 없던 이유, 액터 1만에서 개체당 시간이 는 이유) | 확정한 것만 쓴다. 쓸 때는 "원인 미확정"으로 한 줄만 쓴다(0.5ms 긴 실행, 캡슐 레이캐스트의 분기) |

### 6.2 걷어 낼 이 레포의 이름

만드는 세션은 끝에 아래 정규식으로 스킬 폴더를 검사하고 0건을 확인한다.

| 종류 | 이름 |
| --- | --- |
| 프로젝트, 모듈, 접두사 | `HordeOptLab`, `Lab`으로 시작하는 식별자(`ULab*`, `ALab*`, `FLab*`, `Lab*` 함수), `Lab/` 스탯, `LabOk` 같은 메타데이터 키, "Lab은" |
| 실행 인자 | `-LabStage=`, `-LabN=`, `-LabFullyParallel=`, `-LabRepHold=` 등 모든 `-Lab*` |
| 단계와 조건 | 단계 1\~8의 번호 이름, `s1`\~`s8`, `s1opt`, `s8av`, `s8iso`, `a8*`(17개), `L1`\~`L8`, `A8-*`, `V0`, `V1`, `P1`, `P2`, `r0`, 회차 11\~13 |
| 묶음 | `Full1`\~`Full4`, `Pool1`, `Pool2`, `Ref1`, `Budget1`, `Mgr1`, `V2W*`, `Reh2` |
| 계획 식별자 | `M0`\~`M5`, `M3b`, `Q1`\~`Q8`, `W1`\~`W14`, `D33`\~`D61`, `R1`\~`R7`, `E1`\~`E5`, ADR 번호 |
| 파일과 경로 | `Source/HordeOptLab/...`, `Scripts/*.ps1`, `Scripts/*.py`, `Measurements/`, `Docs/...`, `Posts/...`, `Content/Lab/` |
| 커밋과 태그 | 해시(5fb32d2, de0e21b 등), `v1`, `v2`, `v2-full4` |
| 장면 용어 | 오브, 저격, 근접 미니언, 시즈 미니언, Paragon, Fab 팩 이름 |

검사 명령(만드는 세션이 쓴다):

```bash
grep -rnE 'HordeOptLab|\bU?A?F?Lab[A-Z]|-Lab[A-Z]|\bFull[0-9]|\bPool[0-9]|\ba8[a-z]+|\bs[1-8](opt|av|iso)?\b|\bV[01]-|오브|저격|미니언|Paragon|5fb32d2|de0e21b' .claude/skills/
```

`s[1-8]` 패턴은 보통 낱말과 겹칠 수 있다. 걸린 줄은 사람이 본다.

## 7. 완료 확인

### 7.1 새 세션에서 던질 질문

각 질문은 스킬을 만든 뒤 새 Claude Code 세션(이 레포가 아닌 빈 UE 프로젝트 폴더에 `.claude/skills/`를 복사한 곳)에서 던진다. 평가 루프는 돌리지 않는다. 사람이 동작을 보고 표에 적는다.

| # | 질문 | 불려야 할 스킬 | 기대하는 동작 |
| --- | --- | --- | --- |
| 1 | UE5 Mass로 좀비 2만 마리를 만들었는데 프레임이 30ms야. 뭐부터 해야 해? | 군중(→ 측정) | 먼저 Test 패키지 기준선과 병목을 정하라고 한다. 게임 스레드면 한 스레드로 도는 엔진 회피를 첫 의심으로 든다. 적용 순서 표를 순서대로 낸다 |
| 2 | In what order should I apply optimizations to a 10k-agent Mass crowd? | 군중 | 여덟 칸의 순서와 칸마다 효과가 나는 때, 확인 방법을 낸다. 확인은 측정 스킬로 넘긴다 |
| 3 | ISKM으로 그린 군중이 죽고 다시 생길수록 GPU 메모리가 계속 늘어 | 군중 | 엔티티 풀, `DeallocateTrack` 경로, 포럼 링크, `SetAutoPlayData`를 든다 |
| 4 | My ISKM units lie down or vanish while playing the hit reaction | 군중 | 가산 클립을 `AAT_None` 복제본으로 바꾸라고 한다 |
| 5 | Simulation LOD를 켰는데 프레임이 안 줄어 | 군중 | Off로 빼기, `ShouldTickChunkThisFrame`, `ExecuteAfter(LOD)`를 든다 |
| 6 | 이 최적화가 효과 있는지 제대로 재고 싶어 | 측정 | Test 패키지, 변수 하나, 3회와 버리는 첫 실행, 순서 섞기, A/A, 재는 동안 빌드·에디터 끔을 순서대로 낸다 |
| 7 | Compare these CsvProfiler CSVs: baseline/ vs optimized/ | 측정 | `compare_csv.py`를 돌리고 표와 판정을 범위 규칙으로 설명한다 |
| 8 | GPU 시간이 2ms 줄었는데 프레임은 그대로야 | 측정 | 병목이 아닌 쪽의 감소라 프레임 이득이 아니라고 답한다 |
| 9 | Frame time is longer than game, render and GPU times. Why? | 측정 | `EventWait` 합, `GameThreadTime_CriticalPath`, `WaitForTasks` 절차를 낸다 |
| 10 | 분리를 끄니까 0.9ms 빨라졌어. 이거 써도 돼? | 측정 | 동등성(겹친 쌍)을 먼저 보라고 한다 |
| 11 | Mass 엔티티에 GAS 어빌리티를 붙이는 법 | 없음 | 두 스킬 모두 불리지 않는다 |
| 12 | Unity DOTS로 군중 최적화하는 법 | 없음 | 두 스킬 모두 불리지 않는다 |
| 13 | 블루프린트 컴파일 에러 고쳐 줘 | 없음 | 두 스킬 모두 불리지 않는다 |

### 7.2 내용 검사

1. 6.2절의 검사 명령이 0건이거나, 걸린 줄을 사람이 보고 이 레포 이름이 아님을 확인한다.
2. 두 `SKILL.md`의 frontmatter가 `name`과 `description`을 갖고 description이 1,024자 안이다.
3. `SKILL.md`가 가리키는 참조 파일이 모두 있다.
4. 수치마다 4.7절 조건을 따른다는 표시가 있다.

### 7.3 스크립트 검사(측정 스킬 세션)

이 레포의 프레임 CSV(`Saved/Profiling/CSV/Full4-*.csv`, git 밖)로 돌린다. 스크래치 폴더에 조건별 링크나 복사본을 만든다.

1. `Full4-V0-s8-N10000-P1-r1\~r3`을 기준, `Full4-V0-a8far-...-r1\~r3`을 조건으로 준다.
2. 조건 표의 프레임 p95 중앙값이 `summary.csv`의 `V0-s8` 14.231, `V0-a8far` 17.767과 맞는다(소수 셋째 자리).
3. 판정이 "차이 있음(느려짐)"이다. `V0-a8fx`(13.922)는 "구별되지 않음"이다.
4. `--aa`로 r11\~r13을 주면 차이 0.08ms, 통과다(`V0-s8-aa` 14.31).
5. `--skip-frames 100`이 프레임 수를 100 줄인다.
6. 빈 폴더, 기준 없음에서 종료 코드 2다.

## 8. 사용자가 정할 남은 항목

한 번에 하나씩 묻는다. 각 항목의 첫 선택지가 추천안이다.

**항목 1. 스킬 폴더 이름**
- 추천: `unreal-perf-measurement`, `mass-crowd-optimization`. 측정 절차는 Mass와 무관하게 Unreal 전체에 쓰이고(Test 구성, CsvProfiler 열), 군중 기법은 모두 Mass API에 묶여 있어서다. 이름이 description의 트리거 범위와 맞는다.
- `ue-perf-measurement`, `ue-mass-crowd`: 짧다. "ue"가 description 밖에서 덜 읽힌다.
- `perf-measurement`, `crowd-optimization`: 다른 엔진의 질문에도 이름이 맞아 보여 잘못 불릴 수 있다.

**항목 2. 측정 결과를 얼마나 넣나**
- 추천: `results.md`에 적용 순서 여덟 칸, 구별된 일곱 조건, 액터 군중의 감만 넣는다(4.7절). 기법별 기여 열일곱 중 열은 구별되지 않아 숫자보다 "구별되지 않음" 한 줄이 덜 오해된다.
- 열일곱 조건과 정지 장면까지 모두: 찾을 수 있는 값이 늘지만 약 60줄이 늘고, 다른 프로젝트에서 0.3ms 차이를 일반화할 위험이 있다.
- 수치 없이 방향만: 가장 짧다. "얼마나 큰가"를 판단할 근거가 없어진다(사용자가 결정 5에서 참고값을 넣기로 했다).

**항목 3. 코드 예시의 이름**
- 추천: `Lab` 접두사를 빼고 역할 이름으로 쓰고 "예시 이름" 주석을 단다(1.3절 표). 엔진 클래스와 섞여도 역할이 읽히고, 복사해 쓸 때 자기 접두사를 붙이기 쉽다.
- `My` 접두사(`UMyRepHoldProcessor`): 예시라는 표시가 분명하다. 이름이 길고 문서가 덜 읽힌다.
- `Lab`을 남긴다: 원본과 맞추기 쉽다. 결정 5(이 레포 이름을 쓰지 않는다)와 어긋난다.

**항목 4. 적용 순서의 꼴**
- 추천: 이 레포의 단계 순서(4.4절)를 기본으로 두고, 그 앞에 "병목별 다음 행동" 표를 둔다. 순서는 실제로 쟀고 칸마다 참고값이 있다. 진단 표가 다른 장면에서 순서를 벗어날 근거를 준다.
- 기여 크기 순(회피 대체 → 하이드레이션 몫 → 먼 개체 지우기 → 캡슐 → 고정 바운드): 큰 것부터라 빨리 번다. 하지만 기여는 최종 구성에서 잰 값이라 앞 칸이 없는 상태의 크기와 다르다.
- 진단 흐름만(순서 없음): 장면에 가장 맞는다. 처음 하는 사람에게 출발점이 없다.

**항목 5. 프로파일링 팁 여덟의 자리**
- 추천: 일반적인 것(병목 정하기, 비병목 감소, 게임 스레드 시간 읽기, 프레임 > 세 스레드, Insights 내보내기, 액터 군중 불린 횟수, 기법별 기여, 동등성)은 측정 스킬에, Mass 고유인 것(직렬 트레이스, 한 스레드 엔진 프로세서)은 군중 스킬에 둔다. 측정 스킬이 Mass 밖에서도 쓰이고, 군중 스킬은 확인을 측정 스킬로 넘기기로 했다(결정 3).
- 모두 군중 스킬에: 원본 문서의 묶음과 같다. 측정 스킬에서 병목 해석이 빠진다.
- 두 스킬에 겹쳐 둔다: 어느 쪽이 불려도 답한다. 정제본 둘을 함께 고쳐야 한다.

**항목 6. 비교 스크립트의 입력**
- 추천: 조건 하나 = 폴더 하나, 보조로 `--cond NAME=GLOB`. 폴더 입력은 어떤 러너에도 맞고, 이 레포처럼 한 폴더에 섞인 CSV는 글롭으로 받는다.
- 라벨 정규식(`--label-re`): 이름 규칙이 있는 프로젝트에는 편하다. 처음 쓰는 사람에게 정규식을 요구한다.
- 묶음 정의 파일(JSON): 재현성이 좋다. 파일 형식을 하나 더 정해야 하고 결정 3의 "스크립트 하나" 범위를 넘는다.

**항목 7. 비교 스크립트의 실행 명령**
- 추천: `SKILL.md`에 `python scripts/compare_csv.py ...`를 쓰고 Windows는 `py -3 -I`도 된다고 한 줄 적는다. 다른 프로젝트 PC에서 어느 쪽이 있는지 모른다.
- `py -3 -I`만: 이 레포의 규칙과 같다. Windows 밖에서 안 된다.

**항목 8. 레포 규칙과의 관계**
- 추천: 두 `SKILL.md` 첫 절에 "프로젝트에 측정 규약 문서가 있으면 그쪽을 따른다"를 둔다(1.4절). 이 레포에서는 `measurement.md`가, 다른 프로젝트에서는 그 프로젝트 규칙이 이긴다.
- 넣지 않는다: 짧다. 이 레포 안에서 스킬의 일반 규칙(예: 3회)이 레포 규칙과 다를 때 어느 쪽인지 모호하다.

**항목 9. 작업 세션의 모델**
- 추천: 군중 스킬은 Opus high(31항목을 일반화하며 이름을 걷어 내고 코드 예시를 고친다), 측정 스킬은 Opus high(스크립트를 새로 쓰고 판정 규칙을 맞춘다). AGENTS.md는 설계가 갈리는 글과 코드를 Opus high로 둔다.
- 측정 스킬만 Sonnet medium: 명세가 정해져 있어 싸다. 스크립트의 경계 사례(열이 느는 CSV, 0.5ms 규칙)를 놓칠 위험이 있다.

## 9. 다음 순서

1. 사용자가 8절 항목을 정한다. 코디네이터가 결정을 [autonomous-decisions.md](autonomous-decisions.md)에 적는다.
2. 코디네이터가 작업 세션 둘을 띄운다. 두 세션은 파일이 겹치지 않아 함께 돌 수 있다(`.claude/skills/unreal-perf-measurement/`, `.claude/skills/mass-crowd-optimization/`, 각자의 Worklog 절은 [Skills.md](Skills.md)).
3. 각 세션은 이 문서의 해당 절대로 만들고 7.2·7.3절을 확인하고 커밋한다.
4. 둘이 끝나면 한 세션이 7.1절의 질문을 던지고 결과를 Skills.md에 적는다.
