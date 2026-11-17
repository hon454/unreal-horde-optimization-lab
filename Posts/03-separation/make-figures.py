# Makes the SVG figures of post 03 in images/.
# Run from the repository root: py -3 -I Posts/03-separation/make-figures.py
# Measured values come from Measurements/Full4/summary.csv and equivalence.csv (Test package de0e21b).
# Trace values are constants below: they were exported with Scripts/trace-export.ps1 from the traces of the
# same batch (Saved/Traces/Full4trace*-V0-s2/-s3, outside git). The window and the formulas are in measurements.md.
# The principle diagram draws configuration values from the engine and project source only.
import csv
import math
import os
import random
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")
os.makedirs(OUT, exist_ok=True)
BATCH = os.path.join(ROOT, "Measurements", "Full4")

rows = {}
with open(os.path.join(BATCH, "summary.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        rows[(r["variant"], int(r["n"]), r["preset"])] = r

equiv = {}
with open(os.path.join(BATCH, "equivalence.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        equiv[(r["cond"], int(r["n"]))] = r


def v(variant, col, n=10000, preset="P1"):
    return float(rows[(variant, n, preset)][col])


def e(cond, col, n=10000):
    return equiv[(cond, n)][col]


BUDGET_MS = 16.7  # Scripts/plot.py BUDGET_MS, the 60 fps line
SAME_MS = 0.5     # measurement.md "차이 판정": differences under 0.5 ms are not told apart in this batch

# Trace values (ms per frame, 70-120 s of each trace). Source rows are in measurements.md "트레이스 값".
TRACE = {
    "s2par_frame": 32.824,      # Full4trace-V0-s2: mean of the 1,524 "Game Frames" events
    "s2par_avoid": 24.727,      # same trace: mean of the 1,523 MassMovingAvoidanceProcessor executions, one worker each
    "s2par_gt_wait": 15.066,    # same trace: GameThread WaitForTasks Incl 22.960 s / 1,524 frames
    "s2ser_avoid": 23.486,      # Full4traceser-V0-s2: MassMovingAvoidanceProcessor Incl 28.793 s / 1,226
    "s3ser_sep": 0.824,         # Full4traceser-V0-s3: LabSeparationProcessor Incl 3.078 s / 3,737
    "s2ser_orient": 0.821,      # Full4traceser-V0-s2: MassSmoothOrientationProcessor Incl 1.007 s / 1,226
    "s3ser_orient": 0.223,      # Full4traceser-V0-s3: LabSmoothOrientationProcessor Incl 0.833 s / 3,738
}
TRACE_AVOID_RUNS = 1523  # Full4trace-V0-s2, 70-120 s: avoidance executions; no two overlap in time

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, LIGHTBLUE, GRAY, AMBER, RED, TEAL, PINK, DARK, MID = (
    "#2563eb", "#93c5fd", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151", "#6b7280")
S2, S3 = GRAY, BLUE  # step 2 and step 3 in every figure
W = 800
HALO = 'paint-order="stroke" stroke="white" stroke-width="5" stroke-linejoin="round"'  # keeps value labels readable over grid and budget lines


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
    """Counts are not shortened: 12441.87 -> 12,442."""
    return f"{round(x):,}"


def signed(x):
    return ("+" if x >= 0 else "−") + f"{abs(x):.2f}"


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


def legend(o, x, y, items):
    for colour, label in items:
        o.append(f'<rect x="{x}" y="{y - 13}" width="18" height="16" rx="3" fill="{colour}"/>')
        o.append(f'<text x="{x + 26}" y="{y}" font-size="16">{label}</text>')
        x += 40 + 16 * len(label)


ARROW = (f'<marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{DARK}"/></marker>'
         f'<marker id="arrowr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{RED}"/></marker>'
         f'<marker id="arrowb" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{BLUE}"/></marker>')


# ---------- 1. summary: step 2 and step 3, frame and thread times ----------
def summary():
    o = []
    title(o, "단계 2와 단계 3의 프레임 시간과 스레드 시간",
          "호드 총수 1만, 이동 장면, 오브 시점 카메라. 세 번 잰 중앙값(ms)")
    legend(o, 32, 112, [(S2, "단계 2 엔진 회피"), (S3, "단계 3 자체 밀어내기")])
    items = [("프레임 p95", "frame_p95_ms", True), ("프레임 p50", "frame_p50_ms", True),
             ("게임 스레드", "game_p50_ms", False), ("렌더 스레드", "render_p50_ms", False), ("GPU", "gpu_p50_ms", False)]
    X0, X1, XMAX = 170, 700, 40.0
    X = lambda ms: X0 + ms / XMAX * (X1 - X0)
    top, BH, GAP, RG = 170, 22, 4, 30
    rowh = 2 * BH + GAP + RG
    bottom = top + rowh * len(items) - RG
    for t in [0, 10, 20, 30, 40]:
        o.append(f'<line x1="{X(t):.1f}" y1="{top - 10}" x2="{X(t):.1f}" y2="{bottom + 6}" stroke="{LINE}"/>')
        o.append(f'<text x="{X(t):.1f}" y="{bottom + 28}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')
    xb = X(BUDGET_MS)
    o.append(f'<line x1="{xb:.1f}" y1="{top - 26}" x2="{xb:.1f}" y2="{bottom + 6}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="6 4"/>')
    o.append(f'<text x="{xb + 6:.1f}" y="{top - 30}" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    y = top
    for label, col, ranged in items:
        o.append(f'<text x="32" y="{y + BH + 8}" font-size="17" font-weight="700">{label}</text>')
        for k, (var, colour) in enumerate((("V0-s2", S2), ("V0-s3", S3))):
            yy = y + k * (BH + GAP)
            val = v(var, col)
            o.append(f'<rect x="{X0}" y="{yy}" width="{X(val) - X0:.1f}" height="{BH}" rx="4" fill="{colour}"/>')
            end = X(val)
            if ranged:
                mn, mx = v(var, col + "_min"), v(var, col + "_max")
                cy = yy + BH / 2
                o.append(f'<line x1="{X(mn):.1f}" y1="{cy}" x2="{X(mx):.1f}" y2="{cy}" stroke="{INK}" stroke-width="2"/>')
                for b in (mn, mx):
                    o.append(f'<line x1="{X(b):.1f}" y1="{cy - 7}" x2="{X(b):.1f}" y2="{cy + 7}" stroke="{INK}" stroke-width="2"/>')
                end = X(mx)
            tc = DARK if colour == S2 else BLUE
            o.append(f'<text x="{end + 8:.1f}" y="{yy + 17}" font-size="16" font-weight="700" fill="{tc}" {HALO}>{fmt(val)}</text>')
        y += rowh
    p95 = v("V0-s3", "frame_p95_ms")
    yf = bottom + 74
    yend = footer(o, yf, [f"단계 3의 프레임 p95는 {fmt(p95)}ms다. 16.7ms까지의 여유는 {fmt(BUDGET_MS - p95)}ms다.",
                          "가는 선은 세 실행의 최솟값–최댓값이다. 스레드 시간은 중앙값만 쓴다.",
                          "단계 2는 프레임 시간이 세 스레드 시간보다 길다. 까닭은 글 02가 다룬다."])
    write("summary.svg", svg(yend + 8, o))


# ---------- 2. step 2 trace: one worker runs the engine avoidance ----------
def step2_trace():
    o = []
    title(o, "단계 2의 엔진 회피는 작업자 스레드 하나에서 돈다",
          "호드 총수 1만, 이동 장면, 오브 시점 카메라. 트레이스 50초의 프레임 평균(ms)",
          "트레이스 실행의 값이라 측정값과 견주지 않는다")
    X0, X1, XMAX = 300, 700, 42.0
    X = lambda ms: X0 + ms / XMAX * (X1 - X0)
    BH = 26

    def bar(y, label, sub, val, colour):
        o.append(f'<text x="32" y="{y + 18}" font-size="16" font-weight="700">{label}</text>')
        if sub:
            o.append(f'<text x="32" y="{y + 40}" font-size="16" fill="{SUB}">{sub}</text>')
        o.append(f'<rect x="{X0}" y="{y}" width="{X(val) - X0:.1f}" height="{BH}" rx="4" fill="{colour}"/>')
        o.append(f'<text x="{X(val) + 8:.1f}" y="{y + 19}" font-size="16" font-weight="700" fill="{colour if colour != S2 else DARK}">{fmt(val)}</text>')

    def axis(top, bottom):
        for t in [0, 10, 20, 30, 40]:
            o.append(f'<line x1="{X(t):.1f}" y1="{top - 8}" x2="{X(t):.1f}" y2="{bottom}" stroke="{LINE}"/>')
            o.append(f'<text x="{X(t):.1f}" y="{bottom + 22}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')

    # panel A: parallel run, the same settings as the measurement
    y = 142
    o.append(f'<text x="32" y="{y}" font-size="18" font-weight="700">병렬 실행(측정과 같은 설정), 단계 2</text>')
    top = y + 22
    rows_a = [("한 프레임", "", TRACE["s2par_frame"], DARK),
              ("엔진 회피 프로세서", "프레임마다 작업자 하나에서만", TRACE["s2par_avoid"], RED),
              ("게임 스레드가 기다린 시간", "작업이 끝나기를 기다린다", TRACE["s2par_gt_wait"], MID)]
    pos = [top, top + 52, top + 112]
    axis(top, pos[-1] + 52)
    for (label, sub, val, colour), yy in zip(rows_a, pos):
        bar(yy, label, sub, val, colour)
    # panel B: serial run, one processor at a time
    y = pos[-1] + 120
    o.append(f'<text x="32" y="{y}" font-size="18" font-weight="700">직렬 실행(프로세서를 차례로 돌린다), 프로세서 하나</text>')
    top = y + 22
    rows_b = [("단계 2 엔진 회피", TRACE["s2ser_avoid"], S2),
              ("단계 3 밀어내기 프로세서", TRACE["s3ser_sep"], S3),
              ("단계 2 엔진 방향 보간", TRACE["s2ser_orient"], S2),
              ("단계 3 방향 보간 병렬 사본", TRACE["s3ser_orient"], S3)]
    pos = [top + 40 * k for k in range(len(rows_b))]
    axis(top, pos[-1] + 38)
    for (label, val, colour), yy in zip(rows_b, pos):
        bar(yy, label, "", val, colour)
    yf = pos[-1] + 106
    yend = footer(o, yf, [f"병렬 실행의 엔진 회피 {TRACE_AVOID_RUNS:,}번은 저마다 한 스레드에서 돌았고 서로 겹치지 않았다.",
                          "직렬 실행은 프로세서별 시간을 가르려고 Mass 프로세서를 한 스레드에 둔다."])
    write("step2-trace.svg", svg(yend + 8, o, ARROW))


# ---------- 3. principle: engine avoidance and the hand-written separation (concept) ----------
def principle():
    o = []
    title(o, "엔진 회피는 힘을 더하고, 밀어내기 프로세서는 위치를 민다",
          "개념도. 칸의 값은 엔진과 프로젝트 소스의 설정값이다")
    CW, GAPX, TOP = 352, 32, 116
    cols = [
        (32, "엔진 회피(단계 2)", S2, DARK,
         ["이웃 후보: 2.5m 칸에서 가까운 칸부터 24개까지",
          "힘을 내는 이웃: 가장 가까운 6개",
          "결과: 조향 힘에 더하고 가속 상한으로 자른다",
          "순회: 청크를 한 스레드에서 차례로"]),
        (32 + CW + GAPX, "밀어내기 프로세서(단계 3부터)", S3, BLUE,
         ["이웃: 둘레 9칸에서 겹친 개체 모두",
          "결과: 남은 겹침의 절반씩 위치를 민다",
          "반복: 2번. 모두 같은 위치 사본을 읽는다",
          "순회: 128개씩 나눠 ParallelFor로"]),
    ]
    PH = 210
    for x, head, colour, tc, lines in cols:
        o.append(f'<rect x="{x}" y="{TOP}" width="{CW}" height="{PH + 40 + 30 * len(lines) + 10}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
        o.append(f'<text x="{x + 18}" y="{TOP + 32}" font-size="18" font-weight="700" fill="{tc}">{head}</text>')
        for k, t in enumerate(lines):
            o.append(f'<text x="{x + 18}" y="{TOP + PH + 70 + 30 * k}" font-size="16">{t}</text>')
    # left drawing: candidates and the six that push, force arrow
    cx, cy = 32 + CW / 2, TOP + 140
    rnd = random.Random(3)
    pts = []
    while len(pts) < 26:
        a = rnd.uniform(0, 2 * math.pi)
        r = rnd.uniform(26, 92)
        px, py = cx + r * math.cos(a), cy + r * 0.8 * math.sin(a)
        if all(math.hypot(px - qx, py - qy) > 20 for qx, qy in pts):
            pts.append((px, py))
    pts.sort(key=lambda p: math.hypot(p[0] - cx, p[1] - cy))
    for k, (px, py) in enumerate(pts):
        near = k < 6
        cand = k < 24
        o.append(f'<circle cx="{px:.1f}" cy="{py:.1f}" r="9" fill="{DARK if near else ("white" if cand else LINE)}" '
                 f'stroke="{DARK if cand else GRAY}" stroke-width="1.5"/>')
    o.append(f'<circle cx="{cx}" cy="{cy}" r="10" fill="{RED}"/>')
    o.append(f'<line x1="{cx}" y1="{cy}" x2="{cx - 50}" y2="{cy + 6}" stroke="{RED}" stroke-width="3" marker-end="url(#arrowr)"/>')
    o.append(f'<text x="{cx - 112}" y="{cy - 62}" font-size="16" font-weight="700" fill="{RED}" {HALO}>힘을 더한다</text>')
    o.append(f'<text x="{32 + 18}" y="{TOP + PH + 30}" font-size="16" fill="{SUB}">검은 점 6개만 힘을 낸다. 흰 점은 후보다</text>')
    # right drawing: 3x3 cells around the body, overlapping neighbours, push arrows
    cs = 54
    gx0, gy0 = 32 + CW + GAPX + CW / 2 - 1.5 * cs, TOP + 56
    for i in range(4):
        o.append(f'<line x1="{gx0 + i * cs}" y1="{gy0}" x2="{gx0 + i * cs}" y2="{gy0 + 3 * cs}" stroke="{GRAY}" stroke-dasharray="4 3"/>')
        o.append(f'<line x1="{gx0}" y1="{gy0 + i * cs}" x2="{gx0 + 3 * cs}" y2="{gy0 + i * cs}" stroke="{GRAY}" stroke-dasharray="4 3"/>')
    mx, my = gx0 + 1.5 * cs, gy0 + 1.5 * cs
    pairs = [(-22, -6), (20, 10), (-4, 30), (40, -36)]
    o.append(f'<circle cx="{mx}" cy="{my}" r="18" fill="{BLUE}" fill-opacity="0.85"/>')
    for dx, dy in pairs:
        o.append(f'<circle cx="{mx + dx * 1.2:.1f}" cy="{my + dy * 1.2:.1f}" r="16" fill="none" stroke="{BLUE}" stroke-width="2"/>')
    # push arrows on the centre body, away from its overlapping neighbours (the last one does not overlap)
    for dx, dy in pairs[:3]:
        d = math.hypot(dx, dy)
        o.append(f'<line x1="{mx:.1f}" y1="{my:.1f}" x2="{mx - dx / d * 40:.1f}" y2="{my - dy / d * 40:.1f}" stroke="{DARK}" stroke-width="2" marker-end="url(#arrow)"/>')
    o.append(f'<text x="{32 + CW + GAPX + 18}" y="{TOP + PH + 30}" font-size="16" fill="{SUB}">겹친 이웃마다 남은 겹침의 절반씩 떼어 놓는다</text>')
    yb = TOP + PH + 40 + 30 * 4 + 10
    yend = footer(o, yb + 52, ["엔진 회피는 이웃 수에 상한이 있고, 힘으로 속도만 바꾼다.",
                               "밀어내기 프로세서는 그 프레임의 이동 뒤에 겹침을 바로 푼다."])
    write("principle.svg", svg(yend + 8, o, ARROW))


# ---------- 4. same work: the equivalence check for steps 2 and 3 ----------
def same_work():
    o = []
    title(o, "같은 일을 하는지: 동등성 검사의 네 항목",
          "호드 총수 1만, 이동 장면, 오브 시점 카메라. 세 번 잰 중앙값")
    legend(o, 32, 112, [(S2, "단계 2 엔진 회피"), (S3, "단계 3 자체 밀어내기")])
    ref_death = float(e("V0-s3", "death_rate_ref"))
    ref_near = float(e("V0-s3", "near30_mean_ref"))
    lo, hi = (float(x) for x in e("V0-s3", "overlap_range").split(".."))
    panels = [
        ("초당 사망", "death_rate", 70.0, ("band", ref_death * 0.99, ref_death * 1.01, "기준 ±1%"), fmt),
        ("오브 30m 안 개체 수", "near30_mean", 2400.0, ("band", ref_near * 0.99, ref_near * 1.01, "기준 ±1%"), count),
        ("겹친 쌍", "overlap_mean", 14000.0, ("band", lo, hi, "허용 범위"), count),
        ("깊게 겹친 쌍(반지름 합의 절반보다 가깝다)", "overlap_deep_mean", 4200.0, None, count),
    ]
    X0, X1 = 170, 690
    y = 150
    for name, col, xmax, refspec, f in panels:
        X = lambda val, xm=xmax: X0 + val / xm * (X1 - X0)
        o.append(f'<text x="32" y="{y}" font-size="17" font-weight="700">{name}</text>')
        y += 14
        band_end = X0
        if refspec:
            _, a, b, lab = refspec
            xa, xb = X(a), max(X(b), X(a) + 3)
            band_end = xb
            o.append(f'<rect x="{xa:.1f}" y="{y - 4}" width="{xb - xa:.1f}" height="{2 * 24 + 10}" fill="{AMBER}" fill-opacity="0.25" stroke="{AMBER}" stroke-width="1.5"/>')
            o.append(f'<text x="{xa:.1f}" y="{y + 2 * 24 + 22}" font-size="16" fill="{AMBER}">{lab}</text>')
        for k, (var, colour, lab) in enumerate((("V0-s2", S2, "단계 2"), ("V0-s3", S3, "단계 3"))):
            yy = y + k * 26
            val = v(var, col)
            o.append(f'<text x="48" y="{yy + 17}" font-size="16">{lab}</text>')
            o.append(f'<rect x="{X0}" y="{yy}" width="{max(X(val) - X0, 1.5):.1f}" height="22" rx="3" fill="{colour}"/>')
            # the label goes right of the bar, and right of the reference band when the band is just behind the bar end
            lx = max(X(val), X0) + 8
            if lx < band_end + 8 and lx > band_end - 60:
                lx = band_end + 8
            o.append(f'<text x="{lx:.1f}" y="{yy + 17}" font-size="16" font-weight="700" fill="{DARK if colour == S2 else BLUE}">{f(val)}</text>')
        y += 2 * 26 + 46
    yend = footer(o, y + 10, ["기준은 단계 3–8의 중앙값이다. 겹친 쌍의 허용 범위는 그 중앙값의 절반에서 두 배다.",
                              "단계 3은 기준 세 항목 안에 있고 깊게 겹친 쌍이 0이다.",
                              "단계 2는 초당 사망과 30m 안 개체 수가 기준 밖이고,",
                              "겹친 쌍은 밀어내기 프로세서의 허용 범위를 크게 넘는다."])
    write("same-work.svg", svg(yend + 8, o))


# ---------- 5. final configuration: engine avoidance instead, separation off ----------
def final_swaps():
    o = []
    title(o, "최종 구성에서 겹침 방지를 바꾸거나 끄면",
          "호드 총수 1만, 이동 장면, 오브 시점 카메라. 세 번 잰 중앙값")
    items = [("V0-s8", "최종 구성", "밀어내기 프로세서"),
             ("V0-s8av", "엔진 회피로 바꾼 조건", "하위 실험"),
             ("V0-a8sep", "밀어내기를 끈 조건", "겹침 방지가 없다")]
    X0, X1, XMAX = 260, 560, 25.0
    X = lambda ms: X0 + ms / XMAX * (X1 - X0)
    CX = 640
    top = 150
    o.append(f'<text x="{X0}" y="{top - 22}" font-size="16" fill="{SUB}">프레임 p95(ms)</text>')
    o.append(f'<text x="{CX + 60}" y="{top - 22}" text-anchor="middle" font-size="16" fill="{SUB}">겹친 쌍</text>')
    RH = 78
    bottom = top + RH * len(items) - 30
    for t in [0, 5, 10, 15, 20, 25]:
        o.append(f'<line x1="{X(t):.1f}" y1="{top - 8}" x2="{X(t):.1f}" y2="{bottom + 6}" stroke="{LINE}"/>')
        o.append(f'<text x="{X(t):.1f}" y="{bottom + 28}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')
    xb = X(BUDGET_MS)
    o.append(f'<line x1="{xb:.1f}" y1="{top - 8}" x2="{xb:.1f}" y2="{bottom + 6}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="6 4"/>')
    o.append(f'<text x="{xb:.1f}" y="{bottom + 50}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">16.7</text>')
    base = v("V0-s8", "frame_p95_ms")
    base_rng = v("V0-s8", "frame_p95_ms_max") - v("V0-s8", "frame_p95_ms_min")
    y = top
    for var, name, sub in items:
        val = v(var, "frame_p95_ms")
        mn, mx = v(var, "frame_p95_ms_min"), v(var, "frame_p95_ms_max")
        colour = BLUE if var == "V0-s8" else (RED if val > base else MID)
        o.append(f'<text x="32" y="{y + 16}" font-size="17" font-weight="700">{name}</text>')
        o.append(f'<text x="32" y="{y + 40}" font-size="16" fill="{SUB}">{sub}</text>')
        o.append(f'<rect x="{X0}" y="{y}" width="{X(val) - X0:.1f}" height="30" rx="4" fill="{colour}"/>')
        cy = y + 15
        o.append(f'<line x1="{X(mn):.1f}" y1="{cy}" x2="{X(mx):.1f}" y2="{cy}" stroke="{INK}" stroke-width="2"/>')
        for b in (mn, mx):
            o.append(f'<line x1="{X(b):.1f}" y1="{cy - 7}" x2="{X(b):.1f}" y2="{cy + 7}" stroke="{INK}" stroke-width="2"/>')
        label = fmt(val)
        if var != "V0-s8":
            d = val - base
            rng = max(base_rng, mx - mn)
            # measurement.md "차이 판정": larger than the larger range and at least 0.5 ms
            label += f" ({signed(d)})" if abs(d) > rng and abs(d) >= SAME_MS else " (구별되지 않음)"
        o.append(f'<text x="{X(mx) + 8:.1f}" y="{y + 21}" font-size="16" font-weight="700" fill="{colour}" {HALO}>{label}</text>')
        o.append(f'<text x="{CX + 60}" y="{y + 21}" text-anchor="middle" font-size="17" font-weight="700" fill="{colour}">{count(v(var, "overlap_mean"))}</text>')
        y += RH
    ratio = v("V0-a8sep", "overlap_mean") / v("V0-s8", "overlap_mean")
    yend = footer(o, bottom + 96, ["엔진 회피로 바꾸면 프레임이 길어지고 겹친 쌍이 크게 는다.",
                                    f"밀어내기를 끄면 프레임은 짧지만 겹친 쌍이 {ratio:.0f}배로 늘어 같은 일이 아니다.",
                                    "가는 선은 세 실행의 최솟값–최댓값이다."])
    write("final-swaps.svg", svg(yend + 8, o))


summary()
step2_trace()
principle()
same_work()
final_swaps()
print("wrote", sorted(os.listdir(OUT)))
