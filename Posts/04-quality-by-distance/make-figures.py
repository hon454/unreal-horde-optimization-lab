# Makes the SVG figures of post 04 in images/.
# Run from the repository root: py -3 -I Posts/04-quality-by-distance/make-figures.py
# Measured values come from Measurements/Full4/summary.csv (the full batch, Test package de0e21b).
# The step-to-step distinguishability is the rule of Scripts/plot.py distinguishable() (Docs/Guides/measurement.md "차이 판정").
# The distance-band diagram draws configuration values from the project source only (no measured values):
#   bands and actor cap   Source/HordeOptLab/LabCrowdSubsystem.cpp BuildEntityConfig, LabConfig.h FLabQuality
#   hysteresis 10 %       Engine/Plugins/Runtime/MassGameplay/Source/MassLOD/Public/MassLODCalculator.h ComputeLODFromSettings
#   colours               Source/HordeOptLab/LabRepTint.cpp LabRepTint::Color (linear RGB, converted to sRGB here)
# The structure screenshot itself (Measurements/Full4/shots/reptint-s8-moving-t75.jpg) is not copied or embedded:
# an SVG shown through <img> on GitHub cannot load another file, so the legend is its own figure above the image.
import csv
import os
import re
from decimal import Decimal, ROUND_HALF_UP

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")

BATCH = "Full4"
TARGET_N = 10000
BUDGET_MS = 16.7             # Scripts/plot.py BUDGET_MS, the 60 fps line
DISTINCT_FLOOR_MS = 0.5      # Docs/Guides/measurement.md "차이 판정", Scripts/plot.py FULL_DISTINCT_FLOOR_MS

