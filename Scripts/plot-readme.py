# Makes the SVG figures of the root README in Docs/figures/readme/.
# Run from anywhere: py -3 -I Scripts/plot-readme.py
# Every value comes from the final measurement:
#   Measurements/Full1/summary.csv               ladder and ablation (10000, P1)
#   Docs/figures/full1/full-n-at-60fps.csv       max entities at 60 fps (n_p95)
#   Measurements/Full1/transition-dev/runs.csv   transition pop (mean_excess, noticeable/events)
# Colours and fonts follow Posts/01-actor-baseline/make-figures.py.
import csv, os, statistics

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
OUT = os.path.join(ROOT, "Docs", "figures", "readme")
N, PRESET = 10000, "P1"

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
    ("A8-hydrate", "액터 하이드레이션", []),
    ("A8-budget", "애니메이션 예산", []),
    ("A8-simlod", "Simulation LOD", []),
    ("A8-bounds", "고정 바운드", []),
    ("A8-fix", "전환 대책", []),
    ("A8-pool", "엔티티 풀", []),
    ("A8-iskm", "ISKM", ["(VAT로 되돌림)"]),
    ("A8-farshadow", "80m 밖 그림자 끔", ["(되돌려 그림자 켬)"]),
    ("A8-farcull", "200m 밖을 입자 대신", ["ISKM으로 그림"]),
    ("A8-crowdfx", "Niagara 배경 무리", []),
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


def load_summary():
    rows = {}
    with open(os.path.join(ROOT, "Measurements", "Full1", "summary.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if int(r["n"]) == N and r["preset"] == PRESET:
                rows[r["variant"]] = r
    return rows


S = load_summary()


def val(cond, col):
    return float(S[cond][col])


def rng(cond, col):
    return float(S[cond][col + "_max"]) - float(S[cond][col + "_min"])


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


# ---------- 1. ladder ----------
def ladder():
    o = []
    y = title(o, "단계마다 기법 하나를 더했을 때의 프레임 시간",
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
            o.append(f'<rect x="{BX}" y="{by}" width="{w:.1f}" height="{BAR}" rx="3" fill="{c}"/>')
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
        yy = block(yy, *st, s2)
    target_line(y0 - 26, yy - 10, s2, "16.7ms")
    y = yy + 6
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">프레임 시간은 게임 스레드, 렌더 스레드, GPU 가운데 가장 느린 쪽(병목)이 정한다.</text>')
    write("ladder.svg", svg(y + 34, o))


# ---------- 2. max entities at 60 fps ----------
def max_entities():
    data = {}
    with open(os.path.join(ROOT, "Docs", "figures", "full1", "full-n-at-60fps.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            data[r["series"].split(" ", 1)[1]] = (int(r["n_p95"]), r["note_p95"] == "above range")
    o = []
    y = title(o, "60fps를 지키는 최대 개체 수",
              ["프레임 시간(느린 쪽 5% 경계)이 16.7ms 이하인 가장 큰 호드 총수",
               "오브 시점 카메라, 단계마다 네 가지 호드 총수에서 잰 값 사이를 보간"])
    LX, BX, BR = 32, 290, 640
    NMAX = 30000
    scale = (BR - BX) / NMAX
    y += 40
    top = y - 18
    RH = 44
    for cond, num, name, _ in STEPS:
        n, above = data[cond]
        o.append(f'<text x="{LX}" y="{y + 15}" font-size="17" font-weight="700">{num}단계 {esc(name)}</text>')
        w = n * scale
        if above:
            o.append(f'<rect x="{BX}" y="{y}" width="{w:.1f}" height="22" rx="3" fill="{BLUE}" fill-opacity="0.35" stroke="{BLUE}" stroke-dasharray="5 3"/>')
            o.append(f'<path d="M{BX + w + 4:.1f} {y + 1} L{BX + w + 16:.1f} {y + 11} L{BX + w + 4:.1f} {y + 21}" fill="{BLUE}"/>')
            o.append(f'<text x="{BX + w + 24:.1f}" y="{y + 17}" font-size="16" font-weight="700" fill="{BLUE}">{n // 10000}만 이상</text>')
        else:
            o.append(f'<rect x="{BX}" y="{y}" width="{max(w, 2):.1f}" height="22" rx="3" fill="{BLUE}"/>')
            o.append(f'<text x="{BX + max(w, 2) + 8:.1f}" y="{y + 17}" font-size="16" font-weight="700">{n:,}</text>')
        y += RH
    x = BX + N * scale
    o.append(f'<line x1="{x:.1f}" y1="{top}" x2="{x:.1f}" y2="{y - 14}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<text x="{x:.1f}" y="{y + 6}" text-anchor="middle" font-size="16" font-weight="700" fill="{AMBER}">목표 1만</text>')
    y += 30
    o.append(f'<line x1="32" y1="{y - 14}" x2="{W - 32}" y2="{y - 14}" stroke="{LINE}"/>')
    o.append(f'<text x="32" y="{y + 12}" font-size="16" fill="{SUB}">점선 막대: 잰 범위의 끝인 3만에서도 16.7ms 안이라 실제 최대는 더 크다.</text>')
    write("max-entities.svg", svg(y + 34, o))


# ---------- 3. ablation ----------
def ablation():
    mets = [("프레임 시간", "frame_p50_ms", DARK), ("게임 스레드", "game_p50_ms", BLUE), ("GPU", "gpu_p50_ms", PINK)]
    o = []
    base = {col: val(BASE, col) for _, col, _ in mets}
    y = title(o, "최종 구성(8단계)에서 기법 하나를 뺐을 때의 변화",
              ["호드 총수 1만, 오브 시점 카메라, 세 번 잰 중앙값의 차이(ms)",
               f"0은 최종 구성: 프레임 시간 {fmt(base['frame_p50_ms'])}, 게임 스레드 {fmt(base['game_p50_ms'])}, GPU {fmt(base['gpu_p50_ms'])}"])
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
    for cond, name, lines in ABLATIONS:
        diffs = {}
        for _, col, _ in mets:
            d = val(cond, col) - base[col]
            diffs[col] = (d, abs(d) > max(rng(cond, col), rng(BASE, col)))
        only_game = (not diffs["frame_p50_ms"][1]) and diffs["game_p50_ms"][1]
        sub = list(lines) + (["프레임은 그대로, 게임 스레드만"] if only_game else [])
        label(o, LX, y + 15, name, sub)
        if only_game:  # recolour the tag line
            o[-1] = o[-1].replace(f'fill="{SUB}"', f'fill="{BLUE}" font-weight="700"')
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


if __name__ == "__main__":
    ladder()
    max_entities()
    ablation()
    transition()
    print("wrote", ", ".join(sorted(os.listdir(OUT))), "to", os.path.relpath(OUT, ROOT))
