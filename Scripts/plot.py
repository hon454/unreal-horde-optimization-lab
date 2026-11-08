"""Measurements/*/summary.csv로 Docs/figures/의 그래프를 다시 그린다. 표준 라이브러리만 써서 SVG를 직접 쓴다(PC에 matplotlib가 없다).

그리는 것
- n-at-60fps.svg   : 단계·변형별 "60fps(16.7ms)를 지키는 최대 N"(P1). README 대표 그래프. 값은 measurement.md "N@16.7ms" 정의대로
                     Game/Render/GPU/프레임 p50 중 최댓값이 16.7ms를 넘기 직전 N을 측정 N 사이에서 로그 보간한다.
                     프레임 시간을 넣는 이유: Game Thread 시간은 워커 대기를 빼서 워커 병목(E1 massav)을 놓친다(2026-11-08 사용자 결정).
- n-scaling-P1.svg : N에 따른 프레임 병목 ms(Game/Render/GPU/프레임 p50 최댓값), P1, 로그-로그.

실행: py -3 Scripts/plot.py
"""
import csv
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "Docs" / "figures"
BUDGET_MS = 16.7
COLORS = ["#2563eb", "#dc2626", "#16a34a", "#9333ea", "#ea580c", "#0891b2", "#4b5563"]


def load():
    rows = []
    for path in sorted((ROOT / "Measurements").glob("M*/summary.csv")):
        stage = path.parent.name
        with open(path, newline="", encoding="utf-8") as f:
            for r in csv.DictReader(f):
                vals = [float(r[k]) for k in ("game_p50_ms", "render_p50_ms", "gpu_p50_ms", "frame_p50_ms") if r.get(k)]
                if not vals:
                    continue
                rows.append({"stage": stage, "variant": r["variant"], "n": int(r["n"]), "preset": r["preset"], "ms": max(vals)})
    return rows


def series(rows, preset):
    out = {}
    for r in rows:
        if r["preset"] != preset:
            continue
        out.setdefault(f'{r["stage"]} {r["variant"]}', []).append((r["n"], r["ms"]))
    return {k: sorted(v) for k, v in out.items()}


def n_at_budget(points):
    """측정 N 사이를 로그 보간한 16.7ms 교차 N. 처음부터 넘으면 None, 끝까지 안 넘으면 마지막 N 이상(>)."""
    if not points or points[0][1] > BUDGET_MS:
        return None, "below range"
    for (n0, m0), (n1, m1) in zip(points, points[1:]):
        if m0 <= BUDGET_MS < m1:
            t = (BUDGET_MS - m0) / (m1 - m0)
            return math.exp(math.log(n0) + t * (math.log(n1) - math.log(n0))), ""
    return points[-1][0], "above range"


def svg(width, height, body):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" '
            f'font-family="Segoe UI, Arial, sans-serif" font-size="13">\n<rect width="100%" height="100%" fill="white"/>\n{body}</svg>\n')


def bar_chart(data, title, path):
    W, H, L, R, T, B = 760, 360, 170, 40, 50, 40
    maxv = max(v for _, v, _ in data) * 1.15
    bh = (H - T - B) / max(len(data), 1)
    body = [f'<text x="{W/2}" y="28" text-anchor="middle" font-size="16" font-weight="600">{title}</text>']
    for i, (name, v, note) in enumerate(data):
        y = T + i * bh + bh * 0.15
        w = (W - L - R) * v / maxv
        body.append(f'<text x="{L-10}" y="{y + bh*0.45}" text-anchor="end">{name}</text>')
        body.append(f'<rect x="{L}" y="{y}" width="{w:.1f}" height="{bh*0.7:.1f}" fill="{COLORS[i % len(COLORS)]}"/>')
        label = f'{v:,.0f}' + (f' ({note})' if note else '')
        body.append(f'<text x="{L + w + 6:.1f}" y="{y + bh*0.45}">{label}</text>')
    path.write_text(svg(W, H, "\n".join(body)), encoding="utf-8")


