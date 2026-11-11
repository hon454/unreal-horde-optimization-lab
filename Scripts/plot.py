"""Measurements/*/summary.csv로 Docs/figures/의 그래프를 다시 그린다. 표준 라이브러리만 써서 SVG를 직접 쓴다(PC에 matplotlib가 없다).

그리는 것 (M3b부터 대표 지표는 프레임 p95, measurement.md "N@16.7ms")
- n-at-60fps.svg   : 단계·변형별 "프레임 p95가 16.7ms를 넘기 직전 N"(P1). README 대표 그래프. 측정 N 사이를 로그 보간한다.
                     막대 옆에 p50 기준 N(Game/Render/GPU/프레임 p50 중 최댓값, M3까지의 정의)과 병목 자원을 병기한다.
- n-scaling-P1.svg : N에 따른 프레임 p95(실선)와 p50 최댓값(점선) ms, P1, 로그-로그. 16.7ms 선과 p95 교차점이 보이도록.
- n-at-60fps.csv   : 위 막대의 값(p95 N, p50 N, 병목). 글의 표 재료.

실행: py -3 -I Scripts/plot.py                # 마일스톤 묶음(Measurements/M*/)
      py -3 -I Scripts/plot.py --batch Full1  # 총괄 묶음: Measurements/Full1/만 읽는다(아래 FULL_SERIES 표, Docs/figures/full1/)

총괄 묶음(Docs/Worklog/Q5.md "2단위")이 그리는 것
- full-ladder-P1.svg, full-ladder-P2.svg : 사다리 8단계의 프레임 p95(굵은 막대)와 p50(가는 막대), 목표 총수. 막대 색은 단계의 성격(출발점, 버는 단계, 품질 단계)이다.
                                          묶음 끝 A/A(L8 기준점 반복)의 범위를 제목 아래에 적는다. 13.3ms(여유 20%)와 16.7ms 선을 긋는다.
- full-ablation-P1.svg                    : 빼 보기 아홉 행의 프레임 p95와 최종 구성(L8)의 p95. 막대가 L8보다 길수록 그 기법이 번 것이다.
- full-maxn-P1.svg                        : N에 따른 프레임 p95, 단계 1~8, P1, 로그-로그(보조 지표).
- full-ladder.csv, full-ablation.csv, full-n-at-60fps.csv : 글의 표 재료(프레임 p50·p95·p99, 프로세스 메모리, GPU 메모리, 품질 열, 동등성 값, N@16.7ms).
"""
import argparse
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
                if not named or not r.get("frame_p95_ms") or r["variant"].endswith(("-aa", "-ab", "-aff", "-noaff", "-start", "-ab2", "-t2")):  # A/A는 잡음 기준이라 곡선에 잇지 않는다
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


# ---- 총괄 묶음(--batch Full1) ----
# 그릴 계열: 사다리 단계(plan.md "사다리" 표의 번호, 성격) -> 조건 이름(Scripts/full-conditions.json). 단계마다 대표 조건 하나다.
# 성격: start 출발점, earn 버는 단계(프레임 시간을 번다), quality 품질 단계(프레임을 쓰고 화면·자세·거리 품질을 얻는다)
FULL_SERIES = [
    ("1", "Actor baseline", "L1-full", "start"),
    ("2", "Logic on Mass", "L2-mass", "earn"),
    ("3", "ISM + VAT, cull past 200 m", "L3-vatcull", "quality"),
    ("4", "Near actors + anim budget", "L4-mass", "quality"),
    ("5", "Simulation LOD + entity pool", "L5-m4offpool", "earn"),
    ("6", "ISKM + fixed bounds", "L6-m5bounds", "quality"),
    ("7", "Niagara background crowd", "L7-q2crowd", "quality"),
    ("8", "Raise quality (actor cap, hydrate dist)", "L8-q4final", "quality"),
]
FULL_KIND_COLOR = {"start": "#6b7280", "earn": "#2563eb", "quality": "#ea580c"}
FULL_KIND_NAME = {"start": "start", "earn": "earns frame time", "quality": "spends it on quality"}
FULL_BASELINE = "L8-q4final"      # 빼 보기와 A/A의 기준 조건
FULL_MARGIN_MS = 13.3             # 목표: 16.7ms에서 여유 20%를 뺀 값(plan.md "목표 사양", 잠정)
FULL_ROW_FIELDS = ["frame_p50_ms", "frame_p95_ms", "frame_p99_ms", "game_p50_ms", "render_p50_ms", "gpu_p50_ms", "mem_total_mb", "gpu_mem_p50",
                   "actors_mean", "view_actors_mean", "cover30_mean", "large_skip_ratio", "death_rate", "near30_mean"]


