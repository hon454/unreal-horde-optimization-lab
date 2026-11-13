# Makes the SVG figures of post 09 in images/.
# Run from the repository root: py -3 -I Posts/09-summary/make-figures.py
# Measured values come from Measurements/Full3/summary.csv and Docs/figures/full3/full-n-at-60fps.csv.
# The other figures of the post are the README figures in Docs/figures/readme/ (Scripts/plot-readme.py).
# No trace values are drawn.
import csv, math, os
import re
from decimal import Decimal, ROUND_HALF_UP

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")

rows = {}
with open(os.path.join(ROOT, "Measurements", "Full3", "summary.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        rows[(r["variant"], int(r["n"]), r["preset"])] = r


def v(variant, n, col, preset="P1"):
    return float(rows[(variant, n, preset)][col])


def rng(variant, n, col, preset="P1"):
    r = rows[(variant, n, preset)]
    return float(r[col + "_max"]) - float(r[col + "_min"])


with open(os.path.join(ROOT, "Docs", "figures", "full3", "full-n-at-60fps.csv"), encoding="utf-8") as f:
    NAT = {r["series"]: r for r in csv.DictReader(f)}
N_AT_60_L8 = int(NAT["8 L8-q4final"]["n_p95"])

STEPS = ["L1-full", "L2-mass", "L3-vatcull", "L4-mass", "L5-m4offpool", "L6-m5bounds", "L7-q2crowd", "L8-q4final"]
L8 = STEPS[-1]
NS = [5000, 10000, 20000, 30000]
NLAB = ["5천", "1만", "2만", "3만"]
BUDGET = 16.7          # 60fps line, same value as Scripts/plot.py BUDGET_MS

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, PURPLE, GRAY, AMBER, RED, TEAL, PINK, DARK, GREEN = (
    "#2563eb", "#9333ea", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151", "#15803d")
W = 800


def svg(h, body):
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">',
            f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>']
    return "\n".join(head + body + ["</svg>"]) + "\n"


def write(name, text):
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def _no_zero_decimals(f):
    """27.0 -> 27. A value whose decimals are all zero is written as an integer."""
    return lambda *a, **k: re.sub(r"[.]0+$", "", f(*a, **k))


@_no_zero_decimals
def fmt(x):
    """Three significant figures, rounded half up from the decimal text (2.345 -> 2.35), as in the post body."""
    d = Decimal(repr(round(x, 6)))
    a = abs(d)
    q = "1" if a >= 100 else "0.1" if a >= 10 else "0.01" if a >= 1 else "0.001"
    return str(d.quantize(Decimal(q), rounding=ROUND_HALF_UP))


def sfmt(x):
    """Signed, with a real minus sign (U+2212) as in the post body."""
    s = fmt(x)
    return "−" + s[1:] if s.startswith("-") else "+" + s


def title(o, t, s):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')


BN = {"game": "게임 스레드", "gpu": "GPU", "render": "렌더 스레드"}


def bottleneck(var, n=10000):
    return BN[rows[(var, n, "P1")]["bottleneck"]]


# ---------- 1. what each step earned and spent (waterfall of changes) ----------
TRANS = [  # (from, to, name of the step that was added, group)
    ("L2-mass", "L3-vatcull", "3단계 ISM + VAT", "earn"),
    ("L3-vatcull", "L4-mass", "4단계 액터 하이드레이션", "spend"),
    ("L4-mass", "L5-m4offpool", "5단계 Simulation LOD", "earn"),
    ("L5-m4offpool", "L6-m5bounds", "6단계 ISKM", "earn"),
    ("L6-m5bounds", "L7-q2crowd", "7단계 Niagara 배경 무리", "spend"),
    ("L7-q2crowd", "L8-q4final", "8단계 품질 올리기", "spend"),
]
DELTA = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE), ("GPU", "gpu_p50_ms", PINK)]


def trans_rows():
    out = []
    for a, b, name, grp in TRANS:
        cells = []
        for lab, col, c in DELTA:
            d = v(b, 10000, col) - v(a, 10000, col)
            noise = max(rng(a, 10000, col), rng(b, 10000, col))
            cells.append((lab, col, c, d, abs(d) > noise))
        out.append((a, b, name, grp, cells))
    return out


def waterfall():
    o = []
    title(o, "단계마다 번 시간과 쓴 시간",
          "호드 총수 1만, 오브 시점 카메라, 앞 단계 대비 변화(세 번 잰 중앙값의 차, ms)")
    d12 = v("L2-mass", 10000, "frame_p50_ms") - v("L1-full", 10000, "frame_p50_ms")
    y = 108
    o.append(f'<rect x="32" y="{y}" width="{W - 64}" height="58" rx="8" fill="{CARD}" stroke="{LINE}"/>')
    o.append(f'<text x="48" y="{y + 24}" font-size="17" font-weight="700">2단계 Mass 로직: 프레임 시간 {sfmt(d12)}ms</text>')
    o.append(f'<text x="48" y="{y + 46}" font-size="16" fill="{SUB}">아래 막대의 약 90배라 그리지 않았다. 병목은 '
             f'{bottleneck("L1-full")}에서 {bottleneck("L2-mass")}로 바뀐다.</text>')
    y += 90
    ZX, scale = 330, 280 / 5.0   # zero line; one bar unit = 56px per ms
    top = y - 18
    data = trans_rows()
    for a, b, name, grp, cells in data:
        o.append(f'<text x="32" y="{y + 16}" font-size="17" font-weight="700">{name}</text>')
        ba, bb = bottleneck(a), bottleneck(b)
        btxt = f"병목 {bb}" if ba == bb else f"병목 {ba} → {bb}"
        o.append(f'<text x="32" y="{y + 42}" font-size="16" fill="{RED if ba != bb else SUB}">{btxt}</text>')
        if grp == "spend":
            o.append(f'<text x="32" y="{y + 66}" font-size="16" fill="{SUB}">번 시간을 품질에 쓰는 단계</text>')
        for lab, col, c, d, real in cells:
            w = abs(d) * scale
            x = ZX - w if d < 0 else ZX
            if real:
                o.append(f'<rect x="{x:.1f}" y="{y}" width="{max(w, 1.5):.1f}" height="20" rx="3" fill="{c}"/>')
            else:
                o.append(f'<rect x="{x:.1f}" y="{y}" width="{max(w, 1.5):.1f}" height="20" rx="3" fill="white" stroke="{c}" stroke-width="1.5" stroke-dasharray="4 3"/>')
            tx = ZX + w + 8 if d > 0 else ZX + 8
            tag = "" if real else " (흔들림 안)"
            o.append(f'<text x="{tx:.1f}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{sfmt(d)}</tspan>'
                     f'<tspan fill="{SUB}"> {lab}{tag}</tspan></text>')
            y += 26
        y += 20
    o.append(f'<line x1="{ZX}" y1="{top}" x2="{ZX}" y2="{y - 14}" stroke="{SUB}" stroke-width="1.5"/>')
    o.append(f'<text x="{ZX - 8}" y="{y + 6}" text-anchor="end" font-size="16" fill="{SUB}">← 빨라짐</text>')
    o.append(f'<text x="{ZX + 8}" y="{y + 6}" font-size="16" fill="{SUB}">느려짐 →</text>')
    y += 40
    df = v(L8, 10000, "frame_p50_ms") - v("L2-mass", 10000, "frame_p50_ms")
    dg = v(L8, 10000, "game_p50_ms") - v("L2-mass", 10000, "game_p50_ms")
    du = v(L8, 10000, "gpu_p50_ms") - v("L2-mass", 10000, "gpu_p50_ms")
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">점선 막대는 변화가 세 번 잰 값의 흔들림보다 작아 구별되지 않는 칸이다.</text>')
    fnoise = max(rng("L2-mass", 10000, "frame_p50_ms"), rng(L8, 10000, "frame_p50_ms"))
    ftag = "" if abs(df) > fnoise else "(흔들림 안)"
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">2단계에서 8단계까지: 프레임 시간 {sfmt(df)}ms{ftag}, GPU {sfmt(du)}ms, 게임 스레드 {sfmt(dg)}ms.</text>')
    write("ladder-changes.svg", svg(y + 60, o))


# ---------- 2. target review: final configuration vs horde size ----------
def n_at(points, budget):
    """Same interpolation as Scripts/plot.py n_at_budget: linear in ms, log in N."""
    for (n0, m0), (n1, m1) in zip(points, points[1:]):
        if m0 <= budget < m1:
            t = (budget - m0) / (m1 - m0)
            return math.exp(math.log(n0) + t * (math.log(n1) - math.log(n0)))
    return None


def p95_points():
    return [(n, v(L8, n, "frame_p95_ms")) for n in NS]


def headroom():
    o = []
    title(o, "최종 구성은 호드 총수 1만에서 16.7ms까지 얼마가 남나",
          "8단계, 오브 시점 카메라, 세 번 잰 중앙값. 네 가지 호드 총수에서 쟀다")
    T, B, L, R = 120, 520, 70, 560
    ymax = 25.0
    X = lambda n: L + n / 32000 * (R - L)
    Y = lambda val: B - val / ymax * (B - T)
    o.append(f'<text x="32" y="{T - 18}" font-size="18" font-weight="700">시간(ms)</text>')
    for t in [0, 5, 10, 15, 20, 25]:
        o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
        o.append(f'<text x="{L - 10}" y="{Y(t) + 6:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{t}</text>')
    for n, lab in zip(NS, NLAB):
        o.append(f'<line x1="{X(n):.1f}" y1="{B}" x2="{X(n):.1f}" y2="{B + 5}" stroke="{SUB}"/>')
        o.append(f'<text x="{X(n):.1f}" y="{B + 26}" text-anchor="middle" font-size="16">{lab}</text>')
    o.append(f'<line x1="{L}" y1="{B}" x2="{R}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<line x1="{L}" y1="{T}" x2="{L}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<text x="{(L + R) / 2}" y="{B + 52}" text-anchor="middle" font-size="16" fill="{SUB}">호드 총수</text>')
    # target column and the 60fps crossing
    xt = X(10000)
    o.append(f'<rect x="{xt - 14:.1f}" y="{T}" width="28" height="{B - T}" fill="{GREEN}" fill-opacity="0.08"/>')
    o.append(f'<text x="{xt:.1f}" y="{T + 18}" text-anchor="middle" font-size="16" font-weight="700" fill="{GREEN}">목표 1만</text>')
    xn = X(N_AT_60_L8)
    o.append(f'<line x1="{xn:.1f}" y1="{Y(BUDGET):.1f}" x2="{xn:.1f}" y2="{B}" stroke="{AMBER}" stroke-width="1.5" stroke-dasharray="3 3"/>')
    o.append(f'<text x="{xn - 6:.1f}" y="{B - 10}" text-anchor="end" font-size="16" font-weight="700" fill="{AMBER}">{N_AT_60_L8:,}</text>')
    # budget lines
    for val, lab, col in [(BUDGET, "16.7ms(60fps)", AMBER)]:
        o.append(f'<line x1="{L}" y1="{Y(val):.1f}" x2="{R}" y2="{Y(val):.1f}" stroke="{col}" stroke-dasharray="6 4" stroke-width="1.5"/>')
    o.append(f'<text x="{L + 8}" y="{Y(BUDGET) - 8:.1f}" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    series = [("프레임 시간(느린 쪽 5% 경계)", "frame_p95_ms", DARK),
              ("게임 스레드(중앙값)", "game_p50_ms", BLUE),
              ("GPU(중앙값)", "gpu_p50_ms", PINK),
              ("렌더 스레드(중앙값)", "render_p50_ms", TEAL)]
    for lab, col, c in series:
        vals = [v(L8, n, col) for n in NS]
        pts = " ".join(f"{X(n):.1f},{Y(val):.1f}" for n, val in zip(NS, vals))
        o.append(f'<polyline points="{pts}" fill="none" stroke="{c}" stroke-width="2.5"/>')
        for n, val in zip(NS, vals):
            o.append(f'<circle cx="{X(n):.1f}" cy="{Y(val):.1f}" r="4.5" fill="{c}"/>')
    # value labels. Every line rises to the right, so a label above-left or below-right of its point
    # stays clear of its own line. p95 goes above-left; the three threads go below-right, except GPU at 5천,
    # which sits on the game-thread point and goes above-left.
    for lab, col, c in series:
        for k, n in enumerate(NS):
            val = v(L8, n, col)
            above_left = col == "frame_p95_ms" or (col == "gpu_p50_ms" and k == 0)
            # The p95 point just under 16.7ms gets one more digit, so it does not read as the line itself.
            txt = f"{val:.2f}" if col == "frame_p95_ms" and fmt(val) == fmt(BUDGET) else fmt(val)
            if above_left:
                o.append(f'<text x="{X(n) - 8:.1f}" y="{Y(val) - 8:.1f}" text-anchor="end" '
                         f'font-size="16" font-weight="700" fill="{c}">{txt}</text>')
            else:
                o.append(f'<text x="{X(n) + 8:.1f}" y="{Y(val) + 22:.1f}" '
                         f'font-size="16" font-weight="700" fill="{c}">{fmt(val)}</text>')
    # headroom: the gap above the target point up to 16.7ms
    p95 = v(L8, 10000, "frame_p95_ms")
    o.append(f'<line x1="{xt:.1f}" y1="{Y(BUDGET):.1f}" x2="{xt:.1f}" y2="{Y(p95) - 6:.1f}" stroke="{GREEN}" stroke-width="3"/>')
    o.append(f'<text x="{xt + 8:.1f}" y="{Y(BUDGET) + 24:.1f}" font-size="16" font-weight="700" fill="{GREEN}">{fmt(BUDGET - p95)}ms 남음</text>')
    # legend
    lx = 590
    for i, (lab, _, c) in enumerate(series):
        yy = 150 + i * 30
        short = lab.split("(")[0]
        o.append(f'<line x1="{lx}" y1="{yy}" x2="{lx + 28}" y2="{yy}" stroke="{c}" stroke-width="2.5"/>'
                 f'<text x="{lx + 36}" y="{yy + 6}" font-size="16">{short}</text>')
    o.append(f'<text x="{lx}" y="{150 + 4 * 30 + 6}" font-size="16" fill="{SUB}">프레임 시간은 느린 쪽</text>')
    o.append(f'<text x="{lx}" y="{150 + 4 * 30 + 30}" font-size="16" fill="{SUB}">5% 경계, 나머지는</text>')
    o.append(f'<text x="{lx}" y="{150 + 4 * 30 + 54}" font-size="16" fill="{SUB}">중앙값이다.</text>')
    o.append(f'<text x="{lx}" y="{150 + 4 * 30 + 90}" font-size="16" fill="{SUB}">병목: 5천은 GPU,</text>')
    o.append(f'<text x="{lx}" y="{150 + 4 * 30 + 114}" font-size="16" fill="{SUB}">1만부터 게임 스레드</text>')
    # quality row: share of small-actor frames with neither an update nor an interpolation (animation budget 4 ms)
    yq = B + 92
    o.append(f'<line x1="32" y1="{yq - 18}" x2="{W - 32}" y2="{yq - 18}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{yq + 6}" font-size="16" font-weight="700">소형 액터가 애니메이션을 건너뛴 비율(갱신도 보간도 안 한 프레임)</text>')
    for n in NS:
        sk = v(L8, n, "small_skipped_ratio")
        o.append(f'<text x="{X(n):.1f}" y="{yq + 34}" text-anchor="middle" font-size="16" font-weight="700" fill="{PURPLE}">{fmt(sk * 100) if sk else "0"}%</text>')
    yb = yq + 76
    large_max = max(v(L8, n, "large_skipped_ratio") for n in NS)
    o.append(f'<line x1="32" y1="{yb - 18}" x2="{W - 32}" y2="{yb - 18}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{yb + 6}" font-size="16" fill="{SUB}">대형 액터의 같은 비율은 네 총수 모두 '
             f'{"1% 미만이다" if large_max < 0.01 else fmt(large_max * 100) + "% 이하다"}.</text>')
    o.append(f'<text x="32" y="{yb + 30}" font-size="16" fill="{SUB}">60fps는 약 2만까지다. 2만에서는 애니메이션 예산이 소형 액터를 건너뛴다.</text>')
    o.append(f'<text x="32" y="{yb + 54}" font-size="16" fill="{SUB}">1만의 GPU는 {fmt(v(L8, 10000, "gpu_p50_ms"))}ms다. 남은 여유는 GPU 쪽이 더 크다.</text>')
    write("target-headroom.svg", svg(yb + 78, o))


waterfall()
headroom()
print("wrote", sorted(os.listdir(OUT)))
for a, b, name, grp, cells in trans_rows():
    print(name, bottleneck(a), "->", bottleneck(b),
          " | ".join(f"{col} {d:+.3f} real={real} (ranges {rng(a, 10000, col):.3f} / {rng(b, 10000, col):.3f})" for lab, col, c, d, real in cells))
print("L1->L2 frame", round(v("L2-mass", 10000, "frame_p50_ms") - v("L1-full", 10000, "frame_p50_ms"), 3))
for col in ("frame_p50_ms", "game_p50_ms", "gpu_p50_ms"):
    print("L2->L8", col, round(v(L8, 10000, col) - v("L2-mass", 10000, col), 3))
print("N@60 L8:", N_AT_60_L8, "check:", round(n_at(p95_points(), BUDGET)))
print("headroom at 1만:", round(BUDGET - v(L8, 10000, "frame_p95_ms"), 3))
for n in NS:
    print("L8", n, {c: v(L8, n, c) for c in ("frame_p95_ms", "game_p50_ms", "gpu_p50_ms", "render_p50_ms",
                                             "small_skipped_ratio", "large_skipped_ratio")}, rows[(L8, n, "P1")]["bottleneck"])
