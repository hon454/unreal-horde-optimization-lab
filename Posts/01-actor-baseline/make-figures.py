# Makes the SVG figures of post 01 in images/.
# Run from the repository root: py Posts/01-actor-baseline/make-figures.py
# Measured values come from Measurements/Full2/summary.csv and Docs/figures/full2/full-n-at-60fps.csv.
# Trace values (one serial trace, 25-75 s window) are constants below; their source is in measurements.md.
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
    N_AT_60 = int(next(r for r in csv.DictReader(f) if r["series"] == "1 L1-full")["n_p95"])

# Serial trace Full1traceser-L1-full (build 730ff18, kept: the Full2 traces are not exported), 10000 P1, 25-75 s, per frame (ms). measurements.md "두 트레이스".
T_FRAME, T_CMC, T_SKIN, T_ANIMEVAL = 446.43, 323.49, 27.24, 14.70
T_REST = T_FRAME - T_CMC - T_SKIN - T_ANIMEVAL

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
MONO = "Consolas, SFMono-Regular, Menlo, monospace"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, PURPLE, LILAC, GRAY, AMBER, RED, TEAL, PINK, DARK = (
    "#2563eb", "#9333ea", "#c084fc", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151")
W = 800


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
    return f"{x:.2f}"


def title(o, t, s):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')


# ---------- 1. summary: frame / game / render / GPU at 10k ----------
def summary():
    o = []
    title(o, "호드 총수 1만에서 액터 군중이 쓰는 시간",
          "오브 시점 카메라, 60초씩 세 번 잰 중앙값(ms)")
    X0, scale = 40, 560 / 450.0
    groups = [("액터 군중", "L1-full"), ("액터 군중 + URO", "S1-uro"),
              ("액터 군중 + Animation Budget Allocator(예산 2ms)", "S1-budget")]
    metrics = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE),
               ("렌더 스레드", "render_p50_ms", TEAL), ("GPU", "gpu_p50_ms", PINK)]
    y = 120
    top = y - 8
    for g, var in groups:
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{g}</text>')
        y += 12
        for name, col, c in metrics:
            val = v(var, 10000, col)
            w = val * scale
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="20" rx="3" fill="{c}"/>')
            o.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 16}" font-size="16"><tspan font-weight="700" fill="{c}">{fmt(val)}</tspan>'
                     f'<tspan fill="{SUB}"> {name}</tspan></text>')
            y += 26
        y += 30
    x167 = X0 + 16.667 * scale
    o.append(f'<line x1="{x167:.1f}" y1="{top + 14}" x2="{x167:.1f}" y2="{y - 14}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<text x="{x167 + 6:.1f}" y="{y - 2}" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    y += 30
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">프레임 시간은 셋 가운데 가장 느린 쪽이 정한다. 세 구성 모두 게임 스레드다.</text>')
    write("actor-summary.svg", svg(y + 34, o))


# ---------- 2. one entity's frame: four steps with cost bars ----------
def flow():
    o = []
    title(o, "액터 한 개체가 한 프레임에 하는 일",
          "게임 스레드에서 위에서 아래 순서로 돈다. 막대는 호드 총수 1만의 프레임당 시간")
    CX, CW = 32, 448
    BX, BW = 500, 268
    scale = BW / T_CMC
    o.append(f'<text x="{BX}" y="110" font-size="16" fill="{SUB}">프레임당 시간(같은 축척)</text>')
    steps = [
        ("1", "개체를 걷게 하고 벽과 바닥을 검사한다", ["UCharacterMovementComponent_", "TickComponent"], BLUE, T_CMC),
        ("2", "AnimBP로 애니메이션 상태를 갱신한다", ["USkinnedMeshComponent_", "TickComponent"], PURPLE, T_SKIN),
        ("3", "병렬로 계산한 포즈를 메시에 반영한다", ["USkeletalMeshComponent_", "CompleteParallelAnimationEvaluation"], LILAC, T_ANIMEVAL),
        ("4", "바뀐 상태를 렌더 스레드로 보낸다", ["UWorld_SendAllEndOfFrameUpdates"], GRAY, None),
    ]
    y, RH, GAP = 124, 100, 22
    for i, (n, t, timer, col, ms) in enumerate(steps):
        o.append(f'<rect x="{CX}" y="{y}" width="{CW}" height="{RH}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
        o.append(f'<rect x="{CX}" y="{y}" width="6" height="{RH}" rx="3" fill="{col}"/>')
        o.append(f'<circle cx="{CX + 34}" cy="{y + 30}" r="15" fill="{col}"/>')
        o.append(f'<text x="{CX + 34}" y="{y + 36}" text-anchor="middle" font-size="16" font-weight="700" fill="white">{n}</text>')
        o.append(f'<text x="{CX + 62}" y="{y + 37}" font-size="18" font-weight="700">{t}</text>')
        for k, line in enumerate(timer):
            o.append(f'<text x="{CX + 62}" y="{y + 64 + k * 20}" font-family="{MONO}" font-size="16" fill="{SUB}">{line}</text>')
        by = y + 28
        if ms is not None:
            w = ms * scale
            pct = ms / T_FRAME * 100
            label = f"{fmt(ms)}ms · {pct:.0f}%"
            o.append(f'<rect x="{BX}" y="{by}" width="{w:.1f}" height="44" rx="4" fill="{col}"/>')
            if w > 150:
                o.append(f'<text x="{BX + 12}" y="{by + 29}" font-size="18" font-weight="700" fill="white">{label}</text>')
            else:
                o.append(f'<text x="{BX + w + 10:.1f}" y="{by + 29}" font-size="17" font-weight="700" fill="{PURPLE}">{label}</text>')
        else:
            w = T_REST * scale
            o.append(f'<rect x="{BX}" y="{by}" width="{w:.1f}" height="44" rx="4" fill="{GRAY}" fill-opacity="0.35" stroke="{GRAY}" stroke-dasharray="4 3"/>')
            o.append(f'<text x="{BX + w + 10:.1f}" y="{by + 18}" font-size="17" font-weight="700" fill="{SUB}">나머지 {fmt(T_REST)}ms · {T_REST / T_FRAME * 100:.0f}%</text>')
            o.append(f'<text x="{BX + w + 10:.1f}" y="{by + 40}" font-size="16" fill="{SUB}">이 칸과 그 밖의 일</text>')
        if i < len(steps) - 1:
            ax = CX + 34
            o.append(f'<path d="M{ax} {y + RH + 3} L{ax} {y + RH + GAP - 5}" stroke="{SUB}" stroke-width="2"/>')
            o.append(f'<path d="M{ax - 5} {y + RH + GAP - 10} L{ax} {y + RH + GAP - 3} L{ax + 5} {y + RH + GAP - 10}" fill="none" stroke="{SUB}" stroke-width="2"/>')
        y += RH + GAP
    y += 4
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y += 32
    o.append(f'<text x="32" y="{y}" font-size="17" font-weight="700">게임 스레드 한 프레임 {fmt(T_FRAME)}ms</text>')
    w167 = 16.667 * scale
    o.append(f'<rect x="{BX}" y="{y - 16}" width="{w167:.1f}" height="20" rx="3" fill="{AMBER}"/>')
    o.append(f'<text x="{BX + w167 + 10:.1f}" y="{y}" font-size="17" font-weight="700" fill="{AMBER}">16.7ms = 60fps</text>')
    y += 28
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">오브 시점 카메라, 따로 뜬 트레이스 한 회, 25–75초 구간 평균</text>')
    write("actor-frame-flow.svg", svg(y + 24, o))


# ---------- 3. cost by horde size ----------
def frame_vs_n():
    o = []
    title(o, "호드 총수에 따른 액터 군중의 비용", "오브 시점 카메라, 최종 측정, 세 번 잰 중앙값")
    ns = [500, 1000, 2000, 10000]
    nlab = ["500", "1천", "2천", "1만"]
    p50 = [v("L1-full", n, "frame_p50_ms") for n in ns]
    p95 = [v("L1-full", n, "frame_p95_ms") for n in ns]
    game = [v("L1-full", n, "game_p50_ms") for n in ns]
    L, R, T, B = 88, 548, 128, 390
    o.append(f'<text x="32" y="110" font-size="18" font-weight="700">프레임 시간(ms, 로그 축)</text>')
    xmin, xmax = math.log10(400), math.log10(13000)
    ymin, ymax = math.log10(5), math.log10(700)
    X = lambda n: L + (math.log10(n) - xmin) / (xmax - xmin) * (R - L)
    Y = lambda val: B - (math.log10(val) - ymin) / (ymax - ymin) * (B - T)
    for t in [5, 10, 20, 50, 100, 200, 500]:
        o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
        o.append(f'<text x="{L - 10}" y="{Y(t) + 6:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{t}</text>')
    for n, lab in zip(ns, nlab):
        o.append(f'<line x1="{X(n):.1f}" y1="{B}" x2="{X(n):.1f}" y2="{B + 5}" stroke="{SUB}"/>')
        o.append(f'<text x="{X(n):.1f}" y="{B + 26}" text-anchor="middle" font-size="16">{lab}</text>')
    o.append(f'<line x1="{L}" y1="{B}" x2="{R}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<line x1="{L}" y1="{T}" x2="{L}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<text x="{(L + R) / 2}" y="{B + 52}" text-anchor="middle" font-size="16" fill="{SUB}">호드 총수(로그 축)</text>')
    y167 = Y(16.667)
    o.append(f'<line x1="{L}" y1="{y167:.1f}" x2="{R}" y2="{y167:.1f}" stroke="{AMBER}" stroke-dasharray="6 4" stroke-width="1.5"/>')
    o.append(f'<text x="{R - 4}" y="{y167 + 22:.1f}" text-anchor="end" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    o.append(f'<line x1="{L}" y1="{Y(50):.1f}" x2="{R}" y2="{Y(50):.1f}" stroke="#6b7280" stroke-dasharray="2 3"/>')
    o.append(f'<text x="{L + 8}" y="{Y(50) - 8:.1f}" font-size="16" fill="#6b7280">50ms: 넘으면 하위 스텝이 는다</text>')
    pts = lambda arr: " ".join(f"{X(n):.1f},{Y(val):.1f}" for n, val in zip(ns, arr))
    o.append(f'<polyline points="{pts(p95)}" fill="none" stroke="{RED}" stroke-width="2" stroke-dasharray="5 3"/>')
    o.append(f'<polyline points="{pts(p50)}" fill="none" stroke="{BLUE}" stroke-width="2.5"/>')
    for n, val in zip(ns, p50):
        o.append(f'<circle cx="{X(n):.1f}" cy="{Y(val):.1f}" r="4.5" fill="{BLUE}"/>')
        o.append(f'<text x="{X(n) + 10:.1f}" y="{Y(val) + 22:.1f}" font-size="16" font-weight="700" fill="{BLUE}">{fmt(val)}</text>')
    nx = X(N_AT_60)
    o.append(f'<circle cx="{nx:.1f}" cy="{y167:.1f}" r="5" fill="white" stroke="{AMBER}" stroke-width="2.5"/>')
    o.append(f'<path d="M{nx:.1f} {y167 - 8:.1f} L{nx:.1f} {y167 - 30:.1f}" stroke="{AMBER}" stroke-width="1.5"/>')
    o.append(f'<text x="{nx - 6:.1f}" y="{y167 - 36:.1f}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">{N_AT_60}</text>')
    lx = 580
    o.append(f'<line x1="{lx}" y1="150" x2="{lx + 28}" y2="150" stroke="{BLUE}" stroke-width="2.5"/><text x="{lx + 36}" y="156" font-size="16">중앙값</text>')
    o.append(f'<line x1="{lx}" y1="182" x2="{lx + 28}" y2="182" stroke="{RED}" stroke-width="2" stroke-dasharray="5 3"/><text x="{lx + 36}" y="188" font-size="16">느린 쪽 5% 경계</text>')
    o.append(f'<circle cx="{lx + 14}" cy="214" r="5" fill="white" stroke="{AMBER}" stroke-width="2.5"/><text x="{lx + 36}" y="220" font-size="16">60fps를 지키는</text>')
    o.append(f'<text x="{lx + 36}" y="242" font-size="16">최대 개체 수</text>')
    # lower panel: per-entity game thread time, expected flat vs actual
    T2, B2 = 520, 660
    o.append(f'<text x="32" y="{T2 - 44}" font-size="18" font-weight="700">개체 하나당 게임 스레드 시간(µs)</text>')
    o.append(f'<text x="32" y="{T2 - 18}" font-size="16" fill="{SUB}">게임 스레드 시간(중앙값) ÷ 호드 총수. 하위 스텝 횟수는 엔진 소스로 계산했다</text>')
    per = [g / n * 1000 for g, n in zip(game, ns)]
    s2 = (B2 - T2 - 10) / 45
    steps = ["하위 스텝 1회", "1회", "1회", "8회"]
    exp_y = B2 - per[1] * s2
    for k in range(4):
        cx = 130 + k * 160
        col = RED if k == 3 else BLUE
        h = per[k] * s2
        o.append(f'<rect x="{cx - 30}" y="{B2 - h:.1f}" width="60" height="{h:.1f}" rx="3" fill="{col}"/>')
        o.append(f'<text x="{cx}" y="{B2 - h - 8:.1f}" text-anchor="middle" font-size="17" font-weight="700" fill="{col}">{fmt(per[k])}</text>')
        o.append(f'<text x="{cx}" y="{B2 + 24}" text-anchor="middle" font-size="16">{nlab[k]}</text>')
        o.append(f'<text x="{cx}" y="{B2 + 46}" text-anchor="middle" font-size="16" fill="{SUB}">{steps[k]}</text>')
    o.append(f'<line x1="70" y1="{exp_y:.1f}" x2="{W - 70}" y2="{exp_y:.1f}" stroke="{SUB}" stroke-width="1.5" stroke-dasharray="6 4"/>')
    o.append(f'<line x1="70" y1="{T2 + 6}" x2="100" y2="{T2 + 6}" stroke="{SUB}" stroke-width="1.5" stroke-dasharray="6 4"/>')
    o.append(f'<text x="108" y="{T2 + 12}" font-size="16" fill="{SUB}">예상: 개체당 시간이 1천의 값으로 일정</text>')
    o.append(f'<line x1="60" y1="{B2}" x2="{W - 60}" y2="{B2}" stroke="{SUB}"/>')
    write("actor-frame-vs-n.svg", svg(B2 + 70, o))


# ---------- 4. animation techniques: change at 10k ----------
def techniques():
    o = []
    title(o, "애니메이션 기법을 덧대 줄어든 시간",
          "호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값을 액터 군중과 비교(ms)")
    base_f, base_g = v("L1-full", 10000, "frame_p50_ms"), v("L1-full", 10000, "game_p50_ms")
    X0, scale = 40, 520 / 45.0
    y = 120
    for name, var in [("URO", "S1-uro"), ("Animation Budget Allocator(예산 2ms)", "S1-budget")]:
        f, g = v(var, 10000, "frame_p50_ms"), v(var, 10000, "game_p50_ms")
        o.append(f'<text x="{X0}" y="{y}" font-size="17" font-weight="700">{name}</text>')
        y += 12
        for lab, d, c, extra in [("프레임 시간", f - base_f, DARK, f" (−{-(f - base_f) / base_f * 100:.2f}%)"),
                                 ("게임 스레드", g - base_g, BLUE, "")]:
            w = -d * scale
            o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="22" rx="3" fill="{c}"/>')
            o.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 17}" font-size="16"><tspan font-weight="700" fill="{c}">−{fmt(-d)}{extra}</tspan>'
                     f'<tspan fill="{SUB}"> {lab}</tspan></text>')
            y += 28
        o.append(f'<text x="{X0}" y="{y + 14}" font-size="16" fill="{SUB}">덧댄 뒤의 프레임 시간 {fmt(f)}ms</text>')
        y += 50
    anim = T_SKIN + T_ANIMEVAL
    w = anim * scale
    o.append(f'<line x1="32" y1="{y - 20}" x2="{W - 32}" y2="{y - 20}" stroke="{LINE}"/>')
    o.append(f'<text x="{X0}" y="{y + 8}" font-size="17" font-weight="700">비교: 애니메이션 쪽 두 타이머의 합</text>')
    y += 20
    o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="22" rx="3" fill="{PURPLE}" fill-opacity="0.25" stroke="{PURPLE}" stroke-dasharray="4 3"/>')
    o.append(f'<text x="{X0 + w + 8:.1f}" y="{y + 17}" font-size="16"><tspan font-weight="700" fill="{PURPLE}">{fmt(anim)}</tspan>'
             f'<tspan fill="{SUB}"> 게임 스레드, 트레이스 한 회</tspan></text>')
    y += 52
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">애니메이션을 모두 지워도 줄일 수 있는 몫은 이 크기다.</text>')
    write("actor-anim-techniques.svg", svg(y + 26, o))


summary()
flow()
frame_vs_n()
techniques()
print("wrote", sorted(os.listdir(OUT)))
