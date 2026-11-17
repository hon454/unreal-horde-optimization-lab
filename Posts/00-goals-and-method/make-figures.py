# Makes the SVG figures of post 00 in images/.
# Run from the repository root: py -3 -I Posts/00-goals-and-method/make-figures.py
# Measured values come from Measurements/Full4/summary.csv and Docs/figures/full4/full-n-at-60fps.csv
# (Test package de0e21b). The scenario, the pipeline and the stage map draw configuration values only;
# their sources are in measurements.md.
import csv
import math
import os
import random
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")
os.makedirs(OUT, exist_ok=True)

rows = {}
with open(os.path.join(ROOT, "Measurements", "Full4", "summary.csv"), encoding="utf-8") as f:
    for r in csv.DictReader(f):
        rows[(r["variant"], int(r["n"]), r["preset"])] = r


def v(variant, col, n=10000, preset="P1"):
    return float(rows[(variant, n, preset)][col])


with open(os.path.join(ROOT, "Docs", "figures", "full4", "full-n-at-60fps.csv"), encoding="utf-8") as f:
    NAT = {r["series"]: r for r in csv.DictReader(f)}


def n_at_60(step):
    """Max N of a ladder step, read from the batch table (Scripts/plot.py n_at_budget)."""
    for name, r in NAT.items():
        if name.startswith(f"{step} "):
            return int(r["n_p95"])
    raise KeyError(step)


