# Makes the SVG figures of post 07 in images/.
# Run from the repository root: py -3 -I Posts/07-niagara-crowd/make-figures.py
# Measured values come from Measurements/Full2/summary.csv and Docs/figures/full2/full-n-at-60fps.csv.
# No trace values are drawn (trace values are in the post text and measurements.md only).
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


def rng(variant, n, col, preset="P1"):
    r = rows[(variant, n, preset)]
    return float(r[col + "_max"]) - float(r[col + "_min"])


with open(os.path.join(ROOT, "Docs", "figures", "full2", "full-n-at-60fps.csv"), encoding="utf-8") as f:
    NAT = {r["series"]: r for r in csv.DictReader(f)}
N_AT_60_L6 = int(NAT["6 L6-m5bounds"]["n_p95"])
N_AT_60_L7 = int(NAT["7 L7-q2crowd"]["n_p95"])

L6, L7, L8, AFX, AFAR = "L6-m5bounds", "L7-q2crowd", "L8-q4final", "A8-crowdfx", "A8-farcull"
NS = [5000, 10000, 20000, 30000]
NLAB = ["5천", "1만", "2만", "3만"]

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
    """Three significant figures, rounded half up from the decimal text (2.345 -> 2.35), as in the post body."""
    d = Decimal(repr(round(x, 6)))
    a = abs(d)
    q = "1" if a >= 100 else "0.1" if a >= 10 else "0.01" if a >= 1 else "0.001"
    return str(d.quantize(Decimal(q), rounding=ROUND_HALF_UP))


def sfmt(x):
    """Signed, with a real minus sign (U+2212) as in the post body."""
    s = fmt(x)
    return "−" + s[1:] if s.startswith("-") else "+" + s


def count(x):
    """A count, rounded half up and written with a thousands comma (counts are never shortened)."""
    return f"{int(Decimal(repr(x)).quantize(Decimal('1'), rounding=ROUND_HALF_UP)):,}"


def title(o, t, s):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')


METRICS = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE),
           ("렌더 스레드", "render_p50_ms", TEAL), ("GPU", "gpu_p50_ms", PINK)]


def drawn_counts(var, n=10000):
    """Mean entities drawn as actors and as ISKM, and the rest (not drawn). The ISKM count is the sum of the four
    distance-band counts (Lab/IsmBand0..3); actors are Lab/Actors. Every entity outside the pool is one of the three."""
    actors = v(var, n, "actors_mean")
    iskm = sum(v(var, n, f"ism_band{i}") for i in range(4))
    return actors, iskm, n - actors - iskm


# ---------- 1. summary: step 6 vs step 7 at 10k ----------
def summary():
    o = []
    title(o, "호드 총수 1만에서 6단계와 7단계가 쓰는 시간",
          "오브 시점 카메라, 60초씩 세 번 잰 중앙값(ms)")
    X0, scale = 40, 600 / 17.5
    groups = [("6단계: 200m 밖을 그리지 않는다", L6), ("7단계: 200m 밖을 Niagara 입자로 채운다", L7)]
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
    dg = v(L7, 10000, "gpu_p50_ms") - v(L6, 10000, "gpu_p50_ms")
    df = v(L7, 10000, "frame_p50_ms") - v(L6, 10000, "frame_p50_ms")
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">프레임 시간은 셋 가운데 가장 느린 쪽이 정한다. 두 단계 모두 게임 스레드다.</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">GPU는 {sfmt(dg)}ms 늘었지만 게임 스레드보다 짧다. 프레임 시간은 {sfmt(df)}ms다.</text>')
    write("ncf-summary.svg", svg(y + 58, o))


