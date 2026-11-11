"""빠른 회전 뒤 가까운 액터가 채워지는 시간(Q4 품질 지표 (나), 개발 빌드 참고).

개발 빌드 로그의 `lab_turn k=.. start_frame=.. end_frame=..`(-LabCamTurn, LabCameraRig.cpp)와 `lab_frame frame=.. near_vat=.. near_actors=..`(-LabFrameLog,
LabGameMode.cpp)를 읽는다. near_vat = 화면 안 카메라 수평 30m에서 아직 인스턴스로 그리는 개체 수, near_actors = 그 안의 보이는 액터 수(Docs/Worklog/M3b.md "E3b 화질 캡처"와 같은 정의).
회전마다(k >= --first-turn):
  peak     = 회전 시작부터 다음 회전 시작 전까지 near_vat 최댓값
  fill     = 회전이 끝난 프레임부터 near_vat가 회전 직전 프레임 값 이하로 처음 돌아오기까지의 프레임 수(못 돌아오면 미달). M3b의 "회복(직전)"
  fill_le  = 회전이 끝난 프레임부터 near_vat <= --threshold가 처음 되기까지의 프레임 수(못 되면 미달)
  min_act  = 같은 구간 near_actors 최솟값
-benchmark -fps=60(고정 1/60초)로 돌린 로그면 프레임 수 ÷ 60이 초다. 표준 라이브러리만 쓴다.

실행: py -3 -I Scripts/turn-fill.py <로그>... [--first-turn 2] [--threshold 10]
"""
import argparse
import re
import statistics

TURN_RE = re.compile(r"lab_turn k=(\d+) start_frame=(\d+) end_frame=(\d+)")
FRAME_RE = re.compile(r"lab_frame frame=(\d+) near_vat=(\d+) near_actors=(\d+)")


def analyze(path, first_turn, threshold):
    turns, frames = [], {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = TURN_RE.search(line)
            if m:
                turns.append(tuple(int(x) for x in m.groups()))
                continue
            m = FRAME_RE.search(line)
            if m:
                frames[int(m[1])] = (int(m[2]), int(m[3]))
    turns = [t for t in turns if t[0] >= first_turn]
    rows = []
    for i, (k, start, end) in enumerate(turns):
        stop = turns[i + 1][1] if i + 1 < len(turns) else max(frames) + 1
        before = frames.get(start - 1)
        span = [f for f in range(start, stop) if f in frames]
        if not before or not span:
            continue
        peak = max(frames[f][0] for f in span)
        min_act = min(frames[f][1] for f in span)
        after = [f for f in range(end, stop) if f in frames]
        fill = next((f - end for f in after if frames[f][0] <= before[0]), None)
        fill_le = next((f - end for f in after if frames[f][0] <= threshold), None)
        rows.append({"k": k, "before": before[0], "peak": peak, "fill": fill, "fill_le": fill_le, "min_act": min_act})
    return rows


def stat(vals):
    ok = [v for v in vals if v is not None]
    miss = len(vals) - len(ok)
    if not ok:
        return f"미달 {miss}/{len(vals)}"
    return f"{statistics.median(ok):.0f} / {max(ok)} / 미달 {miss}/{len(vals)}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("logs", nargs="+")
    ap.add_argument("--first-turn", type=int, default=2)
    ap.add_argument("--threshold", type=int, default=10)
    a = ap.parse_args()
    for path in a.logs:
        rows = analyze(path, a.first_turn, a.threshold)
        print(f"{path}: turns={len(rows)}")
        if not rows:
            continue
        print(f"  fill(직전) 중앙값/최댓값/미달: {stat([r['fill'] for r in rows])}")
        print(f"  fill(<= {a.threshold}) 중앙값/최댓값/미달: {stat([r['fill_le'] for r in rows])}")
        print(f"  near_vat 직전 중앙값 {statistics.median(r['before'] for r in rows):.0f}, peak 중앙값 {statistics.median(r['peak'] for r in rows):.0f} 최댓값 {max(r['peak'] for r in rows)}")
        print(f"  near_actors 최솟값 중앙값 {statistics.median(r['min_act'] for r in rows):.0f} 최솟값 {min(r['min_act'] for r in rows)}")
        print("  turns: " + " ".join(f"k{r['k']}:{r['before']}/{r['peak']}/{r['fill']}/{r['fill_le']}" for r in rows))


if __name__ == "__main__":
    main()
