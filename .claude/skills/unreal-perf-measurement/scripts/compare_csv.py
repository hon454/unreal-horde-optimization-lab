"""Unreal CsvProfiler CSV를 조건별로 묶어 실행 표, 조건 표, 판정 표를 Markdown으로 낸다.

사용법
  python compare_csv.py <조건 폴더> [<조건 폴더> ...] --baseline <이름> [인자]
  python compare_csv.py --cond base="CSV/Base-r[1-3].csv" --cond opt="CSV/Opt-r[1-3].csv" --baseline base
  (Windows에서는 py -3 -I compare_csv.py ...도 된다.)

입력
  - 조건 하나 = 폴더 하나. 폴더 이름이 조건 이름이고, 안의 *.csv 하나가 실행 하나다.
  - --cond NAME=GLOB: 한 폴더에 실행이 섞여 있을 때 글롭으로 조건을 만든다. 여러 번 준다. 폴더 입력과 섞어 쓴다.
  - CSV는 CsvProfiler가 압축 없이 쓴 파일이다(csv.CompressionMode 0). 첫 열이 EVENTS인 행이 헤더이고,
    캡처 중 열이 늘면 마지막 헤더가 전체 열 목록이다. [키],값 쌍의 행은 메타데이터다(키는 소문자로 읽는다).
    둘째 열이 숫자가 아닌 행은 버린다.

지표(실행마다)
  프레임(FrameTime) p50·p95·p99, 50ms 초과 수, p50의 2배 초과 수.
  게임 스레드(GameThreadTime), 렌더 스레드(RenderThreadTime), GPU(GPUTime)와 --extra-col 열의 p50.
  백분위수는 선형 보간이고 소수 셋째 자리로 반올림한다. 병목 = 세 스레드 p50 가운데 가장 긴 쪽.
  조건의 값 = 성공한 실행 값의 중앙값, 범위 = 최댓값 - 최솟값.

판정(지표는 프레임 p95)
  차이 = 조건 중앙값 - 기준 중앙값, 문턱 = max(기준 범위, 조건 범위).
  |차이| > 문턱이고 |차이| >= 0.5ms면 차이 있음. |차이| > 문턱이지만 0.5ms 아래면 두 조건이 모두 5회 이상일 때만
  차이 있음, 아니면 구별되지 않음(5회 필요). 그 밖은 구별되지 않음. 실행이 2회 이하면 실행 부족.
  스레드 시간의 차이는 값만 적고, 병목이 아닌 스레드의 감소에는 * 표시(프레임 이득 아님)를 단다.
  A/A(--aa): 기준과의 프레임 p95 중앙값 차이가 0.5ms 이하면 통과.

표시
  실패: 프레임 수 < --min-frames(100), FrameTime 열 없음, 읽기 오류. 실패한 실행은 요약에서 뺀다.
  느린 실행 후보: 게임 스레드와 렌더 스레드 p50이 그 조건 중앙값보다 함께 --slow-pct(2)% 넘게 길다. 요약에서 빼지 않는다.
  세 스레드보다 긴 프레임: 프레임 p50 > 1.1 x 세 스레드 p50 최댓값. 게임 스레드가 작업자를 기다렸다.

종료 코드: 0 정상(실패한 실행이 있어도), 2 입력 오류(폴더 없음, 기준 조건 없음, 읽을 실행 0).
표준 라이브러리만 쓴다. Python 3.9 이상.
"""
import argparse
import csv
import fnmatch
import glob
import statistics
import sys
from pathlib import Path

THREADS = [("game", "GameThreadTime", "게임 스레드"), ("render", "RenderThreadTime", "렌더 스레드"), ("gpu", "GPUTime", "GPU")]
FRAME_COL = "FrameTime"
LONG_FRAME_RATIO = 1.1
MIN_RUNS = 3
STRICT_RUNS = 5


class InputError(Exception):
    pass


