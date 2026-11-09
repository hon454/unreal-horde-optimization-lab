"""벤치마크 CSV를 마일스톤 측정 결과로 정리한다.

Saved/Profiling/CSV/<라벨>.csv(run-bench.ps1이 패키지에서 가져온 것)를 읽어
Measurements/<M>/runs.csv에 실행마다 한 행을 더하고, summary.csv를 조건별 중앙값·최소·최대로 다시 쓴다.
라벨 형식은 <M>-<variant>-N<n>-<preset><tag>-r<i>이고, r0(묶음의 첫 실행, 셰이더 캐시)은 summary에서 뺀다.
태그 aa*는 묶음 끝 A/A로 summary에 <variant>-aa 조건으로 따로 쓰고, trace*는 summary에서 뺀다.
열 정의는 Docs/Guides/measurement.md "지표". 표준 라이브러리만 쓴다.

M3b부터: frame_p95_ms(대표 지표), bottleneck(프레임·Game·Render·GPU p50 중 최댓값의 이름), build(CSV 메타데이터)를 더한다.
기존 runs.csv 행은 건드리지 않고 새 라벨만 더한다. 원본 CSV로 모든 행을 다시 계산하려면 --rebuild.
실행: py -3 -I Scripts/summarize.py --milestone M0 [--pc 1] [--rebuild]
"""
import argparse
import csv
import re
import statistics
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CSV_DIR = ROOT / "Saved" / "Profiling" / "CSV"
LABEL_RE = re.compile(r"^(?P<m>M\d+b?)-(?P<variant>[a-z][a-z0-9]*)-N(?P<n>\d+)-(?P<preset>P\d)(?P<tag>[a-z0-9]*)-r(?P<run>\d+)$")

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
              "game_p50_ms", "game_p99_ms", "render_p50_ms", "gpu_p50_ms", "frame_p50_ms", "frame_p95_ms", "frame_p99_ms",
              "hitch_abs", "hitch_rel", "draw_calls_p50", "mem_total_mb", "bottleneck", "build"]
CORE = ["game_p50_ms", "game_p99_ms", "render_p50_ms", "gpu_p50_ms", "frame_p50_ms", "frame_p95_ms", "frame_p99_ms",
        "hitch_abs", "hitch_rel", "draw_calls_p50", "mem_total_mb"]
# 병목 후보: p50 열 이름 -> 표시 이름 (measurement.md "N@16.7ms": 프레임·Game·Render·GPU p50 중 최댓값)
BOTTLENECK_COLS = [("frame_p50_ms", "frame"), ("game_p50_ms", "game"), ("render_p50_ms", "render"), ("gpu_p50_ms", "gpu")]
# 게임이 CSV_METADATA로 쓰는 빌드 키(소문자). 키 이름은 Docs/Worklog/M3b.md. 없으면 build는 빈칸.
BUILD_KEYS = ["labbuild", "labcvars", "labdriver"]


def percentile(values, q):
    if not values:
        return ""
    s = sorted(values)
    k = (len(s) - 1) * q
    lo, hi = int(k), min(int(k) + 1, len(s) - 1)
    return round(s[lo] + (s[hi] - s[lo]) * (k - lo), 3)


def bottleneck(row):
    best, name = None, ""
    for col, label in BOTTLENECK_COLS:
        v = row.get(col, "")
        if v == "" or v is None:
            continue
        if best is None or float(v) > best:
            best, name = float(v), label
    return name


def build_string(meta):
    parts = [f"{k}={meta[k]}" for k in BUILD_KEYS if meta.get(k, "") != ""]
    return ";".join(parts)


