# Makes the SVG figures of post 02 in images/.
# Run from the repository root: py -3 -I Posts/02-mass-engine-path/make-figures.py
# Measured values come from the full batch of Test package de0e21b:
#   Measurements/Full4/summary.csv, equivalence.csv, Docs/figures/full4/full-n-at-60fps.csv
# Two kinds of values live outside the repository and are written below as constants.
# measurements.md gives the file, the column or timer and the method for each:
#   WAITS  - game thread wait columns of the frame CSVs Saved/Profiling/CSV/Full4-V0-s2-N10000-P1-r<1..3>.csv
#   TRACE  - Unreal Insights exports of Saved/Traces/Full4trace-V0-s1/-s2 and Full4traceser-V0-s2 (75-120 s)
#   FRAME  - one frame (game frame 3305) of Full4trace-V0-s2, TimingEvents export 100.0-100.1 s
# Trace runs are separate runs with tracing on. Their values explain where time goes and are not compared with
# summary.csv values.
import csv
import math
import os
import re
import statistics

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")
os.makedirs(OUT, exist_ok=True)


def read_csv(*parts):
    with open(os.path.join(ROOT, *parts), encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


SUMMARY = {(r["variant"], int(r["n"]), r["preset"]): r for r in read_csv("Measurements", "Full4", "summary.csv")}
EQUIV = {(r["cond"], int(r["n"])): r for r in read_csv("Measurements", "Full4", "equivalence.csv")}
NAT = {r["series"]: r for r in read_csv("Docs", "figures", "full4", "full-n-at-60fps.csv")}


def v(variant, col, n=10000, preset="P1"):
    return float(SUMMARY[(variant, n, preset)][col])


def n_at_60(step):
    """Max N of a ladder step from the batch table (Scripts/plot.py n_at_budget, log interpolation)."""
    for name, r in NAT.items():
        if name.startswith(f"{step} "):
            return int(r["n_p95"])
    raise KeyError(step)


# Per-frame medians of each run (r1, r2, r3), ms. Median of the three is drawn. See measurements.md "프레임 CSV의 기다림 열".
WAITS = {
    "gt_waits_all": (16.236, 16.218, 15.998),   # sum of the Exclusive/GameThread/EventWait/* columns
    "gt_wait_prephysics": (14.509, 14.518, 14.293),  # Exclusive/GameThread/EventWait/PrePhysics
}
# Per-frame averages over 70-120 s of the trace runs (the window and method of post 03).
# Frames = FEngineLoop::Tick count in the window. See measurements.md "트레이스에서 읽은 값".
TRACE = {
    "s1_cmc_ticks": 9982.7,      # UCharacterMovementComponent_TickComponent count 928,391 / 93 frames
    "s1_mesh_ticks": 9906.1,     # USkinnedMeshComponent_TickComponent count 921,266 / 93 frames
    "s1_comp_ticks": 19893.7,    # FActorComponentTickFunction::ExecuteTick count 1,850,117 / 93 frames
    "s2_mesh_ticks": 972.6,      # USkinnedMeshComponent_TickComponent count 1,482,294 / 1,524 frames
    "s2_comp_ticks": 977.6,      # FActorComponentTickFunction::ExecuteTick count 1,489,909 / 1,524 frames
    "s1_cmc_share": 76.4,        # CharacterMovement Excl 410.73 ms / FEngineLoop::Tick 537.63 ms per frame, %
    "s2_avoid_ms": 24.727,       # mean length of the 1,523 MassMovingAvoidanceProcessor runs (TimingEvents)
    "s2_gt_wait_ms": 15.066,     # game thread WaitForTasks Incl 22.960 s / 1,524 frames
}
# Game frame 3305 of Full4trace-V0-s2, ms from the frame start (100.009724 s). See measurements.md "한 프레임".
FRAME = {
    "end": 34.72,
    "gt_mass": (0.66, 4.49),        # MassProcessingQueue Main-Thread Runner Task (game thread processors)
    "gt_waits": [(10.93, 29.38), (29.82, 31.10)],  # WaitForTasks
    "avoid": (1.57, 27.17),         # MassMovingAvoidanceProcessor on Background Worker #15
    "standing": (27.19, 28.82),     # MassStandingAvoidanceProcessor, same worker
    "move": (28.83, 29.12),         # MassApplyForceProcessor + MassApplyMovementProcessor, another worker
}

BUDGET_MS = 16.7      # Scripts/plot.py BUDGET_MS, the 60 fps line

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, LIGHTBLUE, GRAY, AMBER, RED, TEAL, PINK, DARK, MID = (
    "#2563eb", "#93c5fd", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151", "#6b7280")
W = 800
HALO = 'paint-order="stroke" stroke="white" stroke-width="5" stroke-linejoin="round"'


def svg(h, body, defs=""):
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">']
    if defs:
        head.append(f"<defs>{defs}</defs>")
    head.append(f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>')
    return "\n".join(head + body + ["</svg>"]) + "\n"


def write(name, text):
    with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def _no_zero_decimals(f):
    """27.0 -> 27. A value whose decimals are all zero is written as an integer (posting.md)."""
    return lambda *a, **k: re.sub(r"[.]0+$", "", f(*a, **k))


@_no_zero_decimals
def fmt(x):
    """Three significant figures, as in the post body."""
    if abs(x) >= 100:
        return f"{x:.0f}"
    if abs(x) >= 10:
        return f"{x:.1f}"
    return f"{x:.2f}"


def count(x):
    """Counts are not shortened: 9,936."""
    return f"{round(x):,}"


def title(o, t, s, s2=None):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')
    if s2:
        o.append(f'<text x="32" y="98" font-size="16" fill="{SUB}">{s2}</text>')


def footer(o, y, lines):
    o.append(f'<line x1="32" y1="{y - 22}" x2="{W - 32}" y2="{y - 22}" stroke="{LINE}"/>')
    for k, t in enumerate(lines):
        o.append(f'<text x="32" y="{y + 26 * k}" font-size="16" fill="{SUB}">{t}</text>')
    return y + 26 * len(lines)


DEFS = (f'<pattern id="hatch" width="8" height="8" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
        f'<rect width="8" height="8" fill="#dbeafe"/><line x1="0" y1="0" x2="0" y2="8" stroke="{BLUE}" stroke-width="3"/></pattern>'
        f'<pattern id="hatchgray" width="8" height="8" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
        f'<rect width="8" height="8" fill="#f3f4f6"/><line x1="0" y1="0" x2="0" y2="8" stroke="{GRAY}" stroke-width="3"/></pattern>'
        f'<marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
        f'<path d="M0 0 L10 5 L0 10 z" fill="{DARK}"/></marker>')


# ---------- 1. summary: step 1 and step 2 at 10,000, log axis ----------
def summary():
    o = []
    title(o, "단계 1과 단계 2의 프레임 시간과 스레드 시간",
          "호드 총수 1만, 이동 장면, 오브 시점 카메라. 세 번 잰 중앙값(ms)",
          "가로축은 로그 눈금이다")
    items = [("프레임 p95", "frame_p95_ms"), ("프레임 p50", "frame_p50_ms"),
             ("게임 스레드", "game_p50_ms"), ("렌더 스레드", "render_p50_ms"), ("GPU", "gpu_p50_ms")]
    steps = [("V0-s1", "단계 1 액터 기준선", RED), ("V0-s2", "단계 2 Mass, 엔진 기본 경로", BLUE)]
    X0, X1, LO, HI = 180, 690, 1.0, 1000.0
    X = lambda ms: X0 + (math.log10(ms) - math.log10(LO)) / (math.log10(HI) - math.log10(LO)) * (X1 - X0)
    top, BH, GAP, RG = 190, 20, 4, 22
    rows = len(items)
    bottom = top + rows * (2 * BH + GAP + RG) - RG
    # legend
    lx = 32
    for var, name, col in steps:
        o.append(f'<rect x="{lx}" y="118" width="18" height="14" rx="3" fill="{col}"/>')
        o.append(f'<text x="{lx + 26}" y="131" font-size="16" font-weight="700" fill="{col}">{name}</text>')
        lx += 300
    for t in [1, 10, 100, 1000]:
        o.append(f'<line x1="{X(t):.1f}" y1="{top - 10}" x2="{X(t):.1f}" y2="{bottom + 6}" stroke="{LINE}"/>')
        o.append(f'<text x="{X(t):.1f}" y="{bottom + 28}" text-anchor="middle" font-size="16" fill="{SUB}">{t:,}</text>')
    xb = X(BUDGET_MS)
    o.append(f'<line x1="{xb:.1f}" y1="{top - 18}" x2="{xb:.1f}" y2="{bottom + 6}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="6 4"/>')
    o.append(f'<text x="{xb:.1f}" y="{top - 24}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    y = top
    for label, col in items:
        o.append(f'<text x="32" y="{y + BH + 8}" font-size="17" font-weight="700">{label}</text>')
        for k, (var, name, colour) in enumerate(steps):
            val = v(var, col)
            yy = y + k * (BH + GAP)
            o.append(f'<rect x="{X0}" y="{yy}" width="{X(val) - X0:.1f}" height="{BH}" rx="3" fill="{colour}"/>')
            o.append(f'<text x="{X(val) + 8:.1f}" y="{yy + 16}" font-size="16" font-weight="700" fill="{colour}" {HALO}>{fmt(val)}</text>')
        y += 2 * BH + GAP + RG
    yf = bottom + 70
    yf = footer(o, yf, ["단계 2의 프레임 p50은 게임 스레드, 렌더 스레드, GPU 어느 쪽보다도 길다.",
                        "단계 1은 셋 가운데 게임 스레드가 가장 길고, 프레임은 게임 스레드 시간보다 16ms쯤 길다."])
    write("summary-threads.svg", svg(yf, o, DEFS))


# ---------- 2. principle: what each actor did and who does it in step 2 ----------
def what_changed():
    o = []
    title(o, "액터 1만이 하던 일을 단계 2에서는 누가 하나",
          "프레임당 횟수는 트레이스 실행의 평균이다(측정 실행과 따로 뜬 실행)")
    LX, RX, BW = 32, 420, 348
    o.append(f'<text x="{LX}" y="128" font-size="18" font-weight="700" fill="{RED}">단계 1: 액터 1만</text>')
    o.append(f'<text x="{RX}" y="128" font-size="18" font-weight="700" fill="{BLUE}">단계 2: 엔티티 1만</text>')
    rows = [
        ("이동과 겹침 방지",
         ["액터마다 CharacterMovement 틱", f"프레임당 {count(TRACE['s1_cmc_ticks'])}번", "캡슐끼리 막고 스윕으로 미끄러진다"],
         ["엔진 프로세서가 Chunk를 돈다", "조향·힘·이동·방향 보간", "겹침 방지는 Mass Avoidance 프로세서"]),
        ("그리기와 애니메이션",
         ["액터마다 스켈레탈 메시와 AnimBP", f"메시 틱 프레임당 {count(TRACE['s1_mesh_ticks'])}번", ""],
         ["35m 안만 액터(상한 1,000)", f"메시 틱 프레임당 {count(TRACE['s2_mesh_ticks'])}번", "35~200m는 정지 메시 인스턴스"]),
        ("액터의 위치",
         ["액터가 스스로 옮긴다", "", ""],
         ["CharacterMovement 틱 없음", "Lab 동기화 프로세서가 넣는다", ""]),
    ]
    y = 148
    for head, left, right in rows:
        o.append(f'<text x="{LX}" y="{y + 20}" font-size="16" font-weight="700" fill="{SUB}">{head}</text>')
        y += 30
        lines = max(len([t for t in left if t]), len([t for t in right if t]))
        h = 22 + 24 * lines
        for x, texts, col, fill in ((LX, left, RED, "#fef2f2"), (RX, right, BLUE, "#eff6ff")):
            o.append(f'<rect x="{x}" y="{y}" width="{BW}" height="{h}" rx="8" fill="{fill}" stroke="{col}" stroke-width="1.5"/>')
            ty = y + 26
            for k, t in enumerate(texts):
                if not t:
                    continue
                weight = ' font-weight="700"' if k == 0 else ""
                o.append(f'<text x="{x + 14}" y="{ty}" font-size="16"{weight}>{t}</text>')
                ty += 24
        o.append(f'<line x1="{LX + BW + 6}" y1="{y + h / 2}" x2="{RX - 8}" y2="{y + h / 2}" stroke="{DARK}" stroke-width="2" marker-end="url(#arrow)"/>')
        y += h + 14
    yf = y + 48
    yf = footer(o, yf, [f"컴포넌트 틱은 프레임당 {count(TRACE['s1_comp_ticks'])}번에서 {count(TRACE['s2_comp_ticks'])}번이 됐다.",
                        "200m 밖 엔티티는 그리지 않는다."])
    write("what-changed.svg", svg(yf, o, DEFS))


# ---------- 3. the frame is the game thread time plus the time the game thread waited ----------
def frame_vs_threads():
    o = []
    title(o, "단계 2의 프레임 = 게임 스레드 시간 + 게임 스레드가 기다린 시간",
          "호드 총수 1만, 이동 장면, 오브 시점 카메라. 세 번 잰 중앙값(ms)")
    s = "V0-s2"
    frame, gt, rt, gpu = v(s, "frame_p50_ms"), v(s, "game_p50_ms"), v(s, "render_p50_ms"), v(s, "gpu_p50_ms")
    wait = statistics.median(WAITS["gt_waits_all"])
    pre = statistics.median(WAITS["gt_wait_prephysics"])
    X0, X1, XMAX = 200, 720, 40.0
    X = lambda ms: X0 + ms / XMAX * (X1 - X0)
    top, RH, BH = 150, 66, 30
    bottom = top + RH * 4 - (RH - BH)
    for t in [0, 10, 20, 30, 40]:
        o.append(f'<line x1="{X(t):.1f}" y1="{top - 12}" x2="{X(t):.1f}" y2="{bottom + 8}" stroke="{LINE}"/>')
        o.append(f'<text x="{X(t):.1f}" y="{bottom + 30}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')
    xb = X(BUDGET_MS)
    o.append(f'<line x1="{xb:.1f}" y1="{top - 30}" x2="{xb:.1f}" y2="{bottom + 8}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="6 4"/>')
    o.append(f'<text x="{xb:.1f}" y="{top - 36}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    y = top
    # frame p50
    o.append(f'<text x="32" y="{y + 21}" font-size="17" font-weight="700">프레임 p50</text>')
    o.append(f'<rect x="{X0}" y="{y}" width="{X(frame) - X0:.1f}" height="{BH}" rx="4" fill="{DARK}"/>')
    o.append(f'<text x="{X(frame) + 8:.1f}" y="{y + 21}" font-size="16" font-weight="700" fill="{DARK}">{fmt(frame)}</text>')
    y += RH
    # game thread + waits
    o.append(f'<text x="32" y="{y + 21}" font-size="17" font-weight="700">게임 스레드</text>')
    o.append(f'<rect x="{X0}" y="{y}" width="{X(gt) - X0:.1f}" height="{BH}" rx="4" fill="{BLUE}"/>')
    o.append(f'<text x="{(X0 + X(gt)) / 2:.1f}" y="{y + 21}" text-anchor="middle" font-size="16" font-weight="700" fill="white">시간 {fmt(gt)}</text>')
    o.append(f'<rect x="{X(gt):.1f}" y="{y}" width="{X(gt + wait) - X(gt):.1f}" height="{BH}" rx="4" fill="url(#hatch)" stroke="{BLUE}" stroke-width="1.5"/>')
    o.append(f'<text x="{(X(gt) + X(gt + wait)) / 2:.1f}" y="{y + 21}" text-anchor="middle" font-size="16" font-weight="700" fill="{INK}" {HALO}>기다림 {fmt(wait)}</text>')
    o.append(f'<text x="{X(gt + wait) + 8:.1f}" y="{y + 21}" font-size="16" font-weight="700" fill="{BLUE}">{fmt(gt + wait)}</text>')
    y += RH
    for label, val, col in (("렌더 스레드", rt, TEAL), ("GPU", gpu, PINK)):
        o.append(f'<text x="32" y="{y + 21}" font-size="17" font-weight="700">{label}</text>')
        o.append(f'<rect x="{X0}" y="{y}" width="{X(val) - X0:.1f}" height="{BH}" rx="4" fill="{col}"/>')
        o.append(f'<text x="{X(val) + 8:.1f}" y="{y + 21}" font-size="16" font-weight="700" fill="{col}">{fmt(val)}</text>')
        y += RH
    yf = bottom + 74
    yf = footer(o, yf, [f"기다림은 프레임 CSV의 게임 스레드 EventWait 열의 합이다. 그중 {fmt(pre)}ms는",
                        "Mass 처리가 도는 틱 그룹 PrePhysics에서 생긴다. 게임 스레드 시간은 기다림을 뺀 값이다.",
                        "프레임마다 프레임 시간에서 두 값을 빼면 남는 값의 중앙값은 0.03ms보다 작다."])
    write("frame-vs-threads.svg", svg(yf, o, DEFS))


# ---------- 4. one frame of the trace: the game thread waits for the avoidance on one worker ----------
def one_frame():
    o = []
    title(o, "트레이스의 한 프레임: 게임 스레드가 Mass Avoidance를 기다린다",
          "단계 2, 호드 총수 1만, 이동 장면. 트레이스 실행의 한 프레임(측정값과 견주지 않는다)")
    X0, X1, XMAX = 170, 760, 35.0
    X = lambda ms: X0 + ms / XMAX * (X1 - X0)
    top, LH, BH = 160, 92, 40
    lanes_bottom = top + 2 * LH - (LH - BH)
    for t in [0, 5, 10, 15, 20, 25, 30, 35]:
        o.append(f'<line x1="{X(t):.1f}" y1="{top - 14}" x2="{X(t):.1f}" y2="{lanes_bottom + 8}" stroke="{LINE}"/>')
        o.append(f'<text x="{X(t):.1f}" y="{lanes_bottom + 30}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')
    o.append(f'<text x="{X1}" y="{lanes_bottom + 54}" text-anchor="end" font-size="16" fill="{SUB}">프레임 시작부터 ms</text>')
    end = FRAME["end"]
    o.append(f'<line x1="{X(end):.1f}" y1="{top - 30}" x2="{X(end):.1f}" y2="{lanes_bottom + 8}" stroke="{DARK}" stroke-width="2"/>')
    o.append(f'<text x="{X(end):.1f}" y="{top - 36}" text-anchor="end" font-size="16" font-weight="700" fill="{DARK}">프레임 끝 {fmt(end)}</text>')
    # game thread lane: work everywhere except the waits
    y = top
    o.append(f'<text x="32" y="{y + 26}" font-size="17" font-weight="700" fill="{BLUE}">게임 스레드</text>')
    o.append(f'<rect x="{X(0):.1f}" y="{y}" width="{X(end) - X(0):.1f}" height="{BH}" rx="4" fill="{BLUE}"/>')
    for a, b in FRAME["gt_waits"]:
        o.append(f'<rect x="{X(a):.1f}" y="{y}" width="{X(b) - X(a):.1f}" height="{BH}" fill="url(#hatch)" stroke="{BLUE}" stroke-width="1"/>')
    a, b = FRAME["gt_waits"][0]
    o.append(f'<text x="{(X(a) + X(b)) / 2:.1f}" y="{y + 26}" text-anchor="middle" font-size="16" font-weight="700" fill="{INK}" {HALO}>기다림 {fmt(b - a)}</text>')
    ma, mb = FRAME["gt_mass"]
    o.append(f'<path d="M{X(ma):.1f} {y - 6} L{X(ma):.1f} {y - 12} L{X(mb):.1f} {y - 12} L{X(mb):.1f} {y - 6}" fill="none" stroke="{DARK}" stroke-width="1.5"/>')
    o.append(f'<text x="{X(ma):.1f}" y="{y - 18}" font-size="16" fill="{DARK}">Mass 게임 스레드 일</text>')
    # worker lane
    y += LH
    o.append(f'<text x="32" y="{y + 18}" font-size="17" font-weight="700" fill="{PINK}">작업자</text>')
    o.append(f'<text x="32" y="{y + 38}" font-size="17" font-weight="700" fill="{PINK}">스레드 하나</text>')
    a, b = FRAME["avoid"]
    o.append(f'<rect x="{X(a):.1f}" y="{y}" width="{X(b) - X(a):.1f}" height="{BH}" rx="4" fill="{PINK}"/>')
    o.append(f'<text x="{(X(a) + X(b)) / 2:.1f}" y="{y + 26}" text-anchor="middle" font-size="16" font-weight="700" fill="white">엔진 이동 회피 {fmt(b - a)}</text>')
    a, b = FRAME["standing"]
    o.append(f'<rect x="{X(a):.1f}" y="{y}" width="{X(b) - X(a):.1f}" height="{BH}" rx="3" fill="#f9a8d4"/>')
    gb = FRAME["gt_waits"][0][1]
    o.append(f'<line x1="{X(gb):.1f}" y1="{top + BH}" x2="{X(gb):.1f}" y2="{y}" stroke="{DARK}" stroke-width="1.5" stroke-dasharray="4 3"/>')
    yl = lanes_bottom + 92
    o.append(f'<rect x="32" y="{yl - 15}" width="34" height="20" rx="3" fill="url(#hatch)" stroke="{BLUE}"/>')
    o.append(f'<text x="76" y="{yl}" font-size="16">게임 스레드가 Mass 작업을 기다린 구간(WaitForTasks)</text>')
    o.append(f'<rect x="32" y="{yl + 13}" width="34" height="20" rx="3" fill="#f9a8d4"/>')
    o.append(f'<text x="76" y="{yl + 28}" font-size="16">회피 뒤에 같은 스레드가 도는 서 있는 회피</text>')
    yf = yl + 86
    yf = footer(o, yf, ["회피가 끝나야 힘 적용과 이동이 돌고, 그 뒤에 게임 스레드가 다음 일로 간다.",
                        f"트레이스 50초의 평균으로 회피는 한 번에 {fmt(TRACE['s2_avoid_ms'])}ms,"
                        f" 게임 스레드의 기다림은 프레임마다 {fmt(TRACE['s2_gt_wait_ms'])}ms다."])
    write("one-frame.svg", svg(yf, o, DEFS))


# ---------- 5. equivalence: step 2 does not do the same work ----------
def same_work():
    o = []
    title(o, "동등성 검사: 단계 2는 개체가 오브 둘레에 뭉친다",
          "호드 총수 1만, 이동 장면, 오브 시점 카메라. 세 번 잰 중앙값",
          "기준은 단계 3~8의 중앙값이다. 검사는 기준과 1% 안인지 본다")
    r = EQUIV[("V0-s2", 10000)]
    items = [("초당 사망", float(r["death_rate"]), float(r["death_rate_ref"]), float(r["death_rate_dev_pct"]), "", fmt),
             ("오브 30m 안 개체 수", float(r["near30_mean"]), float(r["near30_mean_ref"]), float(r["near30_mean_dev_pct"]), "", count)]
    X0, X1, PMAX = 250, 690, 240.0
    X = lambda pct: X0 + pct / PMAX * (X1 - X0)
    top = 150
    y = top
    for label, val, ref, dev, unit, f in items:
        o.append(f'<text x="32" y="{y + 20}" font-size="17" font-weight="700">{label}</text>')
        y += 34
        for name, value, col in (("기준(단계 3~8)", ref, GRAY), ("단계 2", val, BLUE)):
            pct = value / ref * 100
            o.append(f'<text x="48" y="{y + 20}" font-size="16">{name}</text>')
            o.append(f'<rect x="{X0}" y="{y}" width="{X(pct) - X0:.1f}" height="28" rx="4" fill="{col}"/>')
            # labels of bars shorter than the reference start right of the 1% band so the line does not cross them
            o.append(f'<text x="{max(X(pct), X(101)) + 8:.1f}" y="{y + 20}" font-size="16" font-weight="700" fill="{DARK if col == GRAY else BLUE}">{f(value)}</text>')
            y += 38
        sign = "+" if dev >= 0 else "−"
        o.append(f'<text x="48" y="{y + 16}" font-size="16" fill="{RED}" font-weight="700">기준과 {sign}{fmt(abs(dev))}% → 1%를 넘는다</text>')
        y += 46
    # 100% line and +-1% band
    o.append(f'<rect x="{X(99):.1f}" y="{top + 26}" width="{X(101) - X(99):.1f}" height="{y - top - 40}" fill="{AMBER}" fill-opacity="0.35"/>')
    o.append(f'<line x1="{X(100):.1f}" y1="{top + 26}" x2="{X(100):.1f}" y2="{y - 14}" stroke="{AMBER}" stroke-width="1.5" stroke-dasharray="5 4"/>')
    o.append(f'<text x="{X(100):.1f}" y="{top + 16}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">기준 = 100%</text>')
    yf = y + 40
    yf = footer(o, yf, ["막대의 길이는 기준에 대한 비율이다. 단계 2는 같은 일을 하지 못해 검사가 값만 보고한다."])
    write("same-work.svg", svg(yf, o, DEFS))


# ---------- 6. step 2 frame time by horde size and the max N ----------
def max_n():
    o = []
    title(o, "단계 2의 호드 총수에 따른 프레임 시간",
          "이동 장면, 오브 시점 카메라, 세 번 잰 중앙값. 가로축은 로그 눈금")
    ns = [2500, 5000, 10000]
    series = [("frame_p95_ms", "프레임 p95", DARK, 2.5, ""),
              ("frame_p50_ms", "프레임 p50", MID, 2.0, "6 4"),
              ("game_p50_ms", "게임 스레드 시간", BLUE, 2.5, "")]
    L, R, T, B = 90, 560, 130, 460
    NLO, NHI, YMAX = 2000, 12500, 40.0
    X = lambda n: L + (math.log(n) - math.log(NLO)) / (math.log(NHI) - math.log(NLO)) * (R - L)
    Y = lambda ms: B - ms / YMAX * (B - T)
    for t in [0, 10, 20, 30, 40]:
        o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
        o.append(f'<text x="{L - 10}" y="{Y(t) + 6:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{t}</text>')
    o.append(f'<text x="32" y="{T - 16}" font-size="16" fill="{SUB}">ms</text>')
    for n, lab in [(2500, "2,500"), (5000, "5천"), (10000, "1만")]:
        o.append(f'<line x1="{X(n):.1f}" y1="{B}" x2="{X(n):.1f}" y2="{B + 5}" stroke="{SUB}"/>')
        o.append(f'<text x="{X(n):.1f}" y="{B + 26}" text-anchor="middle" font-size="16">{lab}</text>')
    o.append(f'<line x1="{L}" y1="{B}" x2="{R}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<line x1="{L}" y1="{T}" x2="{L}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<text x="{(L + R) / 2}" y="{B + 52}" text-anchor="middle" font-size="16" fill="{SUB}">호드 총수</text>')
    y167 = Y(BUDGET_MS)
    o.append(f'<line x1="{L}" y1="{y167:.1f}" x2="{R}" y2="{y167:.1f}" stroke="{AMBER}" stroke-dasharray="6 4" stroke-width="1.5"/>')
    o.append(f'<text x="{L + 8}" y="{y167 - 8:.1f}" font-size="16" font-weight="700" fill="{AMBER}" {HALO}>16.7ms(60fps)</text>')
    # label offsets (dy) for the points left of 1만; at 1만 every label goes to the right of its point
    offsets = {"frame_p95_ms": -10, "frame_p50_ms": 22, "game_p50_ms": 22}
    for col, name, colour, width, dash in series:
        pts = [(n, v("V0-s2", col, n)) for n in ns]
        d = f' stroke-dasharray="{dash}"' if dash else ""
        o.append(f'<polyline points="{" ".join(f"{X(n):.1f},{Y(m):.1f}" for n, m in pts)}" fill="none" stroke="{colour}" stroke-width="{width}"{d}/>')
        for n, m in pts:
            o.append(f'<circle cx="{X(n):.1f}" cy="{Y(m):.1f}" r="5" fill="{colour}"/>')
            dy = 6 if n == 10000 else offsets[col]
            o.append(f'<text x="{X(n) + 10:.1f}" y="{Y(m) + dy:.1f}" font-size="16" font-weight="700" fill="{colour}" {HALO}>{fmt(m)}</text>')
    nx = X(n_at_60(2))
    o.append(f'<line x1="{nx:.1f}" y1="{y167:.1f}" x2="{nx:.1f}" y2="{B}" stroke="{DARK}" stroke-width="1.5" stroke-dasharray="3 3"/>')
    o.append(f'<circle cx="{nx:.1f}" cy="{y167:.1f}" r="6" fill="white" stroke="{DARK}" stroke-width="3"/>')
    o.append(f'<text x="{nx + 6:.1f}" y="{B - 10}" font-size="16" font-weight="700" fill="{DARK}" {HALO}>{n_at_60(2):,}</text>')
    lx, ly = 584, 150
    for col, name, colour, width, dash in series:
        d = f' stroke-dasharray="{dash}"' if dash else ""
        o.append(f'<line x1="{lx}" y1="{ly}" x2="{lx + 28}" y2="{ly}" stroke="{colour}" stroke-width="{width}"{d}/>')
        o.append(f'<text x="{lx + 36}" y="{ly + 6}" font-size="16" font-weight="700" fill="{colour}">{name}</text>')
        ly += 34
    ly += 10
    o.append(f'<circle cx="{lx + 14}" cy="{ly}" r="6" fill="white" stroke="{DARK}" stroke-width="3"/>')
    o.append(f'<text x="{lx + 36}" y="{ly + 6}" font-size="16">최대 N:</text>'
             f'<text x="{lx + 36}" y="{ly + 28}" font-size="16">프레임 p95가</text>'
             f'<text x="{lx + 36}" y="{ly + 50}" font-size="16">16.7ms를 넘기</text>'
             f'<text x="{lx + 36}" y="{ly + 72}" font-size="16">직전의 총수</text>')
    yf = B + 96
    yf = footer(o, yf, ["모든 총수에서 프레임이 게임 스레드 시간보다 길고, 그 차이는 총수와 함께 커진다.",
                        "최대 N은 16.7ms를 사이에 둔 2,500과 5천 사이를 로그 보간한 값이다."])
    write("step2-max-n.svg", svg(yf, o, DEFS))


summary()
what_changed()
frame_vs_threads()
one_frame()
same_work()
max_n()
print("wrote", sorted(os.listdir(OUT)))
