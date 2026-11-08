"""두 조건(라벨 접두사)의 CSV 프로파일에서 스탯 열마다 3회 중앙값의 중앙값을 구해, 차이가 큰 순서로 보여 준다.
병목 찾기용(2026-11-09, M3 E3: 액터를 붙여 늘어난 Game Thread 시간을 항목별로 나눔). 표준 라이브러리만 쓴다.

실행: py -3 Scripts/breakdown.py M3-m2off-N20000-P1 M3-mass-N20000-P1 [--prefix Exclusive/GameThread/] [--top 25]
"""
import argparse
import csv
import glob
import os
import statistics

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CSV_DIR = os.path.join(ROOT, "Saved", "Profiling", "CSV")


def read(path):
    with open(path, newline="", encoding="utf-8", errors="ignore") as f:
        rows = list(csv.reader(f))
    # 끝의 헤더 행이 전체 열 목록이다(engine-notes.md 10절). 메타데이터 행([key],value)은 버린다.
    header = None
    for r in reversed(rows):
        if r and r[0] == "EVENTS":
            header = r
            break
    data = [r for r in rows[1:] if r and r[0] != "EVENTS" and not r[0].startswith("[")]
    return header, data


def medians(label_prefix, prefix):
    per_run = []
    for path in sorted(glob.glob(os.path.join(CSV_DIR, label_prefix + "-r[1-9].csv"))):
        header, data = read(path)
        cols = {}
        for i, name in enumerate(header):
            if prefix and not name.startswith(prefix):
                continue
            vals = []
            for r in data:
                if i < len(r) and r[i] not in ("", None):
                    try:
                        vals.append(float(r[i]))
                    except ValueError:
                        pass
            if vals:
                cols[name] = statistics.median(vals)
        per_run.append(cols)
    names = set().union(*per_run) if per_run else set()
    return {n: statistics.median([run.get(n, 0.0) for run in per_run]) for n in names}, len(per_run)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("base")
    ap.add_argument("test")
    ap.add_argument("--prefix", default="Exclusive/GameThread/")
    ap.add_argument("--top", type=int, default=25)
    a = ap.parse_args()
    b, nb = medians(a.base, a.prefix)
    t, nt = medians(a.test, a.prefix)
    rows = sorted(((t.get(n, 0.0) - b.get(n, 0.0), n) for n in set(b) | set(t)), reverse=True)
    print(f"{a.base} ({nb} runs) -> {a.test} ({nt} runs), prefix {a.prefix!r}, ms p50")
    for d, n in rows[: a.top]:
        print(f"{d:+8.3f}  {b.get(n, 0.0):8.3f} -> {t.get(n, 0.0):8.3f}  {n}")
    print("sum of shown deltas", round(sum(d for d, _ in rows[: a.top]), 3), " total delta", round(sum(d for d, _ in rows), 3))


if __name__ == "__main__":
    main()
