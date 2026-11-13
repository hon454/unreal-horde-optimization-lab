# Makes the SVG figures of post 05 in images/.
# Run from the repository root: py -3 -I Posts/05-simulation-lod-entity-pool/make-figures.py
# Measured values come from Measurements/Full2/summary.csv and Docs/figures/full2/full-n-at-60fps.csv.
# The entity pool timeline figure is not made here: the post uses Docs/figures/readme/entity-pool.svg (Scripts/plot-readme.py).
import csv, os
import re
from decimal import Decimal, ROUND_HALF_UP

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")

rows = {}
with open(os.path.join(ROOT, "Measurements", "Full2", "summary.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        rows[(r["variant"], int(r["n"]), r["preset"])] = r


def v(variant, n, col, preset="P1"):
    return float(rows[(variant, n, preset)][col])


with open(os.path.join(ROOT, "Docs", "figures", "full2", "full-n-at-60fps.csv"), encoding="utf-8") as f:
    NAT = {r["series"]: r for r in csv.DictReader(f)}
N_AT_60_L4 = int(NAT["4 L4-mass"]["n_p95"])
L5_ABOVE = NAT["5 L5-m4offpool"]["note_p95"] == "above range"

L4, L5, L8 = "L4-mass", "L5-m4offpool", "L8-q4final"
NS = [5000, 10000, 20000, 30000]
NLAB = ["5천", "1만", "2만", "3만"]

# Simulation LOD bands of step 5 at 10k P1 (frame averages counted by the approach processor, Lab/SimTick*, Lab/SimBand*).
PERIOD_MED = v(L5, 10000, "real_period_medium")
PERIOD_OFF = v(L5, 10000, "real_period_off")
CFG_MED = v(L5, 10000, "cfg_tick_medium")
CFG_OFF = v(L5, 10000, "cfg_tick_off")
BAND_HIGH = v(L5, 10000, "sim_band_high")
BAND_MED = v(L5, 10000, "sim_band_medium")
BAND_OFF = v(L5, 10000, "sim_band_off")

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
MONO = "Consolas, SFMono-Regular, Menlo, monospace"
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
    """Three significant figures, rounded half up from the CSV text (12.35 -> 12.4), as in the post body."""
    q = "1" if abs(x) >= 100 else "0.1" if abs(x) >= 10 else "0.01"
    return str(Decimal(repr(x)).quantize(Decimal(q), rounding=ROUND_HALF_UP))


def cnt(x):
    """Whole count, rounded half up (5996.5 -> 5,997), with thousands separators."""
    return f"{int(Decimal(repr(x)).quantize(Decimal('1'), rounding=ROUND_HALF_UP)):,}"


def fmtd(x):
    """Signed difference with three significant figures (minus sign U+2212)."""
    s = fmt(abs(x))
    return ("+" if x >= 0 else "−") + s


def title(o, t, s):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')


METRICS = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE),
           ("렌더 스레드", "render_p50_ms", TEAL), ("GPU", "gpu_p50_ms", PINK)]


def line167(o, x, top, bottom):
    o.append(f'<line x1="{x:.1f}" y1="{top}" x2="{x:.1f}" y2="{bottom}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<text x="{x - 6:.1f}" y="{bottom + 16}" text-anchor="end" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')


# ---------- 1. summary: step 4 vs step 5 at 10k ----------
def summary():
    o = []
    title(o, "호드 총수 1만에서 4단계와 5단계가 쓰는 시간",
          "오브 시점 카메라, 60초씩 세 번 잰 중앙값(ms)")
    X0, scale = 40, 600 / 17.5
    groups = [("4단계: 가까운 개체는 액터, 나머지는 VAT 인스턴스", L4),
              ("5단계: 4단계 + Simulation LOD, 엔티티 풀", L5)]
    y = 120
    top = y - 8
    for g, var in groups:
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{g}</text>')
        y += 12
        for name, col, c in METRICS:
            val = v(var, 10000, col)
            w = val * scale
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="20" rx="3" fill="{c}"/>')
            o.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{fmt(val)}</tspan>'
                     f'<tspan fill="{SUB}"> {name}</tspan></text>')
            y += 26
        y += 30
    line167(o, X0 + 16.667 * scale, top + 14, y - 30)
    y += 12
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">프레임 시간은 셋 가운데 가장 느린 쪽이 정한다. 두 단계 모두 게임 스레드다.</text>')
    write("simlod-summary.svg", svg(y + 34, o))


