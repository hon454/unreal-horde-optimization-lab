# Makes the SVG figures of the reference batch Budget1 in Docs/figures/budget1/ (Docs/Worklog/Q7.md "측정: Budget1").
# Run: py -3 -I Scripts/plot-budget.py
# Input: Measurements/Budget1/summary.csv (median of three runs, _min/_max = range). Output: one SVG per metric group plus budget-values.csv.
# Each figure has two panels, horde 10,000 (left) and 20,000 (right). X axis = animation budget (off, 1, 2, 3, 4, 6 ms), evenly spaced.
# Dot = median, vertical bar = range of the three runs. Colours and fonts follow Scripts/plot-readme.py.
import csv, os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
SRC = os.path.join(ROOT, "Measurements", "Budget1", "summary.csv")
OUT = os.path.join(ROOT, "Docs", "figures", "budget1")

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG = "#111827", "#4b5563", "#e5e7eb", "#ffffff"
BLUE, TEAL, PINK, AMBER, DARK = "#2563eb", "#0d9488", "#db2777", "#b45309", "#374151"
W = 800
TARGET_MS = 1000.0 / 60.0
BUDGETS = [0, 1, 2, 3, 4, 6]
NS = [10000, 20000]


def fmt(v, digits=2):
    s = f"{v:.{digits}f}"
    if "." in s:
        s = s.rstrip("0").rstrip(".")
    return s


def xlabel(b):
    return "끔" if b == 0 else str(b)


def load():
    rows = {}
    with open(SRC, newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if r["preset"] != "P1":
                continue
            b = int(r["variant"].split("-")[0][1:])
            rows[(b, int(r["n"]))] = r
    return rows


def val(r, col):
    try:
        return float(r[col])
    except (KeyError, ValueError, TypeError):
        return None


def axis_max(vmax):
    # a round upper bound with 4 equal grid steps
    for unit in (0.05, 0.1, 0.2, 0.25, 0.5, 1, 2, 2.5, 5, 10, 20, 25, 50, 100, 250, 500, 1000):
        if vmax <= unit * 4:
            return unit * 4, unit
    return vmax, vmax / 4


def figure(name, title, ylabel, series, hline=None, digits=2, ymax=None):
    """series: list of (label, color, median column, min column or None, max column or None)."""
    rows = load()
    pw, ph = 340, 250           # panel size
    left, top = 56, 96
    H = top + ph + 74
    vmax = 0.0
    for key, r in rows.items():
        for _, _, c, lo, hi in series:
            for col in (c, hi):
                v = val(r, col) if col else None
                if v is not None:
                    vmax = max(vmax, v)
    if hline:
        vmax = max(vmax, hline * 1.05)
    top_v, unit = axis_max(ymax if ymax else vmax * 1.08)
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" font-family="{FONT}">',
           f'<rect width="{W}" height="{H}" fill="{BG}"/>',
           f'<text x="16" y="30" font-size="20" font-weight="700" fill="{INK}">{title}</text>',
           f'<text x="16" y="54" font-size="16" fill="{SUB}">{ylabel}</text>']
    # legend
    lx = 16
    for label, color, *_ in series:
        svg.append(f'<circle cx="{lx + 6}" cy="76" r="6" fill="{color}"/>')
        svg.append(f'<text x="{lx + 18}" y="82" font-size="16" fill="{INK}">{label}</text>')
        lx += 30 + 11 * len(label) + 18
    for pi, n in enumerate(NS):
        ox = 16 + pi * (pw + 40) + left - 16 + 0
        ox = 20 + pi * (pw + 48) + 44
        svg.append(f'<text x="{ox + pw / 2}" y="{top - 8}" font-size="16" font-weight="700" fill="{INK}" text-anchor="middle">호드 {n:,}</text>')
        # grid
        for g in range(5):
            gv = unit * g
            y = top + ph - ph * gv / top_v
            svg.append(f'<line x1="{ox}" y1="{y:.1f}" x2="{ox + pw}" y2="{y:.1f}" stroke="{LINE}"/>')
            svg.append(f'<text x="{ox - 6}" y="{y + 5:.1f}" font-size="16" fill="{SUB}" text-anchor="end">{fmt(gv, digits)}</text>')
        if hline:
            y = top + ph - ph * hline / top_v
            svg.append(f'<line x1="{ox}" y1="{y:.1f}" x2="{ox + pw}" y2="{y:.1f}" stroke="{AMBER}" stroke-width="2" stroke-dasharray="6 4"/>')
            svg.append(f'<text x="{ox + pw * 0.42:.1f}" y="{y - 6:.1f}" font-size="16" fill="{AMBER}" text-anchor="middle">16.7ms</text>')
        step = pw / len(BUDGETS)
        for i, b in enumerate(BUDGETS):
            cx = ox + step * (i + 0.5)
            svg.append(f'<text x="{cx:.1f}" y="{top + ph + 24}" font-size="16" fill="{SUB}" text-anchor="middle">{xlabel(b)}</text>')
        svg.append(f'<text x="{ox + pw / 2}" y="{top + ph + 52}" font-size="16" fill="{SUB}" text-anchor="middle">예산(ms)</text>')
        for label, color, c, lo, hi in series:
            pts = []
            for i, b in enumerate(BUDGETS):
                r = rows.get((b, n))
                v = val(r, c) if r else None
                if v is None:
                    continue
                cx = ox + step * (i + 0.5)
                y = top + ph - ph * v / top_v
                pts.append((cx, y))
                l, h = (val(r, lo) if lo else None), (val(r, hi) if hi else None)
                if l is not None and h is not None and h > l:
                    y1 = top + ph - ph * l / top_v
                    y2 = top + ph - ph * h / top_v
                    svg.append(f'<line x1="{cx:.1f}" y1="{y1:.1f}" x2="{cx:.1f}" y2="{y2:.1f}" stroke="{color}" stroke-width="3" opacity="0.45"/>')
            if len(pts) > 1:
                svg.append('<polyline fill="none" stroke="%s" stroke-width="2" points="%s"/>' % (color, " ".join(f"{x:.1f},{y:.1f}" for x, y in pts)))
            for x, y in pts:
                svg.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4.5" fill="{color}"/>')
    svg.append("</svg>")
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name), "w", encoding="utf-8") as f:
        f.write("\n".join(svg))
    print("wrote", name)


