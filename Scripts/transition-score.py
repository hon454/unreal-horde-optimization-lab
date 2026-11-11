"""전환(하이드레이션) 튐 점수. 60fps 연속 캡처(-LabShotBurst, -benchmark -fps=60)와 그 실행의 개발 로그(lab_event hydrate)로,
전환 순간 앞뒤의 화면 변화량이 평소 움직임보다 얼마나 튀었는지를 사건마다 잰다(2026-11-09, 문제 크기를 숫자로 재려는 사용자 요청).

사건 하나(hydrate 프레임 f, 화면 좌표 u,v)마다 60x60 영역에서 연속 프레임 차이의 평균 d(t) = mean|I(t) - I(t-1)|를 구한다.
- 평소 움직임: d(f-4..f-1)의 중앙값
- 튐: max d(f..f+5) - 평소 움직임(밝기 0~255 단위)
- 눈에 띈 전환: 튐이 THRESH 이상(기본 6)이고 평소의 2배 이상
Pillow만 쓴다.

실행: py -3 Scripts/transition-score.py <캡처 폴더> <로그 파일> [--thresh 6] [--event hydrate|dehydrate|both]
--event(Q3): hydrate(기본, 인스턴스 → 액터, LabActorSyncProcessor), dehydrate(액터 → 인스턴스, VAT·ISKM 프로세서가 같은 조건으로 남긴다), both.
ISKM과 VAT를 같은 절차로 비교하려면 같은 시드·고정 간격 캡처를 두 표현으로 찍고 두 사건 모두 잰다(Docs/Worklog/Q3.md "전환 점수").
"""
import argparse
import os
import re
import statistics

from PIL import Image, ImageChops, ImageStat

HALF = 30


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("frames")
    ap.add_argument("log")
    ap.add_argument("--thresh", type=float, default=6.0)
    ap.add_argument("--event", choices=["hydrate", "dehydrate", "both"], default="hydrate")
    a = ap.parse_args()

    files = {int(f[1:7]): os.path.join(a.frames, f) for f in os.listdir(a.frames) if f.startswith("f") and f.endswith(".png")}
    lo, hi = min(files), max(files)
    log = open(a.log, encoding="utf-8", errors="ignore").read()
    names = "hydrate|dehydrate" if a.event == "both" else a.event
    events = [(int(f), int(u), int(v)) for f, u, v in re.findall(r"lab_event (?:%s) frame=(\d+) u=(-?\d+) v=(-?\d+)" % names, log)]
    cache = {}

    def gray(f):
        if f not in cache:
            cache[f] = Image.open(files[f]).convert("L")
        return cache[f]

    W, H = gray(lo).size
    scores = []
    for f, u, v in events:
        if not (lo + 5 <= f <= hi - 6) or not (HALF <= u < W - HALF and HALF <= v < H - HALF):
            continue
        if any((t not in files) for t in range(f - 5, f + 6)):
            continue
        box = (u - HALF, v - HALF, u + HALF, v + HALF)

        def d(t):
            return ImageStat.Stat(ImageChops.difference(gray(t).crop(box), gray(t - 1).crop(box))).mean[0]

        base = statistics.median([d(t) for t in range(f - 4, f)])
        peak = max(d(t) for t in range(f, f + 6))
        scores.append((peak - base, base))
    if not scores:
        print("no events in range")
        return
    excess = [e for e, _ in scores]
    noticed = [1 for e, b in scores if e >= a.thresh and e >= b]
    print(f"events {len(scores)}  mean_excess {statistics.mean(excess):.2f}  median_excess {statistics.median(excess):.2f}  noticeable {len(noticed)}/{len(scores)} ({100.0 * len(noticed) / len(scores):.0f}%)")


if __name__ == "__main__":
    main()
