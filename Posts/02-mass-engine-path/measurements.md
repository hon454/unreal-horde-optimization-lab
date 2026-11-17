# 02 측정 기록

[글](README.md)의 수치, 그림의 값, 트레이스와 프레임 CSV에서 읽은 값, 엔진 소스 위치, 화면의 출처를 적는다. 본문은 유효 숫자 세 자리로 줄여 쓰고, 최대 N은 유효 숫자 둘로 쓴다([measurement.md](../../Docs/Guides/measurement.md) "지표"). 그림은 `py -3 -I Posts/02-mass-engine-path/make-figures.py`가 만든다.

## 묶음과 빌드

| 항목 | 값 |
| --- | --- |
| 묶음 | 총괄 묶음 `Full4`. 수치의 정의와 묶음의 경위는 [글 00의 측정 기록](../00-goals-and-method/measurements.md) "빌드와 묶음"과 같다 |
| 빌드 | Test 패키지 de0e21b(태그 `v2-full4`). CSV 메타데이터 `labbuild=de0e21bd64ac` |
| 조건 | 따로 적지 않으면 호드 총수 1만, 이동 장면(`V0`), 오브 시점 카메라(`P1`), 워밍업 60초 + 측정 180초, 조건마다 3회. 값은 세 실행의 중앙값이다 |
| 본문 조건의 회차 | `V0-s1`, `V0-s2`(1만, 5천, 2,500)는 r1, r2, r3이다(`summary.csv` `runs_used`). 라벨은 `Full4-V0-<구성>-N<N>-P1-r<회차>`다 |
| 요약 파일 | [summary.csv](../../Measurements/Full4/summary.csv), [runs.csv](../../Measurements/Full4/runs.csv), [equivalence.csv](../../Measurements/Full4/equivalence.csv), [최대 N 표](../../Docs/figures/full4/full-n-at-60fps.csv) |
| 프레임 CSV | `Saved/Profiling/CSV/Full4-V0-s2-N10000-P1-r1.csv`, `-r2.csv`, `-r3.csv`(git 밖). 측정 창 180초의 프레임마다 한 줄이다 |
| 트레이스 | `Saved/Traces/Full4trace-V0-s1-N10000-P1-r1.utrace`, `Full4trace-V0-s2-N10000-P1-r1.utrace`(병렬), `Full4traceser-V0-s2-N10000-P1-r1.utrace`(직렬, `-LabFullyParallel=0`). git 밖. 트레이스 실행은 측정 60초이고 수치를 비교에 쓰지 않는다 |
| 참고 묶음 | `V2M3Avoid`(Test 3072e96). 엔진 회피 파라미터를 바꾼 한 벌이다. `Full4`와 다른 묶음이라 본문에 "참고"로만 적었고 그림에 넣지 않았다(아래 "참고 묶음") |

## 본문의 수치와 출처

"세 실행"은 r1 / r2 / r3 순서다. 트레이스 값의 셈법은 아래 "트레이스에서 읽은 값"에 있다.

