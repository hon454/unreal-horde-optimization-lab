# Makes the SVG figures of post 04 in images/.
# Run from the repository root: py -3 -I Posts/04-hydration-anim-budget/make-figures.py
# Measured values come from Measurements/Full2/summary.csv and Docs/figures/full2/full-n-at-60fps.csv.
# No trace values: the post uses no trace numbers (see measurements.md).
import csv, os
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


def s(variant, n, col, preset="P1"):
    return rows[(variant, n, preset)][col]


with open(os.path.join(ROOT, "Docs", "figures", "full2", "full-n-at-60fps.csv"), encoding="utf-8") as f:
    NAT = {r["series"]: r for r in csv.DictReader(f)}
N_AT_60_L4 = int(NAT["4 L4-mass"]["n_p95"])
L3_ABOVE = NAT["3 L3-vatcull"]["note_p95"] == "above range"

L1, L3, L4, L8 = "L1-full", "L3-vatcull", "L4-mass", "L8-q4final"
NS = [5000, 10000, 20000, 30000]
NLAB = ["5천", "1만", "2만", "3만"]
N = 10000

# Derived values (formulas are in measurements.md).
ACTORS_L4 = v(L4, N, "actors_mean")
D_GAME = v(L4, N, "game_p50_ms") - v(L3, N, "game_p50_ms")
D_FRAME = v(L4, N, "frame_p50_ms") - v(L3, N, "frame_p50_ms")
D_GPU = v(L4, N, "gpu_p50_ms") - v(L3, N, "gpu_p50_ms")
HEADROOM = v(L3, N, "gpu_p50_ms") - v(L3, N, "game_p50_ms")      # game thread time left before it passes the GPU
UNIT_L1_US = v(L1, 1000, "game_p50_ms") / 1000 * 1000             # step 1 actor, game thread per entity at 1,000 (µs)
FIT_L1 = HEADROOM / (UNIT_L1_US / 1000)                            # actors that fit the headroom at the step 1 unit cost
EXPECT_L1 = ACTORS_L4 * UNIT_L1_US / 1000                          # ms if each actor cost as much as a step 1 actor
UNIT_L4_US = D_GAME / ACTORS_L4 * 1000

# Ablations and the sub-experiment, all against the final configuration at 10k P1.
ABL = [("하이드레이션을 뺀다", "A8-hydrate"), ("애니메이션 예산을 뺀다", "A8-budget"),
       ("액터 캡슐 콜리전을 켠다", "S8-col"), ("전환 대책 다섯 가지를 뺀다", "A8-fix")]

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


def fmt(x, sign=False):
    """Three significant figures, rounded half up (12.35 -> 12.4, 0.2485 -> 0.249), as in the post body."""
    a = abs(x)
    q = "1" if a >= 100 else "0.1" if a >= 10 else "0.01" if a >= 1 else "0.001"
    t = str(Decimal(repr(a)).quantize(Decimal(q), rounding=ROUND_HALF_UP))
    if sign:
        return ("+" if x >= 0 else "−") + t
    return ("−" if x < 0 else "") + t


def title(o, t, sub):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{sub}</text>')


def arrow_down(o, x, y0, y1):
    o.append(f'<path d="M{x} {y0} L{x} {y1 - 2}" stroke="{SUB}" stroke-width="2"/>')
    o.append(f'<path d="M{x - 6} {y1 - 8} L{x} {y1} L{x + 6} {y1 - 8}" fill="none" stroke="{SUB}" stroke-width="2"/>')


METRICS = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE),
           ("렌더 스레드", "render_p50_ms", TEAL), ("GPU", "gpu_p50_ms", PINK)]


