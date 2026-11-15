"""Measurements/*/summary.csv로 Docs/figures/의 그래프를 다시 그린다. 표준 라이브러리만 써서 SVG를 직접 쓴다(PC에 matplotlib가 없다).

그리는 것 (M3b부터 대표 지표는 프레임 p95, measurement.md "N@16.7ms")
- n-at-60fps.svg   : 단계·변형별 "프레임 p95가 16.7ms를 넘기 직전 N"(P1). README 대표 그래프. 측정 N 사이를 로그 보간한다.
                     막대 옆에 p50 기준 N(Game/Render/GPU/프레임 p50 중 최댓값, M3까지의 정의)과 병목 자원을 병기한다.
- n-scaling-P1.svg : N에 따른 프레임 p95(실선)와 p50 최댓값(점선) ms, P1, 로그-로그. 16.7ms 선과 p95 교차점이 보이도록.
- n-at-60fps.csv   : 위 막대의 값(p95 N, p50 N, 병목). 글의 표 재료.

실행: py -3 -I Scripts/plot.py                # 마일스톤 묶음(Measurements/M*/)
      py -3 -I Scripts/plot.py --batch Full4  # 총괄 묶음: Measurements/Full4/만 읽는다(조건 표 Scripts/full-conditions.json, Docs/figures/full4/)

총괄 묶음(v2 W12, Docs/Worklog/V2.md "W12 측정 준비")이 그리는 것
- full-ladder.svg                          : 사다리 8단계의 프레임 p95(굵은 막대)와 p50(가는 막대), 목표 총수, 이동 조건(V0). 16.7ms와 p95 + 0.6ms(D52 제안) 선을 긋는다.
- full-ablation-moving.svg, -still.svg     : 단계 8에서 기법 하나를 뺀(또는 엔진 회피를 더한) 행의 프레임 p95와 단계 8의 p95. 이동(V0)과 정지(V1).
- full-maxn.svg                            : N에 따른 프레임 p95, 단계 1~8, 로그-로그(보조 지표). 단계 1의 점은 사다리(1만)와 액터 보조(500, 1천, 2천)를 합친다.
- full-ladder.csv, full-ablation-moving.csv, full-ablation-still.csv, full-n-at-60fps.csv : 표 재료(프레임 p50·p95, 범위, 차이 판정, 겹친 쌍, 캡슐, 왕복 전환, CPU 클럭, N@16.7ms).
--out <폴더>로 출력 위치를 바꾼다(확인용 그림은 커밋하지 않는다).
"""
import argparse
import csv
import json
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import lab_conditions as lc  # noqa: E402

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


# ---- 총괄 묶음(--batch Full4, v2 W12) ----
# 계열과 이름은 Scripts/full-conditions.json(v2)이 정한다: steps(단계 이름·성격), configs(빼 보기 이름), groups. 조건 이름은 "<V0|V1>-<구성>"(V0 이동, V1 정지)이다.
# 성격: start 출발점, earn 버는 단계(프레임 시간을 번다), quality 품질 단계(번 시간을 화면·거리 품질에 쓴다)
FULL_KIND_COLOR = {"start": "#6b7280", "earn": "#2563eb", "quality": "#ea580c"}
FULL_LADDER_CLIP_MS = 20.0        # 사다리 그림의 가로축 끝. 넘는 막대(단계 1, 2)는 끊어 그린다
FULL_JUDGE_MARGIN_MS = 0.6        # D52의 제안: p95 중앙값 + 0.6ms <= 16.7ms. 선은 16.7 - 0.6 = 16.1ms에 긋는다(제안이라 사용자 승인 전이다)
FULL_ROW_FIELDS = ["frame_p50_ms", "frame_p95_ms", "frame_p99_ms", "game_p50_ms", "render_p50_ms", "gpu_p50_ms", "mem_total_mb", "gpu_mem_p50",
                   "actors_mean", "view_actors_mean", "cover30_mean", "overlap_mean", "capsule_traces", "capsule_blocked", "sw_round_all", "death_rate", "near30_mean",
                   "cpu_perf_mean", "cpu_perf_min"]


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