| 본문 값 | 정밀한 값 | 조건 | 출처(열 또는 식) | 세 실행 |
| --- | --- | --- | --- | --- |
| 단계 1 프레임 p50 597ms | 597.108 | `V0-s1` | `frame_p50_ms` | 599.009 / 597.108 / 594.037 |
| 단계 2 프레임 p50 34.1ms | 34.058 | `V0-s2` | `frame_p50_ms` | 34.084 / 34.058 / 33.702 |
| 단계 2 프레임 p95 36.8ms | 36.785 | `V0-s2` | `frame_p95_ms` | 37.212 / 36.694 / 36.785 |
| 단계 2 게임 스레드 시간 18ms | 18.027 | `V0-s2` | `game_p50_ms`. 세 자리로 18.0이고 끝의 0을 지웠다 | 18.027 / 18.092 / 17.904 |
| 최대 N 약 780 | 781 | `V0-s1` | `full-n-at-60fps.csv` "1 Actor baseline" `n_p95` | |
| 최대 N 약 4,100 | 4,102 | `V0-s2` | 같은 파일 "2 Mass, engine default path" `n_p95`. 식은 아래 | |
| 16.7ms | 16.667 | 계산 | 1000 ÷ 60. 그림의 선은 16.7(`Scripts/plot.py` `BUDGET_MS`) | |
| 화면 안 35m, 액터 최대 1,000 | 35, 1000 | 설정 | `summary.csv` `hydrate_m`, `actor_cap`. [ADR-0021](../../Docs/Decisions/0021-unified-actor-cap-and-hydrate-distance.md) | |
| 200m 밖은 그리지 않는다 | 200 | 설정 | CSV 메타데이터 `LabFarM`(조건 표 `common_expect`), `LabTechFarCull` 1 | |
| 단계 1 CharacterMovement 틱 프레임당 9,983번 | 9,982.7 | `Full4trace-V0-s1` | 928,391번 ÷ 93프레임 | 트레이스 한 회 |
| 그 틱이 프레임의 76.4% | 0.7639 | 같은 트레이스 | CharacterMovement Excl 410.73ms ÷ `FEngineLoop::Tick` 537.63ms(프레임당) | |
| 메시 틱 9,906번 → 973번 | 9,906.1, 972.6 | `Full4trace-V0-s1`, `-s2` | `USkinnedMeshComponent_TickComponent` 921,266 ÷ 93, 1,482,294 ÷ 1,524 | |
| 예상: 4분의 1 아래(0.236) | 0.236 | 계산 | 1 − 0.764 | |
| 단계 1의 5.70%, 예상한 23.6% | 0.05704 | 계산 | 34.058 ÷ 597.108. 23.6%는 위 예상 | |
| 메시 틱과 그리기 호출도 함께 줄었다 | 그리기 호출 9,075 → 2,928 | `V0-s1`, `V0-s2` | `draw_calls_p50`(9,126 / 9,009 / 9,075, 2,928 / 2,868 / 2,963). 메시 틱은 위 행 | |
| 렌더 스레드 7.29ms, GPU 12.2ms | 7.294, 12.23 | `V0-s2` | `render_p50_ms`, `gpu_p50_ms` | 7.294 / 7.442 / 7.162; 12.206 / 12.247 / 12.23 |
| 게임 스레드가 기다린 시간 16.2ms | 16.218 | `V0-s2` 프레임 CSV | 게임 스레드 `EventWait/*` 열 합의 프레임 중앙값. 아래 "프레임 CSV의 기다림 열" | 16.236 / 16.218 / 15.998 |
| 그중 14.5ms가 PrePhysics | 14.509 | 같다 | `Exclusive/GameThread/EventWait/PrePhysics`의 프레임 중앙값 | 14.509 / 14.518 / 14.293 |
| 남는 값의 중앙값은 0.03ms보다 작다 | 0.006 / −0.012 / 0.027 | 같다 | 프레임마다 `FrameTime − GameThreadTime − EventWait/* 합`, 그 중앙값 | 실행마다 |
| 한 프레임에서 회피 25.6ms | 25.604 | `Full4trace-V0-s2`, 게임 프레임 3305 | `MassMovingAvoidanceProcessor` 이벤트 길이. 아래 "한 프레임" | |
| 같은 프레임에서 게임 스레드 기다림 18.4ms | 18.447 | 같다 | `WaitForTasks` 이벤트 길이 | |
| 렌더 스레드도 프레임의 대부분을 기다린다 | 26.596 / 26.434 / 26.338 | `V0-s2` 프레임 CSV | `RenderThreadIdle/NonCriticalPath`의 프레임 중앙값(프레임 34.1ms 가운데) | 실행마다 |
| 회피는 프레임마다 한 번 | 1,523번 / 1,524프레임 | `Full4trace-V0-s2` | 70\~120초의 이벤트 수와 `Game Frames` 수 | |
| 트레이스 50초 | 50 | 같다 | 구간 70\~120초 | |
| 회피 한 번 24.7ms | 24.727 | 같다 | 1,523번 길이의 평균(중앙값 24.512) | |
| 게임 스레드 기다림 프레임마다 15.1ms | 15.066 | 같다 | `GameThread` `WaitForTasks` Incl 22.960초 ÷ 1,524 | |
| 직렬 트레이스의 회피 23.5ms | 23.486 | `Full4traceser-V0-s2` | Incl 28.793초 ÷ 1,226번 | |
| Mass 프로세서 시간의 80.7% | 0.8068 | 같다 | 28.793 ÷ 35.686(이름이 `…Processor_<번호>`인 타이머 Incl의 합) | |
| 오브 30m 안 개체 수 2,147 | 2,147.28 | `V0-s2` | `near30_mean` | 2,150.10 / 2,147.28 / 2,138.74 |
| 기준의 2.20배 | 2.195 | 계산 | 2,147.28 ÷ 978.245(`equivalence.csv` `near30_mean_ref`) | |
| 초당 사망 53.9, 기준보다 9% 적다 | 53.928, −9.00% | `V0-s2` | `death_rate`, `death_rate_dev_pct`(기준 59.261) | 53.928 / 53.739 / 54.079 |
| 겹친 쌍 12,442 | 12,441.87 | `V0-s2` | `overlap_mean` | 12,441.87 / 12,654.02 / 11,354.9 |
| 1%를 넘는다, 값만 보고 | `report` | `V0-s2` | `equivalence.csv` `death_rate_ok`, `near30_mean_ok`, `pass` = `report`. 조건 표의 `s2` `"equiv": "report"` | |
| 액터 평균 971개, 상한에 붙어 있다 | 970.75(최대 998) | `V0-s2` | `actors_mean`, `actors_max`. 상한 1,000은 `actor_cap` | 970.75 / 968.03 / 971.55 |
| 참고: 겹친 쌍 75.9% 감소 | 12,172 → 2,929 | `V2M3Avoid` | 아래 "참고 묶음" | |
| 참고: 초당 사망 36.8% 더 감소 | 53.72 → 33.96 | `V2M3Avoid` | 같다 | |
| 참고: 프레임 p95 5.49ms 증가 | 39.02 → 44.51 | `V2M3Avoid` | 같다 | |
| 시나리오 카드의 2m/s, 호드 1만 | 2m/s, 10,000 | 설정 | [measurement.md](../../Docs/Guides/measurement.md) "시나리오" | |

