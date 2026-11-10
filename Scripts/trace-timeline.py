"""trace-export.ps1 TimingEvents 결과(열 ThreadName,TimerName,StartTime,Duration,Depth)에서 프레임마다 PrePhysics 단계 기준 프로세서의 시작 시각·길이·스레드를 요약한다.
M4 병렬 트레이스 분석(Docs/Worklog/M4.md "병렬 트레이스 2회(P2)"). 기준 시각 = 게임 스레드의 `FMassProcessingPhase::ExecuteTick EMassProcessingPhase::PrePhysics` 시작.
출력: 프로세서마다 프레임 평균 시작 오프셋(ms), 길이(ms), 끝 오프셋(ms), 주로 도는 스레드 그룹(Game / Worker). 프로세서 타이머 이름 끝의 _<숫자>는 뗀다.
실행: py -3 -I Scripts/trace-timeline.py a=ev-a.csv b=ev-b.csv
"""
import csv
import re
import statistics
import sys
from collections import defaultdict

sys.dont_write_bytecode = True
PHASE = "FMassProcessingPhase::ExecuteTick EMassProcessingPhase::PrePhysics"


def load(path):
    rows = []
    with open(path, newline="", encoding="utf-8", errors="replace") as f:
        for r in csv.DictReader(f):
            rows.append((r["ThreadName"], re.sub(r"_\d{6,}$", "", r["TimerName"]), float(r["StartTime"]), float(r["Duration"])))
    phases = sorted(s for t, n, s, d in rows if t == "GameThread" and n == PHASE)
    ends = {s: s + d for t, n, s, d in rows if t == "GameThread" and n == PHASE}
    per = defaultdict(lambda: {"off": [], "dur": [], "end": [], "game": 0, "worker": 0})
    j = 0
    for t, n, s, d in sorted(rows, key=lambda r: r[2]):
        if not re.search(r"Processor$|^PrePhysics Group$", n) or n.startswith("UE::Mass"):
            continue
        while j + 1 < len(phases) and phases[j + 1] <= s:
            j += 1
        if not phases or s < phases[j] or s > ends[phases[j]] + 0.012:
            continue
        o = (s - phases[j]) * 1000
        e = per[n]
        e["off"].append(o); e["dur"].append(d * 1000); e["end"].append(o + d * 1000)
        e["game" if t == "GameThread" else "worker"] += 1
    phase_dur = [ends[p] - p for p in phases]
    return per, statistics.fmean(phase_dur) * 1000, len(phases)


def main():
    data = {}
    for a in sys.argv[1:]:
        k, p = a.split("=", 1)
        data[k] = load(p)
    print({k: f"phase {v[1]:.2f}ms frames {v[2]}" for k, v in data.items()})
    names = sorted({n for v in data.values() for n in v[0]}, key=lambda n: -statistics.fmean(next(iter(data.values()))[0].get(n, {"off": [0]})["off"] or [0]))
    print("processor | " + " | ".join(f"{k}: start / dur / end ms, thread G:W" for k in data))
    for n in names:
        cells = []
        for k, (per, pd, nf) in data.items():
            e = per.get(n)
            if not e or not e["off"]:
                cells.append("-")
            else:
                cells.append(f"{statistics.fmean(e['off']):.2f} / {statistics.fmean(e['dur']):.3f} / {statistics.fmean(e['end']):.2f}, {e['game']}:{e['worker']}")
        print(n + " | " + " | ".join(cells))


if __name__ == "__main__":
    main()
