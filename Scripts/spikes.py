"""한 묶음의 주기적 스파이크를 찾는다 (measurement.md "묶음 검사").

각 실행에서 프레임 시간이 그 실행 중앙값의 K배(기본 2)를 넘는 프레임을 스파이크로 보고, 붙어 있는 스파이크 프레임은
하나의 사건으로 묶어 시작 시각(프레임 시간의 누적합, 초)을 모은다. 사건 간격의 중앙값 P와, 간격 중 P의 ±10% 안에 드는 비율을 낸다.
비율이 높고 사건이 충분하면 그 실행은 주기적(periodic)이다. 묶음 판정: 주기적인 실행의 비율, 그 간격 P의 분포.

입력은 라벨 접두사(Saved/Profiling/CSV/<접두사>*.csv) 또는 CSV 경로 목록이다. CSV 메타데이터 행([로 시작)은 건너뛴다.
r0(묶음 첫 실행)은 기본으로 뺀다(--with-r0). 표준 라이브러리만 쓴다.

총괄 묶음(--batch Full1): 본 실행(Full1-*), 트레이스 실행(Full1trace-*, Full1traceser-*)을 따로 센다. 촬영 실행(Full1shot-)은 읽지 않는다.
실패한 실행의 CSV는 하위 폴더(failed/, fg-failed/)에 있어 읽지 않고 Measurements/<묶음>/failures.csv의 라벨도 뺀다.

실행: py -3 -I Scripts/spikes.py M3-            # 접두사
      py -3 -I Scripts/spikes.py --batch Full1  # 총괄 묶음(본 실행과 트레이스를 따로)
      py -3 -I Scripts/spikes.py a.csv b.csv   # 파일 목록
      py -3 -I Scripts/spikes.py M1- --verbose # 실행별 줄도 출력
"""
import argparse
import re
import statistics
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from summarize import CSV_DIR, COLS, read_profile, column, parse_label, failed_labels  # noqa: E402

TOL = 0.10          # 간격이 중앙값의 ±10% 안
MIN_EVENTS = 5      # 주기 판정에 필요한 최소 사건 수
MIN_FRACTION = 0.6  # 주기 판정에 필요한 ±10% 비율


def spike_events(frame_ms, factor):
    med = statistics.median(frame_ms)
    t, times, prev = 0.0, [], False
    for v in frame_ms:
        t += v
        spike = v > factor * med
        if spike and not prev:
            times.append((t - v) / 1000.0)  # 프레임 시작 시각(초)
        prev = spike
    return med, times


def analyze(path, factor):
    header, data, _meta = read_profile(path)
    frame = column(header, data, COLS["frame"])
    if len(frame) < 100:
        return None
    med, times = spike_events(frame, factor)
    gaps = [b - a for a, b in zip(times, times[1:])]
    out = {"label": path.stem, "frames": len(frame), "median_ms": med, "events": len(times), "period": None, "fraction": 0.0, "periodic": False}
    if gaps:
        p = statistics.median(gaps)
        near = sum(1 for g in gaps if abs(g - p) <= TOL * p)
        out.update(period=p, fraction=near / len(gaps))
        out["periodic"] = len(times) >= MIN_EVENTS and out["fraction"] >= MIN_FRACTION
    return out


def report(name, paths, args):
    """경로 목록의 주기 스파이크를 한 묶음으로 보고한다. 읽은 실행이 없으면 False."""
    pre = f"{name}: " if name else ""
    results = []
    for p in paths:
        m = parse_label(p.stem)
        if m and int(m["run"]) == 0 and not args.with_r0:
            continue
        r = analyze(p, args.factor)
        if r:
            results.append(r)
    if not results:
        print(f"{pre}no runs")
        return False
    if args.verbose:
        for r in results:
            per = f'{r["period"]:.2f}s' if r["period"] else "-"
            print(f'{r["label"]:42s} events={r["events"]:3d} period={per:>7s} within10%={r["fraction"]:.0%} {"PERIODIC" if r["periodic"] else ""}')
    per = [r for r in results if r["periodic"]]
    print(f'{pre}runs={len(results)} periodic={len(per)} ({len(per)/len(results):.0%}) factor={args.factor}x events/run median={statistics.median(r["events"] for r in results):.0f}')
    if per:
        ps = sorted(r["period"] for r in per)
        print(f'{pre}periodic runs: period median={statistics.median(ps):.2f}s range={ps[0]:.2f}-{ps[-1]:.2f}s')
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("targets", nargs="*", help="라벨 접두사 또는 CSV 경로")
    ap.add_argument("--batch", help="총괄 묶음 이름(예: Full1). 본 실행과 트레이스 실행을 따로 센다")
    ap.add_argument("--factor", type=float, default=2.0, help="중앙값의 몇 배를 스파이크로 볼지")
    ap.add_argument("--with-r0", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()
    if not args.targets and not args.batch:
        ap.error("targets 또는 --batch가 필요하다")

    ok = False
    if args.batch:
        failed = failed_labels(args.batch)
        main_paths = [p for p in sorted(CSV_DIR.glob(f"{args.batch}-*.csv")) if p.stem not in failed]
        trace_paths = [p for pre in (f"{args.batch}trace-", f"{args.batch}traceser-") for p in sorted(CSV_DIR.glob(f"{pre}*.csv")) if p.stem not in failed]
        ok |= report(f"{args.batch} (measurement runs)", main_paths, args)
        ok |= report(f"{args.batch} (trace runs, not used for values)", trace_paths, args)
    paths = []
    for t in args.targets:
        p = Path(t)
        paths += [p] if p.suffix == ".csv" and p.exists() else sorted(CSV_DIR.glob(f"{t}*.csv"))
    if paths:
        ok |= report("", paths, args)
    if not ok:
        print("no runs")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