최대 N의 식(`Scripts/plot.py` `n_at_budget`, 잰 총수 사이를 로그 보간): 단계 2는 2,500에서 9.201, 5천에서 19.698이다. t = (16.7 − 9.201) ÷ (19.698 − 9.201) = 0.714, N = 2,500 × 2^0.714 = 4,102. 단계 1의 781은 [글 00의 측정 기록](../00-goals-and-method/measurements.md)에 같은 식이 있다.

## 프레임 CSV의 기다림 열

`Saved/Profiling/CSV/Full4-V0-s2-N10000-P1-r<회차>.csv`(git 밖)에서 측정 창의 모든 프레임을 읽고 열마다 프레임 중앙값을 냈다. 스크래치 스크립트로 셌고 저장소에 넣지 않았다. 프레임 수는 5,267 / 5,286 / 5,331이다.

| 열 | r1 | r2 | r3 |
| --- | --- | --- | --- |
| `FrameTime` | 34.084 | 34.058 | 33.702 |
| `GameThreadTime` | 18.027 | 18.092 | 17.904 |
| `Exclusive/GameThread/EventWait/PrePhysics` | 14.509 | 14.518 | 14.293 |
| 게임 스레드 `EventWait/*` 열의 합 | 16.236 | 16.218 | 15.998 |
| `GameThreadTime` + 기다림 합 | 34.089 | 34.049 | 33.705 |
| `FrameTime − GameThreadTime − 기다림 합` | 0.006 | −0.012 | 0.027 |
| `GameThreadTime_CriticalPath` | 34.083 | 34.042 | 33.695 |
| `RenderThreadIdle/NonCriticalPath` | 26.596 | 26.434 | 26.338 |

- 합에 든 게임 스레드 기다림 열은 `EventWait/PrePhysics`, `/StartPhysics`, `/EndPhysics`, `/EndOfFrameUpdates`, `/RenderAssetStreaming`이다. 둘째로 큰 것은 `EndPhysics`(1.339 / 1.329 / 1.322)다.
- 위 두 행(합, 차)은 프레임마다 더하고 뺀 뒤의 중앙값이다. 그래서 `GameThreadTime`과 기다림 합의 중앙값을 그냥 더한 값(18.027 + 16.218 = 34.245)과 다르다. 그림 `frame-vs-threads.svg`의 막대 끝 "34.2"는 이 단순 합이다.
- `GameThreadTime_CriticalPath`가 프레임 시간과 같다. 엔진은 이 값에서 "비임계 경로"의 기다림만 뺀다(`UnrealClient.cpp:1849`). 게임 스레드의 이 기다림은 임계 경로로 세였다는 뜻이다.
- 비교: 단계 3(`Full4-V0-s3-N10000-P1-r2.csv`)에는 `EventWait/PrePhysics` 열이 없고, 기다림 합이 1.041ms, `FrameTime − GameThreadTime − 기다림 합`이 0.002ms다. 단계 1(`Full4-V0-s1-N10000-P1-r2.csv`)은 기다림 합 15.935ms(대부분 `EndPhysics`)다. 둘 다 본문에 쓰지 않았다.

## 트레이스에서 읽은 값

[글 03의 측정 기록](../03-separation/measurements.md) "트레이스 값"과 같은 구간과 셈법을 썼다.

- 구간: 트레이스 시각 70\~120초(50초).
- 내보내기: `Scripts/trace-export.ps1`(`UnrealInsights.exe -NoUI -AutoQuit` 명령줄 내보내기). 결과 CSV는 세션 스크래치(저장소 밖)에 두었다.
  - `TimerStatistics`: `-Extra "-threads=GameThread -startTime=70 -endTime=120 -columns=* -sortBy=TotalInclusiveTime"`. 트레이스마다 한 번(`Full4trace-V0-s1`, `Full4trace-V0-s2`, `Full4traceser-V0-s2`).
  - `TimingEvents`: `-Extra "-threads=* -timers=*MovingAvoidance* -startTime=70 -endTime=120 -columns=ThreadId,ThreadName,TimerName,StartTime,EndTime,Depth"`(`Full4trace-V0-s2`).
- 프레임 수는 구간 안 `FEngineLoop::Tick`의 Count다. 프레임당 값은 타이머 값 ÷ 프레임 수다.