# ---------- 2. problem: step 4 by thread vs horde size ----------
def problem():
    o = []
    title(o, "4단계는 게임 스레드가 가장 느리고, 호드 총수와 함께 는다",
          "오브 시점 카메라, 최종 측정, 세 번 잰 중앙값(ms)")
    X0, scale = 110, 560 / 17.5
    bars = [("게임 스레드", "game_p50_ms", BLUE), ("렌더 스레드", "render_p50_ms", TEAL), ("GPU", "gpu_p50_ms", PINK)]
    lx = X0
    for i, (name, _, c) in enumerate(bars):
        o.append(f'<rect x="{lx}" y="98" width="16" height="16" rx="3" fill="{c}"/><text x="{lx + 22}" y="111" font-size="16">{name}</text>')
        lx += 150
    y = 140
    top = y - 6
    labels = []
    for n, lab in zip(NS, NLAB):
        o.append(f'<text x="{X0 - 16}" y="{y + 42}" text-anchor="end" font-size="17" font-weight="700">{lab}</text>')
        for name, col, c in bars:
            val = v(L4, n, col)
            w = val * scale
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="20" rx="3" fill="{c}"/>')
            # value labels are drawn after the 16.7 ms line, with a white halo, because that line crosses the longest label
            labels.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 16}" font-size="16" font-weight="700" fill="{c}" stroke="white" stroke-width="5" paint-order="stroke">{fmt(val)}</text>')
            y += 25
        y += 22
    line167(o, X0 + 16.667 * scale, top, y - 18)
    o.extend(labels)
    o.append(f'<text x="{X0 - 16}" y="{y + 16}" text-anchor="end" font-size="16" fill="{SUB}">호드 총수</text>')
    y += 34
    o.append(f'<line x1="32" y1="{y - 12}" x2="{W - 32}" y2="{y - 12}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 14}" font-size="16" fill="{SUB}">5천에서는 GPU가, 1만부터는 게임 스레드가 프레임 시간을 정한다.</text>')
    o.append(f'<text x="32" y="{y + 38}" font-size="16" fill="{SUB}">개체의 60%는 그리지 않는 200m 밖에 있지만, 모두 엔진 이동·조향을 매 프레임 거친다.</text>')
    write("simlod-problem.svg", svg(y + 62, o))


