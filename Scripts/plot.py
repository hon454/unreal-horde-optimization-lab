"""Measurements/*/summary.csv로 Docs/figures/의 그래프를 다시 그린다. 표준 라이브러리만 써서 SVG를 직접 쓴다(PC에 matplotlib가 없다).

그리는 것 (M3b부터 대표 지표는 프레임 p95, measurement.md "N@16.7ms")
- n-at-60fps.svg   : 단계·변형별 "프레임 p95가 16.7ms를 넘기 직전 N"(P1). README 대표 그래프. 측정 N 사이를 로그 보간한다.
                     막대 옆에 p50 기준 N(Game/Render/GPU/프레임 p50 중 최댓값, M3까지의 정의)과 병목 자원을 병기한다.
- n-scaling-P1.svg : N에 따른 프레임 p95(실선)와 p50 최댓값(점선) ms, P1, 로그-로그. 16.7ms 선과 p95 교차점이 보이도록.
- n-at-60fps.csv   : 위 막대의 값(p95 N, p50 N, 병목). 글의 표 재료.

실행: py -3 -I Scripts/plot.py                # 마일스톤 묶음(Measurements/M*/)
      py -3 -I Scripts/plot.py --batch Full4  # 총괄 묶음: Measurements/Full4/만 읽는다(조건 표 Scripts/full-conditions.json, Docs/figures/full4/)

총괄 묶음(v2 W12, Docs/Worklog/V2.md "W12 측정 준비")이 그리는 것. 그림 글자는 영어이고 묶음 이름과 조건 라벨을 쓰지 않는다.
- full-ladder.svg                          : 사다리 8단계의 프레임 p95(굵은 막대)와 p50(가는 막대), 목표 총수, 이동 조건(V0). 두 판이다(단계 1 / 단계 2~8).
                                             16.7ms와 p95 + 0.6ms(ADR-0019 판정식) 선을 긋는다. 아래 글에 단계 8의 16.7ms까지의 여유와 같은 조건 다시 재기를 적는다.
- full-ablation-moving.svg, -still.svg     : 기법별 기여. 단계 8에서 기법 하나를 끈(또는 엔진 회피로 바꾼) 행의 프레임 p95와 단계 8의 p95. 이동(V0)과 정지(V1).
                                             모든 행을 그린다. 단계 8과 구별되지 않는 행(distinguishable())은 회색 막대와 세 실행의 범위 선, 구별되는 행만 색을 쓴다.
                                             파일 이름(ablation)과 행 이름("without …")은 조건 표의 식별자라 둔다.
- full-maxn.svg                            : N에 따른 프레임 p95, 단계 1~8과 s1opt, 로그-로그(보조 지표. 공개 문서에 싣지 않는다). 단계 1의 점은 사다리(1만)와 액터 보조(500, 1천, 2천)를 합친다.
                                             기능 미달(equivalence.csv의 UNMET) 점은 속이 빈 원으로 그리고 아래 주석에 적는다.
- full-ladder.csv, full-ablation-moving.csv, full-ablation-still.csv, full-n-at-60fps.csv : 표 재료(프레임 p50·p95, 범위, 차이 판정, 겹친 쌍, 캡슐, 왕복 전환, CPU 클럭, N@16.7ms).
  표는 summary.csv의 값(summarize.py가 runs_used로 고른 실행의 중앙값)을 그대로 읽는다. 하이드레이션을 끈 조건처럼 보충 실행(회차 21)을 쓴 조건도 summary.csv가 이미 담아 runs_used 열로 보인다.
  full-ablation-*.csv의 p95_distinct는 distinguishable()의 판정이다.
  full-n-at-60fps.csv의 excluded는 "excluded: N=… unmet", n_p95_bound는 뺀 뒤 16.7ms를 사이에 두는 두 점이 없을 때의 범위("≥12500")다.
--out <폴더>로 출력 위치를 바꾼다(확인용 그림은 커밋하지 않는다). --equivalence <csv>로 동등성 결과를 바꾼다(기본 Measurements/<묶음>/equivalence.csv).
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
LINE_COLORS = COLORS + ["#be185d", "#65a30d", "#0d9488", "#a16207"]  # line_chart의 계열이 일곱을 넘어도 색이 겹치지 않게(단계 여덟과 하위 실험)


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


def line_chart(sers, title, path, sers50=None, hollow=None, footnote="", legend_w=170):
    """hollow: 계열 이름 -> 속이 빈 원으로 그릴 N들(기능 미달로 최대 N 보간에서 뺀 점). footnote: 그림 아래 글(문자열 또는 줄 목록). legend_w: 오른쪽 범례 폭."""
    foot = [footnote] if isinstance(footnote, str) and footnote else list(footnote or [])
    extra = 16 * len(foot) + (4 if foot else 0)
    W, H, L, R, T, B = 760 + legend_w - 170, 440 + extra, 70, legend_w, 50, 50 + extra
    hollow = hollow or {}
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
            f'<text x="{X(7071) if min(ns) < 7071 < max(ns) else L + 6:.1f}" y="{Y(BUDGET_MS)+16:.1f}" text-anchor="{"middle" if min(ns) < 7071 < max(ns) else "start"}" fill="#b45309">16.7 ms (60 fps)</text>']  # 선 아래, 곡선이 없는 자리
    last_x = -1e9
    for n in sorted(set(ns)):  # 눈금 글자가 겹치면(2,000과 2,500, 10,000과 12,500) 앞 글자와 44px 안인 것은 눈금 선만 긋는다
        body.append(f'<line x1="{X(n):.1f}" y1="{H-B}" x2="{X(n):.1f}" y2="{H-B+4}" stroke="#999"/>')
        if X(n) - last_x >= 44:
            body.append(f'<text x="{X(n):.1f}" y="{H-B+18}" text-anchor="middle">{n:,}</text>')
            last_x = X(n)
    for m in (2, 5, 10, 20, 50, 100, 200):
        if y0 <= math.log10(m) <= y1:
            body.append(f'<text x="{L-8}" y="{Y(m)+4:.1f}" text-anchor="end">{m}</text>')
            body.append(f'<line x1="{L}" y1="{Y(m):.1f}" x2="{W-R}" y2="{Y(m):.1f}" stroke="#eee"/>')
    body.append(f'<text x="{(W-R+L)/2}" y="{H-B+36}" text-anchor="middle">N (log)</text>')
    for k, line in enumerate(foot):
        body.append(f'<text x="{L}" y="{H - 8 - 16 * (len(foot) - 1 - k)}" font-size="11" fill="#555">{line}</text>')
    body.append(f'<text x="18" y="{(H)/2}" transform="rotate(-90 18 {H/2})" text-anchor="middle">frame p95 (solid) / bottleneck p50 (dashed) ms, log</text>')
    for i, (name, pts) in enumerate(sorted(sers.items())):
        c = LINE_COLORS[i % len(LINE_COLORS)]
        d = " ".join(f'{"M" if j == 0 else "L"}{X(n):.1f},{Y(m):.1f}' for j, (n, m) in enumerate(pts))
        body.append(f'<path d="{d}" fill="none" stroke="{c}" stroke-width="2"/>')
        if name in sers50:
            d50 = " ".join(f'{"M" if j == 0 else "L"}{X(n):.1f},{Y(m):.1f}' for j, (n, m) in enumerate(sers50[name]))
            body.append(f'<path d="{d50}" fill="none" stroke="{c}" stroke-width="1" stroke-dasharray="4 3" opacity="0.7"/>')
        body += [(f'<circle cx="{X(n):.1f}" cy="{Y(m):.1f}" r="4" fill="white" stroke="{c}" stroke-width="2"/>' if n in hollow.get(name, ())
                  else f'<circle cx="{X(n):.1f}" cy="{Y(m):.1f}" r="3" fill="{c}"/>') for n, m in pts]
        body.append(f'<text x="{W-R+12}" y="{T + 18*i + 10}" fill="{c}">{name}</text>')
    if sers50:  # 선 모양 범례: 실선 = 프레임 p95, 점선 = 프레임·게임·렌더·GPU p50 중 가장 긴 것
        ly = T + 18 * len(sers) + 20
        body += [f'<line x1="{W-R+12}" y1="{ly}" x2="{W-R+40}" y2="{ly}" stroke="#444" stroke-width="2"/>',
                 f'<text x="{W-R+46}" y="{ly+4}" font-size="12" fill="#444">solid: frame p95</text>',
                 f'<line x1="{W-R+12}" y1="{ly+20}" x2="{W-R+40}" y2="{ly+20}" stroke="#444" stroke-width="1" stroke-dasharray="4 3"/>',
                 f'<text x="{W-R+46}" y="{ly+24}" font-size="12" fill="#444">dashed: slowest of the frame,</text>',
                 f'<text x="{W-R+46}" y="{ly+38}" font-size="12" fill="#444">game, render and GPU p50</text>']
    path.write_text(svg(W, H, "\n".join(body)), encoding="utf-8")


# ---- 총괄 묶음(--batch Full4, v2 W12) ----
# 계열과 이름은 Scripts/full-conditions.json(v2)이 정한다: steps(단계 이름·성격), configs(기법별 기여 이름), groups. 조건 이름은 "<V0|V1>-<구성>"(V0 이동, V1 정지)이다.
# 성격(role, 단계의 의도): start 출발점, earn 비용을 줄이는 단계(2, 3, 5, 8), quality 표현을 더하는 단계(4, 6, 7). 키 "earn"과 "quality"는 식별자라 둔다.
FULL_KIND_COLOR = {"start": "#6b7280", "earn": "#2563eb", "quality": "#ea580c"}
FULL_KIND_TEXT = {"start": "start (step 1)", "earn": "steps that cut cost", "quality": "steps that add representation"}  # 그림 글자(용어집의 영어 대응어)
FULL_LADDER_PANELS = ((700.0, 100, (1,), False), (40.0, 10, (2, 3, 4, 5, 6, 7, 8), True))  # 사다리 그림의 두 판: (가로축 끝 ms, 눈금 간격, 단계, 기준선을 긋는가). 단계 1과 2가 축을 넘지 않게 단계 1을 따로 둔다
FULL_DISTINCT_FLOOR_MS = 0.5      # measurement.md "차이 판정": 0.5ms 아래 차이는 범위를 넘어도 구별되지 않는다(5회 묶음이 없는 총괄 묶음)
FULL_ABL_BASE_COLOR = "#1f2937"   # 기법별 기여 그림: 최종 구성(단계 8)
FULL_ABL_SLOWER_COLOR = "#2563eb" # 끄면 프레임이 길어지는 기법(기법이 시간을 줄인다)
FULL_ABL_FASTER_COLOR = "#9333ea" # 끄면 프레임이 짧아지는 기법(기법이 시간을 쓴다)
FULL_ABL_SAME_COLOR = "#9ca3af"   # 구별되지 않는 행
# 그림용 행 이름: 조건 표의 제목이 이중 부정이거나 어느 쪽인지 모호한 행만 바꾼다. 없으면 "without <제목>"(하위 실험은 "with <제목>")이다
FULL_ABL_LABELS = {"a8shd": "with far shadows back on", "a8bnd": "without ISKM fixed bounds", "a8fsb": "without actor mesh fixed bounds",
                   "a8far": "without the 200 m far cull", "a8bud": "without actor animation budget", "a8hold": "without minimum hold time"}
# 다른 것이 함께 빠지는 행(measurement.md "변수가 둘 이상 섞인 행"). 그림에서 †로 표시하고 아래에 적는다
FULL_ABL_MIXED = {"a8hyd": "hydration off also removes the actor capsule, actor animation budget and actor mesh fixed bounds",
                  "a8iskm": "ISKM off also removes ISKM fixed bounds, the track reserve and the far-hit rule",
                  "a8fx": "background crowd off also removes the far-hit rule and the drawing of off-map records"}
FULL_JUDGE_MARGIN_MS = 0.6        # ADR-0019 판정식(D52에서 온 값): p95 중앙값 + 0.6ms <= 16.7ms. 선은 16.7 - 0.6 = 16.1ms에 긋는다. 그림 글자에는 내부 이름을 쓰지 않는다
UNMET = "UNMET"                   # equivalence-check.py의 기능 미달 등급(목표 총수가 아닌 점). 그 점은 최대 N 보간에서 뺀다
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


def esc(t):
    return str(t).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def fmt_ms(v):
    """그림의 ms 값. 100 이상은 정수, 아래는 소수 둘, 소수점 아래가 0뿐이면 정수(posting.md: 27.0이 아니라 27)."""
    s = f"{v:.0f}" if abs(v) >= 100 else f"{v:.2f}"
    return s[:-3] if s.endswith(".00") else s


def halo_text(x, y, text, size=13, fill="#111827", anchor="start", weight=None):
    """선이 지나가도 읽히게 흰 테두리를 먼저 깔고 같은 글자를 위에 쓴다."""
    attrs = f'x="{x:.1f}" y="{y:.1f}" font-size="{size}" text-anchor="{anchor}"' + (f' font-weight="{weight}"' if weight else "")
    return (f'<text {attrs} fill="white" stroke="white" stroke-width="4" stroke-linejoin="round">{esc(text)}</text>'
            f'<text {attrs} fill="{fill}">{esc(text)}</text>')


def legend_rows(x0, y, width, entries, size=12):
    """entries: (모양, 색, 글자). 모양은 box, dash(점선), dot(촘촘한 점선). 한 줄이 넘치면 다음 줄로 간다. (svg 조각들, 다음 y)를 돌려준다."""
    out, x = [], x0
    for shape, color, text in entries:
        w = 22 + 6.4 * len(text) + 18
        if x + w > x0 + width and x > x0:
            x, y = x0, y + 20
        if shape == "box":
            out.append(f'<rect x="{x}" y="{y-10}" width="14" height="12" fill="{color}"/>')
        else:
            out.append(f'<line x1="{x}" y1="{y-4}" x2="{x+16}" y2="{y-4}" stroke="{color}" stroke-width="2" stroke-dasharray="{"6 4" if shape == "dash" else "2 3"}"/>')
        out.append(f'<text x="{x+22}" y="{y}" font-size="{size}" fill="#374151">{esc(text)}</text>')
        x += w
    return out, y + 20


def _range(r, key):
    return (fnum(r, key + "_max") or 0) - (fnum(r, key + "_min") or 0)


def distinguishable(row, base, key="frame_p95_ms"):
    """단계 8(base)과 구별되는 행인가(measurement.md "차이 판정"). 프레임 p95 중앙값의 차이가 두 조건의 범위(최댓값 - 최솟값) 중 큰 쪽보다 크고 0.5ms 이상일 때만 True.

    총괄 묶음에는 5회 묶음이 없어 0.5ms 아래 차이는 범위를 넘어도 구별되지 않는다. 그림의 색과 CSV의 p95_distinct가 이 함수 하나를 쓴다.
    """
    d = abs(fnum(row, key) - fnum(base, key))
    return d >= FULL_DISTINCT_FLOOR_MS and d > max(_range(row, key), _range(base, key))


def ladder_chart(steps, title, path, lines, footers):
    """steps: dict(num, name, role, p95, p50). lines: (ms, 색, 이름) 세로선. 가로축이 다른 두 판으로 그린다(FULL_LADDER_PANELS).

    값 글자는 막대 끝에 흰 테두리를 깔아 기준선이 지나가도 읽힌다. 기준선의 이름은 막대 위가 아니라 범례에 둔다.
    """
    W, L, R, rowh = 960, 300, 120, 38
    XE = W - R
    body = [f'<text x="{W/2}" y="26" text-anchor="middle" font-size="16" font-weight="600">{esc(title)}</text>']
    kinds = [k for k in ("start", "earn", "quality") if any(s["role"] == k for s in steps)]
    leg = [("box", FULL_KIND_COLOR[k], FULL_KIND_TEXT[k]) for k in kinds] + [("dash", c, n) for _, c, n in lines]
    parts, y = legend_rows(24, 54, W - 48, leg)
    body += parts
    for pi, (axis_max, tick, nums, show_lines) in enumerate(FULL_LADDER_PANELS):
        panel = [s for s in steps if s["num"] in nums]
        if not panel:
            continue
        X = lambda v, axis_max=axis_max: L + (XE - L) * min(v, axis_max) / axis_max
        y += 14
        body.append(f'<line x1="24" y1="{y-12}" x2="{W-24}" y2="{y-12}" stroke="#e5e7eb"/>')
        span = f"step {panel[0]['num']}" if len(panel) == 1 else f"steps {panel[0]['num']}-{panel[-1]['num']}"
        body.append(f'<text x="24" y="{y+4}" font-size="13" fill="#4b5563">Scale {"AB"[pi]}: 0-{axis_max:g} ms ({span})</text>')
        top, bottom = y + 14, y + 14 + rowh * len(panel)
        for k in range(int(axis_max // tick) + 1):
            gx = X(k * tick)
            body.append(f'<line x1="{gx:.1f}" y1="{top}" x2="{gx:.1f}" y2="{bottom}" stroke="#eef0f3"/>')
            body.append(f'<text x="{gx:.1f}" y="{y+4}" font-size="11" fill="#6b7280" text-anchor="middle">{k * tick:g}</text>')
        if show_lines:
            for ms, color, _ in lines:
                body.append(f'<line x1="{X(ms):.1f}" y1="{top}" x2="{X(ms):.1f}" y2="{bottom}" stroke="{color}" stroke-width="1.5" stroke-dasharray="6 4"/>')
        for i, s in enumerate(panel):
            ry = top + i * rowh
            color = FULL_KIND_COLOR[s["role"]]
            body.append(f'<text x="{L-10}" y="{ry+17}" text-anchor="end" font-size="14">{esc(str(s["num"]) + ". " + s["name"])}</text>')
            body.append(f'<rect x="{L}" y="{ry+2}" width="{X(s["p95"])-L:.1f}" height="20" fill="{color}"/>')
            body.append(f'<rect x="{L}" y="{ry+25}" width="{X(s["p50"])-L:.1f}" height="6" fill="{color}" opacity="0.55"/>')
            # 값 글자는 막대 안쪽 끝에 흰 글자로 쓴다. 기준선이 막대 끝 오른쪽에 있어 글자와 겹치지 않는다
            body.append(f'<text x="{X(s["p95"])-8:.1f}" y="{ry+17}" font-size="14" text-anchor="end" fill="white">'
                        f'<tspan font-size="11" opacity="0.9">p50 {fmt_ms(s["p50"])}   </tspan><tspan font-weight="600">p95 {fmt_ms(s["p95"])}</tspan></text>')
        y = bottom + 6
    y += 8
    body.append(f'<line x1="24" y1="{y-12}" x2="{W-24}" y2="{y-12}" stroke="#e5e7eb"/>')
    for line in footers:
        body.append(f'<text x="24" y="{y+4}" font-size="12" fill="#4b5563">{esc(line)}</text>')
        y += 18
    path.write_text(svg(W, y + 6, "\n".join(body)), encoding="utf-8")


def contribution_chart(base, sections, title, subtitle, path, footers):
    """기법별 기여 한 장. base: 최종 구성(단계 8)의 행. sections: (머리글, 행들). 행은 dict(name, p95, p50, lo, hi, d95, state, mixed).

    state는 slower(끄면 프레임이 길어진다), faster(끄면 짧아진다), same(구별되지 않음, 회색). 모든 행에 세 실행의 범위 선(lo~hi)을 긋는다.
    """
    W, L, R, rowh = 1000, 350, 150, 30
    XE = W - R
    rows_all = [base] + [it for _, its in sections for it in its]
    peak = max(max(r["hi"], r["p95"]) for r in rows_all)
    axis_max = (int(max(peak, BUDGET_MS) // 4) + 1) * 4
    X = lambda v: L + (XE - L) * v / axis_max
    color_of = {"base": FULL_ABL_BASE_COLOR, "slower": FULL_ABL_SLOWER_COLOR, "faster": FULL_ABL_FASTER_COLOR, "same": FULL_ABL_SAME_COLOR}
    body = [f'<text x="{W/2}" y="26" text-anchor="middle" font-size="16" font-weight="600">{esc(title)}</text>',
            f'<text x="{W/2}" y="46" text-anchor="middle" font-size="12" fill="#555">{esc(subtitle)}</text>']
    leg = [("box", FULL_ABL_BASE_COLOR, "final configuration (step 8)"), ("box", FULL_ABL_SLOWER_COLOR, "frame time rises without it"),
           ("box", FULL_ABL_FASTER_COLOR, "frame time falls without it"), ("box", FULL_ABL_SAME_COLOR, "not distinguishable from the final configuration"),
           ("dash", "#b45309", "16.7 ms (60 fps)"), ("dot", FULL_ABL_BASE_COLOR, "frame p95 of the final configuration")]
    parts, y = legend_rows(24, 70, W - 48, leg)
    body += parts
    y += 8
    top, bottom = y, y + rowh * len(rows_all) + 28 * len(sections)
    for k in range(int(axis_max // 4) + 1):
        gx = X(k * 4)
        body.append(f'<line x1="{gx:.1f}" y1="{top}" x2="{gx:.1f}" y2="{bottom}" stroke="#eef0f3"/>')
        body.append(f'<text x="{gx:.1f}" y="{bottom+14}" font-size="11" fill="#6b7280" text-anchor="middle">{k * 4}</text>')
    body.append(f'<text x="{(L+XE)/2:.1f}" y="{bottom+30}" font-size="12" fill="#4b5563" text-anchor="middle">frame time (ms)</text>')
    body.append(f'<line x1="{X(BUDGET_MS):.1f}" y1="{top}" x2="{X(BUDGET_MS):.1f}" y2="{bottom}" stroke="#b45309" stroke-width="1.5" stroke-dasharray="6 4"/>')
    body.append(f'<line x1="{X(base["p95"]):.1f}" y1="{top}" x2="{X(base["p95"]):.1f}" y2="{bottom}" stroke="{FULL_ABL_BASE_COLOR}" stroke-width="1.5" stroke-dasharray="2 3"/>')

    def row(ry, it, kind):
        c = color_of[kind]
        txt = "#6b7280" if kind == "same" else "#111827"
        name = it["name"] + (" †" if it.get("mixed") else "")
        out = [f'<text x="{L-10}" y="{ry+15}" text-anchor="end" font-size="14" fill="{txt}">{esc(name)}</text>',
               f'<rect x="{L}" y="{ry+2}" width="{X(it["p95"])-L:.1f}" height="16" fill="{c}"/>',
               f'<rect x="{L}" y="{ry+20}" width="{X(it["p50"])-L:.1f}" height="5" fill="{c}" opacity="0.55"/>']
        for w, col in ((3.5, "white"), (1.4, "#111827")):  # 범위 선: 흰 바탕 위에 검정, 막대 위에서도 보인다
            out.append(f'<line x1="{X(it["lo"]):.1f}" y1="{ry+10}" x2="{X(it["hi"]):.1f}" y2="{ry+10}" stroke="{col}" stroke-width="{w}"/>')
            for xv in (it["lo"], it["hi"]):
                out.append(f'<line x1="{X(xv):.1f}" y1="{ry+5}" x2="{X(xv):.1f}" y2="{ry+15}" stroke="{col}" stroke-width="{w}"/>')
        delta = "" if kind == "base" else f'  ({it["d95"]:+.2f})'
        out.append(halo_text(X(max(it["hi"], it["p95"])) + 8, ry + 15, f'{it["p95"]:.2f}' + delta, size=13, fill=txt, weight="600" if kind != "same" else None))
        return out

    ry = top
    body += row(ry, base, "base")
    ry += rowh
    for head, its in sections:
        body.append(f'<text x="24" y="{ry+18}" font-size="12" font-weight="600" fill="#4b5563">{esc(head)}</text>')
        ry += 28
        for it in its:
            body += row(ry, it, it["state"])
            ry += rowh
    y = bottom + 46
    body.append(f'<line x1="24" y1="{y-14}" x2="{W-24}" y2="{y-14}" stroke="#e5e7eb"/>')
    for line in footers:
        body.append(f'<text x="24" y="{y}" font-size="12" fill="#4b5563">{esc(line)}</text>')
        y += 18
    path.write_text(svg(W, y, "\n".join(body)), encoding="utf-8")


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
    row = dict(extra, runs=r["runs"], runs_used=r.get("runs_used", ""), frame_p95_ms_min=r.get("frame_p95_ms_min", ""), frame_p95_ms_max=r.get("frame_p95_ms_max", ""),
               frame_p50_ms_min=r.get("frame_p50_ms_min", ""), frame_p50_ms_max=r.get("frame_p50_ms_max", ""))
    for k in FULL_ROW_FIELDS:
        row[k] = r.get(k, "")
    return row


def runs_text(rs):
    """"3 runs" 또는 "3-4 runs"."""
    ns = sorted({int(r["runs"]) for r in rs if r.get("runs")})
    return f"{ns[0]} runs" if len(ns) == 1 else f"{ns[0]}-{ns[-1]} runs"


def full_ablation(rows, doc, cfgs, speed, target, out, speed_name):
    """기법별 기여 한 장. 기준은 같은 속도의 단계 8. 단계 8에서 기법 하나를 끈(하위 실험은 바꾼) 행의 프레임 p95를 모두 그린다.

    구별되는 행(distinguishable)만 색을 쓴다: 끄면 길어지면 파랑, 짧아지면 보라. 구별되지 않는 행은 회색이고 세 실행의 범위 선이 보인다.
    묶음 종류(group)는 조건 표에서 읽는다.
    """
    base = rows.get((f"{speed}-s8", target, "P1"))
    if not base or fnum(base, "frame_p95_ms") is None:
        return [], None
    names = [c for g in ("ablation", "sub") for c in doc["groups"][g]["configs"]]
    b95, b50 = fnum(base, "frame_p95_ms"), fnum(base, "frame_p50_ms")
    csv_rows, groups, used, shown = [], {"dist": [], "same": [], "sub": []}, [base], set()
    for cfg in names:
        r = rows.get((f"{speed}-{cfg}", target, "P1"))
        if not r or fnum(r, "frame_p95_ms") is None:
            continue
        v95, v50 = fnum(r, "frame_p95_ms"), fnum(r, "frame_p50_ms")
        sub = cfgs[cfg]["kind"] == "sub"
        d95, d50 = v95 - b95, v50 - b50
        dist = distinguishable(r, base)
        title = cfgs[cfg]["title"]
        name = FULL_ABL_LABELS.get(cfg) or (f"with {title[0].lower() + title[1:]}" if sub else f"without {title}")
        item = {"name": name, "p95": v95, "p50": v50, "lo": fnum(r, "frame_p95_ms_min"), "hi": fnum(r, "frame_p95_ms_max"), "d95": d95,
                "state": ("slower" if d95 > 0 else "faster") if dist else "same", "mixed": cfg in FULL_ABL_MIXED}
        groups["sub" if sub else "dist" if dist else "same"].append(item)
        used.append(r)
        shown.add(cfg)
        csv_rows.append(full_row(r, {"cond": f"{speed}-{cfg}", "title": title, "kind": cfgs[cfg]["kind"], "n": target,
                                     "delta_p50_ms": round(d50, 3), "delta_p95_ms": round(d95, 3), "p95_distinct": "yes" if dist else "no",
                                     "delta_gpu_p50_ms": round((fnum(r, "gpu_p50_ms") or 0) - (fnum(base, "gpu_p50_ms") or 0), 3)}))
    if csv_rows:
        for g in groups.values():
            g.sort(key=lambda it: -it["d95"])
        sections = [(head, groups[k]) for head, k in (
            ("Distinguishable from the final configuration", "dist"),
            (f"Not distinguishable (frame p95 differs by under {FULL_DISTINCT_FLOOR_MS:g} ms or by less than a run range)", "same"),
            ("Sub-experiment: a technique is replaced, not removed", "sub")) if groups[k]]
        base_item = {"name": "final configuration (step 8)", "p95": b95, "p50": b50, "lo": fnum(base, "frame_p95_ms_min"), "hi": fnum(base, "frame_p95_ms_max"), "d95": 0.0}
        footers = [f"Thick bar: frame p95, median of the runs used ({runs_text(used)}). Thin bar: frame p50. Black line: range of the runs' frame p95.",
                   "Value in brackets: change of frame p95 from the final configuration, in ms.",
                   f"Distinguishable: the change is at least {FULL_DISTINCT_FLOOR_MS:g} ms and larger than the larger run range of the two conditions."]
        footers += [f"† {FULL_ABL_MIXED[c]}." for c in names if c in FULL_ABL_MIXED and c in shown]
        contribution_chart(base_item, sections, "Technique contributions (one technique off in the final configuration)",
                           f"Frame p95 at N={target:,}, {speed_name} orb", out, footers)
    return csv_rows, base


def full_main(batch, target_n=None, out_dir=None, eq_path=None):
    doc = lc.load()
    cfgs = lc.configs(doc)
    rows = full_load(batch)
    out = Path(out_dir) if out_dir else OUT / batch.lower()
    out.mkdir(parents=True, exist_ok=True)
    target, target_src = full_target(batch, target_n)
    judge = BUDGET_MS - FULL_JUDGE_MARGIN_MS
    lines = [(BUDGET_MS, "#b45309", "16.7 ms (60 fps)"),
             (judge, "#16a34a", f"{judge:.1f} ms = 16.7 - {FULL_JUDGE_MARGIN_MS} ms: step 8 passes the target check below this frame p95")]

    # 사다리: 단계 1~8, 이동(V0), 목표 총수
    aa = rows.get(("V0-s8-aa", target, "P1"))
    base = rows.get(("V0-s8", target, "P1"))
    items, ladder_csv, used = [], [], []
    for num, st in doc["steps"].items():
        r = rows.get((f"V0-s{num}", target, "P1"))
        if not r or fnum(r, "frame_p95_ms") is None:
            continue
        items.append({"num": int(num), "name": st["name"], "role": st["role"], "p95": fnum(r, "frame_p95_ms"), "p50": fnum(r, "frame_p50_ms")})
        ladder_csv.append(full_row(r, {"step": num, "name": st["name"], "cond": f"V0-s{num}", "kind": st["role"], "n": target}))
        used.append(r)
    aa_note = ""
    if aa and base and fnum(aa, "frame_p95_ms") is not None:
        aa_note = (f'Same-condition rerun of step 8 ({aa["runs"]} runs): frame p95 {fnum(aa, "frame_p95_ms"):.2f} ms '
                   f'(range {fnum(aa, "frame_p95_ms_min"):.2f}-{fnum(aa, "frame_p95_ms_max"):.2f}) against {fnum(base, "frame_p95_ms"):.2f} ms in the ladder.')
    if items:
        footers = [f"Thick bar: frame p95. Thin bar: frame p50. Each bar is the median of {runs_text(used)}. The two panels use different scales."]
        if base and fnum(base, "frame_p95_ms") is not None:
            b95 = fnum(base, "frame_p95_ms")
            footers.append(f"Step 8 (final configuration): frame p95 {b95:.2f} ms, {BUDGET_MS - b95:.2f} ms below 16.7 ms.")
        if aa_note:
            footers.append(aa_note)
        footers.append("Colors show what each step was meant to do, not what it measured.")
        ladder_chart(items, f"Frame time per step (N={target:,}, moving orb)", out / "full-ladder.svg", lines, footers)
    write_rows(out / "full-ladder.csv", ladder_csv, ["step", "name", "cond", "kind", "n", "runs", "runs_used", "frame_p95_ms_min", "frame_p95_ms_max", "frame_p50_ms_min", "frame_p50_ms_max"] + FULL_ROW_FIELDS)

    # 기법별 기여: 이동(V0)과 정지(V1)
    abl_fields = ["cond", "title", "kind", "n", "delta_p50_ms", "delta_p95_ms", "p95_distinct", "delta_gpu_p50_ms", "runs", "runs_used", "frame_p95_ms_min", "frame_p95_ms_max", "frame_p50_ms_min", "frame_p50_ms_max"] + FULL_ROW_FIELDS
    for speed, name in (("V0", "moving"), ("V1", "still")):
        csv_rows, _ = full_ablation(rows, doc, cfgs, speed, target, out / f"full-ablation-{name}.svg", name)
        write_rows(out / f"full-ablation-{name}.csv", csv_rows, abl_fields)

    # 보조: N 스케일링. 계열 = 단계와 액터 보조의 하위 실험(s1opt). 단계 1의 점은 사다리(1만)와 액터 보조(500, 1천, 2천)를 합친다
    # Full4 계획 점검 R1: equivalence.csv에서 기능 미달(UNMET)인 (조건, N) 점은 보간에서 뺀다. 그림에는 속이 빈 원으로 남긴다
    unmet, eq_src = full_unmet(batch, eq_path)
    series_cfgs = [(f"s{num}", f"{num} {st['name']}") for num, st in doc["steps"].items()]
    # 하위 실험 계열의 이름: "<바탕 단계 번호>+ <제목의 ' + ' 뒤>"(s1opt -> "1+ Animation Budget Allocator")
    series_cfgs += [(c, f"{cfgs[c].get('base', c)[1:]}+ {cfgs[c]['title'].split(' + ', 1)[-1]}")
                    for c in doc["groups"].get("actor", {}).get("configs", []) if cfgs[c]["kind"] != "step"]
    p95, p50, ovl, excl = {}, {}, {}, {}
    ns = sorted({n for (v, n, p) in rows if v.startswith("V0-") and p == "P1"})
    for cfg, key in series_cfgs:
        for n in ns:
            r = rows.get((f"V0-{cfg}", n, "P1"))
            if r and fnum(r, "frame_p95_ms") is not None:
                p95.setdefault(key, []).append((n, fnum(r, "frame_p95_ms")))
                ovl.setdefault(key, {})[n] = fnum(r, "overlap_mean")
                named = [fnum(r, k) for k in ("frame_p50_ms", "game_p50_ms", "render_p50_ms", "gpu_p50_ms")]
                p50.setdefault(key, []).append((n, max(v for v in named if v is not None)))
                if (f"V0-{cfg}", n) in unmet:
                    excl.setdefault(key, set()).add(n)
    table = []
    for name, pts in sorted(p95.items()):
        if len(pts) < 2:
            continue
        drop = excl.get(name, set())
        kept = [(n, m) for n, m in sorted(pts) if n not in drop]
        kept50 = [(n, m) for n, m in sorted(p50[name]) if n not in drop]
        n95, note95 = n_at_budget(kept) if kept else (None, "no points left")
        n50, note50 = n_at_budget(kept50) if kept50 else (None, "no points left")
        bound = ""
        if drop and note95 == "above range":
            # 16.7ms를 사이에 두는 두 점이 빼서 없어졌다: 값 대신 남은 가장 큰 N 이상이라는 범위를 낸다
            bound, n95, note95 = f"≥{kept[-1][0]}", None, "above kept range"
        if drop and note50 == "above range":
            n50, note50 = None, f"≥{kept50[-1][0]} (above kept range)"
        table.append({"series": name, "n_p95": round(n95) if n95 else "", "note_p95": note95, "n_p95_bound": bound,
                      "n_p50": round(n50) if n50 else "", "note_p50": note50,
                      "excluded": "; ".join(f"excluded: N={n} unmet" for n in sorted(drop)),
                      "points": " ".join(f"{n}:{m:.2f}" for n, m in sorted(pts)),
                      # 리뷰 M5: 각 점의 겹친 쌍(0.5초 표본 평균). 보간한 N이 겹침 방지가 무너진 구간이면 값을 그대로 쓰지 않는다
                      "overlap_points": " ".join(f"{n}:{ovl[name][n]:.0f}" if ovl[name].get(n) is not None else f"{n}:-" for n, _ in sorted(pts))})
    write_rows(out / "full-n-at-60fps.csv", table, ["series", "n_p95", "note_p95", "n_p95_bound", "n_p50", "note_p50", "excluded", "points", "overlap_points"])
    multi = {k: sorted(v) for k, v in p95.items() if len(v) >= 2}
    if multi:
        dropped = [f"{k.split(' ')[0]} N={n:,}" for k in sorted(multi) for n in sorted(excl.get(k, ()))]
        foot = (["Hollow points: the step does not do the same work at that N (equivalence check). They are left out of the max-N interpolation:",
                 ", ".join(dropped)]) if dropped else ""
        line_chart(multi, "Frame p95 vs N (moving orb)", out / "full-maxn.svg", {k: sorted(v) for k, v in p50.items() if k in multi},
                   hollow=excl, footnote=foot, legend_w=250)
    print(f"{batch}: target N={target:,} (from {target_src}) {aa_note}")
    print(f"equivalence: {eq_src} ({len(unmet)} UNMET points)")
    for t in table:
        print(f'{t["series"]}: N@16.7ms p95 = {t["n_p95"] or t["n_p95_bound"]} {t["note_p95"]} | p50 = {t["n_p50"]} {t["note_p50"]} | p95 points {t["points"]} | overlapping pairs {t["overlap_points"]}'
              + (f' | {t["excluded"]}' if t["excluded"] else ""))
    print(f"wrote {out}")


def full_unmet(batch, eq_path=None):
    """equivalence.csv(equivalence-check.py)에서 기능 미달(pass = UNMET)인 (조건, N). 조건은 P1의 "<V0|V1>-<구성>"이다(A/A와 탑뷰 행은 이름 끝이 달라 걸리지 않는다)."""
    p = Path(eq_path) if eq_path else ROOT / "Measurements" / batch / "equivalence.csv"
    out = set()
    if not p.exists():
        return out, f"{p} not found (no points excluded)"
    with open(p, newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if r.get("pass") == UNMET:
                out.add((r["cond"], int(r["n"])))
    return out, str(p)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch", help="총괄 묶음 이름(예: Full1). 주면 Measurements/<묶음>/만 읽어 Docs/figures/<묶음>/에 그린다")
    ap.add_argument("--target-n", type=int, help="--batch의 목표 총수를 직접 정한다(기본: batch.json의 target_n)")
    ap.add_argument("--out", help="--batch의 출력 폴더(기본 Docs/figures/<묶음>/). 커밋하지 않을 확인용 그림은 스크래치 폴더를 준다")
    ap.add_argument("--equivalence", help="--batch의 동등성 결과 CSV(기본 Measurements/<묶음>/equivalence.csv). 기능 미달(UNMET) 점을 최대 N 보간에서 뺀다")
    args = ap.parse_args()
    if args.batch:
        full_main(args.batch, args.target_n, args.out, args.equivalence)
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
