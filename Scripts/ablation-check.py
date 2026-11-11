"""측정 실행의 CSV 메타데이터를 기대값과 대조하고 프레임 지표와 동등성 통계를 한 줄로 보인다.

두 가지 기대값 출처가 있다.
1. 총괄 묶음 조건(Q5): Scripts/full-conditions.json. 사다리 8단계(L1~L8), 빼 보기 행(A8-*, Q4b부터 farshadow를 더해 열 행), 하위 실험(S1-*, S8-*)의
   인자와 기대 메타데이터. 라벨이 Full<n>-<label_step>-<label_variant>-N<N>-<P1|P2>-r<i>이면 그 조건을 고른다. --cond로 직접 고를 수도 있다.
   기대값 = common_expect + 조건의 expect + 실행값(LabN, LabPreset, LabLabel, LabAlive·LabAliveMin·LabAliveMax = N).
   --shot이면 촬영 실행이라 LabHud 1, LabShotAt > 0을 기대한다. --serial이면 LabOverrides 끝에 fully_parallel=0이 붙는다.
2. Q3·Q4 빼 보기 라벨(옛 방식): plan.md "품질 계획" 빼 보기 표의 아홉 행과 final. 그 행의 기법만 0이고 나머지 여덟은 1이다(LabTech*, FLabConfig::TechList).
   실제 적용값도 본다: LabBudget, LabPool, LabIskmBounds(2종이면 2), LabCrowdFxActive.

실행: py -3 -I Scripts/ablation-check.py <CSV>... [--cond L8-q4final] [--shot] [--serial] [--row final|hydrate|...]
대조가 하나라도 틀리면 종료 코드 1. 표준 라이브러리만 쓴다(summarize.py의 CSV 읽기를 그대로 쓴다).
"""
import argparse
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from summarize import column, percentile, read_profile  # noqa: E402

CONDITIONS = Path(__file__).resolve().parent / "full-conditions.json"
FULL_LABEL = re.compile(r"^Full\d+-(?P<step>[A-Za-z0-9]+)-(?P<variant>[A-Za-z0-9]+)-N(?P<n>\d+)-(?P<preset>P[12])-r\d+$")

# Q3·Q4 행 이름 -> 끄는 기법의 메타데이터 키(소문자). 명령은 Docs/Guides/commands.md "빼 보기 스위치"
ROWS = {
    "final": None,
    "hydrate": "labtechhydrate",
    "budget": "labtechanimbudget",
    "fix": "labtechhydratefix",
    "pool": "labtechpool",
    "simlod": "labtechsimlod",
    "iskm": "labtechiskm",
    "bounds": "labtechfixedbounds",
    "farcull": "labtechfarcull",
    "crowdfx": "labtechcrowdfx",
}
TECH_KEYS = [k for k in ROWS.values() if k]


def expected_row(row, types):
    """Q3·Q4 행의 기대값(메타데이터 키 소문자 -> 문자열)."""
    exp = {k: "1" for k in TECH_KEYS}
    if ROWS[row]:
        exp[ROWS[row]] = "0"
    exp["labbudget"] = "0" if row == "budget" else "1"
    exp["labpool"] = "0" if row == "pool" else "1"
    exp["labiskmbounds"] = "0" if row in ("iskm", "bounds") else str(types)
    exp["labcrowdfxactive"] = "0" if row == "crowdfx" else "1"
    return exp


def load_conditions():
    doc = json.loads(CONDITIONS.read_text(encoding="utf-8"))
    return doc, {c["name"]: c for c in doc["conditions"]}


