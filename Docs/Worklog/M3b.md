# M3b 작업 기록

## 측정 방법 구현 (측정 세션, 2026-11-09)

plan.md "M3b" 측정 방법 1\~7번을 `Scripts/`에 구현했다. 빌드·패키징·벤치마크 실행은 하지 않았다.

### 만든 것

- `summarize.py`: `frame_p95_ms`(runs·summary, `_min`/`_max` 포함), `bottleneck`(Game·Render·GPU p50 중 가장 긴 것의 이름, 프레임 제외, 2026-11-09 코디네이터 결정 9205a80), `build`(CSV 메타데이터 `labbuild`·`labcvars`·`labdriver`를 `키=값;`으로 이음, 없으면 빈칸). `--rebuild`는 기존 행도 원본 CSV로 다시 계산한다. 태그 `aa*` 라벨은 summary에서 `<변형>-aa` 조건으로 따로, `trace*`는 summary에서 뺀다.
- `plot.py`: `n-at-60fps.svg`는 프레임 p95 기준 N, 막대 옆에 p50 N과 병목. `n-scaling-P1.svg`는 p95 실선 + p50 점선과 16.7ms 선. `Docs/figures/n-at-60fps.csv`에 값(p95 N, p50 N, 병목). 30k 점은 summary에 있으면 자연히 들어간다. `-aa` 조건은 곡선에 잇지 않는다.
- `spikes.py`: 사용법은 commands.md. 사건 = 중앙값 2배 초과 프레임(붙은 것은 하나로 묶음), 시각 = 프레임 시간 누적합. 주기 판정 = 사건 5개 이상이고 간격이 간격 중앙값의 ±10% 안인 비율 60% 이상. 임계값(5개, 60%)은 측정 세션이 정했다.
- `run-bench.ps1`: `-RunIndex`, `-Trace`(`Saved/Traces/<라벨>.utrace`), `-ExtraArgs`, 변형 이름 `m3vis0 m3vis15 m3hys20 m0full m0static staticpawn m1mass m2mass m3mass`를 ValidateSet에 추가. 게임이 이 이름을 모르면 실행이 실패한다(코드 세션 몫).
- `run-m3b.ps1`(초안): 위상 e3b(기준점 m3mass 포함 24회 섞기, `-NoBase`면 18회) → matrix(M0 24 + staticpawn 6 + M1\~M3 90) → aa 3 → trace 2. 143회 + 버리는 r0 1 = 144회, 약 204분(85초/회). `-DryRun`으로 목록 확인(동작 확인함). 계획서의 "약 170회"는 budget·uro·pawn 같은 P1 전용 변형까지 센 값으로 보이고 이 러너에는 넣지 않았다.
- `Docs/Reference/pc-specs.md`: CPU·RAM·GPU·드라이버·OS·엔진 커밋을 채웠다. commands.md에 행을 더했다.
- 변형 이름 갱신(대표값 결정 1·결정 4, 2026-11-09): E3b 기준점 `m3mass`→`m3vis30`(조건 이름 `m3vis0`·`m3vis15`·`m3hys20`이 그대로 남고 24회 유지. `m3mass`를 4번째로 두면 `m3vis15`와 같은 기본값이 두 이름으로 나온다), ab2의 `m3noorder`→`m3order`. `-DryRun`으로 e3b 25행·ab2 9행 확인, 모든 변형이 `LabConfig.cpp` `-LabStage`에 있음. 커밋 2fb7507.
- 2종 측정 준비(코디네이터 지시, 2026-11-09): `m3sig`를 ValidateSet에 더했고 `run-bench.ps1 -Types 2`(→ `-LabTypes=2`, 라벨은 `-Tag`로 구분)를 더했다. `run-m3b.ps1 -Phase types2` = 묶음 2: 묶음 1 전수 매트릭스와 같은 40조건 x 3회 = 120회를 태그 `t2`(summary 조건 `<변형>-t2`)로 + E3 (3c) 20k P1 3회 12회(`m3budgetdist`·`m3sig` 각각 1종/2종 쌍. 3a 비교 상대 `m3budgetdist`는 `LabConfig.cpp` 변형 이름이고, 2종끼리 비교하려고 2종도 넣었다). `-DryRun`: 묶음 1 `-Phase all` 158회(+r0), 묶음 2 132회(+r0, 약 188분). `summarize.py`: runs.csv에 `types`·`types_assets`·`large_actor_frames`·`large_actor_skips`·`large_skip_ratio`(= Skips ÷ Frames) 열, summary에 `types`·`types_assets`·`large_skip_ratio`(중앙값) 열, `LabTypesAssets=substituted` 행은 summary에서 빼고 경고를 찍는다(runs.csv에는 남는다), 태그 `t2`는 `-t2` 조건으로 따로 요약, `plot.py`는 `-t2`를 곡선에서 뺀다. 옛 CSV(해당 메타데이터 없음)는 새 열이 빈칸. 실행·빌드 없음. 커밋 0a47b9f.

### 측정 전 점검표 (묶음 1·2, 2026-11-09)

1. **PC 상태.** 에디터·Epic Games Launcher·UBT/`dotnet` 빌드 서버·다른 게임이 꺼져 있다(`tasklist`로 확인). 에디터나 런처가 `.uproject`의 `EngineAssociation`을 다시 쓸 수 있다. 모니터는 켜 두고 화면 끄기·절전을 하지 않는다. 사용자는 측정 동안 PC를 쓰지 않는다.
2. **세션.** 다른 세션은 모두 대기이고 빌드·패키징·에디터를 하지 않는다. `git status`가 깨끗하고 `.uproject`가 바뀌지 않았다.
3. **패키지.** Test 패키지의 `commit.txt` 해시가 HEAD와 같다(dirty는 추적 안 된 `Config/DefaultEditor.ini`뿐). 묶음 1·2는 같은 패키지로 같은 날 연달아 돈다.
4. **순서와 r0.** 묶음 1 `-Phase e3b`(r0 포함) → `matrix -NoR0` → `ab2 -NoR0` → `aa -NoR0` → `trace -NoR0`. 묶음 2 `-Phase types2 -NoR0`. 실행 도중 실패가 있으면 멈추고 알린다. 같은 라벨은 다시 쓰지 않는다(`-Tag`).
5. **묶음 끝 검사.** `py -3 -I Scripts/spikes.py M3b- --verbose`로 주기적 스파이크가 없는지 본다. `summarize.py --milestone M3b` 뒤 `labbuild`가 한 값인지(`build` 열에 `//`가 없는지) 본다.
6. **A/A 판정.** 시작 A/A(`-Phase start`, 조건 `m3mass-start`, 필요하면 e3b 앞에 돌린다)와 끝 A/A(`m3mass-aa`)의 프레임 p50 차이가 0.5 ms 이하면 한 묶음으로 쓴다. 넘으면 그 사실을 적고 코디네이터가 판정한다.
7. **2종 행.** `LabTypesAssets=substituted`인 CSV는 summary에서 빠진다(경고가 찍힌다). 경고가 하나라도 있으면 보고한다. 2종 행의 `large_skip_ratio`도 같이 본다.

### M0\~M3 재요약

`summarize.py --milestone M<N> --rebuild`를 M0\~M3에 돌렸다. 재요약 전후 `runs.csv`·`summary.csv`의 기존 열을 모든 행에서 비교했다(행 수 89/63/71/118, 29/20/23/38 조건): **차이 0, 행 수 같음.** 새 열만 더해졌다(`frame_p95_ms`, summary는 `_min`/`_max`도, `bottleneck`, `build`). `build`는 M0\~M3 CSV에 `lab*` 메타데이터가 없어 비었다. `bottleneck`은 처음 정의(프레임 포함)에서는 항상 `frame`이라, 프레임을 빼는 정의로 바꿔 다시 요약했다(기존 열 차이 0 재확인). M0은 `game`, M1\~M3은 `gpu`다.

### N@16.7ms (P1, 로그 보간)

| 단계 변형 | p95 N | p50 N(이전 정의) |
| --- | --- | --- |
| M0 full | 667 | 813 |
| M0 static | 1,044 | 1,196 |
| M1 mass | 15,498 | 20,577 |
| M2 mass | 16,460 | 20,518 |
| M3 mass | 18,475 | 22,046 |
| M3 m2off(M2 + 200m 밖 Off) | 30,812 | 35,763 |

p50 열은 STATUS.md의 기존 수치(813 / 1,196 / 20,577 / 20,518 / 22,046 / 35,763)와 같다. M3 하위 실험(`m3*`)은 20k까지만 재서 20k에서 p95가 16.7ms 아래인 변형은 "범위 밖(20,000 이상)"으로 나온다. 전체는 `Docs/figures/n-at-60fps.csv`.

### 스파이크 검사 (`py -3 -I Scripts/spikes.py <접두사>`)

| 묶음 | 실행 수(r0 제외) | 주기적 실행 | 주기적 실행의 간격 중앙값 |
| --- | --- | --- | --- |
| M0 | 87 | 38 (44%) | 7.00초 (6.88\~13.99) |
| M1 (2026-11-08) | 60 | 16 (27%) | 7.00초 (6.98\~7.00) |
| M2 (2026-11-08) | 69 | 41 (59%) | 6.99초 (6.89\~7.01) |
| M3 (2026-11-09) | 114 | **0 (0%)**, 실행당 사건 중앙값 0 | |
| diag-* (11-09 진단) | 25 | 0 | |

**M1·M2 묶음에서 7.0초 주기가 뚜렷하고 M3 묶음에는 없다.** 규약의 기대와 일치한다. 해석상 주의:
- 판정이 엄격해서(±10%) 7초 사건에 다른 스파이크가 섞인 실행(M1 mass 1k P2 r3 6.38초, 2k P1 r3 5.51초 등)은 주기적이 아닌 것으로 센다. 27%\~59%는 하한이다. M1 mass 1k\~20k 실행은 거의 모두 간격 중앙값이 6.9\~7.0초였다.
- N=50k처럼 프레임이 느린 실행은 사건이 0\~2개라 주기를 판정할 수 없다(5개 미만).
- M0에도 7.00초 주기가 있다. 11-08 묶음만의 현상이 아니라 M3 묶음 전의 측정 전반에 있었다.
- `massav`(M1 5k)는 간격 약 2.0초(26\~30건)다. 저격 간격 2초와 같아 환경 잡음이 아니라 시나리오 효과(저격 사망 처리)로 보인다. 7초 주기와 구별된다.

### 질문과 답 (코디네이터/코드 세션)

1. `bottleneck`: 프레임을 빼고 Game/Render/GPU 중 최댓값으로 바꿨다(답: 코디네이터, d761020).
2. E3b 기준점: `m3mass`를 같은 섞기에 기본으로 넣는다(24회, `-NoBase`로 18회). 답: 코디네이터, d761020.
3. 변형 이름: 코드 세션이 별칭을 넣었다(`m0full`=full, `m0static`=static, `m1mass`·`m2mass`·`m3mass`=mass, `staticpawn`). `-LabStage`는 러너의 `-Stage`로 준다. 맞음.
4. 직렬 트레이스: 인자 `-LabFullyParallel=0`이 구현됐다. `run-m3b.ps1 -SerialArgs` 기본값과 같다.
5. 메타데이터 키: `LabBuild`·`LabCVars`·`LabDriver` 그대로. `summarize.py`의 `BUILD_KEYS`와 같다. `LabCVars` 값에 `|`가 있어 여러 빌드가 섞인 조건의 `build`는 ` // `로 잇는다. 패키지를 만든 뒤 실제 CSV로 한 번 확인한다.

### 선행 묶음 (측정 세션, 2026-11-09)

E3b (8) 표현 LOD 값(기준점 포함 24회 + 버리는 r0 1회)과 코드 정비 A/B(9회)를 한 패키지로 연달아 쟀다. 해석은 쓰지 않는다. 대표값 결정은 코디네이터 몫.

