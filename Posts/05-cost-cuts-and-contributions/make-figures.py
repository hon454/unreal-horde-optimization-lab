# Makes the SVG figures of post 05 in images/.
# Run from the repository root: py -3 -I Posts/05-cost-cuts-and-contributions/make-figures.py
# Measured values come from Measurements/Full4/summary.csv (the full batch, package de0e21b) and the distinguishability
# column of Docs/figures/full4/full-ablation-moving.csv (Scripts/plot.py distinguishable()).
# The entity pool long-run figure reads Measurements/Pool2/timeline.csv, a reference batch (same package, one run per
# condition). It is drawn on its own and never joined with the full batch values.
# The entity pool diagram is not made here: the post reuses Posts/06-engine-findings/images/entity-pool.svg
# (no measured values, made by that post's make-figures.py), as the W14 plan asks for one diagram in posts 05 and 06.
import csv
import os
import re
from decimal import Decimal, ROUND_HALF_UP

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "images")

BATCH = "Full4"
TARGET_N = 10000
BUDGET_MS = 16.7             # Scripts/plot.py BUDGET_MS
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
    """Difference b - a taken from the CSV text with Decimal, so 12.686 - 11.881 is exactly 0.805 and rounds half up."""
    return Decimal(row(b)[col]) - Decimal(row(a)[col])


with open(os.path.join(ROOT, "Docs", "figures", BATCH.lower(), "full-ablation-moving.csv"), encoding="utf-8") as f:
    ABL = {r["cond"]: r for r in csv.DictReader(f)}

with open(os.path.join(ROOT, "Measurements", "Pool2", "timeline.csv"), encoding="utf-8") as f:
    TIMELINE = list(csv.DictReader(f))


def distinguishable(a, b, key):
    """Same rule as Scripts/plot.py distinguishable(): the median difference is at least 0.5 ms and larger than the larger run range."""
    d = abs(v(a, key) - v(b, key))
    rng = lambda var: v(var, key + "_max") - v(var, key + "_min")
    return d >= DISTINCT_FLOOR_MS and d > max(rng(a), rng(b))


# ---------- formatting (posting.md: three significant figures, 27 not 27.0) ----------
def _no_zero_decimals(fn):
    return lambda *a, **k: re.sub(r"[.]0+$", "", fn(*a, **k))


@_no_zero_decimals
def fmt(x):
    """Three significant figures, rounded half up from the CSV text (12.35 -> 12.4)."""
    q = "1" if abs(x) >= 100 else "0.1" if abs(x) >= 10 else "0.01"
    return str(Decimal(repr(x)).quantize(Decimal(q), rounding=ROUND_HALF_UP))


def fmtd(x):
    """Signed difference in ms with two decimals (as the batch figure CSV), minus sign U+2212. 0.00 stays 0.
    x is a Decimal from dv() so that a difference such as 0.805 rounds half up to 0.81."""
    x = x if isinstance(x, Decimal) else Decimal(repr(x))
    s = str(abs(x).quantize(Decimal("0.01"), rounding=ROUND_HALF_UP))
    if s == "0.00":
        return "0"
    s = re.sub(r"[.]0+$", "", s)
    return ("+" if x > 0 else "−") + s


def cnt(x):
    """A count, never shortened (posting.md): 120428.0 -> 120,428."""
    return f"{int(Decimal(repr(x)).quantize(Decimal('1'), rounding=ROUND_HALF_UP)):,}"


def big3(x):
    """A large measured value that is not a count, to three significant figures: 5105.3 MB -> 5,110."""
    d = Decimal(repr(x))
    exp = d.adjusted() - 2
    q = d.scaleb(-exp).quantize(Decimal("1"), rounding=ROUND_HALF_UP).scaleb(exp)
    return f"{int(q):,}"


def esc(t):
    return str(t).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


# ---------- style (same as the other posts) ----------
FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
MONO = "Consolas, SFMono-Regular, Menlo, monospace"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, PURPLE, GRAY, AMBER, RED, TEAL, PINK, DARK, GREEN = (
    "#2563eb", "#9333ea", "#9ca3af", "#b45309", "#dc2626", "#0d9488", "#db2777", "#374151", "#15803d")
NAVY, LIGHT = "#1f2937", "#cbd5e1"
W = 800


def svg(h, body):
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">',
            f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>']
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


def tw(t, size=16):
    """Rough text width: Hangul is one em, Latin and digits about 0.56 em."""
    w = 0.0
    for ch in t:
        if "가" <= ch <= "힣":
            w += size
        elif ch == " ":
            w += size * 0.3
        elif ch in ".,:()·→−+":
            w += size * 0.4
        else:
            w += size * 0.58
    return w


