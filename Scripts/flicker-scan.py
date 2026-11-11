"""연속 캡처(-LabShotBurst, f<프레임>.png)에서 한두 프레임만 튀는 밝기 변화를 찾는다(Worklog/M5.md "미니언 깜빡임 진단").
실행: py -3 Scripts/flicker-scan.py <캡처 폴더> <출력 csv> [<첫 프레임>]
프레임마다 320x180 휘도로 줄이고, 앞뒤 프레임 평균과의 차(잔차)를 잰다. 카메라는 60초에 한 바퀴라 한 프레임 사이 장면 변화는 작고,
한 프레임만 밝아지거나 어두워지면 잔차가 앞뒤 프레임보다 크게 튄다.
열: frame, luma(화면 평균 휘도 0~255), pos/neg(잔차의 밝아짐/어두워짐 평균), fpos/fneg(|잔차|>T 픽셀 비율),
    top/mid/bot의 fpos·fneg(화면 위·가운데·아래 3등분. 위쪽은 먼 무리, 아래쪽은 가까운 무리와 바닥).
Pillow만 쓴다."""
import os, sys, csv
from PIL import Image, ImageChops, ImageStat

SRC, OUT = sys.argv[1], sys.argv[2]
FIRST = int(sys.argv[3]) if len(sys.argv) > 3 else 0  # 이 프레임 번호부터(앞 실행의 남은 캡처를 뺀다)
W, H, T = 320, 180, 16


def frames(d):
    return sorted(g for g in (int(f[1:7]) for f in os.listdir(d) if f.startswith("f") and f.endswith(".png")) if g >= FIRST)


def load(f):
    return Image.open(os.path.join(SRC, "f%06d.png" % f)).convert("L").resize((W, H), Image.BOX)


def frac(img, box=None):
    m = img.point(lambda x: 255 if x > T else 0)
    if box:
        m = m.crop(box)
    return ImageStat.Stat(m).mean[0] / 255.0


fs = frames(SRC)
bands = {"top": (0, 0, W, H // 3), "mid": (0, H // 3, W, 2 * H // 3), "bot": (0, 2 * H // 3, W, H)}
rows = []
prev, cur = load(fs[0]), load(fs[1])
for i in range(1, len(fs) - 1):
    nxt = load(fs[i + 1])
    # 연속 프레임이 아니면(캡처 누락) 건너뛴다
    if fs[i] - fs[i - 1] == 1 and fs[i + 1] - fs[i] == 1:
        avg = ImageChops.add(prev, nxt, scale=2.0)
        pos = ImageChops.subtract(cur, avg)
        neg = ImageChops.subtract(avg, cur)
        r = {"frame": fs[i], "luma": round(ImageStat.Stat(cur).mean[0], 3),
             "pos": round(ImageStat.Stat(pos).mean[0], 4), "neg": round(ImageStat.Stat(neg).mean[0], 4),
             "fpos": round(frac(pos), 5), "fneg": round(frac(neg), 5)}
        for k, b in bands.items():
            r[k + "_fpos"] = round(frac(pos, b), 5)
            r[k + "_fneg"] = round(frac(neg, b), 5)
        rows.append(r)
    prev, cur = cur, nxt
    if i % 500 == 0:
        print("frame", fs[i], file=sys.stderr)

with open(OUT, "w", newline="") as fh:
    w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()))
    w.writeheader()
    w.writerows(rows)
print("rows", len(rows), "->", OUT)
