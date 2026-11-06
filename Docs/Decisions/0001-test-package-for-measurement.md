# ADR-0001: 수치는 Test 구성 패키지에서 잰다

- 상태: 승인됨
- 날짜: 2026-11-06 (사용자 결정, 채팅)

## 맥락

형제 프로젝트(DS 실험실)는 패키징 없이 Development 에디터 빌드 실행 파일(`-game`)로 측정했다. 그 프로젝트의 backlog에 "Test 패키지로 재측정"이 남아 있다. 이 프로젝트는 클라이언트 렌더링·애니메이션 비용을 재므로 에디터 모듈과 `check()`, 디버그 드로우, STATS 수집 비용이 결과에 섞이는 영향이 더 크다. Shipping은 트레이스와 `csvprofile`이 컴파일되지 않아 쓸 수 없다.

## 결정

**수치를 쓰는 측정은 Test 구성 Win64 패키지(쿡 포함)에서 한다.** Development 에디터 빌드(`-game`)는 개발, 동작 확인, 디버그 오버레이 스크린샷(`stat anim`, `a.Budget.Debug.*`, Mass Debugger)에 쓰고 그 수치는 비교에 쓰지 않는다.

Test 구성에서 쓸 수 있는 것과 없는 것(5.8.4 `Engine/Source/Runtime/Core/Public/Misc/Build.h`, `ProfilingDebugging/CsvProfilerConfig.h`, `TraceLog/Public/Trace/Config.h`, `Engine/Public/EngineDefines.h`):

| 수단 | Test | 근거 |
| --- | --- | --- |
| `csvprofile start/stop`, CSV 커스텀 스탯 | 됨 | `CSV_PROFILER = WITH_ENGINE && (!UE_BUILD_SHIPPING || …)` |
| 엔진이 CSV에 쓰는 `GameThreadTime`, `RenderThreadTime`, `GPUTime`, `RHIThreadTime` | 됨 | `LaunchEngineLoop.cpp:1518-1525` `CSV_CUSTOM_STAT_MINIMAL_GLOBAL` |
| Insights 트레이스(`-trace=`) | 됨 | `UE_TRACE_ENABLED = !UE_BUILD_SHIPPING && !IS_PROGRAM` |
| GPU 스탯(CSV·트레이스) | 됨 | `HAS_GPU_STATS = (STATS \|\| CSV_PROFILER_STATS) && !UE_BUILD_SHIPPING` |
| 콘솔 명령 | 됨 | `ALLOW_CONSOLE 1` |
| `stat unit` 오버레이 | 됨(스크린샷용) | STATS 비의존 |
| `stat anim`, `stat mass`, `stat scenerendering` 등 STATS 그룹 | 안 됨 | `STATS = FORCE_USE_STATS && …`, 기본 0. 대신 `ENABLE_STATNAMEDEVENTS 1`이라 `-statnamedevents`로 사이클 스탯이 트레이스 이벤트로 남는다 |
| `ProfileGPU` | 안 됨 | `WITH_PROFILEGPU`는 Test 제외(`ALLOW_PROFILEGPU_IN_TEST 0`) |
| 디버그 드로우, `a.Budget.Debug.*` | 안 됨 | `UE_ENABLE_DEBUG_DRAWING = !(SHIPPING \|\| TEST) \|\| WITH_EDITOR` |
| `check()`, `ensure()` | 꺼짐 | `DO_CHECK = USE_CHECKS_IN_SHIPPING` |

따라서 벤치마크 러너는 `csvprofile`의 래퍼로 짜고, 프로젝트가 재야 할 값(Mass 프로세서 ms, 표현 전환 수, 밴드 분포, 애니 틱 수)은 `CSV_CUSTOM_STAT`으로 직접 낸다. GPU 패스별 분해가 필요하면 트레이스의 GPU 트랙을 쓴다.

## 고려한 대안

| 대안 | 버린 이유 |
| --- | --- |
| Development 에디터 빌드 `-game`(DS 방식) | 에디터 모듈·`check`·STATS 비용이 섞인다. 클라이언트 프레임 시간을 재는 프로젝트에서 영향이 크다 |
| Development 패키지 | 쿡은 되지만 `check`와 STATS 비용은 남는다. Test가 Shipping에 더 가깝다 |
| Shipping 패키지 | `csvprofile`과 트레이스가 컴파일되지 않는다 |
| Test에 `bUseStatsInTest`류로 STATS 강제 | STATS 비용이 다시 들어온다. 필요한 값은 CSV 커스텀 스탯으로 충분하다 |

## 결과

- 패키징 시간이 측정 루프에 더해진다. 작은 규모 동작 확인은 Development로, 마일스톤 측정만 Test 패키지로 한다.
- 스크린샷용 실행과 수치용 실행이 자연히 분리된다(계획의 측정 함정 항목과 일치).
- 실제 패키징 명령과 소요 시간은 M0에서 확인해 commands.md에 적는다.
