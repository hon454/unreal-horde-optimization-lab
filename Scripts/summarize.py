"""벤치마크 CSV를 마일스톤 측정 결과로 정리한다.

Saved/Profiling/CSV/<라벨>.csv(run-bench.ps1이 패키지에서 가져온 것)를 읽어
Measurements/<M>/runs.csv에 실행마다 한 행을 더하고, summary.csv를 조건별 중앙값·최소·최대로 다시 쓴다.
라벨 형식은 <M>-<variant>-N<n>-<preset>-r<i>이고, r0(묶음의 첫 실행, 셰이더 캐시)은 summary에서 뺀다.
열 정의는 Docs/Guides/measurement.md "지표". 표준 라이브러리만 쓴다.

실행: py -3 Scripts/summarize.py --milestone M0 [--pc 1]
"""
import argparse
import csv
import re
import statistics
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CSV_DIR = ROOT / "Saved" / "Profiling" / "CSV"
LABEL_RE = re.compile(r"^(?P<m>M\d+b?)-(?P<variant>[a-z]+)-N(?P<n>\d+)-(?P<preset>P\d)-r(?P<run>\d+)$")

# 엔진 CSV 열(LaunchEngineLoop.cpp:1518-1525)과 후보 이름
COLS = {
    "frame": ["FrameTime"],
    "game": ["GameThreadTime"],
    "render": ["RenderThreadTime"],
    "gpu": ["GPUTime"],
    "draw_calls": ["RHI/DrawCalls", "DrawCalls"],
    "mem": ["PhysicalUsedMB", "Memory/PhysicalUsedMB"],
}
RUN_FIELDS = ["label", "milestone", "variant", "n", "preset", "seed", "run", "pc", "frames",
              "game_p50_ms", "game_p99_ms", "render_p50_ms", "gpu_p50_ms", "frame_p50_ms", "frame_p99_ms",
              "hitch_abs", "hitch_rel", "draw_calls_p50", "mem_total_mb"]
CORE = ["game_p50_ms", "game_p99_ms", "render_p50_ms", "gpu_p50_ms", "frame_p99_ms",
        "hitch_abs", "hitch_rel", "draw_calls_p50", "mem_total_mb"]


def percentile(values, q):
    if not values:
        return ""
    s = sorted(values)
    k = (len(s) - 1) * q
    lo, hi = int(k), min(int(k) + 1, len(s) - 1)
    return round(s[lo] + (s[hi] - s[lo]) * (k - lo), 3)


def read_profile(path):
    """헤더, 프레임 행, 메타데이터(마지막의 [키],값 쌍)를 돌려준다."""
    with open(path, newline="", encoding="utf-8-sig") as f:
        rows = list(csv.reader(f))
    header = rows[0]
    data, meta = [], {}
    for row in rows[1:]:
        if row and row[0].startswith("["):
            for i in range(0, len(row) - 1, 2):
                meta[row[i].strip("[]")] = row[i + 1]
            continue
        if len(row) == len(header):
            try:
                float(row[0])
            except ValueError:
                continue
            data.append(row)
    return header, data, meta


def column(header, data, names):
    for name in names:
        if name in header:
            i = header.index(name)
            out = []
            for r in data:
                try:
                    out.append(float(r[i]))
                except ValueError:
                    pass
            return out
    return []


def summarize_run(path, pc):
    label = path.stem
    m = LABEL_RE.match(label)
    if not m:
        return None
    header, data, meta = read_profile(path)
    frame = column(header, data, COLS["frame"])
    p50 = percentile(frame, 0.5)
    row = {
        "label": label, "milestone": m["m"], "variant": m["variant"], "n": int(m["n"]),
        "preset": m["preset"], "seed": meta.get("LabSeed", ""), "run": int(m["run"]), "pc": pc,
        "frames": len(data),
        "game_p50_ms": percentile(column(header, data, COLS["game"]), 0.5),
        "game_p99_ms": percentile(column(header, data, COLS["game"]), 0.99),
        "render_p50_ms": percentile(column(header, data, COLS["render"]), 0.5),
        "gpu_p50_ms": percentile(column(header, data, COLS["gpu"]), 0.5),
        "frame_p50_ms": p50,
        "frame_p99_ms": percentile(frame, 0.99),
        "hitch_abs": sum(1 for v in frame if v > 50.0),
        "hitch_rel": sum(1 for v in frame if p50 != "" and v > 2 * p50),
        "draw_calls_p50": percentile(column(header, data, COLS["draw_calls"]), 0.5),
        "mem_total_mb": percentile(column(header, data, COLS["mem"]), 0.5),
    }
    return row


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--milestone", required=True)
    ap.add_argument("--pc", default="1", help="Docs/Reference/pc-specs.md의 행 번호")
    args = ap.parse_args()

    out_dir = ROOT / "Measurements" / args.milestone
    out_dir.mkdir(parents=True, exist_ok=True)
    runs_path = out_dir / "runs.csv"

    existing = {}
    if runs_path.exists():
        with open(runs_path, newline="", encoding="utf-8") as f:
            for r in csv.DictReader(f):
                existing[r["label"]] = r

    added = 0
    for path in sorted(CSV_DIR.glob(f"{args.milestone}-*.csv")):
        if path.stem in existing:
            continue
        row = summarize_run(path, args.pc)
        if row:
            existing[row["label"]] = row
            added += 1

    rows = sorted(existing.values(), key=lambda r: (r["variant"], int(r["n"]), r["preset"], int(r["run"])))
    with open(runs_path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=RUN_FIELDS)
        w.writeheader()
        w.writerows(rows)

    groups = {}
    for r in rows:
        if int(r["run"]) == 0:
            continue
        groups.setdefault((r["variant"], int(r["n"]), r["preset"]), []).append(r)

    fields = ["variant", "n", "preset", "runs"]
    for c in CORE:
        fields += [c, f"{c}_min", f"{c}_max"]
    with open(out_dir / "summary.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for (variant, n, preset), rs in sorted(groups.items()):
            out = {"variant": variant, "n": n, "preset": preset, "runs": len(rs)}
            for c in CORE:
                vals = [float(r[c]) for r in rs if str(r[c]) != ""]
                out[c] = round(statistics.median(vals), 3) if vals else ""
                out[f"{c}_min"] = min(vals) if vals else ""
                out[f"{c}_max"] = max(vals) if vals else ""
            w.writerow(out)

    print(f"runs.csv: {len(rows)} rows (+{added}), summary.csv: {len(groups)} conditions -> {out_dir}")


if __name__ == "__main__":
    main()
