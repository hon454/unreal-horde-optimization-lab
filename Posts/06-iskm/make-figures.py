# Makes the SVG figures of post 06 in images/.
# Run from the repository root: py -3 -I Posts/06-iskm/make-figures.py
# Measured values come from Measurements/Full2/summary.csv and Docs/figures/full2/full-n-at-60fps.csv.
# No trace values are drawn (the Insights screenshot is added later; see measurements.md).
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
L5_ABOVE = NAT["5 L5-m4offpool"]["note_p95"] == "above range"

L5, L6, L8, AISKM, ABND = "L5-m4offpool", "L6-m5bounds", "L8-q4final", "A8-iskm", "A8-bounds"
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


def title(o, t, s):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')


METRICS = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE),
           ("렌더 스레드", "render_p50_ms", TEAL), ("GPU", "gpu_p50_ms", PINK)]


# ---------- 1. summary: step 5 vs step 6 at 10k ----------
def summary():
    o = []
    title(o, "호드 총수 1만에서 5단계와 6단계가 쓰는 시간",
          "오브 시점 카메라, 60초씩 세 번 잰 중앙값(ms)")
    X0, scale = 40, 600 / 17.5
    groups = [("5단계: 30~200m를 VAT 인스턴스로", L5), ("6단계: 30~200m를 ISKM으로, 바운드 고정", L6)]
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
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">프레임 시간은 셋 가운데 가장 느린 쪽이 정한다. 두 단계 모두 게임 스레드다.</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">GPU는 {fmt(v(L5, 10000, "gpu_p50_ms"))}에서 {fmt(v(L6, 10000, "gpu_p50_ms"))}ms로 줄었지만 프레임 시간은 게임 스레드 근처에 머문다.</text>')
    write("iskm-summary.svg", svg(y + 58, o))


