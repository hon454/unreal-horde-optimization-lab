# 스크립트

M0에서 만든다. 사양은 [Docs/Guides/commands.md](../Docs/Guides/commands.md) "스크립트 사양". PowerShell은 UTF-8 BOM으로 저장한다(Windows PowerShell 5.1이 BOM 없는 한글을 잘못 읽는다). Python은 3.10 이상, `matplotlib`, `pandas`.

| 파일 | 역할 |
| --- | --- |
| `common.ps1` | 엔진 경로(`EngineAssociation` → 레지스트리), 공통 검사 |
| `build.ps1` | Development 에디터 빌드 |
| `run-dev.ps1` | Development `-game` 실행(개발·오버레이) |
| `package.ps1` | Test 패키지 |
| `run-bench.ps1` | Test 패키지 벤치마크 실행(라벨, 반복) |
| `summarize.py` | 프레임 CSV → `Measurements/M<N>/runs.csv`, `summary.csv` |
| `plot.py` | `summary.csv` → `Docs/figures/` |
| `capture-video.ps1` | 화면 녹화(승인 뒤) |
