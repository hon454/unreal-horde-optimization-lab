"""Measurements/*/summary.csv로 Docs/figures/의 그래프를 다시 그린다. 표준 라이브러리만 써서 SVG를 직접 쓴다(PC에 matplotlib가 없다).

그리는 것 (M3b부터 대표 지표는 프레임 p95, measurement.md "N@16.7ms")
- n-at-60fps.svg   : 단계·변형별 "프레임 p95가 16.7ms를 넘기 직전 N"(P1). README 대표 그래프. 측정 N 사이를 로그 보간한다.
                     막대 옆에 p50 기준 N(Game/Render/GPU/프레임 p50 중 최댓값, M3까지의 정의)과 병목 자원을 병기한다.
- n-scaling-P1.svg : N에 따른 프레임 p95(실선)와 p50 최댓값(점선) ms, P1, 로그-로그. 16.7ms 선과 p95 교차점이 보이도록.
- n-at-60fps.csv   : 위 막대의 값(p95 N, p50 N, 병목). 글의 표 재료.

실행: py -3 -I Scripts/plot.py
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
                named = {k: float(r[k]) for k in ("game_p50_ms", "render_p50_ms", "gpu_p50_ms", "frame_p50_ms") if r.get(k)}
                if not named or not r.get("frame_p95_ms") or r["variant"].endswith(("-aa", "-ab")):  # A/A는 잡음 기준이라 곡선에 잇지 않는다
                    continue
                top = max(named, key=named.get)
                parts = {k: v for k, v in named.items() if not k.startswith("frame")}
                rows.append({"stage": stage, "variant": r["variant"], "n": int(r["n"]), "preset": r["preset"],
                             "ms50": named[top], "ms95": float(r["frame_p95_ms"]), "bottleneck": max(parts, key=parts.get).split("_")[0] if parts else ""})
    return rows


def series(rows, preset, key):
    out = {}
    for r in rows:
        if r["preset"] != preset:
            continue
        out.setdefault(f'{r["stage"]} {r["variant"]}', []).append((r["n"], r[key]))
    return {k: sorted(v) for k, v in out.items()}


def bottleneck_at(rows, preset, name, n):
    """교차 직전 측정점(n 이하 중 가장 큰 N)의 병목 자원."""
    cand = [r for r in rows if r["preset"] == preset and f'{r["stage"]} {r["variant"]}' == name and r["n"] <= n]
    return max(cand, key=lambda r: r["n"])["bottleneck"] if cand else ""


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


def line_chart(sers, title, path, sers50=None):
    W, H, L, R, T, B = 760, 440, 70, 170, 50, 50
    ns = [n for pts in sers.values() for n, _ in pts]
    ms = [m for pts in sers.values() for _, m in pts] + [BUDGET_MS]
    sers50 = sers50 or {}
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
    body.append(f'<text x="18" y="{(H)/2}" transform="rotate(-90 18 {H/2})" text-anchor="middle">frame p95 (solid) / bottleneck p50 (dashed) ms, log</text>')
    for i, (name, pts) in enumerate(sorted(sers.items())):
        c = COLORS[i % len(COLORS)]
        d = " ".join(f'{"M" if j == 0 else "L"}{X(n):.1f},{Y(m):.1f}' for j, (n, m) in enumerate(pts))
        body.append(f'<path d="{d}" fill="none" stroke="{c}" stroke-width="2"/>')
        if name in sers50:
            d50 = " ".join(f'{"M" if j == 0 else "L"}{X(n):.1f},{Y(m):.1f}' for j, (n, m) in enumerate(sers50[name]))
            body.append(f'<path d="{d50}" fill="none" stroke="{c}" stroke-width="1" stroke-dasharray="4 3" opacity="0.7"/>')
        body += [f'<circle cx="{X(n):.1f}" cy="{Y(m):.1f}" r="3" fill="{c}"/>' for n, m in pts]
        body.append(f'<text x="{W-R+12}" y="{T + 18*i + 10}" fill="{c}">{name}</text>')
    path.write_text(svg(W, H, "\n".join(body)), encoding="utf-8")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    rows = load()
    p95 = series(rows, "P1", "ms95")
    p50 = series(rows, "P1", "ms50")
    bars, table = [], []
    for name, pts in sorted(p95.items()):
        if len(pts) < 2:
            continue  # N 하나만 잰 실험 변형(E1 (d) 등)은 한계 N을 보간할 수 없다
        n, note = n_at_budget(pts)
        if n is None:
            continue
        n50, note50 = n_at_budget(p50[name])
        neck = bottleneck_at(rows, "P1", name, n)
        label = note or (f"p50 N {n50:,.0f}, {neck}" if n50 else neck)
        bars.append((name, n, label))
        table.append({"series": name, "n_p95": round(n), "note_p95": note, "n_p50": round(n50) if n50 else "",
                      "note_p50": note50, "bottleneck": neck})
    bar_chart(bars, "Max N at 60 fps (P1, frame p95 ≤ 16.7 ms)", OUT / "n-at-60fps.svg")
    line_chart({k: v for k, v in p95.items() if len(v) >= 2}, "Frame p95 vs N (P1)", OUT / "n-scaling-P1.svg",
               {k: v for k, v in p50.items() if len(v) >= 2})
    with open(OUT / "n-at-60fps.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["series", "n_p95", "note_p95", "n_p50", "note_p50", "bottleneck"])
        w.writeheader()
        w.writerows(table)
    for t in table:
        print(f'{t["series"]}: N@16.7ms p95 = {t["n_p95"]:,} {t["note_p95"]} | p50 = {t["n_p50"]} {t["note_p50"]} | {t["bottleneck"]}')
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()