# ---------- 2. principle: VAT lane vs ISKM lane ----------
def principle():
    o = []
    title(o, "먼 개체 하나를 움직이는 두 길",
          "위는 5단계의 VAT 인스턴스, 아래는 6단계의 ISKM. 칸마다 누가 일하는지 적었다")
    lanes = [
        ("5단계 · VAT 인스턴스", GRAY, [
            (PURPLE, "에디터에서 한 번", ["AnimToTexture가 클립을", "텍스처로 굽는다", "메시 LOD는 하나"]),
            (BLUE, "게임 스레드, 매 프레임", ["인스턴스마다 프레임", "번호 두 개를", "커스텀 데이터로 쌓는다"]),
            (PINK, "GPU, 매 프레임", ["텍스처를 읽어 정점을", "옮긴다. 정적 메시라", "디스턴스 필드도 갱신"]),
        ]),
        ("6단계 · ISKM", BLUE, [
            (PURPLE, "에셋 하나", ["ASTPD에 AnimSequence", "세 개를 넣는다", "메시 LOD는 원본 다섯"]),
            (BLUE, "게임 스레드, 매 프레임", ["위치를 쌓고, 상태가", "바뀐 개체만 트랙에", "클립·시작 위치를 쓴다"]),
            (PINK, "GPU, 매 프레임", ["뼈 자세로 스키닝한다", "인스턴스마다 메시", "LOD를 고른다"]),
        ]),
    ]
    BW, BH, GAP, X0 = 228, 148, 18, 32
    y = 104
    for lane, lc, boxes in lanes:
        o.append(f'<text x="{X0}" y="{y + 4}" font-size="18" font-weight="700">{lane}</text>')
        y += 18
        for i, (col, head, lines) in enumerate(boxes):
            x = X0 + i * (BW + GAP)
            o.append(f'<rect x="{x}" y="{y}" width="{BW}" height="{BH}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
            o.append(f'<rect x="{x}" y="{y}" width="{BW}" height="6" rx="3" fill="{col}"/>')
            o.append(f'<text x="{x + 14}" y="{y + 34}" font-size="17" font-weight="700" fill="{col}">{head}</text>')
            for k, line in enumerate(lines):
                o.append(f'<text x="{x + 14}" y="{y + 66 + k * 24}" font-size="16">{line}</text>')
            if i < len(boxes) - 1:
                ax = x + BW + 2
                ay = y + BH / 2
                o.append(f'<path d="M{ax} {ay} L{ax + GAP - 5} {ay}" stroke="{SUB}" stroke-width="2"/>')
                o.append(f'<path d="M{ax + GAP - 11} {ay - 6} L{ax + GAP - 3} {ay} L{ax + GAP - 11} {ay + 6}" fill="none" stroke="{SUB}" stroke-width="2"/>')
        y += BH + 34
    # the frame-end box under the ISKM lane
    y -= 16
    o.append(f'<rect x="{X0}" y="{y}" width="{W - 2 * X0}" height="112" rx="10" fill="white" stroke="{AMBER}" stroke-width="1.5" stroke-dasharray="6 4"/>')
    o.append(f'<text x="{X0 + 16}" y="{y + 32}" font-size="17" font-weight="700" fill="{AMBER}">ISKM만 · 프레임 끝, 인스턴스가 바뀐 프레임마다</text>')
    o.append(f'<text x="{X0 + 16}" y="{y + 62}" font-size="16">엔진이 모든 인스턴스의 클립 바운드를 합쳐 컴포넌트 바운드를 다시 만든다.</text>')
    o.append(f'<text x="{X0 + 16}" y="{y + 88}" font-size="16">6단계는 오브를 덮는 상자를 미리 넣어 이 반복을 건너뛴다(고정 바운드).</text>')
    y += 112 + 36
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">줄어드는 곳은 GPU다: 디스턴스 필드 갱신이 빠진다.</text>')
    o.append(f'<text x="32" y="{y + 24}" font-size="16" fill="{SUB}">늘어날 수 있는 곳은 게임 스레드와 프레임 끝이다: 트랙과 바운드를 엔진이 챙긴다.</text>')
    write("iskm-principle.svg", svg(y + 48, o))


def line_chart(o, series, T, B, L, R, ymax, yticks, ylab):
    """series: list of (label, values, color, dash). x is NS (linear)."""
    X = lambda n: L + n / 32000 * (R - L)
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


# ---------- 3. problem: step 5 by thread vs horde size ----------
def problem():
    o = []
    title(o, "5단계는 GPU가 게임 스레드 바로 뒤에 있다",
          "오브 시점 카메라, 최종 측정, 세 번 잰 중앙값")
    T, B, L, R = 130, 470, 70, 560
    series = [("게임 스레드", [v(L5, n, "game_p50_ms") for n in NS], BLUE, False),
              ("GPU", [v(L5, n, "gpu_p50_ms") for n in NS], PINK, False),
              ("렌더 스레드", [v(L5, n, "render_p50_ms") for n in NS], TEAL, False)]
    X, Y = line_chart(o, series, T, B, L, R, 17.5, [0, 5, 10, 15], "스레드별 시간 중앙값(ms)")
    # labels stay off the lines, which all rise to the right: left of the point at 5천 (the three points are 18-26px apart),
    # then above-left for 게임 스레드 and below-right for GPU and 렌더 스레드
    side = {"게임 스레드": ["ml", "ul", "ul", "ul"], "GPU": ["ml", "br", "br", "br"], "렌더 스레드": ["ml", "br", "br", "br"]}
    pos = {"ml": (-10, 5, "end"), "ul": (-8, -12, "end"), "br": (8, 24, "start")}
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
    o.append(f'<text x="{lx}" y="{160 + 3 * 32 + 6}" font-size="16" fill="{SUB}">병목: 5천은 GPU,</text>')
    o.append(f'<text x="{lx}" y="{160 + 3 * 32 + 30}" font-size="16" fill="{SUB}">1만부터 게임 스레드</text>')
    yb = B + 84
    gap = v(L5, 10000, "game_p50_ms") - v(L5, 10000, "gpu_p50_ms")
    o.append(f'<line x1="32" y1="{yb - 18}" x2="{W - 32}" y2="{yb - 18}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{yb + 6}" font-size="16" fill="{SUB}">호드 총수 1만에서 게임 스레드와 GPU의 차이는 {fmt(gap)}ms다.</text>')
    o.append(f'<text x="32" y="{yb + 30}" font-size="16" fill="{SUB}">게임 스레드를 줄여도 GPU가 곧 프레임 시간을 정하게 된다.</text>')
    write("iskm-problem.svg", svg(yb + 52, o))


# ---------- 4. result: change from step 5 to step 6 per horde size ----------
DELTA = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE), ("GPU", "gpu_p50_ms", PINK)]


def delta_rows():
    out = []
    for n in NS:
        for name, col, c in DELTA:
            d = v(L6, n, col) - v(L5, n, col)
            noise = max(rng(L5, n, col), rng(L6, n, col))
            out.append((n, name, col, c, d, abs(d) > noise))
    return out