def main():
    figure("budget-frame.svg", "프레임 시간", "ms. 점은 중앙값, 세로선은 세 실행의 범위",
           [("p50", DARK, "frame_p50_ms", "frame_p50_ms_min", "frame_p50_ms_max"),
            ("p95", PINK, "frame_p95_ms", "frame_p95_ms_min", "frame_p95_ms_max")], hline=TARGET_MS, ymax=20)
    figure("budget-skipped.svg", "애니메이션을 건너뛴 액터 비율", "프레임당 비율(보간한 액터는 뺀다)",
           [("대형", BLUE, "large_skipped_ratio", "large_skipped_ratio_min", "large_skipped_ratio_max"),
            ("소형", TEAL, "small_skipped_ratio", "small_skipped_ratio_min", "small_skipped_ratio_max")], digits=2)
    figure("budget-interp.svg", "애니메이션을 보간한 액터 비율", "프레임당 비율",
           [("대형", BLUE, "large_interp_ratio", "large_interp_ratio_min", "large_interp_ratio_max"),
            ("소형", TEAL, "small_interp_ratio", "small_interp_ratio_min", "small_interp_ratio_max")], digits=2)
    figure("budget-threads.svg", "스레드 시간과 예산 사용량", "ms(중앙값). 세로선은 세 실행의 범위",
           [("게임 스레드", BLUE, "game_p50_ms", "game_p50_ms_min", "game_p50_ms_max"),
            ("GPU", PINK, "gpu_p50_ms", "gpu_p50_ms_min", "gpu_p50_ms_max"),
            ("BudgetedAnimation", TEAL, "budgeted_anim_ms", "budgeted_anim_ms_min", "budgeted_anim_ms_max")], digits=1)
    # value table next to the figures
    rows = load()
    cols = ["frame_p50_ms", "frame_p50_ms_min", "frame_p50_ms_max", "frame_p95_ms", "frame_p95_ms_min", "frame_p95_ms_max",
            "game_p50_ms", "gpu_p50_ms", "budgeted_anim_ms", "large_skipped_ratio", "large_skipped_ratio_min", "large_skipped_ratio_max",
            "large_interp_ratio", "small_skipped_ratio", "small_interp_ratio", "actors_mean", "view_actors_mean"]
    with open(os.path.join(OUT, "budget-values.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["budget_ms", "n", "runs"] + cols)
        for n in NS:
            for b in BUDGETS:
                r = rows.get((b, n))
                if r:
                    w.writerow([b, n, r["runs"]] + [r.get(c, "") for c in cols])
    print("wrote budget-values.csv")


if __name__ == "__main__":
    main()
