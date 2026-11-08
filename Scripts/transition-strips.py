"""M3 전환 대책 조합별 60fps 캡처에서 같은 전환 사건 주변을 잘라 조합마다 한 줄씩 붙인다(Measurements/M3/shots/transition/README.md).
실행: py -3 Scripts/transition-strips.py <출력 폴더> <이름>=<캡처 폴더>=<로그> [<이름>=<캡처 폴더>=<로그> ...]
캡처 폴더는 그 실행의 Saved/Screenshots/LabBurst(f<프레임>.png)를 옮긴 것, 로그는 그 실행의 개발 로그(lab_event).
사건은 마지막 조합(대책 전부)의 hydrate에서 화면 가운데 근처를 고른다. 고정 시간 간격과 같은 시드라 조합 사이 장면이 거의 같다.
Pillow만 쓴다."""
import os, re, sys
from PIL import Image, ImageDraw

OUT = sys.argv[1]
RUNS = [a.split("=", 2) for a in sys.argv[2:]]
os.makedirs(OUT, exist_ok=True)


def events(log, kind):
    text = open(log, encoding="utf-8", errors="ignore").read()
    return [(int(f), int(u), int(v)) for k, f, u, v in re.findall(r"lab_event (\w+) frame=(\d+) u=(-?\d+) v=(-?\d+)", text) if k == kind]


def frames(d):
    return sorted(int(f[1:7]) for f in os.listdir(d) if f.startswith("f") and f.endswith(".png"))


def crop_row(name, d, f0, u, v, n=8, half=30, scale=3):
    tiles = []
    for g in range(f0, f0 + n):
        p = os.path.join(d, "f%06d.png" % g)
        if not os.path.exists(p):
            return None
        tiles.append(Image.open(p).convert("RGB").crop((u - half, v - half, u + half, v + half)).resize((2 * half * scale, 2 * half * scale), Image.NEAREST))
    w = tiles[0].width
    row = Image.new("RGB", (160 + n * (w + 4), w), (24, 24, 24))
    ImageDraw.Draw(row).text((8, w // 2 - 6), name, fill=(255, 255, 255))
    for i, t in enumerate(tiles):
        row.paste(t, (160 + i * (w + 4), 0))
    return row


_, ref_dir, ref_log = RUNS[-1]
fs = frames(ref_dir)
lo, hi = fs[0], fs[-1]
cands = [e for e in events(ref_log, "hydrate") if lo + 2 <= e[0] <= hi - 8 and 300 < e[1] < 1000 and 330 < e[2] < 650]
picked = []
for e in cands:
    if all(abs(e[1] - p[1]) > 80 or abs(e[0] - p[0]) > 6 for p in picked):
        picked.append(e)
    if len(picked) == 4:
        break
print("picked", picked)
for k, (f, u, v) in enumerate(picked):
    rows = [r for r in (crop_row(name, d, f - 2, u, v) for name, d, _ in RUNS) if r]
    img = Image.new("RGB", (rows[0].width, len(rows) * (rows[0].height + 4)), (0, 0, 0))
    for i, r in enumerate(rows):
        img.paste(r, (0, i * (r.height + 4)))
    path = os.path.join(OUT, "event%d.png" % (k + 1))
    img.save(path)
    print(path, "frames", f - 2, "..", f + 5, "at", u, v)
