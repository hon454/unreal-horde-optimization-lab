"""trace-export.ps1 TimerStatistics 결과(GameThread)를 프레임당 ms 표로 만든다. M4 트레이스 분석(Docs/Worklog/M4.md "트레이스 분석").

입력: 내보낸 CSV 여러 개(이름=경로). 열은 Name,Count,C.Avg,Incl(총 초),...,Excl(총 초),... 이다.
프레임당 ms = 총 초 x 1000 / 창 안의 FEngineLoop::Tick 횟수. 프로세서 타이머 이름 끝의 _<숫자>(인스턴스 구분자)는 뗀다.
실행: py -3 -I Scripts/trace-report.py A=a.csv B=b.csv [--top 40] [--sort Incl|Excl] [--grep 정규식]
"""
import argparse
import csv
import re
import sys

sys.dont_write_bytecode = True


def load(path, frames_override=None):
    out = {}
    with open(path, newline="", encoding="utf-8", errors="replace") as f:
        for r in csv.DictReader(f):
            name = re.sub(r"_\d{6,}$", "", r["Name"])
            cur = out.setdefault(name, {"count": 0, "incl": 0.0, "excl": 0.0})
            cur["count"] += int(float(r["Count"]))
            cur["incl"] += float(r["Incl"])
            cur["excl"] += float(r["Excl"])
    frames = frames_override or out.get("FEngineLoop::Tick", {}).get("count", 0)
    return out, frames


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("inputs", nargs="+")
    ap.add_argument("--top", type=int, default=40)
    ap.add_argument("--sort", default="Incl")
    ap.add_argument("--grep", default="")
    ap.add_argument("--frames", default="", help="입력 순서대로 프레임 수(쉼표). 게임 스레드가 아닌 스레드의 표는 FEngineLoop::Tick이 없어 필요하다")
    ap.add_argument("--diff", action="store_true", help="두 입력의 차(둘째 - 첫째)가 큰 순서로 정렬한다")
    args = ap.parse_args()
    data = {}
    fr = [int(x) for x in args.frames.split(",")] if args.frames else []
    for i, a in enumerate(args.inputs):
        k, p = a.split("=", 1)
        data[k] = load(p, fr[i] if i < len(fr) else None)
    key = "incl" if args.sort.lower() == "incl" else "excl"
    first = next(iter(data))
    names = set()
    for d, _ in data.values():
        names |= set(d)
    if args.grep:
        names = {n for n in names if re.search(args.grep, n)}
    rows = sorted(names, key=lambda n: -(data[first][0].get(n, {}).get(key, 0) / max(data[first][1], 1)))
    if args.diff and len(data) >= 2:
        ks = list(data)

        def dv(n):
            a, b = data[ks[0]], data[ks[1]]
            return b[0].get(n, {}).get(key, 0) / max(b[1], 1) - a[0].get(n, {}).get(key, 0) / max(a[1], 1)

        rows = sorted(names, key=lambda n: -abs(dv(n)))
    print("frames:", {k: v[1] for k, v in data.items()})
    print("name | " + " | ".join(f"{k} incl/excl ms, calls/frame" for k in data))
    for n in rows[: args.top]:
        cells = []
        for k, (d, fr) in data.items():
            e = d.get(n)
            cells.append("-" if not e else f"{e['incl']*1000/fr:.3f}/{e['excl']*1000/fr:.3f}, {e['count']/fr:.2f}")
        print(n + " | " + " | ".join(cells))


if __name__ == "__main__":
    main()
