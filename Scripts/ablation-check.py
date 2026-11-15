"""측정 실행의 CSV 메타데이터를 조건 표의 기대값과 대조하고 프레임 지표를 한 줄로 보인다(v2 W12).

기대값은 Scripts/full-conditions.json(v2)이 정한다: 공통 기대값 + 구성(단계 s1~s8, 빼 보기 a8* 등)의 기대값(base가 있으면 그 위에 덮어쓴다)
+ 실행값(오브 궤도 LabOrbit, 워밍업+측정 LabWindow, LabN, LabAlive*, LabLabel, 빌드 LabBuild).
라벨이 <묶음>-<V0|V1>-<구성>-N<N>-P1-r<회차>면 조건을 라벨에서 고른다. 트레이스(<묶음>trace-, <묶음>traceser-)와 촬영(<묶음>shot-) 라벨도 같다.
--cond V0-s8로 직접 고를 수도 있다.

실행: py -3 -I Scripts/ablation-check.py <CSV>... [--cond V0-s8] [--shot] [--serial] [--build 903c3385a45c] [--measure-sec 60]
  --shot    촬영 실행(-LabHud -LabShotAt): LabHud 1, LabShotAt > 0을 기대한다(대조 때 워밍업·측정 구간은 보지 않는다)
  --serial  직렬 트레이스(-LabFullyParallel=0): LabOverrides에 fully_parallel=0이 든다
  --build   패키지의 커밋 해시 앞 12자. 주면 LabBuild와 같아야 한다(러너가 Packaged/Test/commit.txt에서 읽어 넘긴다)
  --measure-sec  측정 구간이 기본(180초)이 아닌 실행(트레이스 60초)의 LabWindow를 맞춘다
대조가 하나라도 틀리면 종료 코드 1. 마지막 줄이 아닌 둘째 줄에 `frames=<n> frame_p50=<ms> frame_p95=<ms>`가 있다(러너가 읽는다).
표준 라이브러리만 쓴다.
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import lab_conditions as lc  # noqa: E402
from summarize import column, percentile, read_profile  # noqa: E402


def mean(vals):
    return round(sum(vals) / len(vals), 3) if vals else "?"


def check_labels(doc, cfgs, path):
    labels = [ln.strip() for ln in Path(path).read_text(encoding="utf-8-sig").splitlines() if ln.strip()]
    bad = 0
    for label in labels:
        p = lc.parse_label(label)
        if not p or p["cfg"] not in cfgs or p["speed"] not in doc["scenario"]["speeds"]:
            bad += 1
            print(f"  LABEL FAIL {label}")
            continue
        lc.expected(doc, cfgs, p["cfg"], p["speed"], p["n"], label, "000000000000", p["kind"] == "shot", p["kind"] == "traceser")
    dup = len(labels) - len(set(labels))
    if dup:
        bad += 1
        print(f"  LABEL FAIL {dup} duplicate labels")
    print(f"  labels: {len(labels)} checked, {bad} problems (pattern, config, speed, expectation, duplicates)")
    return 1 if bad else 0


REFERENCE_CSV = Path(__file__).resolve().parent.parent / "Saved" / "Profiling" / "CSV" / "V2W11Sweep-V0-s8-N10000-P1-r1.csv"


def selftest(doc, cfgs):
    bad = 0
    for g, d in doc["groups"].items():
        for c in d["configs"]:
            if c not in cfgs:
                bad += 1
                print(f"  SELFTEST FAIL group {g}: unknown config {c}")
        if d["speed"] not in doc["scenario"]["speeds"]:
            bad += 1
            print(f"  SELFTEST FAIL group {g}: unknown speed {d['speed']}")
    if REFERENCE_CSV.exists():
        _, _, meta = read_profile(REFERENCE_CSV)
        for name in cfgs:
            exp = lc.expected(doc, cfgs, name, "V0", 10000, "x", "000000000000")
            miss = [k.lstrip("~") for k in exp if k.lstrip("~") not in meta]
            if miss:
                bad += 1
                print(f"  SELFTEST FAIL {name}: expected keys not in the reference CSV metadata: {', '.join(miss)}")
        print(f"  selftest: {len(cfgs)} configs, expected keys checked against {REFERENCE_CSV.name}")
    else:
        print(f"  selftest: {len(cfgs)} configs (reference CSV {REFERENCE_CSV.name} not found, metadata keys not checked)")
    return 1 if bad else 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="*")
    ap.add_argument("--labels", help="라벨 목록 파일(한 줄에 하나). 파일의 모든 라벨이 조건 표의 조건이고 기대값을 만들 수 있는지만 본다(러너 -DryRun이 쓴다)")
    ap.add_argument("--selftest", action="store_true", help="조건 표의 모든 구성의 기대 키가 기준 CSV의 메타데이터에 있는지, 묶음 종류의 구성이 모두 있는지 본다")
    ap.add_argument("--cond", help="조건 이름(예: V0-s8). 없으면 라벨에서 고른다")
    ap.add_argument("--shot", action="store_true")
    ap.add_argument("--serial", action="store_true")
    ap.add_argument("--build", default="")
    ap.add_argument("--measure-sec", type=int, default=0)
    a = ap.parse_args()
    doc = lc.load()
    cfgs = lc.configs(doc)
    if a.labels:
        sys.exit(check_labels(doc, cfgs, a.labels))
    if a.selftest:
        sys.exit(selftest(doc, cfgs))
    if not a.csv:
        ap.error("CSV, --labels, --selftest 중 하나가 필요하다")
    bad = 0
    for path in a.csv:
        header, data, meta = read_profile(path)
        label = meta.get("lablabel", Path(path).stem)
        frame = column(header, data, ["FrameTime"])
        game = column(header, data, ["GameThreadTime"])
        gpu = column(header, data, ["GPUTime"])
        overlap = mean(column(header, data, ["Lab/OverlapPairs"]))
        print(f"{label}: stage={meta.get('labstage', '?')} ladder={meta.get('labladder', '?')} orbit={meta.get('laborbit', '?')} window={meta.get('labwindow', '?')} "
              f"overrides={meta.get('laboverrides', '?')} n={meta.get('labn', '?')} types={meta.get('labtypes', '?')} cap={meta.get('labactorcap', '?')} "
              f"hyd={meta.get('labhydratem', '?')} col={meta.get('labactorcollision', '?')} sep={meta.get('labtechseparate', '?')} avoid={meta.get('labtechavoid', '?')} "
              f"hold={meta.get('labrephold', '?')} fsb={meta.get('labfixedskelbounds', '?')} hud={meta.get('labhud', '?')} build={meta.get('labbuild', '?')}")
        print(f"  frames={len(frame)} frame_p50={percentile(frame, 0.5)} frame_p95={percentile(frame, 0.95)} game_p50={percentile(game, 0.5)} "
              f"gpu_p50={percentile(gpu, 0.5)} ok={meta.get('labok', '?')} alive={meta.get('labalive', '?')} death_rate={meta.get('labdeathrate', '?')} "
              f"near30_mean={meta.get('labnear30mean', '?')} overlap_mean={overlap} capsule={meta.get('labcapsuletraces', '?')}/{meta.get('labcapsulehits', '?')}/{meta.get('labcapsuleblocked', '?')}")
        p = lc.parse_label(label)
        cond = a.cond
        if cond:
            speed, _, cfg = cond.partition("-")
            n = meta.get("labn")
        elif p:
            speed, cfg, n = p["speed"], p["cfg"], str(p["n"])
            cond = f"{speed}-{cfg}"
        else:
            bad += 1
            print(f"  CHECK ?: FAIL label '{label}' does not match <묶음>-<V0|V1>-<구성>-N<N>-P1-r<회차>; pass --cond")
            continue
        if cfg not in cfgs or speed not in doc["scenario"]["speeds"]:
            bad += 1
            print(f"  CHECK {cond}: FAIL unknown condition (Scripts/full-conditions.json)")
            continue
        serial = a.serial or (p is not None and p["kind"] == "traceser")
        measure = a.measure_sec or (doc["groups"]["trace"]["measure_sec"] if p and p["kind"] in ("trace", "traceser") else 0)
        exp = lc.expected(doc, cfgs, cfg, speed, n, label if p else None, a.build or None, a.shot or (p is not None and p["kind"] == "shot"), serial, measure or None)
        if exp.get("labhud") == "1" and meta.get("labshotat", "0") in ("0", ""):
            exp["labshotat"] = "> 0"
        if exp.get("labhud") == "1":
            exp.pop("labwindow", None)  # 촬영 실행은 -LabWarmup, -LabDuration을 따로 준다
        diff = []
        for k, v in exp.items():
            if k.startswith("~"):
                got = meta.get(k[1:], "?")
                if v not in got:
                    diff.append(f"{k[1:]}={got} (expected to contain {v})")
            elif meta.get(k) != v:
                diff.append(f"{k}={meta.get(k, '?')} (expected {v})")
        if diff:
            bad += 1
            print(f"  CHECK {cond}: FAIL " + "; ".join(diff))
        else:
            print(f"  CHECK {cond}: ok")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
