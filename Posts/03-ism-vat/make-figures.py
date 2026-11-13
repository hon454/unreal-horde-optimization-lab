# Makes the SVG figures of post 03 in images/.
# Run from the repository root: py -3 -I Posts/03-ism-vat/make-figures.py
# Measured values come from Measurements/Full3/summary.csv and Docs/figures/full3/full-n-at-60fps.csv.
# Trace values (one trace per step, 25-75 s window) are constants below; their source is in measurements.md.
import csv, os
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


with open(os.path.join(ROOT, "Docs", "figures", "full3", "full-n-at-60fps.csv"), encoding="utf-8") as f:
    NAT = {r["series"]: r for r in csv.DictReader(f)}
N_AT_60_L2 = int(NAT["2 L2-mass"]["n_p95"])
L3_ABOVE = NAT["3 L3-vatcull"]["note_p95"] == "above range"

L2, L3 = "L2-mass", "L3-vatcull"
NS = [5000, 10000, 20000, 30000]
NLAB = ["5천", "1만", "2만", "3만"]

# Instance count drawn at 10k P1 (frame average of the VAT processor's band counts, Lab/IsmBand0-3).
DRAWN_L3 = sum(v(L3, 10000, f"ism_band{i}") for i in range(4))
# Expected share of entities inside 200 m: density ~ 1/r, so band share = band width / R (measurement.md "확정값").
KEEP = 200.0 / 500.0

# Trace values, ms per frame, 10k P1, 25-75 s, Full3 traces (build df41c13; step 3 serial is r2). measurements.md "트레이스".
# (label, thread, step 2, step 3). Game thread from the serial traces, render thread and GPU from the parallel traces.
# Each value is the timer's total in the window (seconds, Incl for the game thread, Excl otherwise) x 1000 / frames in the window.
TRACE = [
    ("쌓은 위치를 ISM 컴포넌트에 넣는다", "게임 스레드 · EndVisualChanges", 12420.331 / 5008, 7965.183 / 6454),
    ("디스턴스 필드 장면을 갱신한다", "렌더 스레드 · PrepareDistanceFieldScene", 13856.630 / 5157, 11044.924 / 6472),
    ("그림자를 그린다", "GPU · ShadowDepths", 12758.173 / 5157, 10460.254 / 6472),
    ("색과 재질을 그린다", "GPU · BasePass", 6676.235 / 5157, 5341.201 / 6472),
    ("움직임 벡터를 그린다", "GPU · RenderVelocities", 7057.009 / 5157, 4356.847 / 6472),
]

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


def title(o, t, s):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')


METRICS = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE),
           ("렌더 스레드", "render_p50_ms", TEAL), ("GPU", "gpu_p50_ms", PINK)]


