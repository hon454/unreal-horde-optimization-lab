# 스크립트

사양과 인자는 [Docs/Guides/commands.md](../Docs/Guides/commands.md). PowerShell은 UTF-8 BOM으로 저장한다(Windows PowerShell 5.1이 BOM 없는 한글을 잘못 읽는다). Python은 `py -3`(3.9)이고 표준 라이브러리만 쓴다. 에디터 Python 스크립트는 커맨드렛으로 돌린다(commands.md "파생 에셋과 맵").

## 매번 쓰는 것

| 파일 | 역할 |
| --- | --- |
| `common.ps1` | 엔진 경로(`EngineAssociation` → 레지스트리), 공통 검사 |
| `build.ps1` | Development 에디터 빌드 |
| `run-dev.ps1`, `run-dev-stages.ps1` | Development `-game` 실행(개발·오버레이·스크린샷). `-Stage 1`\~`8`이 사다리 단계다. `run-dev-stages.ps1`은 단계마다 작은 실행(`-Ablations`면 빼 보기도) |
| `package.ps1` | Test 패키지 |
| `run-full.ps1`, `full-conditions.json`, `lab_conditions.py` | 총괄 묶음 러너와 조건 표(v2 W12, 리뷰 반영. 구성 27개: 단계 `s1`\~`s8`, 빼 보기 17개, 하위 실험 `s8av`와 `s8iso`. 묶음 종류별 구성·속도·N·회차·프리셋). 라벨은 `<묶음>-<V0\|V1>-<구성>-N<N>-P<1\|2>-r<회차>`(P2는 `topview`만). 러너는 `-DryRun`, `-Pick`, `-NFilter`, `-Resume`, `-AllowDirty`(시험 전용), 실행마다 CPU 클럭 기록(`clock.csv`)을 갖는다. 패키지 커밋이 dirty면 시작하지 않는다. 느린 실행을 새 회차 7\~9로 한 번 다시 재고, 끝에 `summarize.py`, `spikes.py`, `equivalence-check.py`, `batch-check.py`를 돌려 동등성이나 묶음 판정이 실패하면 종료 코드 1이다. `lab_conditions.py`는 조건 표를 읽는 공용 코드다 |
| `ablation-check.py`, `equivalence-check.py` | 실행마다 CSV 메타데이터를 조건 표와 대조한다(`--selftest`, `--labels`도 있다). 단계 사이의 동등성(초당 사망, 30m 안 개체 수 ±1%, 겹친 쌍(분리 프로세서는 엄격 단계 중앙값의 0.5배\~2배), 캡슐 트레이스, 왕복 전환)을 검사한다 |
| `batch-check.py`, `batch_rules.py` | 묶음 끝 판정(리뷰 M4): 느린 실행 표시와 `slow.csv`, A/A 3회와 단계 8의 차이 0.5ms, 단계 8의 모든 실행 p95 16.7ms 이하. 실패하면 종료 코드 1. `--dir`, `--out`으로 스크래치에서 돌린다. `batch_rules.py`는 느린 실행 규칙을 `summarize.py`와 나눠 쓰는 공용 코드다 |
| `run-spike.ps1`, `spike-check.py`, `spike-timeline.py` | v2 작업 단위의 작은 묶음(`Spike1`부터 `V2W<n>`까지)의 러너, 메타데이터 대조, 구간별 정리. 구성 표(`$StepExpect`, `$Ablations`)를 러너 안에 갖고 있어 총괄 묶음의 조건 표(`full-conditions.json`)와 따로 맞춰야 한다(W12 때 같은 값이다). 새 총괄 묶음은 `run-full.ps1`을 쓴다(`Measurements/<묶음>/timeline.csv`). v2 W10부터 구성은 사다리 단계 `s1`\~`s8`과 빼 보기 `a8*`다(commands.md "사다리 단계와 스위치(v2 W10)"). 경위는 [Docs/Worklog/V2.md](../Docs/Worklog/V2.md) |
| `run-flash.ps1`, `flash-scan.py` | v2 W9 ISKM 번쩍임 진단의 연속 캡처 러너와 튄 프레임 지표(개발 빌드) |
| `capture-full.ps1` | 단계 1\~8의 화면 촬영(움직이는 WebP, HUD 켬, 이동 조건). 조건 표의 `capture` 절을 읽는다. 측정이 끝난 뒤 따로 돈다 |
| `summarize.py` | 프레임 CSV → `Measurements/<묶음>/runs.csv`, `summary.csv`. v2 W12부터 `clock.csv`의 CPU 클럭 열(`cpu_perf_mean` 등)을 붙이고, 총괄 묶음에는 `slow_mark`(runs.csv)와 `runs_used`, `slow_note`(summary.csv)를 붙인다. 느린 실행을 다시 잰 실행이 대신한다 |
| `spikes.py` | 묶음의 주기 스파이크 검사 |
| `plot.py` | `summary.csv` → `Docs/figures/`. `--batch Full4`는 사다리, 빼 보기(이동·정지), 최대 N 그림을 그린다. 최대 N 표(`full-n-at-60fps.csv`와 출력)는 각 점의 겹친 쌍(`overlap_points`)을 함께 적는다. 탑뷰(P2) 한 점은 그리지 않는다(`--out <폴더>`로 확인용 출력) |
| `plot-code-tour.py` | 코드 투어의 한 프레임 흐름 그림(`Docs/figures/code-tour/frame-flow.svg`). 수치 없이 프로세서 선언 순서를 그린다 |
| `check-tilde.py` | 추적되는 Markdown에서 `\~`로 쓰지 않은 `~`를 찾는다. 코드 스팬, 코드 블록, 주석, 링크 대상은 건너뛴다. 찾으면 종료 코드 1 |

