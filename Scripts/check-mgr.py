"""참고 묶음 Mgr1 실행의 CSV 메타데이터를 기대값과 대조한다(Docs/Worklog/Q8.md "측정: Mgr1").

기준은 총괄 묶음 조건 L2-mass(Scripts/full-conditions.json)이고 조건 셋이 있다.
  mass:       L2-mass 그대로(LabStage M1, LabVariant mass, LabOverrides `-`)
  massserial: L2-mass에 LabOverrides `fully_parallel=0`
  manager:    LabStage M1, LabVariant manager, LabLadder `-`. 나머지는 L2-mass와 같다. CSV에 열 Lab/GameThread/Manager가 있어야 한다
LabOk 1, 살아 있는 총수 = N, 라벨 일치 등은 ablation-check.py의 기대값을 쓴다.
출력 마지막 줄은 run-mgr.ps1이 읽는다: `frames=<n> frame_p50=<ms> frame_p95=<ms>`.
실행: py -3 -I Scripts/check-mgr.py <CSV> --cond manager|mass|massserial [--n N] [--label L]   (틀리면 종료 코드 1)
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

MANAGER_COL = "Lab/GameThread/Manager"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv")
    ap.add_argument("--cond", required=True, choices=["manager", "mass", "massserial"])
    ap.add_argument("--n", help="기대 호드 수(없으면 메타데이터 LabN을 믿는다)")
    ap.add_argument("--label", help="기대 라벨(없으면 대조하지 않는다)")
    a = ap.parse_args()

    doc, conds = ablation.load_conditions()
    header, data, meta = read_profile(a.csv)
    n = a.n or meta.get("labn")
    exp = ablation.expected_cond(doc, conds["L2-mass"], a.label, n, "P1", False, a.cond == "massserial")
    if a.cond == "manager":
        exp.update({"labvariant": "manager", "labladder": "-"})

    diff = []
    for k, v in exp.items():
        if meta.get(k) != v:
            diff.append(f"{k}={meta.get(k, '?')} (expected {v})")
    for k in ("labdeaths",):
        if meta.get(k, "") == "":
            diff.append(f"{k} missing")
    mgr = column(header, data, [MANAGER_COL])
    if a.cond == "manager" and not mgr:
        diff.append(f"column {MANAGER_COL} missing or empty")
    if a.cond != "manager" and mgr:
        diff.append(f"column {MANAGER_COL} present in a non-manager run")
    frame = column(header, data, ["FrameTime"])
    print(f"{meta.get('lablabel', Path(a.csv).stem)}: stage={meta.get('labstage', '?')} variant={meta.get('labvariant', '?')} "
          f"overrides={meta.get('laboverrides', '?')} ladder={meta.get('labladder', '?')} ok={meta.get('labok', '?')} "
          f"alive={meta.get('labalive', '?')} manager_p50={percentile(mgr, 0.5) if mgr else '-'}")
    if diff:
        print("  CHECK mgr: FAIL " + "; ".join(diff))
    else:
        print("  CHECK mgr: ok")
    print(f"  frames={len(frame)} frame_p50={percentile(frame, 0.5)} frame_p95={percentile(frame, 0.95)}")
    sys.exit(1 if diff else 0)


if __name__ == "__main__":
    main()
