# 스크립트

사양과 인자는 [Docs/Guides/commands.md](../Docs/Guides/commands.md). PowerShell은 UTF-8 BOM으로 저장한다(Windows PowerShell 5.1이 BOM 없는 한글을 잘못 읽는다). Python은 `py -3`(3.9)이고 표준 라이브러리만 쓴다. 에디터 Python 스크립트는 커맨드렛으로 돌린다(commands.md "파생 에셋과 맵").

## 매번 쓰는 것

| 파일 | 역할 |
| --- | --- |
| `common.ps1` | 엔진 경로(`EngineAssociation` → 레지스트리), 공통 검사 |
| `build.ps1` | Development 에디터 빌드 |
| `run-dev.ps1` | Development `-game` 실행(개발·오버레이·스크린샷) |
| `package.ps1` | Test 패키지 |
| `run-bench.ps1` | Test 패키지 벤치마크 한 조건(라벨, 반복, 앞 창 검사, 트레이스) |
| `summarize.py` | 프레임 CSV → `Measurements/M<N>/runs.csv`, `summary.csv` |
| `spikes.py` | 묶음의 주기 스파이크 검사 |
| `plot.py` | `summary.csv` → `Docs/figures/` |
| `plot-readme.py` | 최종 측정 CSV → 루트 README의 그림(`Docs/figures/readme/`). 단계별 메모리·드로우 콜 그림(`resources.svg`)은 `summary.csv`의 `mem_total_mb`·`gpu_mem_p50`·`draw_calls_p50`과 `runs.csv`의 세 실행 범위로 그리고, 그림에 적은 증가가 세 실행 범위보다 큰지 확인한다. 시나리오 도식(`scenario.svg`)은 소스의 고정값을 스크립트 위쪽 상수로 옮겨 그린다 |
| `plot-code-tour.py` | 코드 투어의 한 프레임 흐름 그림(`Docs/figures/code-tour/frame-flow.svg`). 수치 없이 프로세서 선언 순서를 그린다 |
| `check-iskm.py` | 러너의 메타데이터 검사(ISKM 인스턴스 수) |

## 에셋을 다시 만드는 것

| 파일 | 역할 |
| --- | --- |
| `restore-assets.ps1`, `restore-assets.manifest.txt` | 클론 뒤 한 번: 아래 스크립트를 순서대로 불러 git 밖 에셋을 복구하고 목록과 대조한다(`-DryRun`, `-Force`, `-AssetHost`). 순서는 [Docs/Guides/setup.md](../Docs/Guides/setup.md) |
| `migrate-assets.py` | Fab 원본 팩에서 쓰는 에셋만 옮긴다 |
| `make-static-minion.py`, `lab_unit_colors.py` | `static` 변형과 M1의 정적 메시, 미니언 색 값 |
| `bake-vat.py` | VAT 베이크(M2\~M4 원거리 표현, 액터 머티리얼) |
| `create-iskm.py` | ISKM 에셋(M5) |
| `create-bench-map.py` | 맵 `L_Bench` |
| `create-anim-bp.py`, `create-anim-bp-large.md`, `bind-anim-start.md` | AnimBP와 Start Position 바인딩 순서 |
| `create-band-floor.py`, `create-band-floor.md` | 바닥 밴드 표시 머티리얼(`-LabBandFloor`) |
| `create-bolt-fx.md` | 저격 선 Niagara 시스템을 만든 순서 |

## 끝난 묶음의 러너와 진단 도구

지난 묶음을 어떻게 돌렸는지의 기록이다. 다시 돌리지 않는다(라벨이 이미 있어 거부된다). 경위는 `Docs/Worklog/M<N>.md`에 있다.

| 파일 | 역할 |
| --- | --- |
| `run-e0.ps1`, `run-e1.ps1`, `run-e1bc.ps1`, `run-e1d.ps1`, `run-e2.ps1`, `run-e3.ps1` | M0\~M3 묶음 |
| `run-m3b.ps1`, `run-m3b-all.ps1`, `m3b-report.py` | M3b 묶음과 보고서 |
| `run-m4-all.ps1`, `run-m4-e4d.ps1`, `run-m4-far.ps1`, `run-m4-trace2.ps1`, `run-m4-trace3.ps1` | M4 묶음, E4 (d), 그리는 거리, 트레이스 |
| `run-m5-all.ps1`, `run-m5-bounds.ps1`, `run-m5-trace.ps1` | M5 묶음, 고정 바운드, 트레이스 |
| `trace-export.ps1`, `trace-report.py`, `trace-timeline.py`, `trace-order.py`, `trace-overlap.py` | Insights 트레이스에서 표 뽑기와 분석 |
| `breakdown.py`, `noise-table.py`, `fg-probe.ps1` | 스탯 차이, 잡음, 앞 창 진단 |
| `transition-score.py`, `transition-strips.py` | 전환(하이드레이션) 화질 점수와 비교 시트 |
| `run-transition.ps1` | Development 전환 점수 묶음(네 조건 × 시드 셋, ADR-0012). 결과는 `Measurements/<묶음>/transition-dev/runs.csv` |
| `run-flicker.ps1`, `flicker-scan.py` | 미니언 번쩍임 진단 |
| `dump-death-clips.py`, `death-clip-sheet.py` | 죽기 클립 후보 비교 |