| 값 | 트레이스 | 타이머와 식 | 결과 |
| --- | --- | --- | --- |
| 프레임 수, 한 프레임 | `Full4trace-V0-s1` | `FEngineLoop::Tick` 93번, Incl 50.0초 | 537.63ms |
| CharacterMovement 틱 | 같다 | `UCharacterMovementComponent_TickComponent` Count 928,391, Excl 38.197초 | 프레임당 9,982.7번, 410.73ms |
| 컴포넌트 틱, 메시 틱 | 같다 | `FActorComponentTickFunction::ExecuteTick` 1,850,117, `USkinnedMeshComponent_TickComponent` 921,266 | 프레임당 19,893.7, 9,906.1 |
| 프레임 수, 한 프레임 | `Full4trace-V0-s2` | `FEngineLoop::Tick` 1,524번, Incl 49.999초 | 32.808ms |
| 게임 스레드 기다림 | 같다 | `WaitForTasks` Incl(= Excl) 22.960초 | 프레임당 15.066ms |
| 컴포넌트 틱, 메시 틱 | 같다 | `FActorComponentTickFunction::ExecuteTick` 1,489,909, `USkinnedMeshComponent_TickComponent` 1,482,294 | 프레임당 977.6, 972.6 |
| 액터 위치 넣기 | 같다 | `LabActorMove` 1,473,332, `LabActorSyncProcessor` Incl 5.400초 | 프레임당 966.8번, 3.543ms |
| CharacterMovement 틱 | 같다 | `GameThread` 통계에 `CharacterMovement` 이름의 타이머가 없다 | 0 |
| 엔진 회피 | 같다(`TimingEvents`) | `MassMovingAvoidanceProcessor` 1,523번의 길이 평균. 32개 작업자 스레드에 나뉘어 돌았고 두 번이 시간상 겹친 적이 없다 | 24.727ms(중앙값 24.512) |
| 프레임 수, 한 프레임 | `Full4traceser-V0-s2` | `FEngineLoop::Tick` 1,227번, Incl 49.999초 | 40.749ms |
| 엔진 회피(직렬) | 같다 | Incl 28.793초 ÷ 1,226번(Excl과 같다) | 23.486ms |
| Mass 프로세서 합(직렬) | 같다 | `…Processor_<번호>` 타이머 Incl의 합 35.686초. 다음으로 큰 것은 `LabActorSyncProcessor` 3.322초 | 회피 몫 80.7% |

- 트레이스 실행의 프레임은 측정 실행보다 짧거나 길 수 있다(병렬 32.8ms, 측정 34.1ms). 그래서 트레이스 값은 어디에 시간을 쓰는지만 보이고, `summary.csv`의 값과 견주지 않는다.
- 처음에는 75\~120초 구간과 `TimerStatistics`의 Excl ÷ 프레임 수로 셌다(회피 24.239ms, 기다림 15.280ms, 1,370프레임). 글 03과 같은 값이 되게 70\~120초와 위 셈법으로 바꿨다. 결론은 같다.
- 단계 2의 메시 틱 972.6번은 측정 실행의 액터 평균 970.75와 가깝다. 단계 1의 메시 틱 9,906번과 CharacterMovement 틱 9,983번이 1만보다 적은 까닭은 보지 않았다.

### 한 프레임(그림 `one-frame.svg`)

`Full4trace-V0-s2`의 게임 프레임 3305다. `-Command TimingEvents -Extra "-threads=* -timers=*Processor*,*WaitForTasks*,*Runner*,FEngineLoop::Tick,*PrePhysics*,*PostPhysics*,UWorld_Tick,FViewport_Draw -startTime=100.0 -endTime=100.1 -columns=ThreadId,ThreadName,TimerName,StartTime,EndTime,Depth"`로 뽑았다. 시각은 프레임 시작(100.009724초)부터의 ms다. 프레임은 구간 안에서 고르기 쉬운 가운데(100초)의 첫 온전한 프레임으로 골랐다.

| 스레드 | 이벤트 | 시작 | 끝 | 길이 |
| --- | --- | --- | --- | --- |
| GameThread | `Game Frame 3305` | 0 | 34.722 | 34.722 |
| GameThread | `MassProcessingQueue Main-Thread Runner Task`(그림의 "Mass 게임 스레드 일") | 0.663 | 4.490 | 3.827 |
| GameThread | `WaitForTasks` | 10.928 | 29.375 | 18.447 |
| GameThread | `WaitForTasks`(둘, 그림은 하나로 그림) | 29.819, 29.849 | 29.845, 31.095 | 0.026, 1.246 |
| Background Worker #15 | `MassMovingAvoidanceProcessor` | 1.568 | 27.172 | 25.604 |
| Background Worker #15 | `MassStandingAvoidanceProcessor` | 27.186 | 28.816 | 1.630 |
| Background Worker #7 | `MassApplyForceProcessor`, `MassApplyMovementProcessor` | 28.825 | 29.121 | 0.296 |
| Background Worker #25 | `MassProcessingQueue Async Runner Task` | 0.668 | 29.368 | 28.700 |

