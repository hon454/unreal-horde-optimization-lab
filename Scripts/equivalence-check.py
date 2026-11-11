"""총괄 묶음의 단계 사이 동등성을 검사한다(Docs/Worklog/Q5.md "동등성 통계", "2단위").

성공한 실행(Saved/Profiling/CSV/<묶음>-*.csv의 r1 이상. 실패 CSV는 하위 폴더로 옮겨져 있어 읽지 않는다)의 CSV 메타데이터
LabDeathRate(초당 사망), LabNear30Mean(오브 수평 30m 안 개체 수의 측정 구간 평균), LabAliveMin·LabAliveMax(살아 있는 총수)를 읽어
같은 N·프리셋의 조건 사이 편차를 본다. 입력은 CSV 끝의 메타데이터 행뿐이라 프레임 행은 읽지 않는다.

기준 = 같은 (N, 프리셋)의 사다리 조건들(JSON group ladder) 조건 중앙값의 중앙값. 사다리가 없으면 그 그룹의 모든 조건으로 정한다.
조건의 값 = 그 조건의 성공한 실행들의 중앙값. 빼 보기·하위 실험·A/A(회차 11 이상)도 같은 기준과 비교한다. 트레이스·촬영 라벨(<묶음>trace-, <묶음>shot-)은 접두사가 달라 읽지 않는다.

허용 편차(근거: Q5 패키지 확인, Worklog/Q5.md "패키지 확인(Test, 6f9c18e)" 표, 2천 P1 10초 열두 실행)
- 초당 사망: 같은 시드에서 155~157건(중앙값 대비 ±0.7%, 범위 1.3%). 허용 ±3%로 둔다(측정 길이가 60초로 늘면 궤적 분기가 누적되고 Poisson 잡음은 건수의 제곱근에 비례하므로 관측 범위의 두 배 남짓 여유).
- 오브 30m 안 평균: 액터 단계 124.3, Mass 단계 125.6~125.9(액터가 약 1% 적다). 허용 ±3%로 둔다(같은 이유, 액터 이동과 엔티티 이동의 경로 차이 약 1%를 포함).
- 살아 있는 총수: 재생성이 같은 프레임이라 정확히 N이다. LabAliveMin = LabAliveMax = N이 아니면 실패(허용 0).
정의상 값이 달라지는 조건은 EXCEPTIONS에 이유와 함께 적는다. 지금은 없다: 빼 보기 아홉 행과 하위 실험은 모두 개체의 이동·표현·비용만 바꾸고 범위 피해와 저격의 판정은 바꾸지 않는다
(사망은 체력 프래그먼트 또는 ALabMinion 체력 차감, 30m 안 수는 엔티티·액터 위치). 실제로 어긋나면 그것이 조사할 일이므로 실패로 둔다.

실행: py -3 -I Scripts/equivalence-check.py --batch Full1 [--out <csv>] [--tol-death 0.03] [--tol-near30 0.03]
출력: Measurements/<묶음>/equivalence.csv와 통과·실패 요약. 실패가 있으면 종료 코드 1. 표준 라이브러리만 쓴다.
"""
import argparse
import csv
import json
import re
import statistics
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CSV_DIR = ROOT / "Saved" / "Profiling" / "CSV"
CONDITIONS = Path(__file__).resolve().parent / "full-conditions.json"
LABEL_RE = re.compile(r"^(?P<batch>[A-Za-z0-9]+)-(?P<step>[A-Z]\d+)-(?P<variant>[A-Za-z0-9]+)-N(?P<n>\d+)-(?P<preset>P[12])-r(?P<run>\d+)$")
TOL_DEATH = 0.03
TOL_NEAR30 = 0.03
# 조건 이름 -> 이유. 정의상 값이 달라져 편차 검사에서 뺄 조건(사망 수·30m 안 수 키 단위로 {"death_rate", "near30_mean"}).
EXCEPTIONS = {}
AA_RUN_MIN = 10  # run-full.ps1: A/A는 회차 11~13


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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch", default="Full1", help="라벨 접두사(Saved/Profiling/CSV/<묶음>-*.csv)")
    ap.add_argument("--out", help="결과 CSV 경로(기본 Measurements/<묶음>/equivalence.csv)")
    ap.add_argument("--tol-death", type=float, default=TOL_DEATH)
    ap.add_argument("--tol-near30", type=float, default=TOL_NEAR30)
    a = ap.parse_args()

    doc = json.loads(CONDITIONS.read_text(encoding="utf-8"))
    by_label = {f'{c["label_step"]}-{c["label_variant"]}': c for c in doc["conditions"]}
    failed = failed_labels(a.batch)

    # (n, preset, cond 이름, aa 여부) -> 실행들의 값
    runs = {}
    skipped = 0
    for path in sorted(CSV_DIR.glob(f"{a.batch}-*.csv")):
        m = LABEL_RE.match(path.stem)
        if not m or int(m["run"]) == 0 or path.stem in failed:
            continue
        cond = by_label.get(f'{m["step"]}-{m["variant"]}')
        if not cond:
            skipped += 1
            continue
        meta = read_meta(path)
        aa = int(m["run"]) > AA_RUN_MIN
        key = (int(m["n"]), m["preset"], cond["name"], aa)
        runs.setdefault(key, []).append({
            "label": path.stem, "death_rate": num(meta, "labdeathrate"), "near30_mean": num(meta, "labnear30mean"),
            "alive_min": num(meta, "labalivemin"), "alive_max": num(meta, "labalivemax"),
            "deaths": num(meta, "labdeaths"), "snipes": num(meta, "labsnipes")})
    if not runs:
        print(f"no runs for batch {a.batch} in {CSV_DIR}")
        return 1

    def med(vals):
        vals = [v for v in vals if v is not None]
        return statistics.median(vals) if vals else None

    groups = {}
    for (n, preset, name, aa), rs in runs.items():
        groups.setdefault((n, preset), []).append((name, aa, rs))

    rows, bad = [], 0
    for (n, preset), conds in sorted(groups.items()):
        ladder = [c for c in conds if not c[1] and by_label_group(doc, c[0]) == "ladder"]
        pool = ladder or conds
        ref = {k: med([med([r[k] for r in rs]) for _, _, rs in pool]) for k in ("death_rate", "near30_mean")}
        for name, aa, rs in sorted(conds, key=lambda c: (c[1], c[0])):
            row = {"n": n, "preset": preset, "cond": name + ("-aa" if aa else ""), "runs": len(rs)}
            verdict = []
            for k, tol in (("death_rate", a.tol_death), ("near30_mean", a.tol_near30)):
                v = med([r[k] for r in rs])
                vals = [r[k] for r in rs if r[k] is not None]
                row[k] = round(v, 3) if v is not None else ""
                row[f"{k}_min"] = round(min(vals), 3) if vals else ""
                row[f"{k}_max"] = round(max(vals), 3) if vals else ""
                row[f"{k}_ref"] = round(ref[k], 3) if ref[k] is not None else ""
                dev = (v - ref[k]) / ref[k] if v is not None and ref[k] else None
                row[f"{k}_dev_pct"] = round(dev * 100, 2) if dev is not None else ""
                exc = k in EXCEPTIONS.get(name, {})
                ok = exc or (dev is not None and abs(dev) <= tol)
                row[f"{k}_ok"] = "exception" if exc else ("ok" if ok else "FAIL")
                if not ok:
                    verdict.append(f"{k} dev {dev * 100:+.2f}% > {tol * 100:.0f}%" if dev is not None else f"{k} missing")
            amin = [r["alive_min"] for r in rs if r["alive_min"] is not None]
            amax = [r["alive_max"] for r in rs if r["alive_max"] is not None]
            row["alive_min"] = int(min(amin)) if amin else ""
            row["alive_max"] = int(max(amax)) if amax else ""
            alive_ok = bool(amin and amax) and min(amin) == n and max(amax) == n
            row["alive_ok"] = "ok" if alive_ok else "FAIL"
            if not alive_ok:
                verdict.append(f"alive {row['alive_min']}..{row['alive_max']} != N")
            row["pass"] = "ok" if not verdict else "FAIL"
            exc_note = "; ".join(f"exception {k}: {why}" for k, why in EXCEPTIONS.get(name, {}).items())
            row["note"] = "; ".join(verdict + ([exc_note] if exc_note else []))
            bad += bool(verdict)
            rows.append(row)

    fields = ["n", "preset", "cond", "runs", "death_rate", "death_rate_min", "death_rate_max", "death_rate_ref", "death_rate_dev_pct", "death_rate_ok",
              "near30_mean", "near30_mean_min", "near30_mean_max", "near30_mean_ref", "near30_mean_dev_pct", "near30_mean_ok",
              "alive_min", "alive_max", "alive_ok", "pass", "note"]
    out = Path(a.out) if a.out else ROOT / "Measurements" / a.batch / "equivalence.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    for r in rows:
        print(f'N={r["n"]:>6} {r["preset"]} {r["cond"]:<16} runs={r["runs"]} death={r["death_rate"]}/s ({r["death_rate_dev_pct"]}%) '
              f'near30={r["near30_mean"]} ({r["near30_mean_dev_pct"]}%) alive={r["alive_min"]}..{r["alive_max"]} {r["pass"]} {r["note"]}')
    print(f"equivalence: {len(rows)} conditions, {len(rows) - bad} pass, {bad} FAIL (tol death {a.tol_death:.0%}, near30 {a.tol_near30:.0%}, alive exact) -> {out}")
    if skipped:
        print(f"note: {skipped} CSVs with unknown condition names skipped")
    return 1 if bad else 0


def by_label_group(doc, name):
    for c in doc["conditions"]:
        if c["name"] == name:
            return c["group"]
    return ""


if __name__ == "__main__":
    sys.exit(main())
