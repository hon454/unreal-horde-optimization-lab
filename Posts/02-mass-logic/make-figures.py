# Makes the SVG figures of post 02 in images/.
# Run from the repository root: py -3 -I Posts/02-mass-logic/make-figures.py
# Measured values come from Measurements/Full2/summary.csv and Docs/figures/full2/full-n-at-60fps.csv.
# No trace values: the diagram (mass-processors.svg) carries no numbers. Standard library only.
import csv, math, os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")

rows = {}
with open(os.path.join(ROOT, "Measurements", "Full2", "summary.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        rows[(r["variant"], int(r["n"]), r["preset"])] = r


def v(variant, n, col):
    return float(rows[(variant, n, "P1")][col])


with open(os.path.join(ROOT, "Docs", "figures", "full2", "full-n-at-60fps.csv"), encoding="utf-8") as f:
    n60 = {r["series"]: int(r["n_p95"]) for r in csv.DictReader(f)}
N60_ACTOR, N60_MASS = n60["1 L1-full"], n60["2 L2-mass"]

A, M = "L1-full", "L2-mass"
NS_A = [500, 1000, 2000, 10000]
NS_M = [5000, 10000, 20000, 30000]
NLAB = {500: "500", 1000: "1천", 2000: "2천", 5000: "5천", 10000: "1만", 20000: "2만", 30000: "3만"}

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
MONO = "Consolas, SFMono-Regular, Menlo, monospace"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, PURPLE, GRAY, AMBER, RED, TEAL, PINK, DARK = (
    "#2563eb", "#9333ea", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151")
W = 800
BUDGET = 16.667


def svg(h, body):
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">',
            f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>']
    return "\n".join(head + body + ["</svg>"]) + "\n"


def write(name, text):
    with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def fmt(x):
    """Three significant figures, as in the post body."""
    if abs(x) >= 100:
        return f"{x:.0f}"
    if abs(x) >= 10:
        return f"{x:.1f}"
    if abs(x) >= 1:
        return f"{x:.2f}"
    return f"{x:.3f}"


def title(o, t, s):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')


METRICS = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE),
           ("렌더 스레드", "render_p50_ms", TEAL), ("GPU", "gpu_p50_ms", PINK)]


# ---------- 1. summary: frame / game / render / GPU at 10k, two scales ----------
def summary():
    o = []
    title(o, "호드 총수 1만에서 두 단계가 쓰는 시간",
          "오브 시점 카메라, 최종 측정, 60초씩 세 번 잰 중앙값(ms)")
    X0 = 40

    def group(y, label, var, scale, top_note=None):
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{label}</text>')
        y += 12
        for name, col, c in METRICS:
            val = v(var, 10000, col)
            w = max(val * scale, 2)
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="20" rx="3" fill="{c}"/>')
            o.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{fmt(val)}</tspan>'
                     f'<tspan fill="{SUB}"> {name}</tspan></text>')
            y += 26
        return y

    # panel A: same scale
    o.append(f'<text x="32" y="112" font-size="16" fill="{SUB}">같은 축척(0–450ms)</text>')
    sA = 560 / 450.0
    y = 146
    y = group(y, "1단계 액터 군중", A, sA) + 26
    y = group(y, "2단계 Mass 로직(표현은 애니메이션 없는 ISM)", M, sA)
    y += 16
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    # panel B: zoom on stage 2
    y += 34
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">2단계만 확대(0–20ms)</text>')
    sB = 560 / 20.0
    y += 34
    topB = y - 8
    y = group(y, "2단계 Mass 로직", M, sB)
    x167 = X0 + BUDGET * sB
    o.append(f'<line x1="{x167:.1f}" y1="{topB + 14}" x2="{x167:.1f}" y2="{y + 4}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<text x="{x167 + 6:.1f}" y="{y + 20}" font-size="16" font-weight="700" fill="{AMBER}">16.7ms</text>')
    y += 50
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">프레임 시간은 셋 가운데 가장 느린 쪽이 정한다.</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">1단계는 게임 스레드, 2단계는 GPU가 가장 느리다.</text>')
    write("mass-summary.svg", svg(y + 58, o))