**실행 경위.** 패키지: `package.ps1 -Config Test` 두 번째 시도에서 성공(1분 29초). 첫 시도는 11초 만에 실패했다: UBT가 `HordeOptLab.Build.cs`를 여러 타깃(Editor, UnrealPak, ShaderCompileWorker, 게임)에서 동시에 평가하며 `Intermediate/Build/LabGenerated/LabBuildVersion.h`를 동시에 쓰다 `IOException: file used by another process`(`HordeOptLab.Build.cs:96`, `File.WriteAllText`). 헤더가 이미 현재 커밋 값이어서 쓰기가 생략된 재시도는 통과했다. **커밋이 바뀐 직후 첫 패키징에서는 다시 날 수 있다**(코드 세션이 고칠 것: 쓰기 재시도 또는 임시 파일 + 교체). `commit.txt` = `58674cea25d130c62a3d72e831451a6e2c669506 (dirty)`(dirty는 추적 안 된 `Config/DefaultEditor.ini`, 그리고 코드 세션이 고치는 중이던 `Build.cs`·이 Worklog). `build` 열 = `labbuild=58674cea25d1`(CSV 메타데이터 `labbuild`·`labcvars`·`labdriver`가 모두 채워짐, 드라이버 `NVIDIA GeForce RTX 4090 621.14`, `mass.FullyParallel=true`, `LockObserversDuringFlushing=true`). 실행: `run-m3b.ps1 -Phase e3b`(25회 33분 31초, 실패 0) → `-Phase ab -NoR0`(9회 12분 3초, 실패 0). 라벨 `M3b-<변형>-N<N>-P1-r<i>`, ab는 태그 `ab`(`M3b-<변형>-N20000-P1ab-r<i>`, summary 조건 `<변형>-ab`). 스파이크 검사 `spikes.py M3b-`: 실행 33개 중 주기적 0(실행당 사건 중앙값 0). 원본 CSV는 `Saved/Profiling/CSV/M3b-*.csv`, 요약은 `Measurements/M3b/`.

**표 1. E3b (8), P1, 3회 중앙값(최소\~최대), ms.** 액터 수 = CSV `Lab/Actors` p50(액터 총수, "화면 안 액터 수" 스탯은 없다). 전환/초 = `Lab/ActorRebinds` 합 ÷ 측정 시간.

| 변형 | N | 프레임 p50 | 프레임 p95 | Game p50 | Render p50 | GPU p50 | 액터 수 | 전환/초 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| m3mass(기준점) | 20k | 18.04 (17.78\~18.43) | 20.12 (19.68\~20.31) | 16.84 (16.61\~17.23) | 8.11 (7.92\~8.24) | 9.46 (9.41\~9.51) | 493 | 81.0 (80.5\~81.3) |
| m3vis0 | 20k | 16.74 (16.66\~19.21) | 19.80 (18.26\~21.06) | 15.74 (15.68\~18.04) | 7.16 (7.03\~10.37) | 9.13 (9.11\~9.22) | 482 (482\~483) | 103.3 (103.1\~103.6) |
| m3vis15 | 20k | 18.41 (18.26\~18.97) | 20.08 (20.00\~20.43) | 17.28 (17.16\~17.82) | 7.87 (7.81\~8.14) | 9.25 (9.24\~9.26) | 491 | 135.1 (134.7\~135.5) |
| m3hys20 | 20k | 18.15 (17.66\~18.30) | 19.50 (19.36\~19.89) | 16.97 (16.51\~17.11) | 7.83 (7.78\~7.90) | 9.26 (9.25\~9.29) | 493 | 81.2 (81.0\~81.7) |
| m3mass(기준점) | 5k | 7.30 (7.29\~8.23) | 8.45 (8.36\~9.31) | 6.74 (6.73\~7.62) | 3.95 (3.91\~4.13) | 5.84 (5.84\~5.85) | 262 | 28.7 (28.7\~28.7) |
| m3vis0 | 5k | 6.38 (6.38\~6.40) | 7.04 (6.97\~7.21) | 4.99 (4.91\~5.21) | 3.92 (3.85\~4.07) | 5.96 (5.93\~5.97) | 114 | 24.3 (24.3\~24.3) |
| m3vis15 | 5k | 6.55 (6.34\~6.76) | 7.75 (7.28\~7.85) | 6.04 (5.69\~6.26) | 3.90 (3.78\~3.99) | 5.89 (5.88\~5.91) | 172 | 29.6 (29.6\~29.6) |
| m3hys20 | 5k | 7.72 (7.33\~7.86) | 8.61 (8.40\~9.09) | 7.16 (6.77\~7.29) | 3.90 (3.85\~3.93) | 5.79 (5.78\~5.79) | 262 (262\~263) | 28.7 (28.7\~28.7) |

**표 2. 코드 정비 A/B, 20k P1, 3회, ms.** 정비된 코드의 `m3mass`(ab 묶음 안에서 다시 잼) vs `m3qold`(쿼리 정비 전 방식) vs `m3syncall`(ActorSync 필터 없음).

| 변형 | 프레임 p50 | 프레임 p95 | Game p50 | Render p50 | GPU p50 | 액터 수 | 전환/초 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| m3mass | 17.77 (16.25\~18.47) | 19.25 (17.71\~19.89) | 16.61 (15.19\~17.27) | 7.88 (7.08\~7.97) | 9.45 (9.42\~9.48) | 493 | 81.1 (80.8\~82.3) |
| m3qold | 17.01 (16.83\~17.73) | 18.95 (18.85\~19.35) | 15.83 (15.67\~16.53) | 8.03 (8.00\~8.07) | 9.60 (9.54\~9.62) | 495 | 87.1 (86.9\~87.5) |
| m3syncall | 18.04 (17.99\~18.72) | 20.02 (19.51\~20.11) | 16.87 (16.84\~17.51) | 8.03 (7.97\~8.14) | 9.46 (9.45\~9.49) | 495 | 86.0 (85.7\~86.1) |

**잡음 기준.** A/A 묶음(`-Phase aa`)은 아직 안 잼. 지금은 세 실행의 범위만 있다. 같은 조건 `m3mass` 20k P1이 한 패키지의 두 묶음(e3b 라운드, ab 라운드)에서 프레임 p50 18.04(17.78\~18.43)와 17.77(16.25\~18.47)로 나왔다. 참고 수치: 정비 전 코드의 M3 `m3base` 20k P1은 프레임 p50 15.46, p95 17.26(`Measurements/M3/summary.csv`, 11-09 이전 묶음, 비교는 "참고").

**메모.** `m3vis0` 20k의 프레임·Render 범위는 세 실행 중 한 실행이 큰 쪽이다(최대 프레임 p50 19.21, Render 10.37). 액터 수는 거의 상한(High 500) 근처에서 일정하고, `m3vis0`·`m3vis15`의 5k는 114·172로 다르다.

#### 잡음 진단(어피니티) (측정 세션, 2026-11-09)

**결과 먼저: 고정은 한 번도 걸리지 않았다.** `run-bench.ps1 -Affinity FFFF`의 16진수 파싱 오류(`[Convert]::ToInt64($Affinity -replace "^0x", "", 16)`가 인자 3개로 읽혀 마스크가 비었다)로 `aff` 6회 + r0 모두 `WARN: could not set affinity`, `affinity=FAILED`가 로그에 남았다. 그래서 라벨 `...P1aff-r*`와 `...P1noaff-r*` 12회는 **모두 고정 없음**이다. 이 12회는 같은 조건 A/A 12회로만 읽을 수 있다. 오류는 이어지는 커밋에서 고쳤다(`$Hex`로 나눔, 파싱 확인). 고정 비교는 다시 재야 한다(같은 라벨은 쓸 수 없어 새 태그 필요).

실행: `run-m3b.ps1 -Phase aff`, r0 1 + 12회, 17분 29초, 실패 0, `.uproject` 변경 없음, 패키지는 선행 묶음과 같은 `58674cea25d1`. `spikes.py`: 12회 모두 주기적 0. 값(m3mass 20k P1 `-LabStage=M3`, ms):

**`aff` 태그(실제로는 고정 없음) 6회**

| 라벨 | 프레임 p50 | 프레임 p95 | Game p50 | Render p50 |
| --- | --- | --- | --- | --- |
| aff-r1 | 16.25 | 17.86 | 15.19 | 7.01 |
| aff-r2 | 16.30 | 18.11 | 15.26 | 7.09 |
| aff-r3 | 15.87 | 17.44 | 14.84 | 6.84 |
| aff-r4 | 15.90 | 17.44 | 14.88 | 6.81 |
| aff-r5 | 16.11 | 17.61 | 15.05 | 6.96 |
| aff-r6 | 16.33 | 17.84 | 15.28 | 7.12 |
| 중앙값 / 최소\~최대 / 표준편차 | 16.18 / 15.87\~16.33 / 0.20 | 17.72 / 17.44\~18.11 / 0.27 | 15.12 / 14.84\~15.28 / 0.19 | 6.98 / 6.81\~7.12 / 0.13 |

**`noaff` 태그(고정 없음) 6회**

| 라벨 | 프레임 p50 | 프레임 p95 | Game p50 | Render p50 |
| --- | --- | --- | --- | --- |
| noaff-r1 | 15.87 | 17.34 | 14.85 | 6.71 |
| noaff-r2 | 16.25 | 17.74 | 15.20 | 7.08 |
| noaff-r3 | 16.31 | 17.88 | 15.26 | 7.10 |
| noaff-r4 | 16.21 | 17.73 | 15.19 | 6.96 |
| noaff-r5 | 16.26 | 18.07 | 15.22 | 6.96 |
| noaff-r6 | 16.02 | 17.48 | 14.99 | 6.82 |
| 중앙값 / 최소\~최대 / 표준편차 | 16.23 / 15.87\~16.31 / 0.17 | 17.73 / 17.34\~18.07 / 0.26 | 15.20 / 14.85\~15.26 / 0.16 | 6.96 / 6.71\~7.10 / 0.15 |

관찰만: 12회 전체 프레임 p50이 15.87\~16.33로 한 수준에 모였고, 선행 묶음 e3b·ab에서 보인 17.8\~18.5 수준은 이 12회에 없었다. `Measurements/M3b/summary.csv`의 `m3mass-aff`·`m3mass-noaff` 조건은 위 사정으로 둘 다 고정 없음이다.

**시간순 표(선행 묶음 34회 + 이번 13회, 2026-11-09).** "종료 시각"은 `Saved/Profiling/CSV/<라벨>.csv`의 수정 시각이다(실행이 끝나고 파일이 닫힌 때, 시작은 약 85초 전). 모든 `m3mass`·`m3vis*`·`m3hys20`·`m3qold`·`m3syncall` 행은 20k 또는 5k P1 `-LabStage=M3`다. 16:33과 17:15 사이 42분은 다른 세션(코드 세션의 화질 캡처)이 PC를 썼고 이 세션은 실행이 없었다. 20k 행의 프레임 p50: 15:48\~16:33 구간은 16.25\~19.21, 17:15\~17:31 구간은 15.87\~16.45.

