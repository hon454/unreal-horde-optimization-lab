"""총괄 묶음의 단계 사이 동등성을 검사한다(v2 W12. 계획은 Docs/Worklog/V2.md "W12 측정 준비").

단계 사이에 같은 일을 하는지 본다. 같은 (개체 수 N, 속도 V0|V1)의 조건끼리 견준다. 입력은 성공한 실행의 CSV 끝 메타데이터(Saved/Profiling/CSV/<묶음>-*.csv, r1 이상.
트레이스·촬영 라벨은 접두사가 달라 읽지 않는다)와 Measurements/<묶음>/runs.csv(겹친 쌍의 평균, 왕복 전환. summarize.py가 만든다. 없으면 그 검사는 건너뛰고 알린다).

검사 항목(조건의 값 = 그 조건의 실행들의 중앙값)
1. 살아 있는 총수: LabAliveMin = LabAliveMax = N(허용 0). 모든 조건. 어기면 실패.
2. 초당 사망(LabDeathRate), 오브 수평 30m 안 평균(LabNear30Mean): 같은 (N, 속도)의 기준과 ±3%(조건 표 equivalence). 기준 = 같은 (N, 속도)의 단계 조건 가운데 equiv가 strict인 것
   (단계 3~8)의 중앙값의 중앙값. 조건 표의 equiv가 strict인 조건은 어기면 실패, report인 조건(단계 1, 단계 2, 겹침 방지 끔, 엔진 회피 더하기)은 값만 보고한다.
   단계 2는 엔진 회피가 오브 둘레에 무리를 쌓아 두 값이 정의상 다르다(V2W10Ladder: 초당 사망 48.5 대 56.7, 30m 안 1,997 대 927. D51).
3. 겹친 쌍의 평균(runs.csv overlap_mean): 조건 표 equivalence.overlap_mean의 범위. 구성의 overlap 종류(sep, zero, engine, off)와 속도로 범위를 고른다.
   근거는 조건 표의 overlap_basis. sep과 zero(N 2000 이하)는 범위 밖이면 실패, 그 밖은 보고만 한다. 단계 2가 범위를 벗어나는 것은 실패로 막지 않는다(D51).
4. 캡슐 트레이스: LabActorCollision이 trace나 pawn이면 트레이스 수가 0보다 크고 적중 + 막힘 = 트레이스 수(빗나감 0)여야 한다. 어기면 실패.
5. 왕복 전환(runs.csv sw_round_all, 초당): LabRepHold가 0보다 크면 조건 표의 round_trip_max_per_sec 이하여야 한다(D48). 어기면 실패. 최소 유지가 없는 조건은 값만 보고한다.

허용 편차의 근거: 사망·30m 안 평균은 V2W10Ladder의 단계 3~8이 56.62~56.76 / 925~930(±0.4%)이고 V2W11Sweep의 네 조건이 56.7~56.8 / 926~927이다. ±3%는 v1과 같은 값이고
실행 간 범위의 7배 남짓이다(Docs/Worklog/Q5.md "패키지 확인"의 근거를 잇는다). 정의상 값이 달라지는 조건은 조건 표의 equiv를 report로 둔다.

실행: py -3 -I Scripts/equivalence-check.py --batch Full4 [--out <csv>]
출력: Measurements/<묶음>/equivalence.csv와 요약. 실패가 있으면 종료 코드 1. 표준 라이브러리만 쓴다.
"""
import argparse
import csv
import statistics
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import lab_conditions as lc  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
CSV_DIR = ROOT / "Saved" / "Profiling" / "CSV"
AA_RUN_MIN = 10  # 묶음 끝 A/A는 회차 11~13


def read_meta(path, tail=1 << 19):
    """CSV 끝의 [키],값 행만 읽는다(키는 소문자). 메타데이터가 tail 바이트 안에 있다고 본다(LabCVars 등 긴 행 포함)."""
    with open(path, "rb") as f:
        f.seek(0, 2)
        size = f.tell()
        f.seek(max(0, size - tail))
        text = f.read().decode("utf-8", errors="replace")
    meta = {}
    for line in text.splitlines():
        if not line.startswith("["):
            continue
        row = next(csv.reader([line]))
        for i in range(0, len(row) - 1, 2):
            meta[row[i].strip("[]").lower()] = row[i + 1]
    return meta


def num(meta, key):
    try:
        return float(meta[key])
    except (KeyError, ValueError):
        return None


def failed_labels(batch):
    p = ROOT / "Measurements" / batch / "failures.csv"
    out = set()
    if p.exists():
        with open(p, newline="", encoding="utf-8-sig") as f:
            for r in csv.DictReader(f):
                out.add(r["label"])
    return out