def percentile(values, q):
    if not values:
        return None
    s = sorted(values)
    k = (len(s) - 1) * q
    lo, hi = int(k), min(int(k) + 1, len(s) - 1)
    return round(s[lo] + (s[hi] - s[lo]) * (k - lo), 3)


def read_profile(path):
    """헤더, 프레임 행, 메타데이터를 돌려준다."""
    with open(path, newline="", encoding="utf-8-sig") as f:
        rows = list(csv.reader(f))
    headers = [r for r in rows if r and r[0] == "EVENTS"]
    if not headers:
        raise ValueError("EVENTS 헤더 행이 없다")
    header = headers[-1]
    data, meta = [], {}
    for row in rows:
        if not row or row[0] == "EVENTS":
            continue
        if row[0].startswith("["):
            for i in range(0, len(row) - 1, 2):
                meta[row[i].strip("[]").lower()] = row[i + 1]
            continue
        try:
            float(row[1])
        except (ValueError, IndexError):
            continue
        data.append(row + [""] * (len(header) - len(row)))
    return header, data, meta


def column(header, data, name):
    if name not in header:
        return None
    i = header.index(name)
    out = []
    for r in data:
        try:
            out.append(float(r[i]))
        except ValueError:
            pass
    return out


def skip_rows(header, data, skip_frames, skip_seconds):
    if skip_frames:
        return data[skip_frames:], min(skip_frames, len(data))
    if skip_seconds:
        i_ft = header.index(FRAME_COL)
        target, acc, n = skip_seconds * 1000.0, 0.0, 0
        for r in data:
            if acc >= target:
                break
            try:
                acc += float(r[i_ft])
            except ValueError:
                pass
            n += 1
        return data[n:], n
    return data, 0


def summarize_run(path, args):
    run = {"file": path.name, "path": path, "fail": "", "flags": [], "meta": {}, "missing": []}
    try:
        header, data, meta = read_profile(path)
    except (OSError, ValueError, csv.Error) as e:
        run["fail"] = f"읽기 오류: {e}"
        return run
    run["meta"] = meta
    if FRAME_COL not in header:
        run["fail"] = "FrameTime 열 없음"
        return run
    data, skipped = skip_rows(header, data, args.skip_frames, args.skip_seconds)
    frame = column(header, data, FRAME_COL)
    run["frames"], run["skipped"] = len(frame), skipped
    run["window_s"] = round(sum(frame) / 1000.0, 1)
    if len(frame) < args.min_frames:
        run["fail"] = f"프레임 {len(frame)} < {args.min_frames}"
        return run
    p50 = percentile(frame, 0.5)
    run.update({"frame_p50": p50, "frame_p95": percentile(frame, 0.95), "frame_p99": percentile(frame, 0.99),
                "hitch_abs": sum(1 for v in frame if v > 50.0), "hitch_rel": sum(1 for v in frame if v > 2 * p50)})
    for key, col, _ in THREADS:
        vals = column(header, data, col)
        run[key] = percentile(vals, 0.5) if vals else None
        if vals is None:
            run["missing"].append(col)
    for col in args.extra_col:
        vals = column(header, data, col)
        run["x:" + col] = percentile(vals, 0.5) if vals else None
        if vals is None:
            run["missing"].append(col)
    run["bottleneck"] = bottleneck(run)
    threads = [run[k] for k, _, _ in THREADS if run[k] is not None]
    if threads and p50 > LONG_FRAME_RATIO * max(threads):
        run["flags"].append("세 스레드보다 긴 프레임")
    return run


def bottleneck(values):
    best, name = None, ""
    for key, _, label in THREADS:
        v = values.get(key)
        if v is not None and (best is None or v > best):
            best, name = v, label
    return name