| 순서 | 라벨 | 종료 시각(CSV 수정 시각) | 프레임 p50 |
| --- | --- | --- | --- |
| 1 | m3mass-N20000-P1-r0 | 15:48:54 | 15.99 |
| 2 | m3mass-N20000-P1-r1 | 15:50:15 | 17.78 |
| 3 | m3vis0-N20000-P1-r1 | 15:51:35 | 16.66 |
| 4 | m3vis15-N20000-P1-r1 | 15:52:56 | 18.26 |
| 5 | m3hys20-N20000-P1-r1 | 15:54:17 | 18.30 |
| 6 | m3mass-N5000-P1-r1 | 15:55:37 | 7.29 |
| 7 | m3vis0-N5000-P1-r1 | 15:56:57 | 6.40 |
| 8 | m3vis15-N5000-P1-r1 | 15:58:18 | 6.34 |
| 9 | m3hys20-N5000-P1-r1 | 15:59:38 | 7.86 |
| 10 | m3mass-N20000-P1-r2 | 16:00:59 | 18.43 |
| 11 | m3vis0-N20000-P1-r2 | 16:02:19 | 19.21 |
| 12 | m3vis15-N20000-P1-r2 | 16:03:39 | 18.41 |
| 13 | m3hys20-N20000-P1-r2 | 16:05:00 | 17.66 |
| 14 | m3mass-N5000-P1-r2 | 16:06:20 | 8.23 |
| 15 | m3vis0-N5000-P1-r2 | 16:07:41 | 6.38 |
| 16 | m3vis15-N5000-P1-r2 | 16:09:01 | 6.76 |
| 17 | m3hys20-N5000-P1-r2 | 16:10:22 | 7.33 |
| 18 | m3mass-N20000-P1-r3 | 16:11:42 | 18.04 |
| 19 | m3vis0-N20000-P1-r3 | 16:13:03 | 16.74 |
| 20 | m3vis15-N20000-P1-r3 | 16:14:23 | 18.97 |
| 21 | m3hys20-N20000-P1-r3 | 16:15:44 | 18.15 |
| 22 | m3mass-N5000-P1-r3 | 16:17:04 | 7.30 |
| 23 | m3vis0-N5000-P1-r3 | 16:18:24 | 6.38 |
| 24 | m3vis15-N5000-P1-r3 | 16:19:45 | 6.55 |
| 25 | m3hys20-N5000-P1-r3 | 16:21:05 | 7.72 |
| 26 | m3mass-N20000-P1ab-r1 | 16:22:26 | 16.25 |
| 27 | m3qold-N20000-P1ab-r1 | 16:23:47 | 17.01 |
| 28 | m3syncall-N20000-P1ab-r1 | 16:25:07 | 18.72 |
| 29 | m3mass-N20000-P1ab-r2 | 16:26:28 | 17.77 |
| 30 | m3qold-N20000-P1ab-r2 | 16:27:48 | 16.83 |
| 31 | m3syncall-N20000-P1ab-r2 | 16:29:08 | 17.99 |
| 32 | m3mass-N20000-P1ab-r3 | 16:30:29 | 18.47 |
| 33 | m3qold-N20000-P1ab-r3 | 16:31:49 | 17.73 |
| 34 | m3syncall-N20000-P1ab-r3 | 16:33:09 | 18.04 |
| 35 | m3mass-N20000-P1aff-r0 | 17:15:32 | 16.45 |
| 36 | m3mass-N20000-P1aff-r1 | 17:16:53 | 16.25 |
| 37 | m3mass-N20000-P1noaff-r1 | 17:18:14 | 15.87 |
| 38 | m3mass-N20000-P1aff-r2 | 17:19:35 | 16.30 |
| 39 | m3mass-N20000-P1noaff-r2 | 17:20:55 | 16.25 |
| 40 | m3mass-N20000-P1aff-r3 | 17:22:16 | 15.87 |
| 41 | m3mass-N20000-P1noaff-r3 | 17:23:37 | 16.31 |
| 42 | m3mass-N20000-P1aff-r4 | 17:24:58 | 15.90 |
| 43 | m3mass-N20000-P1noaff-r4 | 17:26:18 | 16.21 |
| 44 | m3mass-N20000-P1aff-r5 | 17:27:39 | 16.11 |
| 45 | m3mass-N20000-P1noaff-r5 | 17:29:00 | 16.26 |
| 46 | m3mass-N20000-P1aff-r6 | 17:30:21 | 16.33 |
| 47 | m3mass-N20000-P1noaff-r6 | 17:31:41 | 16.02 |

#### 코드 A/B(D안) (측정 세션, 2026-11-09)

패키지: `package.ps1 -Config Test` **첫 시도에서 성공**(1분 23초, `Build.cs` 경합 없음). `commit.txt` = `22679a9a54980f307b666e31cea0ace36f72b3e9 (dirty)`(dirty는 추적 안 된 `Config/DefaultEditor.ini`), 코드 세션의 `98be269` 포함, CSV `labbuild` = `22679a9a5498`. 실행 전 `.uproject` 변경 없음, 에디터·게임 프로세스 없음. `run-m3b.ps1 -Phase start`(r0 + 3회, 5분 22초) → `-Phase ab2 -NoR0`(9회, 12분 7초), 실패 0, 경고 0. `spikes.py`: 12회 모두 주기적 0. 조건은 모두 `-LabStage=M3`, 20k P1, 3회, 단위 ms, 값은 중앙값(최소\~최대).

**CSV 메타데이터 확인.** `labcvars`의 `mass.commands.LockObserversDuringFlushing`: `m3lock0` 3회 모두 `false`, `m3mass`(start 3회 + ab2 3회)와 `m3noorder` 3회는 모두 `true`. `mass.FullyParallel`은 전부 `true`(13개 CSV 모두 확인).

| 조건 | 프레임 p50 | 프레임 p95 | Game p50 | Render p50 | GPU p50 | LockObservers(`labcvars`) |
| --- | --- | --- | --- | --- | --- | --- |
| m3mass-start(시작 A/A) (3회) | 16.03 (15.98\~16.03) | 17.74 (17.50\~17.77) | 14.97 (14.95\~15.01) | 7.03 (6.95\~7.11) | 9.41 (9.38\~9.43) | true |
| m3mass-ab2 (3회) | 16.12 (15.94\~16.26) | 17.80 (17.70\~18.09) | 15.07 (14.91\~15.21) | 7.04 (6.80\~7.06) | 9.46 (9.45\~9.47) | true |
| m3lock0-ab2 (3회) | 16.27 (16.05\~16.52) | 18.18 (17.83\~18.33) | 15.21 (15.01\~15.44) | 7.11 (7.02\~7.28) | 9.47 (9.40\~9.48) | false |
| m3noorder-ab2 (3회) | 15.52 (15.45\~15.75) | 17.19 (17.15\~17.32) | 14.46 (14.39\~14.67) | 7.09 (7.07\~7.19) | 9.44 (9.41\~9.46) | true |

**실행별(시간순, 종료 시각 = CSV 수정 시각).** start 3회는 묶음 맨 앞(패키징 직후), ab2는 그 뒤에 변형을 실행 단위로 섞었다.

| 라벨 | 종료 시각 | 프레임 p50 | 프레임 p95 | Game p50 | Render p50 | GPU p50 |
| --- | --- | --- | --- | --- | --- | --- |
| m3mass-N20000-P1start-r1 | 18:53 | 16.03 | 17.77 | 15.01 | 7.11 | 9.43 |
| m3mass-N20000-P1start-r2 | 18:54 | 16.03 | 17.74 | 14.97 | 7.03 | 9.38 |
| m3mass-N20000-P1start-r3 | 18:56 | 15.98 | 17.50 | 14.95 | 6.95 | 9.41 |
| m3mass-N20000-P1ab2-r1 | 18:57 | 16.12 | 17.80 | 15.07 | 7.04 | 9.47 |
| m3lock0-N20000-P1ab2-r1 | 18:59 | 16.27 | 18.33 | 15.21 | 7.11 | 9.48 |
| m3noorder-N20000-P1ab2-r1 | 19:00 | 15.45 | 17.15 | 14.39 | 7.07 | 9.41 |
| m3mass-N20000-P1ab2-r2 | 19:01 | 15.94 | 17.70 | 14.91 | 6.80 | 9.46 |
| m3lock0-N20000-P1ab2-r2 | 19:03 | 16.05 | 17.83 | 15.01 | 7.02 | 9.47 |
| m3noorder-N20000-P1ab2-r2 | 19:04 | 15.52 | 17.32 | 14.46 | 7.09 | 9.46 |
| m3mass-N20000-P1ab2-r3 | 19:05 | 16.26 | 18.09 | 15.21 | 7.06 | 9.45 |
| m3lock0-N20000-P1ab2-r3 | 19:07 | 16.52 | 18.18 | 15.44 | 7.28 | 9.40 |
| m3noorder-N20000-P1ab2-r3 | 19:08 | 15.75 | 17.19 | 14.67 | 7.19 | 9.44 |

## 코드 정비 (코드 세션, 2026-11-09)

plan.md "M3b" 코드 정비 핵심 5 + 두 줄 + 기본 프로세서 점검. 대상 소스는 `Source/HordeOptLab/LabMassProcessors.cpp`(Approach, Contact, Snipe, State, DamageGen, DamageApply), `LabVatProcessor.cpp`, `LabActorSyncProcessor.cpp`, `LabCrowdSubsystem.cpp`, `LabOrb.cpp`(커밋 915773c 기준). 설계는 사용자와 하나씩 정했다(아래 "결정").

### 프로세서별 읽기·쓰기 표(정비 전)

표기: **W** 쓰기(ReadWrite 선언), **R** 읽기(ReadOnly 선언), **tag** 아키타입 태그 판정(`DoesArchetypeHaveTag`, 선언 없음), **defer** 지연 명령으로 태그 변경, **밖** 쿼리 밖 접근(`GetFragmentDataChecked`·`GetFragmentDataPtr`). 엔진 프래그먼트는 `FTransformFragment` = Xf, `FMassMoveTargetFragment` = MoveTgt, `FMassRepresentationFragment` = Rep, `FMassRepresentationLODFragment` = RepLOD, `FMassActorFragment` = Actor.

| 프로세서(실행 순서 선언) | Xf | MoveTgt | FLabHealth | FLabMinion(Serial, StateTimeLeft) | Rep | RepLOD | Actor | 태그 Hit / Dead / CodeDrivenMovement | 쿼리 밖 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Approach(그룹 Tasks) | R | W | | | | | | Hit·Dead 없음 필터 | |
| OrbContact(after Movement) | R | | W | W(StateTimeLeft) | | | | 없음 필터, defer +Dead −Code | 없음. 공유 상태 `Crowd->Nearest`·`KillCount`는 잠금 |
| OrbSnipe(after Contact) | R | | R | W 선언, **W는 쿼리 밖** | | | | 없음 필터, defer +Hit −Code | `GetFragmentDataChecked<FLabMinionFragment>(Pick.Target).StateTimeLeft = HitDuration`(`LabMassProcessors.cpp:234`). `Crowd->PendingSnipeTick` 읽고 0으로 씀(오브 틱이 씀, 잠금 없음) |
| MinionState(after Snipe) | 밖 R | | | W | | | | Hit·Dead 중 하나 필터, defer −Hit +Dead, DestroyEntities | 개발 빌드 진단 `GetFragmentDataPtr<FTransformFragment>`(`:286`). `StateTimeLeft = DeathDuration`은 즉시 쓰고 태그는 지연 → 그 프레임에 Hit 태그 + Dead 시간(plan.md 3번) |
| DamageGen(after State, E1 (d)·kill 변형만) | | | W | W(StateTimeLeft) | | | | 없음 필터, defer +Hit −Code | 이벤트 엔티티 생성은 지연 명령 안(`FMassDeferredCreateCommand`) |
| DamageApply(after DamageGen) | | | W 선언(HealthQuery, 실제 접근은 밖) | | | | | | `GetFragmentDataPtr<FLabHealthFragment>(Event.Target)`(`:461`). HealthQuery는 솔버에 쓰기를 알리려고 둔 것 |
| VatISM(엔진 `UMassUpdateISMProcessor` 상속, after Representation, 게임 스레드) | R | | | **R(StateTimeLeft, Serial)** | W | R | | **tag** Hit·Dead | 없음. 공유 상태 `Crowd->VatClips`·`SnipeView`·`HitDuration`·`DeathDuration` 읽음 |
| ActorSync(after Representation, 게임 스레드) | R | | | **R(StateTimeLeft, Serial)** | R | R | W | **tag** Hit·Dead. 액터 유무 필터 없음 → 전 엔티티 순회(plan.md 4번) | 없음. `ALabMinionMassActor`·AnimInstance를 직접 씀 |
| ULabCrowdSubsystem::SpawnEntities(게임 스레드, 생성 직후) | 밖 W | | 밖 W | 밖 W(Serial) | | | | | `GetFragmentDataChecked` 셋(`LabCrowdSubsystem.cpp:138-140`). 생성 컨텍스트 안의 초기화라 처리 중 접근은 아니다. 그대로 둔다 |
| ALabOrb::TickMass(액터 틱) | | | | | | | | | 프래그먼트 없음. `OrbCenter`·`SnipeView`·`PendingSnipeTick` 씀, `SnipeTargets`·`Nearest`는 잠금으로 읽음 |
| 엔진: Steering·Movement | W(Movement) | R | | | | | | Code 필터 | |
| 엔진: Representation·VisualizationLOD·LODCollector | R | | | | W | W(VisualizationLOD) | W | | 액터 붙이기·떼기는 `UMassRepresentationActorManagement` 콜백 |

