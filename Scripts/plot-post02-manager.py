# Makes the four extra SVG figures of post 02 in Posts/02-mass-logic/images/.
# Run from the repository root: py -3 -I Scripts/plot-post02-manager.py
#   mass-gain.svg           Measurements/Full3/summary.csv, Docs/figures/full3/full-n-at-60fps.csv
#   manager-diagram.svg     no numbers
#   manager-times.svg       Measurements/Mgr1/summary.csv (reference batch, three runs: median and min-max)
#   manager-gamethread.svg  Measurements/MgrT1/timers.csv (one trace per condition, ms per frame)
# It does not touch the figures that Posts/02-mass-logic/make-figures.py writes. Standard library only.
import csv
import os
import re
import sys
from decimal import ROUND_HALF_UP, Decimal

sys.dont_write_bytecode = True
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
OUT = os.path.join(ROOT, "Posts", "02-mass-logic", "images")

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, PURPLE, GRAY, AMBER, RED, TEAL, PINK = (
    "#2563eb", "#9333ea", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777")
SKY = "#0369a1"  # stage 2 with the processors on one thread
W = 800
BUDGET = 16.667


def read(*parts):
    with open(os.path.join(ROOT, *parts), encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


FULL = {(r["variant"], int(r["n"]), r["preset"]): r for r in read("Measurements", "Full3", "summary.csv")}
N60 = {r["series"]: int(r["n_p95"]) for r in read("Docs", "figures", "full3", "full-n-at-60fps.csv")}
MGR = {(r["variant"], int(r["n"])): r for r in read("Measurements", "Mgr1", "summary.csv")}
TRACE = {(r["cond"], int(r["n"]), r["name"]): float(r["incl_ms"])
         for r in read("Measurements", "MgrT1", "timers.csv") if r["thread"] == "GT"}


def svg(h, body):
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">',
            f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>']
    return "\n".join(head + body + ["</svg>"]) + "\n"


def write(name, text):
    with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def fmt(x):
    """Three significant figures, as in the post body. 27.0 is written 27."""
    places = 0 if abs(x) >= 100 else 1 if abs(x) >= 10 else 2 if abs(x) >= 1 else 3
    # Round half up on the decimal text (5.265 -> 5.27), as the other post figures do.
    s = str(Decimal(repr(round(x, 6))).quantize(Decimal(1).scaleb(-places), rounding=ROUND_HALF_UP))
    return re.sub(r"[.]0+$", "", s)


def title(o, t, s):
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">{t}</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">{s}</text>')


# ---------- 1. actor crowd against Mass on linear axes (final measurement) ----------
def gain():
    o = []
    title(o, "1단계 액터 군중과 2단계 Mass 로직", "오브 시점 카메라, 최종 측정, 세 번 잰 중앙값. 막대 길이는 값에 비례한다")
    n_a, n_m = N60["1 L1-full"], N60["2 L2-mass"]
    per_a = float(FULL[("L1-full", 1000, "P1")]["game_p50_ms"]) / 1000 * 1000
    per_m = float(FULL[("L2-mass", 10000, "P1")]["game_p50_ms"]) / 10000 * 1000
    X0, BW = 40, 560

    def panel(y, head, note, rows, vmax, verdict):
        o.append(f'<text x="32" y="{y}" font-size="18" font-weight="700">{head}</text>')
        o.append(f'<text x="32" y="{y + 24}" font-size="16" fill="{SUB}">{note}</text>')
        y += 44
        for name, val, text, col in rows:
            w = max(val / vmax * BW, 3)
            o.append(f'<text x="{X0}" y="{y + 14}" font-size="16">{name}</text>')
            o.append(f'<rect x="{X0}" y="{y + 22}" width="{w:.1f}" height="30" rx="4" fill="{col}"/>')
            o.append(f'<text x="{X0 + w + 10:.1f}" y="{y + 44}" font-size="20" font-weight="700" fill="{col}">{text}</text>')
            y += 68
        o.append(f'<text x="{X0}" y="{y + 10}" font-size="18" font-weight="700">{verdict}</text>')
        return y + 34

    y = panel(118, "60fps를 지키는 최대 개체 수", "프레임 시간의 느린 쪽 5% 경계가 16.7ms 안에 드는 가장 큰 호드 총수",
              [("1단계 액터 군중", n_a, f"{n_a:,}", RED), ("2단계 Mass 로직", n_m, f"{n_m:,}", BLUE)],
              20000, f"{fmt(n_m / n_a)}배")
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y = panel(y + 38, "개체 하나당 게임 스레드 시간(µs)", "게임 스레드 시간 ÷ 호드 총수. 1단계는 호드 총수 1천, 2단계는 1만의 값",
              [("1단계 액터 군중", per_a, fmt(per_a), RED), ("2단계 Mass 로직", per_m, fmt(per_m), BLUE)],
              18, f"{per_a / per_m:.0f}분의 1")
    write("mass-gain.svg", svg(y + 6, o))
    return n_m / n_a, per_a / per_m


# ---------- 2. diagram: tickless actors and one manager against Mass processors (no numbers) ----------
def diagram():
    o = []
    title(o, "같은 2단계 화면을 만드는 두 방식", "개체의 데이터를 두는 곳과 그것을 도는 코드가 다르다. 이동·피해의 식과 ISM 설정은 같다")
    LX, RX, CW = 32, 416, 352
    o.append(f'<text x="{LX}" y="114" font-size="18" font-weight="700" fill="{PURPLE}">참고: 틱 없는 액터와 매니저 하나</text>')
    o.append(f'<text x="{RX}" y="114" font-size="18" font-weight="700" fill="{BLUE}">2단계: Mass</text>')

    def box(x, y, head, lines, col, tag=None):
        h = 38 + 24 * len(lines) + 8
        o.append(f'<rect x="{x}" y="{y}" width="{CW}" height="{h}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
        o.append(f'<rect x="{x}" y="{y}" width="6" height="{h}" rx="3" fill="{col}"/>')
        o.append(f'<text x="{x + 18}" y="{y + 26}" font-size="17" font-weight="700">{head}</text>')
        if tag:
            o.append(f'<text x="{x + CW - 12}" y="{y + 26}" text-anchor="end" font-size="16" font-weight="700" fill="{col}">{tag}</text>')
        for k, t in enumerate(lines):
            o.append(f'<text x="{x + 18}" y="{y + 52 + 24 * k}" font-size="16" fill="{SUB}">{t}</text>')
        return y + h

    def arrow(x, y):
        cx = x + CW / 2
        o.append(f'<path d="M{cx} {y + 4} L{cx} {y + 18} M{cx - 6} {y + 12} L{cx} {y + 20} L{cx + 6} {y + 12}" '
                 f'stroke="{SUB}" stroke-width="2" fill="none"/>')
        return y + 26

    y = box(LX, 132, "액터, 개체마다 하나", ["틱 함수도 컴포넌트도 없다", "위치·속도·체력·상태를 멤버로 든다"], PURPLE)
    y = arrow(LX, y)
    y = box(LX, y, "매니저의 틱 하나", ["액터 포인터 배열을 한 번 돈다", "개체마다 접근·조향·이동·회전,", "피해, 상태 시간을 차례로 계산한다",
                                  "끝에 그 개체의 ISM 위치를 바로 쓴다"], PURPLE, "게임 스레드")
    y = arrow(LX, y)
    left_end = box(LX, y, "ISM 컴포넌트 둘", ["종류마다 하나. 2단계와 같은 설정"], GRAY)

    y = box(RX, 132, "엔티티", ["번호일 뿐이다", "데이터는 Chunk의 Fragment 배열에 있다"], BLUE)
    y = arrow(RX, y)
    y = box(RX, y, "로직 Processor", ["접근·조향·이동·회전, 범위 피해", "Processor마다 Chunk를 따로 돈다"], TEAL, "워커 스레드")
    y = arrow(RX, y)
    y = box(RX, y, "표현 쪽 Processor 넷", ["카메라 거리 수집, 표현 LOD 계산,", "표현 전환, ISM 위치 쌓기"], BLUE, "게임 스레드")
    y = arrow(RX, y)
    y = box(RX, y, "쌓은 위치를 올린다", ["처리 단계가 끝날 때 컴포넌트에 한 번"], PINK, "게임 스레드")
    y = arrow(RX, y)
    right_end = box(RX, y, "ISM 컴포넌트 둘", ["종류마다 하나"], GRAY)

    y = max(left_end, right_end) + 20
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    o.append(f'<text x="{LX}" y="{y + 30}" font-size="16" fill="{SUB}">표현 LOD 계산과 표현 전환이 없다</text>')
    o.append(f'<text x="{RX}" y="{y + 30}" font-size="16" fill="{SUB}">뒤 단계의 표현 전환이 이 경로를 쓴다</text>')
    write("manager-diagram.svg", svg(y + 50, o))


CONDS = [("L2-manager", "틱 없는 액터와 매니저", PURPLE),
         ("L2-mass", "2단계 Mass 로직", BLUE),
         ("L2-massserial", "2단계, Processor를 한 스레드에", SKY)]


# ---------- 3. reference measurement: game thread and frame time, three conditions ----------
def times():
    o = []
    title(o, "참고 측정: 매니저 방식과 2단계", "오브 시점 카메라, 같은 빌드에서 따로 잰 참고 측정. 세 번 잰 중앙값과 최소–최대(ms)")
    X0, SC = 40, 620 / 20.0
    y = 112

    def panel(y, head, col_name, line):
        top = y
        o.append(f'<text x="32" y="{y}" font-size="18" font-weight="700">{head}</text>')
        y += 12
        for n, nlab in [(10000, "호드 총수 1만"), (20000, "호드 총수 2만")]:
            o.append(f'<text x="{X0}" y="{y + 20}" font-size="16" fill="{SUB}">{nlab}</text>')
            y += 30
            for var, _, col in CONDS:
                r = MGR[(var, n)]
                med, lo, hi = (float(r[col_name + s]) for s in ("", "_min", "_max"))
                w = med * SC
                o.append(f'<rect x="{X0}" y="{y}" width="{w:.1f}" height="24" rx="3" fill="{col}"/>')
                cy = y + 12
                o.append(f'<path d="M{X0 + lo * SC:.1f} {cy} L{X0 + hi * SC:.1f} {cy} M{X0 + lo * SC:.1f} {cy - 6} L{X0 + lo * SC:.1f} {cy + 6} '
                         f'M{X0 + hi * SC:.1f} {cy - 6} L{X0 + hi * SC:.1f} {cy + 6}" stroke="{INK}" stroke-width="2" fill="none"/>')
                text = f'<tspan font-weight="700">{fmt(med)}</tspan> ({fmt(lo)}–{fmt(hi)})'
                if w >= 200:
                    o.append(f'<text x="{X0 + 10}" y="{y + 18}" font-size="16" fill="#ffffff">{text}</text>')
                else:
                    o.append(f'<text x="{X0 + hi * SC + 10:.1f}" y="{y + 18}" font-size="16" fill="{col}">{text}</text>')
                y += 30
            y += 4
        if line:
            x = X0 + BUDGET * SC
            o.append(f'<line x1="{x:.1f}" y1="{top + 16}" x2="{x:.1f}" y2="{y}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
            o.append(f'<text x="{x + 6:.1f}" y="{top + 30}" font-size="16" font-weight="700" fill="{AMBER}">16.7ms</text>')
        return y + 8

    y = panel(y, "게임 스레드 시간(중앙값)", "game_p50_ms", False)
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y = panel(y + 34, "프레임 시간(중앙값)", "frame_p50_ms", True)
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    y += 28
    for k, (_, lab, col) in enumerate(CONDS):
        o.append(f'<rect x="{X0}" y="{y - 14 + 26 * k}" width="18" height="18" rx="3" fill="{col}"/>')
        o.append(f'<text x="{X0 + 28}" y="{y + 26 * k}" font-size="16">{lab}</text>')
    y += 26 * len(CONDS) + 6
    o.append(f'<text x="{X0}" y="{y}" font-size="16" fill="{SUB}">세 조건 모두 GPU가 가장 느리다. 프레임 시간의 차이는 세 번 잰 값의 흔들림보다 작다.</text>')
    write("manager-times.svg", svg(y + 22, o))


# ---------- 4. trace: what the game thread does inside the crowd update, 20k ----------
def trace_parts(n):
    def t(c, name):
        return TRACE.get((c, n, name), 0.0)

    rep = ["MassVisualizationLODProcessor", "MassUpdateISMProcessor", "MassLODCollectorProcessor", "MassVisualizationProcessor"]
    logic = ["MassSmoothOrientationProcessor", "MassSteerToMoveTargetProcessor", "MassApplyMovementProcessor",
             "MassApplyForceProcessor", "LabApproachProcessor", "LabOrbContactProcessor", "LabNearOrbCountProcessor"]
    pre = "FMassProcessingPhase::ExecuteTick EMassProcessingPhase::PrePhysics"
    post = "FMassProcessingPhase::ExecuteTick EMassProcessingPhase::PostPhysics"
    end = "MassVisualizationComponent EndVisualChanges"
    out = {}
    for c in ("mass", "massserial"):
        r = sum(t(c, x) for x in rep)
        lg = sum(t(c, x) for x in logic)
        total = t(c, pre) + t(c, post)
        out[c] = {"logic": lg, "rep": r, "end": t(c, end), "other": total - lg - r - t(c, end), "total": total,
                  "tick_engine": t(c, "Tick_Engine")}
    out["manager"] = {"tick": t("manager", "LabCrowdManager Tick"), "tick_engine": t("manager", "Tick_Engine")}
    return out


def gamethread():
    o = []
    p = trace_parts(20000)
    title(o, "게임 스레드가 군중을 갱신하는 데 쓴 시간", "호드 총수 2만, 오브 시점 카메라, 조건마다 트레이스 한 번. 프레임 평균(ms)")
    X0, SC = 40, 600 / 12.0
    y = 108

    def bar(y, name, segs, total):
        o.append(f'<text x="{X0}" y="{y + 16}" font-size="17" font-weight="700">{name}</text>')
        x = X0
        for lab, val, col in segs:
            w = val * SC
            o.append(f'<rect x="{x:.1f}" y="{y + 28}" width="{w:.1f}" height="40" fill="{col}"/>')
            if lab:
                o.append(f'<text x="{x + 8:.1f}" y="{y + 46}" font-size="16" fill="#ffffff">{lab}</text>')
                o.append(f'<text x="{x + 8:.1f}" y="{y + 64}" font-size="16" font-weight="700" fill="#ffffff">{fmt(val)}</text>')
            x += w
        o.append(f'<text x="{x + 10:.1f}" y="{y + 54}" font-size="18" font-weight="700">{fmt(total)}</text>')
        return y + 94

    m, s = p["mass"], p["massserial"]
    y = bar(y, "2단계 Mass 로직", [("표현 쪽 Processor 넷", m["rep"], BLUE), (None, m["other"], GRAY),
                               ("ISM 위치를 컴포넌트에 올린다", m["end"], PINK)], m["total"])
    o.append(f'<text x="{X0}" y="{y - 8}" font-size="16" fill="{SUB}">로직 Processor는 워커 스레드에서 돌아 이 막대에 없다.</text>')
    y += 22
    y = bar(y, "2단계, Processor를 한 스레드에", [("로직 Processor", s["logic"], TEAL), ("표현 쪽", s["rep"], BLUE), (None, s["other"], GRAY),
                                         ("ISM 위치를 컴포넌트에 올린다", s["end"], PINK)], s["total"])
    y += 4
    y = bar(y, "틱 없는 액터와 매니저", [("매니저의 틱 하나(로직과 ISM 위치 쓰기)", p["manager"]["tick"], PURPLE)], p["manager"]["tick"])
    o.append(f'<text x="{X0}" y="{y - 8}" font-size="16" fill="{SUB}">틱 안을 나눠 재지 못했다. 표현 LOD 계산과 표현 전환은 없다.</text>')
    y += 14
    o.append(f'<line x1="32" y1="{y}" x2="{W - 32}" y2="{y}" stroke="{LINE}"/>')
    o.append(f'<text x="{X0}" y="{y + 28}" font-size="16" fill="{SUB}">표현 쪽 Processor 넷: 카메라 거리 수집, 표현 LOD 계산, 표현 전환, ISM 위치 쌓기.</text>')
    o.append(f'<text x="{X0}" y="{y + 52}" font-size="16" fill="{SUB}">회색은 처리 단계의 나머지 시간이다.</text>')
    write("manager-gamethread.svg", svg(y + 72, o))
    return p


def report(ratios, p):
    print(f"n60 ratio {ratios[0]:.2f}, per-entity ratio {ratios[1]:.2f}")
    for n in (10000, 20000):
        g = {v: float(MGR[(v, n)]["game_p50_ms"]) for v, _, _ in CONDS}
        f = {v: float(MGR[(v, n)]["frame_p50_ms"]) for v, _, _ in CONDS}
        print(n, "game manager-mass", round(g["L2-manager"] - g["L2-mass"], 3), "serial-mass", round(g["L2-massserial"] - g["L2-mass"], 3),
              "frame spread", round(max(f.values()) - min(f.values()), 3),
              "per-entity us manager/mass", round(g["L2-manager"] / n * 1000, 4), round(g["L2-mass"] / n * 1000, 4),
              "bottleneck", [MGR[(v, n)]["bottleneck"] for v, _, _ in CONDS])
        q = trace_parts(n)
        d_tick = q["manager"]["tick"] - q["mass"]["total"]
        d_eng = q["manager"]["tick_engine"] - q["mass"]["tick_engine"]
        print(n, "trace", {k: {a: round(b, 4) for a, b in v.items()} for k, v in q.items()})
        print(n, "trace manager tick - mass phases", round(d_tick, 4), "Tick_Engine diff", round(d_eng, 4), "share", round(d_tick / d_eng, 4),
              "serial-parallel Tick_Engine", round(q["massserial"]["tick_engine"] - q["mass"]["tick_engine"], 4))


ratios = gain()
diagram()
times()
parts = gamethread()
report(ratios, parts)
print("wrote mass-gain.svg manager-diagram.svg manager-times.svg manager-gamethread.svg")