def hbar_chart(items, title, subtitle, path, lines, clip=None, footer=None):
    """items: (이름, p95, p50 또는 None, 색, 주석). lines: (ms, 색, 이름) 세로선.

    clip(ms)을 주면 가로축을 거기서 끊는다. 넘는 막대는 축 끝까지만 그리고 끝에 끊김 표시를 한 뒤 값을 적는다.
    """
    W, L, R, T = 900, 290, 150, 70
    rowh = 38
    H = T + rowh * len(items) + 40
    maxv = clip if clip else max([i[1] for i in items] + [l[0] for l in lines]) * 1.12
    XE = W - R
    X = lambda v: L + (XE - L) * min(v, maxv) / maxv
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
        if clip and v95 > maxv:  # 끊김 표시: 막대 끝 가까이에 흰 사선 둘
            for dx in (14, 22):
                body.append(f'<line x1="{XE-dx-4}" y1="{y+34}" x2="{XE-dx+4}" y2="{y}" stroke="white" stroke-width="3"/>')
        body.append(f'<text x="{X(v95)+6:.1f}" y="{y+17}">{v95:.2f}' + (f' ({note})' if note else '') + '</text>')
    body.append(f'<text x="{L}" y="{H-8}" font-size="11" fill="#555">{footer or "ms. thick bar = frame p95, thin bar = frame p50 (median of runs)"}</text>')
    path.write_text(svg(W, H, "\n".join(body)), encoding="utf-8")


def full_target(batch, override):
    # 목표 총수: --target-n > batch.json의 target_n(run-full.ps1이 적는다) > 조건 표의 scenario.target_n
    if override:
        return override, "--target-n"
    bj = ROOT / "Measurements" / batch / "batch.json"
    if bj.exists():
        try:
            t = json.loads(bj.read_text(encoding="utf-8-sig")).get("target_n")
            if t:
                return int(t), "batch.json"
        except (ValueError, OSError):
            pass
    return int(lc.load()["scenario"]["target_n"]), "full-conditions.json"


def full_row(r, extra):
    row = dict(extra, runs=r["runs"], frame_p95_ms_min=r.get("frame_p95_ms_min", ""), frame_p95_ms_max=r.get("frame_p95_ms_max", ""),
               frame_p50_ms_min=r.get("frame_p50_ms_min", ""), frame_p50_ms_max=r.get("frame_p50_ms_max", ""))
    for k in FULL_ROW_FIELDS:
        row[k] = r.get(k, "")
    return row


def full_ablation(rows, doc, cfgs, speed, target, out, title_extra):
    """빼 보기 한 장. 기준은 같은 속도의 단계 8. 막대가 기준보다 길면 그 기법이 번 것이다. 묶음 종류(group)는 조건 표에서 읽는다."""
    base = rows.get((f"{speed}-s8", target, "P1"))
    if not base or fnum(base, "frame_p95_ms") is None:
        return [], None
    names = [c for g in ("ablation", "sub") for c in doc["groups"][g]["configs"]]
    b95, b50 = fnum(base, "frame_p95_ms"), fnum(base, "frame_p50_ms")
    items = [("final configuration (step 8)", b95, b50, FULL_KIND_COLOR["quality"], "")]
    csv_rows = []
    for cfg in names:
        r = rows.get((f"{speed}-{cfg}", target, "P1"))
        if not r or fnum(r, "frame_p95_ms") is None:
            continue
        v95, v50 = fnum(r, "frame_p95_ms"), fnum(r, "frame_p50_ms")
        added = cfgs[cfg]["kind"] == "sub"
        d95, d50 = v95 - b95, v50 - b50
        label = f'with {cfgs[cfg]["title"].replace(" (added)", "")}' if added else f'without {cfgs[cfg]["title"]}'
        items.append((label, v95, v50, FULL_KIND_COLOR["earn"] if d95 > 0 else FULL_KIND_COLOR["start"], f"{d95:+.2f}"))
        # 차이 판정(measurement.md): p50 변화가 두 조건의 범위(최댓값 - 최솟값) 중 큰 쪽보다 클 때만 차이가 있다
        rng = lambda x, k: (fnum(x, k + "_max") or 0) - (fnum(x, k + "_min") or 0)
        distinct = "yes" if abs(d50) > max(rng(r, "frame_p50_ms"), rng(base, "frame_p50_ms")) else "no"
        csv_rows.append(full_row(r, {"cond": f"{speed}-{cfg}", "title": cfgs[cfg]["title"], "kind": cfgs[cfg]["kind"], "n": target,
                                     "delta_p50_ms": round(d50, 3), "delta_p95_ms": round(d95, 3), "p50_distinct": distinct,
                                     "delta_gpu_p50_ms": round((fnum(r, "gpu_p50_ms") or 0) - (fnum(base, "gpu_p50_ms") or 0), 3)}))
    if len(items) > 1:
        hbar_chart(items, f"Remove one technique from the final configuration (N={target:,}, {title_extra})",
                   "blue: removing it costs frame time (it earned it), gray: no cost", out, [(BUDGET_MS, "#b45309", "16.7 ms")])
    return csv_rows, base