def runs_table(batch):
    p = ROOT / "Measurements" / batch / "runs.csv"
    out = {}
    if p.exists():
        with open(p, newline="", encoding="utf-8") as f:
            for r in csv.DictReader(f):
                out[r["label"]] = r
    return out


def med(vals):
    vals = [v for v in vals if v is not None]
    return statistics.median(vals) if vals else None


def fnum(r, k):
    try:
        return float(r[k])
    except (KeyError, ValueError, TypeError):
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch", required=True, help="라벨 접두사(Saved/Profiling/CSV/<묶음>-*.csv)")
    ap.add_argument("--out", help="결과 CSV 경로(기본 Measurements/<묶음>/equivalence.csv)")
    a = ap.parse_args()

    doc = lc.load()
    cfgs = lc.configs(doc)
    eq = doc["equivalence"]
    failed = failed_labels(a.batch)
    table = runs_table(a.batch)

    # (n, 속도, 구성, aa 여부) -> 실행들
    runs, skipped = {}, 0
    for path in sorted(CSV_DIR.glob(f"{a.batch}-*.csv")):
        p = lc.parse_label(path.stem)
        if not p or p["kind"] or p["run"] == 0 or path.stem in failed:
            continue
        if p["cfg"] not in cfgs:
            skipped += 1
            continue
        meta = read_meta(path)
        t = table.get(path.stem, {})
        runs.setdefault((p["n"], p["speed"], p["cfg"], p["run"] > AA_RUN_MIN), []).append({
            "label": path.stem, "death_rate": num(meta, "labdeathrate"), "near30_mean": num(meta, "labnear30mean"),
            "alive_min": num(meta, "labalivemin"), "alive_max": num(meta, "labalivemax"),
            "traces": num(meta, "labcapsuletraces"), "hits": num(meta, "labcapsulehits"), "blocked": num(meta, "labcapsuleblocked"),
            "collision": meta.get("labactorcollision", ""), "hold": num(meta, "labrephold"),
            "overlap": fnum(t, "overlap_mean"), "round": fnum(t, "sw_round_all")})
    if not runs:
        print(f"no runs for batch {a.batch} in {CSV_DIR}")
        return 1
    have_table = bool(table)

    groups = {}
    for (n, speed, cfg, aa), rs in runs.items():
        groups.setdefault((n, speed), []).append((cfg, aa, rs))

    rows, bad, reports = [], 0, 0
    for (n, speed), conds in sorted(groups.items()):
        peers = [c for c in conds if not c[1] and cfgs[c[0]]["kind"] == "step" and cfgs[c[0]]["equiv"] == "strict"]
        ref = {k: med([med([r[k] for r in rs]) for _, _, rs in peers]) for k in ("death_rate", "near30_mean")} if peers else {"death_rate": None, "near30_mean": None}
        moving = "moving" if speed == "V0" else "still"
        for cfg, aa, rs in sorted(conds, key=lambda c: (c[1], c[0])):
            c = cfgs[cfg]
            strict = c["equiv"] == "strict"
            row = {"n": n, "speed": speed, "cond": f"{speed}-{cfg}" + ("-aa" if aa else ""), "runs": len(rs), "equiv": c["equiv"]}
            fails, notes = [], []
            # 1. 살아 있는 총수
            amin = [r["alive_min"] for r in rs if r["alive_min"] is not None]
            amax = [r["alive_max"] for r in rs if r["alive_max"] is not None]
            row["alive_min"] = int(min(amin)) if amin else ""
            row["alive_max"] = int(max(amax)) if amax else ""
            if not (amin and amax and min(amin) == n and max(amax) == n):
                fails.append(f"alive {row['alive_min']}..{row['alive_max']} != N")
            # 2. 사망, 30m 안
            for k, tol in (("death_rate", eq["death_rate_tol"]), ("near30_mean", eq["near30_tol"])):
                v = med([r[k] for r in rs])
                vals = [r[k] for r in rs if r[k] is not None]
                row[k] = round(v, 3) if v is not None else ""
                row[f"{k}_min"] = round(min(vals), 3) if vals else ""
                row[f"{k}_max"] = round(max(vals), 3) if vals else ""
                row[f"{k}_ref"] = round(ref[k], 3) if ref[k] is not None else ""
                dev = (v - ref[k]) / ref[k] if v is not None and ref[k] else None
                row[f"{k}_dev_pct"] = round(dev * 100, 2) if dev is not None else ""
                if dev is None:
                    row[f"{k}_ok"] = "no peers"
                elif abs(dev) <= tol:
                    row[f"{k}_ok"] = "ok"
                elif strict:
                    row[f"{k}_ok"] = "FAIL"
                    fails.append(f"{k} dev {dev * 100:+.2f}% > {tol * 100:.0f}%")
                else:
                    row[f"{k}_ok"] = "report"
                    notes.append(f"{k} dev {dev * 100:+.2f}%")
            # 3. 겹친 쌍
            ov = med([r["overlap"] for r in rs])
            row["overlap_mean"] = round(ov, 1) if ov is not None else ""
            klass = c["overlap"]
            lo_hi = eq["overlap_mean"].get(klass if klass == "zero" else f"{klass}_{moving}")
            row["overlap_class"] = klass
            row["overlap_range"] = f"{lo_hi[0]}..{lo_hi[1]}" if lo_hi else ""
            if ov is None:
                row["overlap_ok"] = "no runs.csv" if not have_table else "no value"
            elif lo_hi and lo_hi[0] <= ov <= lo_hi[1]:
                row["overlap_ok"] = "ok"
            else:
                blocking = klass == "sep" or (klass == "zero" and n <= 2000)
                row["overlap_ok"] = "FAIL" if blocking else "report"
                (fails if blocking else notes).append(f"overlap {ov:.0f} outside {row['overlap_range']} ({klass})")
            # 4. 캡슐 트레이스
            col = rs[0]["collision"]
            tr = sum(r["traces"] or 0 for r in rs)
            hit = sum(r["hits"] or 0 for r in rs)
            blk = sum(r["blocked"] or 0 for r in rs)
            row["capsule_traces"], row["capsule_miss"] = int(tr), int(tr - hit - blk)
            if col in ("trace", "pawn"):
                if tr <= 0:
                    row["capsule_ok"] = "FAIL"
                    fails.append("capsule traces 0")
                elif tr - hit - blk != 0:
                    row["capsule_ok"] = "FAIL"
                    fails.append(f"capsule misses {int(tr - hit - blk)}")
                else:
                    row["capsule_ok"] = "ok"
            else:
                row["capsule_ok"] = "n/a"
            # 5. 왕복 전환
            rt = med([r["round"] for r in rs])
            hold = rs[0]["hold"]
            row["round_per_sec"] = round(rt, 3) if rt is not None else ""
            if rt is None:
                row["round_ok"] = "no runs.csv" if not have_table else "no value"
            elif hold and hold > 0:
                if rt <= eq["round_trip_max_per_sec"]:
                    row["round_ok"] = "ok"
                else:
                    row["round_ok"] = "FAIL"
                    fails.append(f"round trips {rt:.2f}/s with hold {int(hold)} ms")
            else:
                row["round_ok"] = "report"
                if rt > 0:
                    notes.append(f"round trips {rt:.2f}/s (no min hold)")
            row["pass"] = "FAIL" if fails else ("report" if notes else "ok")
            row["note"] = "; ".join(fails + notes)
            bad += bool(fails)
            reports += (not fails) and bool(notes)
            rows.append(row)

    fields = ["n", "speed", "cond", "runs", "equiv", "alive_min", "alive_max",
              "death_rate", "death_rate_min", "death_rate_max", "death_rate_ref", "death_rate_dev_pct", "death_rate_ok",
              "near30_mean", "near30_mean_min", "near30_mean_max", "near30_mean_ref", "near30_mean_dev_pct", "near30_mean_ok",
              "overlap_mean", "overlap_class", "overlap_range", "overlap_ok", "capsule_traces", "capsule_miss", "capsule_ok", "round_per_sec", "round_ok", "pass", "note"]
    out = Path(a.out) if a.out else ROOT / "Measurements" / a.batch / "equivalence.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    for r in rows:
        print(f'N={r["n"]:>6} {r["cond"]:<14} runs={r["runs"]} death={r["death_rate"]}/s ({r["death_rate_dev_pct"]}%) near30={r["near30_mean"]} ({r["near30_mean_dev_pct"]}%) '
              f'overlap={r["overlap_mean"]} capsule_miss={r["capsule_miss"]} round={r["round_per_sec"]}/s {r["pass"]} {r["note"]}')
    print(f"equivalence: {len(rows)} conditions, {len(rows) - bad - reports} pass, {reports} report, {bad} FAIL "
          f"(tol death {eq['death_rate_tol']:.0%}, near30 {eq['near30_tol']:.0%}, alive exact, overlap range by class) -> {out}")
    if not have_table:
        print(f"note: Measurements/{a.batch}/runs.csv not found; overlap and round-trip checks skipped (run summarize.py --batch {a.batch} first)")
    if skipped:
        print(f"note: {skipped} CSVs with unknown configs skipped")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