# ---------- 1. summary: step 2 vs step 3 at 10k ----------
def summary():
    o = []
    title(o, "호드 총수 1만에서 2단계와 3단계가 쓰는 시간",
          "오브 시점 카메라, 60초씩 세 번 잰 중앙값(ms)")
    X0, scale = 40, 600 / 17.5
    groups = [("2단계: 로직을 Mass로, 정지 인스턴스", L2), ("3단계: VAT 인스턴스, 200m 밖 지우기", L3)]
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
    x167 = X0 + 16.667 * scale
    o.append(f'<line x1="{x167:.1f}" y1="{top + 14}" x2="{x167:.1f}" y2="{y - 14}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<text x="{x167 - 6:.1f}" y="{y - 2}" text-anchor="end" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    y += 30
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">프레임 시간은 셋 가운데 가장 느린 쪽이 정한다. 두 단계 모두 GPU다.</text>')
    write("vat-summary.svg", svg(y + 34, o))


# ---------- 2. principle: what runs where ----------
def principle():
    o = []
    title(o, "VAT 인스턴스 하나가 움직이는 길",
          "굽기는 에디터에서 한 번, 나머지 두 칸은 매 프레임 돈다")
    boxes = [
        (PURPLE, "굽기 · 에디터에서 한 번", "AnimToTexture, Bone 모드",
         ["걷기·피격·죽기 클립을 초당 30프레임으로 굽는다",
          "프레임마다 뼈의 위치·회전을 텍스처 한 줄에 쓴다",
          "정점마다 붙은 뼈와 가중치를 따로 쓴다"],
         "Scripts/bake-vat.py"),
        (BLUE, "매 프레임 · 게임 스레드", "ULabVatISMProcessor",
         ["개체의 상태와 그 상태에 들어간 시각을 읽는다",
          "재생할 프레임 번호 Frame, PrevFrame을 계산한다",
          "위치와 함께 인스턴스 커스텀 데이터로 쌓는다"],
         "LabVatProcessor.cpp"),
        (PINK, "매 프레임 · GPU", "머티리얼 함수 MF_BoneAnimation",
         ["커스텀 데이터의 Frame으로 텍스처의 줄을 고른다",
          "그 줄의 뼈 자세로 정점을 옮긴다",
          "스켈레탈 메시와 AnimBP 없이 정적 메시가 움직인다"],
         "MI_LabMinionVAT_Bone"),
    ]
    y, RH, GAP, CX, CW = 104, 150, 26, 32, W - 64
    for i, (col, head, who, lines, src) in enumerate(boxes):
        o.append(f'<rect x="{CX}" y="{y}" width="{CW}" height="{RH}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
        o.append(f'<rect x="{CX}" y="{y}" width="6" height="{RH}" rx="3" fill="{col}"/>')
        o.append(f'<text x="{CX + 24}" y="{y + 32}" font-size="18" font-weight="700" fill="{col}">{head}</text>')
        o.append(f'<text x="{CX + CW - 18}" y="{y + 32}" text-anchor="end" font-family="{MONO}" font-size="16" fill="{SUB}">{who}</text>')
        for k, line in enumerate(lines):
            o.append(f'<text x="{CX + 24}" y="{y + 62 + k * 26}" font-size="17">{line}</text>')
        o.append(f'<text x="{CX + CW - 18}" y="{y + RH - 14}" text-anchor="end" font-family="{MONO}" font-size="16" fill="{SUB}">{src}</text>')
        if i < len(boxes) - 1:
            ax = W / 2
            o.append(f'<path d="M{ax} {y + RH + 3} L{ax} {y + RH + GAP - 5}" stroke="{SUB}" stroke-width="2"/>')
            o.append(f'<path d="M{ax - 6} {y + RH + GAP - 11} L{ax} {y + RH + GAP - 3} L{ax + 6} {y + RH + GAP - 11}" fill="none" stroke="{SUB}" stroke-width="2"/>')
        y += RH + GAP
    # distance bands, step 2 vs step 3 (horizontal distance from the orb, 0 to 500 m)
    y += 10
    o.append(f'<text x="32" y="{y}" font-size="18" font-weight="700">거리에 따라 무엇으로 그리나(Representation LOD)</text>')
    y += 22
    BX, BW = 120, W - 152
    X = lambda m: BX + m / 500.0 * BW
    for lab, segs in [("2단계", [(0, 500, GRAY, "정지 인스턴스(애니메이션 없음)", "white")]),
                      ("3단계", [(0, 200, BLUE, "VAT 인스턴스", "white"), (200, 500, None, "그리지 않는다", SUB)])]:
        o.append(f'<text x="32" y="{y + 25}" font-size="17" font-weight="700">{lab}</text>')
        for a, b, col, text, tc in segs:
            if col:
                o.append(f'<rect x="{X(a):.1f}" y="{y}" width="{X(b) - X(a):.1f}" height="38" rx="4" fill="{col}"/>')
            else:
                o.append(f'<rect x="{X(a):.1f}" y="{y}" width="{X(b) - X(a):.1f}" height="38" rx="4" fill="white" stroke="{GRAY}" stroke-dasharray="5 4"/>')
            o.append(f'<text x="{(X(a) + X(b)) / 2:.1f}" y="{y + 25}" text-anchor="middle" font-size="16" font-weight="700" fill="{tc}">{text}</text>')
        y += 50
    for m in [0, 30, 80, 200, 500]:
        o.append(f'<line x1="{X(m):.1f}" y1="{y - 8}" x2="{X(m):.1f}" y2="{y}" stroke="{SUB}"/>')
        anchor = "end" if m in (0, 500) else "middle"  # 0m ends at its tick so it does not run into 30m
        o.append(f'<text x="{X(m) + (6 if m == 0 else 0):.1f}" y="{y + 20}" text-anchor="{anchor}" font-size="16" fill="{SUB}">{m}m</text>')
    y += 50
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">거리는 카메라에서 잰다. 맵 반경 500m에서 개체의 60%가 200m 밖에 있다.</text>')
    o.append(f'<text x="32" y="{y + 24}" font-size="16" fill="{SUB}">3단계는 30m·80m 경계를 두지만 세 구간 모두 VAT 인스턴스로 그린다.</text>')
    write("vat-principle.svg", svg(y + 50, o))


def line_chart(o, series, T, B, L, R, ymax, yticks, ylab):
    """series: list of (label, values, color, dash). x is NS (linear)."""
    X = lambda n: L + (n - 0) / 32000 * (R - L)
    Y = lambda val: B - val / ymax * (B - T)
    o.append(f'<text x="32" y="{T - 18}" font-size="18" font-weight="700">{ylab}</text>')
    for t in yticks:
        o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
        o.append(f'<text x="{L - 10}" y="{Y(t) + 6:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{t}</text>')
    for n, lab in zip(NS, NLAB):
        o.append(f'<line x1="{X(n):.1f}" y1="{B}" x2="{X(n):.1f}" y2="{B + 5}" stroke="{SUB}"/>')
        o.append(f'<text x="{X(n):.1f}" y="{B + 26}" text-anchor="middle" font-size="16">{lab}</text>')
    o.append(f'<line x1="{L}" y1="{B}" x2="{R}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<line x1="{L}" y1="{T}" x2="{L}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<text x="{(L + R) / 2}" y="{B + 52}" text-anchor="middle" font-size="16" fill="{SUB}">호드 총수</text>')
    y167 = Y(16.667)
    o.append(f'<line x1="{L}" y1="{y167:.1f}" x2="{R}" y2="{y167:.1f}" stroke="{AMBER}" stroke-dasharray="6 4" stroke-width="1.5"/>')
    o.append(f'<text x="{L + 8}" y="{y167 - 8:.1f}" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    for lab, vals, col, dash in series:
        pts = " ".join(f"{X(n):.1f},{Y(val):.1f}" for n, val in zip(NS, vals))
        da = ' stroke-dasharray="6 4"' if dash else ""
        o.append(f'<polyline points="{pts}" fill="none" stroke="{col}" stroke-width="2.5"{da}/>')
        for n, val in zip(NS, vals):
            o.append(f'<circle cx="{X(n):.1f}" cy="{Y(val):.1f}" r="4.5" fill="{col}"/>')
    return X, Y


# ---------- 3. problem: step 2 by thread vs horde size ----------
def problem():
    o = []
    title(o, "2단계의 스레드별 시간은 호드 총수와 함께 는다",
          "오브 시점 카메라, 최종 측정, 세 번 잰 중앙값")
    T, B, L, R = 130, 470, 70, 560
    series = [("GPU", [v(L2, n, "gpu_p50_ms") for n in NS], PINK, False),
              ("렌더 스레드", [v(L2, n, "render_p50_ms") for n in NS], TEAL, False),
              ("게임 스레드", [v(L2, n, "game_p50_ms") for n in NS], BLUE, False)]
    X, Y = line_chart(o, series, T, B, L, R, 22, [0, 5, 10, 15, 20], "스레드별 시간 중앙값(ms)")
    # value labels sit off the lines, which all rise to the right: above-left (ul), below-right (br) or above-right (ar)
    side = {"GPU": ["ul", "ul", "ul", "ul"], "렌더 스레드": ["ul", "ul", "br", "br"], "게임 스레드": ["br", "br", "ml", "br"]}
    pos = {"ul": (-8, -12, "end"), "br": (8, 24, "start"), "ml": (-10, -2, "end")}  # ml: left of the point, between the GPU and 렌더 스레드 lines
    for lab, vals, col, _ in series:
        for k, (n, val) in enumerate(zip(NS, vals)):
            dx, dy, anc = pos[side[lab][k]]
            o.append(f'<text x="{X(n) + dx:.1f}" y="{Y(val) + dy:.1f}" text-anchor="{anc}" '
                     f'font-size="16" font-weight="700" fill="{col}">{fmt(val)}</text>')
    lx = 590
    for i, (lab, _, col, _) in enumerate(series):
        yy = 160 + i * 32
        o.append(f'<line x1="{lx}" y1="{yy}" x2="{lx + 28}" y2="{yy}" stroke="{col}" stroke-width="2.5"/>'
                 f'<text x="{lx + 36}" y="{yy + 6}" font-size="16">{lab}</text>')
    o.append(f'<text x="{lx}" y="{160 + 3 * 32 + 6}" font-size="16" fill="{SUB}">병목은 모두 GPU</text>')
    yb = B + 84
    o.append(f'<line x1="32" y1="{yb - 18}" x2="{W - 32}" y2="{yb - 18}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{yb + 6}" font-size="16" fill="{SUB}">2단계는 거리와 상관없이 모든 개체를 인스턴스로 올린다.</text>')
    o.append(f'<text x="32" y="{yb + 30}" font-size="16" fill="{SUB}">200m에서 미니언의 키는 화면 높이의 0.8%다.</text>')
    write("vat-problem.svg", svg(yb + 52, o))


# ---------- 4. result: expected vs actual per timer ----------
def expected():
    o = []
    title(o, "인스턴스 수에 비례하는 일: 예상과 실제",
          "호드 총수 1만, 오브 시점 카메라, 트레이스 한 회의 프레임당 시간(ms)")
    X0, scale = 40, 470 / 2.8
    y = 116
    for lab, sub, a, b in TRACE:
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{lab}</text>')
        o.append(f'<text x="{W - 32}" y="{y}" text-anchor="end" font-family="{MONO}" font-size="16" fill="{SUB}">{sub}</text>')
        y += 10
        for name, val, style in [("2단계", a, "l2"), ("예상", a * KEEP, "exp"), ("3단계", b, "l3")]:
            w = val * scale
            if style == "l2":
                o.append(f'<rect x="{X0 + 70}" y="{y}" width="{w:.1f}" height="18" rx="3" fill="{GRAY}"/>')
                tc = SUB
            elif style == "exp":
                o.append(f'<rect x="{X0 + 70}" y="{y}" width="{w:.1f}" height="18" rx="3" fill="white" stroke="{DARK}" stroke-width="1.5" stroke-dasharray="4 3"/>')
                tc = DARK
            else:
                o.append(f'<rect x="{X0 + 70}" y="{y}" width="{w:.1f}" height="18" rx="3" fill="{BLUE}"/>')
                tc = BLUE
            o.append(f'<text x="{X0}" y="{y + 15}" font-size="16" fill="{tc}">{name}</text>')
            o.append(f'<text x="{X0 + 78 + w:.1f}" y="{y + 15}" font-size="16" font-weight="700" fill="{tc}">{fmt(val)}</text>')
            y += 23
        y += 22
    o.append(f'<line x1="32" y1="{y - 12}" x2="{W - 32}" y2="{y - 12}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 14}" font-size="16" fill="{SUB}">예상 = 2단계 × {KEEP:.1f}. 200m 밖 60%를 지워 인스턴스가 40%로 준다고 본 값이다.</text>')
    o.append(f'<text x="32" y="{y + 38}" font-size="16" fill="{SUB}">실제로 올린 인스턴스는 프레임 평균 {DRAWN_L3:,.0f}개였다. 다섯 칸 모두 실제가 예상보다 크다.</text>')
    write("vat-expected.svg", svg(y + 62, o))


# ---------- 5. result: frame p95 vs horde size ----------
def maxn():
    o = []
    title(o, "60fps를 지키는 최대 개체 수",
          "오브 시점 카메라, 프레임 시간의 느린 쪽 5% 경계, 세 번 잰 중앙값")
    T, B, L, R = 130, 450, 70, 560
    p2 = [v(L2, n, "frame_p95_ms") for n in NS]
    p3 = [v(L3, n, "frame_p95_ms") for n in NS]
    series = [("2단계", p2, GRAY, True), ("3단계", p3, BLUE, False)]
    X, Y = line_chart(o, series, T, B, L, R, 25, [0, 5, 10, 15, 20, 25], "프레임 시간 느린 쪽 5% 경계(ms)")
    nx = X(N_AT_60_L2)
    for k, (n, a, b) in enumerate(zip(NS, p2, p3)):
        if k == 2:
            # 2만: the limit marker's dotted line runs just right of the point, so both labels start right of that line
            xa, xb, aa, ab, ya = nx + 8, nx + 8, "start", "start", Y(a) + 24
        elif k < 2:
            # the rising 2단계 line passes right of and below its point, so that label goes above-left
            xa, xb, aa, ab, ya = X(n) - 8, X(n) + 8, "end", "start", Y(a) - 10
        else:
            xa, xb, aa, ab, ya = X(n) - 8, X(n) - 8, "end", "end", Y(a) - 10
        o.append(f'<text x="{xa:.1f}" y="{ya:.1f}" text-anchor="{aa}" font-size="16" font-weight="700" fill="{DARK}">{fmt(a)}</text>')
        o.append(f'<text x="{xb:.1f}" y="{Y(b) + 24:.1f}" text-anchor="{ab}" font-size="16" font-weight="700" fill="{BLUE}">{fmt(b)}</text>')
    y167 = Y(16.667)
    o.append(f'<circle cx="{nx:.1f}" cy="{y167:.1f}" r="5.5" fill="white" stroke="{AMBER}" stroke-width="2.5"/>')
    o.append(f'<path d="M{nx:.1f} {y167 + 8:.1f} L{nx:.1f} {B - 4}" stroke="{AMBER}" stroke-width="1.5" stroke-dasharray="3 3"/>')
    o.append(f'<text x="{nx + 8:.1f}" y="{B - 10}" text-anchor="start" font-size="16" font-weight="700" fill="{AMBER}">{N_AT_60_L2:,}</text>')
    lx = 590
    o.append(f'<line x1="{lx}" y1="160" x2="{lx + 28}" y2="160" stroke="{GRAY}" stroke-width="2.5" stroke-dasharray="6 4"/><text x="{lx + 36}" y="166" font-size="16">2단계</text>')
    o.append(f'<line x1="{lx}" y1="192" x2="{lx + 28}" y2="192" stroke="{BLUE}" stroke-width="2.5"/><text x="{lx + 36}" y="198" font-size="16">3단계</text>')
    o.append(f'<circle cx="{lx + 14}" cy="224" r="5.5" fill="white" stroke="{AMBER}" stroke-width="2.5"/><text x="{lx + 36}" y="230" font-size="16">2단계의 한계</text>')
    if L3_ABOVE:
        o.append(f'<text x="{lx}" y="270" font-size="16" fill="{BLUE}" font-weight="700">3단계는 잰 범위</text>')
        o.append(f'<text x="{lx}" y="292" font-size="16" fill="{BLUE}" font-weight="700">(3만까지) 안에서</text>')
        o.append(f'<text x="{lx}" y="314" font-size="16" fill="{BLUE}" font-weight="700">한계에 닿지 않았다</text>')
    write("vat-maxn.svg", svg(B + 80, o))


summary()
principle()
problem()
expected()
maxn()
print("wrote", sorted(os.listdir(OUT)))
print("drawn instances L3 10k P1:", round(DRAWN_L3, 2), "N@60 L2:", N_AT_60_L2, "L3 above range:", L3_ABOVE)
for lab, sub, a, b in TRACE:
    print(f"{sub}: L2 {a:.4f} exp {a * KEEP:.4f} L3 {b:.4f}")
