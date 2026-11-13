# Makes the SVG figures of the root README in Docs/figures/readme/.
# Run from anywhere: py -3 -I Scripts/plot-readme.py
# Every value comes from the latest full batch (Full2), except the transition figure:
#   Measurements/Full2/summary.csv               ladder, resources and ablation (10000, P1)
#   Measurements/Full2/runs.csv                  run ranges of the resource figure
#   Docs/figures/full2/full-n-at-60fps.csv       max entities at 60 fps (n_p95)
#   Measurements/Pool1/timeline.csv              entity pool on/off over 16 minutes (one run each, 1-minute bins)
#   Measurements/Full1/transition-dev/runs.csv   transition pop (Development reference, not re-measured with Full2)
# Colours and fonts follow Posts/01-actor-baseline/make-figures.py.
import csv, os, statistics
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
OUT = os.path.join(ROOT, "Docs", "figures", "readme")
N, PRESET = 10000, "P1"
BATCH = "Full2"

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, TEAL, PINK, AMBER, DARK, GRAY = "#2563eb", "#0d9488", "#db2777", "#b45309", "#374151", "#9ca3af"
W = 800
TARGET_MS = 1000.0 / 60.0

# Step labels: (number, name, detail lines). Condition labels come from Scripts/full-conditions.json.
STEPS = [
    ("L1-full", 1, "액터 군중", ["ACharacter + AnimBP"]),
    ("L2-mass", 2, "Mass 로직", ["애니메이션 없는 ISM"]),
    ("L3-vatcull", 3, "ISM + VAT", ["200m 밖은 그리지 않음"]),
    ("L4-mass", 4, "액터 하이드레이션", ["30m 안 액터(상한 500)", "+ 애니메이션 예산"]),
    ("L5-m4offpool", 5, "Simulation LOD", ["+ 엔티티 풀"]),
    ("L6-m5bounds", 6, "ISKM", ["+ 고정 바운드"]),
    ("L7-q2crowd", 7, "Niagara 배경 무리", ["200m 밖, 엔티티 위치로", "움직이는 입자"]),
    ("L8-q4final", 8, "품질 올리기", ["액터 0~40m(상한 1,000)", "80m 밖 그림자 끔"]),
]
ABLATIONS = [
    # (condition, what the final configuration does, what the run does instead)
    ("A8-hydrate", "가까운 개체를 액터로(하이드레이션)", ["빼면: 가까운 개체도 ISKM으로 그림"]),
    ("A8-budget", "Animation Budget Allocator", ["빼면: 액터가 예산 없이 매 프레임", "애니메이션을 평가"]),
    ("A8-simlod", "Simulation LOD", ["빼면: 80m 밖도 접근·피해 로직을 매 프레임,", "200m 밖도 엔진 이동·조향으로 옮김"]),
    ("A8-bounds", "ISKM 컴포넌트의 고정 바운드", ["빼면: 엔진이 매 프레임 바운드를 다시 계산"]),
    ("A8-fix", "액터 전환 대책 다섯 가지", ["빼면: 인스턴스와 액터가 바뀔 때의", "보정을 모두 끔"]),
    ("A8-pool", "엔티티 풀", ["빼면: 죽은 개체의 엔티티를 파괴하고", "새로 만듦"]),
    ("A8-iskm", "40~200m를 ISKM으로", ["빼면: ISM + VAT로 그림"]),
    ("A8-farshadow", "80m 밖 인스턴스 그림자 끔", ["빼면: 80m 밖도 그림자를 그림"]),
    ("A8-farcull", "200m 밖 엔티티를 입자로", ["빼면: 입자 대신 ISKM으로 그림"]),
    ("A8-crowdfx", "Niagara 배경 무리", ["빼면: 200m 밖에 아무것도 그리지 않음"]),
]
BASE = "L8-q4final"
TRANSITIONS = [
    ("iskm-fix", "최종 구성", ["ISKM, 전환 대책 켬"]),
    ("vat-fix", "VAT로 되돌림", []),
    ("iskm-nofix", "전환 대책 끔", []),
    ("vat-nofix", "둘 다", ["VAT, 전환 대책 끔"]),
]
METRICS = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE),
           ("렌더 스레드", "render_p50_ms", TEAL), ("GPU", "gpu_p50_ms", PINK)]
BOTTLENECK_COL = {"game": "game_p50_ms", "render": "render_p50_ms", "gpu": "gpu_p50_ms"}

# Scenario figure (scenario.svg). Not measured values: the fixed scenario in the source, in metres and seconds.
MAP_R = 500           # LabConfig.h FLabScenario::MapRadius (50000 cm)
RESPAWN_IN = 490      # FLabScenario::MapRadius - RespawnRingWidth (1000 cm); LabCrowdSpawner.cpp respawn radius
ZONE_R = 8            # FLabScenario::ZoneRadius (800 cm); also the inner edge of the initial spawn radius
ZONE_DPS = (100, 300) # FLabScenario::ZoneDpsEdge, ZoneDpsCenter
HEALTH = (60, 240)    # FLabScenario::HealthMin, HealthMax (FLabConfig::HealthFor, uniform per serial)
SPEED = 3             # FLabScenario::MoveSpeed (300 cm/s)
SNIPE_EVERY, SNIPE_K = 2, 5            # FLabScenario::SnipeInterval, SnipeCount
SNIPE_EDGES = (30, 80, 200)            # LabSnipe.h FLabSnipeBands::Edges (last edge is the map radius)
SNIPE_COUNTS = (1, 2, 2)               # FLabSnipeBands::Counts
LARGE_EVERY = 10      # LabUnitAssets.h FLabUnitTypes::IsLarge: serial % 10 == 0 -> 10 % large
P1_BACK, P1_HEIGHT, P1_PERIOD = 15, 9, 60  # FLabScenario::OrbViewBack, OrbViewHeight, OrbViewPeriod (s per turn)
P2_HEIGHT = 100       # FLabScenario::TopDownHeight (10000 cm), straight down (LabCameraRig.cpp Apply)
FOV_H = 90            # engine default UCameraComponent::FieldOfView (CameraComponent.cpp:78); ALabCameraRig keeps it
SEED = 20261106       # FLabConfig::Seed; only seeds the drawn dots here
DOTS = 700            # dots drawn (illustration, not N)
PURPLE = "#7c3aed"