# ---------- 3. principle: bands and entity pool ----------
def principle():
    o = []
    title(o, "5단계가 거리에 따라 개체를 다루는 방식",
          "거리는 카메라에서 잰다. 칸의 폭은 거리에 비례하지 않는다")
    LX, CX, CW = 32, 214, 554  # label column, cells start, cells width
    edges = [0, 30, 80, 200, 500]
    cols = [CX + CW * k / 4 for k in range(5)]  # four equal columns
    y = 100
    for k, m in enumerate(edges):
        anchor = "start" if k == 0 else "end" if k == 4 else "middle"
        o.append(f'<text x="{cols[k]:.1f}" y="{y + 14}" text-anchor="{anchor}" font-size="16" fill="{SUB}">{m}m</text>')
        o.append(f'<line x1="{cols[k]:.1f}" y1="{y + 20}" x2="{cols[k]:.1f}" y2="{y + 28}" stroke="{SUB}"/>')
    y += 34
    per = lambda s: f"{s:.2f}".rstrip("0").rstrip(".")
    rows_ = [
        ("무엇으로 그리나", "Representation LOD", [(0, 1, RED, "액터", "white"), (1, 3, BLUE, "VAT 인스턴스", "white"), (3, 4, None, "그리지 않는다", SUB)]),
        ("Simulation LOD", "로직을 얼마나 자주", [(0, 2, DARK, "High · 매 프레임", "white"), (2, 3, PURPLE, "Medium", "white"), (3, 4, AMBER, "Off", "white")]),
        ("Lab 접근·범위 피해", "청크 필터를 건다", [(0, 2, DARK, "매 프레임", "white"), (2, 3, PURPLE, f"{per(CFG_MED)}초 간격", "white"), (3, 4, AMBER, f"{per(CFG_OFF)}초 간격", "white")]),
        ("엔진 이동·조향 넷", "Off 개체를 빼는 쿼리", [(0, 3, GRAY, "매 프레임 돈다", "white"), (3, 4, None, "돌지 않는다", SUB)]),
        ("자체 Off 이동", "ULabOffMoveProcessor", [(0, 3, None, "없음", SUB), (3, 4, GREEN, f"{per(CFG_OFF)}초 간격", "white")]),
    ]
    RH = 44
    for name, sub, segs in rows_:
        o.append(f'<text x="{LX}" y="{y + 19}" font-size="17" font-weight="700">{name}</text>')
        o.append(f'<text x="{LX}" y="{y + 39}" font-size="16" fill="{SUB}">{sub}</text>')
        for a, b, col, text, tc in segs:
            x0, x1 = cols[a] + 2, cols[b] - 2
            if col:
                o.append(f'<rect x="{x0:.1f}" y="{y}" width="{x1 - x0:.1f}" height="{RH - 8}" rx="4" fill="{col}"/>')
            else:
                o.append(f'<rect x="{x0:.1f}" y="{y}" width="{x1 - x0:.1f}" height="{RH - 8}" rx="4" fill="white" stroke="{GRAY}" stroke-dasharray="5 4"/>')
            o.append(f'<text x="{(x0 + x1) / 2:.1f}" y="{y + 24}" text-anchor="middle" font-size="16" font-weight="700" fill="{tc}">{text}</text>')
        y += RH + 8
    o.append(f'<text x="{CX}" y="{y + 10}" font-size="16" fill="{SUB}">호드 총수 1만에서 실제 주기: Medium {fmt(PERIOD_MED)}초, Off {fmt(PERIOD_OFF)}초</text>')
    o.append(f'<text x="{CX}" y="{y + 34}" font-size="16" fill="{SUB}">걷는 개체: High {cnt(BAND_HIGH)}, Medium {cnt(BAND_MED)}, Off {cnt(BAND_OFF)}(프레임 평균)</text>')
    y += 70
    # entity pool
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="18" font-weight="700">죽은 개체의 엔티티</text>')
    y += 30
    flows = [("4단계", GRAY, ["사망이 끝난다", "엔티티를 파괴한다", "새 엔티티를 만든다"]),
             ("5단계", BLUE, ["사망이 끝난다", "풀에 넣는다", "같은 엔티티를 다시 쓴다"])]
    BW, GAP = 200, 28
    for lab, col, steps in flows:
        o.append(f'<text x="{LX}" y="{y + 25}" font-size="17" font-weight="700">{lab}</text>')
        x = 120
        for k, s in enumerate(steps):
            o.append(f'<rect x="{x}" y="{y}" width="{BW}" height="38" rx="6" fill="{CARD}" stroke="{col}" stroke-width="2"/>')
            o.append(f'<text x="{x + BW / 2}" y="{y + 25}" text-anchor="middle" font-size="16">{s}</text>')
            if k < len(steps) - 1:
                ax = x + BW + 4
                o.append(f'<path d="M{ax} {y + 19} L{ax + GAP - 10} {y + 19}" stroke="{SUB}" stroke-width="2"/>')
                o.append(f'<path d="M{ax + GAP - 16} {y + 13} L{ax + GAP - 8} {y + 19} L{ax + GAP - 16} {y + 25}" fill="none" stroke="{SUB}" stroke-width="2"/>')
            x += BW + GAP
        y += 50
    o.append(f'<text x="120" y="{y + 10}" font-size="16" fill="{SUB}">두 경우 모두 새 위치는 맵 가장자리 고리(490–500m)이고 일련번호도 같다.</text>')
    write("simlod-principle.svg", svg(y + 36, o))


