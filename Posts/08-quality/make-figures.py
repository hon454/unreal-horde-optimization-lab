# Makes the SVG figures of post 08 in images/.
# Run from the repository root: py -3 -I Posts/08-quality/make-figures.py
# Measured values come from Measurements/Full2/summary.csv, Docs/figures/full2/full-n-at-60fps.csv
# and, for the 1280x720 figure only, Measurements/Ref1/summary.csv (compared only within that batch).
# No trace values: the post uses no trace numbers (see measurements.md).
import csv, os
from decimal import Decimal, ROUND_HALF_UP

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")


def load(path):
    rows = {}
    with open(os.path.join(ROOT, *path), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            rows[(r["variant"], int(r["n"]), r["preset"])] = r
    return rows


FULL = load(("Measurements", "Full2", "summary.csv"))
REF = load(("Measurements", "Ref1", "summary.csv"))


def v(variant, n, col, preset="P1", rows=FULL):
    return float(rows[(variant, n, preset)][col])


def s(variant, n, col, preset="P1", rows=FULL):
    return rows[(variant, n, preset)][col]


def rng(variant, n, col, preset="P1", rows=FULL):
    return v(variant, n, col + "_max", preset, rows) - v(variant, n, col + "_min", preset, rows)


with open(os.path.join(ROOT, "Docs", "figures", "full2", "full-n-at-60fps.csv"), encoding="utf-8") as f:
    NAT = {r["series"]: r for r in csv.DictReader(f)}
N_AT_60_L7 = int(NAT["7 L7-q2crowd"]["n_p95"])
N_AT_60_L8 = int(NAT["8 L8-q4final"]["n_p95"])

L3, L4, L7, L8 = "L3-vatcull", "L4-mass", "L7-q2crowd", "L8-q4final"
L8AA = "L8-q4final-aa"          # in Ref1 the final configuration is measured as the A/A rows (r11-r13)
N = 10000
NS3 = [10000, 20000, 30000]
NLAB3 = ["1만", "2만", "3만"]
BUDGET = 1000.0 / 60.0          # 16.7ms (60fps)
MARGIN = BUDGET * 0.8           # 13.3ms, 20% margin of the target spec (ADR-0010 decision 6)

# Derived values (formulas are in measurements.md).
ADD_ACTORS = v(L8, N, "actors_mean") - v(L7, N, "actors_mean")
UNIT_L4_US = (v(L4, N, "game_p50_ms") - v(L3, N, "game_p50_ms")) / v(L4, N, "actors_mean") * 1000   # post 04: 12.5 µs
EXPECT_GAME = ADD_ACTORS * UNIT_L4_US / 1000
D_GAME = v(L8, N, "game_p50_ms") - v(L7, N, "game_p50_ms")
D_GPU = v(L8, N, "gpu_p50_ms") - v(L7, N, "gpu_p50_ms")
D_FRAME = v(L8, N, "frame_p50_ms") - v(L7, N, "frame_p50_ms")
SHADOW_GPU = v("A8-farshadow", N, "gpu_p50_ms") - v(L8, N, "gpu_p50_ms")

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
MONO = "Consolas, SFMono-Regular, Menlo, monospace"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, PURPLE, GRAY, AMBER, RED, TEAL, PINK, DARK, SKY = (
    "#2563eb", "#9333ea", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151", "#93c5fd")
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


def pct(ratio):
    """Ratio as percent with three significant figures; very small values keep two decimals."""
    p = ratio * 100
    if p == 0:
        return "0"
    if p < 0.1:
        return str(Decimal(repr(p)).quantize(Decimal("0.01"), rounding=ROUND_HALF_UP))
    a = p
    q = "0.1" if a >= 10 else "0.01" if a >= 1 else "0.001"
    return str(Decimal(repr(a)).quantize(Decimal(q), rounding=ROUND_HALF_UP))


def title(o, t, sub):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{sub}</text>')


def foot(o, y, lines):
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    for k, line in enumerate(lines):
        o.append(f'<text x="32" y="{y + 12 + k * 24}" font-size="16" fill="{SUB}">{line}</text>')
    return y + 12 + len(lines) * 24


METRICS = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE),
           ("렌더 스레드", "render_p50_ms", TEAL), ("GPU", "gpu_p50_ms", PINK)]


