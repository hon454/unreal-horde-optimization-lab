"""참고 묶음 MgrT1 트레이스의 타이머를 프레임당 ms로 모아 Measurements/MgrT1/timers.csv를 만든다(Docs/Worklog/Q8.md "트레이스: MgrT1").

입력: Scripts/trace-export.ps1 -Command TimerStatistics로 내보낸 CSV(%TEMP%\\horde-trace\\MgrT1-<조건>-<N>-<GT|RT|WK>.csv, 창 20~70초).
프레임당 ms = 총 초 x 1000 / 게임 스레드의 FEngineLoop::Tick 횟수. 이름 끝의 _<숫자>(인스턴스 구분자)는 뗀다.
출력 열: cond,n,thread,name,calls_per_frame,incl_ms,excl_ms. 프레임당 Incl이 0.02ms 미만인 타이머는 뺀다.
실행: py -3 -I Scripts/mgr-trace-table.py [--src DIR]
"""
import argparse
import csv
import os
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent.parent
CONDS = ["manager", "mass", "massserial"]
NS = [10000, 20000]
THREADS = ["GT", "RT", "WK"]


def load(path):
    out = {}
    with open(path, newline="", encoding="utf-8", errors="replace") as f:
        for r in csv.DictReader(f):
            name = re.sub(r"_\d{6,}$", "", r["Name"])
            cur = out.setdefault(name, [0, 0.0, 0.0])
            cur[0] += int(float(r["Count"]))
            cur[1] += float(r["Incl"])
            cur[2] += float(r["Excl"])
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default=os.path.expandvars(r"%TEMP%\horde-trace"))
    ap.add_argument("--out", default=str(ROOT / "Measurements" / "MgrT1" / "timers.csv"))
    a = ap.parse_args()
    rows = []
    frames_rows = []
    for c in CONDS:
        for n in NS:
            gt = load(Path(a.src) / f"MgrT1-{c}-{n}-GT.csv")
            frames = gt["FEngineLoop::Tick"][0]
            frames_rows.append((c, n, frames))
            for t in THREADS:
                d = gt if t == "GT" else load(Path(a.src) / f"MgrT1-{c}-{n}-{t}.csv")
                for name, (cnt, incl, excl) in d.items():
                    if incl * 1000 / frames < 0.02:
                        continue
                    rows.append([c, n, t, name, round(cnt / frames, 3), round(incl * 1000 / frames, 4), round(excl * 1000 / frames, 4)])
    Path(a.out).parent.mkdir(parents=True, exist_ok=True)
    with open(a.out, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["cond", "n", "thread", "name", "calls_per_frame", "incl_ms", "excl_ms"])
        w.writerows(rows)
    with open(Path(a.out).with_name("frames.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["cond", "n", "frames"])
        w.writerows(frames_rows)
    print(f"{len(rows)} rows -> {a.out}")


if __name__ == "__main__":
    main()
