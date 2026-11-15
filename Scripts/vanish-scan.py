"""v2 리뷰 M6: 입자 → ISKM 전환의 사라짐 검사(Docs/Worklog/V2.md "M6").
연속 캡처(Scripts/run-flash.ps1, f<프레임>.png와 run.log)에서 사건 자리(u, v)의 작은 칸이 사건 앞뒤로 비는지 센다.
실행: py -3 -I Scripts/vanish-scan.py <캡처 폴더> [--min-dist 17000] [--event iskm_enter] [--montage 출력.png] [--montage-n 12]

- 사건: run.log의 `lab_event <이름> frame= u= v= dist= wx= wy= x=`(LabVatProcessor.h LabLogScreenEvent). (u, v)는 발밑 + 90cm의 1280x720 화면 좌표,
  dist는 카메라까지 수평 거리(cm)다. iskm_enter는 x < 9(범위 경계가 아닌 진입: Off·액터·풀에서 ISKM으로)만 쓴다.
- 칸: (u, v) 둘레, 가로 ±0.7m, 세로 위 1.2m, 아래 1.0m를 화면 픽셀로 바꾼 크기(초점 거리 = 640 / tan(FOV/2), FOV는 run.log의 -LabDiagFov=, 없으면 90).
- 개체 픽셀: 휘도가 바닥 기준(칸의 세 배 창의 휘도 90번째 백분위수)보다 문턱(기본 28) 넘게 어두운 픽셀. 개체(입자·ISKM)는 밝은 모래 바닥보다 어둡다.
- 정렬 확인: 사건 프레임 f 둘레 k = −2부터 +3까지 칸의 프레임 간 변화(앞 프레임과의 휘도 차 평균)를 모아, 변화가 가장 큰 k를 센다.
  로그의 프레임 번호와 파일 번호가 맞으면 전환 변화는 k = 0 또는 +1에 몰린다.
- 사라짐: f−1(전환 앞)의 개체 픽셀이 m 이상인데 f, f+1, f+2 가운데 개체 픽셀이 m/3 미만인 프레임 수. m = 칸 넓이의 8%(최소 2).
  비는 프레임 수(0부터 3)를 사건마다 세어 분포를 낸다. 가려짐(앞 개체)·겹침(이웃 개체)이 있으면 비지 않은 쪽으로 센다(보수적).
Pillow만 쓴다. Scripts/flash-scan.py는 고치지 않는다."""
import argparse, math, os, re, sys
from PIL import Image, ImageDraw

ap = argparse.ArgumentParser()
ap.add_argument("src")
ap.add_argument("--min-dist", type=float, default=17000.0)
ap.add_argument("--max-dist", type=float, default=1e9)
ap.add_argument("--event", default="iskm_enter")
ap.add_argument("--thresh", type=float, default=28.0)
ap.add_argument("--montage", default="")
ap.add_argument("--montage-n", type=int, default=12)
ap.add_argument("--list", action="store_true", help="사건마다 한 줄(프레임, u, v, dist, f-1..f+3 개체 픽셀)")
a = ap.parse_args()

log = open(os.path.join(a.src, "run.log"), encoding="utf-8", errors="replace").read()
m = re.search(r"-LabDiagFov=([\d.]+)", log)
fov = float(m.group(1)) if m else 90.0
focal = 640.0 / math.tan(math.radians(fov / 2))
files = set(int(f[1:7]) for f in os.listdir(a.src) if f.startswith("f") and f.endswith(".png"))
if not files:
    sys.exit("no frames")
fmin, fmax = min(files), max(files)

pat = re.compile(r"lab_event (\S+) frame=(\d+) u=(-?\d+) v=(-?\d+) dist=(\d+) wx=(-?\d+) wy=(-?\d+) x=(-?[\d.]+)")
events = []
for name, f, u, v, d, wx, wy, x in pat.findall(log):
    if name != a.event:
        continue
    f, u, v, d, x = int(f), int(u), int(v), float(d), float(x)
    if a.event == "iskm_enter" and x >= 9:
        continue
    if not (a.min_dist <= d <= a.max_dist):
        continue
    if not (fmin + 3 <= f <= fmax - 3):
        continue
    ppm = focal / (d / 100.0)
    hw, up, dn = max(2, round(0.7 * ppm)), max(3, round(1.2 * ppm)), max(2, round(1.0 * ppm))
    if u - 3 * hw < 0 or u + 3 * hw >= 1280 or v - 3 * up < 0 or v + 3 * dn >= 720:
        continue
    events.append((f, u, v, d, hw, up, dn))