def expected_cond(doc, cond, label, n, preset, shot, serial):
    """총괄 묶음 조건의 기대값(키 소문자 -> 문자열). n·preset이 없으면 실행값은 대조하지 않는다."""
    exp = {k.lower(): v for k, v in doc["common_expect"].items()}
    exp.update({k.lower(): v for k, v in cond["expect"].items()})
    if n:
        exp.update({"labn": n, "labalive": n, "labalivemin": n, "labalivemax": n})
    if preset:
        exp["labpreset"] = preset
    if label:
        exp["lablabel"] = label
    if serial:
        ov = exp.get("laboverrides", "-")
        exp["laboverrides"] = "fully_parallel=0" if ov == "-" else ov + "|fully_parallel=0"
    if shot:
        exp["labhud"] = "1"
        exp.pop("labshotat", None)
    return exp


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="+")
    ap.add_argument("--row", choices=list(ROWS))
    ap.add_argument("--cond", help="full-conditions.json의 조건 이름(예: L8-q4final). 없으면 라벨에서 고른다")
    ap.add_argument("--shot", action="store_true", help="촬영 실행(-LabHud -LabShotAt)")
    ap.add_argument("--serial", action="store_true", help="직렬 트레이스 실행(-LabFullyParallel=0)")
    a = ap.parse_args()
    doc, conds = load_conditions()
    bad = 0
    for path in a.csv:
        header, data, meta = read_profile(path)
        label = meta.get("lablabel", Path(path).stem)
        frame = column(header, data, ["FrameTime"])
        game = column(header, data, ["GameThreadTime"])
        gpu = column(header, data, ["GPUTime"])
        tech = ",".join(f"{k[len('labtech'):]}:{meta.get(k, '?')}" for k in TECH_KEYS)
        print(f"{label}: stage={meta.get('labstage', '?')} variant={meta.get('labvariant', '?')} ladder={meta.get('labladder', '?')} "
              f"overrides={meta.get('laboverrides', '?')} n={meta.get('labn', '?')} types={meta.get('labtypes', '?')} "
              f"budget={meta.get('labbudget', '?')} pool={meta.get('labpool', '?')} iskm_bounds={meta.get('labiskmbounds', '?')} "
              f"fixmask={meta.get('labhydratefixmask', '?')} crowdfx_active={meta.get('labcrowdfxactive', '?')} "
              f"cap={meta.get('labactorcap', '?')} hyd={meta.get('labhydratem', '?')} col={meta.get('labactorcollision', '?')} "
              f"hud={meta.get('labhud', '?')} tech={tech}")
        print(f"  frames={len(frame)} frame_p50={percentile(frame, 0.5)} frame_p95={percentile(frame, 0.95)} game_p50={percentile(game, 0.5)} "
              f"gpu_p50={percentile(gpu, 0.5)} ok={meta.get('labok', '?')} alive={meta.get('labalive', '?')} "
              f"death_rate={meta.get('labdeathrate', '?')} deaths={meta.get('labdeaths', '?')} near30_mean={meta.get('labnear30mean', '?')} "
              f"alive_min={meta.get('labalivemin', '?')} alive_max={meta.get('labalivemax', '?')}")

        m = FULL_LABEL.match(label)
        cond_name = a.cond or (f"{m['step']}-{m['variant']}" if m else None)
        if cond_name:
            if cond_name not in conds:
                bad += 1
                print(f"  CHECK {cond_name}: FAIL unknown condition (Scripts/full-conditions.json)")
                continue
            # 라벨이 Full 형식이 아니면(--cond) N은 메타데이터의 LabN을 믿고 살아 있는 총수만 그 값과 대조한다
            n = m["n"] if m else meta.get("labn")
            exp = expected_cond(doc, conds[cond_name], label if m else None, n, m["preset"] if m else None, a.shot, a.serial)
            if a.shot and meta.get("labshotat", "0") in ("0", ""):
                exp["labshotat"] = "> 0"
            check = cond_name
        else:
            row = a.row or next((r for r in ROWS if f"-{r}-" in f"-{label}-"), None)
            if not row:
                continue
            exp = expected_row(row, meta.get("labtypes", "2"))
            check = row
        diff = [f"{k}={meta.get(k, '?')} (expected {v})" for k, v in exp.items() if meta.get(k) != v]
        if diff:
            bad += 1
            print(f"  CHECK {check}: FAIL " + "; ".join(diff))
        else:
            print(f"  CHECK {check}: ok")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