표가 보여 주는 결함(plan.md 코드 정비 1\~4와 같다): (1) 표현 둘(VatISM, ActorSync)이 로직 프래그먼트 `FLabMinionFragment`를 읽는다. 쓰는 쪽이 넷(Contact, Snipe, State, DamageGen)이라 솔버가 표현을 그 넷과 묶고, 병렬 실행에서 게임 스레드가 자원 대기로 돈다(M3 diag-vt*, engine-notes.md 11절). (2) Snipe가 쿼리 밖에서 `FLabMinionFragment`를 쓴다. (3) State가 시간은 즉시, 태그는 지연으로 바꿔 한 프레임 동안 Hit 태그에 Dead 시간이 들어 있다. VatISM이 그 프레임에 돌면(순서 선언 없음) Hit 클립 시각이 음수가 되어 0으로 잘린다. (4) VatISM·ActorSync와 로직 사이에 실행 순서 선언이 없다. (5) ActorSync는 액터가 붙은 엔티티(20k P1에서 약 500)를 찾으려고 20k를 모두 순회한다.

### 결정(사용자, 2026-11-09, 하나씩)

1. **표현 전용 프래그먼트 `FLabAnimStateFragment { ELabMinionState State; float StateStartTime; }`**(A안). 재생 시각 = 월드 시각 − StateStartTime. Walk는 StateStartTime = −위상(`LabWalkPhase(Serial)`)이라 M2와 같은 위상이다. 쓰는 쪽: SpawnEntities(Walk), Contact(Dead, 쿼리 안), State(Dead, 쿼리 안, `StateTimeLeft`와 같은 자리), DamageGen 치명(Hit, 쿼리 안), Snipe(Hit, 지연 명령 `FMassDeferredSetCommand`로 태그 추가와 같은 플러시에서. 대상이 순회 뒤에 정해져 쿼리 안에서 쓸 수 없다). 읽는 쪽: VatISM, ActorSync만. 둘은 태그를 보지 않고 `State`만 보므로 3번(한 프레임 튐)이 구조적으로 사라진다. 버린 안 B(표현 프래그먼트에 `StateTimeLeft` 사본을 매 프레임 복사): 표현이 여전히 태그를 봐야 해서 3번을 따로 고쳐야 한다.
2. **그룹 `LabLogic`**(A안). Contact·Snipe·State·DamageGen·DamageApply가 `ExecuteInGroup = "LabLogic"`, VatISM·ActorSync가 `ExecuteAfter("LabLogic")`. Approach는 엔진 조향이 Tasks 뒤에 돌아야 해서 `Tasks`에 남긴다(MoveTgt만 써서 표현과 무관). 솔버는 그룹 이름 의존성을 자식 노드 전부로 펼친다(`MassProcessorDependencySolver.cpp:680-719`). 엔진(`Movement`·`Tasks`·`Representation`)과 City Sample(`Behavior`·`Traffic`)이 같은 방식이다. 버린 안 B(`ExecuteAfter(State)`만): 체인 끝이 바뀌면 같이 고쳐야 한다.
3. **액터 태그 `FLabActorTag` + 자가 정리**(A안). 붙이기: `ULabActorManagement::OnPostActorSpawn`(새 스폰과 풀 재사용 모두 이 델리게이트를 지난다, `MassActorSpawnerSubsystem.cpp:255,480`)에서 `EntityManager->Defer().AddTag`. 스포너 서브시스템 틱 중 Mass 처리가 워커에서 돌 수 있어 즉시 바꾸지 않는다(`MassEntityManager.h:1625`). 떼기: ActorSync가 "태그는 있는데 `FMassActorFragment`가 비었다"를 보면 `Defer().RemoveTag`. 떼는 쪽 엔진 훅이 없어서다(`ReleaseActorOrCancelSpawning`은 static, `MassRepresentationProcessor.cpp:481`). 표현 종류가 아니라 액터 포인터로 판단하는 이유: 액터를 한 프레임 더 두는 동안 LOD가 되돌아오면 엔진이 같은 액터를 다시 써서(`MassRepresentationProcessor.cpp` "NewActor = Actor") OnPostActorSpawn이 다시 불리지 않는다. 버린 안 B(엔진 `FMassVisualizationChunkFragment`의 표현 종류 비트로 청크 거르기): 액터 엔티티가 청크에 흩어져 있어 거의 거르지 못한다. 확인 스탯 `Lab/ActorTagsRemoved`.
4. **표현 프로세서의 `ExecuteAfter("LabLogic")` 선언을 뺀다**(코디네이터 확정, 2026-11-09, 커밋 4f6db5b). 결정 2의 그룹 `LabLogic` 선언(Contact·Snipe·State·DamageGen·DamageApply의 `ExecuteInGroup`)은 그대로 두고, VatISM·ActorSync 쪽의 `ExecuteAfter(LabLogic)`만 지운다. 순서는 솔버가 프래그먼트 접근(`FLabAnimStateFragment` 쓰기 → 읽기)으로만 정한다. 근거: 20k P1 `m3noorder` 15.52 vs `m3mass` 16.12ms, 세 쌍 모두 0.4\~0.7 빠름(위 "코드 A/B(D안)"). 바뀐 줄: `LabVatProcessor.cpp`·`LabActorSyncProcessor.cpp` 생성자의 `if (!LabUseOldQueries() && !LabSkipLogicOrder())` → `if (LabExplicitLogicOrder())`, `LabMassProcessors.h`의 `LabSkipLogicOrder()`(m3noorder) → `LabExplicitLogicOrder()`(m3order). 변형 `m3noorder`는 지우고 "선언 있음"을 재는 A/B 변형 `m3order`를 남긴다(묶음 1 A/B). `mass.commands.LockObserversDuringFlushing=1`은 비용 없음(`m3lock0` 16.27 vs 16.12)이라 유지하고 `m3lock0` 변형도 유지한다.
5. **M3 대표의 Representation LOD 기본값을 화면 밖 Base 15m, 화면 안 Visible 30m로**(결정 1 B안, 코디네이터 확정 2026-11-09, 커밋 7f0236f). 근거: 5k 빠른 회전에서 `m3vis15`가 `m3vis0`보다 회복이 빠르거나 같다(near_vat 최댓값 중앙값 21 vs 40, near_actors 최솟값 중앙값 119 vs 99, 미달 0/12 둘 다, 회복(직전) 중앙값 5 vs 11. 위 "빠른 회전 > 5k"). 히스테리시스는 엔진 기본 10% 그대로. 바뀐 줄: `LabCrowdSubsystem.cpp:108-113`(`if (bM3)` 블록. `BaseLODDistance[Medium]`을 `m3vis0`은 0, `m3vis30`·`m3hys20`·`m3hyd15`·`m3hyd50`은 Visible과 같은 `HydrateCm`, 나머지 M3 변형은 1500으로), `LabConfig.h:45`(`M3Vis30`), `LabConfig.cpp:52,122`. 변형 정리: `m3vis15`는 기본값과 같아져 별칭으로 남긴다(`run-m3b.ps1`이 쓴다), `m3vis0`은 대조군, 새 `m3vis30`은 바꾸기 전 대표값(30/30m)을 재는 E3b 기준점, `m3hys20`은 그대로(30/30m + 20%). M2 이하는 닿지 않는다: 새 값은 `bM3` 안에만 있고, 같은 블록을 지나는 M2 변형 `vatcull`·`vatcullvis`는 그 전과 같은 Base = Visible이다. 그 밖의 M3 변형(`m3colquery`·`m3nocol`·`m3uro`·`m3budget*`·`m3noactor`·`m3qold`·`m3syncall`·`m3lock0`·`m3order`)도 기본값을 따라 15m가 된다(E3·E3b 태그 시점의 값과 다르다. 최종 비교표는 한 빌드에서 다시 잰다).

### 바꾼 것

- `LabMassTypes.h`: `FLabAnimStateFragment`, `FLabActorTag`, `LabWalkPhase()`. `LabMassTraits.cpp`: 템플릿에 `FLabAnimStateFragment` 추가.
- `LabMassProcessors.*`: 그룹 이름 `LabLogicGroupName`, `LabUseOldQueries()`(m3qold). Contact·State·DamageGen이 `FLabAnimStateFragment`를 쿼리 안에서 쓴다. Snipe는 `FLabMinionFragment`와 `FLabAnimStateFragment`를 쓰기로 선언하고, 고른 대상은 지연 명령으로 쓴다(m3qold는 정비 전처럼 쿼리 밖 쓰기). `PendingSnipeTick`은 `std::atomic<uint32>`로 `exchange(0)`. State는 `FTransformFragment`를 읽기로 선언해 진단 로그의 쿼리 밖 읽기를 없앴다.
- `LabVatProcessor.*`: `FLabAnimStateFragment`만 읽고 태그를 보지 않는다. `ExecuteAfter("LabLogic")`(결정 4, 4f6db5b부터는 `m3order`만 선언하고 기본은 선언 없음). m3qold는 `FLabMinionFragment` + 태그, 순서 선언 없음.
- `LabActorSyncProcessor.*`: 위와 같은 읽기·순서. 쿼리에 `FLabActorTag` All 필터(m3qold·m3syncall은 없음). 액터가 비면 태그를 뗀다. 재바인딩 판정을 `BoundSerial`(로직 프래그먼트) 대신 `BoundEntity`(엔티티 핸들)로. `OnPostActorSpawn`이 태그를 붙인다.
- `LabMinion.*`: `ALabMinionStaticPawn : ALabMinionStatic`(정적 메시 + CharacterMovement 틱 끔). `LabCrowdSpawner.cpp`가 `staticpawn`에 쓴다.
- `LabConfig.*`: 변형 `staticpawn`, `m3qold`, `m3syncall`, `m3vis0`, `m3vis15`, `m3hys20`, `m3lock0`(98be269), `m3order`(4f6db5b. 그 전 98be269\~7f0685a의 `m3noorder`는 기본값이 되어 지웠다), `m3vis30`(7f0236f. `m3vis15`는 기본값의 별칭이 되었다). 별칭 `m0full` = full, `m0static` = static, `m1mass`·`m2mass`·`m3mass` = mass(설정 같고 라벨만 다르다. `-LabStage`는 러너가 따로 준다). 인자 `-LabFullyParallel=0`(게임 모드가 `mass.FullyParallel`을 0으로, massst와 같은 경로. 측정 세션 질문 3·4의 답).
- `LabCrowdSubsystem.cpp`: SpawnEntities가 `FLabAnimStateFragment`를 Walk·−위상으로 초기화. E3b (8) 변형: `m3vis0`는 `BaseLODDistance[Medium] = 0`(화면 밖 High 밴드 0m), `m3vis15`는 1500, `m3hys20`은 `BufferHysteresisOnDistancePercentage = 20`(`FMassVisualizationLODParameters`, `MassRepresentationFragments.h:299-309`). 7f0236f부터 기본값이 Base 15m이고 `m3vis30`이 그 전 30/30m다(결정 5). 코드 정비 목록 밖이지만 `run-m3b.ps1`이 이 이름을 부르므로 함께 넣었다.
- `Config/DefaultEngine.ini`: `[ConsoleVariables] mass.commands.LockObserversDuringFlushing=1`. 이 섹션은 GEngineIni에서 `SetBySystemSettingsIni`로 읽힌다(`ConsoleManager.cpp:607`).
- `LabGameMode.cpp`·`HordeOptLab.Build.cs`: CSV 메타데이터(아래).

### CSV 메타데이터 키(측정 세션 질문 5의 답)

| 키(CSV에는 소문자) | 값 | 출처 |
| --- | --- | --- |
| `LabBuild` | 빌드 커밋 해시 12자리. 작업 트리 dirty 여부는 모른다(그것은 `package.ps1`의 `commit.txt`) | `HordeOptLab.Build.cs`가 `.git/HEAD`와 ref 파일을 읽어 `Intermediate/Build/LabGenerated/LabBuildVersion.h`에 `LAB_BUILD_COMMIT`을 쓴다. 두 파일을 `ExternalDependencies`에 넣어 커밋이 바뀌면 UBT가 Build.cs를 다시 평가한다. git 실행 없음 |
| `LabCVars` | `이름=값`을 `\|`로 이음. r.VSync, t.MaxFPS, r.ScreenPercentage, r.DynamicRes.OperationMode, sg.ShadowQuality, sg.AntiAliasingQuality, sg.GlobalIlluminationQuality, sg.EffectsQuality, r.DynamicGlobalIlluminationMethod, r.Shadow.Virtual.Enable, r.AntiAliasingMethod, mass.FullyParallel, mass.commands.LockObserversDuringFlushing. 쉼표는 CSV 메타데이터 행을 깨므로 쓰지 않는다 | `LabGameMode.cpp` `LabFixedCVarString()`, 캡처 시작 직전 값 |
| `LabDriver` | `<어댑터 이름> <사용자 드라이버 버전>` | `GRHIAdapterName`, `GRHIAdapterUserDriverVersion`(`RHIGlobals.h:819-821`) |