rows = {}
with open(os.path.join(ROOT, "Measurements", BATCH, "summary.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        rows[(r["variant"], int(r["n"]), r["preset"])] = r


def row(variant):
    return rows[(variant, TARGET_N, "P1")]


def v(variant, col):
    return float(row(variant)[col])


def dv(a, b, col):
    """Difference b - a from the CSV text with Decimal, so that 0.805 rounds half up to 0.81."""
    return Decimal(row(b)[col]) - Decimal(row(a)[col])


def distinguishable(a, b, key):
    """Scripts/plot.py distinguishable(): the median difference is at least 0.5 ms and larger than the larger run range."""
    d = abs(v(a, key) - v(b, key))
    rng = lambda var: v(var, key + "_max") - v(var, key + "_min")
    return d >= DISTINCT_FLOOR_MS and d > max(rng(a), rng(b))


# ---------- formatting (posting.md: three significant figures, 27 not 27.0) ----------
def _no_zero_decimals(fn):
    return lambda *a, **k: re.sub(r"[.]0+$", "", fn(*a, **k))


@_no_zero_decimals
def fmt(x):
    """Three significant figures, rounded half up from the CSV text (3.865 -> 3.87)."""
    q = "1" if abs(x) >= 100 else "0.1" if abs(x) >= 10 else "0.01"
    return str(Decimal(repr(x)).quantize(Decimal(q), rounding=ROUND_HALF_UP))


def fmtd(x):
    """Signed difference in ms with two decimals, minus sign U+2212."""
    x = x if isinstance(x, Decimal) else Decimal(repr(x))
    s = str(abs(x).quantize(Decimal("0.01"), rounding=ROUND_HALF_UP))
    if s == "0.00":
        return "0"
    s = re.sub(r"[.]0+$", "", s)
    return ("+" if x > 0 else "−") + s


def esc(t):
    return str(t).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


# ---------- style (same as the other posts) ----------
FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, PURPLE, GRAY, AMBER, RED, TEAL, DARK = "#2563eb", "#9333ea", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#374151"
NAVY, LIGHT, SHADE = "#1f2937", "#cbd5e1", "#fef3c7"
W = 800


def svg(h, body, defs=""):
    h = int(round(h))
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">']
    if defs:
        head.append(f"<defs>{defs}</defs>")
    head.append(f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>')
    return "\n".join(head + body + ["</svg>"]) + "\n"


def write(name, text):
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


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


CHECK = []  # (figure, label, text) for the scratch comparison against the CSV (LAB_FIG_CHECK=1 prints it)


# ---------- 1. steps 3 to 7: frame p95 with ranges, thread medians ----------
STEPS = [  # (condition, step line, name line, adds a representation)
    ("V0-s3", "단계 3", "직접 짠 분리", False),
    ("V0-s4", "단계 4", "애니메이션 표현", True),
    ("V0-s5", "단계 5", "Simulation LOD", False),
    ("V0-s6", "단계 6", "ISKM", True),
    ("V0-s7", "단계 7", "배경 무리", True),
]


def steps():
    o = []
    y = title(o, "표현을 더한 단계 4, 6, 7의 프레임 시간과 스레드 시간",
              ["호드 총수 1만, 오브 시점 카메라, 이동 장면, 세 번 잰 중앙값(ms)",
               "프레임 시간은 p95, 스레드 시간은 p50이다. 노란 칸이 표현을 더하는 단계다"])
    xs = [176 + 132 * k for k in range(len(STEPS))]   # column centres, last one 704
    colw = 124
    top = y + 20

    # ----- panel A: frame p95 -----
    A_LO, A_HI, A_PX = 13.5, 17.0, 62.0
    a_top = top + 30
    a_bot = a_top + (A_HI - A_LO) * A_PX
    YA = lambda ms: a_bot - (ms - A_LO) * A_PX
    # panel B geometry is needed for the shading height
    b_head = a_bot + 114
    B_LO, B_HI, B_PX = 0.0, 14.0, 20.0
    b_top = b_head + 44
    b_bot = b_top + (B_HI - B_LO) * B_PX
    YB = lambda ms: b_bot - (ms - B_LO) * B_PX
    for (cond, s1, s2, adds), x in zip(STEPS, xs):
        if adds:
            o.append(f'<rect x="{x - colw / 2:.1f}" y="{a_top - 10:.1f}" width="{colw}" height="{b_bot - a_top + 20:.1f}" fill="{SHADE}"/>')
    o.append(text(32, top + 6, "프레임 p95(점은 중앙값, 세로선은 세 실행의 범위)", weight="700"))
    for t in (14, 15, 16, 17):
        o.append(f'<line x1="96" y1="{YA(t):.1f}" x2="{W - 32}" y2="{YA(t):.1f}" stroke="#eef0f3"/>')
        o.append(text(86, YA(t) + 5, str(t), fill=SUB, anchor="end"))
    o.append(f'<line x1="96" y1="{YA(BUDGET_MS):.1f}" x2="{W - 32}" y2="{YA(BUDGET_MS):.1f}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="6 4"/>')
    o.append(text(W - 36, YA(BUDGET_MS) - 7, "16.7ms(60fps)", fill=AMBER, anchor="end", weight="700", halo=True))
    pts = []
    for (cond, s1, s2, adds), x in zip(STEPS, xs):
        p95, lo, hi = v(cond, "frame_p95_ms"), v(cond, "frame_p95_ms_min"), v(cond, "frame_p95_ms_max")
        pts.append((x, YA(p95)))
        o.append(f'<line x1="{x}" y1="{YA(lo):.1f}" x2="{x}" y2="{YA(hi):.1f}" stroke="{INK}" stroke-width="2"/>')
        for b in (lo, hi):
            o.append(f'<line x1="{x - 6}" y1="{YA(b):.1f}" x2="{x + 6}" y2="{YA(b):.1f}" stroke="{INK}" stroke-width="2"/>')
        CHECK.append(("steps", f"{cond} frame_p95_ms", (fmt(p95), fmt(lo), fmt(hi))))
    d = " ".join(f'{"M" if i == 0 else "L"}{px:.1f},{py:.1f}' for i, (px, py) in enumerate(pts))
    o.append(f'<path d="{d}" fill="none" stroke="{NAVY}" stroke-width="2.5"/>')
    for (cond, s1, s2, adds), (px, py) in zip(STEPS, pts):
        o.append(f'<circle cx="{px:.1f}" cy="{py:.1f}" r="5.5" fill="{NAVY}"/>')
        o.append(text(px + 12, py + 22, fmt(v(cond, "frame_p95_ms")), weight="700", halo=True))
    # step-to-step differences under the points (all not distinguishable in this batch)
    for k in range(1, len(STEPS)):
        a, b = STEPS[k - 1][0], STEPS[k][0]
        delta = dv(a, b, "frame_p95_ms")
        dist = distinguishable(a, b, "frame_p95_ms")
        if dist:
            raise SystemExit(f"{a} -> {b} is distinguishable; the post says it is not")
        mx = (xs[k - 1] + xs[k]) / 2
        o.append(text(mx, YA(13.62), fmtd(delta), fill=GRAY, anchor="middle", halo=True))
        CHECK.append(("steps", f"{a}->{b} delta frame_p95_ms", (fmtd(delta), dist)))
    # cumulative bracket step 3 -> step 7
    first, last = STEPS[0][0], STEPS[-1][0]
    cum = dv(first, last, "frame_p95_ms")
    cum_dist = distinguishable(first, last, "frame_p95_ms")
    if not cum_dist:
        raise SystemExit("step 3 -> 7 is not distinguishable; the post says it is")
    by = YA(16.2)
    o.append(f'<path d="M{xs[0]},{by + 8:.1f} L{xs[0]},{by:.1f} L{xs[-1]},{by:.1f} L{xs[-1]},{by + 8:.1f}" fill="none" stroke="{RED}" stroke-width="2"/>')
    o.append(text((xs[0] + xs[-1]) / 2, by - 8, f"단계 3 → 7: {fmtd(cum)}ms, 구별된다", fill=RED, anchor="middle", weight="700", halo=True))
    CHECK.append(("steps", "s3->s7 delta frame_p95_ms", (fmtd(cum), cum_dist)))
    o.append(text(96, a_bot + 22, "회색 숫자: 앞 단계와의 차이. 모두 0.5ms 아래라 구별되지 않는다", fill=SUB))
    # step labels between the panels
    for (cond, s1, s2, adds), x in zip(STEPS, xs):
        o.append(text(x, a_bot + 52, s1, anchor="middle", weight="700"))
        o.append(text(x, a_bot + 72, s2, anchor="middle", fill=SUB))

    # ----- panel B: thread medians -----
    o.append(text(32, b_head + 4, "스레드 시간(중앙값)", weight="700"))
    lx = 210
    for colour, label in ((NAVY, "게임 스레드"), (TEAL, "렌더 스레드"), (PURPLE, "GPU")):
        o.append(f'<line x1="{lx}" y1="{b_head - 1}" x2="{lx + 22}" y2="{b_head - 1}" stroke="{colour}" stroke-width="3"/>')
        o.append(text(lx + 30, b_head + 4, label, fill=SUB))
        lx += 52 + 16 * len(label)
    for t in (0, 4, 8, 12):
        o.append(f'<line x1="96" y1="{YB(t):.1f}" x2="{W - 32}" y2="{YB(t):.1f}" stroke="#eef0f3"/>')
        o.append(text(86, YB(t) + 5, str(t), fill=SUB, anchor="end"))
    series = [("game_p50_ms", NAVY, -12), ("gpu_p50_ms", PURPLE, -12), ("render_p50_ms", TEAL, 24)]
    for col, colour, off in series:
        ps = [(x, YB(v(cond, col))) for (cond, *_), x in zip(STEPS, xs)]
        d = " ".join(f'{"M" if i == 0 else "L"}{px:.1f},{py:.1f}' for i, (px, py) in enumerate(ps))
        o.append(f'<path d="{d}" fill="none" stroke="{colour}" stroke-width="2.5"/>')
        for (cond, *_), (px, py) in zip(STEPS, ps):
            o.append(f'<circle cx="{px:.1f}" cy="{py:.1f}" r="4" fill="{colour}"/>')
            o.append(text(px, py + off, fmt(v(cond, col)), fill=colour, anchor="middle", weight="700", halo=True))
            CHECK.append(("steps", f"{cond} {col}", fmt(v(cond, col))))
    # the two GPU moves the post is about
    for a, b, who in (("V0-s5", "V0-s6", "ISKM"), ("V0-s6", "V0-s7", "배경 무리")):
        delta = dv(a, b, "gpu_p50_ms")
        if not distinguishable(a, b, "gpu_p50_ms"):
            raise SystemExit(f"GPU {a} -> {b} is not distinguishable")
        ia = [s[0] for s in STEPS].index(a)
        mx = (xs[ia] + xs[ia + 1]) / 2
        my = (YB(v(a, "gpu_p50_ms")) + YB(v(b, "gpu_p50_ms"))) / 2
        o.append(text(mx, my - 30, f"{who}", fill=PURPLE, anchor="middle", halo=True))
        o.append(text(mx, my - 12, f"GPU {fmtd(delta)}", fill=PURPLE, anchor="middle", weight="700", halo=True))
        CHECK.append(("steps", f"{a}->{b} delta gpu_p50_ms", fmtd(delta)))
    y = b_bot + 48
    o.append(f'<line x1="32" y1="{y - 22}" x2="{W - 32}" y2="{y - 22}" stroke="{LINE}"/>')
    bott = {row(c)["bottleneck"] for c, *_ in STEPS}
    if bott != {"game"}:
        raise SystemExit(f"bottleneck is not the game thread in every step: {bott}")
    for line in ["프레임 시간은 게임 스레드, 렌더 스레드, GPU 가운데 가장 느린 쪽이 정한다.",
                 "다섯 단계 모두 게임 스레드가 가장 느리다. GPU의 움직임은 프레임에 거의 보이지 않는다.",
                 "단계 5는 비용을 줄이는 단계다(Simulation LOD와 엔티티 풀, 글 05)."]:
        o.append(text(32, y, line, fill=SUB))
        y += 22
    write("steps-3-to-7.svg", svg(y + 8, o))


# ---------- 2. distance bands of the final configuration (configuration values only) ----------
def srgb(c):
    """Linear RGB (LabRepTint::Color) to an sRGB hex colour."""
    def one(x):
        x = 12.92 * x if x <= 0.0031308 else 1.055 * x ** (1 / 2.4) - 0.055
        return max(0, min(255, round(x * 255)))
    return "#" + "".join(f"{one(x):02x}" for x in c)


TINT = {  # Source/HordeOptLab/LabRepTint.cpp LabRepTint::Color
    "actor": (0.9, 0.1, 0.1),
    "iskm_shadow": (0.1, 0.3, 0.9),
    "iskm_noshadow": (0.2, 0.8, 0.9),
    "crowd": (0.95, 0.8, 0.1),
}
HYDRATE_M = 35        # LabConfig.h FLabQuality::DefaultHydrateM
SHADOW_M = 80         # LabCrowdSubsystem.cpp BuildEntityConfig Bands[Low] = 8000 cm
FAR_M = 200           # LabConfig.h FarM default 200 (RepOffCm)
MAP_M = 500           # LabConfig.h FLabScenario::MapRadius (map centre)
OUTER_M = 1000        # LabConfig.h FLabCrowdFx::OuterRadius (map centre)
ACTOR_CAP = 1000      # LabConfig.h FLabQuality::DefaultActorCap
HYST = 0.1            # MassLODCalculator.h BufferHysteresisOnDistanceRatio (10 %)
HOLD_S = 2            # LabConfig.h FLabQuality::DefaultRepHoldMs / 1000


def lerp_m(a, b):
    """Entering a closer band happens at Lerp(D[i], D[i+1], 1 - 10 %) (MassLODCalculator.h ComputeLODFromSettings)."""
    return a + (b - a) * (1 - HYST)


def bands():
    o = []
    y = title(o, "카메라 거리마다 개체를 그리는 표현(최종 구성)",
              ["색은 아래 화면에 입힌 표현별 색이다. 눈금 간격은 거리에 비례하지 않는다"])
    x0, widths = 32, [120, 130, 150, 170, 166]   # 0-35, 35-80, 80-200, 200-map edge, map edge-1,000 (sum 736)
    edges = [x0]
    for w_ in widths:
        edges.append(edges[-1] + w_)
    cols = [srgb(TINT["actor"]), srgb(TINT["iskm_shadow"]), srgb(TINT["iskm_noshadow"]), srgb(TINT["crowd"]), srgb(TINT["crowd"])]
    labels = ["액터", "ISKM", "ISKM", "입자", "입자"]
    defs = (f'<pattern id="hatch" width="10" height="10" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
            f'<rect width="10" height="10" fill="{cols[4]}"/><line x1="0" y1="0" x2="0" y2="10" stroke="{BG}" stroke-width="5"/></pattern>')
    # hysteresis notes above the bar
    yb = y + 64
    o.append(text(32, y + 14, f"가까워지는 쪽으로 경계를 넘을 때의 여유(엔진 기본 {round(HYST * 100)}%)", fill=SUB))
    h80, h200 = lerp_m(HYDRATE_M, SHADOW_M), lerp_m(SHADOW_M, FAR_M)
    notes = [(edges[1], "여유 없음"), (edges[2], f"{h80:g}m에서"), (edges[3], f"{h200:g}m에서")]
    for x, t in notes:
        o.append(f'<line x1="{x}" y1="{yb - 14}" x2="{x}" y2="{yb}" stroke="{SUB}" stroke-width="1.5"/>')
        o.append(text(x, yb - 20, t, fill=SUB, anchor="middle", weight="700"))
        CHECK.append(("bands", "hysteresis note", t))
    for k in range(5):
        fill = "url(#hatch)" if k == 4 else cols[k]
        o.append(f'<rect x="{edges[k]}" y="{yb}" width="{widths[k]}" height="40" fill="{fill}" stroke="{BG}" stroke-width="2"/>')
        o.append(text((edges[k] + edges[k + 1]) / 2, yb + 26, labels[k], anchor="middle", weight="700", halo=(k == 4)))
    ticks = ["0", f"{HYDRATE_M}m", f"{SHADOW_M}m", f"{FAR_M}m", "맵 끝", f"{OUTER_M:,}m"]
    for x, t in zip(edges, ticks):
        o.append(f'<line x1="{x}" y1="{yb + 40}" x2="{x}" y2="{yb + 48}" stroke="{SUB}"/>')
        anchor = "start" if x == edges[0] else "end" if x == edges[-1] else "middle"
        o.append(text(x, yb + 68, t, fill=SUB, anchor=anchor))
    o.append(text(32, yb + 92, f"0~{FAR_M}m는 카메라 거리, 맵 끝(맵 중심 {MAP_M}m)부터는 맵 중심 거리다", fill=SUB))
    # rows: what each band is and which step added it
    rows_ = [
        (cols[0], f"0~{HYDRATE_M}m", f"하이드레이션 액터. 상한 {ACTOR_CAP:,}", f"단계 2부터. Animation Budget Allocator와 최소 유지 시간 {HOLD_S}초는 단계 4부터"),
        (cols[1], f"{HYDRATE_M}~{SHADOW_M}m", "ISKM, 그림자를 그린다", "단계 6부터. 단계 4·5는 VAT 인스턴스, 단계 2·3은 정지 메시"),
        (cols[2], f"{SHADOW_M}~{FAR_M}m", "ISKM, 그림자를 끈다", "단계 6부터 ISKM. 그림자를 끈 것은 단계 8(글 05)"),
        (cols[3], f"{FAR_M}m 밖, 맵 안", "입자가 그려지지 않는 엔티티의 자리를 걷는다", "단계 7부터. 단계 2~6은 그리지 않는다"),
        ("url(#hatch)", f"맵 밖", f"입자가 맵 밖 무리의 기록을 그린다", f"단계 7부터. 맵 중심 {MAP_M}~{OUTER_M:,}m"),
    ]
    y = yb + 132
    for colour, rng, what, step in rows_:
        o.append(f'<rect x="32" y="{y - 15}" width="22" height="18" rx="3" fill="{colour}" stroke="{LINE}"/>')
        o.append(text(66, y, rng, weight="700"))
        o.append(text(220, y, what))
        o.append(text(220, y + 22, step, fill=SUB))
        CHECK.append(("bands", rng, what + " / " + step))
        y += 56
    o.append(f'<line x1="32" y1="{y - 18}" x2="{W - 32}" y2="{y - 18}" stroke="{LINE}"/>')
    y += 8
    for line in [f"액터 경계({HYDRATE_M}m)에는 엔진의 여유가 없다. 그래서 바뀐 개체를 {HOLD_S}초 동안 붙잡는다.",
                 f"엔진이 그리는 것은 {FAR_M}m 안의 엔티티다. 그 밖은 입자가 그린다."]:
        o.append(text(32, y, line, fill=SUB))
        y += 22
    write("distance-bands.svg", svg(y + 8, o, defs))


if __name__ == "__main__":
    steps()
    bands()
    if os.environ.get("LAB_FIG_CHECK"):
        for item in CHECK:
            print(item)
    print("wrote", ", ".join(sorted(os.listdir(OUT))))
