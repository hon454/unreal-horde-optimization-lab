"""같은 조건 실행들의 프레임·Game·Render p50 값을 실행별로 늘어놓고 중앙값·최소~최대·표준편차를 낸다 (잡음 진단용, Markdown 표 출력).

예: py -3 -I Scripts/noise-table.py M3b-m3mass-N20000-P1aff- M3b-m3mass-N20000-P1noaff-
인자는 라벨 접두사(Saved/Profiling/CSV/<접두사>*.csv). r0은 뺀다. 표준 라이브러리만 쓴다.
"""
import statistics
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from summarize import CSV_DIR, COLS, LABEL_RE, read_profile, column, percentile  # noqa: E402

sys.stdout.reconfigure(encoding="utf-8")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    for prefix in sys.argv[1:]:
        rows = []
        for p in sorted(CSV_DIR.glob(f"{prefix}*.csv")):
            m = LABEL_RE.match(p.stem)
            if not m or int(m["run"]) == 0:
                continue
            h, d, _ = read_profile(p)
            rows.append((p.stem, *(percentile(column(h, d, COLS[k]), 0.5) for k in ("frame", "game", "render")),
                         percentile(column(h, d, COLS["frame"]), 0.95)))
        print(f"\n**{prefix}** ({len(rows)}회)\n")
        print("| 라벨 | 프레임 p50 | 프레임 p95 | Game p50 | Render p50 |\n| --- | --- | --- | --- | --- |")
        for lab, fr, gm, rd, p95 in rows:
            print(f"| {lab} | {fr:.2f} | {p95:.2f} | {gm:.2f} | {rd:.2f} |")
        if len(rows) >= 2:
            for name, i in (("프레임 p50", 1), ("프레임 p95", 4), ("Game p50", 2), ("Render p50", 3)):
                v = [r[i] for r in rows]
                print(f"- {name}: 중앙값 {statistics.median(v):.2f}, 최소\~최대 {min(v):.2f}\~{max(v):.2f}, 표준편차 {statistics.stdev(v):.2f}")


if __name__ == "__main__":
    sys.exit(main())