개발 실행 로그에도 같은 값이 `lab_build commit=… driver=… cvars=…` 한 줄로 남는다.

### 확인만 한 것(6번, 걷기 루프 끝)

가설 "걷기 루프 `Fmod(Local, Length)`가 End + 0.99까지 가서 다음 클립 첫 행이 섞인다"는 사실이 아니다. 엔진 머티리얼 함수 `BoneIndexUVs`의 Custom 노드가 `UV.y = (0.5 / TextureSize.y) + (int(Frame) / TextureSize.y)`로 행을 정수로 자르고, 굽는 텍스처의 필터가 `TF_Nearest`다(`AnimToTextureUtils.h:269`). End + 0.99는 End 행을 읽는다. 고치지 않았다(engine-notes.md 12절).

### 엔진 기본 프로세서 점검

개발 빌드 `-LabStage=M3 -LabN=2000`에 `-LogCmds="LogMass Log"`를 주면 `UMassCompositeProcessor`가 처음 만든 "Pre Physics Group flat processing graph"를 찍는다(`mass.LogNewProcessingGraph` 기본 1, `MassProcessor.cpp:555`). 우리 프로세서는 모두 PrePhysics 단계이고 다른 다섯 단계의 그래프는 비어 있다. 정비 전 23개가 등록돼 있었다.

| 처리 | 프로세서 | 근거 |
| --- | --- | --- |
| 끔(8) | `MassLookAtTargetGridProcessor`, `MassZoneGraphAnnotationTagUpdateProcessor`(MassAIBehavior), `MassEnvQueryTestProcessor_MassEntityTags`, `MassEnvQueryGeneratorProcessor_MassEntityHandles`(MassEQS), `MassActiveSmartObjectSignalProcessor`(MassSmartObjects), `DebugVisLocationProcessor`(MassGameplayDebug), `MassNavigationSmoothHeightProcessor`(MassNavigation), `MassUpdateInstancedSkinnedMeshProcessor`(MassRepresentation) | 우리 엔티티에 없는 프래그먼트(LookAt, ZoneGraph, EQS, SmartObject, 디버그, 높이 보정, ISKM)를 요구해 매 프레임 빈 순회와 그래프 노드만 만든다. `MassNavigationSmoothHeightProcessor`는 다른 16개 뒤에 오도록 의존성이 걸려 있었다. `Config/DefaultMass.ini`에 `bAutoRegisterWithProcessingPhases=False`. ISKM(M5)에서 `MassUpdateInstancedSkinnedMeshProcessor`를 다시 켠다 |
| 둠(엔진 7) | `MassApplyForceProcessor`, `MassApplyMovementProcessor`, `MassSmoothOrientationProcessor`, `MassSteerToMoveTargetProcessor`, `MassLODCollectorProcessor`, `MassVisualizationLODProcessor`, `MassVisualizationProcessor` | 이동·조향·LOD·표현. `MassApplyForceProcessor`는 힘이 0이라 결과를 바꾸지 않지만 이동 Trait의 일부이고 `massav` 변형의 회피가 힘을 쓴다. M4 (d)에서 이동을 우리 프로세서로 바꾸면 그때 다시 본다 |
| 둠(Lab 8) | Approach, OrbContact, OrbSnipe, MinionState, DamageGen, DamageApply, VatISM, ActorSync | DamageGen·DamageApply는 변형이 아니면 첫 분기에서 돌아간다 |

정비 뒤 그래프(15개): ApplyForce, LODCollector → ApplyMovement → SmoothOrientation → **Contact → Snipe → State → DamageGen → DamageApply**, VisualizationLOD → Visualization, Approach → SteerToMoveTarget, **ActorSync·VatISM은 로직 다섯과 Visualization 뒤**. 선언한 순서가 그래프에 그대로 나온다.

### 확인

- `Scripts/build.ps1` 성공(경고는 엔진 API 폐기 예고뿐, 정비 전과 같다).
- `run-dev.ps1 -Small`(N=100, P1, 20초) 종료 코드 0: `-LabStage=M3` mass, M2 mass, M1 mass, M0 full. 새 변형(`m3qold`·`m3syncall` M3, `staticpawn` M0)도 0. 모두 `respawns=43 kills=8 snipes=39`로 같다(시드·시나리오가 그대로다). 프로세서를 끈 뒤 M3·M2·M1을 다시 돌려 0.
- M3 N=2000 13초: `lab_actors synced=120\~133 rebinds` 로그로 태그 필터 상태에서 액터 동기화가 돈다. `lab_build commit=6274752b9668 driver=NVIDIA GeForce RTX 4090 621.14 cvars=r.VSync=0|…|mass.FullyParallel=true|mass.commands.LockObserversDuringFlushing=true`로 메타데이터 값이 찍힌다(개발 실행이라 `r.ScreenPercentage=0`, 벤치에서는 러너 값).
- 측정 세션이 `run-bench.ps1`·`run-dev.ps1`에 `m3qold`·`m3syncall`·별칭을 넣었다(53c7b05). `run-m3b.ps1 -SerialArgs "-LabFullyParallel=0"`은 그대로 동작한다.
- 패키징·벤치마크는 하지 않았다(측정 세션 몫). A/B(`m3qold`·`m3syncall` vs `m3mass`, 20k P1 3회)는 묶음 1에서 잰다.
- 결정 4(4f6db5b) 뒤: `Scripts/build.ps1` 성공(24.9초). `run-dev.ps1 -Small` 종료 코드 0: `m0full`(M0), `m1mass`(M1), `m2mass`(M2), `m3mass`(M3). `m3order`(M3)는 ValidateSet에 아직 없어 같은 인자(N=100, P1, 20초)로 에디터를 직접 띄워 0. 다섯 실행 모두 `respawns=43 kills=8 snipes=39`.
- 결정 5(7f0236f) 뒤: `Scripts/build.ps1` 성공. `-Small`(M3) 종료 코드 0: `m3mass`, `m3vis0`, `m3vis15`(run-dev.ps1), `m3vis30`(ValidateSet에 없어 같은 인자로 직접 실행). `lab_actors synced` 범위: m3mass 0\~5, m3vis15 0\~5(같다), m3vis0 0\~4, m3vis30 1\~6(N=100이라 작다).

### 패키징 경합(코디네이터 보고, 2026-11-09)

- 증상: 측정 세션의 `package.ps1 -Config Test` 첫 실행이 실패했다. UBT 인스턴스 여럿(쿠킹용 에디터 타깃 + 게임 타깃)이 `Intermediate/Build/LabGenerated/LabBuildVersion.h`를 동시에 만들려다 파일 잠금이 충돌했다. 헤더가 아직 없을 때 "없으면 쓴다" 분기가 모든 프로세스에서 참이 된다. 개발 빌드는 UBT가 하나라 드러나지 않았다.
- 고침(`HordeOptLab.Build.cs` `WriteHeaderIfChanged`): 임시 파일 `LabBuildVersion.h.<guid>`에 쓰고 `File.Move(…, overwrite)`로 바꾼다. `IOException`·`UnauthorizedAccessException`이면 200ms×시도 뒤 다시 읽어 내용이 같으면 성공(다른 인스턴스가 먼저 씀), 다르면 3회까지 재시도 후 예외. 디렉터리 생성 예외도 삼킨다. UBT는 net10.0(`Engine/Source/Programs/Shared/UnrealEngine.csproj.props:32`)이라 덮어쓰기 `Move` 오버로드가 있다.
- 확인: 선행 묶음이 끝난 뒤(a0b9771) `Intermediate/Build/LabGenerated/LabBuildVersion.h`를 지우고 `Scripts/build.ps1`로 빌드해 "없을 때" 경로를 지났다. 성공, 헤더가 `a0b9771a98e7`로 다시 생겼고 임시 파일은 남지 않았다. `run-dev.ps1 -Small -Variant m3mass`가 M0 단계(별칭은 변형만 고르고 단계는 `-Extra "-LabStage=M3"`로 준다)와 M3 단계 모두 종료 코드 0, `respawns=43 kills=8 snipes=39`. 여러 UBT가 동시에 도는 Test 패키징은 측정 세션의 다음 패키징에서 확인한다.

### E3b 화질 캡처(코디네이터 지시, 2026-11-09)

plan.md "M3b" E3b (8)의 화질 쪽. 개발 빌드, `-LabStage=M3 -LabN=20000 -LabPreset=1 -LabSeed=20261106`, 1280x720 창, `-benchmark -fps=60`(고정 1/60초), `-LabShotAt=10 -LabShotBurst=3600`(10\~70초, P1 카메라 한 바퀴 = `OrbViewPeriod` 60초), `-LabDuration=71`. 두 변형 모두 `respawns=8321 kills=8424 snipes=175`로 장면이 같다. 캡처 폴더(git 밖) `Saved/Screenshots/LabBurst-e3b-m3vis0/`, `LabBurst-e3b-mass/`(각 3600장 `f%06d.png`, 5.6GB, `transition-score.py`가 읽는 형식). 로그 `Saved/Logs/LabE3bQual-m3vis0.log`, `LabE3bQual-mass.log`.

프레임별 수치는 새 개발용 인자 `-LabFrameLog`의 `lab_frame` 로그다(`LabCrowdSubsystem::FrameDiag`, `LabVatProcessor.cpp`·`LabActorSyncProcessor.cpp`가 세고 `LabGameMode::Tick`이 이전 프레임 값을 쓴다. 개발 빌드만, `#if !UE_BUILD_TEST`). 정의: "화면 안"은 `FLabSnipeView::Contains`(몸통 높이 +90cm), "30m 안"은 카메라와의 수평 거리 < 3000cm. `near_vat` = 화면 안 30m에서 아직 VAT 인스턴스로 그리는 개체(액터 준비 대기 포함), `near_actors` = 화면 안 30m의 보이는(`bPoseReady`) 액터, `hydrate` = `lab_event hydrate`의 화면 좌표가 1280x720 안인 사건. 집계 범위는 캡처 프레임 600\~4199(3600프레임, 빠진 행 0).

| 변형 | 표현 LOD 값 | near_actors 최소 / 중앙값 / 최대 | near_vat 최소 / 중앙값 / 최대 | near_vat > 0인 프레임 | 화면 안 hydrate 사건 / 그 프레임 수 |
| --- | --- | --- | --- | --- | --- |
| `m3vis0` | Base High 0, Visible 30m | 421 / 482 / 498 | 68 / 102 / 147 | 3600 / 3600 | 5962 / 2326 |
| `mass`(기준점) | Base = Visible = 30m | 175 / 211 / 240 | 316 / 368 / 415 | 3600 / 3600 | 1778 / 1022 |

- 액터 상한은 두 변형 다 `LODMaxCount[High] = 500`(`LabCrowdSubsystem.cpp` 확정값). `lab_actors`(300프레임마다) `synced`(붙은 액터 수)는 m3vis0 434\~497, mass 488\~497.
- 1초(60프레임) 구간별 `near_vat` 평균: m3vis0 82\~131, mass 323\~405. 두 변형 모두 60구간 어디서도 0이 되지 않았다.
- 대표 프레임(`Saved/Screenshots/Lab/`, git 밖): near_vat이 가장 큰 프레임을 "비어 있는 장면", near_actors가 가장 큰 프레임을 "채워진 장면"으로 골랐다.
  - m3vis0: `E3b-m3vis0-empty-f002076.png`(near_vat 147, near_actors 482), `E3b-m3vis0-filled-f004167.png`(113, 498).
  - mass: `E3b-mass-empty-f002094.png`(near_vat 415, near_actors 219), `E3b-mass-filled-f002359.png`(359, 240).