# ---------- 2. problem: what step 6 draws, and the GPU margin ----------
def problem():
    o = []
    title(o, "6단계는 호드의 절반 넘게를 그리지 않는다",
          "호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값(프레임 평균 개수와 ms)")
    actors, iskm, none = drawn_counts(L6)
    X0, X1 = 40, W - 40
    scale = (X1 - X0) / 10000
    y = 112
    o.append(f'<text x="{X0}" y="{y}" font-size="18" font-weight="700">개체 1만을 무엇으로 그리나</text>')
    y += 16
    parts = [("액터", actors, RED), ("ISKM", iskm, BLUE), ("그리지 않음(200m 밖)", none, GRAY)]
    x = X0
    for name, val, c in parts:
        w = val * scale
        fill = c if name != "그리지 않음(200m 밖)" else "white"
        stroke = f' stroke="{c}" stroke-width="2" stroke-dasharray="6 4"' if fill == "white" else ""
        o.append(f'<rect x="{x:.1f}" y="{y}" width="{w:.1f}" height="40" fill="{fill}"{stroke}/>')
        x += w
    # labels under the bar
    x = X0
    ly = y + 68
    lab_x = [X0, X0 + 130, X0 + (actors + iskm) * scale + 20]
    for (name, val, c), lx in zip(parts, lab_x):
        o.append(f'<text x="{lx:.1f}" y="{ly}" font-size="16"><tspan font-weight="700" fill="{c if c != GRAY else DARK}">{count(val)}</tspan>'
                 f'<tspan fill="{SUB}"> {name}</tspan></text>')
    o.append(f'<path d="M{X0 + actors * scale / 2:.1f} {y + 42} L{X0 + 10:.1f} {ly - 18}" stroke="{RED}" stroke-width="1.5" fill="none"/>')
    y = ly + 56
    # time margin
    o.append(f'<text x="{X0}" y="{y}" font-size="18" font-weight="700">같은 실행의 스레드별 시간(ms)</text>')
    y += 16
    tscale = 560 / 10.0
    game, gpu = v(L6, 10000, "game_p50_ms"), v(L6, 10000, "gpu_p50_ms")
    for name, val, c in [("게임 스레드", game, BLUE), ("GPU", gpu, PINK)]:
        w = val * tscale
        o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="22" rx="3" fill="{c}"/>')
        o.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 17}" font-size="16"><tspan font-weight="700" fill="{c}">{fmt(val)}</tspan>'
                 f'<tspan fill="{SUB}"> {name}</tspan></text>')
        y += 30
    gx0, gx1 = X0 + gpu * tscale, X0 + game * tscale
    o.append(f'<rect x="{gx0:.1f}" y="{y - 30}" width="{gx1 - gx0:.1f}" height="22" fill="white" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<text x="{gx0:.1f}" y="{y + 16}" font-size="16" font-weight="700" fill="{AMBER}">GPU의 여유 {fmt(game - gpu)}ms</text>')
    y += 48
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">그리지 않는 개체도 엔티티로 살아 걷는다. 화면의 그 자리는 빈 땅이다.</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">GPU는 여유만큼 일을 더 해도 프레임 시간을 늘리지 않는다.</text>')
    write("ncf-problem.svg", svg(y + 58, o))


# ---------- 3. principle: distance bands and the data flow of one frame ----------
def principle():
    o = []
    title(o, "200m 밖의 엔티티를 입자로 그리는 길",
          "위: 거리 구간마다 그리는 것. 아래: 한 프레임에 엔티티의 자리가 입자가 되는 길")
    X0 = 40
    segs = [(0, 30, 100), (30, 200, 190), (200, 500, 230), (500, 1000, 200)]
    lanes = [
        ("6단계", ["액터", "ISKM", "그리지 않음", "빈 땅"], [RED, BLUE, None, None]),
        ("7단계", ["액터", "ISKM", "엔티티 구동 입자", "독립 입자"], [RED, BLUE, PINK, PURPLE]),
    ]
    y = 112
    for lane, labels, cols in lanes:
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{lane}</text>')
        y += 10
        x = X0
        for (a, b, w), lab, c in zip(segs, labels, cols):
            if c:
                o.append(f'<rect x="{x}" y="{y}" width="{w - 4}" height="38" rx="4" fill="{c}" fill-opacity="0.16" stroke="{c}" stroke-width="1.5"/>')
                o.append(f'<text x="{x + (w - 4) / 2:.1f}" y="{y + 25}" text-anchor="middle" font-size="16" font-weight="700" fill="{c}">{lab}</text>')
            else:
                o.append(f'<rect x="{x}" y="{y}" width="{w - 4}" height="38" rx="4" fill="white" stroke="{GRAY}" stroke-width="1.5" stroke-dasharray="5 4"/>')
                o.append(f'<text x="{x + (w - 4) / 2:.1f}" y="{y + 25}" text-anchor="middle" font-size="16" fill="{SUB}">{lab}</text>')
            x += w
        y += 60
    # ticks
    ty = y - 14
    x = X0
    ticks = [(X0, "0")]
    for a, b, w in segs:
        x += w
        ticks.append((x - 2, f"{b:,}m"))
    for tx, lab in ticks:
        anchor = "start" if tx == X0 else "end" if lab == "1,000m" else "middle"
        o.append(f'<line x1="{tx}" y1="{ty - 6}" x2="{tx}" y2="{ty + 2}" stroke="{SUB}"/>')
        o.append(f'<text x="{tx}" y="{ty + 22}" text-anchor="{anchor}" font-size="16" fill="{SUB}">{lab}</text>')
    y = ty + 48
    o.append(f'<text x="{X0}" y="{y}" font-size="16" fill="{SUB}">0~200m는 카메라에서, 500m·1,000m는 오브에서 잰 거리다. 엔티티는 오브 500m 안에만 있다.</text>')
    y += 24
    o.append(f'<text x="{X0}" y="{y}" font-size="16" fill="{SUB}">엔티티는 카메라 188m 안에 들어와야 ISKM이 된다(표현 LOD의 히스테리시스).</text>')
    y += 34
    # flow boxes
    boxes = [
        (BLUE, "게임 스레드", ["모으기 프로세서가", "표현이 없는 걷는", "엔티티의 자리·시각·", "걷기 위상을 담는다"]),
        (PURPLE, "Niagara 사용자 변수", ["소형·대형 배열 둘을", "매 프레임 통째로", "바꿔 넣는다"]),
        (PINK, "GPU", ["원소마다 입자 하나를", "오브 쪽으로 외삽한", "자리에 만들고 그", "프레임에만 그린다"]),
    ]
    BW, BH, GAP = 228, 160, 18
    for i, (col, head, lines) in enumerate(boxes):
        x = X0 - 8 + i * (BW + GAP)
        o.append(f'<rect x="{x}" y="{y}" width="{BW}" height="{BH}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
        o.append(f'<rect x="{x}" y="{y}" width="{BW}" height="6" rx="3" fill="{col}"/>')
        o.append(f'<text x="{x + 14}" y="{y + 34}" font-size="17" font-weight="700" fill="{col}">{head}</text>')
        for k, line in enumerate(lines):
            o.append(f'<text x="{x + 14}" y="{y + 64 + k * 24}" font-size="16">{line}</text>')
        if i < len(boxes) - 1:
            ax = x + BW + 2
            ay = y + BH / 2
            o.append(f'<path d="M{ax} {ay} L{ax + GAP - 5} {ay}" stroke="{SUB}" stroke-width="2"/>')
            o.append(f'<path d="M{ax + GAP - 11} {ay - 6} L{ax + GAP - 3} {ay} L{ax + GAP - 11} {ay + 6}" fill="none" stroke="{SUB}" stroke-width="2"/>')
    y += BH + 36
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">입자 하나가 엔티티 하나다. 엔티티가 ISKM이 된 프레임에는 배열에서 빠진다.</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">그래서 같은 개체가 입자와 ISKM으로 함께 그려지는 프레임이 없다.</text>')
    write("ncf-principle.svg", svg(y + 58, o))


# ---------- 4. result: change from step 6 to step 7 per horde size ----------
DELTA = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE), ("GPU", "gpu_p50_ms", PINK)]


