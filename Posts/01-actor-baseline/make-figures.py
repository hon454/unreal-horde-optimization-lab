# Makes the SVG figures of post 01 in images/.
# Run from the repository root: py -3 -I Posts/01-actor-baseline/make-figures.py
# Measured values come from Measurements/Full4/summary.csv and Docs/figures/full4/full-n-at-60fps.csv
# (the full batch, Test package de0e21b). Step 1 is the variant V0-s1; the condition with only the
# Animation Budget Allocator on is V0-s1opt (measured at 500, 1,000 and 2,000).
# The trace values of actor-frame.svg are constants below. They were exported from
# Saved/Traces/Full4trace-V0-s1-N10000-P1-r1.utrace (git outside) with Scripts/trace-export.ps1
# (TimerStatistics, GameThread, 70-120 s). The trace run is a separate run with tracing on; its values
# show where the time goes and are not compared with the measured runs. Sources are in measurements.md.
import csv
import math
import os
import re
from decimal import Decimal, ROUND_HALF_UP

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")

BATCH = "Full4"
BUDGET_MS = 16.7          # Scripts/plot.py BUDGET_MS, the 60 fps line
DISTINCT_FLOOR_MS = 0.5   # Docs/Guides/measurement.md "차이 판정", Scripts/plot.py FULL_DISTINCT_FLOOR_MS
BASE, OPT = "V0-s1", "V0-s1opt"
ACTOR_NS = [500, 1000, 2000]