- `py -3 Scripts/transition-score.py <캡처 폴더> <로그>`(전환 튐, 문턱 6): m3vis0 사건 4754건, 튐 평균 2.85, 중앙값 1.98, 눈에 띈 전환 15/4754(0%). mass 1726건, 평균 2.80, 중앙값 2.47, 0/1726(0%). 사건 수가 표의 hydrate 수와 다른 것은 스크립트가 화면 가장자리 30px 안과 캡처 범위 끝 프레임의 사건을 빼기 때문이다.
- 빌드·실행: `Scripts/build.ps1` 성공. `run-dev.ps1 -Small -Variant m3vis0 -Extra "-LabStage=M3"` 종료 코드 0(`respawns=43 kills=8 snipes=39`). `-LabFrameLog`는 run-dev.ps1의 `-Extra`에 두 번째 값으로 주면 PowerShell `-File` 바인딩에서 빠지므로 에디터를 직접 띄워 썼다.

#### 빠른 회전(코디네이터 지시, 2026-11-09)

조건은 위 1rpm 캡처와 같고(20k, P1 위치·높이·피치, 시드 20261106, 1280x720, `-benchmark -fps=60`, 10\~70초 3600장) 카메라만 개발 전용 `-LabCamTurn`이다(`LabCameraRig.cpp`, `#if !UE_BUILD_TEST`): 5초마다 요 90도를 0.5초(30프레임)에 돌고 4.5초 멈춘다. 한 방향, 60초에 12회(k=2\~13, 회전 시작 프레임 600, 901, 1201, …, 3901. 회전마다 `lab_turn k start_frame end_frame yaw` 한 줄). 세 변형 모두 `respawns=8327 kills=8432 snipes=175`. 캡처 `Saved/Screenshots/LabBurst-e3bturn-<변형>/`, 로그 `Saved/Logs/LabE3bTurn-<변형>.log`.

정의(프레임 단위, `lab_frame`은 위와 같다):

- 회복(<100): 회전이 끝난 프레임부터 `near_vat < 100`이 처음 되는 프레임까지의 프레임 수. 다음 회전 시작 전까지 안 내려가면 "미달".
- 회복(직전): 회전이 끝난 프레임부터 `near_vat`가 회전 직전 프레임의 값 이하로 처음 돌아오기까지의 프레임 수. 못 돌아오면 "미달"(코디네이터 정의 밖, 100 아래로 내려가지 않는 변형을 위해 더했다).
- near_vat 최댓값·near_actors 최솟값: 회전 구간 [시작 프레임, 다음 회전 시작 프레임).

| 변형 | 회복(<100) 중앙값 / 최댓값 / 미달 | 회복(직전) 중앙값 / 최댓값 / 미달 | near_vat 최댓값 중앙값 / 최댓값 | near_actors 최솟값 중앙값 / 최솟값 |
| --- | --- | --- | --- | --- |
| `m3vis0` | 14 / 53 / 0/12 | 6 / 79 / 1/12 | 160 / 171 | 418 / 385 |
| `m3vis15` | 미달 12/12 | 6 / 200 / 1/12 | 250 / 275 | 324 / 310 |
| `mass` | 미달 12/12 | 20 / 115 / 3/12 | 402 / 433 | 174 / 150 |

회전별(k=2\~13 순):

- `m3vis0`: near_vat 최댓값 159 147 158 161 161 171 164 147 140 167 138 168. near_actors 최솟값 420 433 421 434 400 438 391 432 396 385 416 408. 회복(<100) 6 5 18 27 16 53 4 17 35 5 12 3. 회복(직전) 6 5 미달 25 42 6 4 6 79 4 11 2.
- `m3vis15`: near_vat 최댓값 251 256 269 270 248 275 232 245 224 240 233 265. near_actors 최솟값 328 323 326 335 313 334 323 343 310 312 330 315. 회복(<100) 전부 미달. 회복(직전) 6 200 29 6 35 6 5 15 미달 4 12 6.
- `mass`: near_vat 최댓값 403 406 407 414 402 430 382 393 368 369 377 433. near_actors 최솟값 169 170 171 186 170 175 178 188 174 179 180 150. 회복(<100) 전부 미달. 회복(직전) 115 미달 22 60 미달 20 1 13 33 2 4 미달.
- `lab_actors` `synced`: m3vis0 450\~497, m3vis15 487\~496, mass 489\~496(상한 500).

대표 프레임(`Saved/Screenshots/Lab/`, git 밖): k=7 회전이 끝난 뒤 60프레임 창에서 near_vat가 가장 큰 프레임(after)과 그 240프레임(4초) 뒤(settled). 회전 도중 프레임은 모션 블러가 있어 쓰지 않았다.

| 변형 | after | settled |
| --- | --- | --- |
| `m3vis0` | `E3b-turn-m3vis0-after-f002132.png`(near_vat 171, near_actors 442) | `E3b-turn-m3vis0-settled-f002372.png`(107, 487) |
| `m3vis15` | `E3b-turn-m3vis15-after-f002132.png`(266, 348) | `E3b-turn-m3vis15-settled-f002372.png`(211, 383) |
| `mass` | `E3b-turn-mass-after-f002131.png`(420, 186) | `E3b-turn-mass-settled-f002371.png`(369, 226) |

`py -3 Scripts/transition-score.py <캡처 폴더> <로그>`(전환 튐, 문턱 6): m3vis0 사건 7500건, 튐 평균 4.27, 중앙값 2.70, 눈에 띈 전환 157/7500(2%). m3vis15 7646건, 3.94, 2.77, 128/7646(2%). mass 3077건, 3.08, 2.69, 19/3077(1%). 1rpm 캡처(위)는 m3vis0 15/4754(0%), mass 0/1726(0%)이었다.

빌드·실행: `Scripts/build.ps1` 성공, `run-dev.ps1 -Small -Variant m3vis15 -Extra "-LabStage=M3"` 종료 코드 0(`respawns=43 kills=8 snipes=39`). `-LabCamTurn` 12초 확인 실행에서 `lab_turn` 세 줄(k=0\~2, 회전 30프레임). 측정 프리셋 P1/P2·러너·CSV는 바꾸지 않았다.

##### 5k(코디네이터 지시, 결정 1 재검토, 2026-11-09)

위 20k와 같은 절차·지표에서 `-LabN=5000`만 다르다(커밋 4f6db5b 빌드, P1, `-LabCamTurn -LabFrameLog`, 10\~70초 3600장, 회전 12회 k=2\~13, 시작 프레임 600, 901, …, 3901). 두 변형 모두 `respawns=2188 kills=2074 snipes=175`. 캡처 `Saved/Screenshots/LabBurst-e3bturn5k-<변형>/`, 로그 `Saved/Logs/Labe3bturn5k-<변형>.log`. 20k 표는 위에 그대로 둔다.

| 변형(5k) | 회복(<100) 중앙값 / 최댓값 / 미달 | 회복(직전) 중앙값 / 최댓값 / 미달 | near_vat 최댓값 중앙값 / 최댓값 | near_actors 최솟값 중앙값 / 최솟값 |
| --- | --- | --- | --- | --- |
| `m3vis0` | 0 / 0 / 0/12 | 11 / 125 / 0/12 | 40 / 50 | 99 / 90 |
| `m3vis15` | 0 / 0 / 0/12 | 5 / 32 / 0/12 | 21 / 27 | 119 / 103 |

회복(<100)이 0인 것은 두 변형 모두 near_vat가 60초 동안 한 번도 100에 닿지 않아서다(최댓값 50, 27).

회전별(k=2\~13 순):

- `m3vis0`: near_vat 최댓값 50 40 38 43 39 40 31 43 36 44 31 36. near_actors 최솟값 101 111 93 110 102 95 90 105 109 96 97 95. 회복(직전) 5 54 2 69 4 11 5 16 11 13 125 0.
- `m3vis15`: near_vat 최댓값 27 21 20 17 21 21 18 24 27 20 17 24. near_actors 최솟값 124 120 108 124 121 109 104 125 119 119 108 103. 회복(직전) 4 32 2 5 5 29 0 8 3 12 12 2.
- `lab_actors` `synced`: m3vis0 101\~138, m3vis15 150\~194(상한 500에 닿지 않음).

대표 프레임(`Saved/Screenshots/Lab/`, git 밖): 두 변형 모두 k=11 회전이 끝난 프레임 3331(after, m3vis0은 60프레임 창의 near_vat 최댓값 프레임)과 240프레임 뒤 3571(settled).

| 변형(5k) | after | settled |
| --- | --- | --- |
| `m3vis0` | `E3b-turn-5k-m3vis0-after-f003331.png`(near_vat 44, near_actors 96) | `E3b-turn-5k-m3vis0-settled-f003571.png`(24, 119) |
| `m3vis15` | `E3b-turn-5k-m3vis15-after-f003331.png`(20, 121) | `E3b-turn-5k-m3vis15-settled-f003571.png`(13, 129) |

`py -3 Scripts/transition-score.py`(문턱 6): m3vis0 사건 1803건, 튐 평균 3.77, 중앙값 2.42, 눈에 띈 전환 52/1803(3%). m3vis15 1374건, 3.45, 2.16, 44/1374(3%).

### 진단 변형 m3lock0·m3noorder(코디네이터 지시, 사용자 승인 D안, 2026-11-09)

깨끗한 상태의 `m3mass` 20k P1이 16.2ms로 M3 묶음(15.5)보다 0.7ms 느리고 `m3qold`도 느렸으므로, 공통 요소 둘을 변형으로 가른다.

- `m3lock0`(`ELabVariant::M3Lock0`): 정비 코드 그대로, 게임 모드 시작 시 `mass.commands.LockObserversDuringFlushing`을 0으로 쓴다(`LabGameMode.cpp`, `-LabFullyParallel=0`과 같은 `LabSetCVar` 경로. `DefaultEngine.ini`의 1은 그대로). 확인: `-Small` 로그 `lab_build … cvars=…|mass.commands.LockObserversDuringFlushing=false`(다른 변형은 true). CSV 메타데이터 `LabCVars`는 같은 함수의 문자열이다.
- `m3noorder`(`ELabVariant::M3NoOrder`): 정비 쿼리(표현 프래그먼트, 액터 태그 필터)는 그대로 두고 VatISM·ActorSync의 `ExecuteAfter(LabLogic)`만 뺀다(`LabMassProcessors.h` `LabSkipLogicOrder()`, 그룹 선언은 유지). 확인: `-LogCmds="LogMass Log"` 실행의 처리 그래프에서 VatISM·ActorSync가 저격·상태 프로세서 앞에도 놓인다(프래그먼트 접근으로만 순서가 정해진다).
- 빌드 성공, `run-dev.ps1 -Small -Extra "-LabStage=M3"` 두 변형 모두 종료 코드 0(`respawns=43 kills=8 snipes=39`).
- 결과(측정 세션 6d040cd, 위 "코드 A/B(D안)"): `m3noorder`가 `m3mass`보다 0.4\~0.7ms 빠르고 `m3lock0`은 차이 없음. 결정 4(4f6db5b)로 `m3noorder` 동작이 기본값이 되었고 변형 이름은 `m3order`(선언 있음)로 바뀌었다. `m3lock0`은 그대로 있다.

#### 측정 세션에 요청

- `run-bench.ps1`·`run-dev.ps1`의 `-Variant` ValidateSet에 `m3lock0`, `m3noorder`를 더해 달라(이번 `-Small` 확인은 현재 스크립트로 통과했다). → 4f6db5b부터 `m3noorder`는 없고 `m3order`다. ValidateSet의 `m3noorder`를 `m3order`로 바꿔 달라.
- ValidateSet에 `m3vis30`도 더해 달라(7f0236f). E3b 24회의 "기준점"(30/30m) 자리는 `m3vis30`이고, `m3vis15`는 이제 `m3mass`와 같은 설정이다.

### 2종(코드)(코디네이터 지시, STATUS 4, 2026-11-09)

plan.md "M3b" 146\~158행(2종 구성, 저격 E안, E3 (3c) 화면 크기 중요도)의 코드 부분. 커밋 ee1a17d. 에셋(시즈 미니언 이전, AnimBP, 정적 메시, VAT 베이크, 피직스)은 에셋 세션 몫이고, 그 전까지 2종 모드는 소형 에셋으로 대체해 돈다(스모크 전용).