def full_main(batch, target_n=None, out_dir=None):
    doc = lc.load()
    cfgs = lc.configs(doc)
    rows = full_load(batch)
    out = Path(out_dir) if out_dir else OUT / batch.lower()
    out.mkdir(parents=True, exist_ok=True)
    target, target_src = full_target(batch, target_n)
    judge = BUDGET_MS - FULL_JUDGE_MARGIN_MS
    lines = [(BUDGET_MS, "#b45309", "16.7 ms (60 fps)"), (judge, "#16a34a", f"{judge:.1f} ms (p95 + {FULL_JUDGE_MARGIN_MS} ms, D52 proposal)")]

    # 사다리: 단계 1~8, 이동(V0), 목표 총수
    aa = rows.get(("V0-s8-aa", target, "P1"))
    base = rows.get(("V0-s8", target, "P1"))
    aa_note = ""
    if aa and base and fnum(aa, "frame_p95_ms") is not None:
        aa_note = (f'A/A (step 8 again, {aa["runs"]} runs): p95 {fnum(aa, "frame_p95_ms"):.2f} ms '
                   f'(range {fnum(aa, "frame_p95_ms_min"):.2f}-{fnum(aa, "frame_p95_ms_max"):.2f}) vs {fnum(base, "frame_p95_ms"):.2f}')
    items, ladder_csv = [], []
    for num, st in doc["steps"].items():
        r = rows.get((f"V0-s{num}", target, "P1"))
        if not r or fnum(r, "frame_p95_ms") is None:
            continue
        items.append((f"{num}. {st['name']}", fnum(r, "frame_p95_ms"), fnum(r, "frame_p50_ms"), FULL_KIND_COLOR[st["role"]], ""))
        ladder_csv.append(full_row(r, {"step": num, "name": st["name"], "cond": f"V0-s{num}", "kind": st["role"], "n": target}))
    if items:
        hbar_chart(items, f"Frame time of the ladder at N={target:,} (moving orb)", "gray: start · blue: earned frame time · orange: spent it on quality",
                   out / "full-ladder.svg", lines, clip=FULL_LADDER_CLIP_MS,
                   footer="ms. thick bar = frame p95, thin bar = frame p50 (median of runs). Steps that exceed the axis are cut")
    write_rows(out / "full-ladder.csv", ladder_csv, ["step", "name", "cond", "kind", "n", "runs", "frame_p95_ms_min", "frame_p95_ms_max", "frame_p50_ms_min", "frame_p50_ms_max"] + FULL_ROW_FIELDS)

    # 빼 보기: 이동(V0)과 정지(V1)
    abl_fields = ["cond", "title", "kind", "n", "delta_p50_ms", "delta_p95_ms", "p50_distinct", "delta_gpu_p50_ms", "runs", "frame_p95_ms_min", "frame_p95_ms_max", "frame_p50_ms_min", "frame_p50_ms_max"] + FULL_ROW_FIELDS
    for speed, name in (("V0", "moving"), ("V1", "still")):
        csv_rows, _ = full_ablation(rows, doc, cfgs, speed, target, out / f"full-ablation-{name}.svg", f"{name} orb")
        write_rows(out / f"full-ablation-{name}.csv", csv_rows, abl_fields)

    # 보조: N 스케일링. 계열 = 단계. 단계 1의 점은 사다리(1만)와 액터 보조(500, 1천, 2천)를 합친다
    p95, p50 = {}, {}
    ns = sorted({n for (v, n, p) in rows if v.startswith("V0-s") and v[4:].isdigit()})
    for num in doc["steps"]:
        for n in ns:
            r = rows.get((f"V0-s{num}", n, "P1"))
            if r and fnum(r, "frame_p95_ms") is not None:
                key = f"{num} {doc['steps'][num]['name']}"
                p95.setdefault(key, []).append((n, fnum(r, "frame_p95_ms")))
                named = [fnum(r, k) for k in ("frame_p50_ms", "game_p50_ms", "render_p50_ms", "gpu_p50_ms")]
                p50.setdefault(key, []).append((n, max(v for v in named if v is not None)))
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
        line_chart(multi, f"Frame p95 vs N ({batch}, moving orb)", out / "full-maxn.svg", {k: sorted(v) for k, v in p50.items() if k in multi})
    print(f"{batch}: target N={target:,} (from {target_src}) {aa_note}")
    for t in table:
        print(f'{t["series"]}: N@16.7ms p95 = {t["n_p95"]} {t["note_p95"]} | p50 = {t["n_p50"]} {t["note_p50"]}')
    print(f"wrote {out}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch", help="총괄 묶음 이름(예: Full1). 주면 Measurements/<묶음>/만 읽어 Docs/figures/<묶음>/에 그린다")
    ap.add_argument("--target-n", type=int, help="--batch의 목표 총수를 직접 정한다(기본: batch.json의 target_n)")
    ap.add_argument("--out", help="--batch의 출력 폴더(기본 Docs/figures/<묶음>/). 커밋하지 않을 확인용 그림은 스크래치 폴더를 준다")
    args = ap.parse_args()
    if args.batch:
        full_main(args.batch, args.target_n, args.out)
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
