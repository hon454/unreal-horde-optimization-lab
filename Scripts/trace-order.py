"""trace-timeline.py와 같은 입력(TimingEvents CSV)으로 프레임마다 MassVisualizationLODProcessor의 끝이 다른 프로세서의 끝·시작과 얼마나 가까운지 센다.
M4 병렬 트레이스 분석. 출력: 프레임 수, VisLOD 길이 중앙값, 'VisLOD 끝 - 프로세서 끝'이 0\~0.4ms 안에 드는 프레임 비율(프로세서마다).
실행: py -3 -I Scripts/trace-order.py ev.csv
"""
import csv
import re
import statistics
import sys

sys.dont_write_bytecode = True
PHASE = "FMassProcessingPhase::ExecuteTick EMassProcessingPhase::PrePhysics"
rows = []
with open(sys.argv[1], newline="", encoding="utf-8", errors="replace") as f:
    for r in csv.DictReader(f):
        rows.append((r["ThreadName"], re.sub(r"_\d{6,}$", "", r["TimerName"]), float(r["StartTime"]), float(r["Duration"])))
phases = sorted(s for t, n, s, d in rows if t == "GameThread" and n == PHASE)
by = {}
for t, n, s, d in rows:
    if n.endswith("Processor") and not n.startswith("UE::Mass"):
        by.setdefault(n, []).append((s, s + d))
for v in by.values():
    v.sort()
vis = by["MassVisualizationLODProcessor"]
others = ["MassSmoothOrientationProcessor", "MassApplyMovementProcessor", "MassApplyForceProcessor", "MassSteerToMoveTargetProcessor", "LabApproachProcessor", "LabVatISMProcessor", "MassLODCollectorProcessor"]
print("frames", len(vis), "VisLOD dur median ms", round(statistics.median((e - s) * 1000 for s, e in vis), 3))
for o in others:
    lst = by.get(o, [])
    near_end = near_start = 0
    for s, e in vis:
        # 같은 프레임의 짝: VisLOD 시작 이후 가장 가까운 o 하나가 아니라, VisLOD 시작 직전 20ms 안에 시작한 o
        c = [(os, oe) for os, oe in lst if s - 0.02 < os < s + 0.02]
        if not c:
            continue
        os, oe = min(c, key=lambda x: abs(x[0] - s))
        if 0 <= (e - oe) * 1000 <= 0.4:
            near_end += 1
        if 0 <= (os - e) * 1000 <= 0.4:
            near_start += 1
    print(f"{o}: VisLOD end within 0..0.4ms after its end: {near_end/len(vis):.0%};  it starts within 0..0.4ms after VisLOD end: {near_start/len(vis):.0%}")