def load_summary():
    rows = {}
    with open(os.path.join(ROOT, "Measurements", BATCH, "summary.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if int(r["n"]) == N and r["preset"] == PRESET:
                rows[r["variant"]] = r
    return rows


S = load_summary()


def val(cond, col):
    return float(S[cond][col])


def rng(cond, col):
    return float(S[cond][col + "_max"]) - float(S[cond][col + "_min"])


def _no_zero_decimals(f):
    """27.0 -> 27. A value whose decimals are all zero is written as an integer."""
    return lambda *a, **k: re.sub(r"[.]0+$", "", f(*a, **k))


@_no_zero_decimals
def fmt(x, signed=False):
    """Three significant figures."""
    a = abs(x)
    s = f"{a:.0f}" if a >= 100 else f"{a:.1f}" if a >= 10 else f"{a:.2f}"
    if float(s) == 0:
        return s
    if signed:
        return ("+" if x > 0 else "−" if x < 0 else "±") + s
    return ("−" if x < 0 else "") + s


def svg(h, body):
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">',
            f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>']
    return "\n".join(head + body + ["</svg>"]) + "\n"


def write(name, text):
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def esc(t):
    return t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def title(o, t, subs):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{esc(t)}</text>')
    for i, s in enumerate(subs):
        o.append(f'<text x="32" y="{74 + i * 22}" font-size="16" fill="{SUB}">{esc(s)}</text>')
    return 74 + (len(subs) - 1) * 22


def label(o, x, y, head, lines, head_size=17):
    o.append(f'<text x="{x}" y="{y}" font-size="{head_size}" font-weight="700">{esc(head)}</text>')
    for i, ln in enumerate(lines):
        o.append(f'<text x="{x}" y="{y + 21 * (i + 1)}" font-size="16" fill="{SUB}">{esc(ln)}</text>')


def legend(o, y, items):
    x = 32
    for name, c in items:
        o.append(f'<rect x="{x}" y="{y - 13}" width="16" height="16" rx="3" fill="{c}"/>')
        o.append(f'<text x="{x + 22}" y="{y}" font-size="16">{esc(name)}</text>')
        x += 22 + len(name) * 16 + 28 if not name.isascii() else 22 + len(name) * 10 + 28


# Steps 1-6 add a technique each and earn frame time; steps 7-8 spend it on quality (README "결과 한눈에 보기").
SPEND_FROM = 7
GROUPS = {1: "1–6단계 · 기법을 더해 프레임 시간을 버는 단계", SPEND_FROM: "7·8단계 · 번 시간을 품질에 쓰는 단계"}
HATCH = ('<defs><pattern id="spend" patternUnits="userSpaceOnUse" width="7" height="7" patternTransform="rotate(45)">'
         '<rect width="3" height="7" fill="#ffffff" fill-opacity="0.55"/></pattern></defs>')


def bar(o, x, y, w, h, c, num, rx=3):
    """A bar; steps that spend frame time get diagonal stripes on the same colour (not colour alone)."""
    o.append(f'<rect x="{x:.1f}" y="{y}" width="{w:.1f}" height="{h}" rx="{rx}" fill="{c}"/>')
    if num >= SPEND_FROM:
        o.append(f'<rect x="{x:.1f}" y="{y}" width="{w:.1f}" height="{h}" rx="{rx}" fill="url(#spend)"/>')


def group_head(o, y, num):
    """Group name above the first step of a group; a heavier rule above the spending group."""
    if num == SPEND_FROM:
        o.append(f'<line x1="32" y1="{y - 20}" x2="{W - 32}" y2="{y - 20}" stroke="{SUB}" stroke-width="1.5"/>')
    o.append(f'<text x="32" y="{y}" font-size="16" font-weight="700" fill="{SUB}">{esc(GROUPS[num])}</text>')


def hatch_key(o, y):
    o.append(f'<rect x="32" y="{y - 13}" width="34" height="16" rx="3" fill="{GRAY}"/>')
    o.append(f'<rect x="32" y="{y - 13}" width="34" height="16" rx="3" fill="url(#spend)"/>')
    o.append(f'<text x="74" y="{y}" font-size="16" fill="{SUB}">빗금 막대: 번 시간을 품질에 쓰는 단계(7·8단계)</text>')


# ---------- 1. ladder ----------
def ladder():
    o = [HATCH]
    y = title(o, "단계별 프레임 시간",
              ["호드 총수 1만, 오브 시점 카메라, 60초씩 세 번 잰 중앙값(ms)"])
    y += 34
    legend(o, y, [(m[0], m[2]) for m in METRICS])
    y += 14
    LX, BX, BR = 32, 330, 740      # label column, bar start, where the 16.7 ms line sits in panel 2
    BAR, STEP = 17, 21

    def block(y, cond, num, name, lines, scale):
        top = y
        label(o, LX, y + 16, f"{num}단계 {name}", lines)
        bot = S[cond]["bottleneck"]
        by = y + 2
        for mname, col, c in METRICS:
            v = val(cond, col)
            w = v * scale
            bar(o, BX, by, w, BAR, c, num)
            tag = ""
            if BOTTLENECK_COL.get(bot) == col:
                tag = f'<tspan font-weight="700" fill="{AMBER}"> 병목</tspan>'
            o.append(f'<text x="{BX + w + 7:.1f}" y="{by + 14}" font-size="16">'
                     f'<tspan font-weight="700" fill="{c}">{fmt(v)}</tspan>{tag}</text>')
            by += STEP
        return max(by, top + 22 + 21 * len(lines)) + 16

    def target_line(y0, y1, scale, note):
        x = BX + TARGET_MS * scale
        o.append(f'<line x1="{x:.1f}" y1="{y0}" x2="{x:.1f}" y2="{y1}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
        o.append(f'<text x="{x - 6:.1f}" y="{y0 + 4}" text-anchor="end" font-size="16" font-weight="700" fill="{AMBER}">{note}</text>')

    # Panel 1: step 1 on its own scale, so the four bars keep their true ratio.
    y += 30
    group_head(o, y, 1)
    y += 26
    o.append(f'<text x="{LX}" y="{y}" font-size="16" fill="{SUB}">축척 A: 0–450ms</text>')
    s1 = (680 - BX) / 450.0
    y0 = y + 12
    y = block(y0, *STEPS[0], s1)
    target_line(y0 - 26, y - 10, s1, "16.7ms")
    # Panel 2: steps 2..8 on a shared, 26x larger scale.
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y += 30
    o.append(f'<text x="{LX}" y="{y}" font-size="16" fill="{SUB}">축척 B: 0–17ms(축척 A의 약 26배)</text>')
    s2 = (BR - BX) / TARGET_MS
    y0 = y + 12
    yy = y0
    for st in STEPS[1:]:
        if st[1] == SPEND_FROM:
            yy += 22
            group_head(o, yy, SPEND_FROM)
            yy += 14
        yy = block(yy, *st, s2)
    target_line(y0 - 26, yy - 10, s2, "16.7ms")
    y = yy + 6
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">프레임 시간은 게임 스레드, 렌더 스레드, GPU 가운데 가장 느린 쪽(병목)이 정한다.</text>')
    hatch_key(o, y + 38)
    write("ladder.svg", svg(y + 60, o))


# ---------- 1b. resources per step ----------
# mem_total_mb   = CSV PhysicalUsedMB = process working set (CsvProfiler.cpp:4189, WindowsPlatformMemory.cpp:342)
# gpu_mem_p50    = CSV GPUMem/LocalUsedMB = DXGI local CurrentUsage of this process (RHIStats.cpp:91, DXGIUtilities.cpp:111)
# draw_calls_p50 = CSV RHI/DrawCalls per frame (GPUProfiler.cpp:2107)
RESOURCES = [("프로세스 메모리", "작업 집합, MB", "mem_total_mb", BLUE),
             ("GPU 메모리", "VRAM 사용량, MB", "gpu_mem_p50", PINK),
             ("드로우 콜", "프레임당 개수", "draw_calls_p50", TEAL)]
# Rises after step 3 named in the note, with what the step changed. Each is checked below against the run range.
RISES = [("gpu_mem_p50", [(4, "30m 안 액터"), (6, "ISKM"), (7, "Niagara 배경 무리")]),
         ("draw_calls_p50", [(4, "30m 안 액터"), (8, "액터 0~40m, 상한 1,000")])]


def run_values(cond, col):
    """Per-run values behind a summary row: r1..r10 (r0 and the A/A runs r11+ are left out, as in summarize.py)."""
    import re
    out = []
    with open(os.path.join(ROOT, "Measurements", BATCH, "runs.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            m = re.search(r"-r(\d+)$", r["label"])
            if (r["variant"] == cond and int(r["n"]) == N and r["preset"] == PRESET
                    and m and 1 <= int(m.group(1)) <= 10 and r[col]):
                out.append(float(r[col]))
    return out


def resources():
    span = {}  # (cond, col) -> (min, max) over the runs
    for cond, *_ in STEPS:
        for _, _, col, _ in RESOURCES:
            vs = run_values(cond, col)
            assert len(vs) == 3 and abs(statistics.median(vs) - val(cond, col)) < 0.01, (cond, col, vs)
            span[cond, col] = (min(vs), max(vs))
    width = lambda c, col: span[c, col][1] - span[c, col][0]
    for col, rises in RISES:  # a named rise must be larger than both run ranges (measurement.md rule)
        for num, _ in rises:
            a, b = STEPS[num - 2][0], STEPS[num - 1][0]
            assert val(b, col) - val(a, col) > max(width(a, col), width(b, col)), (col, num)
    mem_spread = max(width(c, "mem_total_mb") for c, *_ in STEPS)

    o = [HATCH]
    y = title(o, "단계마다 쓰는 메모리와 드로우 콜",
              ["호드 총수 1만, 오브 시점 카메라, 60초씩 세 번 잰 중앙값",
               "막대 둘레의 옅은 띠: 세 번 잰 범위(좁아서 2px가 안 되는 범위는 그리지 않았다)"])
    LX, CX, CR = 32, 262, W - 32
    CW = (CR - CX) / 3
    BWMAX = 92
    y += 44
    for i, (name, unit, _, c) in enumerate(RESOURCES):
        x = CX + i * CW
        o.append(f'<text x="{x:.1f}" y="{y}" font-size="17" font-weight="700" fill="{c}">{esc(name)}</text>')
        o.append(f'<text x="{x:.1f}" y="{y + 21}" font-size="16" fill="{SUB}">{esc(unit)}</text>')
    y += 38
    RH, BAR = 36, 18
    RANGE_MIN_PX = 2.0  # narrower ranges are not drawn (stated in the subtitle)
    for cond, num, name, _ in STEPS:
        if num in GROUPS:
            y += 24 if num == 1 else 30
            group_head(o, y, num)
            y += 10
        o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
        o.append(f'<text x="{LX}" y="{y + 24}" font-size="17" font-weight="700">{num}단계 {esc(name)}</text>')
        for i, (_, _, col, c) in enumerate(RESOURCES):
            x = CX + i * CW
            sc = BWMAX / val(STEPS[0][0], col)
            v = val(cond, col)
            lo, hi = span[cond, col]
            by = y + 9
            if (hi - lo) * sc >= RANGE_MIN_PX:  # pale band of the same colour behind the bar, min to max
                o.append(f'<rect x="{x + lo * sc:.1f}" y="{by - 4}" width="{(hi - lo) * sc:.1f}" height="{BAR + 8}" '
                         f'rx="4" fill="{c}" fill-opacity="0.28"/>')
                end = max(v, hi) * sc
            else:
                end = v * sc
            bar(o, x, by, max(v * sc, 1.5), BAR, c, num)
            tx = x + end + 7
            o.append(f'<text x="{tx:.1f}" y="{by + 15}" font-size="16" font-weight="700" fill="{c}">{round(v):,}</text>')
        y += RH
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y += 32
    o.append(f'<text x="32" y="{y}" font-size="16" font-weight="700">4단계부터 다시 느는 단계와 그 단계에서 바꾼 것</text>')
    for col, rises in RISES:
        y += 22
        name = next(r[0] for r in RESOURCES if r[2] == col)
        o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">{esc(name)}: '
                 + ", ".join(f"{n}단계({esc(t)})" for n, t in rises) + "</text>")
    y += 30
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">프로세스 메모리는 같은 단계에서도 실행마다 최대 약 {round(mem_spread, -1):.0f}MB 달랐다.</text>')
    hatch_key(o, y + 30)
    write("resources.svg", svg(y + 52, o))


# ---------- 2. max entities at 60 fps ----------
def max_entities():
    data = {}
    with open(os.path.join(ROOT, "Docs", "figures", BATCH.lower(), "full-n-at-60fps.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            data[r["series"].split(" ", 1)[1]] = (int(r["n_p95"]), r["note_p95"] == "above range")
    o = [HATCH]
    y = title(o, "60fps를 지키는 최대 개체 수",
              ["프레임 시간(느린 쪽 5% 경계)이 16.7ms 이하인 가장 큰 호드 총수",
               "오브 시점 카메라, 단계마다 네 가지 호드 총수에서 잰 값 사이를 보간"])
    LX, BX, BR = 32, 290, 640
    NMAX = 30000
    scale = (BR - BX) / NMAX
    y += 40
    top = y - 18
    RH = 40
    for cond, num, name, _ in STEPS:
        if num in GROUPS:
            y += 22 if num == SPEND_FROM else 0
            group_head(o, y, num)
            y += 18
            if num == 1:
                top = y - 10  # the target line starts under the first group name
        n, above = data[cond]
        o.append(f'<text x="{LX}" y="{y + 15}" font-size="17" font-weight="700">{num}단계 {esc(name)}</text>')
        w = n * scale
        if above:
            o.append(f'<rect x="{BX}" y="{y}" width="{w:.1f}" height="22" rx="3" fill="{BLUE}" fill-opacity="0.35" stroke="{BLUE}" stroke-dasharray="5 3"/>')
            o.append(f'<path d="M{BX + w + 4:.1f} {y + 1} L{BX + w + 16:.1f} {y + 11} L{BX + w + 4:.1f} {y + 21}" fill="{BLUE}"/>')
            o.append(f'<text x="{BX + w + 24:.1f}" y="{y + 17}" font-size="16" font-weight="700" fill="{BLUE}">{n // 10000}만 이상</text>')
        else:
            bar(o, BX, y, max(w, 2), 22, BLUE, num)
            o.append(f'<text x="{BX + max(w, 2) + 8:.1f}" y="{y + 17}" font-size="16" font-weight="700">{n:,}</text>')
        y += RH
    x = BX + N * scale
    o.append(f'<line x1="{x:.1f}" y1="{top}" x2="{x:.1f}" y2="{y - 14}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<text x="{x:.1f}" y="{y + 6}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">목표 1만</text>')
    y += 30
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">점선 막대: 잰 범위의 끝인 3만에서도 16.7ms 안이라 실제 최대는 더 크다.</text>')
    hatch_key(o, y + 38)
    write("max-entities.svg", svg(y + 60, o))


# ---------- 3. ablation ----------
def ablation():
    mets = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE), ("GPU", "gpu_p50_ms", PINK)]
    o = []
    base = {col: val(BASE, col) for _, col, _ in mets}
    base_bot = S[BASE]["bottleneck"]
    bot_name = {"game": "게임 스레드", "gpu": "GPU", "render": "렌더 스레드"}
    y = title(o, "최종 구성(8단계)에서 기법 하나를 뺐을 때의 변화",
              ["호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값의 차이(ms)",
               f"0은 최종 구성: 프레임 시간 {fmt(base['frame_p50_ms'])}, 게임 스레드 {fmt(base['game_p50_ms'])}, GPU {fmt(base['gpu_p50_ms'])}",
               f"최종 구성의 병목은 {bot_name[base_bot]}다. 위에서부터 프레임 시간이 많이 느려지는 순",
               "굵은 글씨는 최종 구성이 하는 일, 그 아래는 그것을 뺀 실행이 대신 하는 일"])
    y += 34
    legend(o, y, [(m[0], m[2]) for m in mets])
    y += 22
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">흐린 막대: 세 번 잰 범위보다 작은 차이(구별되지 않음)</text>')
    LX, ZX, SC = 32, 540, 30.0
    BAR, STEP = 17, 21
    y += 40
    o.append(f'<text x="{ZX - 10}" y="{y}" text-anchor="end" font-size="16" fill="{SUB}">← 빨라짐</text>')
    o.append(f'<text x="{ZX + 10}" y="{y}" font-size="16" fill="{SUB}">느려짐 →</text>')
    y += 16
    top = y
    rows = sorted(ABLATIONS, key=lambda a: -(val(a[0], "frame_p50_ms") - base["frame_p50_ms"]))
    for cond, name, lines in rows:
        diffs = {}
        for _, col, _ in mets:
            d = val(cond, col) - base[col]
            diffs[col] = (d, abs(d) > max(rng(cond, col), rng(BASE, col)))
        fd, freal = diffs["frame_p50_ms"]
        gd, greal = diffs["gpu_p50_ms"]
        tags = []
        if S[cond]["bottleneck"] != base_bot:  # e.g. without hydrated actors the GPU becomes the bottleneck
            tags.append(f"병목이 {bot_name[S[cond]['bottleneck']]}로 바뀜")
        if not freal and diffs["game_p50_ms"][1] and not greal:
            tags.append("프레임은 그대로, 게임 스레드만")
        elif greal and (not freal or (fd > 0) != (gd > 0)):
            tags.append("GPU 차이가 프레임에 드러나지 않음")
        sub = list(lines) + tags
        label(o, LX, y + 15, name, sub)
        for k in range(len(tags)):  # recolour the tag lines
            i = len(o) - 1 - k
            o[i] = o[i].replace(f'fill="{SUB}"', f'fill="{BLUE}" font-weight="700"')
        by = y + 1
        for _, col, c in mets:
            d, real = diffs[col]
            w = abs(d) * SC
            x = ZX if d >= 0 else ZX - w
            op = "1" if real else "0.3"
            o.append(f'<rect x="{x:.1f}" y="{by}" width="{max(w, 1.5):.1f}" height="{BAR}" rx="2" fill="{c}" fill-opacity="{op}"/>')
            tx = ZX + w + 6 if d >= 0 else ZX - w - 6
            anchor = "start" if d >= 0 else "end"
            o.append(f'<text x="{tx:.1f}" y="{by + 14}" text-anchor="{anchor}" font-size="16"'
                     f'{" font-weight=" + chr(34) + "700" + chr(34) if real else ""} fill="{c if real else SUB}">{fmt(d, True)}</text>')
            by += STEP
        y = max(by, y + 22 + 21 * len(sub)) + 14
    o.append(f'<line x1="{ZX}" y1="{top - 6}" x2="{ZX}" y2="{y - 10}" stroke="{INK}" stroke-width="1.5"/>')
    write("ablation.svg", svg(y + 10, o))


# ---------- 4. transition pop ----------
def transition():
    runs = {}
    with open(os.path.join(ROOT, "Measurements", "Full1", "transition-dev", "runs.csv"), encoding="utf-8-sig") as f:
        for r in csv.DictReader(f):
            if r["exit"] != "0":
                continue
            runs.setdefault(r["cond"], []).append(
                (float(r["mean_excess"]), 100.0 * int(r["noticeable"]) / int(r["events"])))
    o = []
    y = title(o, "인스턴스가 액터로 바뀌는 순간 화면이 튀는 정도",
              ["최종 구성, 호드 총수 1만, 오브 시점 카메라, 시드 세 개",
               "튐 평균: 전환 자리의 밝기 변화가 평소 움직임보다 큰 양(0–255)"])
    LX, X0, X1, V0, V1 = 32, 270, 590, 4.0, 8.0
    sc = (X1 - X0) / (V1 - V0)
    PX = 620
    y += 44
    o.append(f'<text x="{PX}" y="{y}" font-size="16" font-weight="700" fill="{SUB}">눈에 띈 전환</text>')
    y += 18
    top = y
    RH = 64
    for cond, name, lines in TRANSITIONS:
        vals = [m for m, _ in runs[cond]]
        pcts = [p for _, p in runs[cond]]
        med, lo, hi = statistics.median(vals), min(vals), max(vals)
        pl, ph = round(min(pcts)), round(max(pcts))
        cy = y + 30
        label(o, LX, y + 26, name, lines)
        o.append(f'<line x1="{X0 + (lo - V0) * sc:.1f}" y1="{cy}" x2="{X0 + (hi - V0) * sc:.1f}" y2="{cy}" stroke="{BLUE}" stroke-width="6" stroke-linecap="round" stroke-opacity="0.35"/>')
        o.append(f'<circle cx="{X0 + (med - V0) * sc:.1f}" cy="{cy}" r="7" fill="{BLUE}"/>')
        o.append(f'<text x="{X0 + (med - V0) * sc:.1f}" y="{cy - 14}" text-anchor="middle" font-size="16" font-weight="700" fill="{BLUE}">{fmt(med)}</text>')
        o.append(f'<text x="{X0 + (med - V0) * sc:.1f}" y="{cy + 26}" text-anchor="middle" font-size="16" fill="{SUB}">{fmt(lo)}–{fmt(hi)}</text>')
        pct = f"{pl}%" if pl == ph else f"{pl}–{ph}%"
        o.append(f'<text x="{PX}" y="{cy + 6}" font-size="17" font-weight="700">{pct}</text>')
        y += RH + 14
    # axis
    o.append(f'<line x1="{X0}" y1="{y}" x2="{X1}" y2="{y}" stroke="{SUB}"/>')
    for t in range(int(V0), int(V1) + 1):
        x = X0 + (t - V0) * sc
        o.append(f'<line x1="{x:.1f}" y1="{top - 6}" x2="{x:.1f}" y2="{y}" stroke="{LINE}"/>')
        o.append(f'<text x="{x:.1f}" y="{y + 22}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')
    y += 50
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">점: 세 시드의 중앙값. 막대와 아래 숫자: 시드 사이 범위. 가로축은 4에서 시작한다.</text>')
    write("transition.svg", svg(y + 26, o))


# ---------- 4b. entity pool over a long run ----------
# Measurements/Pool1/timeline.csv: one 960 s run each (Scripts/run-pool.ps1), 1-minute bins (Scripts/pool-timeline.py).
# Pool on = L8-q4final arguments, pool off = A8-pool arguments (-LabEntityPool=0). One run each, so no run range.
POOL_RUNS = [("L8-q4final", "풀 켬(최종 구성)", BLUE, ""), ("A8-pool", "풀 끔", PINK, "6 4")]
POOL_PANELS = [("iskm_tracks_end", "ISKM 트랙 수", "개", 40000),
               ("gpu_mem_mean", "GPU 메모리", "MB", 1000),
               ("frame_p50", "프레임 시간 중앙값", "ms", 1)]


def pool():
    data = {}
    with open(os.path.join(ROOT, "Measurements", "Pool1", "timeline.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            for cond, *_ in POOL_RUNS:
                if f"-{cond}-N{N}-{PRESET}-" in r["label"]:
                    data.setdefault(cond, []).append(r)
    for cond in data:
        data[cond].sort(key=lambda r: int(r["minute"]))
    minutes = [int(r["minute"]) for r in data[POOL_RUNS[0][0]]]
    m0, m1 = minutes[0], minutes[-1]
    o = []
    y = title(o, "엔티티 풀을 끄면 오래 돌수록 느려진다",
              [f"최종 구성, 호드 총수 1만, 오브 시점 카메라, {m1}분 실행 한 번씩",
               "1분 구간마다의 값. 실행이 한 번이라 범위는 없다"])
    y += 34
    legend(o, y, [(name, c) for _, name, c, _ in POOL_RUNS])
    PX0, PX1, PH, GAP = 120, W - 150, 110, 64
    sx = (PX1 - PX0) / (m1 - m0)
    y += 40
    for col, name, unit, step in POOL_PANELS:
        vals = [float(r[col]) for cond, *_ in POOL_RUNS for r in data[cond]]
        hi = max(vals)
        lo = 0.0 if col == "iskm_tracks_end" else min(vals) * 0.9
        lo = step * int(lo // step)
        top = step * (int(hi // step) + 1)
        sy = PH / (top - lo)
        o.append(f'<text x="32" y="{y}" font-size="17" font-weight="700">{esc(name)}</text>'
                 f'<text x="{32 + len(name) * 17 + 10}" y="{y}" font-size="16" fill="{SUB}">({unit})</text>')
        y0 = y + 16
        base = y0 + PH
        t = lo
        while t <= top + 1e-9:
            ty = base - (t - lo) * sy
            o.append(f'<line x1="{PX0}" y1="{ty:.1f}" x2="{PX1}" y2="{ty:.1f}" stroke="{LINE}"/>')
            lab = f"{t / 10000:g}만" if col == "iskm_tracks_end" and t else f"{t:,.0f}"
            o.append(f'<text x="{PX0 - 8}" y="{ty + 5:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{lab}</text>')
            t += step
        for cond, rname, c, dash in POOL_RUNS:
            pts = " ".join(f"{PX0 + (int(r['minute']) - m0) * sx:.1f},{base - (float(r[col]) - lo) * sy:.1f}" for r in data[cond])
            da = f' stroke-dasharray="{dash}"' if dash else ""
            o.append(f'<polyline points="{pts}" fill="none" stroke="{c}" stroke-width="2.5"{da}/>')
            last = float(data[cond][-1][col])
            ly = base - (last - lo) * sy
            txt = f"{last:,.0f}" if col != "frame_p50" else f"{last:.2f}"
            o.append(f'<circle cx="{PX1:.1f}" cy="{ly:.1f}" r="4" fill="{c}"/>')
            o.append(f'<text x="{PX1 + 10}" y="{ly + 5:.1f}" font-size="16" font-weight="700" fill="{c}">{txt}</text>')
        y = base + GAP
    # minute axis under the last panel
    ay = y - GAP + 24
    for mm in range(m0, m1 + 1):
        if mm in (m0, m1) or mm % 4 == 0:
            o.append(f'<text x="{PX0 + (mm - m0) * sx:.1f}" y="{ay}" text-anchor="middle" font-size="16" fill="{SUB}">{mm}분</text>')
    y = ay + 32
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    off0, off1 = data["A8-pool"][0], data["A8-pool"][-1]
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">오른쪽 숫자: {m1}분째 값. 풀 끔의 1분째는 트랙 {float(off0["iskm_tracks_end"]):,.0f}, '
             f'GPU 메모리 {float(off0["gpu_mem_mean"]):,.0f}MB, 프레임 {float(off0["frame_p50"]):.2f}ms</text>')
    o.append(f'<text x="32" y="{y + 36}" font-size="16" fill="{SUB}">60초 측정에서는 풀 켬과 풀 끔이 구별되지 않았다.</text>')
    write("entity-pool.svg", svg(y + 60, o))


# ---------- 5. scenario (top-down sketch) ----------
def scenario():
    import math, random
    o = ['<defs>'
         f'<marker id="arr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{SUB}"/></marker>'
         f'<marker id="arrb" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{BLUE}"/></marker>'
         '</defs>']
    y = title(o, "시나리오: 위에서 내려다본 맵",
              ["사방에서 오브로 걸어오는 호드. 플레이어 캐릭터는 없고 카메라는 고정 위치 둘 중 하나다"])

    def pol(cx, cy, r, deg):  # screen angle: 0 = right, 90 = up
        a = math.radians(deg)
        return cx + r * math.cos(a), cy - r * math.sin(a)

    # ---- Panel A: the whole map to scale ----
    CX, CY, RP = 262, y + 272, 220
    s = RP / MAP_R
    VIEW = 60   # P1 looks this way in the sketch (it turns once a minute)
    o.append(f'<text x="32" y="{y + 42}" font-size="17" font-weight="700">맵 전체(반경 {MAP_R}m)</text>')
    o.append(f'<circle cx="{CX}" cy="{CY}" r="{RP}" fill="{CARD}" stroke="{GRAY}" stroke-width="1.5"/>')
    # P1 field of view (horizontal), from the orb outward
    a0, a1 = VIEW - FOV_H / 2, VIEW + FOV_H / 2
    x0, y0 = pol(CX, CY, RP, a0)
    x1, y1 = pol(CX, CY, RP, a1)
    o.append(f'<path d="M{CX} {CY} L{x0:.1f} {y0:.1f} A{RP} {RP} 0 0 0 {x1:.1f} {y1:.1f} Z" fill="{BLUE}" fill-opacity="0.10" stroke="{BLUE}" stroke-opacity="0.5" stroke-dasharray="4 4"/>')
    # respawn ring
    rr = (RESPAWN_IN + MAP_R) / 2 * s
    o.append(f'<circle cx="{CX}" cy="{CY}" r="{rr:.1f}" fill="none" stroke="{AMBER}" stroke-opacity="0.45" stroke-width="{(MAP_R - RESPAWN_IN) * s:.1f}"/>')
    # dots: radius uniform in [ZONE_R, MAP_R] -> density ~ 1/r, every LARGE_EVERY-th is large
    rnd = random.Random(SEED)
    dots = []
    for i in range(DOTS):
        r = rnd.uniform(ZONE_R, MAP_R)
        a = rnd.uniform(0, 360)
        dots.append((i, r, a))
    for i, r, a in dots:
        x, yy = pol(CX, CY, r * s, a)
        if i % LARGE_EVERY == 0:
            o.append(f'<circle cx="{x:.1f}" cy="{yy:.1f}" r="3.2" fill="{DARK}"/>')
        else:
            o.append(f'<circle cx="{x:.1f}" cy="{yy:.1f}" r="1.8" fill="{GRAY}"/>')
    # snipe rings and shots: in view, per distance band, the first dots in seeded order stand in for the smallest hash
    for e in SNIPE_EDGES:
        o.append(f'<circle cx="{CX}" cy="{CY}" r="{e * s:.1f}" fill="none" stroke="{SUB}" stroke-width="1" stroke-dasharray="3 3"/>')
    edges = list(SNIPE_EDGES) + [MAP_R + 1]
    for b, k in enumerate(SNIPE_COUNTS):
        picks = [d for d in dots if edges[b] <= d[1] < edges[b + 1] and a0 + 6 < d[2] < a1 - 6][:k]
        for _, r, a in picks:
            x, yy = pol(CX, CY, r * s, a)
            o.append(f'<line x1="{CX}" y1="{CY}" x2="{x:.1f}" y2="{yy:.1f}" stroke="{PINK}" stroke-width="1.6"/>')
            o.append(f'<circle cx="{x:.1f}" cy="{yy:.1f}" r="5" fill="none" stroke="{PINK}" stroke-width="2"/>')
    # walking in
    for a in (165, 215, 260, 320):
        xa, ya = pol(CX, CY, 455 * s, a)
        xb, yb = pol(CX, CY, 330 * s, a)
        o.append(f'<line x1="{xa:.1f}" y1="{ya:.1f}" x2="{xb:.1f}" y2="{yb:.1f}" stroke="{SUB}" stroke-width="2" marker-end="url(#arr)"/>')
    # orb
    o.append(f'<circle cx="{CX}" cy="{CY}" r="5" fill="{PURPLE}"/>')
    # ruler under the disc
    ry = CY + RP + 26
    o.append(f'<line x1="{CX}" y1="{ry}" x2="{CX + RP}" y2="{ry}" stroke="{SUB}"/>')
    ticks = [(0, 1), (SNIPE_EDGES[0], 2), (SNIPE_EDGES[1], 1), (SNIPE_EDGES[2], 1), (MAP_R, 1)]
    for d, row in ticks:
        x = CX + d * s
        o.append(f'<line x1="{x:.1f}" y1="{ry - 5}" x2="{x:.1f}" y2="{ry + 5 + (row - 1) * 18}" stroke="{SUB}"/>')
        t = f"{d}m" if d == MAP_R else str(d)
        o.append(f'<text x="{x:.1f}" y="{ry + 22 + (row - 1) * 18}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')
    o.append(f'<text x="{CX - 12}" y="{ry + 5}" text-anchor="end" font-size="16" fill="{SUB}">오브에서 거리</text>')

    # legend column
    LX = 510
    ly = y + 82

    def item(mark, head, lines):
        nonlocal ly
        o.append(mark(LX + 10, ly - 5))
        o.append(f'<text x="{LX + 28}" y="{ly}" font-size="16" font-weight="700">{esc(head)}</text>')
        for j, ln in enumerate(lines):
            o.append(f'<text x="{LX + 28}" y="{ly + 21 * (j + 1)}" font-size="16" fill="{SUB}">{esc(ln)}</text>')
        ly += 21 * (len(lines) + 1) + 14

    item(lambda x, yy: f'<circle cx="{x}" cy="{yy}" r="2.5" fill="{GRAY}"/>', "근접 미니언 90%", [])
    item(lambda x, yy: f'<circle cx="{x}" cy="{yy}" r="4" fill="{DARK}"/>', "시즈 미니언 10%",
         [f"둘 다 {SPEED}m/s로 오브에 직진", f"체력은 개체마다 {HEALTH[0]}–{HEALTH[1]}"])
    item(lambda x, yy: f'<circle cx="{x}" cy="{yy}" r="5" fill="{PURPLE}"/>', "오브(맵 중앙)", [])
    item(lambda x, yy: f'<g><line x1="{x - 8}" y1="{yy + 5}" x2="{x + 6}" y2="{yy - 6}" stroke="{PINK}" stroke-width="2"/>'
                       f'<circle cx="{x + 6}" cy="{yy - 6}" r="4" fill="none" stroke="{PINK}" stroke-width="2"/></g>',
         f"저격: {SNIPE_EVERY}초마다 {SNIPE_K}명", ["화면 안의 개체만",
                                      f"{SNIPE_EDGES[0]}–{SNIPE_EDGES[1]}m {SNIPE_COUNTS[0]}명, {SNIPE_EDGES[1]}–{SNIPE_EDGES[2]}m {SNIPE_COUNTS[1]}명,",
                                      f"{SNIPE_EDGES[2]}m 밖 {SNIPE_COUNTS[2]}명"])
    item(lambda x, yy: f'<rect x="{x - 8}" y="{yy - 8}" width="16" height="16" rx="3" fill="{BLUE}" fill-opacity="0.15" stroke="{BLUE}" stroke-dasharray="3 3"/>',
         f"오브 시점 화면(가로 {FOV_H}°)", ["1분에 한 바퀴 돈다"])
    item(lambda x, yy: f'<rect x="{x - 8}" y="{yy - 4}" width="16" height="8" fill="{AMBER}" fill-opacity="0.45"/>',
         f"재생성 링 {RESPAWN_IN}–{MAP_R}m", ["죽은 만큼 여기서 다시 생겨", "총수가 그대로다"])
    o.append(f'<text x="{LX}" y="{ly + 6}" font-size="16" fill="{SUB}">점은 처음 배치: 거리마다 같은 수라</text>')
    o.append(f'<text x="{LX}" y="{ly + 27}" font-size="16" fill="{SUB}">밀도가 1/거리로 준다</text>')

    # ---- Panel B: the middle, enlarged ----
    y = ry + 70
    o.append(f'<line x1="32" y1="{y - 24}" x2="{W - 32}" y2="{y - 24}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 6}" font-size="17" font-weight="700">가운데 확대(오브 둘레 20m)</text>')
    BX, BY, SB = 400, y + 170, 6.0  # px per metre
    o.append(f'<circle cx="{BX}" cy="{BY}" r="{20 * SB}" fill="{CARD}" stroke="{GRAY}" stroke-width="1.5"/>')
    # P1 view cone from the camera, 15 m behind, looking through the orb
    CAMX, CAMY = BX - P1_BACK * SB, BY
    cone = 20 * SB + P1_BACK * SB
    cx0, cy0 = pol(CAMX, CAMY, cone, -FOV_H / 2)
    cx1, cy1 = pol(CAMX, CAMY, cone, FOV_H / 2)
    o.append(f'<clipPath id="zc"><circle cx="{BX}" cy="{BY}" r="{20 * SB}"/></clipPath>')
    o.append(f'<path clip-path="url(#zc)" d="M{CAMX} {CAMY} L{cx0:.1f} {cy0:.1f} L{cx1:.1f} {cy1:.1f} Z" fill="{BLUE}" fill-opacity="0.10" stroke="{BLUE}" stroke-opacity="0.5" stroke-dasharray="4 4"/>')
    # P1 orbit
    o.append(f'<circle cx="{BX}" cy="{BY}" r="{P1_BACK * SB}" fill="none" stroke="{BLUE}" stroke-width="1.5" stroke-dasharray="6 5"/>')
    ox, oy = pol(BX, BY, P1_BACK * SB, 145)  # yaw grows clockwise seen from above (UE is left-handed)
    o.append(f'<path d="M{CAMX} {CAMY} A{P1_BACK * SB} {P1_BACK * SB} 0 0 1 {ox:.1f} {oy:.1f}" fill="none" stroke="{BLUE}" stroke-width="2.5" marker-end="url(#arrb)"/>')
    # damage zone and orb
    o.append(f'<circle cx="{BX}" cy="{BY}" r="{ZONE_R * SB}" fill="{PURPLE}" fill-opacity="0.18" stroke="{PURPLE}" stroke-width="1.5"/>')
    o.append(f'<circle cx="{BX}" cy="{BY}" r="6" fill="{PURPLE}"/>')
    # top-down camera straight above the orb
    o.append(f'<circle cx="{BX}" cy="{BY}" r="11" fill="none" stroke="{TEAL}" stroke-width="2.5"/>')
    # minions walking into the zone
    for a, r0 in ((75, 18.5), (110, 17), (300, 18), (255, 16.5), (20, 17.5)):
        xa, ya = pol(BX, BY, r0 * SB, a)
        xb, yb = pol(BX, BY, (r0 - 5) * SB, a)
        o.append(f'<line x1="{xa:.1f}" y1="{ya:.1f}" x2="{xb:.1f}" y2="{yb:.1f}" stroke="{SUB}" stroke-width="1.5" marker-end="url(#arr)"/>')
        o.append(f'<circle cx="{xa:.1f}" cy="{ya:.1f}" r="3.5" fill="{GRAY}"/>')
    # camera icon
    o.append(f'<rect x="{CAMX - 9}" y="{CAMY - 7}" width="18" height="14" rx="2" fill="{BLUE}"/>')
    o.append(f'<path d="M{CAMX + 9} {CAMY - 4} L{CAMX + 15} {CAMY - 8} L{CAMX + 15} {CAMY + 8} L{CAMX + 9} {CAMY + 4} Z" fill="{BLUE}"/>')
    # labels, left: P1 camera
    label_l = [("오브 시점 카메라", BLUE), (f"오브 뒤 {P1_BACK}m, 높이 {P1_HEIGHT}m", SUB),
               ("오브 너머 바깥을 보며", SUB), ("오브 둘레를 돈다", SUB)]
    for j, (t, c) in enumerate(label_l):
        o.append(f'<text x="32" y="{BY - 40 + 21 * j}" font-size="16"{" font-weight=" + chr(34) + "700" + chr(34) if j == 0 else ""} fill="{c}">{esc(t)}</text>')
    o.append(f'<line x1="196" y1="{BY - 4}" x2="{CAMX - 14}" y2="{CAMY}" stroke="{BLUE}" stroke-width="1"/>')
    # labels, right: zone and top-down camera
    RX = 548
    o.append(f'<text x="{RX}" y="{BY - 70}" font-size="16" font-weight="700" fill="{PURPLE}">피해 범위 {ZONE_R}m</text>')
    o.append(f'<text x="{RX}" y="{BY - 49}" font-size="16" fill="{SUB}">초당 {ZONE_DPS[0]}(가장자리)–</text>')
    o.append(f'<text x="{RX}" y="{BY - 28}" font-size="16" fill="{SUB}">{ZONE_DPS[1]}(중심) 피해</text>')
    zx, zy = pol(BX, BY, ZONE_R * SB, 35)
    o.append(f'<line x1="{RX - 6}" y1="{BY - 75}" x2="{zx:.1f}" y2="{zy:.1f}" stroke="{PURPLE}" stroke-width="1"/>')
    o.append(f'<text x="{RX}" y="{BY + 34}" font-size="16" font-weight="700" fill="{TEAL}">탑뷰 카메라</text>')
    o.append(f'<text x="{RX}" y="{BY + 55}" font-size="16" fill="{SUB}">오브 바로 위 {P2_HEIGHT}m에서</text>')
    o.append(f'<text x="{RX}" y="{BY + 76}" font-size="16" fill="{SUB}">곧장 내려다본다</text>')
    o.append(f'<line x1="{RX - 6}" y1="{BY + 29}" x2="{BX + 10}" y2="{BY + 5}" stroke="{TEAL}" stroke-width="1"/>')
    # ruler
    ry = BY + int(20 * SB) + 26
    o.append(f'<line x1="{BX}" y1="{ry}" x2="{BX + 20 * SB}" y2="{ry}" stroke="{SUB}"/>')
    for d in (0, ZONE_R, P1_BACK, 20):
        x = BX + d * SB
        o.append(f'<line x1="{x:.1f}" y1="{ry - 5}" x2="{x:.1f}" y2="{ry + 5}" stroke="{SUB}"/>')
        o.append(f'<text x="{x:.1f}" y="{ry + 22}" text-anchor="middle" font-size="16" fill="{SUB}">{d}{"m" if d == 20 else ""}</text>')
    o.append(f'<text x="{BX - 12}" y="{ry + 5}" text-anchor="end" font-size="16" fill="{SUB}">오브에서 거리</text>')
    y = ry + 44
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 28}" font-size="16" fill="{SUB}">두 그림 모두 거리는 축척대로다. 개체 점은 실제 수보다 적게 그렸다.</text>')
    write("scenario.svg", svg(y + 50, o))


if __name__ == "__main__":
    scenario()
    ladder()
    resources()
    max_entities()
    ablation()
    transition()
    pool()
    print("wrote", ", ".join(sorted(os.listdir(OUT))), "to", os.path.relpath(OUT, ROOT))