def collect(args, notes):
    """{조건 이름: [csv 경로]}를 입력 순서대로 돌려준다."""
    conds = {}
    for d in args.folders:
        p = Path(d)
        if not p.is_dir():
            raise InputError(f"폴더가 없다: {d}")
        if p.name in conds:
            raise InputError(f"조건 이름이 겹친다: {p.name}")
        conds[p.name] = sorted(p.glob("*.csv"))
        if list(p.glob("*.csv.bin")):
            notes.append(f"{p.name}: .csv.bin(압축) 파일은 읽지 않는다. 캡처 전에 csv.CompressionMode 0을 건다")
    for spec in args.cond:
        if "=" not in spec:
            raise InputError(f"--cond는 NAME=GLOB 꼴이다: {spec}")
        name, pattern = spec.split("=", 1)
        if name in conds:
            raise InputError(f"조건 이름이 겹친다: {name}")
        conds[name] = sorted(Path(f) for f in glob.glob(pattern) if f.endswith(".csv"))
        if any(f.endswith(".csv.bin") for f in glob.glob(pattern + "*")):
            notes.append(f"{name}: .csv.bin(압축) 파일은 읽지 않는다. 캡처 전에 csv.CompressionMode 0을 건다")
    excluded = []
    for name, files in conds.items():
        keep = []
        for f in files:
            if any(fnmatch.fnmatch(f.name, g) or fnmatch.fnmatch(f.as_posix(), g) for g in args.exclude):
                excluded.append(f"{name}/{f.name}")
            else:
                keep.append(f)
        conds[name] = keep
    if excluded:
        notes.append("버린 실행(--exclude): " + ", ".join(excluded))
    return conds


def metric_keys(args):
    return ["frame_p50", "frame_p95", "frame_p99"] + [k for k, _, _ in THREADS] + ["x:" + c for c in args.extra_col]


def summarize_cond(runs, args):
    ok = [r for r in runs if not r["fail"]]
    out = {"n": len(ok)}
    for key in metric_keys(args):
        vals = [r[key] for r in ok if r.get(key) is not None]
        out[key] = (round(statistics.median(vals), 3), min(vals), max(vals)) if vals else None
    out["bottleneck"] = bottleneck({k: (out[k][0] if out[k] else None) for k, _, _ in THREADS})
    # 느린 실행 후보: 게임·렌더 p50이 함께 조건 중앙값보다 slow_pct% 넘게 길다
    if out["game"] and out["render"]:
        lim = 1.0 + args.slow_pct / 100.0
        for r in ok:
            if r["game"] is not None and r["render"] is not None and \
                    r["game"] > out["game"][0] * lim and r["render"] > out["render"][0] * lim:
                r["flags"].append("느린 실행 후보")
    return out


def judge(base, cond, args):
    """프레임 p95 판정. (차이, 문턱, 판정)을 돌려준다."""
    b, c = base.get("frame_p95"), cond.get("frame_p95")
    if not b or not c or base["n"] < MIN_RUNS or cond["n"] < MIN_RUNS:
        diff = round(c[0] - b[0], 3) if b and c else None
        return diff, None, "실행 부족"
    diff = round(c[0] - b[0], 3)
    thr = round(max(b[2] - b[1], c[2] - c[1]), 3)
    word = "빨라짐" if diff < 0 else "느려짐"
    if abs(diff) > thr:
        if abs(diff) >= args.min_diff_ms - 1e-9:
            return diff, thr, f"차이 있음({word})"
        if base["n"] >= STRICT_RUNS and cond["n"] >= STRICT_RUNS:
            return diff, thr, f"차이 있음({word})"
        return diff, thr, f"구별되지 않음({args.min_diff_ms:g}ms 아래, {STRICT_RUNS}회 필요)"
    return diff, thr, "구별되지 않음"


def fmt(v, signed=False):
    if v is None or v == "":
        return ""
    if isinstance(v, int):
        return str(v)
    return f"{v:+.3f}" if signed else f"{v:.3f}"


def fmt_range(t):
    return f"{t[0]:.3f} [{t[1]:.3f}, {t[2]:.3f}]" if t else ""


def table(head, rows):
    lines = ["| " + " | ".join(head) + " |", "| " + " | ".join("---" for _ in head) + " |"]
    lines += ["| " + " | ".join(str(c) for c in row) + " |" for row in rows]
    return "\n".join(lines)