cache = {}


def gray(f):
    if f not in cache:
        if len(cache) > 64:
            cache.clear()
        cache[f] = Image.open(os.path.join(a.src, "f%06d.png" % f)).convert("L")
    return cache[f]


def patch(f, u, v, hw, up, dn, s=1):
    return list(gray(f).crop((u - s * hw, v - s * up, u + s * hw + 1, v + s * dn + 1)).tobytes())


def objpix(f, e):
    _, u, v, _, hw, up, dn = e
    big = sorted(patch(f, u, v, hw, up, dn, 3))
    ground = big[int(len(big) * 0.9)]
    return sum(1 for p in patch(f, u, v, hw, up, dn) if p < ground - a.thresh)


peak = {k: 0 for k in range(-2, 4)}
dist_empty = [0, 0, 0, 0]
vanished = []
rows = []
for e in sorted(events):
    f, u, v, d, hw, up, dn = e
    area = (2 * hw + 1) * (up + dn + 1)
    mth = max(2, round(area * 0.08))
    cnt = {k: objpix(f + k, e) for k in range(-2, 4)}
    rows.append((e, cnt))
    # 정렬: 앞 프레임과의 칸 휘도 차 평균이 가장 큰 k
    best, bestk = -1.0, 0
    for k in range(-2, 4):
        p0, p1 = patch(f + k - 1, u, v, hw, up, dn), patch(f + k, u, v, hw, up, dn)
        diff = sum(abs(x - y) for x, y in zip(p0, p1)) / len(p0)
        if diff > best:
            best, bestk = diff, k
    peak[bestk] += 1
    if cnt[-1] >= mth:
        empty = sum(1 for k in (0, 1, 2) if cnt[k] < mth / 3)
        dist_empty[empty] += 1
        if empty:
            vanished.append((empty, e, cnt))

print(f"src={a.src} fov={fov:g} event={a.event} dist={a.min_dist:.0f}..{a.max_dist:.0f} frames={fmin}..{fmax} events_in_view={len(events)}")
print("align: frame-to-frame change peak at k (event frame f + k): " + " ".join(f"{k:+d}:{n}" for k, n in peak.items()))
present = sum(dist_empty)
print(f"present_before (f-1 object pixels >= m): {present}")
print("empty frames among f, f+1, f+2: " + " ".join(f"{i}:{n}" for i, n in enumerate(dist_empty)))
if a.list:
    for e, cnt in rows:
        print("ev f=%d u=%d v=%d dist=%.0f  " % e[:4] + " ".join(f"{k:+d}:{cnt[k]}" for k in range(-2, 4)))
for empty, e, cnt in sorted(vanished, reverse=True)[:20]:
    print("vanish empty=%d f=%d u=%d v=%d dist=%.0f  " % ((empty,) + e[:4]) + " ".join(f"{k:+d}:{cnt[k]}" for k in range(-2, 4)))

if a.montage:
    pick = [x[1] for x in sorted(vanished, reverse=True)[: a.montage_n]] or [e for e, _ in rows[: a.montage_n]]
    k8 = 6
    tiles = []
    for e in pick:
        f, u, v, d, hw, up, dn = e
        r = max(hw, up, dn) * 2
        row = []
        for k in range(-1, 3):
            im = Image.open(os.path.join(a.src, "f%06d.png" % (f + k))).convert("RGB").crop((u - r, v - r, u + r, v + r))
            im = im.resize((2 * r * k8 if r * k8 < 120 else 240, 2 * r * k8 if r * k8 < 120 else 240), Image.NEAREST)
            dr = ImageDraw.Draw(im)
            dr.text((3, 2), f"f{f + k}", fill=(255, 255, 0))
            row.append(im)
        tiles.append(row)
    tw = tiles[0][0].width
    W = Image.new("RGB", (4 * (tw + 3), len(tiles) * (tw + 3)), (30, 30, 30))
    for i, row in enumerate(tiles):
        for j, im in enumerate(row):
            W.paste(im.resize((tw, tw)), (j * (tw + 3), i * (tw + 3)))
    W.save(a.montage)
    print("montage:", a.montage)
