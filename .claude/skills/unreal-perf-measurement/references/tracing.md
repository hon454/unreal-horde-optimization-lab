# 트레이스: 원인을 가르기

수치가 아니라 원인을 볼 때 연다. 트레이스는 이벤트 기록 비용이 있어 그 실행의 수치는 비교에 쓰지 않는다. 트레이스 실행은 측정 실행과 따로 돌린다. 예시 수치는 `SKILL.md` "예시 수치의 조건"에서 잰 참고값이다.

## Test 구성에서 되는 것

`Engine/Source/Runtime/Core/Public/Misc/Build.h`의 `UE_BUILD_TEST` 블록과 관련 헤더에서 확인한 값(UE 5.8.4)이다.

| 매크로 | Test 값 | 영향 |
| --- | --- | --- |
| `STATS` | 0(기본) | `stat anim` 같은 스탯 그룹이 없다 |
| `ENABLE_STATNAMEDEVENTS` | 1 | `-statnamedevents`로 사이클 스탯이 트레이스 이벤트가 된다 |
| `CSV_PROFILER` | 1 | `csvprofile`, `FCsvProfiler`를 쓸 수 있다 |
| `UE_TRACE_ENABLED` | 1 | Unreal Insights 트레이스를 쓸 수 있다 |
| `HAS_GPU_STATS` | 1 | GPU 시간이 CSV와 트레이스에 남는다 |
| `WITH_PROFILEGPU` | 0 | `ProfileGPU`가 없다 |
| `UE_ENABLE_DEBUG_DRAWING` | 0 | 디버그 드로우와 그것을 쓰는 디버그 CVar가 없다 |
| `ALLOW_CONSOLE` | 1 | 콘솔 명령을 쓸 수 있다 |

- 세 스레드 시간의 원천(`GGameThreadTime`, `GRenderThreadTime`, `RHIGetGPUFrameCycles()`)은 `STATS` 없이 쓰인다. 그래서 Test에서도 Development와 같은 정의의 값이다.
- 엔진 CSV의 기본 열: `GameThreadTime`, `RenderThreadTime`, `GPUTime`, `RHIThreadTime`, `GameThreadTime_CriticalPath`, `RenderThreadTime_CriticalPath`(`Engine/Source/Runtime/Launch/Private/LaunchEngineLoop.cpp`).
- 트레이스 명령줄 예: `-trace=cpu,gpu,frame,counters -statnamedevents`. 쓰는 시스템의 채널을 더한다.
- 디버그 화면이 필요하면 Development 빌드로 따로 찍는다. 그 실행의 수치는 비교에 쓰지 않는다.

## 프레임이 세 스레드보다 길 때

**증상**: 프레임 시간이 게임 스레드, 렌더 스레드, GPU 어느 것보다 길다.

**설명**
- 모자란 몫은 게임 스레드가 작업자 스레드를 기다린 시간이다.
- 엔진은 이벤트 대기를 유휴 시간으로 센다(`FThreadIdleStats::FScopeIdle`). 그래서 이 기다림은 게임 스레드 시간에서 빠진다.
- `GameThreadTime_CriticalPath`가 프레임 시간과 같으면 그 기다림은 임계 경로에 있다.

**절차**
1. 프레임 CSV에서 게임 스레드의 `EventWait/*` 열을 더한다.
2. 게임 스레드 시간에 그 합을 더해 프레임 시간과 맞춰 본다.
3. `GameThreadTime_CriticalPath`를 프레임 시간과 견준다.
4. 트레이스에서 게임 스레드의 `WaitForTasks`가 끝나는 순간을 찾는다.
5. 그 순간에 끝나는 작업자 스레드의 이벤트를 찾는다. 그 이벤트가 범인이다.

`compare_csv.py`는 프레임 p50이 세 스레드 p50 최댓값의 1.1배를 넘는 실행에 표시를 단다. `--extra-col GameThreadTime_CriticalPath`로 그 열도 함께 볼 수 있다.