**인자.** `-LabTypes=2`(기본 1, `FLabConfig::Types`). 2면 일련번호가 10의 배수인 개체가 대형이다(초기 배치와 재생성 모두, `FLabUnitTypes::IsLarge`). `lab_config … types=2`, HUD 첫 줄에 `2 types`. CSV 메타데이터 `LabTypes`(1·2), `LabTypesAssets`(`ok`·`substituted`·`-`). 1종은 코드 경로가 그대로라(아래 "확인") 결과가 바뀌지 않는다.

**종류.** `FLabUnitTypeParams`(const shared fragment)에 `UnitType`을 더했다. 소형 `{0, 300, 100, 40}`, 대형 `{1, 300, 100, LargeBoundRadius}`로 값이 달라 공유 프래그먼트가 둘이 되고, 종류별 `UMassEntityConfigAsset`(`LabMinionConfig`, `LabMinionLargeConfig`, `ULabMinionTrait::UnitType`)에서 템플릿이 따로 나와 아키타입이 둘로 갈린다(`ULabCrowdSubsystem::BuildEntityConfig`). 프로세서는 `AddConstSharedRequirement<FLabUnitTypeParams>`로 청크마다 종류를 읽는다(개체별 비용 없음). `SpawnEntities`는 일련번호로 나눠 각 템플릿으로 만든다(순서 유지. 1종은 전부 [0]이라 전과 같은 한 번의 생성). `FAgentRadiusFragment`는 둘 다 40으로 두었다(회피가 꺼져 있어 쓰이지 않는다). 피격·사망 시간은 소형 애니 길이 하나다(행동은 같다).

**표현.** 종류별 ISM(각 Config의 `StaticMeshInstanceDesc`가 다른 메시 → 엔진이 ISM 컴포넌트를 따로 만든다), 종류별 VAT 클립(`ULabCrowdSubsystem::VatClipsByType[2]`, VatISM·ActorSync가 청크의 종류로 고른다), 종류별 액터 클래스 `ALabMinionMassActorLarge`·`ALabMinionMassActorBudgetLarge`(엔진 액터 풀이 클래스 키라 풀도 갈린다, `MassActorSpawnerSubsystem.h:321`). M0 액터도 일련번호로 대형 에셋을 받는다(`ALabCrowdSpawner::SpawnAt`, `ALabMinion::UnitType`). 에셋 경로는 `Config/DefaultGame.ini` `[LabUnitAssets]`에서 읽는다(`LabUnitAssets.h/.cpp`, 키 접두사 Small·Large. Small 키가 없으면 2종 전 코드의 경로). 대형 세트가 하나라도 없으면 `FLabUnitTypes::bLargeSubstituted = true`: 소형 에셋으로 대체하고 `lab_types_warning … smoke_only=1 do_not_measure=1` 경고, 메타데이터 `LabTypesAssets=substituted`. 종류(아키타입·저격 예약·풀)는 대체 중에도 갈린다.

필요한 에셋과 기대 경로(에셋 세션이 만들고 ini를 맞춘다. 이름은 소형 세트를 따라 지은 제안이라 바꿔도 된다. ini만 맞으면 코드는 그대로다):

| 에셋 | 기대 경로(ini 키) | 비고 |
| --- | --- | --- |
| 스켈레탈 메시(시즈) | `/Game/ParagonMinions/Characters/Minions/Down_Minions/Meshes/Minion_Lane_Siege_Dawn`(`LargeSkeletalMesh`) | `Scripts/migrate-assets.py` `siege` 세트. 피직스 에셋은 메시에 지정된 것을 쓴다(소형과 같은 수정, 코드가 따로 읽지 않음) |
| AnimBP | `/Game/Lab/Anim/ABP_LabMinionLarge`(`LargeAnimClass`, `…_C`) | 스켈레톤이 달라 따로 만든다. 부모 `ULabMinionAnimInstance`, Start Position 바인딩(`Scripts/bind-anim-start.md`). 그래프가 비어 있으면 없는 것으로 본다 |
| 애니 셋 걷기·피격·죽기 | `…/Animations/Siege/NonCombat_JogFwd`, `HitReact_Front`, `Death_A`(`LargeWalk`·`LargeHit`·`LargeDeath`) | M0 액터와 AnimBP 시퀀스. 실제 시즈 애니 이름이 다르면 ini를 바꾼다 |
| 정적 메시(M0 static·M1 ISM) | `/Game/Lab/Derived/SM_LabMinionLarge`(`LargeStaticMesh`) | `Scripts/make-static-minion.py` |
| VAT 메시·데이터(Bone) | `/Game/Lab/VAT/Baked/SM_LabMinionLargeVAT_Bone`, `DA_LabMinionLargeVAT_Bone`(`LargeVatMesh`·`LargeVatData`, 접미사 없이) | `Scripts/bake-vat.py`. 걷기·피격·죽기 세 클립. 텍스처 2장(위치·회전)은 데이터 에셋이 가리킨다 |
| 액터 머티리얼 | `/Game/Lab/VAT/Baked/M_LabMinionLargeActor`(`LargeActorMaterial`) | VAT 인스턴스와 같은 색. 비우면 메시 기본 머티리얼 |
| 바운드 반경 | `LargeBoundRadius=60`(cm) | **확정 전** 임시값. 메시 바운드로 잰 뒤 바꾸고 measurement.md 확정값에 적는다 |

**저격 E안**(`LabSnipe.h` `TLabSnipeSelector`, M0 `ALabOrb::Snipe`와 Mass `ULabOrbSnipeProcessor` 둘 다). 틱이 4의 배수면 200m 밖 구간 1발, 나머지 2면 80\~200m 구간 1발이 대형 전용(그 구간 화면 안 대형 중 해시 최소). 대형이 있으면 그 구간 소형 후보 중 해시가 가장 큰 하나를 뺀다. 예약 구간에 대형이 없으면 소형 후보가 그대로라 전과 같다. 1종은 대형이 없어 늘 이 경로다.

**E3 (3c) `m3sig`**(`ELabVariant::M3Sig`). Budget 액터(`ALabMinionMassActorBudget`, `a.Budget.*` CVar는 m3budget*과 같다)에 ActorSync가 매 프레임 중요도 = `BoundRadius ÷ 카메라 거리`를 넘긴다(`LabActorSyncProcessor.cpp`, 할당기는 상대값만 쓴다 `AnimationBudgetAllocator.cpp:327-342`). 새 스탯: 프레임별 `Lab/LargeActors`, `Lab/LargeActorSkips`(대형 액터 중 이번 프레임 애니 틱을 건너뛴 수. `IsUsingExternalTickRateControl() && !ShouldTickAnimation()`, `SkeletalMeshComponent.cpp:1918-1923`), 측정 구간 합 메타데이터 `LabLargeActorFrames`·`LabLargeActorSkips`(비율 = Skips ÷ Frames). 할당기 갱신 전후 어느 값인지는 틱 순서에 달려 한 프레임 어긋날 수 있다. 1종이나 예산 변형이 아니면 0이다.

**바꾼 파일.** 새 `LabUnitAssets.h/.cpp`. `LabConfig.h/.cpp`(Types, M3Sig), `LabMassTypes.h`, `LabMassTraits.h/.cpp`, `LabCrowdSubsystem.h/.cpp`, `LabCrowdSpawner.h/.cpp`, `LabMinion.h/.cpp`, `LabSnipe.h`, `LabOrb.cpp`, `LabMassProcessors.cpp`(Snipe), `LabVatProcessor.cpp`, `LabActorSyncProcessor.cpp`, `LabGameMode.h/.cpp`(메타데이터, m3sig CVar), `LabHud.cpp`, `Config/DefaultGame.ini`.

**확인.** `Scripts/build.ps1` 성공. `-Small`(N=100, P1, 20초) 종료 코드 0:
- 1종: `m0full`(M0) `m1mass`(M1) `m2mass`(M2) `m3mass`(M3) 모두 `respawns=43 kills=8 snipes=39`(2종 코드 전과 같다. 저격 대상 선택이 같은 경로를 지난다).
- 2종(`-LabTypes=2`, 대형 에셋 없음 → 대체): `m0full`(M0), `m3mass`(M3) 종료 코드 0, `lab_types_warning … large_assets_missing=mesh,walk,hit,death,animbp,staticmesh,vatmesh,vatdata substituting=small smoke_only=1`, `lab_types types=2 large_assets=substituted large_radius=60 small_radius=40`. M3는 `lab_mass_config type=0 … actor=LabMinionMassActor`와 `type=1 … actor=LabMinionMassActorLarge` 두 줄(템플릿·아키타입 둘). `respawns=43 kills=8 snipes=39`.
- `m3sig`(M3) 1종·2종 종료 코드 0, 액터 `LabMinionMassActorBudget`·`LabMinionMassActorBudgetLarge`. `run-dev.ps1`의 ValidateSet에 `m3sig`가 없고 `-Extra` 두 값은 PowerShell이 떨어뜨리므로 같은 인자로 에디터를 직접 띄웠다(`-LabTypes=2` 실행도 같다).
- 패키징·벤치마크는 하지 않았다(측정 세션 몫).

**측정 세션에 요청.** ValidateSet에 `m3sig`를 더해 달라. 2종 실행은 `-Extra "-LabTypes=2"`(러너에 인자를 두려면 `-Types 2` 같은 스위치). `LabTypesAssets=substituted`인 CSV는 비교에 쓰지 않는다.

**에셋 세션에 요청.** 위 표의 에셋을 만들고 `Config/DefaultGame.ini` `[LabUnitAssets]`의 Large 키를 실제 경로로 맞춘 뒤, `-LabTypes=2` 실행 로그에 `lab_types types=2 large_assets=ok`가 찍히는지 본다. 반경을 재면 `LargeBoundRadius`를 바꾼다.

### 2종(에셋)(에셋 세션, STATUS 4b, 2026-11-09)

"2종(코드)" 절의 에셋 표를 채운다. 1단계(에디터 없이)는 시즈 에셋 이전, 2단계(에디터 + MCP)는 AnimBP·피직스·정적 메시·VAT·액터 머티리얼·ini·반경·바닥 띠다.

**1단계: 시즈 이전.** `Scripts/migrate-assets.py`에 `siege` 세트를 더했다(메시 `Minion_Lane_Siege_Dawn`, 피직스 `Minion_Lane_Siege_Physics`, 애니 셋). 실제 애니 이름은 AssetHost `.../Down_Minions/Animations/Siege/`에서 확인했다.

| 용도 | 시즈 애니(실제 이름) | 근접(소형) | ini 키 |
| --- | --- | --- | --- |
| 걷기 | `Animations/Siege/Jog_Fwd_NonCombat` | `Animations/Melee/NonCombat_JogFwd` | `LargeWalk`(표의 기대값 `NonCombat_JogFwd`와 다르다. 2단계에서 ini를 이 이름으로 맞춘다) |
| 피격 | `Animations/Siege/HitReact_Front` | `HitReact_Front` | `LargeHit` |
| 죽기 | `Animations/Siege/Death_A` | `Death_A` | `LargeDeath` |

- 실행: commands.md "에셋 마이그레이션"의 명령에 `LAB_MIGRATE_SET=siege`. 건식(`LAB_MIGRATE_DRY=1`) 의존성 36개, 원본 기준 149.1MB. 그중 25개(근접 세트가 이미 옮긴 `Characters/Global/` 머티리얼 함수·레이어·텍스처, `M_Dawn_Minion_MASTER`, Super 텍스처 3장)는 대상에 있어 건너뛰었다(엔진 기본 `EAssetMigrationConflict::Skip`, `IAssetTools.h` `FMigrationOptions`. 스크립트가 값을 로그에 찍는다). 이전 전후 `Content/ParagonMinions/` 기존 파일의 md5가 모두 같았다.
- 새로 온 11개(원본 기준 56.5MB): 애니 3(`Death_A`, `HitReact_Front`, `Jog_Fwd_NonCombat`), 메시 `Minion_Lane_Siege_Dawn`, 피직스 `Minion_Lane_Siege_Physics`, 스켈레톤 `Minion_Lane_Siege_Skeleton`, 그림자 원기둥 `Minion_Lane_Siege_ShadowCyl`(메시 의존성), 머티리얼 `Materials/Siege/M_Dawn_Minion_Siege`, 텍스처 `Textures/Siege/T_Minion_Dawn_Seige_D`·`_MASK01`·`_N`. 옮긴 뒤 `Content/ParagonMinions/` 244 → 292MB. gitignore 대상이라 커밋하지 않았다(`.gitignore:26`).
- 막힌 것과 대처: 첫 건식 실행이 Python 출력 없이 종료 코드 1로 끝났다. AssetHost 로그에 "The game module 'AssetHost' could not be found". AssetHost의 빈 C++ 모듈(`Source/AssetHost`)이 11-07에 소스 빌드로 컴파일되어(`Binaries/Win64/UnrealEditor.modules` BuildId `4f0f03df…`) 런처 엔진(BuildId `55116800`)이 건너뛰었다. 런처 엔진 `Engine/Build/BatchFiles/Build.bat AssetHostEditor Win64 Development -project=…AssetHost.uproject`로 모듈만 다시 컴파일해(18초, AssetHost의 `Binaries/`만 바뀜) 해결했다. commands.md에 적힌 11-06의 `orbfx` 종료 코드 1도 같은 증상이다. troubleshooting.md에 한 줄 더했다. AssetHost `.uproject`의 `EngineAssociation`은 지금 레지스트리에 없는 GUID `{31E80386…}`이라 탐색기에서 열면 엔진을 묻는다(손대지 않음).