rows = {}
with open(os.path.join(ROOT, "Measurements", BATCH, "summary.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        rows[(r["variant"], int(r["n"]), r["preset"])] = r


def row(variant, n):
    return rows[(variant, n, "P1")]


def v(variant, n, col):
    return float(row(variant, n)[col])


def dv(a, b, n, col):
    """Difference b - a from the CSV text with Decimal, so that it rounds half up."""
    return Decimal(row(b, n)[col]) - Decimal(row(a, n)[col])


def distinguishable(a, b, n, key):
    """Same rule as Scripts/plot.py distinguishable(): at least 0.5 ms and larger than the larger run range."""
    d = abs(v(a, n, key) - v(b, n, key))
    rng = lambda var: v(var, n, key + "_max") - v(var, n, key + "_min")
    return d >= DISTINCT_FLOOR_MS and d > max(rng(a), rng(b))


with open(os.path.join(ROOT, "Docs", "figures", BATCH.lower(), "full-n-at-60fps.csv"), encoding="utf-8") as f:
    NAT = {r["series"]: r for r in csv.DictReader(f)}
MAXN = {BASE: int(NAT["1 Actor baseline"]["n_p95"]), OPT: int(NAT["1+ Animation Budget Allocator"]["n_p95"])}


def n_at_budget(points):
    """Scripts/plot.py n_at_budget: log interpolation of N where frame p95 crosses 16.7 ms."""
    for (n0, m0), (n1, m1) in zip(points, points[1:]):
        if m0 <= BUDGET_MS < m1:
            t = (BUDGET_MS - m0) / (m1 - m0)
            return math.exp(math.log(n0) + t * (math.log(n1) - math.log(n0)))
    raise ValueError("no crossing")


for var in (BASE, OPT):
    mine = round(n_at_budget([(n, v(var, n, "frame_p95_ms")) for n in ACTOR_NS]))
    if mine != MAXN[var]:
        raise SystemExit(f"max N of {var} differs from full-n-at-60fps.csv: {mine} != {MAXN[var]}")

# ---------- trace constants: Full4trace-V0-s1-N10000-P1-r1, GameThread, 70-120 s (measurements.md "트레이스") ----------
# Same window and the same CharacterMovement measure (exclusive time) as post 02, so that both posts show one value.
T_FRAMES = 93               # FEngineLoop::Tick events that overlap the window (91 whole frames and 2 cut at the edges)
T_TICK_S = 49.999963        # FEngineLoop::Tick, inclusive total (s)
T_CMC_S = 38.197391         # UCharacterMovementComponent_TickComponent, exclusive total (s); inclusive is 38.320052
T_CMC_COUNT = 928391        # its count
T_SKIN_S = 2.502503         # USkinnedMeshComponent_TickComponent, inclusive total (s): holds the AnimBP update
T_ANIMEVAL_S = 1.671582     # USkeletalMeshComponent_CompleteParallelAnimationEvaluation, inclusive total (s)
T_EOF_S = 0.411929          # UWorld_SendAllEndOfFrameUpdates, inclusive total (s)
per_frame = lambda s: s / T_FRAMES * 1000.0
T_FRAME = per_frame(T_TICK_S)
T_CMC, T_SKIN, T_ANIMEVAL, T_EOF = (per_frame(s) for s in (T_CMC_S, T_SKIN_S, T_ANIMEVAL_S, T_EOF_S))
T_REST = T_FRAME - T_CMC - T_SKIN - T_ANIMEVAL - T_EOF


# ---------- formatting (posting.md: three significant figures, 27 not 27.0) ----------
def _no_zero_decimals(fn):
    return lambda *a, **k: re.sub(r"[.]0+$", "", fn(*a, **k))


@_no_zero_decimals
def fmt(x):
    """Three significant figures, rounded half up from the CSV text (18.255 -> 18.3)."""
    q = "1" if abs(x) >= 100 else "0.1" if abs(x) >= 10 else "0.01"
    return str(Decimal(repr(x)).quantize(Decimal(q), rounding=ROUND_HALF_UP))


def fmtd(x):
    """Signed difference in ms with two decimals, minus sign U+2212."""
    x = x if isinstance(x, Decimal) else Decimal(repr(x))
    s = re.sub(r"[.]0+$", "", str(abs(x).quantize(Decimal("0.01"), rounding=ROUND_HALF_UP)))
    return ("+" if x > 0 else "−") + s


@_no_zero_decimals
def pct(x):
    """A share with one decimal: 76.59 -> 76.6, 5.02 -> 5 (posting.md: 27 not 27.0)."""
    return str(Decimal(repr(x)).quantize(Decimal("0.1"), rounding=ROUND_HALF_UP))


def cnt(x):
    return f"{int(Decimal(repr(x)).quantize(Decimal('1'), rounding=ROUND_HALF_UP)):,}"


def esc(t):
    return str(t).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


# ---------- style (same as the other posts) ----------
FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
MONO = "Consolas, SFMono-Regular, Menlo, monospace"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, PURPLE, LILAC, GRAY, AMBER, RED, TEAL, PINK, DARK, MID = (
    "#2563eb", "#9333ea", "#c084fc", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151", "#6b7280")
W = 800
HATCH = (f'<defs><pattern id="hatch" width="8" height="8" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
         f'<rect width="8" height="8" fill="#fca5a5"/><line x1="0" y1="0" x2="0" y2="8" stroke="#ffffff" stroke-width="3"/></pattern></defs>')


def svg(h, body, defs=""):
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">']
    if defs:
        head.append(defs)
    head.append(f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>')
    return "\n".join(head + body + ["</svg>"]) + "\n"


def write(name, text):
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def title(o, t, subs):
    o.append(f'<text x="32" y="44" font-size="22" font-weight="700">{esc(t)}</text>')
    y = 72
    for s in subs:
        o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">{esc(s)}</text>')
        y += 22
    return y


def text(x, y, t, size=16, fill=INK, anchor="start", weight=None, halo=False, family=None):
    attrs = f'x="{x:.1f}" y="{y:.1f}" font-size="{size}" text-anchor="{anchor}"' + (f' font-weight="{weight}"' if weight else "")
    if family:
        attrs += f' font-family="{family}"'
    out = ""
    if halo:
        out += f'<text {attrs} fill="{BG}" stroke="{BG}" stroke-width="5" stroke-linejoin="round">{esc(t)}</text>'
    return out + f'<text {attrs} fill="{fill}">{esc(t)}</text>'


def footer(o, y, lines):
    o.append(f'<line x1="32" y1="{y - 22}" x2="{W - 32}" y2="{y - 22}" stroke="{LINE}"/>')
    for line in lines:
        o.append(text(32, y, line, fill=SUB))
        y += 24
    return y


CHECK = []  # (figure, item, text) for the scratch comparison against the CSV


# ---------- 1. summary: frame p95, frame p50 and the three threads at 1,000 and 10,000 ----------
def threads():
    o = []
    y = title(o, "단계 1 액터 기준선이 쓰는 시간",
              ["이동 장면, 오브 시점 카메라, 세 번 잰 중앙값(ms)",
               "두 판의 가로 축척이 다르다. 가는 선은 세 실행의 최솟값–최댓값"])
    items = [("프레임 p95", "frame_p95_ms", DARK, True), ("프레임 p50", "frame_p50_ms", MID, True),
             ("게임 스레드", "game_p50_ms", BLUE, False), ("렌더 스레드", "render_p50_ms", TEAL, False),
             ("GPU", "gpu_p50_ms", PINK, False)]
    panels = [(1000, "호드 총수 1천: 60fps 근처", 25.0, [0, 5, 10, 15, 20, 25]),
              (10000, "호드 총수 1만: 같은 일을 하지 못해 보고만 하는 값", 700.0, [0, 100, 200, 300, 400, 500, 600, 700])]
    X0, X1 = 170, 700
    RH, BH = 34, 22
    y += 18
    for n, head, xmax, ticks in panels:
        X = lambda ms, xmax=xmax: X0 + ms / xmax * (X1 - X0)
        o.append(text(32, y + 16, head, size=17, weight="700"))
        top = y + 58
        bottom = top + RH * len(items) - (RH - BH)
        for t in ticks:
            o.append(f'<line x1="{X(t):.1f}" y1="{top - 8}" x2="{X(t):.1f}" y2="{bottom + 6}" stroke="{LINE}"/>')
            o.append(text(X(t), bottom + 26, str(t), fill=SUB, anchor="middle"))
        xb = X(BUDGET_MS)
        o.append(f'<line x1="{xb:.1f}" y1="{top - 12}" x2="{xb:.1f}" y2="{bottom + 6}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="6 4"/>')
        # 1,000: the label sits left of the line, 10,000: right of it (the line is near the left edge)
        if n == 10000:
            o.append(text(xb + 6, top - 14, "16.7ms(60fps)", fill=AMBER, weight="700"))
        else:
            o.append(text(xb - 6, top - 14, "16.7ms(60fps)", fill=AMBER, weight="700", anchor="end"))
        bottleneck = max(("game_p50_ms", "render_p50_ms", "gpu_p50_ms"), key=lambda c: v(BASE, n, c))
        if row(BASE, n)["bottleneck"] != "game" or bottleneck != "game_p50_ms":
            raise SystemExit(f"bottleneck of {BASE} at {n} is not the game thread")
        yy = top
        for label, col, colour, ranged in items:
            val = v(BASE, n, col)
            o.append(text(40, yy + 17, label, size=16, weight="700"))
            o.append(f'<rect x="{X0}" y="{yy}" width="{X(val) - X0:.1f}" height="{BH}" rx="3" fill="{colour}"/>')
            end = X(val)
            if ranged:
                mn, mx = v(BASE, n, col + "_min"), v(BASE, n, col + "_max")
                cy = yy + BH / 2
                o.append(f'<line x1="{X(mn):.1f}" y1="{cy}" x2="{X(mx):.1f}" y2="{cy}" stroke="{INK}" stroke-width="2"/>')
                for e in (mn, mx):
                    o.append(f'<line x1="{X(e):.1f}" y1="{cy - 7}" x2="{X(e):.1f}" y2="{cy + 7}" stroke="{INK}" stroke-width="2"/>')
                end = max(end, X(mx))
            o.append(text(end + 8, yy + 17, fmt(val), fill=colour, weight="700"))
            CHECK.append(("threads", f"{n} {col}", fmt(val)))
            if col == bottleneck:
                o.append(text(end + 8 + 11 * len(fmt(val)) + 10, yy + 17, "병목", fill=BLUE, weight="700"))
            yy += RH
        y = bottom + 56
    y = footer(o, y + 8, ["프레임 시간은 가장 느린 스레드(병목)가 정하고, 그보다 조금 길다.",
                          "1만은 월드 시간이 프레임마다 0.4초로 잘려 같은 일을 하지 못한다."])
    write("step1-threads.svg", svg(y, o))


# ---------- 2. principle: what one actor does in one frame, with the trace time per frame at 10,000 ----------
def actor_frame():
    o = []
    title(o, "액터 한 개체가 한 프레임에 하는 일",
          ["게임 스레드에서 위에서 아래 순서로 돈다. 막대는 호드 총수 1만의 프레임당 시간"])
    CX, CW = 32, 448
    BX, BW = 500, 268
    scale = BW / T_CMC
    o.append(text(BX, 112, "프레임당 시간(같은 축척)", fill=SUB))
    steps = [
        ("1", "걷게 하고 캡슐을 스윕으로 옮긴다", ["UCharacterMovementComponent_", "TickComponent"], BLUE, T_CMC),
        ("2", "AnimBP로 애니메이션 상태를 갱신한다", ["USkinnedMeshComponent_", "TickComponent"], PURPLE, T_SKIN),
        ("3", "병렬로 계산한 포즈를 메시에 반영한다", ["USkeletalMeshComponent_", "CompleteParallelAnimationEvaluation"], LILAC, T_ANIMEVAL),
        ("4", "바뀐 상태를 렌더 스레드로 보낸다", ["UWorld_SendAllEndOfFrameUpdates"], TEAL, T_EOF),
    ]
    y, RH, GAP = 126, 100, 20
    for i, (num, t, timer, col, ms) in enumerate(steps):
        o.append(f'<rect x="{CX}" y="{y}" width="{CW}" height="{RH}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
        o.append(f'<rect x="{CX}" y="{y}" width="6" height="{RH}" rx="3" fill="{col}"/>')
        o.append(f'<circle cx="{CX + 34}" cy="{y + 30}" r="15" fill="{col}"/>')
        o.append(text(CX + 34, y + 36, num, fill="white", anchor="middle", weight="700"))
        o.append(text(CX + 62, y + 37, t, size=18, weight="700"))
        for k, line in enumerate(timer):
            o.append(text(CX + 62, y + 64 + k * 20, line, fill=SUB, family=MONO))
        w = ms * scale
        share = ms / T_FRAME * 100
        label = f"{fmt(ms)}ms · {pct(share)}%"
        by = y + 28
        o.append(f'<rect x="{BX}" y="{by}" width="{max(w, 2):.1f}" height="44" rx="4" fill="{col}"/>')
        if w > 160:
            o.append(text(BX + 12, by + 29, label, size=18, fill="white", weight="700"))
        else:
            o.append(text(BX + max(w, 2) + 10, by + 29, label, size=17, fill=col, weight="700"))
        CHECK.append(("actor-frame", timer[0], label))
        if i < len(steps) - 1:
            ax = CX + 34
            o.append(f'<path d="M{ax} {y + RH + 3} L{ax} {y + RH + GAP - 5}" stroke="{SUB}" stroke-width="2"/>')
            o.append(f'<path d="M{ax - 5} {y + RH + GAP - 10} L{ax} {y + RH + GAP - 3} L{ax + 5} {y + RH + GAP - 10}" '
                     f'fill="none" stroke="{SUB}" stroke-width="2"/>')
        y += RH + GAP
    # the rest of the game thread frame
    w = T_REST * scale
    o.append(text(CX, y + 30, "나머지: 틱 작업의 관리와 그 밖의 게임 스레드 일", size=17, weight="700", fill=SUB))
    o.append(f'<rect x="{BX}" y="{y + 6}" width="{w:.1f}" height="36" rx="4" fill="{GRAY}" fill-opacity="0.35" stroke="{GRAY}" stroke-dasharray="4 3"/>')
    rest = f"{fmt(T_REST)}ms · {pct(T_REST / T_FRAME * 100)}%"
    o.append(text(BX + w + 10, y + 30, rest, size=17, fill=SUB, weight="700"))
    CHECK.append(("actor-frame", "rest", rest))
    y += 64
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y += 34
    o.append(text(32, y, f"게임 스레드 한 프레임 {fmt(T_FRAME)}ms", size=17, weight="700"))
    CHECK.append(("actor-frame", "frame", fmt(T_FRAME)))
    w167 = BUDGET_MS * scale
    o.append(f'<rect x="{BX}" y="{y - 16}" width="{w167:.1f}" height="20" rx="3" fill="{AMBER}"/>')
    o.append(text(BX + w167 + 10, y, "16.7ms = 60fps", size=17, fill=AMBER, weight="700"))
    y += 28
    o.append(text(32, y, f"CharacterMovement 틱은 프레임마다 {cnt(T_CMC_COUNT / T_FRAMES)}번 불린다.", fill=SUB))
    CHECK.append(("actor-frame", "cmc calls per frame", cnt(T_CMC_COUNT / T_FRAMES)))
    y += 24
    o.append(text(32, y, "이동 장면, 오브 시점 카메라. 따로 뜬 트레이스 한 회, 70–120초 구간의 프레임 평균", fill=SUB))
    write("actor-frame.svg", svg(y + 26, o))


# ---------- 3. key figure: frame p95 by horde size, step 1 and the Budget Allocator condition ----------
def frame_by_n():
    o = []
    title(o, "호드 총수에 따른 프레임 p95",
          ["이동 장면, 오브 시점 카메라, 세 번 잰 중앙값. 가로축은 로그 눈금"])
    series = [(BASE, RED, ["단계 1", "액터 기준선"]), (OPT, BLUE, ["Budget Allocator", "(예산 4ms)를 켠 액터"])]
    L, R, T, B = 90, 560, 136, 456
    NLO, NHI, YMAX = 400, 2600, 45.0
    X = lambda n: L + (math.log(n) - math.log(NLO)) / (math.log(NHI) - math.log(NLO)) * (R - L)
    Y = lambda ms: B - ms / YMAX * (B - T)
    for t in [0, 10, 20, 30, 40]:
        o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
        o.append(text(L - 10, Y(t) + 6, str(t), fill=SUB, anchor="end"))
    o.append(text(32, T - 16, "프레임 p95(ms)", fill=SUB))
    for n, lab in [(500, "500"), (1000, "1천"), (2000, "2천")]:
        o.append(f'<line x1="{X(n):.1f}" y1="{B}" x2="{X(n):.1f}" y2="{B + 5}" stroke="{SUB}"/>')
        o.append(text(X(n), B + 26, lab, anchor="middle"))
    o.append(f'<line x1="{L}" y1="{B}" x2="{R}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<line x1="{L}" y1="{T}" x2="{L}" y2="{B}" stroke="{SUB}"/>')
    o.append(text((L + R) / 2, B + 52, "호드 총수", fill=SUB, anchor="middle"))
    y167 = Y(BUDGET_MS)
    o.append(f'<line x1="{L}" y1="{y167:.1f}" x2="{R}" y2="{y167:.1f}" stroke="{AMBER}" stroke-dasharray="6 4" stroke-width="1.5"/>')
    o.append(text(L + 8, y167 - 8, "16.7ms(60fps)", fill=AMBER, weight="700"))
    for var, col, _ in series:
        pts = [(n, v(var, n, "frame_p95_ms")) for n in ACTOR_NS]
        # straight segments on a log-N axis = the log interpolation of Scripts/plot.py n_at_budget
        o.append(f'<polyline points="{" ".join(f"{X(n):.1f},{Y(m):.1f}" for n, m in pts)}" fill="none" stroke="{col}" stroke-width="2.5"/>')
        for n, m in pts:
            lo, hi = v(var, n, "frame_p95_ms_min"), v(var, n, "frame_p95_ms_max")
            o.append(f'<line x1="{X(n):.1f}" y1="{Y(lo):.1f}" x2="{X(n):.1f}" y2="{Y(hi):.1f}" stroke="{col}" stroke-width="2"/>')
            o.append(f'<circle cx="{X(n):.1f}" cy="{Y(m):.1f}" r="5" fill="{col}"/>')
            # step 1 labels on the left of the point, the Budget Allocator labels on the right
            if var == BASE:
                o.append(text(X(n) - 10, Y(m) - 6, fmt(m), fill=col, anchor="end", weight="700", halo=True))
            else:
                o.append(text(X(n) + 10, Y(m) + 18, fmt(m), fill=col, weight="700", halo=True))
            CHECK.append(("frame-by-n", f"{var} {n}", fmt(m)))
    # crossings: the two max N are close, so one label goes left and one goes right
    for var, col, _ in series:
        nx = X(MAXN[var])
        o.append(f'<line x1="{nx:.1f}" y1="{y167:.1f}" x2="{nx:.1f}" y2="{B}" stroke="{col}" stroke-width="1.5" stroke-dasharray="3 3"/>')
        o.append(f'<circle cx="{nx:.1f}" cy="{y167:.1f}" r="6" fill="white" stroke="{col}" stroke-width="3"/>')
        if var == BASE:
            o.append(text(nx + 6, B - 10, f"{MAXN[var]:,}", fill=col, weight="700", halo=True))
        else:
            o.append(text(nx - 6, B - 10, f"{MAXN[var]:,}", fill=col, weight="700", anchor="end", halo=True))
        CHECK.append(("frame-by-n", f"max N {var}", f"{MAXN[var]:,}"))
    # 2,000: the only difference beyond the noise floor that the Budget Allocator makes smaller
    d2k = dv(BASE, OPT, 2000, "frame_p95_ms")
    if not distinguishable(BASE, OPT, 2000, "frame_p95_ms"):
        raise SystemExit("the 2,000 difference is expected to be distinguishable")
    x2 = X(2000)
    o.append(text(x2 + 10, (Y(v(BASE, 2000, "frame_p95_ms")) + Y(v(OPT, 2000, "frame_p95_ms"))) / 2 + 5,
                  f"{fmtd(d2k)}", fill=INK, weight="700", halo=True))
    CHECK.append(("frame-by-n", "2000 diff", fmtd(d2k)))
    # legend
    lx, ly = 588, 150
    for var, col, words in series:
        o.append(f'<line x1="{lx}" y1="{ly}" x2="{lx + 28}" y2="{ly}" stroke="{col}" stroke-width="2.5"/>')
        o.append(f'<circle cx="{lx + 14}" cy="{ly}" r="5" fill="{col}"/>')
        o.append(text(lx + 36, ly + 6, words[0], fill=col, weight="700"))
        o.append(text(lx + 36, ly + 28, words[1]))
        ly += 68
    o.append(f'<circle cx="{lx + 14}" cy="{ly}" r="6" fill="white" stroke="{DARK}" stroke-width="3"/>')
    for k, line in enumerate(["최대 N: 16.7ms를", "넘기 직전의", "호드 총수"]):
        o.append(text(lx + 36, ly + 6 + 22 * k, line))
    ly += 92
    o.append(f'<line x1="{lx + 14}" y1="{ly - 10}" x2="{lx + 14}" y2="{ly + 10}" stroke="{DARK}" stroke-width="2"/>')
    o.append(text(lx + 36, ly + 6, "세 실행의 범위"))
    # the Budget Allocator's own animation time per horde size
    yb = B + 92
    o.append(f'<line x1="32" y1="{yb - 22}" x2="{W - 32}" y2="{yb - 22}" stroke="{LINE}"/>')
    o.append(text(32, yb, "Budget Allocator가 잰 액터 메시의 애니메이션 시간(ms, 예산 4ms)", fill=BLUE, weight="700"))
    for n in ACTOR_NS:
        val = fmt(v(OPT, n, "budgeted_anim_ms"))
        o.append(text(X(n), yb + 28, val, fill=BLUE, anchor="middle", weight="700"))
        CHECK.append(("frame-by-n", f"budgeted_anim_ms {n}", val))
    s1_10k = v(BASE, 10000, "frame_p95_ms")
    CHECK.append(("frame-by-n", "s1 10000 p95", fmt(s1_10k)))
    y = footer(o, yb + 84, [f"단계 1의 1만(프레임 p95 {fmt(s1_10k)}ms)은 그림 밖이고 같은 일을 한 값이 아니다.",
                            "16.7ms를 사이에 둔 두 점 사이를 로그 보간해 최대 N을 읽는다."])
    write("frame-p95-by-n.svg", svg(y, o))


# ---------- 4. expected and actual: game thread time per entity ----------
def per_entity():
    o = []
    y = title(o, "개체 하나당 게임 스레드 시간",
              ["단계 1, 이동 장면, 오브 시점 카메라. 게임 스레드 시간(중앙값) ÷ 호드 총수(µs)",
               "하위 스텝 횟수는 엔진 규칙(50ms 단위, 월드 시간 상한 0.4초)으로 셈했다"])
    ns = [500, 1000, 2000, 10000]
    labs = ["500", "1천", "2천", "1만"]
    steps = ["하위 스텝 1회", "1회", "1회", "8회"]
    per = [v(BASE, n, "game_p50_ms") / n * 1000.0 for n in ns]
    T, B = y + 54, y + 304
    YMAX = 65.0
    Y = lambda us: B - us / YMAX * (B - T)
    for t in [0, 20, 40, 60]:
        o.append(f'<line x1="90" y1="{Y(t):.1f}" x2="{W - 60}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
        o.append(text(80, Y(t) + 6, str(t), fill=SUB, anchor="end"))
    exp = per[1]
    o.append(f'<line x1="90" y1="{Y(exp):.1f}" x2="{W - 60}" y2="{Y(exp):.1f}" stroke="{SUB}" stroke-width="1.5" stroke-dasharray="6 4"/>')
    o.append(f'<line x1="90" y1="{T - 22}" x2="120" y2="{T - 22}" stroke="{SUB}" stroke-width="1.5" stroke-dasharray="6 4"/>')
    o.append(text(128, T - 16, f"예상: 1천의 값({fmt(exp)}µs)으로 일정", fill=SUB))
    CHECK.append(("per-entity", "expected", fmt(exp)))
    for k, (n, lab, st, us) in enumerate(zip(ns, labs, steps, per)):
        cx = 180 + k * 150
        h = B - Y(us)
        fill = "url(#hatch)" if n == 10000 else BLUE
        o.append(f'<rect x="{cx - 32}" y="{Y(us):.1f}" width="64" height="{h:.1f}" rx="3" fill="{fill}"'
                 + (f' stroke="{RED}" stroke-width="1.5"' if n == 10000 else "") + "/>")
        o.append(text(cx, Y(us) - 10, fmt(us), size=17, fill=RED if n == 10000 else BLUE, anchor="middle", weight="700", halo=True))
        o.append(text(cx, B + 24, lab, anchor="middle"))
        o.append(text(cx, B + 48, st, fill=SUB, anchor="middle"))
        CHECK.append(("per-entity", f"{n}", fmt(us)))
    o.append(f'<line x1="90" y1="{B}" x2="{W - 60}" y2="{B}" stroke="{SUB}"/>')
    g1k, g10k = v(BASE, 1000, "game_p50_ms"), v(BASE, 10000, "game_p50_ms")
    CHECK.append(("per-entity", "expected 10000 game", fmt(g1k * 10)))
    CHECK.append(("per-entity", "actual 10000 game", fmt(g10k)))
    y = footer(o, B + 96, [f"1천으로 예상한 1만의 게임 스레드는 {fmt(g1k * 10)}ms, 잰 값은 {fmt(g10k)}ms다.",
                           "빗금 막대(1만)는 월드 시간이 잘려 같은 일을 하지 못한 값이다."])
    write("per-entity-cost.svg", svg(y, o, HATCH))


if __name__ == "__main__":
    threads()
    actor_frame()
    frame_by_n()
    per_entity()
    if os.environ.get("LAB_FIG_CHECK"):
        for item in CHECK:
            print(item)
    print("wrote", ", ".join(sorted(os.listdir(OUT))))
