"""총괄 묶음 끝 검사 하나(리뷰 M4, Docs/Guides/measurement.md "묶음 판정"). runs.csv(summarize.py가 만든다)만 읽는다.

검사
1. 느린 실행: 한 실행의 Game p50과 Render p50이 그 조건 원 실행 중앙값보다 함께 2% 넘게 길면 표시한다(Scripts/batch_rules.py). 러너가 새 회차(7~9)로 한 번 다시 잰다.
   다시 잰 실행이 아직 없는 표시(slow)는 실패다. 다시 잰 실행도 느리면(slow-kept) 둘 다 요약에 넣고 보고만 한다. 결과는 slow.csv에 쓴다.
2. A/A: 단계 8 이동 조건(V0-s8, N=목표 총수, P1)의 회차 11 이상 실행 p95 중앙값과 요약에 쓴 단계 8 p95 중앙값이 0.5ms를 넘게 다르면 실패(묶음을 참고로 내린다).
   A/A 실행이 없어도 실패다.
3. 단계 8의 모든 실행(사다리 3 + A/A 3, 느린 실행과 재측정 포함)이 프레임 p95 16.7ms 이하여야 한다. 넘는 실행이 있으면 실패다.
보고만 하는 것: ADR-0019 판정식(단계 8 이동 조건 p95 중앙값 + 0.6ms <= 16.7ms). 식은 바꾸지 않는다.

실행: py -3 -I Scripts/batch-check.py --batch Full4 [--dir <runs.csv가 있는 폴더>] [--out <slow.csv 경로>] [--list-slow]
--list-slow  다시 재야 할 느린 실행을 `RERUN <라벨> <새 회차>` 줄로 출력하고 끝낸다(러너가 읽는다. 종료 코드 0).
실패가 있으면 종료 코드 1. 표준 라이브러리만 쓴다.
"""
import argparse
import csv
import statistics
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import batch_rules as br  # noqa: E402
import lab_conditions as lc  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
BUDGET_MS = 16.7
AA_TOL_MS = 0.5
JUDGE_MARGIN_MS = 0.6   # ADR-0019 결정 3
COND = "V0-s8"


def fnum(r, k):
    try:
        return float(r[k])
    except (KeyError, ValueError, TypeError):
        return None