## `v1` 전용(v2의 단계 이름을 쓰지 않아 지금 빌드에서 돌지 않는 것)

지우지 않고 남긴다. `v1` 묶음의 수치를 다시 만들거나 경위를 볼 때 태그 `v1`의 소스와 함께 쓴다.

| 파일 | 역할 |
| --- | --- |
| `run-bench.ps1` | v1 Test 패키지 한 조건 러너(`-LabVariant`를 넘긴다). `run-q4.ps1`, `run-budget.ps1`, `run-mgr.ps1`, `run-pool.ps1`, `run-transition.ps1`도 같다 |
| `run-pool.ps1`, `pool-timeline.py`, `run-budget.ps1`, `check-budget.py`, `plot-budget.py`, `run-mgr.ps1`, `check-mgr.py`, `mgr-trace-table.py`, `plot-post02-manager.py`, `check-iskm.py` | v1 참고 묶음(`Pool1`, `Budget1`, `Mgr1`, `MgrT1`)과 글 02의 러너·그림 |
| `plot-readme.py`, `build-site.py` | 루트 README의 그림, 사이트 데이터. v1 총괄 묶음(`Full3`)의 조건 이름(`L1`\~`L8`, `A8-*`)을 읽는다. 새 묶음의 이름으로 바꾸는 것은 W14의 일이다 |

## 에셋을 다시 만드는 것

| 파일 | 역할 |
| --- | --- |
| `restore-assets.ps1`, `restore-assets.manifest.txt` | 클론 뒤 한 번: 아래 스크립트를 순서대로 불러 git 밖 에셋을 복구하고 목록과 대조한다(`-DryRun`, `-Force`, `-AssetHost`). 순서는 [Docs/Guides/setup.md](../Docs/Guides/setup.md) |
| `migrate-assets.py` | Fab 원본 팩에서 쓰는 에셋만 옮긴다 |
| `make-static-minion.py`, `lab_unit_colors.py` | 사다리 단계 2·3의 정지 메시 인스턴스(`SM_LabMinion`), 미니언 색 값 |
| `bake-vat.py` | VAT 베이크(M2\~M4 원거리 표현, 액터 머티리얼) |
| `create-iskm.py` | ISKM 에셋(M5) |
| `create-bench-map.py` | 맵 `L_Bench` |
| `create-anim-bp.py`, `create-anim-bp-large.md`, `bind-anim-start.md` | AnimBP와 Start Position 바인딩 순서 |
| `create-band-floor.py`, `create-band-floor.md` | 바닥 밴드 표시 머티리얼(`-LabBandFloor`) |
| `create-bolt-fx.md` | 저격 선 Niagara 시스템을 만든 순서 |
| `create-crowd-fx.md` | 배경 무리 Niagara 시스템 `NS_LabCrowd`를 만든 순서 |

## 끝난 묶음의 러너와 진단 도구

지난 묶음을 어떻게 돌렸는지의 기록이다. 다시 돌리지 않는다(라벨이 이미 있어 거부된다). 경위는 `Docs/Worklog/M<N>.md`에 있다. `Archive/`의 러너는 옮기기 전 위치(`Scripts/`)를 기준으로 다른 스크립트를 부르므로 그 자리에서는 돌지 않는다.

| 파일 | 역할 |
| --- | --- |
| `Archive/run-e0.ps1`, `run-e1.ps1`, `run-e1bc.ps1`, `run-e1d.ps1`, `run-e2.ps1`, `run-e3.ps1` | M0\~M3 묶음 |
| `Archive/run-m3b.ps1`, `run-m3b-all.ps1`, `m3b-report.py` | M3b 묶음과 보고서 |
| `Archive/run-m4-all.ps1`, `run-m4-e4d.ps1`, `run-m4-far.ps1`, `run-m4-trace2.ps1`, `run-m4-trace3.ps1` | M4 묶음, E4 (d), 그리는 거리, 트레이스 |
| `Archive/run-m5-all.ps1`, `run-m5-bounds.ps1`, `run-m5-trace.ps1` | M5 묶음, 고정 바운드, 트레이스 |
| `trace-export.ps1`, `trace-report.py`, `trace-timeline.py`, `trace-order.py`, `trace-overlap.py` | Insights 트레이스에서 표 뽑기와 분석 |
| `breakdown.py`, `noise-table.py`, `Archive/fg-probe.ps1` | 스탯 차이, 잡음, 앞 창 진단 |
| `transition-score.py`, `transition-strips.py` | 전환(하이드레이션) 화질 점수와 비교 시트 |
| `run-transition.ps1` | Development 전환 점수 묶음(네 조건 × 시드 셋, ADR-0012). 결과는 `Measurements/<묶음>/transition-dev/runs.csv` |
| `Archive/run-flicker.ps1`, `flicker-scan.py` | 미니언 번쩍임 진단 |
| `run-q4.ps1`, `turn-fill.py` | 품질 올리기 묶음(액터 상한, 전환 거리)과 빠른 회전 뒤 액터가 채워지는 시간 |
| `dump-death-clips.py`, `death-clip-sheet.py` | 죽기 클립 후보 비교 |