**2단계: 에디터 작업(2026-11-09, 에디터는 사용자가 소스 빌드로 켬).** 이 세션의 MCP 클라이언트는 세션 시작 때(에디터 꺼짐) 붙지 못한 채로 남아, 에디터의 MCP 서버(127.0.0.1:8001)를 HTTP JSON-RPC로 직접 불렀다(commands.md "Unreal MCP를 HTTP로 직접 부르기"). MCP의 `ProgrammaticToolset.execute_tool_script`는 `unreal` 모듈을 들여오지 못하고(허용 모듈 `json`·`math`·`re`·`time`·`datetime`·`copy`) 콘솔 명령 툴도 없어, Python이 필요한 일은 소스 빌드 커맨드렛으로 돌렸다(에디터가 떠 있는 채로. 새 패키지만 쓴다).

| 표의 에셋 | 만든 것 | 방법 |
| --- | --- | --- |
| (a) AnimBP | `/Game/Lab/Anim/ABP_LabMinionLarge`(커밋, LFS). 스켈레톤 `Minion_Lane_Siege_Skeleton`, 부모 `LabMinionAnimInstance`, 플레이어 `_0`·`_3` `Jog_Fwd_NonCombat`, `_1` `HitReact_Front`, `_2` `Death_A` | `ABP_LabMinion` 복제 + 스켈레톤(`create-anim-bp.py` `LAB_UNIT=large`) → 클립 교체·컴파일·저장(MCP). Start Position 연결과 `ResetChildOnActivate`는 복제로 그대로 왔다(`get_node_infos`로 확인). 순서 `Scripts/create-anim-bp-large.md` |
| (b) 피직스 | 수정 없음 | 소형도 에셋을 고치지 않았다. 래그돌 방지는 코드가 메시 콜리전을 끄는 것이고(`LabMinion.cpp:30-34`, `ALabMinion` 생성자), `ALabMinionMassActorLarge`·`…BudgetLarge`도 그 클래스를 상속한다. 메시에는 `Minion_Lane_Siege_Physics`가 지정돼 있다 |
| (c) 정적 메시 | `/Game/Lab/Derived/SM_LabMinionLarge`(gitignore) | `make-static-minion.py` `LAB_UNIT=large`. 바운드 (−78.2, −41.7, −0.1)..(77.7, 169.3, 196.0). 바탕 머티리얼 `M_Dawn_Minion_MASTER`의 ISM 사용 플래그는 이미 켜져 있었다 |
| (d) VAT | `/Game/Lab/VAT/Baked/SM_LabMinionLargeVAT_Bone`·`DA_LabMinionLargeVAT_Bone`(+ `_Vertex` 한 벌, 텍스처·MI, gitignore) | `bake-vat.py` `LAB_UNIT=large`. 86프레임(걷기 0..40, 피격 41..65, 죽기 66..85, 30fps), Bone 텍스처 위치·회전 39x87, 가중치 2649x4. Bone 메시 바운드는 Vertex 굽기의 전 프레임 정점 범위 (−140.8, −151.1, −108.0)..(213.3, 206.1, 204.1) |
| (e) 액터 머티리얼 | `/Game/Lab/VAT/Baked/M_LabMinionLargeActor`(gitignore) | `bake-vat.py`의 `make_actor_material`(VAT 머티리얼과 같은 기본 색·노멀·거칠기·죽기 진홍색, 텍스처는 `Textures/Siege/T_Minion_Dawn_Seige_D`·`_N`, 팩의 철자 그대로) |
| (f) ini | `LargeWalk`를 `Animations/Siege/Jog_Fwd_NonCombat`로, `LargeBoundRadius`를 47로 바꿨다. 나머지 Large 키는 이미 맞았다 | `Config/DefaultGame.ini` `[LabUnitAssets]` |
| (g) 반경 | `LargeBoundRadius=47` | 아래 |
| (h) 바닥 띠 | `/Game/Lab/Materials/M_LabBandFloor`(커밋, LFS) | `Scripts/create-band-floor.py`(커맨드렛), 순서·파라미터 `Scripts/create-band-floor.md` |

스크립트 셋(`make-static-minion.py`, `bake-vat.py`, `create-anim-bp.py`)은 `LAB_UNIT=large` 환경 변수로 대형을 만든다. 변수가 없으면 경로와 이름이 전과 같다(소형 에셋은 다시 만들지 않았다).

**대형 반경(g).** 소형 40은 측정값이 아니라 M1 트레이트의 값이고(`LabUnitAssets.cpp:43`), 중요도는 상대값만 쓰인다(`m3sig`, `AnimationBudgetAllocator.cpp:327-342`). 그래서 소형 40을 기준으로 두 메시 바운드의 비를 곱했다. 바운드는 Unreal MCP `SkeletalMeshTools.get_bounds`(레퍼런스 포즈, 메시 로컬)로 쟀다.

| 메시 | 박스 반폭 x·y·z(cm) | 구 반지름(cm) |
| --- | --- | --- |
| `Minion_Lane_Melee_Dawn`(소형) | 64.9 · 87.1 · 87.9 | 139.74 |
| `Minion_Lane_Siege_Dawn`(대형) | 78.0 · 105.5 · 98.1 | 163.81 |

대형 = 40 × 163.81 ÷ 139.74 = 40 × 1.172 = 46.9 → **47**(정수로 반올림, 소형과 같은 자리수). 박스 반폭 비는 1.20 · 1.21 · 1.12라 어느 축으로 재도 1.1\~1.2배다. 구 반지름을 그대로(소형 140 / 대형 164) 쓰면 비는 같고 절대값만 바뀐다. 소형 값을 바꾸는 일이라 하지 않았다. measurement.md 확정값 반영은 코디네이터 몫이다.

**바닥 띠 파라미터(코드 세션에 요청).** `-LabBandFloor`일 때 Floor(`L_Bench` `Floor_0`의 `StaticMeshComponent0`, 지금 머티리얼 `MI_ProcGrid`)에 `M_LabBandFloor`의 MID를 넣고 Vector 파라미터 **`BandCenter`**에 오브 월드 위치(cm, XY만 씀)를 넣는다. 반경은 Scalar **`BandRadius1`·`BandRadius2`·`BandRadius3`**(기본 3000·8000·20000cm = `FLabSnipeBands`)라 넣지 않아도 되고, 넣는다면 같은 상수에서 넣는다. 색은 `ColorNear`·`ColorMid`·`ColorFar`·`ColorOff`·`LineColor`, 선 폭 `LineWidth`(40), 격자 `GridSize`(1000), `BaseGray`(0.35). 확인 캡처(에디터에서 Floor에 잠시 넣고 찍은 뒤 되돌림): `Saved/Screenshots/Lab/m3b_bandfloor_top.png`(탑뷰 260m, 30 : 80 : 200 반지름 비 일치), `m3b_bandfloor_p1.png`(오브 뒤 15m 낮은 시점).

**에디터 상태 메모.** 확인 때 `L_Bench`의 Floor 머티리얼을 바꿨다가 되돌렸으므로 레벨이 수정됨 표시일 수 있다. 저장해도 내용은 같고, 저장하지 않아도 된다. `HordeOptLab.uproject`의 `EngineAssociation`이 `{31E80386…}`로 바뀌고 끝 개행이 빠져 있다(troubleshooting.md의 그 줄. 에디터를 켜면서 바뀐 것으로 보인다. 이 세션은 고치지 않았다. 에디터를 닫은 뒤 되돌려야 한다).

**확인(에디터를 닫은 뒤, 코드 세션 빌드 6ad7f7e).** `Scripts/run-dev.ps1 -Small -Extra "-LabTypes=2"`(M0 `full`): `lab_types types=2 large_assets=ok large_radius=47 small_radius=40`, `respawns=43 kills=8 snipes=39`, 종료 코드 0. `Scripts/run-dev.ps1 -N 5000 -Preset P1 -Duration 13 -ShotAt 10 -Variant m3mass -Extra "-LabStage=M3 -LabTypes=2"`(두 인자를 한 문자열로 넘기면 PowerShell이 떨어뜨리지 않는다): `large_assets=ok`, `lab_mass_config type=1 … mesh=SM_LabMinionLargeVAT_Bone actor=LabMinionMassActorLarge`, 종료 코드 0. 스크린샷 `Saved/Screenshots/Lab/m3b_types2_m3mass_n5000_p1.png`(2560x1440, 10초, git 밖). 대형은 칼이 없고 몸통이 굵은 시즈 미니언으로 가까운 액터와 먼 VAT 양쪽에 보인다. 크기 차이가 1.17배라 멀리서는 실루엣으로만 구별된다.

### 바닥 띠(코드)(코디네이터 지시, 결정 6, 2026-11-09)

커밋 6ad7f7e. 사양은 위 "2종(에셋)"의 "바닥 띠 파라미터(코드 세션에 요청)"와 backlog.md 결정 6. 개발 전용(`#if !UE_BUILD_TEST`, 기본 꺼짐)이라 Test 패키지에는 코드가 없다.

- 인자 `-LabBandFloor`(`ALabGameMode::StartPlay`·`SetupBandFloor`·`Tick`, `LabGameMode.h/.cpp`). P1이면 CVar `lab.BandFloor`를 1로 두고(새 CVar. `LabFixedCVarString`의 목록에 더해 `lab_build` 로그와 CSV 메타데이터 `LabCVars`에 `lab.BandFloor=1`로 찍힌다. 켜고 잰 실행을 걸러내는 용도), `L_Bench`의 바닥(`Floor*` 이름의 `AStaticMeshActor`. 라벨은 에디터 전용이라 이름 접두사로 찾는다)의 머티리얼 0을 `M_LabBandFloor`의 MID로 바꾼다. `BandRadius1~3`은 `FLabSnipeBands::Edges`(3000/8000/20000cm)에서, `BandCenter`는 오브 월드 위치에서 매 프레임 넣는다. P2(탑뷰)면 `lab_bandfloor on=0 reason=P2`로 끈다. 머티리얼 경로는 `Config/DefaultGame.ini` `[Lab] BandFloorMaterial`(코드에 박지 않음). ini 키·머티리얼·바닥이 없으면 경고만 남기고 기본 바닥으로 돈다.
- 확인: `Scripts/build.ps1` 성공. `-Small`(M3, N=100, 20초) 종료 코드 0: `m3mass` P1(기본 바닥, `respawns=43 kills=8 snipes=39`), `m3mass` P1 `-LabBandFloor`(`lab_bandfloor on=1 floor=Floor_0 previous=MI_ProcGrid material=M_LabBandFloor radii=3000/8000/20000`, `cvars=…|lab.BandFloor=1`, 같은 43/8/39), `m3mass` P2 `-LabBandFloor`(`lab_bandfloor on=0 reason=P2`, `lab.BandFloor=0`). `run-dev.ps1`은 `-Extra` 두 값을 떨어뜨리므로 뒤 둘은 같은 인자로 에디터를 직접 띄웠다.
- 캡처는 하지 않았다(지시 범위 밖). 띠가 보이는 확인 캡처는 에셋 세션의 `m3b_bandfloor_top.png`·`m3b_bandfloor_p1.png`(위 절).