def delta_rows():
    out = []
    for n in NS:
        for name, col, c in DELTA:
            d = v(L7, n, col) - v(L6, n, col)
            noise = max(rng(L6, n, col), rng(L7, n, col))
            out.append((n, name, col, c, d, abs(d) > noise))
    return out


def delta():
    o = []
    title(o, "6단계에서 7단계로: 호드 총수별 변화",
          "오브 시점 카메라, 7단계 − 6단계, 세 번 잰 중앙값의 차(ms)")
    ZX, scale = 120, 380 / 2.0  # zero line; 0 .. +2.0 ms
    y = 118
    top = y - 14
    data = delta_rows()
    for n, lab in zip(NS, NLAB):
        bott = rows[(L7, n, "P1")]["bottleneck"]
        o.append(f'<text x="32" y="{y + 40}" font-size="18" font-weight="700">호드 {lab}</text>')
        o.append(f'<text x="{W - 32}" y="{y + 40}" text-anchor="end" font-size="16" fill="{SUB}">병목 {"GPU" if bott == "gpu" else "게임 스레드"}</text>')
        for (nn, name, col, c, d, real) in [r for r in data if r[0] == n]:
            w = abs(d) * scale
            x = ZX - w if d < 0 else ZX
            if real:
                o.append(f'<rect x="{x:.1f}" y="{y}" width="{max(w, 1.5):.1f}" height="20" rx="3" fill="{c}"/>')
            else:
                o.append(f'<rect x="{x:.1f}" y="{y}" width="{max(w, 1.5):.1f}" height="20" rx="3" fill="white" stroke="{c}" stroke-width="1.5" stroke-dasharray="4 3"/>')
            tag = "" if real else " (흔들림 안)"
            o.append(f'<text x="{ZX + max(w, 0) + 8:.1f}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{sfmt(d)}</tspan><tspan fill="{SUB}"> {name}{tag}</tspan></text>')
            y += 26
        y += 22
    o.append(f'<line x1="{ZX}" y1="{top}" x2="{ZX}" y2="{y - 14}" stroke="{SUB}" stroke-width="1.5"/>')
    o.append(f'<text x="{ZX + 8}" y="{y + 6}" font-size="16" fill="{SUB}">7단계가 길다 →</text>')
    y += 40
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">프레임 시간은 병목인 쪽의 변화를 따른다. 5천은 GPU, 1만부터는 게임 스레드다.</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">60fps를 지키는 최대 개체 수: 6단계 {N_AT_60_L6:,}, 7단계 {N_AT_60_L7:,}.</text>')
    write("ncf-delta.svg", svg(y + 60, o))


