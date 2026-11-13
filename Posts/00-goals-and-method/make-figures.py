# Makes the SVG figures of post 00 in images/.
# Run from the repository root: py -3 -I Posts/00-goals-and-method/make-figures.py
# Measured values come from Measurements/Full2/summary.csv and Docs/figures/full2/full-n-at-60fps.csv.
# Distances and caps in the stage map are configuration values; their source is in measurements.md.
import csv, math, os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")
os.makedirs(OUT, exist_ok=True)

rows = {}
with open(os.path.join(ROOT, "Measurements", "Full2", "summary.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        rows[(r["variant"], int(r["n"]), r["preset"])] = r


def v(variant, n, col, preset="P1"):
    return float(rows[(variant, n, preset)][col])


with open(os.path.join(ROOT, "Docs", "figures", "full2", "full-n-at-60fps.csv"), encoding="utf-8") as f:
    NAT = {r["series"]: r for r in csv.DictReader(f)}
N_AT_60 = int(NAT["8 L8-q4final"]["n_p95"])
N_AT_60_P50_NOTE = NAT["8 L8-q4final"]["note_p50"]  # "above range": 30k is still under 16.7 ms
BUDGET_MS = 16.7  # Scripts/plot.py BUDGET_MS, the line the N interpolation uses

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, LIGHTBLUE, GRAY, AMBER, RED, TEAL, PINK, DARK = (
    "#2563eb", "#93c5fd", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151")
W = 800


def svg(h, body, defs=""):
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">']
    if defs:
        head.append(f"<defs>{defs}</defs>")
    head.append(f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>')
    return "\n".join(head + body + ["</svg>"]) + "\n"


def write(name, text):
    with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def _no_zero_decimals(f):
    """27.0 -> 27. A value whose decimals are all zero is written as an integer."""
    return lambda *a, **k: re.sub(r"[.]0+$", "", f(*a, **k))


@_no_zero_decimals
def fmt(x):
    """Three significant figures, as in the post body."""
    if abs(x) >= 100:
        return f"{x:.0f}"
    if abs(x) >= 10:
        return f"{x:.1f}"
    return f"{x:.2f}"


def title(o, t, s, s2=None):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')
    if s2:
        o.append(f'<text x="32" y="98" font-size="16" fill="{SUB}">{s2}</text>')


HATCH = (f'<pattern id="hatch" width="8" height="8" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
         f'<rect width="8" height="8" fill="#f3f4f6"/><line x1="0" y1="0" x2="0" y2="8" stroke="{GRAY}" stroke-width="3"/></pattern>'
         f'<pattern id="shade" width="7" height="7" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
         f'<line x1="0" y1="0" x2="0" y2="7" stroke="#ffffff" stroke-opacity="0.55" stroke-width="2.5"/></pattern>')


# ---------- 1. pipeline: the slowest worker sets the pace (concept, no measured values) ----------
def pipeline():
    o = []
    title(o, "세 일꾼은 서로 다른 프레임을 동시에 처리한다",
          "개념도. 게임 스레드가 다음 프레임을 계산하는 동안",
          "렌더 스레드와 GPU는 앞 프레임을 처리한다")
    LX, X0, X1 = 32, 170, 770
    u = (X1 - X0) / 28.0
    lanes = [
        ("게임 스레드", BLUE, [(0, 4), (7, 11), (14, 18), (21, 25)], [(4, 7), (11, 14), (18, 21), (25, 28)]),
        ("렌더 스레드", TEAL, [(4, 7), (11, 14), (18, 21), (25, 28)], [(0, 4), (7, 11), (14, 18), (21, 25)]),
        ("GPU", PINK, [(7, 14), (14, 21), (21, 28)], [(0, 7)]),
    ]
    y, RH, GAP = 146, 46, 22
    for name, col, work, wait in lanes:
        o.append(f'<text x="{LX}" y="{y + 29}" font-size="17" font-weight="700" fill="{col}">{name}</text>')
        for a, b in wait:
            o.append(f'<rect x="{X0 + a * u:.1f}" y="{y + 6}" width="{(b - a) * u:.1f}" height="{RH - 12}" rx="3" fill="url(#hatch)" stroke="{GRAY}" stroke-width="0.8"/>')
        for k, (a, b) in enumerate(work):
            num = k + 1 if name != "GPU" else k + 1
            o.append(f'<rect x="{X0 + a * u:.1f}" y="{y}" width="{(b - a) * u - 2:.1f}" height="{RH}" rx="5" fill="{col}"/>')
            o.append(f'<text x="{X0 + (a + b) / 2 * u:.1f}" y="{y + 29}" text-anchor="middle" font-size="17" font-weight="700" fill="white">{num}</text>')
        y += RH + GAP
    # frame interval bracket under the GPU lane: frame 2 on GPU
    by = y + 4
    a, b = X0 + 14 * u, X0 + 21 * u
    o.append(f'<path d="M{a:.1f} {by} L{a:.1f} {by + 10} L{b:.1f} {by + 10} L{b:.1f} {by}" fill="none" stroke="{INK}" stroke-width="1.5"/>')
    o.append(f'<text x="{(a + b) / 2:.1f}" y="{by + 34}" text-anchor="middle" font-size="16" font-weight="700">프레임 시간</text>')
    o.append(f'<text x="{(a + b) / 2:.1f}" y="{by + 56}" text-anchor="middle" font-size="16" fill="{SUB}">= 가장 느린 GPU의 한 칸</text>')
    ly = by + 92
    o.append(f'<rect x="{LX}" y="{ly - 15}" width="34" height="20" rx="3" fill="url(#hatch)" stroke="{GRAY}" stroke-width="0.8"/>')
    o.append(f'<text x="{LX + 44}" y="{ly}" font-size="16">기다림: 앞서 나갈 수 있는 프레임 수에 한도가 있어 쉰다</text>')
    o.append(f'<text x="{LX}" y="{ly + 26}" font-size="16">칸의 숫자는 프레임 번호다. 칸의 길이는 지어낸 값이다.</text>')
    o.append(f'<line x1="32" y1="{ly + 46}" x2="{W - 32}" y2="{ly + 46}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{ly + 74}" font-size="16" fill="{SUB}">게임 스레드가 가장 느리면 게임 스레드의 한 칸이 프레임 시간이 된다.</text>')
    write("frame-pipeline.svg", svg(ly + 96, o, HATCH))


# ---------- 2. judging a difference: median and three-run range ----------
def judging():
    o = []
    title(o, "차이 판정: 중앙값의 차이가 범위보다 커야 다르다",
          "호드 총수 1만, 오브 시점 카메라, 프레임 시간 중앙값(ms)",
          "점은 세 번 잰 값의 중앙값, 가로선은 세 번의 최솟값–최댓값")
    lo, hi = 7.5, 9.9
    X0, X1 = 330, 700
    X = lambda ms: X0 + (ms - lo) / (hi - lo) * (X1 - X0)
    base = ("L8-q4final", "최종 구성(8단계)")
    items = [
        ("L8-q4final", "최종 구성(8단계)", None),
        ("L8-q4final-aa", "같은 구성을 측정 끝에 다시", "구별되지 않음"),
        ("A8-fix", "전환 대책을 뺐을 때", "구별되지 않음"),
        ("A8-budget", "애니메이션 예산을 뺐을 때", "차이 있음"),
    ]
    b_med = v(base[0], 10000, "frame_p50_ms")
    b_rng = v(base[0], 10000, "frame_p50_ms_max") - v(base[0], 10000, "frame_p50_ms_min")
    y = 164
    for t in [7.5, 8.0, 8.5, 9.0, 9.5]:
        o.append(f'<line x1="{X(t):.1f}" y1="{y - 22}" x2="{X(t):.1f}" y2="{y + 4 * 78 - 40}" stroke="{LINE}"/>')
        o.append(f'<text x="{X(t):.1f}" y="{y - 28}" text-anchor="middle" font-size="16" fill="{SUB}">{t:.1f}</text>')
    for var, label, verdict in items:
        med = v(var, 10000, "frame_p50_ms")
        mn, mx = v(var, 10000, "frame_p50_ms_min"), v(var, 10000, "frame_p50_ms_max")
        col = DARK if verdict is None else (RED if verdict == "차이 있음" else BLUE)
        o.append(f'<text x="32" y="{y + 6}" font-size="17" font-weight="700">{label}</text>')
        o.append(f'<line x1="{X(mn):.1f}" y1="{y}" x2="{X(mx):.1f}" y2="{y}" stroke="{col}" stroke-width="3"/>')
        for e in (mn, mx):
            o.append(f'<line x1="{X(e):.1f}" y1="{y - 9}" x2="{X(e):.1f}" y2="{y + 9}" stroke="{col}" stroke-width="2"/>')
        o.append(f'<circle cx="{X(med):.1f}" cy="{y}" r="6" fill="{col}"/>')
        o.append(f'<text x="{X(mx) + 14:.1f}" y="{y + 6}" font-size="16" font-weight="700" fill="{col}">{fmt(med)}</text>')
        if verdict is not None:
            d = med - b_med
            rng = max(mx - mn, b_rng)
            sign = "+" if d >= 0 else "−"
            rel = "&gt;" if abs(d) > rng else "&lt;"
            o.append(f'<text x="32" y="{y + 32}" font-size="16" fill="{SUB}">차이 {sign}{abs(d):.3f}  {rel}  큰 쪽 범위 {rng:.3f}</text>')
            o.append(f'<text x="{X0}" y="{y + 32}" font-size="16" font-weight="700" fill="{col}">→ {verdict}</text>')
        else:
            o.append(f'<text x="32" y="{y + 32}" font-size="16" fill="{SUB}">기준. 범위 {b_rng:.3f}</text>')
        y += 78
    o.append(f'<line x1="32" y1="{y - 20}" x2="{W - 32}" y2="{y - 20}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 8}" font-size="16" fill="{SUB}">범위는 두 구성 가운데 큰 쪽을 쓴다. 차이가 그보다 작으면 "구별되지 않았다"고 적는다.</text>')
    write("judging-difference.svg", svg(y + 32, o))


# ---------- 3. max entities at 60 fps: how the number is read ----------
def max_entities():
    o = []
    title(o, "60fps를 지키는 최대 개체 수를 읽는 법",
          "최종 구성(8단계), 오브 시점 카메라, 세 번 잰 중앙값. 네 가지 호드 총수에서 쟀다")
    ns = [5000, 10000, 20000, 30000]
    nlab = ["5천", "1만", "2만", "3만"]
    p95 = [v("L8-q4final", n, "frame_p95_ms") for n in ns]
    p50 = [v("L8-q4final", n, "frame_p50_ms") for n in ns]
    L, R, T, B = 90, 600, 120, 470
    X = lambda n: L + n / 32000 * (R - L)
    Y = lambda ms: B - ms / 20.0 * (B - T)
    for t in [0, 5, 10, 15, 20]:
        o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
        o.append(f'<text x="{L - 10}" y="{Y(t) + 6:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{t}</text>')
    o.append(f'<text x="32" y="{T - 12}" font-size="16" fill="{SUB}">프레임 시간(ms)</text>')
    for n, lab in zip(ns, nlab):
        o.append(f'<line x1="{X(n):.1f}" y1="{B}" x2="{X(n):.1f}" y2="{B + 5}" stroke="{SUB}"/>')
        o.append(f'<text x="{X(n):.1f}" y="{B + 26}" text-anchor="middle" font-size="16">{lab}</text>')
    o.append(f'<line x1="{L}" y1="{B}" x2="{R}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<line x1="{L}" y1="{T}" x2="{L}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<text x="{(L + R) / 2}" y="{B + 52}" text-anchor="middle" font-size="16" fill="{SUB}">호드 총수</text>')
    # target N
    o.append(f'<line x1="{X(10000):.1f}" y1="{T}" x2="{X(10000):.1f}" y2="{B}" stroke="{GRAY}" stroke-width="1.5" stroke-dasharray="3 3"/>')
    o.append(f'<text x="{X(10000) + 6:.1f}" y="{T + 18}" font-size="16" fill="{SUB}">목표 1만</text>')
    y167 = Y(BUDGET_MS)
    o.append(f'<line x1="{L}" y1="{y167:.1f}" x2="{R}" y2="{y167:.1f}" stroke="{AMBER}" stroke-dasharray="6 4" stroke-width="1.5"/>')
    o.append(f'<text x="{L + 8}" y="{y167 - 8:.1f}" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')

    def curve(vals):
        # log interpolation between measured N, as Scripts/plot.py n_at_budget does
        pts = []
        for (n0, m0), (n1, m1) in zip(zip(ns, vals), zip(ns[1:], vals[1:])):
            for k in range(0, 21):
                t = k / 20
                n = math.exp(math.log(n0) + t * (math.log(n1) - math.log(n0)))
                pts.append((n, m0 + t * (m1 - m0)))
        return " ".join(f"{X(n):.1f},{Y(m):.1f}" for n, m in pts)

    o.append(f'<polyline points="{curve(p95)}" fill="none" stroke="{RED}" stroke-width="2.5" stroke-dasharray="6 3"/>')
    o.append(f'<polyline points="{curve(p50)}" fill="none" stroke="{BLUE}" stroke-width="2.5"/>')
    for n, a, b in zip(ns, p95, p50):
        o.append(f'<circle cx="{X(n):.1f}" cy="{Y(a):.1f}" r="4.5" fill="{RED}"/>')
        o.append(f'<circle cx="{X(n):.1f}" cy="{Y(b):.1f}" r="4.5" fill="{BLUE}"/>')
        o.append(f'<text x="{X(n) - 8:.1f}" y="{Y(a) - 10:.1f}" text-anchor="end" font-size="16" font-weight="700" fill="{RED}">{fmt(a)}</text>')
        o.append(f'<text x="{X(n) + 8:.1f}" y="{Y(b) + 22:.1f}" font-size="16" font-weight="700" fill="{BLUE}">{fmt(b)}</text>')
    nx = X(N_AT_60)
    o.append(f'<line x1="{nx:.1f}" y1="{y167:.1f}" x2="{nx:.1f}" y2="{B}" stroke="{AMBER}" stroke-width="1.5"/>')
    o.append(f'<circle cx="{nx:.1f}" cy="{y167:.1f}" r="6" fill="white" stroke="{AMBER}" stroke-width="3"/>')
    o.append(f'<text x="{nx:.1f}" y="{B - 10}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}" '
             f'paint-order="stroke" stroke="white" stroke-width="5">{N_AT_60:,}</text>')
    lx = 624
    o.append(f'<line x1="{lx}" y1="150" x2="{lx + 28}" y2="150" stroke="{RED}" stroke-width="2.5" stroke-dasharray="6 3"/>')
    o.append(f'<text x="{lx + 36}" y="156" font-size="16">느린 쪽</text><text x="{lx + 36}" y="178" font-size="16">5% 경계</text>')
    o.append(f'<line x1="{lx}" y1="212" x2="{lx + 28}" y2="212" stroke="{BLUE}" stroke-width="2.5"/>')
    o.append(f'<text x="{lx + 36}" y="218" font-size="16">중앙값</text>')
    o.append(f'<circle cx="{lx + 14}" cy="250" r="6" fill="white" stroke="{AMBER}" stroke-width="3"/>')
    o.append(f'<text x="{lx + 36}" y="256" font-size="16">16.7ms를</text><text x="{lx + 36}" y="278" font-size="16">넘기 직전의</text>'
             f'<text x="{lx + 36}" y="300" font-size="16">호드 총수</text>')
    y = B + 84
    o.append(f'<line x1="32" y1="{y - 20}" x2="{W - 32}" y2="{y - 20}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 6}" font-size="16" fill="{SUB}">느린 쪽 5% 경계가 16.7ms를 넘는 곳을 잰 점 사이에서 보간한다(로그 보간).</text>')
    note = "중앙값은 3만에서도 16.7ms 안이다." if N_AT_60_P50_NOTE == "above range" else ""
    o.append(f'<text x="32" y="{y + 30}" font-size="16" fill="{SUB}">{note} 그래서 느린 쪽으로 판정하면 한계가 더 낮게 나온다.</text>')
    write("max-entities-method.svg", svg(y + 54, o))


# ---------- 4. stage map: what draws each distance band in each stage ----------
def stage_map():
    o = []
    title(o, "여덟 단계: 거리마다 무엇으로 그리나",
          "앞 단계 위에 하나씩 더한다. 가로는 카메라에서의 거리이고 축척이 아니다")
    LX, X0, X1 = 32, 330, 768
    # band edges in metres -> x position (not to scale)
    edges = [0, 30, 40, 80, 200, 500]
    xs = [X0, X0 + 92, X0 + 120, X0 + 210, X0 + 330, X1]
    XE = dict(zip(edges, xs))
    ACT, STAT, VAT, ISKM, FX = RED, GRAY, LIGHTBLUE, BLUE, TEAL
    names = {ACT: "액터", STAT: "ISM", VAT: "VAT", ISKM: "ISKM", FX: "입자"}
    # (number, name, change line, [(from_m, to_m, kind, shadow_off)])
    stages = [
        ("1", "액터 군중", "모든 개체가 ACharacter 액터다", [(0, 500, ACT, False)]),
        ("2", "Mass 로직", "로직을 Mass로 옮기고, 애니메이션 없는 ISM으로 그린다", [(0, 500, STAT, False)]),
        ("3", "ISM + VAT", "인스턴스에 VAT 애니메이션을 입히고, 200m 밖은 그리지 않는다", [(0, 200, VAT, False), (200, 500, None, False)]),
        ("4", "액터 하이드레이션", "30m 안을 액터로 바꾸고(상한 500), 애니메이션 예산을 둔다", [(0, 30, ACT, False), (30, 200, VAT, False), (200, 500, None, False)]),
        ("5", "Simulation LOD", "먼 개체의 계산을 줄이고, 엔티티를 다시 쓴다", [(0, 30, ACT, False), (30, 200, VAT, False), (200, 500, None, False)]),
        ("6", "ISKM", "VAT 자리를 ISKM으로 바꾸고, 바운드를 고정한다", [(0, 30, ACT, False), (30, 200, ISKM, False), (200, 500, None, False)]),
        ("7", "Niagara 배경 무리", "200m 밖 엔티티 자리에 입자를 그린다", [(0, 30, ACT, False), (30, 200, ISKM, False), (200, 500, FX, False)]),
        ("8", "품질 올리기", "40m 안을 액터로(상한 1,000), 80m 밖은 그림자를 끈다", [(0, 40, ACT, False), (40, 80, ISKM, False), (80, 200, ISKM, True), (200, 500, FX, True)]),
    ]
    y = 118
    for e, x in zip(edges, xs):
        anchor = "middle" if e < 500 else "end"
        o.append(f'<text x="{x:.1f}" y="{y}" text-anchor="{anchor}" font-size="16" fill="{SUB}">{e if e < 500 else "500m"}</text>')
    y += 12
    BH = 30
    for num, name, change, segs in stages:
        o.append(f'<circle cx="{LX + 14}" cy="{y + 15}" r="14" fill="{DARK}"/>')
        o.append(f'<text x="{LX + 14}" y="{y + 21}" text-anchor="middle" font-size="16" font-weight="700" fill="white">{num}</text>')
        o.append(f'<text x="{LX + 38}" y="{y + 21}" font-size="17" font-weight="700">{name}</text>')
        o.append(f'<text x="{LX + 38}" y="{y + 50}" font-size="16" fill="{SUB}">{change}</text>')
        for a, b, kind, shadow_off in segs:
            xa, xb = XE[a], XE[b]
            if kind is None:
                o.append(f'<rect x="{xa + 1:.1f}" y="{y}" width="{xb - xa - 2:.1f}" height="{BH}" rx="4" fill="white" stroke="{GRAY}" stroke-dasharray="4 3"/>')
                o.append(f'<text x="{(xa + xb) / 2:.1f}" y="{y + 21}" text-anchor="middle" font-size="16" fill="{SUB}">그리지 않음</text>')
                continue
            o.append(f'<rect x="{xa + 1:.1f}" y="{y}" width="{xb - xa - 2:.1f}" height="{BH}" rx="4" fill="{kind}"/>')
            if shadow_off:
                o.append(f'<rect x="{xa + 1:.1f}" y="{y}" width="{xb - xa - 2:.1f}" height="{BH}" rx="4" fill="url(#shade)"/>')
            if xb - xa >= 60:
                fill = INK if kind in (VAT, STAT) else "white"
                o.append(f'<text x="{(xa + xb) / 2:.1f}" y="{y + 21}" text-anchor="middle" font-size="16" font-weight="700" fill="{fill}">{names[kind]}</text>')
        y += 72
    # legend, one item per line so that nothing runs past the right edge
    o.append(f'<line x1="32" y1="{y - 8}" x2="{W - 32}" y2="{y - 8}" stroke="{LINE}"/>')
    y += 14
    legend = [(ACT, False, "액터: 스켈레탈 메시와 AnimBP를 가진 ACharacter"),
              (STAT, False, "ISM: 애니메이션 없는 정적 메시 인스턴스"),
              (VAT, False, "VAT: 애니메이션을 텍스처로 구워 입힌 ISM"),
              (ISKM, False, "ISKM: 스켈레탈 메시를 그대로 인스턴스로 그린다"),
              (FX, False, "입자: 엔티티 자리에 그리는 Niagara 입자"),
              (ISKM, True, "빗금: 그림자를 그리지 않는다")]
    for kind, shade, lab in legend:
        o.append(f'<rect x="32" y="{y}" width="22" height="18" rx="3" fill="{kind}"/>')
        if shade:
            o.append(f'<rect x="32" y="{y}" width="22" height="18" rx="3" fill="url(#shade)"/>')
        o.append(f'<text x="64" y="{y + 15}" font-size="16">{lab}</text>')
        y += 28
    y += 6
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">액터 구간은 화면 안의 거리다. 화면 밖은 15m 안만 액터로 바꾼다.</text>')
    write("stage-map.svg", svg(y + 36, o, HATCH))


pipeline()
judging()
max_entities()
stage_map()
print("wrote", sorted(os.listdir(OUT)))
