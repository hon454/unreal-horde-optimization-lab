"""빼 보기(Q3) 실행의 CSV 메타데이터를 기대값과 대조하고 프레임 지표를 한 줄로 보인다.

plan.md "품질 계획" 빼 보기 표의 아홉 행과 최종 구성(final). 기대값: 그 행의 기법만 0이고 나머지 여덟은 1이다(LabTech*, FLabConfig::TechList).
실제 적용값도 본다: LabBudget(할당기가 도는가), LabPool, LabIskmBounds(고정 바운드가 들어간 ISKM 컴포넌트 수, 2종이면 2), LabCrowdFxActive.
기법끼리 딸린 효과는 실제 적용값에서만 0이 된다(iskm 행의 LabIskmBounds 0). 하이드레이션을 끄면 Budget 할당기는 켜진 채 받을 액터가 없다.

실행: py -3 -I Scripts/ablation-check.py <CSV>... [--row final|hydrate|budget|fix|pool|simlod|iskm|bounds|farcull|crowdfx]
--row를 주지 않으면 라벨(LabLabel)에 행 이름이 들어 있으면 그것으로, 없으면 대조 없이 값만 보인다. 대조가 하나라도 틀리면 종료 코드 1.
표준 라이브러리만 쓴다(summarize.py의 CSV 읽기를 그대로 쓴다).
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from summarize import column, percentile, read_profile  # noqa: E402

# 행 이름 -> 끄는 기법의 메타데이터 키(소문자). 명령은 Docs/Guides/commands.md "빼 보기 스위치"
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


def expected(row, types):
    """행의 기대값(메타데이터 키 소문자 -> 문자열)."""
    exp = {k: "1" for k in TECH_KEYS}
    if ROWS[row]:
        exp[ROWS[row]] = "0"
    exp["labbudget"] = "0" if row == "budget" else "1"
    exp["labpool"] = "0" if row == "pool" else "1"
    exp["labiskmbounds"] = "0" if row in ("iskm", "bounds") else str(types)
    exp["labcrowdfxactive"] = "0" if row == "crowdfx" else "1"
    return exp


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="+")
    ap.add_argument("--row", choices=list(ROWS))
    a = ap.parse_args()
    bad = 0
    for path in a.csv:
        header, data, meta = read_profile(path)
        label = meta.get("lablabel", Path(path).stem)
        row = a.row or next((r for r in ROWS if f"-{r}-" in f"-{label}-"), None)
        frame = column(header, data, ["FrameTime"])
        game = column(header, data, ["GameThreadTime"])
        gpu = column(header, data, ["GPUTime"])
        tech = ",".join(f"{k[len('labtech'):]}:{meta.get(k, '?')}" for k in TECH_KEYS)
        print(f"{label}: stage={meta.get('labstage', '?')} variant={meta.get('labvariant', '?')} n={meta.get('labn', '?')} types={meta.get('labtypes', '?')} "
              f"budget={meta.get('labbudget', '?')} pool={meta.get('labpool', '?')} iskm_bounds={meta.get('labiskmbounds', '?')} "
              f"fixmask={meta.get('labhydratefixmask', '?')} crowdfx_active={meta.get('labcrowdfxactive', '?')} tech={tech}")
        print(f"  frames={len(frame)} frame_p50={percentile(frame, 0.5)} frame_p95={percentile(frame, 0.95)} game_p50={percentile(game, 0.5)} "
              f"gpu_p50={percentile(gpu, 0.5)} ok={meta.get('labok', '?')} alive={meta.get('labalive', '?')}")
        if not row:
            continue
        exp = expected(row, meta.get("labtypes", "2"))
        diff = [f"{k}={meta.get(k, '?')} (expected {v})" for k, v in exp.items() if meta.get(k) != v]
        if diff:
            bad += 1
            print(f"  CHECK {row}: FAIL " + "; ".join(diff))
        else:
            print(f"  CHECK {row}: ok")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