# ---------- 1. summary: step 3 vs step 4 at 10k ----------
def summary():
    o = []
    title(o, "호드 총수 1만에서 3단계와 4단계가 쓰는 시간", "오브 시점 카메라, 60초씩 세 번 잰 중앙값(ms)")
    X0, scale = 40, 600 / 17.5
    groups = [("3단계: VAT 인스턴스, 200m 밖 지우기", L3), ("4단계: 30m 안을 액터로, 애니메이션 예산 2ms", L4)]
    y = 120
    top = y - 8
    for g, var in groups:
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{g}</text>')
        y += 12
        slowest = max(METRICS[1:], key=lambda m: v(var, N, m[1]))[0]
        for name, col, c in METRICS:
            val = v(var, N, col)
            w = val * scale
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="20" rx="3" fill="{c}"/>')
            mark = " ← 가장 느리다" if name == slowest else ""
            o.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{fmt(val)}</tspan>'
                     f'<tspan fill="{SUB}"> {name}{mark}</tspan></text>')
            y += 26
        y += 30
    x167 = X0 + 16.667 * scale
    o.append(f'<line x1="{x167:.1f}" y1="{top + 14}" x2="{x167:.1f}" y2="{y - 14}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<text x="{x167 - 6:.1f}" y="{y - 2}" text-anchor="end" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    y += 30
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">프레임 시간은 셋 가운데 가장 느린 쪽이 정한다. 3단계는 GPU, 4단계는 게임 스레드다.</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">프레임 시간은 {fmt(D_FRAME, True)}ms, 게임 스레드는 {fmt(D_GAME, True)}ms, GPU는 {fmt(D_GPU, True)}ms 바뀌었다.</text>')
    write("hyd-summary.svg", svg(y + 60, o))


# ---------- 2. principle: bands and what an actor does per frame ----------
def principle():
    o = []
    title(o, "4단계가 바꾼 것: 가까운 개체를 액터로 그린다",
          "위: 거리에 따라 무엇으로 그리나. 아래: 액터 하나가 한 프레임에 게임 스레드에서 하는 일")
    y = 112
    BX, BW = 120, W - 152
    X = lambda m: BX + m / 500.0 * BW
    for lab, segs in [("3단계", [(0, 200, BLUE, "VAT 인스턴스", "white"), (200, 500, None, "그리지 않는다", SUB)]),
                      ("4단계", [(0, 30, RED, "", "white"), (30, 200, BLUE, "VAT 인스턴스", "white"), (200, 500, None, "그리지 않는다", SUB)])]:
        o.append(f'<text x="32" y="{y + 25}" font-size="17" font-weight="700">{lab}</text>')
        for a, b, col, text, tc in segs:
            if col:
                o.append(f'<rect x="{X(a):.1f}" y="{y}" width="{X(b) - X(a):.1f}" height="38" rx="4" fill="{col}"/>')
            else:
                o.append(f'<rect x="{X(a):.1f}" y="{y}" width="{X(b) - X(a):.1f}" height="38" rx="4" fill="white" stroke="{GRAY}" stroke-dasharray="5 4"/>')
            if text:
                o.append(f'<text x="{(X(a) + X(b)) / 2:.1f}" y="{y + 25}" text-anchor="middle" font-size="16" font-weight="700" fill="{tc}">{text}</text>')
        y += 50
    # actor label for the narrow red segment
    o.append(f'<path d="M{X(15):.1f} {y - 10} L{X(15):.1f} {y + 4}" stroke="{RED}" stroke-width="2"/>')
    for m in [0, 30, 80, 200, 500]:
        o.append(f'<line x1="{X(m):.1f}" y1="{y - 8}" x2="{X(m):.1f}" y2="{y}" stroke="{SUB}"/>')
        anchor = "start" if m == 0 else "end" if m == 500 else "middle"
        if m in (0, 30):
            continue
        o.append(f'<text x="{X(m):.1f}" y="{y + 20}" text-anchor="{anchor}" font-size="16" fill="{SUB}">{m}m</text>')
    o.append(f'<text x="{X(30) + 4:.1f}" y="{y + 20}" font-size="16" fill="{SUB}">30m</text>')
    o.append(f'<text x="32" y="{y + 44}" font-size="16" font-weight="700" fill="{RED}">빨강 = 액터. 화면 안 30m, 화면 밖 15m 안. 두 종류를 합쳐 500개까지</text>')
    y += 76
    boxes = [
        (PURPLE, "엔진 표현 프로세서", "UMassVisualizationProcessor",
         ["거리 구간이 바뀐 개체에 액터를 붙이고 뗀다",
          "뗀 액터는 풀에 넣었다가 다음 개체에 다시 쓴다"]),
        (BLUE, "액터 동기화(이 프로젝트)", "ULabActorSyncProcessor",
         ["엔티티의 위치·상태·사망 색을 액터에 넣는다",
          "화면 크기(바운드 반경 ÷ 거리)를 중요도로 넘긴다"]),
        (RED, "스켈레탈 메시 틱과 애니메이션 평가", "USkeletalMeshComponentBudgeted",
         ["Animation Budget Allocator가 2ms 예산을 나눈다",
          "중요도 큰 순으로 매 프레임, 나머지는 간격을 늘린다"]),
    ]
    RH, GAP, CX, CW = 118, 22, 32, W - 64
    for i, (col, head, who, lines) in enumerate(boxes):
        o.append(f'<rect x="{CX}" y="{y}" width="{CW}" height="{RH}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
        o.append(f'<rect x="{CX}" y="{y}" width="6" height="{RH}" rx="3" fill="{col}"/>')
        o.append(f'<text x="{CX + 24}" y="{y + 32}" font-size="18" font-weight="700" fill="{col}">{head}</text>')
        o.append(f'<text x="{CX + CW - 18}" y="{y + 32}" text-anchor="end" font-family="{MONO}" font-size="16" fill="{SUB}">{who}</text>')
        for k, line in enumerate(lines):
            o.append(f'<text x="{CX + 24}" y="{y + 64 + k * 28}" font-size="17">{line}</text>')
        if i < len(boxes) - 1:
            arrow_down(o, W / 2, y + RH + 3, y + RH + GAP - 3)
        y += RH + GAP
    y += 8
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">액터는 액터 틱과 이동 컴포넌트를 쓰지 않는다. 위치는 엔티티가 정한다.</text>')
    o.append(f'<text x="32" y="{y + 24}" font-size="16" fill="{SUB}">캡슐 콜리전도 끈다. 판정은 엔티티의 위치와 반경으로 한다.</text>')
    write("hyd-principle.svg", svg(y + 50, o))


# ---------- 3. transition: one hydration, frame by frame, and the five fixes ----------
def transition():
    o = []
    title(o, "VAT 인스턴스가 액터로 바뀌는 네 프레임", "전환 대책을 모두 켠 상태. 칸 하나가 한 프레임이다")
    L, C = 210, 4
    CW = (W - 32 - L) / C
    heads = ["액터가 붙는다", "메시 틱 1", "메시 틱 2", "액터가 보인다"]
    y = 108
    for i, h in enumerate(heads):
        o.append(f'<text x="{L + CW * i + CW / 2:.1f}" y="{y}" text-anchor="middle" font-size="16" font-weight="700">{h}</text>')
    y += 16
    lanes = [("VAT 인스턴스", [(BLUE, "그린다", "white")] * 3 + [(None, "뗀다", SUB)]),
             ("액터", [(GRAY, "패스에서 뺌", "white")] * 3 + [(RED, "그린다", "white")])]
    for name, cells in lanes:
        o.append(f'<text x="32" y="{y + 30}" font-size="17" font-weight="700">{name}</text>')
        for i, (col, text, tc) in enumerate(cells):
            x = L + CW * i + 4
            if col:
                o.append(f'<rect x="{x:.1f}" y="{y}" width="{CW - 8:.1f}" height="44" rx="5" fill="{col}"/>')
            else:
                o.append(f'<rect x="{x:.1f}" y="{y}" width="{CW - 8:.1f}" height="44" rx="5" fill="white" stroke="{GRAY}" stroke-dasharray="5 4"/>')
            o.append(f'<text x="{x + (CW - 8) / 2:.1f}" y="{y + 28}" text-anchor="middle" font-size="16" font-weight="700" fill="{tc}">{text}</text>')
        y += 56
    o.append(f'<text x="32" y="{y + 14}" font-size="16" fill="{SUB}">화면에는 늘 둘 중 하나만 보인다. 액터는 VAT가 재생하던 시각부터 걷는다.</text>')
    y += 50
    fixes = [
        ("높이 보정", "엔진이 액터를 발밑으로 옮길 때 캡슐 반높이를 더한다"),
        ("준비 대기", "메시가 두 번 틱할 때까지 VAT 인스턴스를 남긴다"),
        ("", "되돌아갈 때는 액터를 한 프레임 더 남긴다"),
        ("시작 시각", "AnimBP를 VAT가 재생하던 시각부터 시작한다"),
        ("패스 숨김", "기다리는 액터를 숨기지 않고 화면 패스에서만 뺀다"),
        ("진홍 지우기", "풀에 넣을 때 사망 색을 지운다"),
    ]
    o.append(f'<text x="32" y="{y}" font-size="18" font-weight="700">전환 대책 다섯 가지</text>')
    y += 14
    k = 0
    for name, text in fixes:
        if name:
            k += 1
            o.append(f'<circle cx="46" cy="{y + 17}" r="13" fill="{PURPLE}"/>')
            o.append(f'<text x="46" y="{y + 23}" text-anchor="middle" font-size="16" font-weight="700" fill="white">{k}</text>')
            o.append(f'<text x="70" y="{y + 23}" font-size="17" font-weight="700">{name}</text>')
        o.append(f'<text x="190" y="{y + 23}" font-size="17">{text}</text>')
        y += 34
    y += 12
    o.append(f'<line x1="32" y1="{y - 10}" x2="{W - 32}" y2="{y - 10}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 14}" font-size="16" fill="{SUB}">하나만 켜서는 화면의 튐이 거의 줄지 않았다. 한 전환에 문제가 여럿 겹쳐 있었다.</text>')
    write("hyd-transition.svg", svg(y + 40, o))


# ---------- 4. result: expected vs actual game thread cost of the actors ----------
def expected():
    o = []
    title(o, "액터가 더한 게임 스레드 시간: 여유, 예상, 실제",
          "호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값에서 계산(ms)")
    X0, scale = 40, 560 / 7.0
    bars = [
        ("3단계의 여유: GPU − 게임 스레드", f"{fmt(v(L3, N, 'gpu_p50_ms'))} − {fmt(v(L3, N, 'game_p50_ms'))}", HEADROOM, "line"),
        ("1단계 액터의 단가로 본 예상", f"액터 {ACTORS_L4:,.0f}개 × {fmt(UNIT_L1_US)}µs", EXPECT_L1, "exp"),
        ("실제: 4단계 − 3단계 게임 스레드", f"액터 하나에 {fmt(UNIT_L4_US)}µs", D_GAME, "act"),
    ]
    y = 118
    for lab, sub, val, style in bars:
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{lab}</text>')
        o.append(f'<text x="{W - 32}" y="{y}" text-anchor="end" font-size="16" fill="{SUB}">{sub}</text>')
        y += 12
        w = val * scale
        if style == "line":
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="26" rx="3" fill="white" stroke="{PINK}" stroke-width="2"/>')
            tc = PINK
        elif style == "exp":
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="26" rx="3" fill="white" stroke="{DARK}" stroke-width="1.5" stroke-dasharray="4 3"/>')
            tc = DARK
        else:
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="26" rx="3" fill="{BLUE}"/>')
            tc = BLUE
        o.append(f'<text x="{X0 + w + 10:.1f}" y="{y + 20}" font-size="17" font-weight="700" fill="{tc}">{fmt(val)}</text>')
        y += 64
    xh = X0 + HEADROOM * scale
    o.append(f'<line x1="{xh:.1f}" y1="112" x2="{xh:.1f}" y2="{y - 30}" stroke="{PINK}" stroke-width="1.5" stroke-dasharray="4 4"/>')
    o.append(f'<line x1="32" y1="{y - 18}" x2="{W - 32}" y2="{y - 18}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 6}" font-size="16" fill="{SUB}">1단계 단가라면 여유는 액터 {FIT_L1:,.0f}개에서 찬다.</text>')
    o.append(f'<text x="32" y="{y + 30}" font-size="16" fill="{SUB}">실제 액터는 그보다 싸지만, 개수가 많아 여유를 넘었다.</text>')
    o.append(f'<text x="32" y="{y + 54}" font-size="16" fill="{SUB}">같은 단계에서 GPU가 {fmt(-D_GPU)}ms 줄어, 프레임 시간은 {fmt(D_FRAME, True)}ms만 늘었다.</text>')
    write("hyd-expected.svg", svg(y + 78, o))


def line_chart(o, series, T, B, L, R, ymax, yticks, ylab):
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


# ---------- 5. result: frame p95 vs horde size, actor counts ----------
def maxn():
    o = []
    title(o, "60fps를 지키는 최대 개체 수", "오브 시점 카메라, 프레임 시간의 느린 쪽 5% 경계, 세 번 잰 중앙값")
    T, B, L, R = 130, 450, 70, 560
    p3 = [v(L3, n, "frame_p95_ms") for n in NS]
    p4 = [v(L4, n, "frame_p95_ms") for n in NS]
    series = [("3단계", p3, GRAY, True), ("4단계", p4, RED, False)]
    X, Y = line_chart(o, series, T, B, L, R, 25, [0, 5, 10, 15, 20, 25], "프레임 시간 느린 쪽 5% 경계(ms)")
    for k, (n, a, b) in enumerate(zip(NS, p3, p4)):
        # both lines rise to the right: a label goes above-left or below-right of its point so that no line crosses it
        # 5천: step 4 sits just below step 3, so step 3 goes above-left and step 4 below-right
        if k == 0:
            xa, aa, ya = X(n) - 8, "end", Y(a) - 10
            xb, ab, yb = X(n) + 8, "start", Y(b) + 24
        else:
            xa, aa, ya = X(n) + 8, "start", Y(a) + 24
            xb, ab, yb = X(n) - 8, "end", Y(b) - 10
        o.append(f'<text x="{xa:.1f}" y="{ya:.1f}" text-anchor="{aa}" font-size="16" font-weight="700" fill="{DARK}">{fmt(a)}</text>')
        o.append(f'<text x="{xb:.1f}" y="{yb:.1f}" text-anchor="{ab}" font-size="16" font-weight="700" fill="{RED}">{fmt(b)}</text>')
    y167 = Y(16.667)
    nx = X(N_AT_60_L4)
    o.append(f'<circle cx="{nx:.1f}" cy="{y167:.1f}" r="5.5" fill="white" stroke="{AMBER}" stroke-width="2.5"/>')
    o.append(f'<path d="M{nx:.1f} {y167 + 8:.1f} L{nx:.1f} {B - 4}" stroke="{AMBER}" stroke-width="1.5" stroke-dasharray="3 3"/>')
    o.append(f'<text x="{nx + 8:.1f}" y="{B - 10}" text-anchor="start" font-size="16" font-weight="700" fill="{AMBER}">{N_AT_60_L4:,}</text>')
    lx = 590
    o.append(f'<line x1="{lx}" y1="160" x2="{lx + 28}" y2="160" stroke="{GRAY}" stroke-width="2.5" stroke-dasharray="6 4"/><text x="{lx + 36}" y="166" font-size="16">3단계</text>')
    o.append(f'<line x1="{lx}" y1="192" x2="{lx + 28}" y2="192" stroke="{RED}" stroke-width="2.5"/><text x="{lx + 36}" y="198" font-size="16">4단계</text>')
    o.append(f'<circle cx="{lx + 14}" cy="224" r="5.5" fill="white" stroke="{AMBER}" stroke-width="2.5"/><text x="{lx + 36}" y="230" font-size="16">4단계의 한계</text>')
    if L3_ABOVE:
        o.append(f'<text x="{lx}" y="270" font-size="16" fill="{DARK}" font-weight="700">3단계는 3만까지</text>')
        o.append(f'<text x="{lx}" y="292" font-size="16" fill="{DARK}" font-weight="700">한계에 닿지 않았다</text>')
    # actor counts per horde size (step 4)
    yb = B + 84
    acts = [round(v(L4, n, "actors_mean")) for n in NS]
    o.append(f'<line x1="32" y1="{yb - 18}" x2="{W - 32}" y2="{yb - 18}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{yb + 6}" font-size="16" font-weight="700" fill="{RED}">4단계의 액터 수(프레임 평균)</text>')
    for n, a in zip(NS, acts):
        o.append(f'<text x="{X(n):.1f}" y="{yb + 34}" text-anchor="middle" font-size="17" font-weight="700" fill="{RED}">{a:,}</text>')
    o.append(f'<text x="32" y="{yb + 64}" font-size="16" fill="{SUB}">2만부터는 액터 수가 상한 500 가까이에서 더 늘지 않는다.</text>')
    write("hyd-maxn.svg", svg(yb + 90, o))


# ---------- 6. ablation: final configuration minus one technique ----------
def ablation():
    o = []
    title(o, "최종 구성에서 하나만 바꾸면", "호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값의 차이(ms). 기준은 최종 구성")
    C0 = 400
    scale = 240 / 4.5
    y = 116
    o.append(f'<text x="{C0 - 10}" y="{y}" text-anchor="end" font-size="16" fill="{SUB}">← 빨라진다</text>')
    o.append(f'<text x="{C0 + 10}" y="{y}" font-size="16" fill="{SUB}">느려진다 →</text>')
    y += 26
    top = y - 6
    base_frame, base_game = v(L8, N, "frame_p50_ms"), v(L8, N, "game_p50_ms")
    for lab, cond in ABL:
        df = v(cond, N, "frame_p50_ms") - base_frame
        dg = v(cond, N, "game_p50_ms") - base_game
        rng = max(v(cond, N, "frame_p50_ms_max") - v(cond, N, "frame_p50_ms_min"),
                  v(L8, N, "frame_p50_ms_max") - v(L8, N, "frame_p50_ms_min"))
        within = abs(df) <= rng
        o.append(f'<text x="32" y="{y + 4}" font-size="17" font-weight="700">{lab}</text>')
        if within:
            o.append(f'<text x="{W - 32}" y="{y + 4}" text-anchor="end" font-size="16" fill="{SUB}">세 번 잰 범위 안: 구별되지 않는다</text>')
        elif s(cond, N, "bottleneck") != s(L8, N, "bottleneck"):
            o.append(f'<text x="{W - 32}" y="{y + 4}" text-anchor="end" font-size="16" font-weight="700" fill="{PINK}">병목이 GPU로 바뀐다</text>')
        y += 14
        for name, d, c in [("프레임", df, DARK), ("게임 스레드", dg, BLUE)]:
            w = abs(d) * scale
            x = C0 if d >= 0 else C0 - w
            o.append(f'<rect x="{x:.1f}" y="{y}" width="{max(w, 1.5):.1f}" height="20" rx="3" fill="{c}"/>')
            if d >= 0:
                o.append(f'<text x="{C0 + w + 8:.1f}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{fmt(d, True)}</tspan><tspan fill="{SUB}"> {name}</tspan></text>')
            else:
                o.append(f'<text x="{C0 - w - 8:.1f}" y="{y + 16}" text-anchor="end" font-size="16"><tspan fill="{SUB}">{name} </tspan><tspan font-weight="700" fill="{c}">{fmt(d, True)}</tspan></text>')
            y += 26
        y += 24
    o.append(f'<line x1="{C0}" y1="{top}" x2="{C0}" y2="{y - 20}" stroke="{SUB}" stroke-width="1.5"/>')
    o.append(f'<line x1="32" y1="{y - 8}" x2="{W - 32}" y2="{y - 8}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 16}" font-size="16" fill="{SUB}">최종 구성은 화면 안 40m까지 액터(상한 1,000)로 그린다.</text>')
    o.append(f'<text x="32" y="{y + 40}" font-size="16" fill="{SUB}">애니메이션 예산은 2ms이고, 캡슐 콜리전은 끈다.</text>')
    o.append(f'<text x="32" y="{y + 64}" font-size="16" fill="{SUB}">최종 구성의 프레임 시간은 {fmt(base_frame)}ms, 게임 스레드는 {fmt(base_game)}ms다.</text>')
    write("hyd-ablation.svg", svg(y + 88, o))


summary()
principle()
transition()
expected()
maxn()
ablation()
print("wrote", sorted(os.listdir(OUT)))
print(f"L4 actors {ACTORS_L4} dGame {D_GAME:.4f} dFrame {D_FRAME:.4f} dGPU {D_GPU:.4f}")
print(f"headroom {HEADROOM:.4f} unitL1 {UNIT_L1_US:.4f}us fitL1 {FIT_L1:.2f} expectL1 {EXPECT_L1:.4f} unitL4 {UNIT_L4_US:.4f}us")
print("N@60 L4:", N_AT_60_L4, "L3 above range:", L3_ABOVE)
for lab, cond in ABL:
    print(cond, f"dframe {v(cond, N, 'frame_p50_ms') - v(L8, N, 'frame_p50_ms'):+.4f} dgame {v(cond, N, 'game_p50_ms') - v(L8, N, 'game_p50_ms'):+.4f}",
          f"drender {v(cond, N, 'render_p50_ms') - v(L8, N, 'render_p50_ms'):+.4f} dgpu {v(cond, N, 'gpu_p50_ms') - v(L8, N, 'gpu_p50_ms'):+.4f}", s(cond, N, "bottleneck"))
