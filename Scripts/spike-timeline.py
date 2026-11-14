"""시험 실행 묶음(Spike<n>)의 가까운 개체 수를 시간 구간으로 정리한다(Docs/Worklog/V2.md "시험 실행").

Saved/Profiling/CSV/<접두사>*.csv를 읽어 측정 구간을 누적 FrameTime으로 --bin초씩 나누고, 구간마다
액터 수(Lab/Actors), ISKM 인스턴스 수(Lab/IskmInstances), 오브 30m 안 개체 수(Lab/Near30)의 평균과 최댓값,
프레임 p50·p95를 쓴다. --batch를 주면 Measurements/<묶음>/timeline.csv에 쓰고, 아니면 화면에만 낸다.
표준 라이브러리만 쓴다.
실행: py -3 -I Scripts/spike-timeline.py --batch Spike1 [--bin 5]
      py -3 -I Scripts/spike-timeline.py --prefix SpikeDev-b- [--bin 5]
"""
import argparse
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from summarize import CSV_DIR, ROOT, column, percentile, read_profile  # noqa: E402

STATS = [("actors", "Lab/Actors"), ("iskm", "Lab/IskmInstances"), ("near30", "Lab/Near30")]
# 표현 전환 수(ULabRepSwitchCountProcessor, 프레임마다 그 프레임의 수). 구간의 합 ÷ 구간 길이 = 초당 전환 수
SWITCH = [("none_to_inst", "Lab/SwNoneToInst"), ("inst_to_actor", "Lab/SwInstToActor"), ("actor_to_inst", "Lab/SwActorToInst"),
          ("inst_to_none", "Lab/SwInstToNone"), ("other", "Lab/SwOther"), ("round", "Lab/SwRound")]
FIELDS = (["label", "orbit", "bin", "t_end", "frames", "frame_p50", "frame_p95"] + [f"{k}_{s}" for k, _ in STATS for s in ("mean", "max")]
          + [f"sw_{k}_per_s" for k, _ in SWITCH])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch", default="")
    ap.add_argument("--prefix", default="")
    ap.add_argument("--bin", type=float, default=5.0, help="구간 길이(초)")
    args = ap.parse_args()
    prefix = args.prefix or f"{args.batch}-"
    if prefix == "-":
        ap.error("--batch 또는 --prefix가 필요하다")
    rows = []
    for path in sorted(CSV_DIR.glob(f"{prefix}*.csv")):
        header, data, meta = read_profile(path)
        ft = column(header, data, ["FrameTime"])
        if not ft:
            continue
        cols = {k: column(header, data, [name]) for k, name in STATS + SWITCH}
        orbit = meta.get("laborbit", meta.get("LabOrbit", ""))
        t, start, b, t_start = 0.0, 0, 0, 0.0
        edges = []
        for i, v in enumerate(ft):
            t += v / 1000.0
            if t >= (b + 1) * args.bin:
                edges.append((start, i + 1, t, t - t_start))
                start, b, t_start = i + 1, b + 1, t
        if start < len(ft):
            edges.append((start, len(ft), t, t - t_start))
        for n, (s, e, t_end, span) in enumerate(edges + [(0, len(ft), t, t)]):
            row = {"label": path.stem, "orbit": orbit, "bin": "all" if n == len(edges) else n + 1, "t_end": round(t_end, 1), "frames": e - s,
                   "frame_p50": round(percentile(ft[s:e], 0.5), 2), "frame_p95": round(percentile(ft[s:e], 0.95), 2)}
            for k, _ in STATS:
                v = cols[k][s:e] if len(cols[k]) == len(ft) else []
                row[f"{k}_mean"] = round(sum(v) / len(v), 1) if v else ""
                row[f"{k}_max"] = int(max(v)) if v else ""
            for k, _ in SWITCH:
                v = cols[k][s:e] if len(cols[k]) == len(ft) else []
                row[f"sw_{k}_per_s"] = round(sum(v) / span, 2) if v and span > 0 else ""
            rows.append(row)
    if args.batch:
        out_dir = ROOT / "Measurements" / args.batch
        out_dir.mkdir(parents=True, exist_ok=True)
        with open(out_dir / "timeline.csv", "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=FIELDS)
            w.writeheader()
            w.writerows(rows)
    for r in rows:
        print(",".join(str(r[k]) for k in FIELDS))


if __name__ == "__main__":
    main()
