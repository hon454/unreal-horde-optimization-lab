"""M3 전환 대책 단계별(L0~L4) 60fps 캡처에서 같은 전환 사건 주변을 잘라 단계마다 한 줄씩 붙인다(Measurements/M3/shots/transition/README.md).
실행: py -3 Scripts/transition-strips.py <작업 폴더> <출력 폴더>
작업 폴더 구조: levels/L<n>/f<프레임>.png(각 단계의 Saved/Screenshots/LabBurst를 옮긴 것), levels/L<n>.log(그 실행의 개발 로그).
Pillow만 쓴다."""
import os, re, sys
from PIL import Image, ImageDraw

S = sys.argv[1]
OUT = sys.argv[2]
os.makedirs(OUT, exist_ok=True)


def events(level, kind):
    log = open(os.path.join(S, "levels", "L%d.log" % level), encoding="utf-8", errors="ignore").read()
    return [(int(f), int(u), int(v)) for k, f, u, v in re.findall(r"lab_event (\w+) frame=(\d+) u=(-?\d+) v=(-?\d+)", log) if k == kind]


def frames(level):
    d = os.path.join(S, "levels", "L%d" % level)
    return d, sorted(int(f[1:7]) for f in os.listdir(d))


def crop_row(level, f0, u, v, n=8, half=30, scale=3):
    d, fs = frames(level)
    tiles = []
    for g in range(f0, f0 + n):
        p = os.path.join(d, "f%06d.png" % g)
        if not os.path.exists(p):
            return None
        tiles.append(Image.open(p).convert("RGB").crop((u - half, v - half, u + half, v + half)).resize((2 * half * scale, 2 * half * scale), Image.NEAREST))
    w = tiles[0].width
    row = Image.new("RGB", (120 + n * (w + 4), w), (24, 24, 24))
    ImageDraw.Draw(row).text((8, w // 2 - 6), "L%d" % level, fill=(255, 255, 255))
    for i, t in enumerate(tiles):
        row.paste(t, (120 + i * (w + 4), 0))
    return row


# L4에서 화면 가운데 근처의 hydrate 사건을 고르고, 모든 단계에서 그 위치·프레임을 같이 자른다(고정 시간 간격이라 장면이 거의 같다).
_, fs4 = frames(4)
lo, hi = fs4[0], fs4[-1]
cands = [e for e in events(4, "hydrate") if lo + 2 <= e[0] <= hi - 8 and 300 < e[1] < 1000 and 330 < e[2] < 650]
picked = []
for e in cands:
    if all(abs(e[1] - p[1]) > 80 or abs(e[0] - p[0]) > 6 for p in picked):
        picked.append(e)
    if len(picked) == 4:
        break
print("picked", picked)
for k, (f, u, v) in enumerate(picked):
    rows = [r for r in (crop_row(L, f - 2, u, v) for L in range(5)) if r]
    img = Image.new("RGB", (rows[0].width, len(rows) * (rows[0].height + 4)), (0, 0, 0))
    for i, r in enumerate(rows):
        img.paste(r, (0, i * (r.height + 4)))
    path = os.path.join(OUT, "event%d.png" % (k + 1))
    img.save(path)
    print(path, "frames", f - 2, "..", f + 5, "at", u, v)
