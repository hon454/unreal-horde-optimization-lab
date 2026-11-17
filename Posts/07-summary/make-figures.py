# Makes the SVG figures of post 07 in images/.
# Run from the repository root: py -3 -I Posts/07-summary/make-figures.py
# Measured values come from the full batch, Test package de0e21b:
#   Measurements/Full4/summary.csv   step medians and run ranges (horde 10000, moving scene V0, orb camera P1)
#   Measurements/Full4/runs.csv      run ranges of the resource figure (the runs listed in summary.csv runs_used)
#   Docs/figures/full4/full-n-at-60fps.csv   max N of steps 1 and 2 only (posts write max N for these two steps only)
# The step-to-step rule is Scripts/plot.py distinguishable() (Docs/Guides/measurement.md "차이 판정"), copied here so the
# post's figures do not import the batch scripts. Step names follow Docs/Guides/glossary.md 5.1 and Scripts/plot-readme.py.
# LAB_FIG_CHECK=1 prints every value written into a figure, for the scratch comparison against the CSV files.
import csv
import os
import re
import statistics
from decimal import Decimal, ROUND_HALF_UP

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")

BATCH = "Full4"
TARGET_N = 10000
BUDGET_MS = Decimal("16.7")   # Scripts/plot.py BUDGET_MS, the 60 fps line and "16.7ms까지의 여유"
DISTINCT_FLOOR_MS = 0.5       # Docs/Guides/measurement.md "차이 판정", Scripts/plot.py FULL_DISTINCT_FLOOR_MS

