# Makes the SVG figures of the root README in Docs/figures/readme/.
# Run from anywhere: py -3 -I Scripts/plot-readme.py
# Measured values come from the latest full batch, except the entity pool figure, which reads a reference batch and is never
# joined to full-batch values in one figure. The figures name neither batch nor condition label (AGENTS.md "문서와 글").
#   Measurements/Full4/summary.csv     ladder, resources, technique contributions, round trips (horde 10000, orb camera P1)
#   Measurements/Full4/runs.csv        run ranges of the resource figure: the runs listed in summary.csv runs_used (r5 r6 r21 ...)
#   Measurements/Pool2/timeline.csv    entity pool on (s8) and off (a8pool) over a 960 s window, one run each, 1-minute bins
#   Scripts/full-conditions.json       step roles, condition kinds and groups, warm-up and window, equivalence limits
# "Distinguishable" is plot.py's distinguishable(): the batch figures and these figures use one rule (measurement.md "차이 판정").
# Words follow Docs/Guides/glossary.md. Colours and fonts are the posts' (posting.md "시각 자료").
import csv
import math
import os
import random
import re
import statistics
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import batch_rules  # noqa: E402  base_run(): original runs and A/A runs
import lab_conditions as lc  # noqa: E402
import plot  # noqa: E402  distinguishable(), BUDGET_MS, FULL_ABL_MIXED

OUT = os.path.join(ROOT, "Docs", "figures", "readme")
BATCH = "Full4"          # latest full batch (measurement.md "총괄 묶음 규약")
POOL_BATCH = "Pool2"     # entity pool long run (measurement.md "엔티티 풀 묶음 규약"), a reference batch
N, PRESET, SPEED = 10000, "P1", "V0"
BUDGET_MS = plot.BUDGET_MS  # 16.7 ms, the line and the "16.7ms까지의 여유" of the glossary

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, TEAL, PINK, AMBER, DARK, GRAY = "#2563eb", "#0d9488", "#db2777", "#b45309", "#374151", "#9ca3af"
PURPLE = "#9333ea"   # same as the post figures ("끄면 짧아진다")
RED = "#dc2626"      # same as the post figures (entity pool off)
W = 800

DOC = lc.load()
CFGS = lc.configs(DOC)

# Public step names (glossary 5.1, v2-plan.md "사다리") and one line of what the step changed. Roles come from the condition table.
STEP_TEXT = {
    1: ("액터 기준선", "ACharacter 액터, 캡슐과 이동 컴포넌트"),
    2: ("Mass, 엔진 기본 경로", "엔진 이동·회피·표현, 가까운 개체는 액터"),
    3: ("직접 짠 분리", "분리 프로세서, 방향 보간 병렬 사본"),
    4: ("애니메이션 표현", "VAT 인스턴스, Animation Budget Allocator, 최소 유지 시간"),
    5: ("Simulation LOD + 엔티티 풀", "먼 개체의 로직을 드물게, 엔티티를 다시 쓴다"),
    6: ("ISKM", "35~200m를 ISKM으로, 고정 바운드"),
    7: ("배경 무리", "200m 밖을 Niagara 메시 입자로"),
    8: ("남은 비용 줄이기", "80m 밖 그림자 끔, 액터 메시 고정 바운드"),
}
STEPS = [(int(k), v["role"]) for k, v in DOC["steps"].items()]
assert [n for n, _ in STEPS] == sorted(STEP_TEXT), STEPS
ROLE_TEXT = {"start": "출발점", "earn": "비용을 줄이는 단계", "quality": "표현을 더하는 단계"}  # glossary 1.3. Keys are condition-table identifiers
HATCHED = {n for n, role in STEPS if role == "quality"}  # steps 4, 6 and 7 get diagonal stripes

# Technique contributions: row names "~를 끈 조건" (glossary 5.2). a8shd turns a technique back on; s8av replaces one.
ABL_TEXT = {
    "a8hyd": "하이드레이션을 끈 조건", "a8bud": "Animation Budget Allocator를 끈 조건", "a8fix": "전환 대책을 끈 조건",
    "a8hold": "최소 유지 시간을 끈 조건", "a8pool": "엔티티 풀을 끈 조건", "a8sim": "Simulation LOD를 끈 조건",
    "a8iskm": "ISKM을 끈 조건", "a8bnd": "ISKM 고정 바운드를 끈 조건", "a8far": "200m 밖을 그리지 않는 일을 끈 조건",
    "a8fx": "배경 무리를 끈 조건", "a8sep": "겹침 방지(분리 프로세서)를 끈 조건", "a8cap": "액터 캡슐을 끈 조건",
    "a8shd": "80m 밖 그림자를 다시 켠 조건", "a8fsb": "액터 메시 고정 바운드를 끈 조건", "a8res": "ISKM 트랙 예약을 끈 조건",
    "a8fxf": "입자의 오브 따라가기를 끈 조건", "a8stp": "ISKM 바운드 상자 따라가기를 끈 조건",
    "s8av": "분리 프로세서 대신 엔진 회피를 쓴 조건",
}
# Rows that remove more than one thing (measurement.md "변수가 둘 이상 섞인 행"). Same keys as plot.py's FULL_ABL_MIXED.
ABL_MIXED = {
    "a8hyd": "하이드레이션을 끄면 액터 캡슐, Animation Budget Allocator, 액터 메시 고정 바운드도 함께 빠진다.",
    "a8iskm": "ISKM을 끄면 ISKM 고정 바운드, 트랙 예약, 200m 밖에서 맞은 개체를 ISKM으로 그리는 규칙도 함께 빠진다.",
    "a8fx": "배경 무리를 끄면 200m 밖에서 맞은 개체를 ISKM으로 그리는 규칙과 맵 밖 무리의 그리기도 함께 빠진다.",
}
assert set(ABL_MIXED) == set(plot.FULL_ABL_MIXED), "keep the mixed rows the same as Scripts/plot.py"
TRACK_NOTE_RATIO = 1.2  # a row notes its ISKM track count only when it ends this much above the final configuration (a leak; fewer tracks are not noted)
BOT_NAME = {"game": "게임 스레드", "render": "렌더 스레드", "gpu": "GPU"}

# Scenario figure (scenario.svg). Not measured values: the fixed scenario in the source, in metres and seconds.
MAP_R = 500            # LabConfig.h FLabScenario::MapRadius (50000 cm)
FAR_OUT = 1000         # FLabCrowdFx::OuterRadius (100000 cm): off-map records live in 500-1,000 m (ADR-0020, LabFarStream.cpp)
ZONE_R = 8             # FLabScenario::ZoneRadius (800 cm); also the inner edge of the initial spawn radius (LabCrowdSpawner.cpp SpawnInitial)
ZONE_DPS = (100, 300)  # FLabScenario::ZoneDpsEdge, ZoneDpsCenter
HEALTH = (60, 240)     # FLabScenario::HealthMin, HealthMax
SPEED_MPS = 3          # FLabScenario::MoveSpeed (300 cm/s); off-map records move at the same speed
ORBIT_R, ORBIT_V, ORB_H = 57.3, 2, 6   # FLabScenario::OrbitRadius, OrbitSpeed, OrbHeight; FLabOrbit::LocationAt turns clockwise seen from above
SNIPE_EVERY, SNIPE_K = 2, 5            # FLabScenario::SnipeInterval, SnipeCount
SNIPE_EDGES = (30, 80, 200)            # LabSnipe.h FLabSnipeBands::Edges, horizontal distance from the orb (last edge is the map radius)
SNIPE_COUNTS = (1, 2, 2)               # FLabSnipeBands::Counts
LARGE_EVERY = 10       # LabUnitAssets.h FLabUnitTypes::IsLarge: serial % 10 == 0 -> 10 % large
P1_BACK, P1_HEIGHT, P1_PERIOD = 15, 9, 60  # FLabScenario::OrbViewBack, OrbViewHeight, OrbViewPeriod (s per turn); LabCameraRig.cpp Apply follows the orb
P2_HEIGHT = 100        # FLabScenario::TopDownHeight (10000 cm), straight down over the orb (LabCameraRig.cpp Apply)
FOV_H = 90             # engine default UCameraComponent::FieldOfView (CameraComponent.cpp:78); ALabCameraRig keeps it
SEED = 20261106        # FLabConfig::Seed; only seeds the drawn dots here
DOTS, FAR_DOTS = 520, 260  # dots drawn (illustration, not N)
B_R = 40               # radius of the enlarged panel around the orb (m)