def thread_bars(o, groups, y, rows=FULL, n=N, scale=600 / 17.5, X0=40, line167=True):
    top = y - 8
    for g, var in groups:
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{g}</text>')
        y += 12
        slowest = max(METRICS[1:], key=lambda m: v(var, n, m[1], rows=rows))[0]
        for name, col, c in METRICS:
            val = v(var, n, col, rows=rows)
            w = val * scale
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="20" rx="3" fill="{c}"/>')
            mark = " ← 가장 느리다" if name == slowest else ""
            o.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{fmt(val)}</tspan>'
                     f'<tspan fill="{SUB}"> {name}{mark}</tspan></text>')
            y += 26
        y += 30
    if line167:
        x167 = X0 + BUDGET * scale
        o.append(f'<line x1="{x167:.1f}" y1="{top + 14}" x2="{x167:.1f}" y2="{y - 14}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
        o.append(f'<text x="{x167 - 6:.1f}" y="{y - 2}" text-anchor="end" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    return y


# ---------- 1. summary: step 7 vs step 8 at 10k ----------
def summary():
    o = []
    title(o, "호드 총수 1만에서 7단계와 8단계가 쓰는 시간", "오브 시점 카메라, 60초씩 세 번 잰 중앙값(ms)")
    groups = [("7단계: 액터 0~30m(상한 500), 모든 거리에 그림자", L7),
              ("8단계: 액터 0~40m(상한 1,000), 80m 밖 그림자 끔", L8)]
    y = thread_bars(o, groups, 120)
    y += 30
    a7, a8 = v(L7, N, "actors_mean"), v(L8, N, "actors_mean")
    y = foot(o, y, [f"프레임 시간은 {fmt(D_FRAME, True)}ms로 세 번 잰 범위 안이다. GPU는 {fmt(D_GPU, True)}ms 바뀌었다.",
                    f"같은 프레임 시간에 액터가 {a7:,.0f}개에서 {a8:,.0f}개로 늘었다(프레임 평균)."])
    write("q-summary.svg", svg(y + 24, o))


# ---------- 2. problem: what step 7 leaves unused at 10k ----------
def headroom():
    o = []
    title(o, "7단계는 호드 총수 1만에서 시간이 남는다", "오브 시점 카메라, 세 번 잰 중앙값(ms)")
    X0, scale = 40, 600 / 17.5
    bars = [("프레임 시간(느린 쪽 5% 경계)", v(L7, N, "frame_p95_ms"), DARK),
            ("게임 스레드(중앙값)", v(L7, N, "game_p50_ms"), BLUE),
            ("GPU(중앙값)", v(L7, N, "gpu_p50_ms"), PINK)]
    y = 116
    top = y - 6
    for lab, val, c in bars:
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{lab}</text>')
        y += 12
        w = val * scale
        o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="24" rx="3" fill="{c}"/>')
        o.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 18}" font-size="17" font-weight="700" fill="{c}">{fmt(val)}</text>')
        y += 58
    for val, lab in [(MARGIN, "13.3ms(여유 20%)"), (BUDGET, "16.7ms(60fps)")]:
        x = X0 + val * scale
        o.append(f'<line x1="{x:.1f}" y1="{top}" x2="{x:.1f}" y2="{y - 26}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
        o.append(f'<text x="{x:.1f}" y="{y - 6}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">{lab}</text>')
    y += 34
    p95 = v(L7, N, "frame_p95_ms")
    gap = v(L7, N, "game_p50_ms") - v(L7, N, "gpu_p50_ms")
    y = foot(o, y, [f"느린 쪽 5% 경계는 13.3ms까지 {fmt(MARGIN - p95)}ms, 16.7ms까지 {fmt(BUDGET - p95)}ms가 남는다.",
                    f"GPU는 게임 스레드보다 {fmt(gap)}ms 짧다. 병목은 게임 스레드다.",
                    f"대형 액터의 애니메이션 갱신 가운데 건너뛴 것은 {pct(v(L7, N, 'large_skip_ratio'))}%다. 예산 2ms가 거의 쉰다."])
    write("q-headroom.svg", svg(y + 24, o))


# ---------- 3. principle: distance bands of step 7 and step 8 ----------
def bands():
    o = []
    title(o, "8단계가 바꾼 것: 거리 구간과 그림자", "화면 안 개체를 카메라 거리로 나눠 무엇으로 그리나. 빗금 없는 칸은 그림자를 그린다")
    BX, BW, MAXM = 120, W - 152, 250
    X = lambda m: BX + min(m, MAXM) / MAXM * BW
    o.append('<defs><pattern id="noshadow" width="10" height="10" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
             '<rect width="10" height="10" fill="white" fill-opacity="0"/><line x1="0" y1="0" x2="0" y2="10" stroke="white" stroke-width="4" stroke-opacity="0.75"/></pattern></defs>')
    rows = [("7단계", [(0, 30, RED, "액터", True), (30, 80, BLUE, "ISKM", True), (80, 200, BLUE, "ISKM", True), (200, MAXM, TEAL, "입자", True)]),
            ("8단계", [(0, 40, RED, "액터", True), (40, 80, BLUE, "ISKM", True), (80, 200, BLUE, "ISKM", False), (200, MAXM, TEAL, "입자", False)])]
    y = 112
    for lab, segs in rows:
        o.append(f'<text x="32" y="{y + 27}" font-size="17" font-weight="700">{lab}</text>')
        for a, b, col, text, shadow in segs:
            o.append(f'<rect x="{X(a):.1f}" y="{y}" width="{X(b) - X(a) - 2:.1f}" height="42" rx="4" fill="{col}"/>')
            if not shadow:
                o.append(f'<rect x="{X(a):.1f}" y="{y}" width="{X(b) - X(a) - 2:.1f}" height="42" rx="4" fill="url(#noshadow)"/>')
            o.append(f'<text x="{(X(a) + X(b)) / 2:.1f}" y="{y + 27}" text-anchor="middle" font-size="16" font-weight="700" fill="white">{text}</text>')
        y += 56
    for m, lab in [(0, "0"), (30, "30"), (40, "40"), (80, "80m"), (200, "200m")]:
        o.append(f'<line x1="{X(m):.1f}" y1="{y - 10}" x2="{X(m):.1f}" y2="{y - 2}" stroke="{SUB}"/>')
        anchor = "end" if m == 30 else "start" if m == 40 else "middle"
        dx = -2 if m == 30 else 2 if m == 40 else 0
        o.append(f'<text x="{X(m) + dx:.1f}" y="{y + 16}" text-anchor="{anchor}" font-size="16" fill="{SUB}">{lab}</text>')
    o.append(f'<text x="{X(MAXM):.1f}" y="{y + 16}" text-anchor="end" font-size="16" fill="{SUB}">카메라 거리 →</text>')
    y += 44
    o.append(f'<rect x="32" y="{y - 14}" width="28" height="18" rx="3" fill="{BLUE}"/><rect x="32" y="{y - 14}" width="28" height="18" rx="3" fill="url(#noshadow)"/>')
    o.append(f'<text x="70" y="{y}" font-size="16" fill="{SUB}">빗금 = 그림자를 그리지 않는다. 액터 상한: 7단계 500, 8단계 1,000(두 종류 합계)</text>')
    y += 34
    items = [
        (RED, "액터 구간 30m → 40m, 상한 500 → 1,000",
         ["액터는 게임 스레드를 쓴다(동기화, 메시 틱, 애니메이션 평가)",
          "애니메이션 평가는 예산 2ms 안에서만 늘고, 넘으면 갱신을 건너뛴다"]),
        (PURPLE, "80m 밖 그림자 켬 → 끔",
         ["80~200m ISKM과 200m 밖 입자의 그림자를 끈다",
          "그림자 패스는 GPU를 쓴다"]),
    ]
    RH, CX, CW = 108, 32, W - 64
    for col, head, lines in items:
        o.append(f'<rect x="{CX}" y="{y}" width="{CW}" height="{RH}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
        o.append(f'<rect x="{CX}" y="{y}" width="6" height="{RH}" rx="3" fill="{col}"/>')
        o.append(f'<text x="{CX + 24}" y="{y + 32}" font-size="18" font-weight="700" fill="{col}">{head}</text>')
        for k, line in enumerate(lines):
            o.append(f'<text x="{CX + 24}" y="{y + 62 + k * 26}" font-size="17">{line}</text>')
        y += RH + 16
    y += 10
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">두 변화가 쓰는 자원이 다르다. 병목이 어느 쪽이냐에 따라 프레임 시간에 보이는 값이 달라진다.</text>')
    write("q-bands.svg", svg(y + 28, o))


# ---------- 4. result: expected vs actual ----------
def expected():
    o = []
    title(o, "8단계가 더하고 뺀 시간: 예상과 실제", "호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값에서 계산(ms)")
    C0, scale = 330, 300 / 1.2
    rows = [
        ("게임 스레드", [
            (f"예상: 늘어난 액터 {ADD_ACTORS:.0f}개 × 4단계 액터 단가 {fmt(UNIT_L4_US)}µs", EXPECT_GAME, "exp", BLUE),
            ("실제: 8단계 − 7단계(세 번 잰 범위 안)", D_GAME, "act", BLUE)]),
        ("GPU", [
            ("80m 밖 그림자의 몫: 최종 구성에서 다시 켠 측정의 차이", -SHADOW_GPU, "exp", PINK),
            ("실제: 8단계 − 7단계", D_GPU, "act", PINK)]),
    ]
    y = 112
    o.append(f'<text x="{C0 - 10}" y="{y}" text-anchor="end" font-size="16" fill="{SUB}">← 줄었다</text>')
    o.append(f'<text x="{C0 + 10}" y="{y}" font-size="16" fill="{SUB}">늘었다 →</text>')
    y += 34
    top = y - 14
    for head, bars in rows:
        o.append(f'<text x="32" y="{y}" font-size="18" font-weight="700">{head}</text>')
        y += 26
        for lab, val, style, c in bars:
            o.append(f'<text x="48" y="{y}" font-size="16" fill="{SUB}">{lab}</text>')
            y += 10
            w = abs(val) * scale
            x = C0 if val >= 0 else C0 - w
            if style == "exp":
                o.append(f'<rect x="{x:.1f}" y="{y}" width="{max(w, 2):.1f}" height="24" rx="3" fill="white" stroke="{c}" stroke-width="2" stroke-dasharray="5 3"/>')
            else:
                o.append(f'<rect x="{x:.1f}" y="{y}" width="{max(w, 2):.1f}" height="24" rx="3" fill="{c}"/>')
            tx, anchor = (C0 + w + 10, "start") if val >= 0 else (C0 - w - 10, "end")
            o.append(f'<text x="{tx:.1f}" y="{y + 18}" text-anchor="{anchor}" font-size="17" font-weight="700" fill="{c}">{fmt(val, True)}</text>')
            o.append(f'<line x1="{C0}" y1="{y - 4}" x2="{C0}" y2="{y + 28}" stroke="{SUB}" stroke-width="1.5"/>')
            y += 50
        y += 6
    y += 10
    s7, s8 = v(L7, N, "large_skip_ratio"), v(L8, N, "large_skip_ratio")
    y = foot(o, y, [f"늘어난 액터의 게임 스레드 비용은 거의 보이지 않았다. 대신 대형 액터의 애니메이션 갱신을",
                    f"건너뛴 비율이 {pct(s7)}%에서 {pct(s8)}%로 뛰었다. GPU 감소는 80m 밖 그림자의 몫과 거의 같다."])
    write("q-expected.svg", svg(y + 24, o))


# ---------- 5. result: coverage and skipped updates by horde size ----------
def tradeoff():
    o = []
    title(o, "호드 총수가 늘 때 8단계가 사는 것과 내는 것", "오브 시점 카메라, 세 번 잰 중앙값. 회색 = 7단계, 빨강 = 8단계")
    panels = [("화면 안 액터 구간에서 액터인 비율(%)", "coverhyd_mean", 40),
              ("대형 액터 갱신을 건너뛴 비율(%)", "large_skip_ratio", 416)]
    T, B = 150, 380
    H = B - T
    for head, col, PX in panels:
        PW = 330
        o.append(f'<text x="{PX}" y="{T - 36}" font-size="17" font-weight="700">{head}</text>')
        for t in (0, 50, 100):
            yy = B - t / 100 * H
            o.append(f'<line x1="{PX}" y1="{yy:.1f}" x2="{PX + PW}" y2="{yy:.1f}" stroke="{LINE}"/>')
            o.append(f'<text x="{PX + PW + 6}" y="{yy + 5:.1f}" font-size="16" fill="{SUB}">{t}</text>')
        gw = PW / 3
        for k, (n, lab) in enumerate(zip(NS3, NLAB3)):
            gx = PX + gw * k
            for j, (var, c) in enumerate([(L7, GRAY), (L8, RED)]):
                val = v(var, n, col)
                h = val * H
                bx = gx + 14 + j * 48
                o.append(f'<rect x="{bx:.1f}" y="{B - h:.1f}" width="42" height="{max(h, 1.5):.1f}" rx="3" fill="{c}"/>')
                o.append(f'<text x="{bx + 21:.1f}" y="{B - h - 8:.1f}" text-anchor="middle" font-size="16" font-weight="700" fill="{DARK if j == 0 else RED}">{pct(val)}</text>')
            o.append(f'<text x="{gx + 59:.1f}" y="{B + 24}" text-anchor="middle" font-size="16">{lab}</text>')
        o.append(f'<line x1="{PX}" y1="{B}" x2="{PX + PW}" y2="{B}" stroke="{SUB}"/>')
    y = B + 70
    y = foot(o, y, ["액터 구간은 7단계가 30m, 8단계가 40m다. 7단계는 2만부터 상한 500에 막혀 구간이 비고,",
                    "8단계는 3만까지 대부분을 채운다. 대신 1만부터 대형 액터 갱신의 다섯에 하나를 건너뛴다.",
                    f"60fps를 지키는 최대 개체 수는 7단계 {N_AT_60_L7:,}, 8단계 {N_AT_60_L8:,}이다."])
    write("q-tradeoff.svg", svg(y + 24, o))


# ---------- 6. ablation: final configuration with one change ----------
ABL = [("80m 밖 그림자를 다시 켠다", "A8-farshadow"), ("전환 대책 다섯 가지를 뺀다", "A8-fix"), ("하이드레이션을 뺀다", "A8-hydrate")]


def ablation():
    o = []
    title(o, "최종 구성에서 하나만 바꾸면", "호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값의 차이(ms). 기준은 최종 구성")
    C0 = 400
    scale = 240 / 4.5
    y = 116
    o.append(f'<text x="{C0 - 10}" y="{y}" text-anchor="end" font-size="16" fill="{SUB}">← 줄었다</text>')
    o.append(f'<text x="{C0 + 10}" y="{y}" font-size="16" fill="{SUB}">늘었다 →</text>')
    y += 26
    top = y - 6
    for lab, cond in ABL:
        df = v(cond, N, "frame_p50_ms") - v(L8, N, "frame_p50_ms")
        within = abs(df) <= max(rng(cond, N, "frame_p50_ms"), rng(L8, N, "frame_p50_ms"))
        o.append(f'<text x="32" y="{y + 4}" font-size="17" font-weight="700">{lab}</text>')
        if within:
            o.append(f'<text x="{W - 32}" y="{y + 4}" text-anchor="end" font-size="16" fill="{SUB}">프레임 시간은 세 번 잰 범위 안</text>')
        elif s(cond, N, "bottleneck") != s(L8, N, "bottleneck"):
            o.append(f'<text x="{W - 32}" y="{y + 4}" text-anchor="end" font-size="16" font-weight="700" fill="{PINK}">병목이 GPU로 바뀐다</text>')
        y += 14
        for name, col, c in METRICS:
            d = v(cond, N, col) - v(L8, N, col)
            w = abs(d) * scale
            x = C0 if d >= 0 else C0 - w
            o.append(f'<rect x="{x:.1f}" y="{y}" width="{max(w, 1.5):.1f}" height="18" rx="3" fill="{c}"/>')
            if d >= 0:
                o.append(f'<text x="{C0 + w + 8:.1f}" y="{y + 15}" font-size="16"><tspan font-weight="700" fill="{c}">{fmt(d, True)}</tspan><tspan fill="{SUB}"> {name}</tspan></text>')
            else:
                o.append(f'<text x="{C0 - w - 8:.1f}" y="{y + 15}" text-anchor="end" font-size="16"><tspan fill="{SUB}">{name} </tspan><tspan font-weight="700" fill="{c}">{fmt(d, True)}</tspan></text>')
            y += 23
        y += 22
    o.append(f'<line x1="{C0}" y1="{top}" x2="{C0}" y2="{y - 20}" stroke="{SUB}" stroke-width="1.5"/>')
    y += 6
    y = foot(o, y, [f"최종 구성의 프레임 시간은 {fmt(v(L8, N, 'frame_p50_ms'))}ms, 게임 스레드는 {fmt(v(L8, N, 'game_p50_ms'))}ms, GPU는 {fmt(v(L8, N, 'gpu_p50_ms'))}ms다.",
                    f"하이드레이션을 빼면 GPU {fmt(v('A8-hydrate', N, 'gpu_p50_ms'))}ms가 가장 느려져 게임 스레드가 준 만큼 프레임이 줄지 않는다."])
    write("q-ablation.svg", svg(y + 24, o))


# ---------- 7. reference: the same removal at 1280x720 ----------
def lowres():
    o = []
    title(o, "해상도를 1280×720으로 낮춰 다시 잰 측정", "같은 패키지, 호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값(ms). 이 그림 안에서만 견준다")
    groups = [("최종 구성", L8AA), ("최종 구성에서 하이드레이션을 뺐다", "A8-hydrate")]
    y = thread_bars(o, groups, 120, rows=REF, scale=600 / 10.0, line167=False)
    d = v("A8-hydrate", N, "frame_p50_ms", rows=REF) - v(L8AA, N, "frame_p50_ms", rows=REF)
    dg = v("A8-hydrate", N, "game_p50_ms", rows=REF) - v(L8AA, N, "game_p50_ms", rows=REF)
    y += 10
    y = foot(o, y, ["GPU가 짧아져 두 구성 모두 게임 스레드가 가장 느리다.",
                    f"하이드레이션을 빼면 게임 스레드가 {fmt(dg, True)}ms, 프레임 시간이 {fmt(d, True)}ms 바뀐다."])
    write("q-lowres.svg", svg(y + 24, o))


summary()
headroom()
bands()
expected()
tradeoff()
ablation()
lowres()
print("wrote", sorted(os.listdir(OUT)))
print(f"addActors {ADD_ACTORS:.4f} unitL4 {UNIT_L4_US:.4f}us expectGame {EXPECT_GAME:.4f} dGame {D_GAME:+.4f} dGPU {D_GPU:+.4f} dFrame {D_FRAME:+.4f} shadowGPU {SHADOW_GPU:+.4f}")
print("N@60 L7/L8:", N_AT_60_L7, N_AT_60_L8, "margin", round(MARGIN, 4))
for lab, cond in ABL:
    print(cond, *[f"{m[0]} {v(cond, N, m[1]) - v(L8, N, m[1]):+.4f}" for m in METRICS], s(cond, N, "bottleneck"),
          "rng", round(max(rng(cond, N, 'frame_p50_ms'), rng(L8, N, 'frame_p50_ms')), 4))
for n in NS3:
    print(n, "cov", v(L7, n, "coverhyd_mean"), v(L8, n, "coverhyd_mean"), "skip", v(L7, n, "large_skip_ratio"), v(L8, n, "large_skip_ratio"),
          "act", v(L7, n, "actors_mean"), v(L8, n, "actors_mean"))
print("P2 L7/L8 frame", v(L7, N, "frame_p50_ms", "P2"), v(L8, N, "frame_p50_ms", "P2"), "gpu", v(L7, N, "gpu_p50_ms", "P2"), v(L8, N, "gpu_p50_ms", "P2"))
print("Ref1", *[f"{m[0]} {v(L8AA, N, m[1], rows=REF)} -> {v('A8-hydrate', N, m[1], rows=REF)}" for m in METRICS])
