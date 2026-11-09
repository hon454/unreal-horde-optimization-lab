# M3b 작업 기록

## 측정 방법 구현 (측정 세션, 2026-11-09)

plan.md "M3b" 측정 방법 1\~7번을 `Scripts/`에 구현했다. 빌드·패키징·벤치마크 실행은 하지 않았다.

### 만든 것

- `summarize.py`: `frame_p95_ms`(runs·summary, `_min`/`_max` 포함), `bottleneck`(프레임·Game·Render·GPU p50 중 최댓값 이름), `build`(CSV 메타데이터 `labbuild`·`labcvars`·`labdriver`를 `키=값;`으로 이음, 없으면 빈칸). `--rebuild`는 기존 행도 원본 CSV로 다시 계산한다. 태그 `aa*` 라벨은 summary에서 `<변형>-aa` 조건으로 따로, `trace*`는 summary에서 뺀다.
- `plot.py`: `n-at-60fps.svg`는 프레임 p95 기준 N, 막대 옆에 p50 N과 병목. `n-scaling-P1.svg`는 p95 실선 + p50 점선과 16.7ms 선. `Docs/figures/n-at-60fps.csv`에 값(p95 N, p50 N, 병목). 30k 점은 summary에 있으면 자연히 들어간다. `-aa` 조건은 곡선에 잇지 않는다.
- `spikes.py`: 사용법은 commands.md. 사건 = 중앙값 2배 초과 프레임(붙은 것은 하나로 묶음), 시각 = 프레임 시간 누적합. 주기 판정 = 사건 5개 이상이고 간격이 간격 중앙값의 ±10% 안인 비율 60% 이상. 임계값(5개, 60%)은 측정 세션이 정했다.
- `run-bench.ps1`: `-RunIndex`, `-Trace`(`Saved/Traces/<라벨>.utrace`), `-ExtraArgs`, 변형 이름 `m3vis0 m3vis15 m3hys20 m0full m0static staticpawn m1mass m2mass m3mass`를 ValidateSet에 추가. 게임이 이 이름을 모르면 실행이 실패한다(코드 세션 몫).
- `run-m3b.ps1`(초안): 위상 e3b(18회 섞기) → matrix(M0 24 + staticpawn 6 + M1\~M3 90) → aa 3 → trace 2. 143회 + 버리는 r0 1 = 144회, 약 204분(85초/회). `-DryRun`으로 목록 확인(동작 확인함). 계획서의 "약 170회"는 budget·uro·pawn 같은 P1 전용 변형까지 센 값으로 보이고 이 러너에는 넣지 않았다.
- `Docs/Reference/pc-specs.md`: CPU·RAM·GPU·드라이버·OS·엔진 커밋을 채웠다. commands.md에 행을 더했다.

### M0\~M3 재요약

`summarize.py --milestone M<N> --rebuild`를 M0\~M3에 돌렸다. 재요약 전후 `runs.csv`·`summary.csv`의 기존 열을 모든 행에서 비교했다(행 수 89/63/71/118, 29/20/23/38 조건): **차이 0, 행 수 같음.** 새 열만 더해졌다(`frame_p95_ms`, summary는 `_min`/`_max`도, `bottleneck`, `build`). `build`는 M0\~M3 CSV에 `lab*` 메타데이터가 없어 비었다. `bottleneck`은 모든 행이 `frame`이다(프레임 시간이 스레드 시간 이상이라 항상 최댓값. 질문 1).

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

### 질문 (코디네이터/코드 세션)

1. `bottleneck`: 규약 문장대로 4개 중 최댓값으로 구현했더니 항상 `frame`이다. 정보가 있는 쪽은 프레임을 뺀 Game/Render/GPU 중 최댓값이다. 그렇게 바꾸는 편이 낫다고 본다(`BOTTLENECK_COLS`에서 한 줄 삭제 후 `--rebuild`). measurement.md는 고치지 않았다.
2. E3b 18회에 기준점 `mass`가 없다. 같은 묶음 기준이 필요하면 `run-m3b.ps1 -IncludeBase`로 6회가 늘어난다(24회). 기본은 계획서대로 18회.
3. 변형 이름: 라벨이 `M3b-<변형>-N..`라 단계마다 변형 이름이 달라야 한다(`m0full m0static staticpawn m1mass m2mass m3mass`). 게임이 이 이름과 `-LabStage`(`-Stage`로 전달) 매핑을 받아야 한다.
4. 직렬 트레이스: `mass.FullyParallel=0`을 거는 게임 인자가 아직 없다. 자리 표시자로 `-LabFullyParallel=0`을 `run-m3b.ps1 -SerialArgs`에 두었다. 현재 코드에서 이 CVar를 0으로 쓰는 것은 `massst` 변형뿐이다(`LabGameMode.cpp:64`).
5. CSV 메타데이터 키: `LabBuild`·`LabCVars`·`LabDriver`를 가정했다(소문자로 읽음). 코드 세션이 키를 정하면 `summarize.py`의 `BUILD_KEYS`를 맞춘다.