def full_load(batch):
    path = ROOT / "Measurements" / batch / "summary.csv"
    rows = {}
    with open(path, newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            rows[(r["variant"], int(r["n"]), r["preset"])] = r
    return rows


def fnum(r, k):
    try:
        return float(r[k])
    except (KeyError, ValueError, TypeError):
        return None


def write_rows(path, rows, fields):
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows([{k: r.get(k, "") for k in fields} for r in rows])


def hbar_chart(items, title, subtitle, path, lines):
    """items: (이름, p95, p50 또는 None, 색, 주석). lines: (ms, 색, 이름) 세로선."""
    W, L, R, T = 900, 290, 150, 70
    rowh = 38
    H = T + rowh * len(items) + 40
    maxv = max([i[1] for i in items] + [l[0] for l in lines]) * 1.12
    X = lambda v: L + (W - L - R) * v / maxv
    body = [f'<text x="{W/2}" y="26" text-anchor="middle" font-size="16" font-weight="600">{title}</text>',
            f'<text x="{W/2}" y="46" text-anchor="middle" font-size="12" fill="#555">{subtitle}</text>']
    for ms, color, name in lines:
        body.append(f'<line x1="{X(ms):.1f}" y1="{T-8}" x2="{X(ms):.1f}" y2="{H-30}" stroke="{color}" stroke-dasharray="6 4"/>')
        body.append(f'<text x="{X(ms)+4:.1f}" y="{T-12}" fill="{color}" font-size="11">{name}</text>')
    for i, (name, v95, v50, color, note) in enumerate(items):
        y = T + i * rowh
        body.append(f'<text x="{L-10}" y="{y+17}" text-anchor="end">{name}</text>')
        body.append(f'<rect x="{L}" y="{y+2}" width="{X(v95)-L:.1f}" height="20" fill="{color}"/>')
        if v50 is not None:
            body.append(f'<rect x="{L}" y="{y+25}" width="{X(v50)-L:.1f}" height="6" fill="{color}" opacity="0.55"/>')
        body.append(f'<text x="{X(v95)+6:.1f}" y="{y+17}">{v95:.2f}' + (f' ({note})' if note else '') + '</text>')
    body.append(f'<text x="{L}" y="{H-8}" font-size="11" fill="#555">ms. thick bar = frame p95, thin bar = frame p50 (median of runs)</text>')
    path.write_text(svg(W, H, "\n".join(body)), encoding="utf-8")


def full_main(batch):
    rows = full_load(batch)
    out = OUT / batch.lower()
    out.mkdir(parents=True, exist_ok=True)
    ns = sorted({n for (_, n, _) in rows})
    target = max((n for (v, n, p) in rows if v == FULL_BASELINE and p == "P1"), default=None)
    if target is None:
        raise SystemExit(f"no {FULL_BASELINE} P1 rows in Measurements/{batch}/summary.csv")
    aa = rows.get((FULL_BASELINE + "-aa", target, "P1"))
    base = rows.get((FULL_BASELINE, target, "P1"))
    aa_note = ""
    if aa and base and fnum(aa, "frame_p95_ms") is not None:
        aa_note = (f'A/A ({FULL_BASELINE} again, {aa["runs"]} runs): p95 {fnum(aa, "frame_p95_ms"):.2f} ms '
                   f'(range {fnum(aa, "frame_p95_ms_min"):.2f}-{fnum(aa, "frame_p95_ms_max"):.2f}) vs {fnum(base, "frame_p95_ms"):.2f}')
    budget = [(BUDGET_MS, "#b45309", "16.7 ms"), (FULL_MARGIN_MS, "#16a34a", "13.3 ms")]

    ladder_csv = []
    for preset in ("P1", "P2"):
        items = []
        for num, name, cond, kind in FULL_SERIES:
            r = rows.get((cond, target, preset))
            if not r or fnum(r, "frame_p95_ms") is None:
                continue
            items.append((f"{num}. {name}", fnum(r, "frame_p95_ms"), fnum(r, "frame_p50_ms"), FULL_KIND_COLOR[kind], ""))
            row = {"step": num, "name": name, "cond": cond, "kind": kind, "n": target, "preset": preset, "runs": r["runs"]}
            for k in FULL_ROW_FIELDS:
                row[k] = r.get(k, "")
            row["frame_p95_ms_min"], row["frame_p95_ms_max"] = r.get("frame_p95_ms_min", ""), r.get("frame_p95_ms_max", "")
            ladder_csv.append(row)
        if items:
            legend = ", ".join(f"{k}: {FULL_KIND_NAME[k]}" for k in FULL_KIND_COLOR)
            hbar_chart(items, f"Ladder at N={target:,} ({preset})", (aa_note if preset == "P1" and aa_note else legend), out / f"full-ladder-{preset}.svg", budget)
    write_rows(out / "full-ladder.csv", ladder_csv, ["step", "name", "cond", "kind", "n", "preset", "runs", "frame_p95_ms_min", "frame_p95_ms_max"] + FULL_ROW_FIELDS)

    # 빼 보기: 조건 JSON의 group ablation 전부(summary.csv의 group 열). 막대가 기준(L8)보다 길면 그 기법이 번 것
    abl_items, abl_csv = [], []
    if base:
        b95 = fnum(base, "frame_p95_ms")
        abl_items.append((f"{FULL_BASELINE} (final)", b95, fnum(base, "frame_p50_ms"), FULL_KIND_COLOR["quality"], ""))
        for (v, n, p), r in sorted(rows.items()):
            if p != "P1" or n != target or r.get("group") != "ablation":
                continue
            v95 = fnum(r, "frame_p95_ms")
            if v95 is None:
                continue
            delta = v95 - b95
            abl_items.append((f"without {v.split('-', 1)[1]}", v95, fnum(r, "frame_p50_ms"), FULL_KIND_COLOR["earn"] if delta > 0 else FULL_KIND_COLOR["start"], f"{delta:+.2f}"))
            row = {"cond": v, "n": n, "delta_p95_ms": round(delta, 3), "runs": r["runs"], "frame_p95_ms_min": r.get("frame_p95_ms_min", ""), "frame_p95_ms_max": r.get("frame_p95_ms_max", "")}
            for k in FULL_ROW_FIELDS:
                row[k] = r.get(k, "")
            abl_csv.append(row)
        if len(abl_items) > 1:
            hbar_chart(abl_items, f"Remove one technique from the final configuration (N={target:,}, P1)",
                       (aa_note or "blue: removing it costs frame time (it earned it), gray: no cost"), out / "full-ablation-P1.svg", budget)
    write_rows(out / "full-ablation.csv", abl_csv, ["cond", "n", "delta_p95_ms", "runs", "frame_p95_ms_min", "frame_p95_ms_max"] + FULL_ROW_FIELDS)

    # 보조: N 스케일링(P1). 계열 = 사다리 대표 조건(사다리 번호를 앞에 붙인다)
    p95, p50 = {}, {}
    for num, name, cond, kind in FULL_SERIES:
        for n in ns:
            r = rows.get((cond, n, "P1"))
            if r and fnum(r, "frame_p95_ms") is not None:
                p95.setdefault(f"{num} {cond}", []).append((n, fnum(r, "frame_p95_ms")))
                named = [fnum(r, k) for k in ("frame_p50_ms", "game_p50_ms", "render_p50_ms", "gpu_p50_ms")]
                p50.setdefault(f"{num} {cond}", []).append((n, max(v for v in named if v is not None)))
    table = []
    for name, pts in sorted(p95.items()):
        if len(pts) < 2:
            continue
        n95, note95 = n_at_budget(sorted(pts))
        n50, note50 = n_at_budget(sorted(p50[name]))
        table.append({"series": name, "n_p95": round(n95) if n95 else "", "note_p95": note95, "n_p50": round(n50) if n50 else "", "note_p50": note50,
                      "points": " ".join(f"{n}:{m:.2f}" for n, m in sorted(pts))})
    write_rows(out / "full-n-at-60fps.csv", table, ["series", "n_p95", "note_p95", "n_p50", "note_p50", "points"])
    multi = {k: sorted(v) for k, v in p95.items() if len(v) >= 2}
    if multi:
        line_chart(multi, f"Frame p95 vs N ({batch}, P1)", out / "full-maxn-P1.svg", {k: sorted(v) for k, v in p50.items() if k in multi})
    print(f"{batch}: target N={target:,} {aa_note}")
    for t in table:
        print(f'{t["series"]}: N@16.7ms p95 = {t["n_p95"]} {t["note_p95"]} | p50 = {t["n_p50"]} {t["note_p50"]}')
    print(f"wrote {out}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch", help="총괄 묶음 이름(예: Full1). 주면 Measurements/<묶음>/만 읽어 Docs/figures/<묶음>/에 그린다")
    args = ap.parse_args()
    if args.batch:
        full_main(args.batch)
        return
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