BUDGET_MS = 16.7      # Scripts/plot.py BUDGET_MS, the 60 fps line
VERDICT_MARGIN = 0.6  # ADR-0019 decision 3: step 8 frame p95 median + 0.6 ms <= 16.7 ms
SAME_MS = 0.5         # measurement.md "차이 판정": differences under 0.5 ms are not told apart in this batch

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, LIGHTBLUE, GRAY, AMBER, RED, TEAL, PINK, DARK, MID = (
    "#2563eb", "#93c5fd", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151", "#6b7280")
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
    """27.0 -> 27. A value whose decimals are all zero is written as an integer (posting.md)."""
    return lambda *a, **k: re.sub(r"[.]0+$", "", f(*a, **k))


@_no_zero_decimals
def fmt(x):
    """Three significant figures, as in the post body."""
    if abs(x) >= 100:
        return f"{x:.0f}"
    if abs(x) >= 10:
        return f"{x:.1f}"
    return f"{x:.2f}"


def signed(x):
    """A difference with its sign and two decimals: +0.08, −1.86."""
    return ("+" if x >= 0 else "−") + f"{abs(x):.2f}"


def title(o, t, s, s2=None):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')
    if s2:
        o.append(f'<text x="32" y="98" font-size="16" fill="{SUB}">{s2}</text>')


HATCH = (f'<pattern id="hatch" width="8" height="8" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
         f'<rect width="8" height="8" fill="#f3f4f6"/><line x1="0" y1="0" x2="0" y2="8" stroke="{GRAY}" stroke-width="3"/></pattern>'
         f'<pattern id="shade" width="7" height="7" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
         f'<line x1="0" y1="0" x2="0" y2="7" stroke="#ffffff" stroke-opacity="0.6" stroke-width="2.5"/></pattern>'
         f'<marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{DARK}"/></marker>'
         f'<marker id="arrowb" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{BLUE}"/></marker>')


# ---------- 1. how one condition is read: the final configuration ----------
def final_reading():
    o = []
    title(o, "최종 구성 한 조건을 읽는 법",
          "호드 총수 1만, 이동 장면, 오브 시점 카메라. 세 번 잰 중앙값(ms)")
    s = "V0-s8"
    p95, p50 = v(s, "frame_p95_ms"), v(s, "frame_p50_ms")
    margin = BUDGET_MS - p95
    items = [
        ("프레임 p95", "frame_p95_ms", DARK, True),
        ("프레임 p50", "frame_p50_ms", MID, True),
        ("게임 스레드", "game_p50_ms", BLUE, False),
        ("렌더 스레드", "render_p50_ms", TEAL, False),
        ("GPU", "gpu_p50_ms", PINK, False),
    ]
    X0, X1, XMAX = 200, 740, 18.0
    X = lambda ms: X0 + ms / XMAX * (X1 - X0)
    top, RH, BH = 160, 56, 28
    bottom = top + RH * len(items) - (RH - BH)
    for t in [0, 5, 10, 15]:
        o.append(f'<line x1="{X(t):.1f}" y1="{top - 12}" x2="{X(t):.1f}" y2="{bottom + 8}" stroke="{LINE}"/>')
        o.append(f'<text x="{X(t):.1f}" y="{bottom + 30}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')
    xb = X(BUDGET_MS)
    o.append(f'<line x1="{xb:.1f}" y1="{top - 34}" x2="{xb:.1f}" y2="{bottom + 8}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="6 4"/>')
    o.append(f'<text x="{xb:.1f}" y="{top - 42}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    # the bottleneck is the longest of the three thread medians (summary.csv "bottleneck")
    bottleneck = max(("game_p50_ms", "render_p50_ms", "gpu_p50_ms"), key=lambda c: v(s, c))
    y = top
    for label, col, colour, ranged in items:
        val = v(s, col)
        o.append(f'<text x="32" y="{y + 20}" font-size="17" font-weight="700">{label}</text>')
        o.append(f'<rect x="{X0}" y="{y}" width="{X(val) - X0:.1f}" height="{BH}" rx="4" fill="{colour}"/>')
        end = X(val)
        if ranged:
            mn, mx = v(s, col + "_min"), v(s, col + "_max")
            cy = y + BH / 2
            o.append(f'<line x1="{X(mn):.1f}" y1="{cy}" x2="{X(mx):.1f}" y2="{cy}" stroke="{INK}" stroke-width="2"/>')
            for e in (mn, mx):
                o.append(f'<line x1="{X(e):.1f}" y1="{cy - 8}" x2="{X(e):.1f}" y2="{cy + 8}" stroke="{INK}" stroke-width="2"/>')
            end = X(mx)
        text = fmt(val)
        if col == bottleneck:
            text += '<tspan font-weight="700" fill="' + BLUE + '">  병목</tspan>'
        o.append(f'<text x="{end + 8:.1f}" y="{y + 20}" font-size="16" font-weight="700" fill="{colour}">{text}</text>')
        if col == "frame_p95_ms":
            # margin bracket from frame p95 to the 16.7 ms line, above the bar
            a, b, by = X(p95), xb, y - 8
            o.append(f'<path d="M{a:.1f} {by + 6} L{a:.1f} {by} L{b:.1f} {by} L{b:.1f} {by + 6}" fill="none" stroke="{AMBER}" stroke-width="1.5"/>')
            o.append(f'<text x="{(a + b) / 2:.1f}" y="{by - 6}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">여유 {fmt(margin)}</text>')
        y += RH
    yf = bottom + 64
    o.append(f'<line x1="32" y1="{yf - 22}" x2="{W - 32}" y2="{yf - 22}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{yf}" font-size="16" fill="{SUB}">가는 선은 세 실행의 최솟값–최댓값이다. 스레드 시간은 중앙값만 쓴다.</text>')
    o.append(f'<text x="32" y="{yf + 26}" font-size="16" fill="{SUB}">프레임 시간은 가장 느린 스레드(병목)보다 조금 길다.</text>')
    o.append(f'<text x="32" y="{yf + 52}" font-size="16" fill="{SUB}">목표 판정: 프레임 p95 {fmt(p95)} + {VERDICT_MARGIN:g} = '
             f'{fmt(p95 + VERDICT_MARGIN)}ms ≤ 16.7ms</text>')
    write("final-reading.svg", svg(yf + 76, o))


# ---------- 2. scenario: the orb circles the map centre, the horde chases it (concept, not to scale) ----------
def scenario():
    o = []
    title(o, "시나리오: 원을 도는 오브를 사방에서 쫓는다",
          "위에서 본 개념도. 축척은 맞지 않다")
    CX, CY, RM, RO, RB = 255, 360, 190, 245, 58
    o.append(f'<circle cx="{CX}" cy="{CY}" r="{RO}" fill="none" stroke="{GRAY}" stroke-width="1.5" stroke-dasharray="5 5"/>')
    o.append(f'<circle cx="{CX}" cy="{CY}" r="{RM}" fill="{CARD}" stroke="{GRAY}" stroke-width="2"/>')
    rnd = random.Random(20261106)
    # far stream records (500-1,000 m): hollow dots between the map edge and the outer ring
    for _ in range(46):
        r = rnd.uniform(RM + 9, RO - 7)
        a = rnd.uniform(0, 2 * math.pi)
        o.append(f'<circle cx="{CX + r * math.cos(a):.1f}" cy="{CY + r * math.sin(a):.1f}" r="2.6" fill="white" stroke="{MID}" stroke-width="1.2"/>')
    # minions: radius drawn uniformly so the density goes as 1/r (measurement.md "시나리오"); every tenth is large
    for k in range(300):
        r = rnd.uniform(12, RM - 6)
        a = rnd.uniform(0, 2 * math.pi)
        big = k % 10 == 0
        o.append(f'<circle cx="{CX + r * math.cos(a):.1f}" cy="{CY + r * math.sin(a):.1f}" r="{3.6 if big else 2.2}" '
                 f'fill="{DARK if big else GRAY}"/>')
    # orbit and orb
    o.append(f'<circle cx="{CX}" cy="{CY}" r="{RB}" fill="none" stroke="{BLUE}" stroke-width="2" stroke-dasharray="6 4"/>')
    ang = math.radians(-35)
    ox, oy = CX + RB * math.cos(ang), CY + RB * math.sin(ang)
    # tangent arrow along the orbit (counter-clockwise on screen)
    a0, a1 = ang - math.radians(4), ang - math.radians(48)
    o.append(f'<path d="M{CX + (RB + 14) * math.cos(a0):.1f} {CY + (RB + 14) * math.sin(a0):.1f} '
             f'A{RB + 14} {RB + 14} 0 0 0 {CX + (RB + 14) * math.cos(a1):.1f} {CY + (RB + 14) * math.sin(a1):.1f}" '
             f'fill="none" stroke="{BLUE}" stroke-width="2.5" marker-end="url(#arrowb)"/>')
    # camera behind the orb, looking past it
    cam_dir = math.radians(150)
    cx_, cy_ = ox + 30 * math.cos(cam_dir), oy + 30 * math.sin(cam_dir)
    look = cam_dir + math.pi
    for da in (-0.45, 0.45):
        o.append(f'<line x1="{cx_:.1f}" y1="{cy_:.1f}" x2="{cx_ + 120 * math.cos(look + da):.1f}" y2="{cy_ + 120 * math.sin(look + da):.1f}" '
                 f'stroke="{AMBER}" stroke-width="1.5" stroke-dasharray="4 3"/>')
    o.append(f'<rect x="{cx_ - 8:.1f}" y="{cy_ - 6:.1f}" width="16" height="12" rx="2" fill="{AMBER}"/>')
    o.append(f'<circle cx="{ox:.1f}" cy="{oy:.1f}" r="15" fill="none" stroke="{RED}" stroke-width="2" stroke-dasharray="3 3"/>')
    o.append(f'<circle cx="{ox:.1f}" cy="{oy:.1f}" r="7" fill="{BLUE}"/>')
    # a few minions walking toward the orb
    for a, r in [(200, 150), (95, 165), (330, 175), (250, 120)]:
        sx, sy = CX + r * math.cos(math.radians(a)), CY + r * math.sin(math.radians(a))
        dx, dy = ox - sx, oy - sy
        d = math.hypot(dx, dy)
        ex, ey = sx + dx / d * 46, sy + dy / d * 46
        o.append(f'<circle cx="{sx:.1f}" cy="{sy:.1f}" r="3.6" fill="{DARK}"/>')
        o.append(f'<line x1="{sx:.1f}" y1="{sy:.1f}" x2="{ex:.1f}" y2="{ey:.1f}" stroke="{DARK}" stroke-width="1.6" marker-end="url(#arrow)"/>')
    # respawn: the nearest far record becomes an entity again
    ra = math.radians(-30)
    rx, ry = CX + (RM + 22) * math.cos(ra), CY + (RM + 22) * math.sin(ra)
    o.append(f'<circle cx="{rx:.1f}" cy="{ry:.1f}" r="4" fill="white" stroke="{RED}" stroke-width="2"/>')
    o.append(f'<path d="M{rx - 4:.1f} {ry + 6:.1f} Q{rx - 30:.1f} {ry + 20:.1f} {rx - 40:.1f} {ry + 4:.1f}" fill="none" stroke="{RED}" stroke-width="1.8" marker-end="url(#arrow)"/>')
    # labels on the drawing
    halo = 'paint-order="stroke" stroke="white" stroke-width="6" stroke-linejoin="round"'
    o.append(f'<text x="{CX}" y="{CY + RM - 14}" text-anchor="middle" font-size="16" font-weight="700" fill="{SUB}" {halo}>맵 반경 500m</text>')
    o.append(f'<text x="{CX}" y="{CY + RO + 24}" text-anchor="middle" font-size="16" fill="{SUB}">맵 밖 무리 500–1,000m</text>')
    o.append(f'<text x="{CX - 22}" y="{CY + RB + 22}" text-anchor="middle" font-size="16" font-weight="700" fill="{BLUE}" {halo}>오브 궤도</text>')
    # legend on the right, one fact per line
    LX, y = 530, 128
    def head(t, colour):
        nonlocal y
        o.append(f'<text x="{LX}" y="{y}" font-size="17" font-weight="700" fill="{colour}">{t}</text>')
        y += 25
    def line(t):
        nonlocal y
        o.append(f'<text x="{LX}" y="{y}" font-size="16">{t}</text>')
        y += 23
    o.append(f'<circle cx="{LX + 7}" cy="{y - 6}" r="7" fill="{BLUE}"/>')
    LX += 20; head("오브", BLUE); LX -= 20
    line("반지름 57.3m 원을 돈다")
    line("2m/s, 한 바퀴 180초")
    line("8m 안 범위 피해")
    line("2초마다 화면 안 5명 저격")
    y += 10
    o.append(f'<circle cx="{LX + 5}" cy="{y - 6}" r="2.4" fill="{GRAY}"/><circle cx="{LX + 14}" cy="{y - 6}" r="3.8" fill="{DARK}"/>')
    LX += 26; head("개체 1만", DARK); LX -= 26
    line("소형 90%, 대형 10%")
    line("3m/s로 오브를 쫓는다")
    y += 10
    o.append(f'<rect x="{LX}" y="{y - 13}" width="16" height="12" rx="2" fill="{AMBER}"/>')
    LX += 24; head("카메라", AMBER); LX -= 24
    line("오브 뒤 15m, 높이 9m")
    line("오브를 따라가며")
    line("1분에 한 바퀴 돈다")
    y += 10
    o.append(f'<circle cx="{LX + 6}" cy="{y - 6}" r="4" fill="white" stroke="{RED}" stroke-width="2"/>')
    LX += 20; head("재생성", RED); LX -= 20
    line("죽은 개체는 맵 밖 무리에서")
    line("오브에 가장 가까운")
    line("기록의 자리로 돌아온다")
    yf = CY + RO + 62
    o.append(f'<line x1="32" y1="{yf - 22}" x2="{W - 32}" y2="{yf - 22}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{yf}" font-size="16" fill="{SUB}">처음 배치는 맵 중심에서 반지름마다 개체 수가 같다. 그래서 가운데가 빽빽하다.</text>')
    write("scenario.svg", svg(yf + 26, o, HATCH))


# ---------- 3. pipeline: the slowest thread sets the pace (concept, no measured values) ----------
def pipeline():
    o = []
    title(o, "세 스레드는 서로 다른 프레임을 겹쳐 처리한다",
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
            o.append(f'<rect x="{X0 + a * u:.1f}" y="{y}" width="{(b - a) * u - 2:.1f}" height="{RH}" rx="5" fill="{col}"/>')
            o.append(f'<text x="{X0 + (a + b) / 2 * u:.1f}" y="{y + 29}" text-anchor="middle" font-size="17" font-weight="700" fill="white">{k + 1}</text>')
        y += RH + GAP
    by = y + 4
    a, b = X0 + 14 * u, X0 + 21 * u
    o.append(f'<path d="M{a:.1f} {by} L{a:.1f} {by + 10} L{b:.1f} {by + 10} L{b:.1f} {by}" fill="none" stroke="{INK}" stroke-width="1.5"/>')
    o.append(f'<text x="{(a + b) / 2:.1f}" y="{by + 34}" text-anchor="middle" font-size="16" font-weight="700">프레임 시간</text>')
    o.append(f'<text x="{(a + b) / 2:.1f}" y="{by + 56}" text-anchor="middle" font-size="16" fill="{SUB}">= 병목인 GPU의 한 칸</text>')
    ly = by + 92
    o.append(f'<rect x="{LX}" y="{ly - 15}" width="34" height="20" rx="3" fill="url(#hatch)" stroke="{GRAY}" stroke-width="0.8"/>')
    o.append(f'<text x="{LX + 44}" y="{ly}" font-size="16">기다림: 앞서 나갈 수 있는 프레임 수에 한도가 있다</text>')
    o.append(f'<text x="{LX}" y="{ly + 26}" font-size="16">칸의 숫자는 프레임 번호다. 칸의 길이는 지어낸 값이다.</text>')
    o.append(f'<line x1="32" y1="{ly + 46}" x2="{W - 32}" y2="{ly + 46}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{ly + 74}" font-size="16" fill="{SUB}">게임 스레드가 병목이면 게임 스레드의 한 칸이 프레임 시간이 된다.</text>')
    write("frame-pipeline.svg", svg(ly + 96, o, HATCH))


# ---------- 4. judging a difference: median, three-run range and the 0.5 ms floor ----------
def judging():
    o = []
    title(o, "차이 판정: 범위보다 크고 0.5ms 이상이어야 차이다",
          "호드 총수 1만, 이동 장면, 오브 시점 카메라. 프레임 p95(ms)",
          "점은 세 번 잰 값의 중앙값, 가로선은 세 번의 최솟값–최댓값")
    pairs = [
        ("같은 조건 다시 재기", ("V0-s8", "최종 구성"), ("V0-s8-aa", "측정 끝에 다시 잰 최종 구성")),
        ("단계 6에서 단계 7로", ("V0-s6", "단계 6 ISKM"), ("V0-s7", "단계 7 배경 무리")),
        ("최종 구성에서 액터 캡슐만 껐을 때", ("V0-s8", "최종 구성"), ("V0-a8cap", "액터 캡슐을 끈 조건")),
    ]
    lo, hi = 12.0, 16.0
    X0, X1 = 330, 700
    X = lambda ms: X0 + (ms - lo) / (hi - lo) * (X1 - X0)
    TICKS = [12, 13, 14, 15, 16]
    y = 128
    for t in TICKS:
        o.append(f'<text x="{X(t):.1f}" y="{y}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')
    y += 14
    top = y
    head_part, o = o, []  # marks go to a new list so that the grid can be drawn under them
    for head, a, b in pairs:
        y += 28
        o.append(f'<text x="32" y="{y}" font-size="17" font-weight="700">{head}</text>')
        y += 14
        stats = []
        for var, label in (a, b):
            med = v(var, "frame_p95_ms")
            mn, mx = v(var, "frame_p95_ms_min"), v(var, "frame_p95_ms_max")
            stats.append((med, mx - mn))
            y += 30
            o.append(f'<text x="48" y="{y + 6}" font-size="16">{label}</text>')
            o.append(f'<line x1="{X(mn):.1f}" y1="{y}" x2="{X(mx):.1f}" y2="{y}" stroke="{DARK}" stroke-width="3"/>')
            for e in (mn, mx):
                o.append(f'<line x1="{X(e):.1f}" y1="{y - 8}" x2="{X(e):.1f}" y2="{y + 8}" stroke="{DARK}" stroke-width="2"/>')
            o.append(f'<circle cx="{X(med):.1f}" cy="{y}" r="6" fill="{DARK}"/>')
            o.append(f'<text x="{X(mx) + 14:.1f}" y="{y + 6}" font-size="16" font-weight="700">{fmt(med)}</text>')
        d = stats[1][0] - stats[0][0]
        rng = max(stats[0][1], stats[1][1])
        # measurement.md "차이 판정": larger than the larger range, and at least 0.5 ms in this batch
        beyond = abs(d) > rng
        distinct = beyond and abs(d) >= SAME_MS
        col = RED if distinct else BLUE
        verdict = "차이 있음" if distinct else "구별되지 않음"
        if distinct:
            why = f"차이 {signed(d)}, 큰 쪽 범위 {abs(rng):.2f}보다 크고 0.5ms 이상"
        elif beyond:
            why = f"차이 {signed(d)}, 큰 쪽 범위 {abs(rng):.2f}보다 크지만 0.5ms 아래"
        else:
            why = f"차이 {signed(d)}, 큰 쪽 범위 {abs(rng):.2f}보다 작다"
        y += 32
        o.append(f'<text x="48" y="{y}" font-size="16" fill="{SUB}">{why}</text>')
        o.append(f'<text x="{W - 32}" y="{y}" text-anchor="end" font-size="16" font-weight="700" fill="{col}">→ {verdict}</text>')
        y += 14
    grid = [f'<line x1="{X(t):.1f}" y1="{top}" x2="{X(t):.1f}" y2="{y}" stroke="{LINE}"/>' for t in TICKS]
    o = head_part + grid + o
    yf = y + 34
    o.append(f'<line x1="32" y1="{yf - 22}" x2="{W - 32}" y2="{yf - 22}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{yf}" font-size="16" fill="{SUB}">범위는 두 조건 가운데 큰 쪽을 쓴다. 조건마다 세 번 재서</text>')
    o.append(f'<text x="32" y="{yf + 24}" font-size="16" fill="{SUB}">0.5ms 아래의 차이는 범위를 넘어도 구별되지 않는다고 적는다.</text>')
    write("judging-difference.svg", svg(yf + 48, o))


# ---------- 5. max N: how the number is read, steps 1 and 2 only ----------
def max_entities():
    o = []
    title(o, "최대 N을 읽는 법: 단계 1과 2",
          "이동 장면, 오브 시점 카메라, 세 번 잰 중앙값. 가로축은 로그 눈금")
    series = [
        ("V0-s1", [500, 1000, 2000], RED, "단계 1 액터 기준선", 1),
        ("V0-s2", [2500, 5000, 10000], BLUE, "단계 2 Mass, 엔진 기본 경로", 2),
    ]
    L, R, T, B = 90, 560, 130, 470
    NLO, NHI, YMAX = 400, 12000, 45.0
    X = lambda n: L + (math.log(n) - math.log(NLO)) / (math.log(NHI) - math.log(NLO)) * (R - L)
    Y = lambda ms: B - ms / YMAX * (B - T)
    for t in [0, 10, 20, 30, 40]:
        o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
        o.append(f'<text x="{L - 10}" y="{Y(t) + 6:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{t}</text>')
    o.append(f'<text x="32" y="{T - 16}" font-size="16" fill="{SUB}">프레임 p95(ms)</text>')
    for n, lab in [(500, "500"), (1000, "1천"), (2000, "2천"), (5000, "5천"), (10000, "1만")]:
        o.append(f'<line x1="{X(n):.1f}" y1="{B}" x2="{X(n):.1f}" y2="{B + 5}" stroke="{SUB}"/>')
        o.append(f'<text x="{X(n):.1f}" y="{B + 26}" text-anchor="middle" font-size="16">{lab}</text>')
    o.append(f'<line x1="{L}" y1="{B}" x2="{R}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<line x1="{L}" y1="{T}" x2="{L}" y2="{B}" stroke="{SUB}"/>')
    o.append(f'<text x="{(L + R) / 2}" y="{B + 52}" text-anchor="middle" font-size="16" fill="{SUB}">호드 총수</text>')
    y167 = Y(BUDGET_MS)
    o.append(f'<line x1="{L}" y1="{y167:.1f}" x2="{R}" y2="{y167:.1f}" stroke="{AMBER}" stroke-dasharray="6 4" stroke-width="1.5"/>')
    # label between the two crossings, where neither line passes
    o.append(f'<text x="{X(n_at_60(2)) - 14:.1f}" y="{y167 - 8:.1f}" text-anchor="end" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
    for var, ns, col, name, step in series:
        pts = [(n, v(var, "frame_p95_ms", n)) for n in ns]
        # straight segments on a log-N axis = the log interpolation of Scripts/plot.py n_at_budget
        o.append(f'<polyline points="{" ".join(f"{X(n):.1f},{Y(m):.1f}" for n, m in pts)}" fill="none" stroke="{col}" stroke-width="2.5"/>')
        for n, m in pts:
            o.append(f'<circle cx="{X(n):.1f}" cy="{Y(m):.1f}" r="5" fill="{col}"/>')
            o.append(f'<text x="{X(n) - 9:.1f}" y="{Y(m) - 9:.1f}" text-anchor="end" font-size="16" font-weight="700" fill="{col}">{fmt(m)}</text>')
        nx = X(n_at_60(step))
        o.append(f'<line x1="{nx:.1f}" y1="{y167:.1f}" x2="{nx:.1f}" y2="{B}" stroke="{col}" stroke-width="1.5" stroke-dasharray="3 3"/>')
        o.append(f'<circle cx="{nx:.1f}" cy="{y167:.1f}" r="6" fill="white" stroke="{col}" stroke-width="3"/>')
        o.append(f'<text x="{nx + 6:.1f}" y="{B - 10}" font-size="16" font-weight="700" fill="{col}" '
                 f'paint-order="stroke" stroke="white" stroke-width="5">{n_at_60(step):,}</text>')
    lx, ly = 584, 150
    for var, ns, col, name, step in series:
        o.append(f'<line x1="{lx}" y1="{ly}" x2="{lx + 28}" y2="{ly}" stroke="{col}" stroke-width="2.5"/>')
        words = name.split(" ", 2)
        o.append(f'<text x="{lx + 36}" y="{ly + 6}" font-size="16" font-weight="700" fill="{col}">{words[0]} {words[1]}</text>')
        o.append(f'<text x="{lx + 36}" y="{ly + 28}" font-size="16">{words[2]}</text>')
        ly += 70
    o.append(f'<circle cx="{lx + 14}" cy="{ly}" r="6" fill="white" stroke="{DARK}" stroke-width="3"/>')
    o.append(f'<text x="{lx + 36}" y="{ly + 6}" font-size="16">16.7ms를</text><text x="{lx + 36}" y="{ly + 28}" font-size="16">넘기 직전의</text>'
             f'<text x="{lx + 36}" y="{ly + 50}" font-size="16">호드 총수</text>')
    s1_10k = v("V0-s1", "frame_p95_ms", 10000)
    yf = B + 92
    o.append(f'<line x1="32" y1="{yf - 22}" x2="{W - 32}" y2="{yf - 22}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{yf}" font-size="16" fill="{SUB}">16.7ms를 사이에 둔 두 점 사이를 로그 보간한다.</text>')
    o.append(f'<text x="32" y="{yf + 26}" font-size="16" fill="{SUB}">단계 1의 1만({fmt(s1_10k)}ms)은 그림 밖이다.</text>')
    write("max-entities-method.svg", svg(yf + 50, o))


# ---------- 6. stage map: what draws each distance band in each step ----------
def stage_map():
    o = []
    title(o, "여덟 단계: 카메라 거리마다 무엇으로 그리나",
          "앞 단계 위에 하나씩 더한다. 가로는 카메라 거리이고 축척이 아니다")
    LX, X0, X1 = 32, 330, 768
    edges = [0, 35, 80, 200, 500, 1000]
    xs = [X0, X0 + 96, X0 + 164, X0 + 272, X0 + 352, X1]
    XE = dict(zip(edges, xs))
    ACT, STAT, VAT, ISKM, FX = RED, GRAY, LIGHTBLUE, BLUE, TEAL
    names = {ACT: "액터", STAT: "정지 메시", VAT: "VAT", ISKM: "ISKM", FX: "입자"}
    COST, ADD = "비용을 줄이는 단계", "표현을 더하는 단계"
    # (number, name, kind, change, [(from_m, to_m, representation or None, shadow_off)])
    # Bands and steps: Docs/Guides/commands.md "사다리 단계와 스위치", LabCrowdSubsystem.cpp BuildEntityConfig
    stages = [
        ("1", "액터 기준선", None, "모든 개체가 캡슐과 CharacterMovement를 가진 액터",
         [(0, 500, ACT, False)]),
        ("2", "Mass, 엔진 기본 경로", COST, "로직을 Mass로, 엔진의 이동·회피·표현을 그대로",
         [(0, 35, ACT, False), (35, 200, STAT, False), (200, 500, None, False)]),
        ("3", "자체 밀어내기", COST, "Mass Avoidance 대신 밀어내기 프로세서, 방향 보간 병렬 사본",
         [(0, 35, ACT, False), (35, 200, STAT, False), (200, 500, None, False)]),
        ("4", "애니메이션 표현", ADD, "VAT 인스턴스, Animation Budget Allocator, 최소 유지 시간",
         [(0, 35, ACT, False), (35, 200, VAT, False), (200, 500, None, False)]),
        ("5", "Simulation LOD + 엔티티 풀", COST, "먼 개체의 로직을 드물게, 죽은 엔티티를 다시 쓴다",
         [(0, 35, ACT, False), (35, 200, VAT, False), (200, 500, None, False)]),
        ("6", "ISKM", ADD, "VAT 자리를 ISKM으로, 카메라를 따라가는 고정 바운드",
         [(0, 35, ACT, False), (35, 200, ISKM, False), (200, 500, None, False)]),
        ("7", "배경 무리", ADD, "200m 밖과 맵 밖에 Niagara 입자 무리",
         [(0, 35, ACT, False), (35, 200, ISKM, False), (200, 500, FX, False), (500, 1000, FX, False)]),
        ("8", "남은 비용 줄이기(최종 구성)", COST, "80m 밖 그림자 끔, 액터 메시 고정 바운드",
         [(0, 35, ACT, False), (35, 80, ISKM, False), (80, 200, ISKM, True), (200, 500, FX, True), (500, 1000, FX, True)]),
    ]
    y = 124
    labels = {0: "0", 35: "35", 80: "80", 200: "200", 500: "500", 1000: "1,000m"}
    for e, x in zip(edges, xs):
        anchor = "middle" if e < 1000 else "end"
        o.append(f'<text x="{x:.1f}" y="{y}" text-anchor="{anchor}" font-size="16" fill="{SUB}">{labels[e]}</text>')
    o.append(f'<text x="{(XE[500] + XE[1000]) / 2:.1f}" y="{y - 22}" text-anchor="middle" font-size="16" fill="{SUB}">맵 밖</text>')
    y += 12
    BH = 30
    for num, name, kind, change, segs in stages:
        o.append(f'<circle cx="{LX + 14}" cy="{y + 15}" r="14" fill="{DARK}"/>')
        o.append(f'<text x="{LX + 14}" y="{y + 21}" text-anchor="middle" font-size="16" font-weight="700" fill="white">{num}</text>')
        o.append(f'<text x="{LX + 38}" y="{y + 21}" font-size="17" font-weight="700">{name}</text>')
        prefix = f'<tspan font-weight="700" fill="{AMBER if kind == COST else TEAL}">{kind}</tspan> · ' if kind else ""
        o.append(f'<text x="{LX + 38}" y="{y + 52}" font-size="16" fill="{SUB}">{prefix}{change}</text>')
        for a, b, rep, shadow_off in segs:
            xa, xb = XE[a], XE[b]
            if rep is None:
                o.append(f'<rect x="{xa + 1:.1f}" y="{y}" width="{xb - xa - 2:.1f}" height="{BH}" rx="4" fill="white" stroke="{GRAY}" stroke-dasharray="4 3"/>')
                o.append(f'<text x="{(xa + xb) / 2:.1f}" y="{y + 21}" text-anchor="middle" font-size="16" fill="{SUB}">그리지 않음</text>')
                continue
            o.append(f'<rect x="{xa + 1:.1f}" y="{y}" width="{xb - xa - 2:.1f}" height="{BH}" rx="4" fill="{rep}"/>')
            if shadow_off:
                o.append(f'<rect x="{xa + 1:.1f}" y="{y}" width="{xb - xa - 2:.1f}" height="{BH}" rx="4" fill="url(#shade)"/>')
            fill = INK if rep in (VAT, STAT) else "white"
            o.append(f'<text x="{(xa + xb) / 2:.1f}" y="{y + 21}" text-anchor="middle" font-size="16" font-weight="700" fill="{fill}">{names[rep]}</text>')
        y += 74
    o.append(f'<line x1="32" y1="{y - 8}" x2="{W - 32}" y2="{y - 8}" stroke="{LINE}"/>')
    y += 14
    legend = [(ACT, False, "액터: 스켈레탈 메시와 AnimBP를 가진 액터. 단계 2부터는 가까운 개체만"),
              (STAT, False, "정지 메시: 애니메이션 없는 정적 메시 인스턴스"),
              (VAT, False, "VAT: 텍스처에 구운 애니메이션을 정점 셰이더로 재생하는 인스턴스"),
              (ISKM, False, "ISKM: GPU에서 스키닝하는 인스턴스 스켈레탈 메시"),
              (FX, False, "입자: 엔티티 자리에 걷는 모습을 그리는 Niagara 메시 입자"),
              (ISKM, True, "빗금: 그림자를 그리지 않는다")]
    for rep, shade, lab in legend:
        o.append(f'<rect x="32" y="{y}" width="22" height="18" rx="3" fill="{rep}"/>')
        if shade:
            o.append(f'<rect x="32" y="{y}" width="22" height="18" rx="3" fill="url(#shade)"/>')
        o.append(f'<text x="64" y="{y + 15}" font-size="16">{lab}</text>')
        y += 28
    y += 6
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">액터 구간은 화면 안의 거리이고 상한은 1,000이다. 화면 밖은 15m 안만 액터다.</text>')
    write("stage-map.svg", svg(y + 36, o, HATCH))


final_reading()
scenario()
pipeline()
judging()
max_entities()
stage_map()
print("wrote", sorted(os.listdir(OUT)))