def read_profile(path):
    """헤더, 프레임 행, 메타데이터를 돌려준다.

    엔진 CSV는 첫 행이 헤더(첫 열 EVENTS), 이어 프레임 행, 끝에 헤더 반복과 [키],값 쌍의 메타데이터 행이 온다.
    메타데이터 키는 소문자다(예: [labn],500).
    """
    with open(path, newline="", encoding="utf-8-sig") as f:
        rows = list(csv.reader(f))
    # 캡처 중에 새 스탯이 생기면 뒤쪽 행일수록 열이 늘어난다. 끝의 헤더([HasHeaderRowAtEnd])가 전체 열 목록이다.
    header = [r for r in rows if r and r[0] == "EVENTS"][-1]
    data, meta = [], {}
    for row in rows:
        if not row or row[0] == "EVENTS":
            continue
        if row[0].startswith("["):
            for i in range(0, len(row) - 1, 2):
                meta[row[i].strip("[]").lower()] = row[i + 1]
            continue
        try:
            float(row[1])
        except (ValueError, IndexError):
            continue
        data.append(row + [""] * (len(header) - len(row)))
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
        "preset": m["preset"], "seed": meta.get("labseed", ""), "run": int(m["run"]), "pc": pc,
        "frames": len(data),
        "game_p50_ms": percentile(column(header, data, COLS["game"]), 0.5),
        "game_p99_ms": percentile(column(header, data, COLS["game"]), 0.99),
        "render_p50_ms": percentile(column(header, data, COLS["render"]), 0.5),
        "gpu_p50_ms": percentile(column(header, data, COLS["gpu"]), 0.5),
        "frame_p50_ms": p50,
        "frame_p95_ms": percentile(frame, 0.95),
        "frame_p99_ms": percentile(frame, 0.99),
        "hitch_abs": sum(1 for v in frame if v > 50.0),
        "hitch_rel": sum(1 for v in frame if p50 != "" and v > 2 * p50),
        "draw_calls_p50": percentile(column(header, data, COLS["draw_calls"]), 0.5),
        "mem_total_mb": percentile(column(header, data, COLS["mem"]), 0.5),
        "build": build_string(meta),
    }
    row["bottleneck"] = bottleneck(row)
    return row


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--milestone", required=True)
    ap.add_argument("--rebuild", action="store_true", help="기존 행도 원본 CSV로 다시 계산한다(CSV가 없는 행은 그대로 둔다)")
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
        if path.stem in existing and not args.rebuild:
            continue
        row = summarize_run(path, existing[path.stem]["pc"] if path.stem in existing else args.pc)
        if row:
            added += path.stem not in existing
            existing[row["label"]] = row

    rows = sorted(existing.values(), key=lambda r: (r["variant"], int(r["n"]), r["preset"], int(r["run"])))
    with open(runs_path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=RUN_FIELDS)
        w.writeheader()
        for r in rows:
            r.setdefault("bottleneck", bottleneck(r))
            w.writerow({k: r.get(k, "") for k in RUN_FIELDS})

    groups = {}
    for r in rows:
        if int(r["run"]) == 0:
            continue
        tag = LABEL_RE.match(r["label"])["tag"]
        if tag.startswith("trace"):
            continue  # Insights 트레이스 실행은 비교에 쓰지 않는다(runs.csv에는 남는다)
        variant = r["variant"] + ("-aa" if tag.startswith("aa") else "")  # 묶음 끝 A/A는 기준점과 따로 요약한다
        groups.setdefault((variant, int(r["n"]), r["preset"]), []).append(r)

    fields = ["variant", "n", "preset", "runs"]
    for c in CORE:
        fields += [c, f"{c}_min", f"{c}_max"]
    fields += ["bottleneck", "build"]
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
            out["bottleneck"] = bottleneck(out)
            builds = sorted({r.get("build", "") for r in rs if r.get("build", "")})
            out["build"] = builds[0] if len(builds) == 1 else " | ".join(builds)
            w.writerow(out)

    print(f"runs.csv: {len(rows)} rows (+{added}), summary.csv: {len(groups)} conditions -> {out_dir}")


if __name__ == "__main__":
    main()
