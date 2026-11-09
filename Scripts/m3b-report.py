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
    # 최종 묶음(태그 fin)만 본다. 로그는 마지막 "M3b ALL start" 이후만 본다(앞선 시도의 실패 줄을 세지 않는다).
    if "M3b ALL start" in log_text:
        log_text = log_text[log_text.rindex("M3b ALL start"):]
    runs = [r for r in runs if str(r.get("final", "")) == "1"]
    summ = [r for r in summ if str(r.get("final", "")) == "1"]
    csv_dir = ROOT / "Saved" / "Profiling" / "CSV"
    files = sorted(str(p) for p in csv_dir.glob("M3b-*fin-r*.csv"))
    spikes = []
    for i in range(0, len(files), 60):
        res = subprocess.run([sys.executable, "-I", str(ROOT / "Scripts" / "spikes.py"), *files[i:i + 60], "--verbose"], capture_output=True, text=True, encoding="utf-8")
        spikes += [l for l in res.stdout.splitlines() if not l.startswith("runs=") and not l.startswith("periodic runs:")]
    n_runs = len(spikes)
    n_per = sum(1 for l in spikes if l.rstrip().endswith("PERIODIC"))
    spikes = [f"runs={n_runs} periodic={n_per} ({(n_per / n_runs if n_runs else 0):.0%}) factor=2.0x (r0 제외, 최종 묶음 fin 파일만)"] + spikes
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

    builds = sorted({r.get("build", "").split(";")[0] for r in runs if r.get("run") != "0" and r.get("build")})
    out.append(f"runs.csv {len(runs)}행, summary.csv {len(summ)}조건. `labbuild` 값: {', '.join('`' + b + '`' for b in builds) or '(없음)'}.")
    out.append("")

    def med(variant):
        for r in summ:
            if r["variant"] == variant and r["n"] == "20000" and r["preset"] == "P1":
                return r
        return None

    a, b = med("m3mass-start-fin"), med("m3mass-aa-fin")
    out.append("**A/A (m3mass 20k P1, 프레임 p50 중앙값, ms)**")
    out.append("")
    out.append("| 구분 | 프레임 p50 | 최소\\~최대 | 프레임 p95 |")
    out.append("| --- | --- | --- | --- |")
    for name, r in (("시작(-start-fin)", a), ("끝(-aa-fin)", b)):
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
