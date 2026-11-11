# 측정 결과

마일스톤마다 `M<N>/`에 `runs.csv`(실행 하나당 한 행), `summary.csv`(조건당 한 행, 3회 중앙값과 범위), `shots/`(대표 스크린샷)를 둔다. 열은 `Scripts/summarize.py`가 정하고, 보조 지표(밴드별 개체 수, Simulation LOD 틱, ISKM 트랙 등)는 따로 파일을 두지 않고 `summary.csv`의 뒤쪽 열로 붙는다. 프레임 단위 원본 CSV, 트레이스, 영상은 저장소 밖(`Saved/`)에 두고 라벨만 적는다. 규약은 [Docs/Guides/measurement.md](../Docs/Guides/measurement.md).