def load_runs(path):
    with open(path, newline="", encoding="utf-8") as f:
        return [r for r in csv.DictReader(f)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch", required=True)
    ap.add_argument("--dir", help="runs.csv가 있는 폴더(기본 Measurements/<묶음>)")
    ap.add_argument("--out", help="slow.csv 경로(기본 <폴더>/slow.csv)")
    ap.add_argument("--list-slow", action="store_true")
    a = ap.parse_args()

    d = Path(a.dir) if a.dir else ROOT / "Measurements" / a.batch
    runs_path = d / "runs.csv"
    if not runs_path.exists():
        print(f"FAIL: {runs_path} not found (run summarize.py --batch {a.batch} first)")
        return 1
    rows = [r for r in load_runs(runs_path) if int(r["run"]) >= 1]
    target = int(lc.load()["scenario"]["target_n"])

    # 1. 느린 실행
    by_cond = {}
    for r in rows:
        if int(r["run"]) <= br.RERUN_RUN_MAX:
            by_cond.setdefault((r["variant"], int(r["n"]), r["preset"]), []).append(r)
    marks, med_of = {}, {}
    for key, rs in by_cond.items():
        m = br.slow_marks(rs)
        marks.update(m)
        med_of[key] = br.condition_medians(rs)
    if a.list_slow:
        for key, rs in sorted(by_cond.items()):
            flagged = sorted((r for r in rs if marks.get(r["label"]) == "slow"), key=lambda r: int(r["run"]))
            allflag = sorted((r for r in rs if marks.get(r["label"], "").startswith("slow")), key=lambda r: int(r["run"]))
            for r in flagged:
                print(f'RERUN {r["label"]} {br.RERUN_BASE + allflag.index(r)}')
        return 0

    slow_rows = []
    for key, rs in sorted(by_cond.items()):
        med = med_of[key]
        for r in sorted(rs, key=lambda r: int(r["run"])):
            mk = marks.get(r["label"], "")
            if not mk:
                continue
            g, n = fnum(r, "game_p50_ms"), fnum(r, "render_p50_ms")
            slow_rows.append({"label": r["label"], "cond": r["variant"], "n": r["n"], "preset": r["preset"], "run": r["run"], "mark": mk,
                              "game_p50_ms": g, "render_p50_ms": n, "cond_game_median": round(med[0], 3), "cond_render_median": round(med[1], 3),
                              "game_pct": round((g / med[0] - 1) * 100, 1), "render_pct": round((n / med[1] - 1) * 100, 1),
                              "frame_p95_ms": r.get("frame_p95_ms", "")})
    out = Path(a.out) if a.out else d / "slow.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    fields = ["label", "cond", "n", "preset", "run", "mark", "game_p50_ms", "render_p50_ms", "cond_game_median", "cond_render_median", "game_pct", "render_pct", "frame_p95_ms"]
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(slow_rows)

    fails = []
    pending = [r for r in slow_rows if r["mark"] == "slow"]
    kept = [r for r in slow_rows if r["mark"] == "slow-kept"]
    replaced = [r for r in slow_rows if r["mark"] == "slow-replaced"]
    print(f"slow runs: {len(replaced)} replaced by a rerun, {len(kept)} slow again (both used), {len(pending)} without a rerun -> {out}")
    for r in slow_rows:
        if r["mark"].startswith("slow"):
            print(f'  {r["mark"]:<14} {r["label"]} game {r["game_pct"]:+.1f}% render {r["render_pct"]:+.1f}% p95 {r["frame_p95_ms"]}')
    if pending:
        fails.append(f"{len(pending)} slow runs have no rerun")

    # 2. A/A
    s8 = [r for r in rows if r["variant"] == COND and int(r["n"]) == target and r["preset"] == "P1"]
    ladder = [r for r in s8 if int(r["run"]) <= br.AA_RUN_MIN and marks.get(r["label"]) != "slow-replaced"]
    aa = [r for r in s8 if int(r["run"]) > br.AA_RUN_MIN]
    ladder_p95 = [fnum(r, "frame_p95_ms") for r in ladder if fnum(r, "frame_p95_ms") is not None]
    aa_p95 = [fnum(r, "frame_p95_ms") for r in aa if fnum(r, "frame_p95_ms") is not None]
    if not aa_p95 or not ladder_p95:
        fails.append(f"A/A: {len(aa_p95)} A/A runs and {len(ladder_p95)} ladder runs of {COND} N={target} (both needed)")
        print(f"A/A: missing ({len(aa_p95)} A/A runs, {len(ladder_p95)} ladder runs)")
    else:
        lm, am = statistics.median(ladder_p95), statistics.median(aa_p95)
        print(f"A/A: ladder {COND} median p95 {lm:.2f} ms ({len(ladder_p95)} runs), A/A median {am:.2f} ms ({len(aa_p95)} runs), diff {am - lm:+.2f} ms (limit {AA_TOL_MS} ms)")
        if abs(am - lm) > AA_TOL_MS:
            fails.append(f"A/A differs {am - lm:+.2f} ms > {AA_TOL_MS} ms (the batch becomes reference-only)")
        print(f"ADR-0019 judgement (report only): median p95 {lm:.2f} + {JUDGE_MARGIN_MS} = {lm + JUDGE_MARGIN_MS:.2f} ms {'<=' if lm + JUDGE_MARGIN_MS <= BUDGET_MS else '>'} {BUDGET_MS} ms")

    # 3. 단계 8의 모든 실행
    allp95 = [(fnum(r, "frame_p95_ms"), r["label"]) for r in s8 if fnum(r, "frame_p95_ms") is not None]
    over = [(v, lb) for v, lb in allp95 if v > BUDGET_MS]
    if allp95:
        print(f"step 8 runs: {len(allp95)} runs, max p95 {max(v for v, _ in allp95):.2f} ms, {len(over)} above {BUDGET_MS} ms")
        for v, lb in over:
            print(f"  over {lb} {v:.2f} ms")
    else:
        print("step 8 runs: none")
    if over:
        fails.append(f"{len(over)} step 8 runs above {BUDGET_MS} ms")
    if not allp95:
        fails.append("no step 8 runs")

    if fails:
        print("BATCH CHECK FAIL: " + "; ".join(fails))
        return 1
    print("BATCH CHECK ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