def legend(o, x0, y, width, entries):
    """entries: (shape, color, label). Wraps to a new line when the row is full. Returns the y below the legend."""
    x = x0
    for shape, color, label in entries:
        w = 24 + tw(label) + 22
        if x + w > x0 + width and x > x0:
            x, y = x0, y + 24
        if shape == "box":
            o.append(f'<rect x="{x}" y="{y-12}" width="16" height="14" rx="2" fill="{color}"/>')
        elif shape == "dash":
            o.append(f'<line x1="{x}" y1="{y-5}" x2="{x+18}" y2="{y-5}" stroke="{color}" stroke-width="2" stroke-dasharray="5 4"/>')
        elif shape == "dot":
            o.append(f'<line x1="{x}" y1="{y-5}" x2="{x+18}" y2="{y-5}" stroke="{color}" stroke-width="2" stroke-dasharray="2 3"/>')
        elif shape == "line":
            o.append(f'<line x1="{x}" y1="{y-5}" x2="{x+18}" y2="{y-5}" stroke="{color}" stroke-width="3"/>')
        o.append(text(x + 24, y, label, fill=SUB))
        x += w
    return y + 24


CHECK = []  # (figure, label, value text) for the scratch comparison against the CSV


# ---------- 1. steps 5 and 8: frame p95 and thread medians ----------
def steps():
    o = []
    y = title(o, "비용을 줄이는 두 단계에서 바뀐 시간",
              ["호드 총수 1만, 오브 시점 카메라, 이동 장면, 세 번 잰 중앙값(ms)",
               "프레임 시간은 p95, 스레드 시간은 p50이다. 회색 글자는 구별되지 않는 차이다"])
    y = legend(o, 32, y + 10, W - 64, [("box", LIGHT, "앞 단계"), ("box", NAVY, "이 단계"),
                                       ("dash", AMBER, "16.7ms(60fps)")])
    X0, PX = 170, 17.0          # bar start, pixels per ms
    XT = 472                    # value text column (right of the 16.7 ms line at 170 + 16.7 * 17 = 454)
    metrics = [("프레임 p95", "frame_p95_ms"), ("게임 스레드", "game_p50_ms"), ("렌더 스레드", "render_p50_ms"), ("GPU", "gpu_p50_ms")]
    groups = [("단계 5: Simulation LOD와 엔티티 풀 (단계 4 → 5)", "V0-s4", "V0-s5"),
              ("단계 8: 80m 밖 그림자 끔, 액터 메시 고정 바운드 (단계 7 → 8)", "V0-s7", "V0-s8")]
    y += 8
    for head, before, after in groups:
        o.append(text(32, y + 16, head, size=17, weight="700"))
        y += 30
        top = y
        for name, col in metrics:
            a, b = v(before, col), v(after, col)
            d = dv(before, after, col)
            dist = distinguishable(before, after, col)
            o.append(text(32, y + 20, name, fill=INK))
            o.append(f'<rect x="{X0}" y="{y+2}" width="{a*PX:.1f}" height="11" rx="2" fill="{LIGHT}"/>')
            o.append(f'<rect x="{X0}" y="{y+15}" width="{b*PX:.1f}" height="11" rx="2" fill="{NAVY}"/>')
            label = f"{fmt(a)} → {fmt(b)}  ({fmtd(d)})"
            o.append(text(XT, y + 20, label + ("" if dist else "  구별되지 않음"), fill=INK if dist else GRAY,
                          weight="700" if dist else None))
            CHECK.append(("steps", f"{after} {col}", (fmt(a), fmt(b), fmtd(d), dist)))
            y += 34
        x167 = X0 + BUDGET_MS * PX
        o.append(f'<line x1="{x167:.1f}" y1="{top-2}" x2="{x167:.1f}" y2="{y-4}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
        y += 14
    o.append(f'<line x1="32" y1="{y}" x2="{W-32}" y2="{y}" stroke="{LINE}"/>')
    y += 26
    for line in ["프레임 시간은 게임 스레드, 렌더 스레드, GPU 가운데 가장 느린 쪽이 정한다.",
                 "네 단계 모두 게임 스레드가 가장 느리다."]:
        o.append(text(32, y, line, fill=SUB))
        y += 22
    write("steps-5-and-8.svg", svg(y + 8, o))


# ---------- 2. technique contributions: every row, grey when not distinguishable ----------
LABELS = {
    "a8hyd": "하이드레이션 끔 †", "a8far": "200m 밖을 그리지 않는 일 끔", "a8cap": "액터 캡슐 끔", "a8fsb": "액터 메시 고정 바운드 끔",
    "a8sep": "겹침 방지 끔", "a8iskm": "ISKM 끔 †", "a8bnd": "ISKM 고정 바운드 끔", "a8bud": "액터 애니메이션 예산 끔",
    "a8fix": "전환 대책 끔", "a8hold": "최소 유지 시간 끔", "a8pool": "엔티티 풀 끔", "a8sim": "Simulation LOD 끔",
    "a8fx": "배경 무리 끔 †", "a8shd": "80m 밖 그림자 다시 켬", "a8res": "ISKM 트랙 예약 끔", "a8fxf": "입자의 오브 따라가기 끔",
    "a8stp": "ISKM 상자 따라가기 끔", "s8av": "엔진 회피로 바꿈",
}


def note_of(cfg):
    """One line under a row: a benefit that frame time does not show, or the bottleneck change. Values come from summary.csv."""
    base = "V0-s8"
    c = "V0-" + cfg
    if cfg == "a8hyd":
        return f"병목이 GPU로 바뀐다: 게임 스레드 {fmt(v(c, 'game_p50_ms'))}ms, GPU {fmt(v(c, 'gpu_p50_ms'))}ms"
    if cfg == "a8iskm":
        return f"GPU 시간 {fmtd(dv(base, c, 'gpu_p50_ms'))}ms(VAT 인스턴스로 되돌린다)"
    if cfg == "a8sep":
        return f"몸통이 겹친 쌍 {cnt(v(base, 'overlap_mean'))} → {cnt(v(c, 'overlap_mean'))}"
    if cfg == "a8hold":
        return f"왕복 전환 초당 {fmt(v(base, 'sw_round_all'))} → {fmt(v(c, 'sw_round_all'))}건"
    if cfg == "a8pool":
        return f"실행 끝 ISKM 트랙 {cnt(v(base, 'iskm_tracks_last'))} → {cnt(v(c, 'iskm_tracks_last'))}"
    if cfg == "a8shd":
        return f"GPU 시간 {fmtd(dv(base, c, 'gpu_p50_ms'))}ms, 0.5ms 아래라 구별되지 않는다"
    if cfg == "a8fx":
        return f"GPU 시간 {fmtd(dv(base, c, 'gpu_p50_ms'))}ms"
    return ""


def contributions():
    o = []
    y = title(o, "기법별 기여: 최종 구성에서 하나씩 껐을 때",
              ["호드 총수 1만, 오브 시점 카메라, 이동 장면",
               "막대는 프레임 p95(세 번 잰 중앙값), 검은 선은 세 실행의 범위"])
    y = legend(o, 32, y + 10, W - 64, [("box", NAVY, "최종 구성"), ("box", BLUE, "끄면 길어진다: 시간을 줄이는 기법"),
                                       ("box", PURPLE, "끄면 짧아진다: 시간을 쓰는 기법"), ("box", GRAY, "구별되지 않음"),
                                       ("dash", AMBER, "16.7ms(60fps)"), ("dot", NAVY, "최종 구성의 프레임 p95")])
    L, PX = 272, 13.0           # bar start, pixels per ms
    X = lambda ms: L + ms * PX
    base = "V0-s8"
    b95 = v(base, "frame_p95_ms")
    groups = {"dist": [], "same": [], "sub": []}
    for cond, r in ABL.items():
        cfg = cond.split("-", 1)[1]
        mine = distinguishable(cond, base, "frame_p95_ms")
        if mine != (r["p95_distinct"] == "yes"):
            raise SystemExit(f"distinguishability differs from the batch figure CSV for {cond}")
        d = dv(base, cond, "frame_p95_ms")
        if abs(float(d) - float(r["delta_p95_ms"])) > 0.0011:
            raise SystemExit(f"difference differs from the batch figure CSV for {cond}")
        key = "sub" if r["kind"] == "sub" else "dist" if mine else "same"
        groups[key].append((cfg, cond, d))
    for g in groups.values():
        g.sort(key=lambda it: -it[2])
    heads = {"dist": "구별되는 행",
             "same": "구별되지 않는 행: 차이가 0.5ms 아래이거나 세 실행 범위 안",
             "sub": "하위 실험: 기법을 끈 것이 아니라 바꾼 조건"}
    rowh, noteh = 32, 24
    n_rows = 1 + sum(len(g) for g in groups.values())
    n_notes = sum(1 for g in groups.values() for cfg, _, _ in g if note_of(cfg))
    y += 6
    top = y
    bottom = top + rowh * n_rows + noteh * n_notes + 34 * len([g for g in groups.values() if g])
    for k in range(0, 25, 4):
        gx = X(k)
        o.append(f'<line x1="{gx:.1f}" y1="{top}" x2="{gx:.1f}" y2="{bottom}" stroke="#eef0f3"/>')
        o.append(text(gx, bottom + 20, str(k), fill=SUB, anchor="middle"))
    o.append(text(X(12), bottom + 44, "프레임 p95(ms)", fill=SUB, anchor="middle"))
    o.append(f'<line x1="{X(BUDGET_MS):.1f}" y1="{top}" x2="{X(BUDGET_MS):.1f}" y2="{bottom}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
    o.append(f'<line x1="{X(b95):.1f}" y1="{top}" x2="{X(b95):.1f}" y2="{bottom}" stroke="{NAVY}" stroke-width="2" stroke-dasharray="2 3"/>')

    def bar(y, label, cond, color, txt_color, delta):
        p95, lo, hi = v(cond, "frame_p95_ms"), v(cond, "frame_p95_ms_min"), v(cond, "frame_p95_ms_max")
        out = [text(L - 12, y + 19, label, anchor="end", fill=txt_color),
               f'<rect x="{L}" y="{y+5}" width="{X(p95)-L:.1f}" height="18" rx="2" fill="{color}"/>']
        for w, col in ((4, BG), (1.6, INK)):
            out.append(f'<line x1="{X(lo):.1f}" y1="{y+14}" x2="{X(hi):.1f}" y2="{y+14}" stroke="{col}" stroke-width="{w}"/>')
            for xv in (lo, hi):
                out.append(f'<line x1="{X(xv):.1f}" y1="{y+8}" x2="{X(xv):.1f}" y2="{y+20}" stroke="{col}" stroke-width="{w}"/>')
        val = fmt(p95) + ("" if delta is None else f" ({fmtd(delta)})")
        out.append(text(X(max(p95, hi)) + 9, y + 19, val, fill=txt_color, weight=None if txt_color == GRAY else "700", halo=True))
        CHECK.append(("contributions", cond, (fmt(p95), None if delta is None else fmtd(delta), fmt(lo), fmt(hi))))
        return out

    o += bar(y, "최종 구성(단계 8)", base, NAVY, INK, None)
    y += rowh
    for key in ("dist", "same", "sub"):
        if not groups[key]:
            continue
        o.append(text(32, y + 22, heads[key], weight="700", fill=SUB))
        y += 34
        for cfg, cond, d in groups[key]:
            if key == "same":
                color, tc = GRAY, GRAY
            else:
                color, tc = (BLUE if d > 0 else PURPLE), INK
            o += bar(y, LABELS[cfg], cond, color, tc, d)
            y += rowh
            note = note_of(cfg)
            if note:
                o.append(text(L, y + 12, note, fill=SUB, halo=True))
                CHECK.append(("contributions-note", cond, note))
                y += noteh
    y = bottom + 88
    o.append(f'<line x1="32" y1="{y-22}" x2="{W-32}" y2="{y-22}" stroke="{LINE}"/>')
    foot = [f"괄호 안은 최종 구성({fmt(b95)}ms)과의 차이(ms)다. 구별은 차이가 0.5ms 이상이고",
            "두 조건의 세 실행 범위 가운데 큰 쪽보다 클 때만 한다.",
            "80m 밖 그림자는 끈 조건이 아니라 다시 켠 조건이다.",
            "† 참고 값: 다른 것이 함께 빠진다. 하이드레이션 끔은 액터 캡슐, 액터 애니메이션",
            "예산, 액터 메시 고정 바운드가 함께 빠지고 전 구간이 ISKM이 된다. ISKM 끔은",
            "ISKM 고정 바운드, 트랙 예약, 먼 피격 규칙이 빠진다. 배경 무리 끔은 먼 피격 규칙과",
            "맵 밖 무리 그리기가 빠진다. 먼 피격 규칙은 200m 밖에서 맞은 개체를 ISKM으로 그린다."]
    for line in foot:
        o.append(text(32, y, line, fill=SUB))
        y += 22
    write("contributions.svg", svg(y + 8, o))


# ---------- 3. entity pool long run (reference batch, one run per condition) ----------
def pool_long_run():
    series = {}
    for r in TIMELINE:
        cfg = r["label"].split("-")[2]   # Pool2-V0-<cfg>-N10000-P1-r1
        series.setdefault(cfg, []).append(r)
    for cfg in series:
        series[cfg].sort(key=lambda r: int(r["minute"]))
    on, off = series["s8"], series["a8pool"]
    if [r["minute"] for r in on] != [r["minute"] for r in off]:
        raise SystemExit("the two runs have different minute bins")
    minutes = [int(r["minute"]) for r in on]
    o = []
    y = title(o, "엔티티 풀 켬과 끔을 960초 동안 1분마다 본 값",
              ["참고 측정: 최종 구성, 호드 총수 1만, 오브 시점 카메라, 이동 장면",
               "조건마다 한 번 돌렸다. 범위가 없다"])
    y = legend(o, 32, y + 10, W - 64, [("line", BLUE, "엔티티 풀 켬(최종 구성)"), ("line", RED, "엔티티 풀 끔")])
    L, R = 120, 620
    X = lambda m: L + (m - minutes[0]) / (minutes[-1] - minutes[0]) * (R - L)
    panels = [("ISKM 트랙 수(구간 끝)", "iskm_tracks_end", 0, 130000, [0, 50000, 100000], cnt, ""),
              ("GPU 메모리 평균(MB)", "gpu_mem_mean", 3000, 5500, [3000, 4000, 5000], big3, "MB"),
              ("프레임 p95(ms)", "frame_p95", 13, 17, [13, 14, 15, 16, 17], fmt, "ms")]
    ph = 130
    y += 4
    for name, col, lo, hi, ticks, f, unit in panels:
        o.append(text(32, y + 18, name, weight="700"))
        top, bot = y + 32, y + 32 + ph
        Y = lambda val, lo=lo, hi=hi, top=top, bot=bot: bot - (val - lo) / (hi - lo) * (bot - top)
        for t in ticks:
            o.append(f'<line x1="{L}" y1="{Y(t):.1f}" x2="{R}" y2="{Y(t):.1f}" stroke="#eef0f3"/>')
            o.append(text(L - 10, Y(t) + 5, f(t) if col != "frame_p95" else str(t), anchor="end", fill=SUB))
        o.append(f'<line x1="{L}" y1="{bot}" x2="{R}" y2="{bot}" stroke="{GRAY}"/>')
        if col == "frame_p95":
            o.append(f'<line x1="{L}" y1="{Y(BUDGET_MS):.1f}" x2="{R}" y2="{Y(BUDGET_MS):.1f}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="5 4"/>')
            o.append(text(L + 6, Y(BUDGET_MS) - 6, "16.7ms(60fps)", fill=AMBER, size=16))
        ends = []
        for runs, color, who in ((on, BLUE, "켬"), (off, RED, "끔")):
            pts = [(X(int(r["minute"])), Y(float(r[col]))) for r in runs]
            d = " ".join(f'{"M" if i == 0 else "L"}{px:.1f},{py:.1f}' for i, (px, py) in enumerate(pts))
            o.append(f'<path d="{d}" fill="none" stroke="{color}" stroke-width="2.5"/>')
            o += [f'<circle cx="{px:.1f}" cy="{py:.1f}" r="2.5" fill="{color}"/>' for px, py in pts]
            last = float(runs[-1][col])
            ends.append([pts[-1][1], f"{f(last)}{unit} {who}", color])
            CHECK.append(("pool-long-run", f"{col} {who} minute {runs[-1]['minute']}", f(last)))
        ends.sort(key=lambda e: e[0])
        if ends[1][0] - ends[0][0] < 22:     # keep the two end labels apart
            mid = (ends[0][0] + ends[1][0]) / 2
            ends[0][0], ends[1][0] = mid - 11, mid + 11
        for ey, label, color in ends:
            o.append(text(R + 10, ey + 5, label, fill=color, weight="700"))
        y = bot + 28
    y -= 10
    for m in (1, 4, 8, 12, 16):
        o.append(text(X(m), y + 4, str(m), anchor="middle", fill=SUB))
    o.append(text((L + R) / 2, y + 28, "측정 시작부터의 분(1분 구간)", anchor="middle", fill=SUB))
    write("pool-long-run.svg", svg(y + 46, o))


if __name__ == "__main__":
    steps()
    contributions()
    pool_long_run()
    if os.environ.get("LAB_FIG_CHECK"):
        for item in CHECK:
            print(item)
    print("wrote", ", ".join(sorted(os.listdir(OUT))))