rows = {}
with open(os.path.join(ROOT, "Measurements", BATCH, "summary.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        rows[(r["variant"], int(r["n"]), r["preset"])] = r

maxn = {}
with open(os.path.join(ROOT, "Docs", "figures", "full4", "full-n-at-60fps.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        maxn[r["series"].split(" ", 1)[0]] = r["n_p95"]


def row(cond):
    return rows[(cond, TARGET_N, "P1")]


def v(cond, col):
    return float(row(cond)[col])


def dv(a, b, col):
    """b - a from the CSV text with Decimal (half up later, as in the other posts)."""
    return Decimal(row(b)[col]) - Decimal(row(a)[col])


def distinguishable(a, b, key="frame_p95_ms"):
    """Scripts/plot.py distinguishable(): the median difference is at least 0.5 ms and larger than the larger run range."""
    d = abs(v(a, key) - v(b, key))
    rng = lambda c: v(c, key + "_max") - v(c, key + "_min")
    return d >= DISTINCT_FLOOR_MS and d > max(rng(a), rng(b))


def run_values(cond, col):
    """Per-run values behind a summary row: the runs listed in summary.csv runs_used."""
    used = row(cond)["runs_used"].split()
    out = []
    with open(os.path.join(ROOT, "Measurements", BATCH, "runs.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if (r["variant"] == cond and int(r["n"]) == TARGET_N and r["preset"] == "P1"
                    and r["label"].rsplit("-", 1)[1] in used and r[col]):
                out.append(float(r[col]))
    if len(out) != len(used):
        raise SystemExit(f"{cond} {col}: runs {used}, values {out}")
    if abs(statistics.median(out) - v(cond, col)) > 0.01:
        raise SystemExit(f"{cond} {col}: median of runs {out} is not the summary value")
    return out


# ---------- formatting (posting.md: three significant figures, 27 not 27.0, counts are not shortened) ----------
def _no_zero_decimals(fn):
    return lambda *a, **k: re.sub(r"[.]0+$", "", fn(*a, **k))


@_no_zero_decimals
def fmt(x):
    """Three significant figures, half up from the decimal text (3.865 -> 3.87). Thousands get a comma (9,250)."""
    x = x if isinstance(x, Decimal) else Decimal(repr(x))
    a = abs(x)
    if a >= 1000:
        q = Decimal(10) ** (len(str(int(a))) - 3)
        s = f"{int((a / q).quantize(Decimal(1), rounding=ROUND_HALF_UP) * q):,}"
    else:
        q = "1" if a >= 100 else "0.1" if a >= 10 else "0.01"
        s = str(a.quantize(Decimal(q), rounding=ROUND_HALF_UP))
    return ("−" if x < 0 else "") + s


def mb(x):
    """Memory in MB to the nearest integer (9252.58 -> 9,253), as the README figures write it."""
    x = x if isinstance(x, Decimal) else Decimal(repr(x))
    return f"{int(x.quantize(Decimal(1), rounding=ROUND_HALF_UP)):,}"


def fmtd(x):
    """Signed difference in ms with two decimals, minus sign U+2212."""
    x = x if isinstance(x, Decimal) else Decimal(repr(x))
    s = str(abs(x).quantize(Decimal("0.01"), rounding=ROUND_HALF_UP))
    if s == "0.00":
        return "0"
    s = re.sub(r"[.]0+$", "", s)
    return ("+" if x > 0 else "−") + s


def count(x):
    """Counts are not shortened: 9075.0 -> 9,075."""
    return f"{round(x):,}"


def esc(t):
    return str(t).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


# ---------- style (same as the other posts and Scripts/plot-readme.py) ----------
FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, PURPLE, GRAY, AMBER, RED, TEAL, PINK, DARK = (
    "#2563eb", "#9333ea", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151")
NAVY, SHADE = "#1f2937", "#fef3c7"
W = 800

# Representation steps (4, 6, 7) get diagonal stripes on the same colour, as in the README figures.
HATCH = ('<pattern id="rep" patternUnits="userSpaceOnUse" width="7" height="7" patternTransform="rotate(45)">'
         '<rect width="3" height="7" fill="#ffffff" fill-opacity="0.55"/></pattern>')


def svg(h, body, defs=""):
    h = int(round(h))
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">']
    if defs:
        head.append(f"<defs>{defs}</defs>")
    head.append(f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>')
    return "\n".join(head + body + ["</svg>"]) + "\n"


def write(name, text_):
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(text_)


def title(o, t, subs):
    o.append(f'<text x="32" y="44" font-size="22" font-weight="700">{esc(t)}</text>')
    y = 72
    for s in subs:
        o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">{esc(s)}</text>')
        y += 22
    return y


def text(x, y, t, size=16, fill=INK, anchor="start", weight=None, halo=False):
    attrs = f'x="{x:.1f}" y="{y:.1f}" font-size="{size}" text-anchor="{anchor}"' + (f' font-weight="{weight}"' if weight else "")
    out = ""
    if halo:
        out += f'<text {attrs} fill="{BG}" stroke="{BG}" stroke-width="5" stroke-linejoin="round">{esc(t)}</text>'
    return out + f'<text {attrs} fill="{fill}">{esc(t)}</text>'


def text_w(t, size=16):
    """Rough text width (Malgun Gothic on Edge): Hangul 1 em, space 0.3 em, other ASCII 0.55 em (Scripts/plot-readme.py)."""
    return sum(size * (1.0 if ord(ch) > 0x2000 else 0.3 if ch == " " else 0.55) for ch in t)


CHECK = []  # (figure, label, text) printed with LAB_FIG_CHECK=1


# Steps: glossary 5.1 names, one line of what the step changed, and the role of the condition table (start, earn, quality).
STEPS = [
    (1, "액터 기준선", "ACharacter 액터, 캡슐과 이동 컴포넌트", "start"),
    (2, "Mass, 엔진 기본 경로", "엔진 이동·회피·표현, 가까운 개체는 액터", "earn"),
    (3, "직접 짠 분리", "분리 프로세서, 방향 보간 병렬 사본", "earn"),
    (4, "애니메이션 표현", "VAT 인스턴스, 액터 애니메이션 예산, 최소 유지 시간", "quality"),
    (5, "Simulation LOD + 엔티티 풀", "먼 개체의 로직을 드물게, 엔티티를 다시 쓴다", "earn"),
    (6, "ISKM", "35~200m를 ISKM으로, 고정 바운드", "quality"),
    (7, "배경 무리", "200m 밖을 Niagara 메시 입자로", "quality"),
    (8, "남은 비용 줄이기", "80m 밖 그림자 끔, 액터 메시 고정 바운드", "earn"),
]
HATCHED = {n for n, _, _, role in STEPS if role == "quality"}


def cond(n):
    return f"V0-s{n}"


def check_roles():
    """The roles in the figure must be the condition table's (full-ladder.csv kind)."""
    with open(os.path.join(ROOT, "Docs", "figures", "full4", "full-ladder.csv"), encoding="utf-8") as f:
        kinds = {int(r["step"]): r["kind"] for r in csv.DictReader(f)}
    for n, _, _, role in STEPS:
        if kinds[n] != role:
            raise SystemExit(f"step {n}: role {role} is not the condition table's {kinds[n]}")


def margin(n):
    return BUDGET_MS - Decimal(row(cond(n))["frame_p95_ms"])


def hatch_bar(o, x, y, w, h, colour, num, rx=3, opacity=1.0):
    op = "" if opacity == 1.0 else f' fill-opacity="{opacity}"'
    o.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h}" rx="{rx}" fill="{colour}"{op}/>')
    if num in HATCHED:
        o.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h}" rx="{rx}" fill="url(#rep)"/>')


def hatch_key(o, y):
    o.append(f'<rect x="32" y="{y - 13}" width="34" height="16" rx="3" fill="{GRAY}"/>')
    o.append(f'<rect x="32" y="{y - 13}" width="34" height="16" rx="3" fill="url(#rep)"/>')
    o.append(text(74, y, "빗금 막대: 표현을 더하는 단계(4·6·7). 나머지는 비용을 줄이는 단계와 출발점(단계 1)", fill=SUB))


# ---------- 1. ladder: frame p95 and p50 of steps 1-8, two scales ----------
def ladder():
    check_roles()
    o = []
    y = title(o, "여덟 단계의 프레임 시간",
              ["호드 총수 1만, 이동 장면, 오브 시점 카메라, 세 번 잰 중앙값(ms)"])
    y += 22
    o.append(f'<rect x="32" y="{y - 13}" width="22" height="16" rx="2" fill="{DARK}"/>')
    o.append(text(62, y, "굵은 막대: 프레임 p95(느린 쪽 5%의 경계)"))
    o.append(f'<rect x="420" y="{y - 8}" width="22" height="7" rx="2" fill="{DARK}" fill-opacity="0.45"/>')
    o.append(text(450, y, "가는 막대: 프레임 p50(중앙값)"))
    y += 26
    o.append(f'<line x1="32" y1="{y - 5}" x2="54" y2="{y - 5}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(text(62, y, "16.7ms(60fps). 오른쪽 글자는 16.7ms까지의 여유, 넘는 단계는 최대 N"))
    X0, X1 = 32, 680
    RH = 72
    panels = ((700.0, 100, [1]), (40.0, 5, [2, 3, 4, 5, 6, 7, 8]))
    first_inside = None
    for pi, (axis_max, tick, nums) in enumerate(panels):
        sc = (X1 - X0) / axis_max
        y += 40
        o.append(f'<line x1="32" y1="{y - 22}" x2="{W - 32}" y2="{y - 22}" stroke="{LINE}"/>')
        span = f"단계 {nums[0]}" if len(nums) == 1 else f"단계 {nums[0]}–{nums[-1]}"
        extra = "" if pi == 0 else f", 축척 A의 {round(panels[0][0] / axis_max, 1):g}배"
        o.append(text(32, y, f"축척 {'AB'[pi]}: 0–{axis_max:g}ms({span}{extra})", fill=SUB))
        y += 10
        top = y + 8
        bottom = top + RH * len(nums)
        for k in range(int(axis_max // tick) + 1):
            gx = X0 + k * tick * sc
            o.append(f'<line x1="{gx:.1f}" y1="{top + 22}" x2="{gx:.1f}" y2="{bottom - 10}" stroke="#eef0f3"/>')
            o.append(text(gx, bottom + 8, f"{k * tick:g}", fill=SUB, anchor="middle"))
        lx = X0 + float(BUDGET_MS) * sc
        ry = top
        for num in nums:
            c = cond(num)
            name, detail = next((s[1], s[2]) for s in STEPS if s[0] == num)
            p95, p50 = v(c, "frame_p95_ms"), v(c, "frame_p50_ms")
            o.append(f'<text x="32" y="{ry + 16}" font-size="17" font-weight="700">단계 {num} {esc(name)}'
                     f'<tspan font-size="16" font-weight="400" fill="{SUB}">  {esc(detail)}</tspan></text>')
            w95, w50 = min(p95, axis_max) * sc, min(p50, axis_max) * sc
            hatch_bar(o, X0, ry + 24, w95, 22, DARK, num)
            hatch_bar(o, X0, ry + 50, w50, 7, DARK, num, rx=2, opacity=0.45)
            o.append(f'<line x1="{lx:.1f}" y1="{ry + 20}" x2="{lx:.1f}" y2="{ry + 62}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
            a = f'x="{X0 + w95 - 8:.1f}" y="{ry + 41}" font-size="16" text-anchor="end"'
            tx = f'p50 {fmt(p50)} · <tspan font-weight="700">p95 {fmt(p95)}</tspan>'
            o.append(f'<text {a} fill="{DARK}" stroke="{DARK}" stroke-width="3" stroke-linejoin="round">{tx}</text><text {a} fill="white">{tx}</text>')
            CHECK.append(("ladder", f"{c} frame_p50_ms / frame_p95_ms", (fmt(p50), fmt(p95))))
            if Decimal(row(c)["frame_p95_ms"]) < BUDGET_MS:
                if first_inside is None:
                    first_inside = num
                t = f"여유 {fmt(margin(num))}ms"
                o.append(text(lx + 10, ry + 41, t, fill=AMBER, weight="700", halo=True))
                CHECK.append(("ladder", f"{c} margin", fmt(margin(num))))
                if num == first_inside:
                    o.append(text(lx + 18 + text_w(t), ry + 41, "처음 16.7ms 안", fill=SUB))
            else:
                n = int(maxn[str(num)])
                approx = f"{round(n, -1 if n < 1000 else -2):,}"  # two significant figures (measurement.md "지표")
                o.append(text(X0 + w95 + 12, ry + 41, f"최대 N 약 {approx}", fill=AMBER, weight="700", halo=True))
                CHECK.append(("ladder", f"{c} max N", (n, approx)))
            ry += RH
        y = bottom + 14
    if first_inside != 3:
        raise SystemExit(f"the first step inside 16.7 ms is {first_inside}; the post says step 3")
    for n in (1, 2):
        if row(cond(n))["frame_p95_ms"] and Decimal(row(cond(n))["frame_p95_ms"]) < BUDGET_MS:
            raise SystemExit(f"step {n} is inside 16.7 ms; max N is written only for steps outside")
    y += 24
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    for line in ["단계 1과 2는 1만에서 같은 일을 하지 못해 값만 보고하고, 최대 N으로 견준다.",
                 "최대 N: 프레임 p95가 16.7ms를 넘기 직전의 호드 총수(잰 총수 사이를 로그 보간).",
                 "16.7ms까지의 여유: 16.7ms에서 프레임 p95를 뺀 값."]:
        o.append(text(32, y + 10, line, fill=SUB))
        y += 23
    hatch_key(o, y + 18)
    write("ladder.svg", svg(y + 42, o, HATCH))


# ---------- 2. steps 3-8: frame p95 with run ranges, the rise and the take-back ----------
def steps_3_to_8():
    nums = [3, 4, 5, 6, 7, 8]
    o = []
    y = title(o, "단계 3에서 8까지의 프레임 p95",
              ["호드 총수 1만, 이동 장면, 오브 시점 카메라, 세 번 잰 중앙값(ms)",
               "점은 중앙값, 세로선은 세 실행의 범위다. 노란 칸이 표현을 더하는 단계다"])
    xs = [136 + 117 * k for k in range(len(nums))]   # column centres 136..721
    colw = 111
    A_LO, A_HI, A_PX = 13.5, 17.0, 78.0
    top = y + 64
    bot = top + (A_HI - A_LO) * A_PX
    Y = lambda ms: bot - (ms - A_LO) * A_PX
    for n, x in zip(nums, xs):
        if n in HATCHED:
            o.append(f'<rect x="{x - colw / 2:.1f}" y="{top - 56:.1f}" width="{colw}" height="{bot - top + 138:.1f}" fill="{SHADE}"/>')
    for t in (14, 15, 16, 17):
        o.append(f'<line x1="86" y1="{Y(t):.1f}" x2="{W - 32}" y2="{Y(t):.1f}" stroke="#eef0f3"/>')
        o.append(text(76, Y(t) + 5, str(t), fill=SUB, anchor="end"))
    o.append(f'<line x1="86" y1="{Y(float(BUDGET_MS)):.1f}" x2="{W - 32}" y2="{Y(float(BUDGET_MS)):.1f}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="6 4"/>')
    o.append(text(W - 36, Y(float(BUDGET_MS)) - 7, "16.7ms(60fps)", fill=AMBER, anchor="end", weight="700", halo=True))
    pts = []
    for n, x in zip(nums, xs):
        c = cond(n)
        p95, lo, hi = v(c, "frame_p95_ms"), v(c, "frame_p95_ms_min"), v(c, "frame_p95_ms_max")
        pts.append((x, Y(p95)))
        o.append(f'<line x1="{x}" y1="{Y(lo):.1f}" x2="{x}" y2="{Y(hi):.1f}" stroke="{INK}" stroke-width="2"/>')
        for b in (lo, hi):
            o.append(f'<line x1="{x - 6}" y1="{Y(b):.1f}" x2="{x + 6}" y2="{Y(b):.1f}" stroke="{INK}" stroke-width="2"/>')
        CHECK.append(("steps-3-to-8", f"{c} frame_p95_ms (min, max)", (fmt(p95), fmt(lo), fmt(hi))))
    d = " ".join(f'{"M" if i == 0 else "L"}{px:.1f},{py:.1f}' for i, (px, py) in enumerate(pts))
    o.append(f'<path d="{d}" fill="none" stroke="{NAVY}" stroke-width="2.5"/>')
    for n, (px, py) in zip(nums, pts):
        o.append(f'<circle cx="{px:.1f}" cy="{py:.1f}" r="5.5" fill="{NAVY}"/>')
        o.append(text(px + 12, py + 24, fmt(v(cond(n), "frame_p95_ms")), weight="700", halo=True))
    # step-to-step differences under the points: 3->4 ... 6->7 are not distinguishable, 7->8 is
    for k in range(1, len(nums)):
        a, b = cond(nums[k - 1]), cond(nums[k])
        delta = dv(a, b, "frame_p95_ms")
        dist = distinguishable(a, b)
        want = nums[k] == 8
        if dist != want:
            raise SystemExit(f"{a} -> {b}: distinguishable {dist}; the post says {want}")
        mx = (xs[k - 1] + xs[k]) / 2
        o.append(text(mx, Y(13.62), fmtd(delta), fill=(RED if dist else GRAY), anchor="middle",
                      weight=("700" if dist else None), halo=True))
        CHECK.append(("steps-3-to-8", f"{a}->{b} delta", (fmtd(delta), dist)))
    # the rise over steps 3-7 and the take-back in step 8
    c3, c7, c8 = cond(3), cond(7), cond(8)
    rise, back, net = dv(c3, c7, "frame_p95_ms"), dv(c7, c8, "frame_p95_ms"), dv(c3, c8, "frame_p95_ms")
    if not distinguishable(c3, c7) or not distinguishable(c7, c8) or distinguishable(c3, c8):
        raise SystemExit("the post says: 3->7 and 7->8 are distinguishable, 3->8 is not")
    by = top - 26
    o.append(f'<path d="M{xs[0]},{by + 8:.1f} L{xs[0]},{by:.1f} L{xs[4]},{by:.1f} L{xs[4]},{by + 8:.1f}" fill="none" stroke="{RED}" stroke-width="2"/>')
    o.append(text((xs[0] + xs[4]) / 2, by - 8, f"단계 3 → 7: {fmtd(rise)}ms, 구별된다", fill=RED, anchor="middle", weight="700", halo=True))
    CHECK.append(("steps-3-to-8", "s3->s7 delta", (fmtd(rise), True)))
    CHECK.append(("steps-3-to-8", "s7->s8 delta", (fmtd(back), True)))
    CHECK.append(("steps-3-to-8", "s3->s8 delta", (fmtd(net), False)))
    o.append(text(86, bot + 24, "숫자: 앞 단계와의 차이. 회색은 구별되지 않고, 빨강은 구별된다", fill=SUB))
    for n, x in zip(nums, xs):
        name = next(s[1] for s in STEPS if s[0] == n)
        # two lines where the glossary name is as wide as the column ("애니메이션 표현" next to "Simulation LOD")
        short = {"애니메이션 표현": "애니메이션\n표현", "Simulation LOD + 엔티티 풀": "Simulation\nLOD",
                 "남은 비용 줄이기": "남은 비용"}.get(name, name)
        o.append(text(x, bot + 56, f"단계 {n}", anchor="middle", weight="700"))
        for k, part in enumerate(short.split("\n")):
            o.append(text(x, bot + 76 + 20 * k, part, anchor="middle", fill=SUB))
    y = bot + 132
    o.append(f'<line x1="32" y1="{y - 20}" x2="{W - 32}" y2="{y - 20}" stroke="{LINE}"/>')
    for line in [f"단계 8은 단계 3과 {fmtd(net)}ms 차이로, 둘은 구별되지 않는다.",
                 "구별되지 않음: 차이가 0.5ms 아래이거나 두 조건의 세 실행 범위보다 작다.",
                 "단계 5는 비용을 줄이려던 단계지만 잰 값은 줄지 않았다."]:
        o.append(text(32, y + 4, line, fill=SUB))
        y += 23
    write("steps-3-to-8.svg", svg(y + 10, o))


# ---------- 3. resources per step: process memory, GPU memory, draw calls ----------
# mem_total_mb   = CSV PhysicalUsedMB, the process working set
# gpu_mem_p50    = CSV GPUMem/LocalUsedMB, local video memory in use by the process
# draw_calls_p50 = CSV RHI/DrawCalls per frame
# (column meanings and engine lines: Scripts/plot-readme.py RESOURCES, Docs/Guides/measurement.md)
RESOURCES = [("프로세스 메모리", "작업 집합, MB", "mem_total_mb", BLUE, mb),
             ("GPU 메모리", "VRAM 사용량, MB", "gpu_mem_p50", PINK, mb),
             ("드로우 콜", "프레임당 개수", "draw_calls_p50", TEAL, count)]
# Rises after step 3 that are larger than both run ranges, named under the figure. resources() checks them.
RISES = {"mem_total_mb": [7], "gpu_mem_p50": [6, 7, 8], "draw_calls_p50": [6, 8]}


def resources():
    span = {}
    for n, *_ in STEPS:
        for _, _, col, _, _ in RESOURCES:
            vs = run_values(cond(n), col)
            span[n, col] = (min(vs), max(vs))
    width = lambda n, col: span[n, col][1] - span[n, col][0]
    for _, _, col, _, _ in RESOURCES:
        found = [n for n, *_ in STEPS if n >= 4
                 and v(cond(n), col) - v(cond(n - 1), col) > max(width(n - 1, col), width(n, col))]
        if found != RISES[col]:
            raise SystemExit(f"{col}: rises at {found}, the figure names {RISES[col]}")
    o = []
    y = title(o, "여덟 단계의 메모리와 드로우 콜",
              ["호드 총수 1만, 이동 장면, 오브 시점 카메라, 세 번 잰 중앙값",
               "막대 둘레의 옅은 띠: 세 번 잰 범위(2px보다 좁은 범위는 그리지 않았다)"])
    LX, CX, CR = 32, 330, W - 32
    CW = (CR - CX) / 3
    BWMAX = 70
    y += 44
    for i, (name, unit, _, colour, _) in enumerate(RESOURCES):
        x = CX + i * CW
        o.append(text(x, y, name, size=17, weight="700", fill=colour))
        o.append(text(x, y + 21, unit, fill=SUB))
    y += 38
    RH, BAR = 36, 18
    for n, name, _, _ in STEPS:
        o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
        o.append(text(LX, y + 24, f"단계 {n} {name}", weight="700"))
        for i, (_, _, col, colour, form) in enumerate(RESOURCES):
            x = CX + i * CW
            sc = BWMAX / v(cond(1), col)
            val = v(cond(n), col)
            lo, hi = span[n, col]
            by = y + 9
            end = val * sc
            if (hi - lo) * sc >= 2.0:
                o.append(f'<rect x="{x + lo * sc:.1f}" y="{by - 4}" width="{(hi - lo) * sc:.1f}" height="{BAR + 8}" '
                         f'rx="4" fill="{colour}" fill-opacity="0.28"/>')
                end = max(val, hi) * sc
            hatch_bar(o, x, by, max(val * sc, 1.5), BAR, colour, n)
            label = form(val)
            o.append(text(x + end + 7, by + 15, label, weight="700", fill=colour))
            CHECK.append(("resources", f"{cond(n)} {col}", label))
        y += RH
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y += 32
    o.append(text(32, y, "단계 4부터 다시 느는 단계(세 번 잰 범위보다 큰 증가)", weight="700"))
    for name, _, col, _, _ in RESOURCES:
        y += 22
        names = ", ".join(f"단계 {n} {next(s[1] for s in STEPS if s[0] == n)}" for n in RISES[col])
        o.append(text(32, y, f"{name}: {names}", fill=SUB))
    y += 30
    o.append(text(32, y, "메모리는 MB 정수로 적었고, 드로우 콜은 개수라 줄이지 않았다.", fill=SUB))
    hatch_key(o, y + 30)
    write("resources.svg", svg(y + 52, o, HATCH))


if __name__ == "__main__":
    ladder()
    steps_3_to_8()
    resources()
    if os.environ.get("LAB_FIG_CHECK"):
        for item in CHECK:
            print(item)
    print("wrote", ", ".join(sorted(os.listdir(OUT))))