def report(conds, runs_by, summ, args, notes):
    out = []
    extra = args.extra_col
    out.append("## 실행 표\n")
    head = ["조건", "파일", "프레임 수", "버린 프레임", "측정 창(초)", "프레임 p50", "p95", "p99",
            "게임 스레드 p50", "렌더 스레드 p50", "GPU p50"] + [f"{c} p50" for c in extra] + ["병목", "표시"]
    rows = []
    for name in conds:
        for r in runs_by[name]:
            if r["fail"]:
                rows.append([name, r["file"], fmt(r.get("frames")), fmt(r.get("skipped")), fmt(r.get("window_s")),
                             "", "", "", "", "", ""] + [""] * len(extra) + ["", "실패: " + r["fail"]])
                continue
            rows.append([name, r["file"], r["frames"], r["skipped"], r["window_s"], fmt(r["frame_p50"]), fmt(r["frame_p95"]),
                         fmt(r["frame_p99"])] + [fmt(r[k]) for k, _, _ in THREADS] + [fmt(r["x:" + c]) for c in extra]
                        + [r["bottleneck"], ", ".join(r["flags"])])
    out.append(table(head, rows))

    out.append("\n## 조건 표\n")
    out.append("값은 중앙값 [최소, 최대]다.\n")
    head = ["조건", "실행 수", "프레임 p50", "프레임 p95", "프레임 p99", "게임 스레드 p50", "렌더 스레드 p50", "GPU p50"] \
        + [f"{c} p50" for c in extra] + ["병목"]
    rows = [[name, s["n"]] + [fmt_range(s[k]) for k in metric_keys(args)] + [s["bottleneck"]] for name, s in summ.items()]
    out.append(table(head, rows))

    base = summ[args.baseline]
    out.append(f"\n## 판정 표(기준: {args.baseline})\n")
    out.append("차이 = 조건 - 기준(ms). 판정은 프레임 p95로 한다. 스레드 시간은 값만 적는다. "
               "* = 병목이 아닌 스레드의 감소라 프레임 이득이 아니다.\n")
    head = ["조건", "실행 수", "프레임 p95 차이", "문턱", "판정", "프레임 p50 차이", "게임 스레드", "렌더 스레드", "GPU", "병목 변화"]
    rows = []
    for name, s in summ.items():
        if name in (args.baseline, args.aa):
            continue
        diff, thr, verdict = judge(base, s, args)
        p50d = round(s["frame_p50"][0] - base["frame_p50"][0], 3) if s["frame_p50"] and base["frame_p50"] else None
        cells = []
        for key, _, label in THREADS:
            if s[key] and base[key]:
                d = round(s[key][0] - base[key][0], 3)
                mark = "*" if d < 0 and label != s["bottleneck"] else ""
                cells.append(fmt(d, True) + mark)
            else:
                cells.append("")
        change = base["bottleneck"] if base["bottleneck"] == s["bottleneck"] else f"{base['bottleneck']} → {s['bottleneck']}"
        rows.append([name, s["n"], fmt(diff, True), fmt(thr), verdict, fmt(p50d, True)] + cells + [change])
    out.append(table(head, rows) if rows else "견줄 조건이 없다.")

    if args.aa:
        aa = summ[args.aa]
        if aa["frame_p95"] and base["frame_p95"]:
            d = round(aa["frame_p95"][0] - base["frame_p95"][0], 3)
            ok = abs(d) <= args.min_diff_ms + 1e-9
            out.append(f"\n## A/A\n\n{args.aa} - {args.baseline} 프레임 p95 중앙값 차이 {d:+.3f}ms: "
                       + ("통과" if ok else f"{args.min_diff_ms:g}ms 초과. 이 묶음은 참고로 내린다"))
        else:
            out.append(f"\n## A/A\n\n{args.aa}: 성공한 실행이 없어 판정하지 못했다. 이 묶음은 참고로 내린다")

    for name, s in summ.items():
        if s["n"] < MIN_RUNS:
            notes.append(f"{name}: 성공한 실행이 {s['n']}회다({MIN_RUNS}회 미만)")
        missing = sorted({c for r in runs_by[name] for c in r["missing"]})
        if missing:
            notes.append(f"{name}: 없는 열은 빈칸이다: {', '.join(missing)}")
    for key in args.meta_key:
        seen = {}
        for name in conds:
            for r in runs_by[name]:
                if not r["fail"]:
                    seen.setdefault(r["meta"].get(key.lower(), "(없음)"), set()).add(name)
        if len(seen) > 1:
            parts = [f"{v} ({', '.join(sorted(ns))})" for v, ns in seen.items()]
            notes.append(f"메타데이터 {key}가 실행 사이에 다르다: " + "; ".join(parts))
    if notes:
        out.append("\n## 주의\n")
        out += [f"- {n}" for n in notes]
    return "\n".join(out) + "\n"