def delta():
    o = []
    title(o, "5단계에서 6단계로: 호드 총수별 변화",
          "오브 시점 카메라, 6단계 − 5단계, 세 번 잰 중앙값의 차(ms)")
    ZX, scale = 470, 300 / 5.5  # zero line; -5.5 .. +2.2 ms
    y = 118
    top = y - 14
    data = delta_rows()
    for n, lab in zip(NS, NLAB):
        o.append(f'<text x="32" y="{y + 40}" font-size="18" font-weight="700">호드 {lab}</text>')
        for (nn, name, col, c, d, real) in [r for r in data if r[0] == n]:
            w = abs(d) * scale
            x = ZX - w if d < 0 else ZX
            if real:
                o.append(f'<rect x="{x:.1f}" y="{y}" width="{max(w, 1.5):.1f}" height="20" rx="3" fill="{c}"/>')
            else:
                o.append(f'<rect x="{x:.1f}" y="{y}" width="{max(w, 1.5):.1f}" height="20" rx="3" fill="white" stroke="{c}" stroke-width="1.5" stroke-dasharray="4 3"/>')
            tag = "" if real else " (흔들림 안)"
            if d < 0:
                o.append(f'<text x="{ZX + 8}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{sfmt(d)}</tspan><tspan fill="{SUB}"> {name}{tag}</tspan></text>')
            else:
                o.append(f'<text x="{ZX + w + 8:.1f}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{sfmt(d)}</tspan><tspan fill="{SUB}"> {name}{tag}</tspan></text>')
            y += 26
        y += 22
    o.append(f'<line x1="{ZX}" y1="{top}" x2="{ZX}" y2="{y - 14}" stroke="{SUB}" stroke-width="1.5"/>')
    o.append(f'<text x="{ZX - 8}" y="{y + 6}" text-anchor="end" font-size="16" fill="{SUB}">← 6단계가 짧다</text>')
    o.append(f'<text x="{ZX + 8}" y="{y + 6}" font-size="16" fill="{SUB}">6단계가 길다 →</text>')
    y += 40
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">점선 막대는 변화가 세 번 잰 값의 흔들림보다 작아 구별되지 않는 칸이다.</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">GPU는 개체 수가 늘수록 더 줄고, 게임 스레드는 개체 수가 늘수록 더 는다.</text>')
    write("iskm-delta.svg", svg(y + 60, o))


# ---------- 5. ablation: the final configuration drawn three ways ----------
def ablation():
    o = []
    title(o, "최종 구성의 40~200m를 세 가지로 그리면",
          "호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값(ms)")
    groups = [("VAT 인스턴스", "최종 구성에서 ISKM을 뺐다", AISKM),
              ("ISKM, 엔진 기본 바운드", "최종 구성에서 고정 바운드를 뺐다", ABND),
              ("ISKM, 고정 바운드", "최종 구성", L8)]
    mets = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE), ("GPU", "gpu_p50_ms", PINK)]
    X0, scale = 40, 520 / 10.0
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
    d_bnd = v(ABND, 10000, "frame_p50_ms") - v(L8, 10000, "frame_p50_ms")
    d_vat = v(AISKM, 10000, "frame_p50_ms") - v(L8, 10000, "frame_p50_ms")
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">GPU는 두 ISKM이 VAT보다 짧다. 프레임 시간은 기본 바운드가 가장 길다.</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">고정 바운드를 빼면 {sfmt(d_bnd)}ms, VAT로 되돌리면 {sfmt(d_vat)}ms다(프레임 시간, 최종 구성 대비).</text>')
    write("iskm-ablation.svg", svg(y + 60, o))


summary()
principle()
problem()
delta()
ablation()
print("wrote", sorted(os.listdir(OUT)))
print("N@60 L6:", N_AT_60_L6, "L5 above range:", L5_ABOVE)
for (n, name, col, c, d, real) in delta_rows():
    print(f"delta {n} {col}: {d:+.3f} real={real} ranges L5 {rng(L5, n, col):.3f} L6 {rng(L6, n, col):.3f}")
for var in (AISKM, ABND):
    for col in ("frame_p50_ms", "game_p50_ms", "render_p50_ms", "gpu_p50_ms"):
        print(f"{var} - L8 {col}: {v(var, 10000, col) - v(L8, 10000, col):+.3f} (ranges {rng(var, 10000, col):.3f} / {rng(L8, 10000, col):.3f})")