def cond(cfg):
    return f"{SPEED}-{cfg}"


def load_summary():
    rows = {}
    with open(os.path.join(ROOT, "Measurements", BATCH, "summary.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if int(r["n"]) == N and r["preset"] == PRESET:
                rows[r["variant"]] = r
    return rows


S = load_summary()


def val(c, col):
    return float(S[c][col])


def _no_zero_decimals(f):
    """27.0 -> 27. A value whose decimals are all zero is written as an integer."""
    return lambda *a, **k: re.sub(r"[.]0+$", "", f(*a, **k))


@_no_zero_decimals
def fmt(x, signed=False):
    """Three significant figures."""
    from decimal import Decimal, ROUND_HALF_UP
    a = Decimal(repr(round(abs(x), 6)))  # half up from the decimal text (1.125 -> 1.13), as in the posts
    s = str(a.quantize(Decimal("1" if a >= 100 else "0.1" if a >= 10 else "0.01"), rounding=ROUND_HALF_UP))
    if float(s) == 0:
        return s
    if signed:
        return ("+" if x > 0 else "−" if x < 0 else "±") + s
    return ("−" if x < 0 else "") + s


def svg(h, body):
    h = int(math.ceil(h))
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


def halo(x, y, text, size=16, fill=INK, anchor="start", weight=None):
    """Text with a white outline first, so a line crossing it stays readable."""
    a = f'x="{x:.1f}" y="{y:.1f}" font-size="{size}" text-anchor="{anchor}"' + (f' font-weight="{weight}"' if weight else "")
    return (f'<text {a} fill="white" stroke="white" stroke-width="4" stroke-linejoin="round">{esc(text)}</text>'
            f'<text {a} fill="{fill}">{esc(text)}</text>')


def text_w(t, size=16):
    """Rough text width for layout (Malgun Gothic, measured on Edge renders): Hangul 1 em, space 0.3 em, other ASCII 0.55 em."""
    return sum(size * (1.0 if ord(ch) > 0x2000 else 0.3 if ch == " " else 0.55) for ch in t)


def wrap(t, maxw, size=16, indent=""):
    """Splits a sentence at spaces so that each line is at most maxw px wide (text_w). Later lines start with indent."""
    lines, cur = [], ""
    for word in t.split(" "):
        cand = word if not cur else cur + " " + word
        if cur and text_w(cand, size) > maxw:
            lines.append(cur)
            cur = indent + word
        else:
            cur = cand
    return lines + [cur]


def foot_lines(o, y, texts, maxw=W - 64):
    """Writes footnote sentences under a figure, wrapped to the figure width. Returns the y after the last line."""
    for t in texts:
        for ln in wrap(t, maxw, indent="  " if t.startswith("†") else ""):
            o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">{esc(ln)}</text>')
            y += 23
    return y


# Representation steps (4, 6, 7) get diagonal stripes on the same colour (not colour alone).
HATCH = ('<defs><pattern id="rep" patternUnits="userSpaceOnUse" width="7" height="7" patternTransform="rotate(45)">'
         '<rect width="3" height="7" fill="#ffffff" fill-opacity="0.55"/></pattern></defs>')


def bar(o, x, y, w, h, c, num, rx=3, opacity=1.0):
    op = "" if opacity == 1.0 else f' fill-opacity="{opacity}"'
    o.append(f'<rect x="{x:.1f}" y="{y}" width="{w:.1f}" height="{h}" rx="{rx}" fill="{c}"{op}/>')
    if num in HATCHED:
        o.append(f'<rect x="{x:.1f}" y="{y}" width="{w:.1f}" height="{h}" rx="{rx}" fill="url(#rep)"/>')


def hatch_key(o, y):
    nums = "·".join(str(n) for n in sorted(HATCHED))
    o.append(f'<rect x="32" y="{y - 13}" width="34" height="16" rx="3" fill="{GRAY}"/>')
    o.append(f'<rect x="32" y="{y - 13}" width="34" height="16" rx="3" fill="url(#rep)"/>')
    o.append(f'<text x="74" y="{y}" font-size="16" fill="{SUB}">빗금 막대: {ROLE_TEXT["quality"]}(단계 {nums}). '
             f'나머지는 {ROLE_TEXT["earn"]}와 {ROLE_TEXT["start"]}(단계 1)이다</text>')


def run_values(c, col):
    """Per-run values behind a summary row: the runs summarize.py used (summary.csv runs_used, e.g. r5 r6 r21).

    Reading runs_used keeps the second retries (run 21 and up) and leaves out the A/A runs (11-13) and replaced slow runs.
    Every used run must be an original run or its rerun (batch_rules.base_run 1..9), never an A/A run.
    """
    used = S[c]["runs_used"].split()
    assert used and all(1 <= batch_rules.base_run(u[1:]) <= batch_rules.RERUN_RUN_MAX for u in used), (c, used)
    out = []
    with open(os.path.join(ROOT, "Measurements", BATCH, "runs.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if (r["variant"] == c and int(r["n"]) == N and r["preset"] == PRESET
                    and r["label"].rsplit("-", 1)[1] in used and r[col]):
                out.append(float(r[col]))
    assert len(out) == len(used), (c, col, used, out)
    return out


# ---------- 1. ladder ----------
def ladder():
    o = [HATCH]
    y = title(o, "단계별 프레임 시간",
              ["호드 총수 1만, 이동 장면, 오브 시점 카메라, 세 번 잰 중앙값(ms)"])
    y += 34
    # legend: thick bar p95, thin bar p50, the 16.7 ms line
    o.append(f'<rect x="32" y="{y - 13}" width="22" height="16" rx="2" fill="{DARK}"/>')
    o.append(f'<text x="62" y="{y}" font-size="16">굵은 막대: 프레임 p95(느린 쪽 5%의 경계)</text>')
    o.append(f'<rect x="420" y="{y - 8}" width="22" height="7" rx="2" fill="{DARK}" fill-opacity="0.45"/>')
    o.append(f'<text x="450" y="{y}" font-size="16">가는 막대: 프레임 p50(중앙값)</text>')
    y += 26
    o.append(f'<line x1="32" y1="{y - 5}" x2="54" y2="{y - 5}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<text x="62" y="{y}" font-size="16">16.7ms(60fps). 넘지 않는 단계는 오른쪽에 16.7ms까지의 여유를 적었다</text>')
    X0, X1 = 32, 680
    RH = 72
    panels = ((700.0, 100, [n for n, _ in STEPS if n == 1]), (40.0, 5, [n for n, _ in STEPS if n != 1]))
    prev = None
    over = []
    for pi, (axis_max, tick, nums) in enumerate(panels):
        sc = (X1 - X0) / axis_max
        y += 38
        o.append(f'<line x1="32" y1="{y - 22}" x2="{W - 32}" y2="{y - 22}" stroke="{LINE}"/>')
        span = f"단계 {nums[0]}" if len(nums) == 1 else f"단계 {nums[0]}–{nums[-1]}"
        extra = "" if pi == 0 else f", 축척 A의 {fmt(panels[0][0] / axis_max)}배"
        o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">축척 {"AB"[pi]}: 0–{axis_max:g}ms({span}{extra})</text>')
        y += 10
        top = y + 8
        bottom = top + RH * len(nums)
        for k in range(int(axis_max // tick) + 1):
            gx = X0 + k * tick * sc
            o.append(f'<line x1="{gx:.1f}" y1="{top + 22}" x2="{gx:.1f}" y2="{bottom - 10}" stroke="#eef0f3"/>')
            o.append(f'<text x="{gx:.1f}" y="{bottom + 8}" font-size="16" fill="{SUB}" text-anchor="middle">{k * tick:g}</text>')
        lx = X0 + BUDGET_MS * sc
        ry = top
        for num in nums:
            c = cond(f"s{num}")
            name, detail = STEP_TEXT[num]
            p95, p50 = val(c, "frame_p95_ms"), val(c, "frame_p50_ms")
            o.append(f'<text x="32" y="{ry + 16}" font-size="17" font-weight="700">단계 {num} {esc(name)}'
                     f'<tspan font-size="16" font-weight="400" fill="{SUB}">  {esc(detail)}</tspan></text>')
            w95, w50 = min(p95, axis_max) * sc, min(p50, axis_max) * sc
            bar(o, X0, ry + 24, w95, 22, DARK, num)
            bar(o, X0, ry + 50, w50, 7, DARK, num, rx=2, opacity=0.45)
            # the 16.7 ms line runs through the bars only, not through the step names
            o.append(f'<line x1="{lx:.1f}" y1="{ry + 20}" x2="{lx:.1f}" y2="{ry + 62}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
            # value inside the bar end, white on a dark outline so the stripes do not hide it
            a = f'x="{X0 + w95 - 8:.1f}" y="{ry + 41}" font-size="16" text-anchor="end"'
            tx = f'p50 {fmt(p50)} · <tspan font-weight="700">p95 {fmt(p95)}</tspan>'
            o.append(f'<text {a} fill="{DARK}" stroke="{DARK}" stroke-width="3" stroke-linejoin="round">{tx}</text><text {a} fill="white">{tx}</text>')
            tag_x = lx + 8
            if p95 < BUDGET_MS:
                t = f"여유 {fmt(BUDGET_MS - p95)}ms"
                o.append(halo(tag_x, ry + 41, t, fill=AMBER, weight="700"))
                tag_x += text_w(t) + 16
            else:
                over.append(num)
                tag_x = X0 + w95 + 12
            if prev and not plot.distinguishable(S[c], S[prev]):
                o.append(f'<text x="{tag_x:.1f}" y="{ry + 41}" font-size="16" fill="{SUB}">앞 단계와 구별되지 않음</text>')
            prev = c
            ry += RH
        y = bottom + 14
    y += 24
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    foot = []
    if over:
        foot.append(f"단계 {num_list(over)} 1만에서 16.7ms를 넘는다.")
    foot.append(f"앞 단계와 구별되지 않음: 프레임 p95 차이가 {plot.FULL_DISTINCT_FLOOR_MS:g}ms 아래이거나 두 조건의 세 실행 범위보다 작다.")
    aa, s8 = S.get(cond("s8") + "-aa"), cond("s8")
    if aa:
        d = float(aa["frame_p95_ms"]) - val(s8, "frame_p95_ms")
        foot.append(f"측정 끝에 단계 8을 같은 조건으로 다시 세 번 잰 프레임 p95는 {fmt(float(aa['frame_p95_ms']))}ms로, "
                    f"{fmt(abs(d))}ms 달랐다.")
    foot.append("빗금은 단계가 하려던 일을 나타내고, 잰 결과가 아니다.")
    y = foot_lines(o, y + 10, foot) + 8
    hatch_key(o, y)
    write("ladder.svg", svg(y + 24, o))


def num_list(nums):
    """[1, 2] -> "1과 2는": Korean particles after the last digit's reading (2 이, 4 사, 5 오, 9 구 end in a vowel)."""
    vowel = lambda n: n % 10 in (2, 4, 5, 9)
    parts = [f"{n}{'와' if vowel(n) else '과'}" for n in nums[:-1]]
    return " ".join(parts + [f"{nums[-1]}{'는' if vowel(nums[-1]) else '은'}"])


# ---------- 2. resources per step ----------
# mem_total_mb   = CSV PhysicalUsedMB = process working set (CsvProfiler.cpp:4189, WindowsPlatformMemory.cpp:342)
# gpu_mem_p50    = CSV GPUMem/LocalUsedMB = DXGI local CurrentUsage of this process (RHIStats.cpp:91, DXGIUtilities.cpp:111)
# draw_calls_p50 = CSV RHI/DrawCalls per frame (GPUProfiler.cpp:2107)
RESOURCES = [("프로세스 메모리", "작업 집합, MB", "mem_total_mb", BLUE),
             ("GPU 메모리", "VRAM 사용량, MB", "gpu_mem_p50", PINK),
             ("드로우 콜", "프레임당 개수", "draw_calls_p50", TEAL)]
# Rises after step 3, named in the note with the step's name. resources() checks that each one is larger than both run ranges
# and that no other step after step 3 rises by more than both ranges.
RISES = {"mem_total_mb": [7], "gpu_mem_p50": [6, 7, 8], "draw_calls_p50": [6, 8]}
RISE_FROM = 4


def resources():
    span = {}  # (step, col) -> (min, max) over the used runs
    for num, _ in STEPS:
        for _, _, col, _ in RESOURCES:
            vs = run_values(cond(f"s{num}"), col)
            assert abs(statistics.median(vs) - val(cond(f"s{num}"), col)) < 0.01, (num, col, vs)
            span[num, col] = (min(vs), max(vs))
    width = lambda n, col: span[n, col][1] - span[n, col][0]
    for _, _, col, _ in RESOURCES:  # a named rise must be larger than both run ranges, and every such rise is named (measurement.md rule)
        found = [n for n, _ in STEPS if n >= RISE_FROM
                 and val(cond(f"s{n}"), col) - val(cond(f"s{n - 1}"), col) > max(width(n - 1, col), width(n, col))]
        assert found == RISES[col], (col, found, RISES[col])
    mem_spread = max(width(n, "mem_total_mb") for n, _ in STEPS)

    o = [HATCH]
    y = title(o, "단계마다 쓰는 메모리와 드로우 콜",
              ["호드 총수 1만, 이동 장면, 오브 시점 카메라, 세 번 잰 중앙값",
               "막대 둘레의 옅은 띠: 세 번 잰 범위(좁아서 2px가 안 되는 범위는 그리지 않았다)"])
    LX, CX, CR = 32, 330, W - 32
    CW = (CR - CX) / 3
    BWMAX = 70
    y += 44
    for i, (name, unit, _, c) in enumerate(RESOURCES):
        x = CX + i * CW
        o.append(f'<text x="{x:.1f}" y="{y}" font-size="17" font-weight="700" fill="{c}">{esc(name)}</text>')
        o.append(f'<text x="{x:.1f}" y="{y + 21}" font-size="16" fill="{SUB}">{esc(unit)}</text>')
    y += 38
    RH, BAR = 36, 18
    RANGE_MIN_PX = 2.0  # narrower ranges are not drawn (stated in the subtitle)
    for num, _ in STEPS:
        c0 = cond(f"s{num}")
        o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
        o.append(f'<text x="{LX}" y="{y + 24}" font-size="16" font-weight="700">단계 {num} {esc(STEP_TEXT[num][0])}</text>')
        for i, (_, _, col, c) in enumerate(RESOURCES):
            x = CX + i * CW
            sc = BWMAX / val(cond("s1"), col)
            v = val(c0, col)
            lo, hi = span[num, col]
            by = y + 9
            if (hi - lo) * sc >= RANGE_MIN_PX:  # pale band of the same colour behind the bar, min to max
                o.append(f'<rect x="{x + lo * sc:.1f}" y="{by - 4}" width="{(hi - lo) * sc:.1f}" height="{BAR + 8}" '
                         f'rx="4" fill="{c}" fill-opacity="0.28"/>')
                end = max(v, hi) * sc
            else:
                end = v * sc
            bar(o, x, by, max(v * sc, 1.5), BAR, c, num)
            o.append(f'<text x="{x + end + 7:.1f}" y="{by + 15}" font-size="16" font-weight="700" fill="{c}">{round(v):,}</text>')
        y += RH
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y += 32
    o.append(f'<text x="32" y="{y}" font-size="16" font-weight="700">단계 {RISE_FROM}부터 다시 느는 단계(세 번 잰 범위보다 큰 증가)</text>')
    for name, _, col, _ in RESOURCES:
        y += 22
        o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">{esc(name)}: '
                 + ", ".join(f"단계 {n} {esc(STEP_TEXT[n][0])}" for n in RISES[col]) + "</text>")
    y += 30
    o.append(f'<text x="32" y="{y}" font-size="16" fill="{SUB}">프로세스 메모리는 같은 단계에서도 실행마다 최대 약 {round(mem_spread, -1):.0f}MB 달랐다.</text>')
    hatch_key(o, y + 30)
    write("resources.svg", svg(y + 52, o))


# ---------- 3. technique contributions ----------
def row_notes(cfg, r, base):
    """What the frame bar does not show, one short line each. Every rule reads the data; nothing is written by hand per row."""
    out = []
    if r["bottleneck"] != base["bottleneck"]:
        out.append(f"병목이 {BOT_NAME[r['bottleneck']]}로 바뀐다")
    fd = float(r["frame_p95_ms"]) - float(base["frame_p95_ms"])
    gd = float(r["gpu_p50_ms"]) - float(base["gpu_p50_ms"])
    if plot.distinguishable(r, base, "gpu_p50_ms") and (not plot.distinguishable(r, base) or (fd > 0) != (gd > 0)):
        out.append(f"GPU 시간 {fmt(gd, True)}ms")
    rt, rt0 = float(r["sw_round_all"] or 0), float(base["sw_round_all"] or 0)
    if abs(rt - rt0) > DOC["equivalence"]["round_trip_max_per_sec"]:
        out.append(f"왕복 전환 초당 {fmt(rt)}건(최종 구성 {fmt(rt0)}건)")
    if r["iskm_tracks_last"] and base["iskm_tracks_last"]:
        t, t0 = float(r["iskm_tracks_last"]), float(base["iskm_tracks_last"])
        if t > t0 * TRACK_NOTE_RATIO:
            secs = DOC["scenario"]["speeds"][SPEED]["warmup_sec"] + DOC["scenario"]["measure_sec"]
            out.append(f"{secs}초 끝 ISKM 트랙 {t:,.0f}개(최종 구성 {t0:,.0f}개)")
    ov, ov0 = float(r["overlap_mean"]), float(base["overlap_mean"])
    if ov > ov0 * DOC["equivalence"]["overlap_rel_hi"]:
        out.append(f"겹친 쌍 {ov:,.0f}(최종 구성 {ov0:,.0f})")
    return out


def ablation():
    base_c = cond("s8")
    base = S[base_c]
    b95 = val(base_c, "frame_p95_ms")
    names = DOC["groups"]["ablation"]["configs"] + DOC["groups"]["sub"]["configs"]
    assert set(names) == set(ABL_TEXT), set(names) ^ set(ABL_TEXT)
    groups = {"dist": [], "same": [], "sub": []}
    run_counts = set()
    for cfg in names:
        r = S[cond(cfg)]
        d = float(r["frame_p95_ms"]) - b95
        dist = plot.distinguishable(r, base)
        item = {"cfg": cfg, "name": ABL_TEXT[cfg] + (" †" if cfg in ABL_MIXED else ""), "d": d,
                "lo": float(r["frame_p95_ms_min"]) - b95, "hi": float(r["frame_p95_ms_max"]) - b95,
                "state": ("slower" if d > 0 else "faster") if dist else "same", "notes": row_notes(cfg, r, base)}
        groups["sub" if CFGS[cfg]["kind"] == "sub" else "dist" if dist else "same"].append(item)
        run_counts.add(int(r["runs"]))
    for g in groups.values():
        g.sort(key=lambda it: -it["d"])
    color = {"slower": BLUE, "faster": PURPLE, "same": GRAY}

    o = []
    y = title(o, "기법별 기여(최종 구성에서 하나씩 껐을 때)",
              ["최종 구성(단계 8)에서 기법 하나를 끄고 같은 조건으로 잰 프레임 p95의 차이(ms)",
               f"호드 총수 1만, 이동 장면, 오브 시점 카메라, 세 번 잰 중앙값. 0은 최종 구성(프레임 p95 {fmt(b95)}ms)",
               f"최종 구성의 병목은 {BOT_NAME[base['bottleneck']]}다. 프레임에 드러나지 않는 차이는 행 아래에 적었다"])
    y += 34
    keys = [(BLUE, "길어진다(그 기법이 시간을 줄인다)"), (PURPLE, "짧아진다(그 기법이 시간을 쓴다)"), (GRAY, "구별되지 않는다")]
    x = 32
    for c, t in keys:
        if x + 22 + text_w(t) > W - 32:
            x, y = 32, y + 26
        o.append(f'<rect x="{x:.0f}" y="{y - 13}" width="16" height="16" rx="3" fill="{c}"/>')
        o.append(f'<text x="{x + 22:.0f}" y="{y}" font-size="16">{esc(t)}</text>')
        x += 22 + text_w(t) + 24
    y += 26
    o.append(f'<rect x="32" y="{y - 13}" width="22" height="16" fill="{GRAY}" fill-opacity="0.18"/>')
    o.append(f'<text x="62" y="{y}" font-size="16" fill="{SUB}">옅은 띠: ±{plot.FULL_DISTINCT_FLOOR_MS:g}ms. 막대 위의 검은 선: 그 조건을 세 번 잰 범위</text>')
    LX, X0, X1 = 32, 360, W - 32
    lo_ms, hi_ms = -8.0, 10.0
    assert all(lo_ms < it["lo"] and it["hi"] < hi_ms for g in groups.values() for it in g), "axis too short"
    sc = (X1 - X0) / (hi_ms - lo_ms)
    ZX = X0 - lo_ms * sc
    X = lambda v: ZX + v * sc
    y += 40
    o.append(f'<text x="{ZX - 10:.1f}" y="{y}" text-anchor="end" font-size="16" fill="{SUB}">← 짧아짐</text>')
    o.append(f'<text x="{ZX + 10:.1f}" y="{y}" font-size="16" fill="{SUB}">길어짐 →</text>')
    y += 12
    top = y
    heads = {"dist": "최종 구성과 구별되는 조건",
             "same": f"구별되지 않는 조건(차이가 {plot.FULL_DISTINCT_FLOOR_MS:g}ms 아래이거나 세 번 잰 범위 안)",
             "sub": "하위 실험: 기법을 끈 것이 아니라 바꾼 조건"}
    body = []
    BAR, BASE_H, NOTE_H, GAP = 18, 30, 20, 8
    for g in ("dist", "same", "sub"):
        if not groups[g]:
            continue
        y += 14
        body.append(f'<text x="{LX}" y="{y + 16}" font-size="16" font-weight="700" fill="{SUB}">{esc(heads[g])}</text>')
        y += 30
        for it in groups[g]:
            c = color[it["state"]]
            same = it["state"] == "same"
            body.append(f'<text x="{LX}" y="{y + 15}" font-size="16"{"" if same else " font-weight=" + chr(34) + "700" + chr(34)}'
                        f' fill="{SUB if same else INK}">{esc(it["name"])}</text>')
            x0, x1 = sorted((ZX, X(it["d"])))
            body.append(f'<rect x="{x0:.1f}" y="{y + 1}" width="{max(x1 - x0, 1.5):.1f}" height="{BAR}" rx="2" fill="{c}"/>')
            cy = y + 1 + BAR / 2
            for wdt, col in ((3.5, "white"), (1.4, INK)):  # range line: white under black, readable on and off the bar
                body.append(f'<line x1="{X(it["lo"]):.1f}" y1="{cy}" x2="{X(it["hi"]):.1f}" y2="{cy}" stroke="{col}" stroke-width="{wdt}"/>')
                for v in (it["lo"], it["hi"]):
                    body.append(f'<line x1="{X(v):.1f}" y1="{cy - 5}" x2="{X(v):.1f}" y2="{cy + 5}" stroke="{col}" stroke-width="{wdt}"/>')
            txt = fmt(it["d"], True)
            inside = (x1 - x0) > text_w(txt) + 20
            if inside:  # long bar: value inside its far end, in white
                tx, anchor = (x1 - 8, "end") if it["d"] > 0 else (x0 + 8, "start")
                body.append(f'<text x="{tx:.1f}" y="{y + 15}" font-size="16" font-weight="700" text-anchor="{anchor}" fill="white">{txt}</text>')
            else:
                ex = max(X(it["hi"]), x1) + 7 if it["d"] >= 0 else min(X(it["lo"]), x0) - 7
                body.append(halo(ex, y + 15, txt, fill=SUB if same else c, anchor="start" if it["d"] >= 0 else "end",
                                 weight=None if same else "700"))
            y += BASE_H
            for t in it["notes"]:
                body.append(f'<text x="{LX + 14}" y="{y + 9}" font-size="16" font-weight="700" fill="{BLUE}">{esc(t)}</text>')
                y += NOTE_H
            y += GAP
    bottom = y
    o.append(f'<rect x="{X(-plot.FULL_DISTINCT_FLOOR_MS):.1f}" y="{top}" width="{2 * plot.FULL_DISTINCT_FLOOR_MS * sc:.1f}" '
             f'height="{bottom - top}" fill="{GRAY}" fill-opacity="0.18"/>')
    for t in range(int(lo_ms), int(hi_ms) + 1, 2):
        if t:
            o.append(f'<line x1="{X(t):.1f}" y1="{top}" x2="{X(t):.1f}" y2="{bottom}" stroke="#eef0f3"/>')
        o.append(f'<text x="{X(t):.1f}" y="{bottom + 20}" text-anchor="middle" font-size="16" fill="{SUB}">{t:+d}'.replace("+0", "0") + "</text>")
    o += body
    o.append(f'<line x1="{ZX:.1f}" y1="{top}" x2="{ZX:.1f}" y2="{bottom}" stroke="{INK}" stroke-width="1.5"/>')
    y = bottom + 46
    o.append(f'<line x1="32" y1="{y - 16}" x2="{W - 32}" y2="{y - 16}" stroke="{LINE}"/>')
    foot = [f"구별된다: 차이가 {plot.FULL_DISTINCT_FLOOR_MS:g}ms 이상이고, 두 조건의 세 실행 범위 가운데 큰 쪽보다 크다."]
    if run_counts != {3}:
        foot.append("느린 실행을 다시 잰 한 조건은 네 번 잰 중앙값이다.")
    foot += ["† " + ABL_MIXED[c] for c in names if c in ABL_MIXED]
    y = foot_lines(o, y + 6, foot)
    write("ablation.svg", svg(y + 12, o))


# ---------- 4. round trips with and without the minimum hold time ----------
def round_trips():
    groups = [("이동 장면", "V0"), ("정지 장면", "V1")]
    lim = DOC["equivalence"]["round_trip_max_per_sec"]
    o = []
    y = title(o, "최소 유지 시간을 끄면 생기는 왕복 전환",
              ["왕복 전환: 표현 경계를 넘은 개체가 2초 안에 같은 경계를 반대로 되넘은 수(초당)",
               "호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값. 막대 위의 검은 선: 세 번 잰 범위",
               "최소 유지 시간: 한 번 바뀐 개체를 2초 동안 바로 전의 표현으로 되돌리지 않는다"])
    LX, X0, X1 = 32, 330, 700
    vmax = 5.0
    peak = max(float(S[f"{sp}-a8hold"]["sw_round_all_max"]) for _, sp in groups)
    assert peak < vmax, peak
    sc = (X1 - X0) / vmax
    X = lambda v: X0 + v * sc
    y += 40
    top = y
    RH = 36
    for head, sp in groups:
        y += 12
        o.append(f'<text x="{LX}" y="{y + 10}" font-size="17" font-weight="700">{head}</text>')
        y += 24
        for cfg, name in (("s8", "최종 구성"), ("a8hold", ABL_TEXT["a8hold"])):
            r = S[f"{sp}-{cfg}"]
            v, lo, hi = float(r["sw_round_all"]), float(r["sw_round_all_min"]), float(r["sw_round_all_max"])
            o.append(f'<text x="{LX + 14}" y="{y + 15}" font-size="16">{esc(name)}</text>')
            c = PURPLE if v > lim else GRAY
            o.append(f'<rect x="{X0}" y="{y + 1}" width="{max(v * sc, 2):.1f}" height="18" rx="2" fill="{c}"/>')
            if hi > lo:
                for wdt, col in ((3.5, "white"), (1.4, INK)):
                    o.append(f'<line x1="{X(lo):.1f}" y1="{y + 10}" x2="{X(hi):.1f}" y2="{y + 10}" stroke="{col}" stroke-width="{wdt}"/>')
                    for xv in (lo, hi):
                        o.append(f'<line x1="{X(xv):.1f}" y1="{y + 5}" x2="{X(xv):.1f}" y2="{y + 15}" stroke="{col}" stroke-width="{wdt}"/>')
            o.append(halo(X(max(v, hi)) + 8, y + 15, f"{fmt(v)}건", fill=c if v > lim else SUB, weight="700"))
            y += RH
    bottom = y
    o.append(f'<line x1="{X(lim):.1f}" y1="{top}" x2="{X(lim):.1f}" y2="{bottom}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    for t in range(int(vmax) + 1):
        o.append(f'<text x="{X(t):.1f}" y="{bottom + 20}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')
    o.append(f'<text x="{X1 + 8}" y="{bottom + 20}" font-size="16" fill="{SUB}">건/초</text>')
    y = bottom + 50
    o.append(f'<line x1="{X(lim) - 30:.1f}" y1="{y - 5}" x2="{X(lim) - 8:.1f}" y2="{y - 5}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<text x="{X(lim):.1f}" y="{y}" font-size="16" fill="{SUB}">동등성 검사의 상한(초당 {lim:g}건)</text>')
    y += 34
    o.append(f'<line x1="32" y1="{y - 16}" x2="{W - 32}" y2="{y - 16}" stroke="{LINE}"/>')
    b, h = S[cond("s8")], S[cond("a8hold")]
    d95 = float(h["frame_p95_ms"]) - float(b["frame_p95_ms"])
    same = not plot.distinguishable(h, b)
    foot = [f"이동 장면에서 인스턴스가 액터로 바뀌는 전환은 최종 구성에서 초당 {fmt(float(b['sw_inst_to_actor']))}건이다.",
            f"최소 유지 시간을 끈 조건의 프레임 p95 차이({fmt(d95, True)}ms)는 최종 구성과 "
            + ("구별되지 않았다." if same else "구별되었다.")]
    y = foot_lines(o, y + 4, foot)
    write("round-trips.svg", svg(y + 12, o))


# ---------- 5. entity pool over a long run (reference batch) ----------
# Measurements/Pool2/timeline.csv: one run each with a 960 s window (Scripts/run-pool.ps1), 1-minute bins (Scripts/pool-timeline.py).
# Pool on = s8 arguments, pool off = a8pool arguments (-LabEntityPool=0). One run each, so no run range and no distinguishable test.
POOL_RUNS = [("s8", "풀 켬(최종 구성)", BLUE, ""), ("a8pool", "풀 끔", RED, "6 4")]
POOL_PANELS = [("iskm_tracks_end", "ISKM 트랙 수", "개", 30000, 0.0),
               ("gpu_mem_mean", "GPU 메모리", "MB", 1000, None),
               ("frame_p95", "프레임 p95", "ms", 1, None)]
POOL_LAST = 4  # the footnote compares the last four 1-minute bins


def pool():
    data = {}
    with open(os.path.join(ROOT, "Measurements", POOL_BATCH, "timeline.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            for cfg, *_ in POOL_RUNS:
                if f"-{SPEED}-{cfg}-N{N}-{PRESET}-" in r["label"]:
                    data.setdefault(cfg, []).append(r)
    for cfg in data:
        data[cfg].sort(key=lambda r: int(r["minute"]))
    minutes = [int(r["minute"]) for r in data["s8"]]
    assert minutes == [int(r["minute"]) for r in data["a8pool"]], "the two runs must have the same bins"
    m0, m1 = minutes[0], minutes[-1]
    o = []
    y = title(o, "엔티티 풀을 끄면 오래 돌수록 ISKM 트랙과 GPU 메모리가 는다",
              [f"참고: 다른 그림과 따로 잰 긴 실행이다. 최종 구성을 풀 켬과 풀 끔으로 {m1}분씩 한 번 돌렸다",
               "호드 총수 1만, 이동 장면, 오브 시점 카메라. 1분 구간마다의 값. 실행이 한 번씩이라 범위는 없다"])
    y += 34
    x = 32
    for _, name, c, dash in POOL_RUNS:
        da = f' stroke-dasharray="{dash}"' if dash else ""
        o.append(f'<line x1="{x}" y1="{y - 5}" x2="{x + 26}" y2="{y - 5}" stroke="{c}" stroke-width="3"{da}/>')
        o.append(f'<text x="{x + 34}" y="{y}" font-size="16">{esc(name)}</text>')
        x += 34 + text_w(name) + 30
    PX0, PX1, PH, GAP = 120, W - 150, 110, 64
    sx = (PX1 - PX0) / (m1 - m0)
    y += 40
    for col, name, unit, step, floor in POOL_PANELS:
        vals = [float(r[col]) for cfg, *_ in POOL_RUNS for r in data[cfg]]
        hi = max(vals)
        lo = floor if floor is not None else step * int(min(vals) // step)
        top = step * (int(hi // step) + 1)
        sy = PH / (top - lo)
        o.append(f'<text x="32" y="{y}" font-size="17" font-weight="700">{esc(name)}</text>'
                 f'<text x="{32 + text_w(name, 17) + 16:.0f}" y="{y}" font-size="16" fill="{SUB}">({unit})</text>')
        y0 = y + 16
        base = y0 + PH
        t = lo
        while t <= top + 1e-9:
            ty = base - (t - lo) * sy
            o.append(f'<line x1="{PX0}" y1="{ty:.1f}" x2="{PX1}" y2="{ty:.1f}" stroke="{LINE}"/>')
            lab = f"{t / 10000:g}만" if col == "iskm_tracks_end" and t else f"{t:,.0f}"
            o.append(f'<text x="{PX0 - 8}" y="{ty + 5:.1f}" text-anchor="end" font-size="16" fill="{SUB}">{lab}</text>')
            t += step
        ends = []
        for cfg, rname, c, dash in POOL_RUNS:
            pts = " ".join(f"{PX0 + (int(r['minute']) - m0) * sx:.1f},{base - (float(r[col]) - lo) * sy:.1f}" for r in data[cfg])
            da = f' stroke-dasharray="{dash}"' if dash else ""
            o.append(f'<polyline points="{pts}" fill="none" stroke="{c}" stroke-width="2.5"{da}/>')
            last = float(data[cfg][-1][col])
            ends.append((base - (last - lo) * sy, last, c))
        if abs(ends[0][0] - ends[1][0]) < 18:  # keep the two end labels apart
            mid = (ends[0][0] + ends[1][0]) / 2
            hi_first = ends[0][0] < ends[1][0]
            ends = [(mid - 9 if (i == 0) == hi_first else mid + 9, v, c, ly) for i, (ly, v, c) in enumerate(ends)]
        else:
            ends = [(ly, v, c, ly) for ly, v, c in ends]
        for ty, last, c, ly in ends:
            txt = f"{last:,.0f}" if col != "frame_p95" else fmt(last)
            o.append(f'<circle cx="{PX1:.1f}" cy="{ly:.1f}" r="4" fill="{c}"/>')
            o.append(f'<text x="{PX1 + 10}" y="{ty + 5:.1f}" font-size="16" font-weight="700" fill="{c}">{txt}</text>')
        y = base + GAP
    ay = y - GAP + 24
    for mm in range(m0, m1 + 1):
        if mm in (m0, m1) or mm % 4 == 0:
            o.append(f'<text x="{PX0 + (mm - m0) * sx:.1f}" y="{ay}" text-anchor="middle" font-size="16" fill="{SUB}">{mm}분</text>')
    y = ay + 32
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    on, off = data["s8"], data["a8pool"]
    late = [float(b["frame_p95"]) - float(a["frame_p95"]) for a, b in zip(on[-POOL_LAST:], off[-POOL_LAST:])]
    foot = [f"오른쪽 숫자: {m1}분째 값. 풀 끔의 1분째는 트랙 {float(off[0]['iskm_tracks_end']):,.0f}, "
            f"GPU 메모리 {float(off[0]['gpu_mem_mean']):,.0f}MB, 프레임 p95 {fmt(float(off[0]['frame_p95']))}ms다.",
            f"마지막 {POOL_LAST}분({m1 - POOL_LAST + 1}–{m1}분)의 프레임 p95는 풀 끔이 {fmt(min(late))}–{fmt(max(late))}ms 길었다."
            if min(late) > 0 else f"마지막 {POOL_LAST}분의 프레임 p95 차이는 {fmt(min(late), True)}–{fmt(max(late), True)}ms였다."]
    for i, t in enumerate(foot):
        o.append(f'<text x="32" y="{y + 12 + 24 * i}" font-size="16" fill="{SUB}">{esc(t)}</text>')
    write("entity-pool.svg", svg(y + 12 + 24 * len(foot) + 10, o))


# ---------- 6. scenario (top-down sketch) ----------
def pol(cx, cy, r, deg):
    """Screen point at screen angle deg (0 = right, 90 = up). UE seen from above is screen x = X, screen y = Y, so the orbit and the camera turn clockwise."""
    a = math.radians(deg)
    return cx + r * math.cos(a), cy - r * math.sin(a)


def scenario():
    o = ['<defs>'
         f'<marker id="arr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{SUB}"/></marker>'
         f'<marker id="arrb" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{BLUE}"/></marker>'
         f'<marker id="arrp" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{PURPLE}"/></marker>'
         f'<marker id="arra" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
         f'<path d="M0 0 L10 5 L0 10 z" fill="{AMBER}"/></marker>'
         '</defs>']
    y = title(o, "시나리오: 위에서 내려다본 맵",
              ["호드가 맵 중심 둘레를 도는 오브를 사방에서 쫓는다. 카메라는 오브를 따라간다"])
    rnd = random.Random(SEED)
    ORB_DEG = 30  # where the orb is in the sketch (screen angle); it moves clockwise

    # ---- Panel A: the whole map and the off-map crowd, to scale ----
    CX, CY, RP = 250, y + 262, 215
    s = RP / FAR_OUT
    o.append(f'<text x="32" y="{y + 42}" font-size="17" font-weight="700">맵 전체(반경 {MAP_R}m)와 맵 밖 무리</text>')
    o.append(f'<circle cx="{CX}" cy="{CY}" r="{RP}" fill="{AMBER}" fill-opacity="0.08" stroke="{AMBER}" stroke-opacity="0.45" stroke-dasharray="4 4"/>')
    o.append(f'<circle cx="{CX}" cy="{CY}" r="{MAP_R * s:.1f}" fill="{CARD}" stroke="{GRAY}" stroke-width="1.5"/>')
    ox, oy = pol(CX, CY, ORBIT_R * s, ORB_DEG)
    # P1 field of view (horizontal) from the orb outward, to the outer ring
    view = ORB_DEG
    a0, a1 = view - FOV_H / 2, view + FOV_H / 2
    o.append(f'<clipPath id="ac"><circle cx="{CX}" cy="{CY}" r="{RP}"/></clipPath>')
    p0, p1 = pol(ox, oy, RP * 1.6, a0), pol(ox, oy, RP * 1.6, a1)
    o.append(f'<path clip-path="url(#ac)" d="M{ox:.1f} {oy:.1f} L{p0[0]:.1f} {p0[1]:.1f} L{p1[0]:.1f} {p1[1]:.1f} Z" fill="{BLUE}" fill-opacity="0.08" '
             f'stroke="{BLUE}" stroke-opacity="0.5" stroke-dasharray="4 4"/>')
    # off-map records: radius uniform in [MAP_R, FAR_OUT] -> density ~ 1/r (ADR-0020 decision 1)
    for i in range(FAR_DOTS):
        r = rnd.uniform(MAP_R, FAR_OUT)
        a = rnd.uniform(0, 360)
        x, yy = pol(CX, CY, r * s, a)
        o.append(f'<circle cx="{x:.1f}" cy="{yy:.1f}" r="{2.2 if i % LARGE_EVERY == 0 else 1.3}" fill="{AMBER}" fill-opacity="0.55"/>')
    # minions: radius uniform in [ZONE_R, MAP_R] around the map centre -> density ~ 1/r (LabCrowdSpawner.cpp SpawnInitial)
    dots = []
    for i in range(DOTS):
        r = rnd.uniform(ZONE_R, MAP_R)
        a = rnd.uniform(0, 360)
        dots.append((i, r, a))
        x, yy = pol(CX, CY, r * s, a)
        if i % LARGE_EVERY == 0:
            o.append(f'<circle cx="{x:.1f}" cy="{yy:.1f}" r="2.6" fill="{DARK}"/>')
        else:
            o.append(f'<circle cx="{x:.1f}" cy="{yy:.1f}" r="1.5" fill="{GRAY}"/>')
    # snipe shots: in view, per distance band from the orb, the first dots in seeded order stand in for the smallest hash
    edges = list(SNIPE_EDGES) + [MAP_R + 1]
    for b, k in enumerate(SNIPE_COUNTS):
        picks = []
        for _, r, a in dots:
            x, yy = pol(CX, CY, r * s, a)
            dx, dy = (x - ox) / s, -(yy - oy) / s
            dist = math.hypot(dx, dy)
            ang = math.degrees(math.atan2(dy, dx))
            if edges[b] <= dist < edges[b + 1] and a0 + 6 < ang < a1 - 6 and math.hypot(x - CX, yy - CY) < MAP_R * s:
                picks.append((x, yy))
        for x, yy in picks[:k]:
            o.append(f'<line x1="{ox:.1f}" y1="{oy:.1f}" x2="{x:.1f}" y2="{yy:.1f}" stroke="{PINK}" stroke-width="1.6"/>')
            o.append(f'<circle cx="{x:.1f}" cy="{yy:.1f}" r="4.5" fill="none" stroke="{PINK}" stroke-width="2"/>')
    # walking towards the orb: minions in the map and records outside it
    for a in (165, 215, 262, 318):
        xa, ya = pol(CX, CY, 430 * s, a)
        xb, yb = pol(CX, CY, 300 * s, a)
        o.append(f'<line x1="{xa:.1f}" y1="{ya:.1f}" x2="{xb:.1f}" y2="{yb:.1f}" stroke="{SUB}" stroke-width="2" marker-end="url(#arr)"/>')
    for a in (140, 200, 250, 300, 345):
        xa, ya = pol(CX, CY, 900 * s, a)
        xb, yb = pol(CX, CY, 700 * s, a)
        o.append(f'<line x1="{xa:.1f}" y1="{ya:.1f}" x2="{xb:.1f}" y2="{yb:.1f}" stroke="{AMBER}" stroke-width="2" marker-end="url(#arra)"/>')
    # orbit and orb
    o.append(f'<circle cx="{CX}" cy="{CY}" r="{ORBIT_R * s:.1f}" fill="none" stroke="{PURPLE}" stroke-width="1.5" stroke-dasharray="3 2"/>')
    o.append(f'<circle cx="{ox:.1f}" cy="{oy:.1f}" r="4" fill="{PURPLE}"/>')
    # the enlarged area of panel B: around the orb
    o.append(f'<circle cx="{ox:.1f}" cy="{oy:.1f}" r="{B_R * s + 2:.1f}" fill="none" stroke="{INK}" stroke-width="1.2"/>')
    o.append(f'<text x="{ox + B_R * s + 4:.1f}" y="{oy + 20:.1f}" font-size="16" font-weight="700" fill="{INK}">B</text>')
    # ruler under the disc: distance from the map centre
    ry = CY + RP + 26
    o.append(f'<line x1="{CX}" y1="{ry}" x2="{CX + RP}" y2="{ry}" stroke="{SUB}"/>')
    for d in (0, MAP_R, FAR_OUT):
        x = CX + d * s
        o.append(f'<line x1="{x:.1f}" y1="{ry - 5}" x2="{x:.1f}" y2="{ry + 5}" stroke="{SUB}"/>')
        t = f"{d:,}m" if d else "0"
        o.append(f'<text x="{x:.1f}" y="{ry + 22}" text-anchor="middle" font-size="16" fill="{SUB}">{t}</text>')
    o.append(f'<text x="{CX - 12}" y="{ry + 5}" text-anchor="end" font-size="16" fill="{SUB}">맵 중심에서 거리</text>')

    # legend column
    LX = 490
    ly = y + 82

    def item(mark, head, lines):
        nonlocal ly
        o.append(mark(LX + 10, ly - 5))
        o.append(f'<text x="{LX + 28}" y="{ly}" font-size="16" font-weight="700">{esc(head)}</text>')
        for j, ln in enumerate(lines):
            o.append(f'<text x="{LX + 28}" y="{ly + 21 * (j + 1)}" font-size="16" fill="{SUB}">{esc(ln)}</text>')
        ly += 21 * (len(lines) + 1) + 12

    item(lambda x, yy: f'<circle cx="{x}" cy="{yy}" r="2.5" fill="{GRAY}"/>', "근접 미니언 90%", [])
    item(lambda x, yy: f'<circle cx="{x}" cy="{yy}" r="4" fill="{DARK}"/>', "시즈 미니언 10%",
         [f"둘 다 {SPEED_MPS}m/s로 오브의 지금", "자리를 향해 걷는다", f"체력은 개체마다 {HEALTH[0]}–{HEALTH[1]}"])
    item(lambda x, yy: f'<circle cx="{x}" cy="{yy}" r="5" fill="{PURPLE}"/>', f"오브(궤도 반지름 {fmt(ORBIT_R)}m)", ["확대는 아래 B"])
    item(lambda x, yy: f'<rect x="{x - 8}" y="{yy - 6}" width="16" height="12" rx="2" fill="{AMBER}" fill-opacity="0.45"/>',
         f"맵 밖 무리 {MAP_R}–{FAR_OUT:,}m", ["코드가 굴리는 기록. 개체가 죽으면", "같은 종류에서 오브에 가장 가까운", "기록의 자리에서 다시 생겨", "호드 총수가 그대로다"])
    item(lambda x, yy: f'<g><line x1="{x - 8}" y1="{yy + 5}" x2="{x + 6}" y2="{yy - 6}" stroke="{PINK}" stroke-width="2"/>'
                       f'<circle cx="{x + 6}" cy="{yy - 6}" r="4" fill="none" stroke="{PINK}" stroke-width="2"/></g>',
         f"저격: {SNIPE_EVERY}초마다 {SNIPE_K}명", ["화면 안의 개체만, 오브에서",
                                      f"{SNIPE_EDGES[0]}–{SNIPE_EDGES[1]}m {SNIPE_COUNTS[0]}명, {SNIPE_EDGES[1]}–{SNIPE_EDGES[2]}m {SNIPE_COUNTS[1]}명,",
                                      f"{SNIPE_EDGES[2]}m 밖 {SNIPE_COUNTS[2]}명"])
    item(lambda x, yy: f'<rect x="{x - 8}" y="{yy - 8}" width="16" height="16" rx="3" fill="{BLUE}" fill-opacity="0.15" stroke="{BLUE}" stroke-dasharray="3 3"/>',
         f"오브 시점 화면(가로 {FOV_H}°)", ["카메라가 1분에 한 바퀴 돈다"])

    # ---- Panel B: around the orb, enlarged ----
    y = ry + 66
    o.append(f'<line x1="32" y1="{y - 24}" x2="{W - 32}" y2="{y - 24}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 6}" font-size="17" font-weight="700">B: 오브 둘레 확대(반경 {B_R}m)</text>')
    SB = 4.6  # px per metre
    RB = B_R * SB
    BX, BY = 360, y + 30 + RB
    ox, oy = BX, BY
    centre_deg = ORB_DEG + 180  # the map centre is 57.3 m from the orb, opposite the view in this sketch
    o.append(f'<clipPath id="bc"><circle cx="{BX}" cy="{BY:.1f}" r="{RB:.1f}"/></clipPath>')
    o.append(f'<circle cx="{BX}" cy="{BY:.1f}" r="{RB:.1f}" fill="{CARD}" stroke="{GRAY}" stroke-width="1.5"/>')
    # P1: the camera is 15 m behind the orb and looks past it; the sketch shows it looking outward from the map centre
    cx_, cy_ = pol(ox, oy, P1_BACK * SB, centre_deg)
    q0, q1 = pol(cx_, cy_, RB * 3, ORB_DEG - FOV_H / 2), pol(cx_, cy_, RB * 3, ORB_DEG + FOV_H / 2)
    o.append(f'<path clip-path="url(#bc)" d="M{cx_:.1f} {cy_:.1f} L{q0[0]:.1f} {q0[1]:.1f} L{q1[0]:.1f} {q1[1]:.1f} Z" fill="{BLUE}" fill-opacity="0.08" '
             f'stroke="{BLUE}" stroke-opacity="0.5" stroke-dasharray="4 4"/>')
    # snipe edge: 30 m from the orb
    o.append(f'<circle cx="{ox:.1f}" cy="{oy:.1f}" r="{SNIPE_EDGES[0] * SB:.1f}" fill="none" stroke="{PINK}" stroke-opacity="0.7" stroke-dasharray="4 4"/>')
    # orbit: a circle of 57.3 m around the map centre, clockwise; an arrow ahead of the orb
    mx, my = pol(ox, oy, ORBIT_R * SB, centre_deg)
    o.append(f'<circle clip-path="url(#bc)" cx="{mx:.1f}" cy="{my:.1f}" r="{ORBIT_R * SB:.1f}" fill="none" stroke="{PURPLE}" stroke-width="1.5" stroke-dasharray="6 5"/>')
    e0 = pol(mx, my, ORBIT_R * SB, ORB_DEG - 3)
    e1 = pol(mx, my, ORBIT_R * SB, ORB_DEG - 18)
    o.append(f'<path d="M{e0[0]:.1f} {e0[1]:.1f} A{ORBIT_R * SB:.1f} {ORBIT_R * SB:.1f} 0 0 1 {e1[0]:.1f} {e1[1]:.1f}" fill="none" '
             f'stroke="{PURPLE}" stroke-width="2.5" marker-end="url(#arrp)"/>')
    # towards the map centre
    t0, t1 = pol(ox, oy, 20 * SB, centre_deg + 22), pol(ox, oy, (B_R - 3) * SB, centre_deg + 22)
    o.append(f'<line x1="{t0[0]:.1f}" y1="{t0[1]:.1f}" x2="{t1[0]:.1f}" y2="{t1[1]:.1f}" stroke="{PURPLE}" stroke-width="1.2" stroke-dasharray="2 3" marker-end="url(#arrp)"/>')
    # camera circle around the orb (1 rpm, clockwise)
    o.append(f'<circle cx="{ox:.1f}" cy="{oy:.1f}" r="{P1_BACK * SB:.1f}" fill="none" stroke="{BLUE}" stroke-width="1.2" stroke-dasharray="4 4"/>')
    c0 = pol(ox, oy, P1_BACK * SB, centre_deg - 12)
    c1 = pol(ox, oy, P1_BACK * SB, centre_deg - 60)
    o.append(f'<path d="M{c0[0]:.1f} {c0[1]:.1f} A{P1_BACK * SB:.1f} {P1_BACK * SB:.1f} 0 0 1 {c1[0]:.1f} {c1[1]:.1f}" fill="none" '
             f'stroke="{BLUE}" stroke-width="2" marker-end="url(#arrb)"/>')
    # minions walking towards the orb's present position
    for a, r0 in ((70, 37), (110, 33), (150, 38), (250, 36), (290, 30), (330, 37), (10, 34), (205, 24), (45, 22)):
        xa, ya = pol(ox, oy, r0 * SB, a)
        xb, yb = pol(ox, oy, (r0 - 7) * SB, a)
        o.append(f'<circle cx="{xa:.1f}" cy="{ya:.1f}" r="4" fill="{GRAY}"/>')
        o.append(f'<line x1="{xa:.1f}" y1="{ya:.1f}" x2="{xb:.1f}" y2="{yb:.1f}" stroke="{SUB}" stroke-width="1.5" marker-end="url(#arr)"/>')
    # damage zone and orb, top-down camera ring
    o.append(f'<circle cx="{ox:.1f}" cy="{oy:.1f}" r="{ZONE_R * SB:.1f}" fill="{PURPLE}" fill-opacity="0.18" stroke="{PURPLE}" stroke-width="1.5"/>')
    o.append(f'<circle cx="{ox:.1f}" cy="{oy:.1f}" r="6" fill="{PURPLE}"/>')
    o.append(f'<circle cx="{ox:.1f}" cy="{oy:.1f}" r="12" fill="none" stroke="{TEAL}" stroke-width="2.5"/>')
    # camera icon, pointing along the view
    ang = -ORB_DEG  # SVG rotation is clockwise-positive
    o.append(f'<g transform="translate({cx_:.1f} {cy_:.1f}) rotate({ang})"><rect x="-9" y="-7" width="18" height="14" rx="2" fill="{BLUE}"/>'
             f'<path d="M9 -4 L15 -8 L15 8 L9 4 Z" fill="{BLUE}"/></g>')
    # labels with leader lines: right column and left column
    RX, LXB = 565, 32
    labels = [
        (RX, BY - 150, PURPLE, "오브", ["맵 중심 둘레 반지름", f"{fmt(ORBIT_R)}m 궤도를 {ORBIT_V}m/s로 돈다", f"한 바퀴 {round(2 * math.pi * ORBIT_R / ORBIT_V)}초, 높이 {ORB_H}m"], (ox + 7, oy - 5)),
        (RX, BY - 40, PURPLE, f"피해 범위 {ZONE_R}m", [f"초당 {ZONE_DPS[0]}(가장자리)–", f"{ZONE_DPS[1]}(중심)"], pol(ox, oy, ZONE_R * SB, -15)),
        (RX, BY + 50, TEAL, "탑뷰 카메라", ["오브 바로 위", f"{P2_HEIGHT}m에서 내려다본다"], pol(ox, oy, 12, -50)),
        (RX, BY + 140, PINK, f"오브에서 {SNIPE_EDGES[0]}m", ["저격은 이 밖부터"], pol(ox, oy, SNIPE_EDGES[0] * SB, -38)),
        (LXB, BY + 40, BLUE, "오브 시점 카메라", [f"오브 뒤 {P1_BACK}m, 높이 {P1_HEIGHT}m.", "오브를 따라가며", "1분에 한 바퀴 돈다"], (cx_ - 12, cy_ + 4)),
        (LXB, BY - 150, SUB, "개체는 오브의 지금", ["자리를 향해 걷는다"], None),
        (LXB, BY + 150, PURPLE, "맵 중심 쪽", [f"({fmt(ORBIT_R)}m 떨어져 있다)"], (t1[0] - 6, t1[1] - 4)),
    ]
    for lx_, ty, c, head, rest, tip in labels:
        if tip:
            sx_ = lx_ - 6 if lx_ == RX else lx_ + text_w(head) + 6
            o.append(f'<line x1="{sx_:.1f}" y1="{ty - 5:.1f}" x2="{tip[0]:.1f}" y2="{tip[1]:.1f}" stroke="{c}" stroke-width="1"/>')
        o.append(f'<text x="{lx_}" y="{ty:.1f}" font-size="16" font-weight="700" fill="{c}">{esc(head)}</text>')
        for j, t in enumerate(rest):
            o.append(f'<text x="{lx_}" y="{ty + 21 * (j + 1):.1f}" font-size="16" fill="{SUB}">{esc(t)}</text>')
    # scale bar
    sy_ = BY + RB + 30
    o.append(f'<line x1="{BX - 10 * SB:.1f}" y1="{sy_:.1f}" x2="{BX + 10 * SB:.1f}" y2="{sy_:.1f}" stroke="{SUB}" stroke-width="2"/>')
    for xv in (BX - 10 * SB, BX + 10 * SB):
        o.append(f'<line x1="{xv:.1f}" y1="{sy_ - 5:.1f}" x2="{xv:.1f}" y2="{sy_ + 5:.1f}" stroke="{SUB}" stroke-width="2"/>')
    o.append(f'<text x="{BX + 10 * SB + 10:.1f}" y="{sy_ + 5:.1f}" font-size="16" fill="{SUB}">20m</text>')
    y = sy_ + 30
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 28}" font-size="16" fill="{SUB}">두 그림 모두 거리는 축척대로다. 개체 점은 실제 수보다 적게 그렸다.</text>')
    o.append(f'<text x="32" y="{y + 52}" font-size="16" fill="{SUB}">오브와 카메라의 자리와 방향은 한 순간의 예다. 둘 다 위에서 보아 시계 방향으로 돈다.</text>')
    write("scenario.svg", svg(y + 74, o))


if __name__ == "__main__":
    scenario()
    ladder()
    resources()
    ablation()
    round_trips()
    pool()
    print("wrote", ", ".join(sorted(os.listdir(OUT))), "to", os.path.relpath(OUT, ROOT))
