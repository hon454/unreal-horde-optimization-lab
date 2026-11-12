"""엔티티 풀 묶음(Pool<n>)의 시간 추이를 1분 구간으로 정리한다(Docs/Worklog/Q6.md "Full2 재측정").

Saved/Profiling/CSV/<묶음>-*.csv(run-pool.ps1)를 읽어 측정 구간을 누적 FrameTime으로 1분씩 나누고,
구간마다 ISKM 트랙 수(Lab/IskmTracks, 구간 끝 값과 최댓값), GPU 메모리(GPUMem/LocalUsedMB, 평균과 최댓값),
프레임·Game·GPU p50을 Measurements/<묶음>/timeline.csv에 쓴다. 표준 라이브러리만 쓴다.
실행: py -3 -I Scripts/pool-timeline.py --batch Pool1 [--bin 60]
"""
import argparse
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from summarize import CSV_DIR, ROOT, column, percentile, read_profile  # noqa: E402

FIELDS = ["label", "minute", "frames", "frame_p50", "game_p50", "gpu_p50",
          "iskm_tracks_end", "iskm_tracks_max", "gpu_mem_mean", "gpu_mem_max"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch", required=True)
    ap.add_argument("--bin", type=float, default=60.0, help="구간 길이(초)")
    args = ap.parse_args()
    out_dir = ROOT / "Measurements" / args.batch
    out_dir.mkdir(parents=True, exist_ok=True)
    rows = []
    for path in sorted(CSV_DIR.glob(f"{args.batch}-*.csv")):
        header, data, _meta = read_profile(path)
        cols = {k: column(header, data, [k]) for k in ["FrameTime", "GameThreadTime", "GPUTime", "Lab/IskmTracks", "GPUMem/LocalUsedMB"]}
        ft = cols["FrameTime"]
        if not ft:
            continue
        t, start, b = 0.0, 0, 0
        edges = []
        for i, v in enumerate(ft):
            t += v / 1000.0
            if t >= (b + 1) * args.bin:
                edges.append((start, i + 1))
                start, b = i + 1, b + 1
        if start < len(ft):
            edges.append((start, len(ft)))
        for m, (s, e) in enumerate(edges):
            def seg(k):
                v = cols[k]
                return v[s:e] if len(v) == len(ft) else []
            tr, gm = seg("Lab/IskmTracks"), seg("GPUMem/LocalUsedMB")
            rows.append({
                "label": path.stem, "minute": m + 1, "frames": e - s,
                "frame_p50": percentile(seg("FrameTime"), 0.5),
                "game_p50": percentile(seg("GameThreadTime"), 0.5),
                "gpu_p50": percentile(seg("GPUTime"), 0.5),
                "iskm_tracks_end": tr[-1] if tr else "",
                "iskm_tracks_max": max(tr) if tr else "",
                "gpu_mem_mean": round(sum(gm) / len(gm), 1) if gm else "",
                "gpu_mem_max": max(gm) if gm else "",
            })
    with open(out_dir / "timeline.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=FIELDS)
        w.writeheader()
        w.writerows(rows)
    for r in rows:
        print(",".join(str(r[k]) for k in FIELDS))


if __name__ == "__main__":
    main()
