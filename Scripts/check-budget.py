"""참고 묶음 Budget1 실행의 CSV 메타데이터를 기대값과 대조한다(Docs/Worklog/Q7.md "측정: Budget1").

기준은 총괄 묶음 조건 L8-q4final(Scripts/full-conditions.json)이고, 예산 값 하나만 다르다.
  예산 2(기본): LabOverrides `-`, LabBudget 1, LabBudgetMs 2
  예산 0(끔):   LabOverrides `budget_ms=0`, LabBudget 0, LabTechAnimBudget 0, LabBudgetMs `-`
  그 밖의 x:    LabOverrides `budget_ms=<x>`, LabBudget 1, LabBudgetMs <x>
LabOk 1, 살아 있는 총수 = N 등 나머지는 ablation-check.py의 L8-q4final 기대값을 그대로 쓴다.
출력 마지막 줄은 run-budget.ps1이 읽는다: `frames=<n> frame_p50=<ms> frame_p95=<ms>`.
실행: py -3 -I Scripts/check-budget.py <CSV> --budget <ms>   (틀리면 종료 코드 1)
"""
import argparse
import importlib.util
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from summarize import column, percentile, read_profile  # noqa: E402

_spec = importlib.util.spec_from_file_location("ablation_check", HERE / "ablation-check.py")
ablation = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(ablation)


def num_eq(a, b):
    try:
        return float(a) == float(b)
    except (TypeError, ValueError):
        return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv")
    ap.add_argument("--budget", type=float, required=True)
    ap.add_argument("--n", help="기대 호드 수(없으면 메타데이터 LabN을 믿는다)")
    ap.add_argument("--label", help="기대 라벨(없으면 대조하지 않는다)")
    a = ap.parse_args()

    doc, conds = ablation.load_conditions()
    header, data, meta = read_profile(a.csv)
    n = a.n or meta.get("labn")
    exp = ablation.expected_cond(doc, conds["L8-q4final"], a.label, n, "P1", False, False)
    b = a.budget
    budget_ms_expect = None  # 숫자 비교
    if b <= 0:
        exp.update({"laboverrides": "budget_ms=0", "labbudget": "0", "labtechanimbudget": "0", "labbudgetms": "-"})
    elif b == 2:
        budget_ms_expect = 2.0
    else:
        exp.update({"laboverrides": f"budget_ms={b:g}"})
        budget_ms_expect = b

    diff = []
    for k, v in exp.items():
        if meta.get(k) != v:
            diff.append(f"{k}={meta.get(k, '?')} (expected {v})")
    if budget_ms_expect is not None:
        if not num_eq(meta.get("labbudgetms"), budget_ms_expect):
            diff.append(f"labbudgetms={meta.get('labbudgetms', '?')} (expected {budget_ms_expect:g})")
    for k in ("labdeaths",):
        if meta.get(k, "") == "":
            diff.append(f"{k} missing")
    # 새 스탯 메타데이터 키가 Test 빌드 CSV에 남아야 한다
    for k in ("lablargeactorinterps", "labsmallactorframes", "labsmallactorskips", "labsmallactorinterps", "labbudgetms"):
        if k not in meta:
            diff.append(f"{k} missing")
    frame = column(header, data, ["FrameTime"])
    print(f"{meta.get('lablabel', Path(a.csv).stem)}: budget_ms={meta.get('labbudgetms', '?')} overrides={meta.get('laboverrides', '?')} "
          f"budget={meta.get('labbudget', '?')} ok={meta.get('labok', '?')} alive={meta.get('labalive', '?')}")
    if diff:
        print("  CHECK budget: FAIL " + "; ".join(diff))
    else:
        print("  CHECK budget: ok")
    print(f"  frames={len(frame)} frame_p50={percentile(frame, 0.5)} frame_p95={percentile(frame, 0.95)}")
    sys.exit(1 if diff else 0)


if __name__ == "__main__":
    main()