# ---------- 5. ablation: the final configuration with three things beyond 200m ----------
def ablation():
    o = []
    title(o, "최종 구성의 200m 밖을 세 가지로 그리면",
          "호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값(ms)")
    groups = [("그리지 않음", "최종 구성에서 배경 무리를 뺐다", AFX),
              ("Niagara 입자", "최종 구성", L8),
              ("ISKM(500m까지)", "최종 구성에서 입자 대신 ISKM", AFAR)]
    mets = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE), ("GPU", "gpu_p50_ms", PINK)]
    X0, scale = 40, 520 / 13.0
    y = 118
    for g, sub, var in groups:
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{g}</text>')
        o.append(f'<text x="{W - 32}" y="{y}" text-anchor="end" font-size="16" fill="{SUB}">{sub}</text>')
        y += 12
        for name, col, c in mets:
            val = v(var, 10000, col)
            w = val * scale
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="20" rx="3" fill="{c}"/>')
            o.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{fmt(val)}</tspan>'
                     f'<tspan fill="{SUB}"> {name}</tspan></text>')
            y += 26
        y += 28
    y += 4
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    d_fx = v(AFX, 10000, "frame_p50_ms") - v(L8, 10000, "frame_p50_ms")
    d_far = v(AFAR, 10000, "frame_p50_ms") - v(L8, 10000, "frame_p50_ms")
    n_far = v(AFAR, 10000, "ism_band3")
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">입자를 빼면 {sfmt(d_fx)}ms, 입자 대신 ISKM {count(n_far)}개로 그리면 {sfmt(d_far)}ms다(프레임 시간).</text>')
    d_game = v(AFAR, 10000, "game_p50_ms") - v(L8, 10000, "game_p50_ms")
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">ISKM은 GPU가 입자와 거의 같고, 게임 스레드가 {sfmt(d_game)}ms 길다.</text>')
    o.append(f'<text x="32" y="{y + 60}" font-size="16" fill="{SUB}">셋째 구성도 엔티티가 없는 500m 밖은 입자로 그린다.</text>')
    write("ncf-ablation.svg", svg(y + 84, o))


summary()
problem()
principle()
delta()
ablation()
print("wrote", sorted(os.listdir(OUT)))
print("N@60 L6:", N_AT_60_L6, "L7:", N_AT_60_L7)
for var in (L6, L7):
    a, i, nn = drawn_counts(var)
    print(f"{var} drawn: actors {a:.2f} iskm {i:.2f} none {nn:.2f}")
print("L6 GPU margin:", round(v(L6, 10000, "game_p50_ms") - v(L6, 10000, "gpu_p50_ms"), 3))
for (n, name, col, c, d, real) in delta_rows():
    print(f"delta {n} {col}: {d:+.3f} real={real} ranges L6 {rng(L6, n, col):.3f} L7 {rng(L7, n, col):.3f}")
for col in ("frame_p95_ms", "render_p50_ms"):
    for n in NS:
        print(f"delta {n} {col}: {v(L7, n, col) - v(L6, n, col):+.3f} (L6 {v(L6, n, col)}, L7 {v(L7, n, col)})")
print(f"P2 1만 frame: L6 {v(L6, 10000, 'frame_p50_ms', 'P2')} L7 {v(L7, 10000, 'frame_p50_ms', 'P2')} "
      f"d {v(L7, 10000, 'frame_p50_ms', 'P2') - v(L6, 10000, 'frame_p50_ms', 'P2'):+.3f} "
      f"ranges {rng(L6, 10000, 'frame_p50_ms', 'P2'):.3f}/{rng(L7, 10000, 'frame_p50_ms', 'P2'):.3f}")
for var in (AFX, AFAR):
    for col in ("frame_p50_ms", "frame_p95_ms", "game_p50_ms", "render_p50_ms", "gpu_p50_ms"):
        ra = rng(var, 10000, col) if col != "frame_p95_ms" else rng(var, 10000, col)
        print(f"{var} - L8 {col}: {v(var, 10000, col) - v(L8, 10000, col):+.3f} (ranges {ra:.3f} / {rng(L8, 10000, col):.3f})")
print("A8-farcull ism_band3", v(AFAR, 10000, "ism_band3"), "iskm_instances_p50", v(AFAR, 10000, "iskm_instances_p50"),
      "L8", v(L8, 10000, "iskm_instances_p50"))