**예**(참고값, 프레임이 34ms대인 중간 구성)
- 프레임 p50 34.1ms, 게임 스레드 18.0ms, GPU 12.2ms, 렌더 스레드 7.29ms였다.
- 게임 스레드는 프레임마다 16.2ms를 기다렸다. 그중 14.5ms가 `EventWait/PrePhysics`였다.
- 게임 스레드 시간에 기다림을 더하면 프레임 시간과 0.03ms 안에서 맞았다.
- 기다린 작업자 이벤트는 작업자 하나가 한 스레드로 돌린 엔진 회피 프로세서였다(한 프레임 25.6ms).

## Unreal Insights의 Timers 합계

**증상**: Timers 목록에서 고른 구간의 합계가 프레임 수 × 프레임 시간보다 크다.

**설명**
- Timers 목록은 모든 스레드의 같은 이름 타이머를 합쳐 보인다.
- 명령줄 내보내기를 `-threads=GameThread`로 걸러도 GPU 타이머와 `Frame` 행이 섞여 나온다.

**주의**: 엔진 소스 빌드에는 `UnrealInsights.exe`가 들어 있지 않다. 한 번 빌드한다.

```text
Engine/Build/BatchFiles/Build.bat UnrealInsights Win64 Development
```

**절차**
1. Insights의 명령줄 내보내기로 스레드별 값을 얻는다.
2. 내보낸 값에서 GPU 타이머와 `Frame` 행을 버린다.
3. 프레임당 값 = 구간 합 ÷ 구간 안 `FEngineLoop::Tick`의 Count.
4. 한 번의 시간 = 구간 합 ÷ 그 타이머의 Count.

```text
UnrealInsights.exe -OpenTraceFile=<trace.utrace> -NoUI -AutoQuit
  -ExecOnAnalysisCompleteCmd="TimingInsights.ExportTimerStatistics <out.csv> -threads=GameThread -startTime=70 -endTime=120 -columns=*"
```

**예**(참고값): 50초 구간에서 `WaitForTasks`가 목록에 60,369번, 1분 42.7초로 나왔다. 같은 구간의 게임 스레드만의 값은 22.96초(프레임당 15.1ms)였다.

## 액터가 많은 장면

**증상**: 액터가 많은 장면의 게임 스레드 트레이스에서 한두 컴포넌트의 틱이 대부분을 차지한다.

**설명**
- 액터 군중의 비용은 개체마다 컴포넌트가 차례로 틱하는 일의 합이다. 타이머 합계 = 한 번의 비용 × 불린 횟수다.
- 프레임이 50ms를 넘으면 `UCharacterMovementComponent`는 시간을 최대 50ms씩, 여덟 번까지의 하위 스텝으로 나눠 계산한다. 그래서 무거운 프레임에서 한 번의 비용이 는다.
- 엔진은 월드 델타를 0.4초(`MaxUndilatedFrameTime`)로 자른다. 그보다 긴 프레임은 일을 덜 한다(`equivalence.md`).

**절차**
1. 트레이스에서 큰 타이머의 합계와 불린 횟수를 읽는다.
2. 불린 횟수가 개체 수를 따르는지 본다.
3. 한 번의 비용이 개체 수에 따라 변하는지 따로 본다.

**예**(참고값, 모든 개체가 `ACharacter`인 1만)
- `UCharacterMovementComponent_TickComponent`가 프레임마다 9,983번 불렸고 게임 스레드 프레임의 76.4%(411ms)를 썼다.
- 한 번의 시간은 약 41µs(411ms ÷ 9,983)였다.
- 개체당 게임 스레드 시간은 2천 개체까지 18µs 안팎이었고 1만에서 58.1µs였다. 하위 스텝과 같은 방향이지만 하위 스텝을 끈 실행으로 가르지는 않았다(원인 미확정).

## 병렬 작업의 시간 읽기

- 병렬 트레이스에서 한 작업의 시간에는 다른 작업을 기다린 시간이 섞인다. 자원 충돌로 기다린 작업이 비싸 보인다.
- 작업별 순수 시간이 필요하면 병렬 실행을 끈 트레이스(직렬 트레이스)를 따로 남긴다. 직렬 트레이스의 프레임 시간은 비교에 쓰지 않는다.
- Mass Entity 군중이면 직렬 트레이스와 한 스레드로 도는 엔진 프로세서 찾기는 `unreal-mass-crowd-optimization` 스킬의 `references/processors.md`에 있다.
