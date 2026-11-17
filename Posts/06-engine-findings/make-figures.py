# Makes the SVG figures of post 06 in images/.
# Run from the repository root: py -3 -I Posts/06-engine-findings/make-figures.py
# Measured values come from two batches of the same Test package (build de0e21b):
#   Measurements/Full4/summary.csv, equivalence.csv  (the full batch, three runs per condition)
#   Measurements/Pool2/timeline.csv                   (reference batch, entity pool long run, one run per configuration)
# The two batches are never drawn in one figure.
# images/entity-pool.svg is a diagram without measured values. Post 05 links the same file.
import csv, math, os, re, struct
from decimal import Decimal, ROUND_HALF_UP

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")


def read_csv(*parts):
    with open(os.path.join(ROOT, *parts), encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


SUMMARY = {(r["variant"], int(r["n"]), r["preset"]): r for r in read_csv("Measurements", "Full4", "summary.csv")}
EQUIV = read_csv("Measurements", "Full4", "equivalence.csv")
POOL = read_csv("Measurements", "Pool2", "timeline.csv")


def full4(variant, col, n=10000, preset="P1"):
    return float(SUMMARY[(variant, n, preset)][col])


# ---------- values ----------
N = 10000
TRACKS_ON = full4("V0-s8", "iskm_tracks_last")      # entity pool on (final configuration)
TRACKS_OFF = full4("V0-a8pool", "iskm_tracks_last")  # entity pool off


def f32(x):
    return struct.unpack("f", struct.pack("f", x))[0]


# Track reserve per ISKM component (Source/HordeOptLab/LabIskmProcessor.cpp:309-311, :409). The product is float32 in C++.
N_LARGE = (N + 9) // 10
RES_SMALL = math.ceil(f32(f32(float(N - N_LARGE)) * f32(1.1))) + 64
RES_LARGE = math.ceil(f32(f32(float(N_LARGE)) * f32(1.1))) + 64
RES_FARHIT = 64
# Step 8 has three ISKM components per unit type: [High, Low) shadow on, [Low, Off) shadow off, [Off, Max) far hit
# (Source/HordeOptLab/LabCrowdSubsystem.cpp:175-208).
RES_SUM = 2 * RES_SMALL + 2 * RES_LARGE + 2 * RES_FARHIT

# Capsule check traces, Full4 equivalence (collision on; a8cap has no capsule collision, a8hyd has no actors).
still = [r for r in EQUIV if r["speed"] == "V1" and r["n"] == "10000" and r["capsule_ok"] == "report"]
moving = [r for r in EQUIV if r["speed"] == "V0" and r["n"] == "10000" and r["capsule_ok"] != "n/a"
          and not r["cond"].endswith("a8cap") and float(r["capsule_traces"] or 0) > 0]
STILL_TRACES = sum(int(float(r["capsule_traces"])) for r in still)
STILL_MISS = sum(int(float(r["capsule_miss"])) for r in still)
MOVING_TRACES = sum(int(float(r["capsule_traces"])) for r in moving)
MOVING_MISS = sum(int(float(r["capsule_miss"])) for r in moving)
STILL_PCT = 100.0 * STILL_MISS / STILL_TRACES

# Pool2 long run, 1-minute bins.
POOL_LABEL = {"on": "Pool2-V0-s8-N10000-P1-r1", "off": "Pool2-V0-a8pool-N10000-P1-r1"}
SERIES = {}
for key, label in POOL_LABEL.items():
    rs = sorted((r for r in POOL if r["label"] == label), key=lambda r: int(r["minute"]))
    SERIES[key] = {
        "minute": [int(r["minute"]) for r in rs],
        "tracks": [float(r["iskm_tracks_end"]) for r in rs],
        "mem": [float(r["gpu_mem_mean"]) for r in rs],
        "p95": [float(r["frame_p95"]) for r in rs],
    }
    assert SERIES[key]["minute"] == list(range(1, 17)), (label, SERIES[key]["minute"])

# ---------- style (same as the other posts) ----------
FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
MONO = "Consolas, SFMono-Regular, Menlo, monospace"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, PURPLE, GRAY, AMBER, RED, TEAL, PINK, DARK, GREEN = (
    "#2563eb", "#9333ea", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151", "#15803d")
W = 800


def svg(h, body, defs=""):
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


def _no_zero_decimals(f):
    """27.0 -> 27. A value whose decimals are all zero is written as an integer."""
    return lambda *a, **k: re.sub(r"[.]0+$", "", f(*a, **k))


def fmt_keep(x):
    """Three significant figures, rounded half up from the CSV text (12.35 -> 12.4), keeping a trailing zero (14.0)
    so that values written side by side show the same precision."""
    q = "1" if abs(x) >= 100 else "0.1" if abs(x) >= 10 else "0.01"
    return str(Decimal(repr(x)).quantize(Decimal(q), rounding=ROUND_HALF_UP))


fmt = _no_zero_decimals(fmt_keep)  # as in the post body: 27.0 -> 27


def sig3(x):
    """A value of 1,000 or more to the nearest integer (5105.3 -> 5,105), with thousands separators.
    The README and the site write the pool run's GPU memory this way."""
    return f"{int(Decimal(repr(x)).quantize(Decimal('1'), rounding=ROUND_HALF_UP)):,}"


def cnt(x):
    """Whole count, rounded half up (20214.72 -> 20,215), with thousands separators."""
    return f"{int(Decimal(repr(x)).quantize(Decimal('1'), rounding=ROUND_HALF_UP)):,}"


def tw(s, size=16):
    """Rough text width in px: Hangul and CJK symbols 1 em, other characters 0.56 em, spaces 0.3 em."""
    w = 0.0
    for ch in s:
        if ch == " ":
            w += 0.3
        elif ord(ch) >= 0x2E80:
            w += 1.0
        else:
            w += 0.56
    return w * size


def title(o, t, s):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')


def lines(o, x, y, rows, color=INK, size=16, weight="400", anchor="start", step=22):
    for k, s in enumerate(rows):
        o.append(f'<text x="{x:.1f}" y="{y + k * step:.1f}" text-anchor="{anchor}" font-size="{size}" font-weight="{weight}" fill="{color}">{s}</text>')
    return y + len(rows) * step


def arrow(o, x1, y1, x2, y2, color=SUB, width=2):
    o.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{color}" stroke-width="{width}"/>')
    a = math.atan2(y2 - y1, x2 - x1)
    for s in (-1, 1):
        ax = x2 - 9 * math.cos(a) + s * 5 * math.sin(a)
        ay = y2 - 9 * math.sin(a) - s * 5 * math.cos(a)
        o.append(f'<line x1="{x2:.1f}" y1="{y2:.1f}" x2="{ax:.1f}" y2="{ay:.1f}" stroke="{color}" stroke-width="{width}"/>')


def badge(o, x, y, n, color=DARK):
    o.append(f'<circle cx="{x}" cy="{y}" r="14" fill="{color}"/>')
    o.append(f'<text x="{x}" y="{y + 6}" text-anchor="middle" font-size="16" font-weight="700" fill="white">{n}</text>')


def box(o, x, y, w, h, rows, stroke=DARK, fill=CARD, bold_first=True, dashed=False, color=INK):
    da = ' stroke-dasharray="6 4"' if dashed else ""
    o.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="6" fill="{fill}" stroke="{stroke}" stroke-width="2"{da}/>')
    n = len(rows)
    y0 = y + h / 2 - (n - 1) * 11 + 6
    for k, s in enumerate(rows):
        wt = "700" if bold_first and k == 0 else "400"
        assert tw(s) <= w - 12, ("box text too wide", s, tw(s), w)
        o.append(f'<text x="{x + w / 2:.1f}" y="{y0 + k * 22:.1f}" text-anchor="middle" font-size="16" font-weight="{wt}" fill="{color}">{s}</text>')


# ---------- 1. summary map: where the five problems sit ----------
def findings_map():
    o = []
    title(o, "엔진에서 찾은 문제 다섯과 이 레포의 우회", "번호는 본문의 차례다. 파란 글자가 이 레포가 바꾼 것이다")
    BW, GAP, X0 = 170, 20, 32
    xs = [X0 + k * (BW + GAP) for k in range(4)]
    y = 108
    o.append(f'<text x="32" y="{y}" font-size="17" font-weight="700">ISKM 개체를 그리는 한 프레임</text>')
    y += 14
    names = [["Mass 프로세서", "게임·작업자 스레드"], ["Mass 표현", "ISKM 트랙 맵"],
             ["트랙 제공자", "클립 → 뼈 자세"], ["렌더러", "뼈대 버퍼"]]
    for k, (x, rows) in enumerate(zip(xs, names)):
        box(o, x, y, BW, 62, rows)
        if k < 3:
            arrow(o, x + BW + 2, y + 31, x + BW + GAP - 2, y + 31)
    y += 62 + 32
    items = [  # (column, number, symptom lines, workaround lines)
        (0, 4, ["쓰기를 선언한 쿼리가", "게임 스레드를", "기다리게 한다"], ["위치 쓰기를", "따로 된 쿼리로"]),
        (1, 1, ["움직이는 엔티티를", "파괴하면 트랙이", "남는다"], ["엔티티 풀"]),
        (2, 3, ["가산 클립을 그대로", "자세로 쓴다"], ["비가산 복제 클립"]),
        (3, 2, ["트랙 수가 바뀌면", "이전 자세 칸이", "깨진다"], ["트랙 풀을", "미리 늘린다"]),
    ]
    bottom = y
    for col, n, sym, fix in items:
        x = xs[col]
        badge(o, x + 14, y, n)
        for s in sym + fix:
            assert tw(s) <= BW, ("map text too wide", s)
        yy = lines(o, x, y + 34, sym)
        yy = lines(o, x, yy + 6, fix, color=BLUE, weight="700")
        bottom = max(bottom, yy)
    y = bottom + 18
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y += 34
    o.append(f'<text x="32" y="{y}" font-size="17" font-weight="700">동등성 검사의 확인 트레이스</text>')
    y += 14
    box(o, xs[0], y, BW, 62, ["오브", "캡슐로 선을 쏜다"])
    arrow(o, xs[0] + BW + 2, y + 31, xs[1] - 2, y + 31)
    box(o, xs[1], y, BW, 62, ["Chaos 장면 질의", "라인 트레이스"])
    badge(o, xs[2] + 14, y + 16, 5)
    lines(o, xs[2] + 38, y + 22, ["캡슐 정면 평면 위에서 출발한 선을 놓친다"])
    lines(o, xs[2] + 38, y + 50, ["정지 장면의 빗나감은 보고만 한다"], color=BLUE, weight="700")
    assert xs[2] + 38 + tw("캡슐 정면 평면 위에서 출발한 선을 놓친다") <= W - 24
    y += 62 + 30
    write("findings-map.svg", svg(y, o))


# ---------- 2. entity pool diagram (shared with post 05) ----------
def entity_pool():
    o = []
    title(o, "엔티티 풀: 죽은 엔티티를 파괴하지 않고 다시 쓴다",
          "ISKM 트랙 맵의 키는 엔티티 핸들이다. 핸들이 같으면 엔진이 옛 트랙을 다시 쓴다")
    BW, GAP, X0, BH = 166, 22, 32, 76
    xs = [X0 + k * (BW + GAP) for k in range(4)]
    lanes = [
        ("엔티티 풀이 없을 때(파괴)", GRAY, [["사망이 끝난", "엔티티 A"], ["A를 파괴한다"], ["새 엔티티 B를", "만든다"], ["맵에 B가 없어", "새 트랙을 할당"]],
         RED, "A의 트랙은 돌려받지 않는다. 트랙 수가 계속 는다"),
        ("엔티티 풀(이 레포)", BLUE, [["사망이 끝난", "엔티티 A"], ["풀에 넣고", "맵 밖에 세운다"], ["재생성 때 A를 꺼내", "위치·체력·상태를", "다시 넣는다"], ["맵에서 A의", "옛 트랙을 찾는다"]],
         BLUE, "새 트랙이 생기지 않는다. 트랙 수가 그대로다"),
    ]
    y = 104
    for name, col, steps, ncol, note in lanes:
        o.append(f'<text x="32" y="{y + 4}" font-size="17" font-weight="700" fill="{col if col != GRAY else DARK}">{name}</text>')
        y += 16
        for k, rows in enumerate(steps):
            box(o, xs[k], y, BW, BH, rows, stroke=col, bold_first=False)
            if k < 3:
                arrow(o, xs[k] + BW + 2, y + BH / 2, xs[k] + BW + GAP - 2, y + BH / 2)
        y += BH + 26
        o.append(f'<text x="{xs[3] + BW:.1f}" y="{y}" text-anchor="end" font-size="16" font-weight="700" fill="{ncol}">{note}</text>')
        y += 40
    o.append(f'<line x1="32" y1="{y - 16}" x2="{W - 32}" y2="{y - 16}" stroke="{LINE}"/>')
    y = lines(o, 32, y + 10, ["트랙은 ISKM 컴포넌트 안에서 인스턴스 하나가 애니메이션을 재생하는 칸이다.",
                              "풀에 넣은 엔티티는 표현이 꺼져 ISKM 인스턴스는 지워지지만, 트랙은 맵에 남아 있다."], color=SUB)
    write("entity-pool.svg", svg(y + 10, o))


# ---------- 3. entity pool long run (reference batch) ----------
def pool_long_run():
    o = []
    title(o, "엔티티 풀을 켜고 끈 16분 실행", "호드 총수 1만, 이동 장면, 오브 시점 카메라, 1분 구간의 값")
    o.append(f'<text x="32" y="98" font-size="16" fill="{SUB}">조건마다 한 번 잰 참고 값이라 범위가 없다</text>')
    L, R = 120, 620
    X = lambda m: L + (m - 1) / 15 * (R - L)
    # legend
    ly = 128
    o.append(f'<line x1="{L}" y1="{ly - 5}" x2="{L + 32}" y2="{ly - 5}" stroke="{BLUE}" stroke-width="3"/>')
    o.append(f'<text x="{L + 40}" y="{ly}" font-size="16">풀 켬(최종 구성)</text>')
    o.append(f'<line x1="{L + 200}" y1="{ly - 5}" x2="{L + 232}" y2="{ly - 5}" stroke="{RED}" stroke-width="3" stroke-dasharray="7 5"/>')
    o.append(f'<text x="{L + 240}" y="{ly}" font-size="16">풀 끔</text>')
    panels = [
        ("ISKM 트랙 수(구간 끝)", "tracks", 0, 130000, [0, 50000, 100000], lambda v: f"{int(v):,}", cnt, None),
        ("GPU 메모리(구간 평균, MB)", "mem", 3000, 5500, [3000, 4000, 5000], lambda v: f"{int(v):,}", sig3, None),
        ("프레임 p95(ms)", "p95", 13, 17, [13, 14, 15, 16, 17], lambda v: f"{v:g}", fmt_keep, 16.667),
    ]
    PH = 120
    y = 160
    labels = []
    for name, key, lo, hi, ticks, tickfmt, valfmt, line167 in panels:
        o.append(f'<text x="32" y="{y + 4}" font-size="17" font-weight="700">{name}</text>')
        top, bot = y + 18, y + 18 + PH
        Y = lambda v, top=top, bot=bot, lo=lo, hi=hi: bot - (v - lo) / (hi - lo) * (bot - top)
        for t in ticks:
            o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="{LINE}"/>')
            o.append(f'<text x="{L - 10}" y="{Y(t) + 6:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{tickfmt(t)}</text>')
        o.append(f'<line x1="{L}" y1="{bot}" x2="{R}" y2="{bot}" stroke="{SUB}"/>')
        if line167:
            o.append(f'<line x1="{L}" y1="{Y(line167):.1f}" x2="{R}" y2="{Y(line167):.1f}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
            o.append(f'<text x="{L + 8}" y="{Y(line167) - 7:.1f}" font-size="16" font-weight="700" fill="{AMBER}">16.7ms(60fps)</text>')
        for sk, col, dash in [("off", RED, ' stroke-dasharray="7 5"'), ("on", BLUE, "")]:
            s = SERIES[sk]
            pts = " ".join(f"{X(m):.1f},{Y(v):.1f}" for m, v in zip(s["minute"], s[key]))
            o.append(f'<polyline points="{pts}" fill="none" stroke="{col}" stroke-width="2.5"{dash}/>')
        # end labels at minute 16, pushed apart when they are closer than 20 px
        ends = sorted([(Y(SERIES[sk][key][-1]), sk) for sk in ("on", "off")])
        if ends[1][0] - ends[0][0] < 20:
            mid = (ends[0][0] + ends[1][0]) / 2
            ends = [(mid - 10, ends[0][1]), (mid + 10, ends[1][1])]
        for yy, sk in ends:
            col = BLUE if sk == "on" else RED
            v = SERIES[sk][key][-1]
            labels.append(f'<text x="{R + 10}" y="{yy + 6:.1f}" font-size="16" font-weight="700" fill="{col}">{valfmt(v)}</text>')
        # start values (minute 1) written on the panel title line, right-aligned
        on1, off1 = SERIES["on"][key][0], SERIES["off"][key][0]
        o.append(f'<text x="{W - 32}" y="{y + 4}" text-anchor="end" font-size="16" fill="{SUB}">1분: 풀 켬 <tspan font-weight="700" fill="{BLUE}">{valfmt(on1)}</tspan>, '
                 f'풀 끔 <tspan font-weight="700" fill="{RED}">{valfmt(off1)}</tspan></text>')
        y = bot + 40
    o.extend(labels)
    # x axis ticks under the last panel
    y -= 18
    for m in (1, 4, 8, 12, 16):
        o.append(f'<line x1="{X(m):.1f}" y1="{y - 22}" x2="{X(m):.1f}" y2="{y - 17}" stroke="{SUB}"/>')
        o.append(f'<text x="{X(m):.1f}" y="{y}" text-anchor="middle" font-size="16" fill="{SUB}">{m}</text>')
    o.append(f'<text x="{(L + R) / 2:.1f}" y="{y + 24}" text-anchor="middle" font-size="16" fill="{SUB}">측정을 시작한 뒤의 시간(분)</text>')
    y += 46
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y = lines(o, 32, y + 28, ["오른쪽 숫자는 16분 구간의 값이다. 측정은 워밍업 60초 뒤에 시작했다."], color=SUB)
    write("pool-long-run.svg", svg(y + 4, o))


# ---------- 4. ISKM bone buffer re-allocation ----------
def iskm_realloc():
    o = []
    pat = (f'<pattern id="hatch" width="8" height="8" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
           f'<rect width="8" height="8" fill="#dbeafe"/><line x1="0" y1="0" x2="0" y2="8" stroke="{BLUE}" stroke-width="3"/></pattern>')
    title(o, "트랙 수가 바뀔 때 뼈대 버퍼를 다시 잡는 순서",
          "한 칸은 트랙 하나의 지금·이전 자세다. 옛 자리 5칸을 새 자리 6칸으로 옮긴다")
    C, X0, CW = 14, 236, 38  # cells, first cell x, cell width
    cx = lambda i: X0 + i * CW
    LX = 32

    def cells(y, spec, h=34):
        # spec: list of (first, last, fill, stroke, dashed, text, text color)
        for a, b, fill, stroke, dashed, text, tc in spec:
            da = ' stroke-dasharray="5 4"' if dashed else ""
            o.append(f'<rect x="{cx(a) + 1:.1f}" y="{y}" width="{(b - a + 1) * CW - 2:.1f}" height="{h}" rx="4" fill="{fill}" stroke="{stroke}" stroke-width="2"{da}/>')
            if text:
                assert tw(text) <= (b - a + 1) * CW - 6, ("cell text too wide", text)
                o.append(f'<text x="{cx(a) + (b - a + 1) * CW / 2:.1f}" y="{y + h / 2 + 6:.1f}" text-anchor="middle" font-size="16" font-weight="700" fill="{tc}">{text}</text>')

    OLD = (2, 6)
    NEW = (0, 5)
    other = (7, 13, "#e5e7eb", GRAY, False, "다른 컴포넌트", DARK)
    y = 104
    lines(o, LX, y + 23, ["처음"], weight="700")
    cells(y, [(0, 1, "white", GRAY, True, "빈칸", SUB), (OLD[0], OLD[1], BLUE, BLUE, False, "옛 자리", "white"), other])
    y += 64
    lines(o, LX, y + 14, ["① 옛 자리를", "먼저 해제한다"], weight="700")
    cells(y, [(0, 1, "white", GRAY, True, "", SUB), (OLD[0], OLD[1], "url(#hatch)", BLUE, False, "", INK), other])
    o.append(f'<text x="{cx(OLD[0]) + 5 * CW / 2:.1f}" y="{y + 23}" text-anchor="middle" font-size="16" font-weight="700" fill="{INK}" '
             f'stroke="white" stroke-width="5" paint-order="stroke">해제, 내용은 남음</text>')
    y += 64
    lines(o, LX, y + 14, ["② 새 자리를", "할당한다"], weight="700")
    cells(y, [(NEW[0], NEW[1], GREEN, GREEN, False, "새 자리", "white"), (6, 6, "white", GRAY, True, "", SUB), other], h=30)
    lines(o, cx(0), y + 52, ["맞는 빈칸이 없어 빈칸과 해제한 칸을 합치고, 그 앞에서 시작한다"], color=SUB)
    y += 80
    lines(o, LX, y + 14, ["③ 옛 → 새", "병렬 복사"], weight="700")
    # source row (old) and destination row (new)
    sy, dy = y + 64, y
    cells(dy, [(NEW[0], NEW[1] - 1, "#dcfce7", GREEN, False, "", INK)], h=26)
    cells(sy, [(OLD[0], OLD[1], "#dbeafe", BLUE, False, "", INK)], h=26)
    for i in range(OLD[0], OLD[1] + 1):
        o.append(f'<text x="{cx(i) + CW / 2:.1f}" y="{sy + 19}" text-anchor="middle" font-size="16" fill="{BLUE}">{i - OLD[0] + 1}</text>')
    for i in range(NEW[0], NEW[1]):
        o.append(f'<text x="{cx(i) + CW / 2:.1f}" y="{dy + 19}" text-anchor="middle" font-size="16" fill="{GREEN}">{i - NEW[0] + 1}</text>')
    for k in range(OLD[1] - OLD[0] + 1):
        x1, x2 = cx(OLD[0] + k) + CW / 2, cx(NEW[0] + k) + CW / 2
        arrow(o, x1, sy - 2, x2, dy + 28, color=SUB, width=1.5)
    # overlap: buffer cells 2..4 are read as old 1..3 and written as new 3..5
    ov0, ov1 = OLD[0], NEW[1] - 1
    o.append(f'<rect x="{cx(ov0) - 2:.1f}" y="{dy - 6}" width="{(ov1 - ov0 + 1) * CW + 4:.1f}" height="{sy - dy + 38}" rx="6" fill="none" stroke="{RED}" stroke-width="2.5" stroke-dasharray="6 4"/>')
    lines(o, cx(OLD[1] + 1) + 16, dy + 20, ["겹친 칸: 같은 버퍼에서", "아직 읽지 않은 옛 칸을", "새 칸이 덮어쓸 수 있다"], color=RED, weight="700")
    assert cx(OLD[1] + 1) + 16 + tw("아직 읽지 않은 옛 칸을") <= W - 24
    y = sy + 26 + 36
    o.append(f'<line x1="32" y1="{y - 12}" x2="{W - 32}" y2="{y - 12}" stroke="{LINE}"/>')
    y = lines(o, 32, y + 14, ["복사 셰이더는 두 자리가 겹치지 않는다고 가정한다.",
                              "이전 자세 칸이 깨진 프레임에는 컴포넌트의 모든 인스턴스가 틀린 속도를 받는다.",
                              "모션 블러가 그 속도로 같은 종류의 개체를 한 프레임 길게 끈다."], color=SUB)
    y = lines(o, 32, y + 6, ["이 레포: 컴포넌트가 처음 쓰일 때 트랙 수를 용량까지 늘려 둔다. 그 뒤로는 ①–③이 없다."], color=BLUE, weight="700")
    write("iskm-realloc.svg", svg(y + 6, o, defs=pat))


# ---------- 5. query contention wait ----------
def query_wait():
    o = []
    pat = (f'<pattern id="wait" width="8" height="8" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
           f'<rect width="8" height="8" fill="#f3f4f6"/><line x1="0" y1="0" x2="0" y2="8" stroke="{GRAY}" stroke-width="3"/></pattern>')
    title(o, "위치를 쓰는 쿼리끼리는 서로를 기다린다", "한 프레임의 개념도다. 막대의 길이는 잰 값이 아니다")
    LX, T0 = 32, 180
    X = lambda t: T0 + t * 5.2  # t in arbitrary units 0..100

    def bar(y, t0, t1, fill, text, tc="white", h=34, dashed=False, stroke=None):
        da = ' stroke-dasharray="6 4"' if dashed else ""
        st = stroke or fill
        o.append(f'<rect x="{X(t0):.1f}" y="{y}" width="{X(t1) - X(t0):.1f}" height="{h}" rx="5" fill="{fill}" stroke="{st}" stroke-width="2"{da}/>')
        if text:
            assert tw(text) <= X(t1) - X(t0) - 8, ("bar text too wide", text)
            o.append(f'<text x="{(X(t0) + X(t1)) / 2:.1f}" y="{y + h / 2 + 6:.1f}" text-anchor="middle" font-size="16" font-weight="700" fill="{tc}">{text}</text>')

    y = 108
    for head, fixed in [("고치기 전", False), ("고친 뒤", True)]:
        o.append(f'<text x="{LX}" y="{y + 4}" font-size="18" font-weight="700">{head}</text>')
        y += 20
        o.append(f'<text x="{LX}" y="{y + 23}" font-size="16" fill="{SUB}">작업자 스레드</text>')
        bar(y, 0, 76, PURPLE, "밀어내기 프로세서: 위치를 읽고 쓴다")
        y += 46
        o.append(f'<text x="{LX}" y="{y + 23}" font-size="16" fill="{SUB}">게임 스레드</text>')
        if not fixed:
            bar(y, 12, 76, "url(#wait)", "", stroke=GRAY)
            o.append(f'<text x="{(X(12) + X(76)) / 2:.1f}" y="{y + 23}" text-anchor="middle" font-size="16" font-weight="700" fill="{INK}" '
                     f'stroke="white" stroke-width="5" paint-order="stroke">쿼리 시작 전에 기다린다</text>')
            bar(y, 76, 84, BLUE, "")
            o.append(f'<text x="{X(84) + 10:.1f}" y="{y + 23}" font-size="16" font-weight="700" fill="{BLUE}">실제 일</text>')
            y += 46
            lines(o, X(0), y + 8, ["피격 ISKM 프로세서의 쿼리가 위치를 읽기·쓰기로 선언했다.",
                                   "엔진은 두 쿼리를 아키타입과 상관없이 동시에 돌리지 않는다."], color=SUB)
            y += 60
        else:
            bar(y, 12, 20, BLUE, "")
            o.append(f'<text x="{X(20) + 10:.1f}" y="{y + 23}" font-size="16" font-weight="700" fill="{BLUE}">주 쿼리: 위치 없음, 바로 돈다</text>')
            bar(y, 80, 98, "white", "따라잡기", tc=SUB, dashed=True, stroke=GRAY)
            y += 46
            lines(o, X(0), y + 8, ["위치를 쓰는 따라잡기는 따로 된 쿼리로 뺐다.",
                                   "따라잡을 개체가 있는 드문 프레임에만 돈다(점선)."], color=SUB)
            y += 60
        y += 14
    write("query-wait.svg", svg(y, o, defs=pat))


# ---------- 6. capsule check trace on the front plane ----------
def capsule_plane():
    o = []
    title(o, "확인 트레이스가 캡슐 정면 평면 위에 놓이면 빗나간다",
          "위에서 본 그림. 선은 오브 구체 중심에서 대상 캡슐 중심으로 간다")
    # left: still scene, right: moving scene
    panels = [("정지 장면: 오브가 원점에 서 있다", 200, 0.0, RED,
               [f"{STILL_TRACES:,}회 중 {STILL_MISS}회 빗나감({fmt(STILL_PCT)}%)"], "선이 정면 평면 위에 있다"),
              ("이동 장면: 오브가 원을 돈다", 600, 24.0, GREEN,
               [f"{MOVING_TRACES:,}회 중 {MOVING_MISS}회 빗나감"], "회전이 늦게 따라가 평면이 비켜 있다")]
    for name, cxp, yaw, col, stat, note in panels:
        o.append(f'<text x="{cxp}" y="112" text-anchor="middle" font-size="17" font-weight="700">{name}</text>')
        sx, sy = cxp, 160       # trace start (orb sphere centre, seen from above)
        tx, ty = cxp, 340       # target capsule centre
        # the minion faces the orb centre; in the moving scene the orb has moved, so the facing is turned by `yaw`
        a = math.radians(90 + yaw)  # facing direction from the capsule (up the screen)
        fx, fy = math.cos(a), -math.sin(a)
        # front plane = facing axis through the capsule centre (a line seen from above)
        topx, topy = tx + fx * 215, ty + fy * 215
        o.append(f'<line x1="{tx - fx * 50:.1f}" y1="{ty - fy * 50:.1f}" x2="{topx:.1f}" y2="{topy:.1f}" '
                 f'stroke="{AMBER}" stroke-width="2" stroke-dasharray="7 5"/>')
        o.append(f'<text x="{topx - 12:.1f}" y="{topy + 6:.1f}" text-anchor="end" font-size="16" font-weight="700" fill="{AMBER}">정면 평면</text>')
        o.append(f'<circle cx="{tx}" cy="{ty}" r="30" fill="#e5e7eb" stroke="{DARK}" stroke-width="2"/>')
        arrow(o, tx, ty, tx + fx * 26, ty + fy * 26, color=DARK, width=2.5)
        o.append(f'<line x1="{sx}" y1="{sy}" x2="{tx}" y2="{ty}" stroke="{col}" stroke-width="3"/>')
        o.append(f'<circle cx="{sx}" cy="{sy}" r="9" fill="{PURPLE}"/>')
        o.append(f'<text x="{sx + 16}" y="{sy + 6}" font-size="16" fill="{SUB}">시작점(오브)</text>')
        o.append(f'<text x="{tx + 40}" y="{ty + 30}" font-size="16" fill="{SUB}">대상 캡슐</text>')
        o.append(f'<text x="{cxp}" y="408" text-anchor="middle" font-size="16" fill="{SUB}">{note}</text>')
        for k, s in enumerate(stat):
            o.append(f'<text x="{cxp}" y="{434 + k * 22}" text-anchor="middle" font-size="16" font-weight="700" fill="{col}">{s}</text>')
    y = 470
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y = lines(o, 32, y + 28, ["호드 총수 1만, 오브 시점 카메라, 캡슐 콜리전을 켠 조건의 합. 세 번씩 쟀다.",
                              "개발 빌드 진단: 같은 프레임, 같은 자리의 겹침 질의는 빗나간 캡슐을 모두 찾았다."], color=SUB)
    write("capsule-plane.svg", svg(y + 4, o))


findings_map()
entity_pool()
pool_long_run()
iskm_realloc()
query_wait()
capsule_plane()

print("wrote", sorted(f for f in os.listdir(OUT) if f.endswith(".svg")))
print("Full4 iskm_tracks_last: pool on", TRACKS_ON, "->", cnt(TRACKS_ON), "| pool off", TRACKS_OFF, "->", cnt(TRACKS_OFF))
print("track reserve: small", RES_SMALL, "large", RES_LARGE, "far hit", RES_FARHIT, "sum of six components", RES_SUM)
print("frame p95: s8", full4("V0-s8", "frame_p95_ms"), "a8pool", full4("V0-a8pool", "frame_p95_ms"), "a8res", full4("V0-a8res", "frame_p95_ms"))
print("capsule still", STILL_MISS, "/", STILL_TRACES, "=", round(STILL_PCT, 3), "% over", len(still), "conditions | moving", MOVING_MISS, "/", MOVING_TRACES, "over", len(moving), "conditions")
for key in ("on", "off"):
    s = SERIES[key]
    print("Pool2", key, "min1 tracks", s["tracks"][0], "mem", s["mem"][0], "p95", s["p95"][0],
          "| min16 tracks", s["tracks"][-1], "mem", s["mem"][-1], "p95", s["p95"][-1])
