# -*- coding: utf-8 -*-
"""run-m3b-all.ps1 의 마무리: 묶음 1·2 결과 사실 요약을 쓴다 (해석 없음).

입력: Measurements/M3b/{batch-phases.csv, summary.csv, runs.csv, spikes.txt}, Saved/Lab/m3b-all.log
출력: Measurements/M3b/batch-report.md, Docs/Worklog/M3b.md 끝에 "## 묶음 1·2 결과" 절(이미 있으면 바꾼다).
실행: py -3 -I Scripts/m3b-report.py [--failed 0|1]
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent.parent
M = ROOT / "Measurements" / "M3b"
WORKLOG = ROOT / "Docs" / "Worklog" / "M3b.md"
HEAD = "## 묶음 1·2 결과 (측정 세션, 자동 생성)"


def read_csv(p):
    if not p.exists():
        return []
    with open(p, newline="", encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def fmt_t(sec):
    sec = int(sec)
    return f"{sec // 3600}:{sec % 3600 // 60:02d}:{sec % 60:02d}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--failed", type=int, default=0)
    args = ap.parse_args()

    phases = read_csv(M / "batch-phases.csv")
    summ = read_csv(M / "summary.csv")
    runs = read_csv(M / "runs.csv")
    log = (ROOT / "Saved" / "Lab" / "m3b-all.log")
    log_text = log.read_text(encoding="utf-8-sig", errors="replace") if log.exists() else ""
    # 최종 묶음(태그 fg)만 본다. 로그는 마지막 "M3b ALL start" 이후만 본다(앞선 시도의 실패 줄을 세지 않는다).
    if "M3b ALL start" in log_text:
        log_text = log_text[log_text.rindex("M3b ALL start"):]
    runs = [r for r in runs if str(r.get("final", "")) == "1"]
    summ = [r for r in summ if str(r.get("final", "")) == "1"]
    csv_dir = ROOT / "Saved" / "Profiling" / "CSV"
    files = sorted(str(p) for p in csv_dir.glob("M3b-*fg-r*.csv"))
    spikes = []
    for i in range(0, len(files), 60):
        res = subprocess.run([sys.executable, "-I", str(ROOT / "Scripts" / "spikes.py"), *files[i:i + 60], "--verbose"], capture_output=True, text=True, encoding="utf-8")
        spikes += [l for l in res.stdout.splitlines() if not l.startswith("runs=") and not l.startswith("periodic runs:")]
    n_runs = len(spikes)
    n_per = sum(1 for l in spikes if l.rstrip().endswith("PERIODIC"))
    spikes = [f"runs={n_runs} periodic={n_per} ({(n_per / n_runs if n_runs else 0):.0%}) factor=2.0x (r0 제외, 최종 묶음 fg 파일만)"] + spikes
    (M / "spikes.txt").write_text(chr(10).join(spikes) + chr(10), encoding="utf-8")

    out = [HEAD, ""]
    out.append(f"전체 실행 {'**실패로 중단**' if args.failed else '완료'}. 로그 `Saved/Lab/m3b-all.log`(git 밖). 해석은 쓰지 않았다.")
    out.append("")
    out.append("| 페이즈 | 시작 | 끝 | 소요 | 종료 코드 |")
    out.append("| --- | --- | --- | --- | --- |")
    total = 0
    for p in phases:
        total += int(p["seconds"])
        out.append(f"| {p['phase']} | {p['start']} | {p['end']} | {fmt_t(p['seconds'])} | {p['exit_code']} |")
    out.append(f"| 합계 | | | {fmt_t(total)} | |")
    out.append("")

    fails = re.findall(r"^FAILED: .*$", log_text, flags=re.M)
    warns = re.findall(r"^WARNING: .*$", log_text, flags=re.M)
    out.append(f"실패한 실행 {len(fails)}건" + ("".join(f"\n- `{x}`" for x in fails)) + ".")
    out.append("")
    out.append(f"`summarize.py` 경고 {len(warns)}건(`LabTypesAssets=substituted` 제외 등)" + ("".join(f"\n- {x}" for x in warns)) + ".")
    out.append("")

    builds = sorted({r.get("build", "").split(";")[0] for r in runs if r.get("build")})
    out.append(f"**점검.** 최종 묶음(`final=1`): runs.csv {len(runs)}행(r0 {sum(1 for r in runs if r.get('run') == '0')}행 포함), summary.csv {len(summ)}조건. `labbuild` 값 {len(builds)}개: {', '.join('`' + b + '`' for b in builds) or '(없음)'}. (runs.csv 전체는 옛 prelude 60행을 포함해 빌드가 3개다.)")
    out.append("")

    def med(variant):
        for r in summ:
            if r["variant"] == variant and r["n"] == "20000" and r["preset"] == "P1":
                return r
        return None

    a, b = med("m3mass-start-fg"), med("m3mass-aa-fg")
    out.append("**A/A (m3mass 20k P1, 프레임 p50 중앙값, ms)**")
    out.append("")
    out.append("| 구분 | 프레임 p50 | 최소\\~최대 | 프레임 p95 |")
    out.append("| --- | --- | --- | --- |")
    for name, r in (("시작(-start-fg)", a), ("끝(-aa-fg)", b)):
        if r:
            out.append(f"| {name} | {r['frame_p50_ms']} | {r['frame_p50_ms_min']}\\~{r['frame_p50_ms_max']} | {r['frame_p95_ms']} |")
        else:
            out.append(f"| {name} | (없음) | | |")
    if a and b:
        d = abs(float(a["frame_p50_ms"]) - float(b["frame_p50_ms"]))
        out.append("")
        out.append(f"시작과 끝의 프레임 p50 차이 {d:.2f} ms (판정선 0.5 ms: {'넘음' if d > 0.5 else '넘지 않음'}).")
    out.append("")

    out.append("**스파이크 검사 (`spikes.py M3b- --verbose`, 전문은 `Measurements/M3b/spikes.txt`)**")
    out.append("")
    head = next((l for l in spikes if l.startswith("runs=")), "(없음)")
    out.append(f"- 요약: `{head.strip()}`")
    per = [l.strip() for l in spikes if l.rstrip().endswith("PERIODIC") or l.startswith("periodic runs:")]
    out.append(f"- 주기적으로 표시된 줄 {len(per)}개" + ("".join(f"\n  - `{x}`" for x in per[:30])) + ("" if len(per) <= 30 else "\n  - …(나머지는 spikes.txt)"))
    out.append("")

    def row(variant, n="20000"):
        for r in summ:
            if r["variant"] == variant and r["n"] == n and r["preset"] == "P1":
                return r
        return None

    def rng(r, c):
        return f"{float(r[c]):.2f} ({float(r[c + '_min']):.2f}\~{float(r[c + '_max']):.2f})"

    out.append("**대표 조건 (P1, 3회 중앙값(최소\~최대), ms; 병목 = Game·Render·GPU p50 중 최대)**")
    out.append("")
    out.append("| 조건 | N | 종 | 프레임 p95 | 프레임 p50 | Game p50 | Render p50 | GPU p50 | 병목 | 대형 스킵 비율 |")
    out.append("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
    for v, n in (("m3mass-fg", "20000"), ("m3vis30-fg", "20000"), ("m3vis15-fg", "20000"), ("m3vis0-fg", "20000"), ("m3hys20-fg", "20000"),
                 ("m3mass-fg", "5000"), ("m3vis30-fg", "5000"), ("m3vis15-fg", "5000"), ("m3vis0-fg", "5000"), ("m3hys20-fg", "5000"),
                 ("m3mass-t2-fg", "20000"), ("m3mass-t2-fg", "5000"),
                 ("m3budgetdist-fg", "20000"), ("m3budgetdist-t2-fg", "20000"), ("m3sig-fg", "20000"), ("m3sig-t2-fg", "20000")):
        r = row(v, n)
        if r:
            out.append(f"| {v} | {n} | {r.get('types') or '-'} | {rng(r, 'frame_p95_ms')} | {rng(r, 'frame_p50_ms')} | {float(r['game_p50_ms']):.2f} | {float(r['render_p50_ms']):.2f} | {float(r['gpu_p50_ms']):.2f} | {r['bottleneck']} | {r.get('large_skip_ratio') or '-'} |")
        else:
            out.append(f"| {v} | {n} | | (없음) | | | | | | |")
    out.append("")
    base, m30 = row("m3mass-fg"), row("m3vis30-fg")
    if base and m30:
        out.append(f"기준점 대비(20k P1): `m3mass-fg`(기본 15/30 m) − `m3vis30-fg`(옛 M3 값, Base 30 m) = 프레임 p95 {float(base['frame_p95_ms']) - float(m30['frame_p95_ms']):+.2f} ms, p50 {float(base['frame_p50_ms']) - float(m30['frame_p50_ms']):+.2f} ms.")
        out.append("")
    t2r, t1r = row("m3mass-t2-fg"), row("m3mass-fg")
    if t2r and t1r:
        out.append(f"2종 − 1종(`m3mass`, 20k P1): 프레임 p95 {float(t2r['frame_p95_ms']) - float(t1r['frame_p95_ms']):+.2f} ms, p50 {float(t2r['frame_p50_ms']) - float(t1r['frame_p50_ms']):+.2f} ms.")
        out.append("")

    out.append("**코드 A/B (ab2, 20k P1)**")
    out.append("")
    out.append("| 조건 | 프레임 p95 | 프레임 p50 | Game p50 |")
    out.append("| --- | --- | --- | --- |")
    for v in ("m3mass-ab2-fg", "m3lock0-ab2-fg", "m3order-ab2-fg"):
        r = row(v)
        if r:
            out.append(f"| {v} | {rng(r, 'frame_p95_ms')} | {rng(r, 'frame_p50_ms')} | {float(r['game_p50_ms']):.2f} |")
    out.append("")

    # N@16.7ms (P1): plot.py와 같은 로그 보간. p95 정의 = 프레임 p95, p50 정의 = Game/Render/GPU/프레임 p50 중 최댓값(M3까지의 정의).
    sys.path.insert(0, str(ROOT / "Scripts"))
    from plot import n_at_budget  # noqa: E402

    def n_at(variant, defn):
        pts = []
        for r in summ:
            if r["variant"] != variant or r["preset"] != "P1":
                continue
            if defn == "p95":
                v = float(r["frame_p95_ms"])
            else:
                v = max(float(r[c]) for c in ("game_p50_ms", "render_p50_ms", "gpu_p50_ms", "frame_p50_ms") if r.get(c))
            pts.append((int(r["n"]), v))
        n, note = n_at_budget(sorted(pts))
        rng_n = f"{min(p[0] for p in pts):,}\~{max(p[0] for p in pts):,}" if pts else "-"
        if not pts:
            return "-"
        if n is None:
            return f"< {min(p[0] for p in pts):,}"
        return (">" if note == "above range" else "") + f"{n:,.0f}"

    out.append("**N@16.7ms (P1, 측정 N 사이 로그 보간. p95 정의 / p50 정의 = Game·Render·GPU·프레임 p50 중 최댓값)**")
    out.append("")
    out.append("| 단계 | 1종 p95 | 1종 p50 | 2종(t2) p95 | 2종(t2) p50 |")
    out.append("| --- | --- | --- | --- | --- |")
    for name, v in (("M0 full", "m0full"), ("M0 static", "m0static"), ("M1", "m1mass"), ("M2", "m2mass"), ("M3 (m3mass)", "m3mass")):
        out.append(f"| {name} | {n_at(v + '-fg', 'p95')} | {n_at(v + '-fg', 'p50')} | {n_at(v + '-t2-fg', 'p95')} | {n_at(v + '-t2-fg', 'p50')} |")
    out.append("")
    out.append("`>`는 측정한 가장 큰 N에서도 16.7ms를 넘지 않았다는 뜻(그 N 이상). N 범위는 M0가 1k\~5k, M1\~M3가 1k\~50k다. `< 1,000`은 최종 묶음의 가장 작은 N(1,000)이 이미 16.7ms를 넘어 보간할 수 없다는 뜻이다(M0 full은 M0 마일스톤 측정의 더 작은 N 값을 쓴다). 2종 M0는 대형 개체 10%를 같은 M0 액터 경로로 돌린 값이다.")
    out.append("")

    out.append("**E3 (3a·3c) 프레임 p95, 20k P1 (3회 중앙값(최소\~최대), ms)**")
    out.append("")
    out.append("| 변형 | 1종 | 2종(t2) |")
    out.append("| --- | --- | --- |")
    for label, v in (("m3mass (기준)", "m3mass"), ("m3budgetdist (3a)", "m3budgetdist"), ("m3sig (3c)", "m3sig")):
        r1, r2 = row(v + "-fg"), row(v + "-t2-fg")
        out.append(f"| {label} | {rng(r1, 'frame_p95_ms') if r1 else '(없음)'} | {rng(r2, 'frame_p95_ms') if r2 else '(없음)'} |")
    out.append("")

    traces = sorted((ROOT / "Saved" / "Traces").glob("M3b-*fg-r*.utrace"))
    out.append("**트레이스 파일 (git 밖, `Saved/Traces/`)**: " + (", ".join(f"`{t.name}`({t.stat().st_size // 1_000_000} MB)" for t in traces) or "(없음)") + ". 트레이스 실행의 수치는 비교에 쓰지 않는다.")
    out.append("")

    t2 = [r for r in runs if r.get("types_assets") == "substituted"]
    out.append(f"`LabTypesAssets=substituted` 행 {len(t2)}개(summary에서 제외).")
    out.append("")

    text = "\n".join(out) + "\n"
    (M / "batch-report.md").write_text(text, encoding="utf-8")
    s = WORKLOG.read_text(encoding="utf-8")
    if HEAD in s:
        s = s[: s.index(HEAD)].rstrip("\n") + "\n\n"
    else:
        s = s.rstrip("\n") + "\n\n"
    WORKLOG.write_text(s + text, encoding="utf-8")
    print(f"wrote {M / 'batch-report.md'} and Worklog section")


if __name__ == "__main__":
    main()