def parse_args(argv):
    ap = argparse.ArgumentParser(description="CsvProfiler CSV를 조건별로 비교한다. 판정 규칙은 파일 머리 주석에 있다.",
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("folders", nargs="*", help="조건 폴더. 폴더 이름 = 조건 이름, 안의 *.csv 하나 = 실행 하나")
    ap.add_argument("--cond", action="append", default=[], metavar="NAME=GLOB", help="글롭으로 만든 조건(여러 번)")
    ap.add_argument("--baseline", required=True, help="기준 조건 이름")
    ap.add_argument("--aa", help="A/A 조건 이름(기준 조건을 묶음 끝에 다시 잰 것)")
    ap.add_argument("--exclude", action="append", default=[], metavar="GLOB", help="버릴 실행의 파일 이름 글롭(여러 번)")
    ap.add_argument("--skip-seconds", type=float, default=0.0, help="앞에서 FrameTime 누적이 이 초가 될 때까지 버린다")
    ap.add_argument("--skip-frames", type=int, default=0, help="앞 K프레임을 버린다")
    ap.add_argument("--extra-col", action="append", default=[], metavar="NAME", help="p50을 더 볼 열(여러 번)")
    ap.add_argument("--meta-key", action="append", default=[], metavar="KEY", help="실행 사이에 같아야 할 메타데이터 키(여러 번)")
    ap.add_argument("--min-frames", type=int, default=100, help="이보다 프레임이 적은 실행은 실패(기본 100)")
    ap.add_argument("--min-diff-ms", type=float, default=0.5, help="5회 없이 주장할 수 있는 최소 차이(기본 0.5)")
    ap.add_argument("--slow-pct", type=float, default=2.0, help="느린 실행 후보의 문턱 %%(기본 2)")
    ap.add_argument("--out", help="같은 Markdown을 이 파일에도 쓴다")
    args = ap.parse_args(argv)
    if args.skip_seconds and args.skip_frames:
        raise InputError("--skip-seconds와 --skip-frames는 함께 쓰지 않는다")
    if not args.folders and not args.cond:
        raise InputError("조건 폴더나 --cond가 하나 이상 필요하다")
    return args


def main(argv=None):
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except AttributeError:
        pass
    try:
        args = parse_args(argv)
        notes = []
        conds = collect(args, notes)
        for name in [args.baseline] + ([args.aa] if args.aa else []):
            if name not in conds:
                raise InputError(f"조건이 없다: {name} (있는 조건: {', '.join(conds)})")
        runs_by = {name: [summarize_run(f, args) for f in files] for name, files in conds.items()}
        if not any(not r["fail"] for rs in runs_by.values() for r in rs):
            raise InputError("읽을 수 있는 실행이 하나도 없다")
        for name, files in conds.items():
            if not files:
                notes.append(f"{name}: 실행 파일이 없다")
        summ = {name: summarize_cond(runs_by[name], args) for name in conds}
    except InputError as e:
        print(f"입력 오류: {e}", file=sys.stderr)
        return 2
    text = report(conds, runs_by, summ, args, notes)
    sys.stdout.write(text)
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