# ---------- 2. principle: per-actor ticks vs processors over chunks (no numbers) ----------
def processors():
    o = []
    title(o, "같은 로직을 도는 두 방식",
          "게임 스레드가 한 프레임에 하는 일. 위에서 아래 순서로 돈다")
    # left: actors
    LX, LW = 32, 352
    RX, RW = 416, 352
    o.append(f'<text x="{LX}" y="114" font-size="18" font-weight="700" fill="{RED}">1단계: 액터마다 틱한다</text>')
    o.append(f'<text x="{RX}" y="114" font-size="18" font-weight="700" fill="{BLUE}">2단계: Processor가 Chunk를 돈다</text>')
    chips = [("이동 컴포넌트 틱", BLUE), ("스켈레탈 메시 틱 · AnimBP", PURPLE), ("렌더 상태 보내기", GRAY)]
    y = 134
    for name in ["액터 1", "액터 2", None, "액터 10,000"]:
        if name is None:
            o.append(f'<text x="{LX + LW / 2}" y="{y + 22}" text-anchor="middle" font-size="22" font-weight="700" fill="{SUB}">⋮</text>')
            y += 36
            continue
        h = 34 + 30 * len(chips)
        o.append(f'<rect x="{LX}" y="{y}" width="{LW}" height="{h}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
        o.append(f'<text x="{LX + 14}" y="{y + 24}" font-size="17" font-weight="700">{name}</text>')
        cy = y + 34
        for c, col in chips:
            o.append(f'<rect x="{LX + 14}" y="{cy}" width="{LW - 28}" height="24" rx="5" fill="{col}" fill-opacity="0.14" stroke="{col}"/>')
            o.append(f'<text x="{LX + 24}" y="{cy + 18}" font-size="16">{c}</text>')
            cy += 30
        y += h + 12
    left_end = y
    # right: one chunk of an archetype
    cols = ["위치", "속도", "이동 목표", "체력", "상태"]
    gx, gy, cw, rh = RX, 134, RW / len(cols), 26
    box_h = rh * 5 + 34 + 30
    o.append(f'<rect x="{gx}" y="{gy}" width="{RW}" height="{box_h}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
    o.append(f'<text x="{gx + 14}" y="{gy + 24}" font-size="17" font-weight="700">Chunk 하나(같은 Archetype)</text>')
    o.append(f'<text x="{gx + 14}" y="{gy + box_h - 12}" font-size="16" fill="{SUB}">행 하나가 엔티티, 열 하나가 Fragment 배열</text>')
    ty = gy + 34
    for i, c in enumerate(cols):
        o.append(f'<text x="{gx + cw * i + cw / 2:.1f}" y="{ty + 18}" text-anchor="middle" font-size="16" fill="{SUB}">{c}</text>')
    for r in range(4):
        yy = ty + rh * (r + 1)
        for i in range(len(cols)):
            o.append(f'<rect x="{gx + cw * i + 6:.1f}" y="{yy + 3}" width="{cw - 12:.1f}" height="{rh - 6}" rx="3" fill="{BLUE}" fill-opacity="{0.18 + 0.08 * i:.2f}"/>')
    py = gy + box_h + 20
    procs = [("오브 쪽으로 이동 목표를 쓴다", "ULabApproachProcessor"),
             ("조향 · 이동 · 회전(엔진)", "UMassApplyMovementProcessor 등"),
             ("범위 피해로 체력을 깎는다", "ULabOrbContactProcessor"),
             ("2초마다 다섯을 저격한다", "ULabOrbSnipeProcessor"),
             ("피격 · 사망 시간을 센다", "ULabMinionStateProcessor"),
             ("ISM 위치를 쓴다(엔진)", "UMassUpdateISMProcessor")]
    for k, (t, cls) in enumerate(procs):
        o.append(f'<rect x="{RX}" y="{py}" width="{RW}" height="52" rx="8" fill="{CARD}" stroke="{LINE}"/>')
        o.append(f'<rect x="{RX}" y="{py}" width="6" height="52" rx="3" fill="{BLUE}"/>')
        o.append(f'<text x="{RX + 18}" y="{py + 22}" font-size="16" font-weight="700">{k + 1}. {t}</text>')
        o.append(f'<text x="{RX + 18}" y="{py + 43}" font-family="{MONO}" font-size="16" fill="{SUB}">{cls}</text>')
        py += 58
    right_end = py
    y = max(left_end, right_end) + 8
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y += 30
    o.append(f'<text x="{LX}" y="{y}" font-size="17" font-weight="700" fill="{RED}">틱 함수 호출 = 개체 수 × 컴포넌트 수</text>')
    o.append(f'<text x="{RX}" y="{y}" font-size="17" font-weight="700" fill="{BLUE}">순회 = Processor 수 × Chunk 수</text>')
    y += 26
    o.append(f'<text x="{LX}" y="{y}" font-size="16" fill="{SUB}">액터마다 메시 컴포넌트를 따로 그린다</text>')
    o.append(f'<text x="{RX}" y="{y}" font-size="16" fill="{SUB}">종류마다 ISM 컴포넌트 하나가 모두 그린다</text>')
    write("mass-processors.svg", svg(y + 26, o))


# ---------- log-log helper ----------
def loglog(o, L, R, T, B, xr, yr, xticks, yticks, xlabel, ylabel):
    xmin, xmax = map(math.log10, xr)
    ymin, ymax = map(math.log10, yr)
    X = lambda n: L + (math.log10(n) - xmin) / (xmax - xmin) * (R - L)
    Y = lambda val: B - (math.log10(val) - ymin) / (ymax - ymin) * (B - T)
    o.append(f'<text x="32" y="{T - 18}" font-size="18" font-weight="700">{ylabel}</text>')
    for t, lab in yticks:
        o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
        o.append(f'<text x="{L - 10}" y="{Y(t) + 6:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{lab}</text>')
    for n in xticks:
        o.append(f'<line x1="{X(n):.1f}" y1="{B}" x2="{X(n):.1f}" y2="{B + 5}" stroke="{SUB}"/>')
        o.append(f'<text x="{X(n):.1f}" y="{B + 26}" text-anchor="middle" font-size="16">{NLAB[n]}</text>')
    o.append(f'<line x1="{L}" y1="{B}" x2="{R}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<line x1="{L}" y1="{T}" x2="{L}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<text x="{(L + R) / 2}" y="{B + 52}" text-anchor="middle" font-size="16" fill="{SUB}">{xlabel}</text>')
    return X, Y


def legend(o, x, y, items):
    for k, (lab, col, dash) in enumerate(items):
        yy = y + k * 30
        d = f' stroke-dasharray="{dash}"' if dash else ""
        o.append(f'<line x1="{x}" y1="{yy}" x2="{x + 28}" y2="{yy}" stroke="{col}" stroke-width="2.5"{d}/>')
        o.append(f'<circle cx="{x + 14}" cy="{yy}" r="4.5" fill="{col}"/>')
        o.append(f'<text x="{x + 38}" y="{yy + 6}" font-size="16">{lab}</text>')


# ---------- 3. problem: draw calls by horde size ----------
def draw_calls():
    o = []
    title(o, "호드 총수에 따른 드로우 콜", "오브 시점 카메라, 최종 측정, 세 번 잰 중앙값")
    L, R, T, B = 96, 560, 136, 430
    ns = sorted(set(NS_A + NS_M))
    X, Y = loglog(o, L, R, T, B, (400, 40000), (60, 30000), ns,
                  [(100, "100"), (300, "300"), (1000, "1천"), (3000, "3천"), (10000, "1만")],
                  "호드 총수(로그 축)", "한 프레임의 드로우 콜(로그 축)")
    # reference: one draw call per entity
    o.append(f'<line x1="{X(400):.1f}" y1="{Y(400):.1f}" x2="{X(30000):.1f}" y2="{Y(30000):.1f}" stroke="{GRAY}" stroke-width="1.5" stroke-dasharray="3 4"/>')
    o.append(f'<text x="{X(14000):.1f}" y="{Y(30000) + 4:.1f}" text-anchor="end" font-size="16" fill="{SUB}">개체 하나에 하나</text>')
    for var, ns_, col, below in [(A, NS_A, RED, False), (M, NS_M, BLUE, True)]:
        vals = [v(var, n, "draw_calls_p50") for n in ns_]
        pts = " ".join(f"{X(n):.1f},{Y(d):.1f}" for n, d in zip(ns_, vals))
        o.append(f'<polyline points="{pts}" fill="none" stroke="{col}" stroke-width="2.5"/>')
        for n, d in zip(ns_, vals):
            o.append(f'<circle cx="{X(n):.1f}" cy="{Y(d):.1f}" r="4.5" fill="{col}"/>')
            dy = 26 if below else -12
            o.append(f'<text x="{X(n):.1f}" y="{Y(d) + dy:.1f}" text-anchor="middle" font-size="16" font-weight="700" fill="{col}">{int(d):,}</text>')
    legend(o, 588, 170, [("1단계 액터 군중", RED, None), ("2단계 Mass 로직", BLUE, None)])
    o.append(f'<text x="588" y="252" font-size="16" fill="{SUB}">두 단계는 잰 호드</text>')
    o.append(f'<text x="588" y="274" font-size="16" fill="{SUB}">총수가 다르다.</text>')
    o.append(f'<text x="588" y="296" font-size="16" fill="{SUB}">1만은 둘 다 쟀다.</text>')
    write("mass-draw-calls.svg", svg(B + 80, o))


# ---------- 4. result: frame p95 by N and per-entity game thread time ----------
def frame_vs_n():
    o = []
    title(o, "호드 총수에 따른 두 단계의 비용", "오브 시점 카메라, 최종 측정, 세 번 잰 중앙값")
    ns = sorted(set(NS_A + NS_M))
    L, R, T, B = 88, 548, 136, 400
    X, Y = loglog(o, L, R, T, B, (300, 40000), (4, 700), ns,
                  [(5, "5"), (10, "10"), (20, "20"), (50, "50"), (100, "100"), (200, "200"), (500, "500")],
                  "호드 총수(로그 축)", "프레임 시간, 느린 쪽 5% 경계(ms, 로그 축)")
    y167 = Y(BUDGET)
    o.append(f'<line x1="{L}" y1="{y167:.1f}" x2="{R}" y2="{y167:.1f}" stroke="{AMBER}" stroke-dasharray="6 4" stroke-width="1.5"/>')
    o.append(f'<text x="{R + 6}" y="{y167 + 6:.1f}" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    # label placement per point: (dx, dy, anchor). "ul" = up-left, "dr" = down-right
    place = {"ul": (-10, -10, "end"), "dr": (10, 22, "start")}
    where = {(A, 500): "ul", (A, 1000): "ul", (A, 2000): "ul", (A, 10000): "ul",
             (M, 5000): "dr", (M, 10000): "ul", (M, 20000): "dr", (M, 30000): "ul"}
    for var, ns_, col in [(A, NS_A, RED), (M, NS_M, BLUE)]:
        vals = [v(var, n, "frame_p95_ms") for n in ns_]
        pts = " ".join(f"{X(n):.1f},{Y(m):.1f}" for n, m in zip(ns_, vals))
        o.append(f'<polyline points="{pts}" fill="none" stroke="{col}" stroke-width="2.5"/>')
        for n, m in zip(ns_, vals):
            o.append(f'<circle cx="{X(n):.1f}" cy="{Y(m):.1f}" r="4.5" fill="{col}"/>')
            dx, dy, anchor = place[where[(var, n)]]
            o.append(f'<text x="{X(n) + dx:.1f}" y="{Y(m) + dy:.1f}" text-anchor="{anchor}" font-size="16" font-weight="700" fill="{col}">{fmt(m)}</text>')
    for nn, col in [(N60_ACTOR, RED), (N60_MASS, BLUE)]:
        nx = X(nn)
        o.append(f'<circle cx="{nx:.1f}" cy="{y167:.1f}" r="5.5" fill="white" stroke="{AMBER}" stroke-width="2.5"/>')
        o.append(f'<path d="M{nx:.1f} {y167 + 8:.1f} L{nx:.1f} {y167 + 48:.1f}" stroke="{AMBER}" stroke-width="1.5"/>')
        o.append(f'<text x="{nx:.1f}" y="{y167 + 66:.1f}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">{nn:,}</text>')
    legend(o, 580, 160, [("1단계 액터 군중", RED, None), ("2단계 Mass 로직", BLUE, None)])
    o.append(f'<circle cx="594" cy="220" r="5.5" fill="white" stroke="{AMBER}" stroke-width="2.5"/>')
    o.append(f'<text x="618" y="226" font-size="16">60fps를 지키는</text>')
    o.append(f'<text x="618" y="248" font-size="16">최대 개체 수</text>')

    # lower panel: per-entity game thread time, log axis
    T2, B2 = 540, 700
    o.append(f'<text x="32" y="{T2 - 52}" font-size="18" font-weight="700">개체 하나당 게임 스레드 시간(µs, 로그 축)</text>')
    o.append(f'<text x="32" y="{T2 - 26}" font-size="16" fill="{SUB}">게임 스레드 시간(중앙값) ÷ 호드 총수</text>')
    ymin, ymax = math.log10(0.3), math.log10(100)
    Y2 = lambda val: B2 - (math.log10(val) - ymin) / (ymax - ymin) * (B2 - T2)
    for t, lab in [(0.5, "0.5"), (1, "1"), (5, "5"), (10, "10"), (50, "50")]:
        o.append(f'<line x1="{L}" y1="{Y2(t):.1f}" x2="{W - 40}" y2="{Y2(t):.1f}" stroke="{LINE}"/>')
        o.append(f'<text x="{L - 10}" y="{Y2(t) + 6:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{lab}</text>')
    o.append(f'<line x1="{L}" y1="{B2}" x2="{W - 40}" y2="{B2}" stroke="{SUB}"/>')
    slots = [(A, n) for n in NS_A] + [(M, n) for n in NS_M]
    x0, dx = L + 40, (W - 40 - L - 80) / (len(slots) - 1)
    for k, (var, n) in enumerate(slots):
        cx = x0 + k * dx + (20 if var == M else 0)
        col = RED if var == A else BLUE
        per = v(var, n, "game_p50_ms") / n * 1000
        o.append(f'<line x1="{cx:.1f}" y1="{B2}" x2="{cx:.1f}" y2="{Y2(per):.1f}" stroke="{col}" stroke-width="3"/>')
        o.append(f'<circle cx="{cx:.1f}" cy="{Y2(per):.1f}" r="6" fill="{col}"/>')
        o.append(f'<text x="{cx:.1f}" y="{Y2(per) - 12:.1f}" text-anchor="middle" font-size="16" font-weight="700" fill="{col}">{fmt(per)}</text>')
        o.append(f'<text x="{cx:.1f}" y="{B2 + 24}" text-anchor="middle" font-size="16">{NLAB[n]}</text>')
    o.append(f'<text x="{x0 + 1.5 * dx:.1f}" y="{B2 + 50}" text-anchor="middle" font-size="16" fill="{RED}">1단계 액터 군중</text>')
    o.append(f'<text x="{x0 + 5.5 * dx + 20:.1f}" y="{B2 + 50}" text-anchor="middle" font-size="16" fill="{BLUE}">2단계 Mass 로직</text>')
    write("mass-frame-vs-n.svg", svg(B2 + 76, o))


def report():
    """Derived values for measurements.md."""
    fa, fm = v(A, 10000, "frame_p50_ms"), v(M, 10000, "frame_p50_ms")
    ga, gm = v(A, 10000, "game_p50_ms"), v(M, 10000, "game_p50_ms")
    print(f"frame ratio {fa / fm:.2f}, game ratio {ga / gm:.2f}, n60 ratio {N60_MASS / N60_ACTOR:.2f}")
    print(f"L1 GPU / 16.667 = {v(A, 10000, 'gpu_p50_ms') / BUDGET:.3f}")
    for var, ns_ in [(A, NS_A), (M, NS_M)]:
        for n in ns_:
            print(var, n, "per-entity game us", round(v(var, n, "game_p50_ms") / n * 1000, 4),
                  "draws", v(var, n, "draw_calls_p50"), "p95", v(var, n, "frame_p95_ms"),
                  "bottleneck", rows[(var, n, "P1")]["bottleneck"])
    p1k = v(A, 1000, "game_p50_ms") / 1000 * 1000
    p10m = v(M, 10000, "game_p50_ms") / 10000 * 1000
    print(f"actor 1k per-entity / mass 10k per-entity = {p1k / p10m:.2f}")
    print(f"actor 10k per-entity / mass 10k per-entity = {(ga / 10000) / (gm / 10000):.2f}")
    print(f"mass slope 5k->30k us = {(v(M, 30000, 'game_p50_ms') - v(M, 5000, 'game_p50_ms')) / 25000 * 1000:.4f}")


summary()
processors()
draw_calls()
frame_vs_n()
report()
print("wrote", sorted(os.listdir(OUT)))
