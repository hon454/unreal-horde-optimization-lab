"""병렬 트레이스의 프로세서 이벤트(TimingEvents CSV: ThreadName,TimerName,StartTime,Duration,Depth)에서 프레임마다
(1) 수집기 끝 ~ 표현 LOD 시작 사이에 게임 스레드가 돈 프로세서와 합, (2) 표현 LOD가 돌던 구간과 워커의 이동 프로세서 구간이 겹치는 프레임 비율을 센다.
M4 병렬 트레이스 분석(Docs/Worklog/M4.md "표현 LOD 대기: 가르는 트레이스"). 실행: py -3 -I Scripts/trace-overlap.py a=ev-a.csv b=ev-b.csv
"""
import csv
import re
import statistics
import sys
from collections import defaultdict

sys.dont_write_bytecode = True
PHASE = "FMassProcessingPhase::ExecuteTick EMassProcessingPhase::PrePhysics"
MOVE = ("MassApplyMovementProcessor", "MassSmoothOrientationProcessor")


def analyse(path):
    rows = []
    with open(path, newline="", encoding="utf-8", errors="replace") as f:
        for r in csv.DictReader(f):
            n = re.sub(r"_\d{6,}$", "", r["TimerName"])
            if n.startswith("UE::Mass"):
                continue
            rows.append((r["ThreadName"], n, float(r["StartTime"]), float(r["Duration"])))
    ph = sorted(s for t, n, s, d in rows if t == "GameThread" and n == PHASE)
    frames = defaultdict(dict)  # 프레임 -> 이름 -> (시작, 끝, 스레드)
    import bisect
    for t, n, s, d in rows:
        if not n.endswith("Processor"):
            continue
        i = bisect.bisect_right(ph, s) - 1
        if i < 0:
            continue
        frames[i][n] = (s, s + d, t)
    res = defaultdict(list)
    gap_names = defaultdict(list)
    for i, f in frames.items():
        c, v = f.get("MassLODCollectorProcessor"), f.get("MassVisualizationLODProcessor")
        if not c or not v:
            continue
        res["vis_dur"].append((v[1] - v[0]) * 1000)
        res["vis_start_after_collector_end"].append((v[0] - c[1]) * 1000)
        mid = [(n, (e - s) * 1000) for n, (s, e, t) in f.items() if t == "GameThread" and c[1] - 1e-5 <= s < v[0] - 1e-5 and n != "MassVisualizationLODProcessor"]
        res["gap_work"].append(sum(d for n, d in mid))
        for n, d in mid:
            gap_names[n].append(d)
        ov = {}
        for m in MOVE:
            if m in f:
                ms, me, _ = f[m]
                ov[m] = max(0.0, min(v[1], me) - max(v[0], ms)) * 1000
        res["move_overlap_ms"].append(sum(ov.values()))
        res["overlaps_any_move"].append(1 if any(x > 0.05 for x in ov.values()) else 0)
        res["vis_starts_after_move_start"].append(1 if any(m in f and f[m][0] < v[0] for m in MOVE) else 0)
    return res, gap_names, len(frames)


def main():
    for a in sys.argv[1:]:
        k, p = a.split("=", 1)
        res, gap, n = analyse(p)
        print(f"== {k}: {len(res['vis_dur'])} frames")
        print(f"  표현 LOD 길이 중앙값 {statistics.median(res['vis_dur']):.2f}ms, 수집기 끝→표현 LOD 시작 중앙값 {statistics.median(res['vis_start_after_collector_end']):.2f}ms (그 사이 게임 스레드 일의 합 중앙값 {statistics.median(res['gap_work']):.2f}ms)")
        print("  그 사이에 도는 프로세서(프레임 비율, 평균 길이): " + "; ".join(f"{nm.replace('Processor','')} {len(v)/len(res['vis_dur']):.0%} {statistics.fmean(v):.2f}ms" for nm, v in sorted(gap.items(), key=lambda x: -len(x[1])) if len(v) / len(res['vis_dur']) > 0.05))
        print(f"  표현 LOD 시작 때 이미 이동 프로세서(Smooth/ApplyMovement)가 시작해 있는 프레임 {statistics.fmean(res['vis_starts_after_move_start']):.0%}; 구간이 겹치는 프레임 {statistics.fmean(res['overlaps_any_move']):.0%}, 겹침 길이 중앙값 {statistics.median(res['move_overlap_ms']):.2f}ms")


if __name__ == "__main__":
    main()