def line_chart(sers, title, path):
    W, H, L, R, T, B = 760, 440, 70, 170, 50, 50
    ns = [n for pts in sers.values() for n, _ in pts]
    ms = [m for pts in sers.values() for _, m in pts] + [BUDGET_MS]
    x0, x1 = math.log10(min(ns)), math.log10(max(ns))
    y0, y1 = math.log10(min(ms) * 0.8), math.log10(max(ms) * 1.2)
    X = lambda n: L + (math.log10(n) - x0) / (x1 - x0) * (W - L - R)
    Y = lambda m: H - B - (math.log10(m) - y0) / (y1 - y0) * (H - T - B)
    body = [f'<text x="{(W-R+L)/2}" y="28" text-anchor="middle" font-size="16" font-weight="600">{title}</text>',
            f'<line x1="{L}" y1="{H-B}" x2="{W-R}" y2="{H-B}" stroke="#999"/>',
            f'<line x1="{L}" y1="{T}" x2="{L}" y2="{H-B}" stroke="#999"/>',
            f'<line x1="{L}" y1="{Y(BUDGET_MS):.1f}" x2="{W-R}" y2="{Y(BUDGET_MS):.1f}" stroke="#f59e0b" stroke-dasharray="6 4"/>',
            f'<text x="{W-R-4}" y="{Y(BUDGET_MS)-6:.1f}" text-anchor="end" fill="#b45309">16.7 ms (60 fps)</text>']
    for n in sorted(set(ns)):
        body.append(f'<text x="{X(n):.1f}" y="{H-B+18}" text-anchor="middle">{n:,}</text>')
    for m in (2, 5, 10, 20, 50, 100, 200):
        if y0 <= math.log10(m) <= y1:
            body.append(f'<text x="{L-8}" y="{Y(m)+4:.1f}" text-anchor="end">{m}</text>')
            body.append(f'<line x1="{L}" y1="{Y(m):.1f}" x2="{W-R}" y2="{Y(m):.1f}" stroke="#eee"/>')
    body.append(f'<text x="{(W-R+L)/2}" y="{H-12}" text-anchor="middle">N (log)</text>')
    body.append(f'<text x="18" y="{(H)/2}" transform="rotate(-90 18 {H/2})" text-anchor="middle">bottleneck p50 ms (log)</text>')
    for i, (name, pts) in enumerate(sorted(sers.items())):
        c = COLORS[i % len(COLORS)]
        d = " ".join(f'{"M" if j == 0 else "L"}{X(n):.1f},{Y(m):.1f}' for j, (n, m) in enumerate(pts))
        body.append(f'<path d="{d}" fill="none" stroke="{c}" stroke-width="2"/>')
        body += [f'<circle cx="{X(n):.1f}" cy="{Y(m):.1f}" r="3" fill="{c}"/>' for n, m in pts]
        body.append(f'<text x="{W-R+12}" y="{T + 18*i + 10}" fill="{c}">{name}</text>')
    path.write_text(svg(W, H, "\n".join(body)), encoding="utf-8")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    rows = load()
    p1 = series(rows, "P1")
    bars = []
    for name, pts in sorted(p1.items()):
        if len(pts) < 2:
            continue  # N 하나만 잰 실험 변형(E1 (d) 등)은 한계 N을 보간할 수 없다
        n, note = n_at_budget(pts)
        if n is not None:
            bars.append((name, n, note))
    bar_chart(bars, "Max N at 60 fps (P1, bottleneck p50 ≤ 16.7 ms)", OUT / "n-at-60fps.svg")
    line_chart({k: v for k, v in p1.items() if len(v) >= 2}, "Frame bottleneck vs N (P1)", OUT / "n-scaling-P1.svg")
    for name, n, note in bars:
        print(f"{name}: N@16.7ms = {n:,.0f} {note}")
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()