- 그림의 값은 위 값을 소수 둘째 자리로 줄인 상수다(`make-figures.py` `FRAME`). 그림의 "기다림 18.4", "엔진 이동 회피 25.6", "프레임 끝 34.7"은 세 자리다.
- 게임 스레드의 첫 기다림은 비동기 러너가 끝난 0.007ms 뒤에 끝난다. 앞 프레임(3304)도 기다림의 끝이 비동기 러너의 끝과 맞았다(프레임 3305 시작 기준 −5.38ms 대 −5.39ms). 회피는 3304에서 25.59ms, 3306에서 25.54ms다.
- `MassSmoothOrientationProcessor`(Background Worker #7)는 0.691\~28.816ms에 걸쳐 있지만 Excl은 프레임당 약 1.1ms다. 위치 쓰기 선언 때문에 회피가 끝나기를 기다린 것으로 본다([engine-notes.md](../../Docs/Reference/engine-notes.md) 47절). 본문에 쓰지 않았다.

## 참고 묶음 `V2M3Avoid`

엔진 회피 파라미터 셋(분리 반경 배율 1.5, 분리 강성 3,000, 예측 시간 0.5초)을 바꾼 한 벌이다. Test 3072e96, 1만, 이동 장면, 오브 시점 카메라, 3회. 값은 [V2.md](../../Docs/Worklog/V2.md) "묶음 `V2M3Avoid`"와 `Measurements/V2M3Avoid/runs.csv`다. 사용자가 단계 2를 엔진 기본 파라미터로 두고 이 묶음을 글에서 참고로만 인용하기로 정했다([V2-full4-review.md](../../Docs/Worklog/V2-full4-review.md) "사용자 판단").

| 지표 | `s2`(엔진 기본값) | `s2avt`(한 벌) | 변화 |
| --- | --- | --- | --- |
| 겹친 쌍 평균 | 12,172 | 2,929 | −75.9% |
| 초당 사망 | 53.72 | 33.96 | −36.8% |
| 프레임 p95(ms) | 39.02 | 44.51 | +5.49 |

- 이 묶음의 `s2`는 `Full4`의 `V0-s2`(프레임 p95 36.785)와 다른 빌드와 묶음이다. 두 값을 잇지 않는다.
- 세 값 모두 엔진 주석의 권장 범위 밖이다([engine-notes.md](../../Docs/Reference/engine-notes.md) 51절). 본문에는 쓰지 않았다.

## 그림의 값

### `summary-threads.svg`

1만, `V0`, `P1`, ms. 가로축은 로그 눈금이다.

| 행 | 단계 1(`V0-s1`) | 단계 2(`V0-s2`) |
| --- | --- | --- |
| 프레임 p95 | 665.254 | 36.785 |
| 프레임 p50 | 597.108 | 34.058 |
| 게임 스레드 | 580.969 | 18.027 |
| 렌더 스레드 | 28.661 | 7.294 |
| GPU | 54.68 | 12.23 |

세 실행: 단계 1 프레임 p95 665.254 / 668.172 / 653.996, 게임 스레드 582.745 / 580.969 / 578.155, 렌더 28.661 / 28.931 / 28.362, GPU 52.757 / 54.775 / 54.68. 단계 2는 위 "본문의 수치와 출처" 표에 있다.

### `what-changed.svg`

| 그림의 글자 | 값 | 출처 |
| --- | --- | --- |
| CharacterMovement 틱 프레임당 9,983번 | 9,982.7 | 위 "트레이스에서 읽은 값" |
| 메시 틱 프레임당 9,906번 → 973번 | 9,906.1, 972.6 | 같다 |
| 컴포넌트 틱 19,894번 → 978번 | 19,893.7, 977.6 | 같다 |
| 35m 안 액터, 상한 1,000 | 35, 1000 | `hydrate_m`, `actor_cap` |
| 35\~200m 정지 메시 인스턴스, 200m 밖은 그리지 않는다 | | [code-tour.md](../../Docs/Guides/code-tour.md) "최종 구성에서는 돌지 않는 코드"의 `UMassUpdateISMProcessor` 행, 글 00 측정 기록의 `stage-map.svg` |
| 조향·힘·이동·방향 보간, 엔진 회피 | | `BuildEntityConfig`의 Trait(아래 "Lab 코드 위치"), [code-tour.md](../../Docs/Guides/code-tour.md) "이동과 겹침 방지" |
| 액터 위치는 Lab 동기화 프로세서가 넣는다 | | `LabActorMove` 프레임당 966.8번(위), [engine-notes.md](../../Docs/Reference/engine-notes.md) 43절 끝 |

### `frame-vs-threads.svg`

`V0-s2`, 1만, ms.

| 막대 | 값 | 출처 |
| --- | --- | --- |
| 프레임 p50 | 34.058 | `summary.csv` `frame_p50_ms` |
| 게임 스레드 시간 | 18.027 | `game_p50_ms` |
| 게임 스레드가 기다린 시간 | 16.218 | 프레임 CSV 기다림 합의 세 실행 중앙값(위) |
| 막대 끝 34.2 | 34.245 | 18.027 + 16.218 |
| 렌더 스레드, GPU | 7.294, 12.23 | `render_p50_ms`, `gpu_p50_ms` |
| 글자 "그중 14.5ms", "0.03ms보다 작다" | 14.509, 최대 0.027 | 위 "프레임 CSV의 기다림 열" |

### `one-frame.svg`

위 "한 프레임"의 표다. 그림 아래 글자의 24.7ms와 15.1ms는 위 "트레이스에서 읽은 값"의 24.727, 15.066이다.

### `same-work.svg`

`equivalence.csv`의 1만 `V0-s2` 행이다. 막대의 길이는 기준에 대한 비율이다.

| 항목 | 단계 2 | 기준(단계 3\~8) | 차이 | 그림의 글자 |
| --- | --- | --- | --- | --- |
| 초당 사망 | 53.928 | 59.261 | −9.0% | 53.9, 59.3, −9% |
| 오브 30m 안 개체 수 | 2,147.28 | 978.245 | +119.5% | 2,147, 978, +120% |

허용 폭 1%는 조건 표 `equivalence.death_rate_tol`, `near30_tol`이다. 차이는 `equivalence.csv`가 소수 한 자리로 적은 값이고, 그림은 세 자리로 줄였다(−9.0 → 9, 119.5 → 120).

### `step2-max-n.svg`

`V0-s2`, 이동 장면, `P1`, ms.

| 총수 | 프레임 p95 | 프레임 p50 | 게임 스레드 시간 |
| --- | --- | --- | --- |
| 2,500 | 9.201(9.331 / 9.201 / 9.192) | 7.979(8.138 / 7.979 / 7.899) | 4.278(4.278 / 4.187 / 4.291) |
| 5천 | 19.698(19.517 / 19.869 / 19.698) | 17.452(17.421 / 17.566 / 17.452) | 10.904(10.904 / 10.927 / 10.89) |
| 1만 | 36.785 | 34.058 | 18.027 |

교차점 4,102는 `full-n-at-60fps.csv`에서 읽는다. 그림의 "9.20"은 세 자리 그대로이고 "18"은 18.0에서 끝의 0을 지운 값이다.

SVG 여섯은 [make-figures.py](make-figures.py)가 만든다. 측정값은 `summary.csv`, `equivalence.csv`, `full-n-at-60fps.csv`에서 읽고, 프레임 CSV와 트레이스의 값은 스크립트의 상수(`WAITS`, `TRACE`, `FRAME`)다. 폭 800 기준으로 글자는 16px 이상이고 자기 흰 배경을 가진다.

## 화면

`Docs/figures/steps/step2-mass-engine.webp`(960×540, 15fps, 8초)는 촬영 실행 `Full4shot-V0-s2-N10000-P1-r1`의 원본이다([V2.md](../../Docs/Worklog/V2.md) "2026-11-17 W14-3 단계 화면", [captures.csv](../../Measurements/Full4/captures.csv)). HUD를 켜고 녹화와 함께 돈 실행이라 HUD의 숫자는 측정값이 아니다. 본문은 화면의 숫자를 인용하지 않는다. 캡션의 "개체가 오브 둘레에 겹겹이 뭉쳐 있다"는 화면 눈 확인이다(대표 프레임 `Measurements/Full4/shots/step2.jpg`).

## 엔진 소스

로컬 소스 빌드 `G:/Epic Games/UE_5.8.4`(UE 5.8.4)에서 2026-11-17에 직접 열어 확인했다. 경로는 `Engine/` 아래다.

| 사실 | 위치 |
| --- | --- |
| 틱 그룹 PrePhysics를 도는 동안의 기다림 이름이 `EventWait/PrePhysics`다 | `Source/Runtime/Engine/Private/LevelTick.cpp:1749-1750` `UWorld::Tick`, 매크로 `Source/Runtime/Core/Public/ProfilingDebugging/CsvProfiler.h:148-150` |
| Mass PrePhysics 처리 단계는 틱 그룹 `TG_PrePhysics`에서 돈다 | [engine-notes.md](../../Docs/Reference/engine-notes.md) 27절(`MassProcessingPhaseManager.cpp:32,44-51`) |
| 이벤트 기다림은 CSV 기다림으로 적히고 같은 구간이 유휴 시간이 된다 | `Source/Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp:1931-1942` `FEventWin::Wait`(`:1936` `CSV_SCOPED_WAIT`, `:1940` `FScopeIdle`) |
| 유휴 시간을 `Waits`에 더한다 | `Source/Runtime/Core/Public/Stats/ThreadIdleStats.h:98-113`(`:104`) |
| 게임 스레드 시간 = 흐른 시간 − `Waits`, 임계 경로 시간 = 흐른 시간 − 비임계 기다림 | `Source/Runtime/Engine/Private/UnrealClient.cpp:1844, 1847-1849` `FViewport::Draw` |
| 게임 스레드가 작업을 기다리는 곳(`WaitForTasks` 스코프)과 그 이벤트 기다림 | `Source/Runtime/Core/Private/Async/TaskGraph.cpp:781-784` |
| 이 기다림의 둘째 인자 `bCountAsStall`은 `#if STATS` 안에서만 참이 된다. Test 구성은 `STATS`가 0이라 거짓이고 기다림이 유휴 시간에 든다 | `TaskGraph.cpp:729-755`, `STATS`의 Test 값은 [engine-notes.md](../../Docs/Reference/engine-notes.md) 6절 |
| 엔진 회피 쿼리는 Medium·Low·Off LOD 태그가 있는 엔티티를 뺀다 | `Plugins/AI/MassAI/Source/MassNavigation/Private/MassAvoidanceProcessors.cpp:301-303` |
| 엔진 회피는 `ForEachEntityChunk`(한 스레드)로 돈다 | 같은 파일 `:321, 331` `UMassMovingAvoidanceProcessor::Execute` |
| 엔진 회피는 그룹 `Avoidance`, 힘 적용은 `Avoidance` 뒤다 | 같은 파일 `:287-288`, [engine-notes.md](../../Docs/Reference/engine-notes.md) 27절, 37절 |
| 엔진 회피 파라미터 기본값, 이웃 수 상한 | [engine-notes.md](../../Docs/Reference/engine-notes.md) 36절, 51절(`MassAvoidanceProcessors.cpp:49-51, 832`) |

## 엔진 코드 발췌의 줄 수

| 발췌 | 줄 수(`// ...` 포함) | 코드 줄 |
| --- | --- | --- |
| `LevelTick.cpp` `UWorld::Tick`(`:1749-1750`) | 2 | 2 |
| `WindowsPlatformProcess.cpp` `FEventWin::Wait`(`:1936`, `:1940`) | 3 | 2 |
| `MassAvoidanceProcessors.cpp`(`:301-303`, `:331`) | 5 | 4 |
| 합 | 10 | 8 |

posting.md의 상한(글 하나에 엔진 코드 30줄)보다 작다. 남긴 줄은 소스 그대로이고 앞쪽 들여쓰기만 지웠다. `:331`은 람다의 첫 줄까지만 실었다.

## Lab 코드 위치

측정 빌드 de0e21b의 줄이다(`git show de0e21b:<경로>`). 이 글을 쓴 때의 HEAD와 `LabConfig.*`, `LabCrowdSubsystem.cpp`, `LabMinion.cpp`, `HordeOptLab.cpp`가 다르다(c12daf7, 개발 전용 표현 색). 아래 줄의 내용은 같다.

| 발췌와 문장 | 파일과 줄 | 심볼 |
| --- | --- | --- |
| Trait 붙이기(발췌) | `Source/HordeOptLab/LabCrowdSubsystem.cpp:62-92`(발췌 `:69-71, 79, 90, 92`) | `ULabCrowdSubsystem::BuildEntityConfig` |
| 발췌의 `ULabMovementTrait`는 엔진 `UMassMovementTrait`의 하위 클래스다. 본문은 엔진의 이동을 붙인다고 적었다 | `Source/HordeOptLab/LabMassTraits.h:28` | `ULabMovementTrait` |
| 발췌의 `UMassObstacleAvoidanceTrait` 줄은 실제로 `-LabAvoidTuned=1`이 아닐 때의 `else` 갈래다. 단계 2는 이 갈래다(`LabAvoidTuned` 0) | 같은 파일 `:84-91` | 같다 |
| 엔진 회피는 단계 2에서만 켠다 | `Source/HordeOptLab/LabConfig.cpp:47`(`C.bAvoid = C.Step == 2`), `LabConfig.h:428` `UsesAvoidance` | `FLabConfig` |
| Simulation LOD는 단계 5부터 | `LabConfig.h:422` `UsesSimLod`, `LabCrowdSubsystem.cpp:73-78` | 같다 |
| 단계 2는 엔진 방향 보간 프로세서를 쓴다 | `LabConfig.cpp:49`, `Source/HordeOptLab/HordeOptLab.cpp:70` | |
| 정지 메시 인스턴스는 엔진 `UMassUpdateISMProcessor` | `HordeOptLab.cpp:41` | |
| 액터 동기화 프로세서는 단계 2부터 | `HordeOptLab.cpp:44` | |
| 단계 1 액터는 액터 틱을 켠다 | `Source/HordeOptLab/LabMinion.cpp:22` | `ALabMinion::ALabMinion` |
| 하이드레이션 액터는 CharacterMovement 틱과 액터 틱을 끈다 | `LabMinion.cpp:228-229, 237, 242-247` | `LabDisableCharacterMovement`, `ALabMinionMassActor` |

## Insights 스크린샷

코디네이터가 2026-11-17에 화면 1을 찍어 README의 "게임 스레드는 작업자 스레드 하나의 엔진 회피를 기다린다" 절에 넣었다(`images/insights-avoidance-wait.jpg`). 아래 표는 찍기 전에 적은 계획이고, 화면의 값은 표 아래에 있다. 찍는 법은 [Worklog/Posts.md](../../Docs/Worklog/Posts.md) "Insights 스크린샷(글 02\~07)"과 같다.

| 항목 | 값 |
| --- | --- |
| 트레이스 | `Saved/Traces/Full4trace-V0-s2-N10000-P1-r1.utrace`(병렬) |
| 화면 1(구간) | Timing 뷰, 트레이스 시각 70\~120초. Timers를 `Avoidance`와 `WaitForTasks`로 거른다. 읽을 곳: `MassMovingAvoidanceProcessor`의 Count 약 1,523과 Incl 약 37.7초, GameThread `WaitForTasks`의 Incl 약 23.0초 |
| 화면 2(한 프레임, 선택) | 100.01\~100.05초를 확대한다. GameThread의 `WaitForTasks`(10.9\~29.4ms)와 작업자 트랙 하나의 `MassMovingAvoidanceProcessor`(1.6\~27.2ms)가 끝나는 자리가 맞는 것을 표시한다 |
| 저장할 파일 | `Posts/02-mass-engine-path/images/insights-avoidance-wait.jpg`(위아래로 붙인 한 장, 제목 줄을 자른다) |
| 넣은 뒤 | README의 주석을 풀고 캡션을 화면 값으로 맞춘다. 구간을 손으로 고르면 Count가 한두 개 다를 수 있다. 그 차이는 이 절에 적는다 |

화면 1의 Incl 37.7초는 1,523 × 24.727ms = 37.66초로 셈한 기대값이다.

화면의 값(시간 눈금자를 손으로 끌어 고른 구간 50.21초):

| 행 | Count | Incl | Excl |
| --- | --- | --- | --- |
| `MassMovingAvoidanceProcessor` | 1,529 | 37.81초 | 37.02초 |
| `MassStandingAvoidanceProcessor` | 1,529 | 846.86ms | 73.63ms |

- 37.81 ÷ 1,529 = 24.73ms로 본문의 24.7ms와 맞다. Count는 기대값보다 6 많다. 구간이 0.21초 길어서다.
- 게임 스레드의 `WaitForTasks`(약 23.0초)는 화면에 없다. Timers 목록은 모든 스레드를 합쳐 보여서 `WaitForTasks`가 60,369번, 1분 42.7초로 나온다. 게임 스레드만의 값은 내보낸 표가 근거다. 그래서 그림에는 `Avoidance`로 거른 목록만 실었다.
- 화면 2(한 프레임)는 찍지 않았다. 한 프레임의 기다림과 회피가 맞물리는 것은 `images/one-frame.svg`가 보인다.
- 그림은 화면 왼쪽 절반과 오른쪽 절반을 확대 캡처해 붙인 것이다(2128폭). 창 제목 줄은 잘랐고 빨간 상자는 붙인 뒤에 그렸다.

## 확인하지 않은 것

- 엔진 회피가 왜 개체를 오브 둘레에 쌓는지. 회피 힘이 속도 조향이고 가속 상한으로 잘린다는 것, 힘을 내는 이웃이 여섯이라는 것은 소스로 확인했다([engine-notes.md](../../Docs/Reference/engine-notes.md) 51절). 이것이 쌓임의 원인이라는 것은 추정이라 본문에 쓰지 않았다.
- 단계 2의 시간을 회피 자체의 비용과 뭉침이 늘린 액터의 비용으로 나누지 않았다. 같은 장면에서 회피만 뺀 조건이 총괄 묶음에 없다.
- 예상(4분의 1)은 트레이스 실행의 프레임(537.63ms)에서 셌고, 측정 실행의 프레임(597.108ms)과 다르다. 단계 2에서 메시 틱과 그리기 호출이 프레임을 얼마나 줄였는지는 나누지 않았다.
- 트레이스 구간 70\~120초가 측정 구간 안인지 트레이스 시각으로 맞춰 보지 않았다. 글 03과 같은 구간이다.
- 렌더 스레드 기다림(`RenderThreadIdle/NonCriticalPath`)의 대상을 트레이스로 보지 않았다. 본문은 "게임 스레드를 기다린다"로 적었고, 근거는 그 열이 렌더 스레드의 비임계 경로 유휴라는 이름과 게임 스레드 임계 경로 시간이 프레임과 같다는 것이다.
- 단계 2의 게임 스레드 기다림이 모두 엔진 회피 때문인지 프레임마다 맞춰 보지 않았다. 프레임 3305와 앞 프레임 3304에서 기다림의 끝이 비동기 러너의 끝과 맞았다.
- 단계 1 트레이스에서 프레임당 틱 수(메시 9,906번, CharacterMovement 9,983번)가 1만보다 적은 까닭.
- 게임 스레드의 기다림을 Insights 화면으로 보이지 못했다(위 절).
- 본문이 가리키는 글 01(`01-actor-baseline`)은 `v1` 폴더 이름과 같고 아직 `v2`로 고쳐 쓰지 않았을 수 있다. 글 03(`03-separation`)은 커밋돼 있다.