# ---------- 4. result: frame p50 vs horde size, step 4 vs step 5 ----------
def result():
    o = []
    title(o, "5단계의 이득은 호드 총수와 함께 커진다",
          "오브 시점 카메라, 프레임 시간 중앙값(ms), 세 번 잰 중앙값")
    T, B, L, R = 130, 450, 70, 560
    ymax = 20
    X = lambda n: L + n / 32000 * (R - L)
    Y = lambda val: B - val / ymax * (B - T)
    for t in [0, 5, 10, 15, 20]:
        o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
        o.append(f'<text x="{L - 10}" y="{Y(t) + 6:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{t}</text>')
    o.append(f'<line x1="{L}" y1="{B}" x2="{R}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<line x1="{L}" y1="{T}" x2="{L}" y2="{B}" stroke="{SUB}"/>')
    y167 = Y(16.667)
    o.append(f'<line x1="{L}" y1="{y167:.1f}" x2="{R}" y2="{y167:.1f}" stroke="{AMBER}" stroke-dasharray="6 4" stroke-width="1.5"/>')
    o.append(f'<text x="{L + 8}" y="{y167 - 8:.1f}" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    a = [v(L4, n, "frame_p50_ms") for n in NS]
    b = [v(L5, n, "frame_p50_ms") for n in NS]
    for vals, col, dash in [(a, GRAY, True), (b, BLUE, False)]:
        pts = " ".join(f"{X(n):.1f},{Y(val):.1f}" for n, val in zip(NS, vals))
        da = ' stroke-dasharray="6 4"' if dash else ""
        o.append(f'<polyline points="{pts}" fill="none" stroke="{col}" stroke-width="2.5"{da}/>')
        for n, val in zip(NS, vals):
            o.append(f'<circle cx="{X(n):.1f}" cy="{Y(val):.1f}" r="4.5" fill="{col}"/>')
    for k, (n, va, vb) in enumerate(zip(NS, a, b)):
        # both lines rise to the right: the step 4 label goes above-left and the step 5 label below-right of its point
        o.append(f'<text x="{X(n) - 8:.1f}" y="{Y(va) - 10:.1f}" text-anchor="end" font-size="16" font-weight="700" fill="{DARK}">{fmt(va)}</text>')
        o.append(f'<text x="{X(n) + 8:.1f}" y="{Y(vb) + 24:.1f}" text-anchor="start" font-size="16" font-weight="700" fill="{BLUE}">{fmt(vb)}</text>')
    o.append(f'<text x="{L + 44}" y="{B + 26}" text-anchor="end" font-size="16" fill="{SUB}">호드 총수</text>')
    o.append(f'<text x="{L + 44}" y="{B + 50}" text-anchor="end" font-size="16" fill="{BLUE}">차이</text>')
    for n, lab, va, vb in zip(NS, NLAB, a, b):
        o.append(f'<line x1="{X(n):.1f}" y1="{B}" x2="{X(n):.1f}" y2="{B + 5}" stroke="{SUB}"/>')
        o.append(f'<text x="{X(n):.1f}" y="{B + 26}" text-anchor="middle" font-size="16">{lab}</text>')
        o.append(f'<text x="{X(n):.1f}" y="{B + 50}" text-anchor="middle" font-size="16" font-weight="700" fill="{BLUE}">{fmtd(vb - va)}</text>')
    lx = 590
    o.append(f'<line x1="{lx}" y1="160" x2="{lx + 28}" y2="160" stroke="{GRAY}" stroke-width="2.5" stroke-dasharray="6 4"/><text x="{lx + 36}" y="166" font-size="16">4단계</text>')
    o.append(f'<line x1="{lx}" y1="192" x2="{lx + 28}" y2="192" stroke="{BLUE}" stroke-width="2.5"/><text x="{lx + 36}" y="198" font-size="16">5단계</text>')
    o.append(f'<text x="{lx}" y="238" font-size="16" fill="{SUB}">5천은 두 단계 모두</text>')
    o.append(f'<text x="{lx}" y="260" font-size="16" fill="{SUB}">GPU가 정해 같다</text>')
    yb = B + 96
    o.append(f'<line x1="32" y1="{yb - 18}" x2="{W - 32}" y2="{yb - 18}" stroke="{LINE}"/>')
    p95_30k = v(L5, 30000, "frame_p95_ms")
    o.append(f'<text x="32" y="{yb + 6}" font-size="16" fill="{SUB}">60fps를 지키는 최대 개체 수(느린 쪽 5% 경계 기준): 4단계 {N_AT_60_L4:,}.</text>')
    tail = f"5단계는 3만에서도 느린 쪽 5% 경계가 {fmt(p95_30k)}ms라 한계에 닿지 않았다." if L5_ABOVE else ""
    o.append(f'<text x="32" y="{yb + 30}" font-size="16" fill="{SUB}">{tail}</text>')
    write("simlod-result.svg", svg(yb + 52, o))


# ---------- 5. ablation: remove one technique from the final configuration ----------
def ablation():
    o = []
    title(o, "최종 구성에서 하나씩 빼 보면",
          "호드 총수 1만, 오브 시점 카메라, 60초씩 세 번 잰 프레임 시간(ms)")
    items = [("최종 구성", L8, DARK), ("Simulation LOD를 뺐다", "A8-simlod", BLUE), ("엔티티 풀을 뺐다", "A8-pool", GREEN)]
    X0, scale = 40, 620 / 10.5
    y = 116
    base = v(L8, 10000, "frame_p50_ms")
    base_lo, base_hi = v(L8, 10000, "frame_p50_ms_min"), v(L8, 10000, "frame_p50_ms_max")
    for name, var, c in items:
        val = v(var, 10000, "frame_p50_ms")
        lo, hi = v(var, 10000, "frame_p50_ms_min"), v(var, 10000, "frame_p50_ms_max")
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{name}</text>')
        y += 10
        w = val * scale
        o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="24" rx="3" fill="{c}"/>')
        # range whisker (min to max of the three runs)
        o.append(f'<line x1="{X0 + lo * scale:.1f}" y1="{y + 12}" x2="{X0 + hi * scale:.1f}" y2="{y + 12}" stroke="{INK}" stroke-width="2"/>')
        for e in (lo, hi):
            o.append(f'<line x1="{X0 + e * scale:.1f}" y1="{y + 4}" x2="{X0 + e * scale:.1f}" y2="{y + 20}" stroke="{INK}" stroke-width="2"/>')
        label = f"{fmt(val)}"
        if var != L8:
            d = val - base
            sig = abs(d) > max(hi - lo, base_hi - base_lo)
            label += f"  ({fmtd(d)}{'' if sig else ', 세 번의 범위 안'})"
        o.append(f'<text x="{X0 + hi * scale + 12:.1f}" y="{y + 18}" font-size="16" font-weight="700" fill="{c}">{label}</text>')
        y += 56
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">막대는 중앙값, 가로선은 세 번 잰 값의 범위다. 병목은 세 구성 모두 게임 스레드다.</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">최종 구성의 200m 안은 액터와 ISKM이고, 200m 밖은 입자가 그린다.</text>')
    write("simlod-ablation.svg", svg(y + 60, o))


summary()
problem()
principle()
result()
ablation()
print("wrote", sorted(os.listdir(OUT)))
print("N@60 L4:", N_AT_60_L4, "L5 above range:", L5_ABOVE)
print("L5 10k: period med", PERIOD_MED, "off", PERIOD_OFF, "bands", BAND_HIGH, BAND_MED, BAND_OFF)
for n in NS:
    print(n, "L4", v(L4, n, "frame_p50_ms"), "L5", v(L5, n, "frame_p50_ms"), "diff", round(v(L5, n, "frame_p50_ms") - v(L4, n, "frame_p50_ms"), 3))
