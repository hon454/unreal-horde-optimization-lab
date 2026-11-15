"""v2 W9 ISKM 번쩍임: 연속 캡처(Scripts/run-flash.ps1, f<프레임>.png와 run.log)에서 한 프레임만 튀는 프레임을 센다(Docs/Worklog/V2.md "W9").
실행: py -3 -I Scripts/flash-scan.py <캡처 폴더> [문턱] [y0:y1]
- y0:y1은 판정에 쓰는 화면 행(1280x720 기준)이다. P1 1280x720에서 ISKM 구간(40-200m)은 대략 225:355다. 주지 않으면 화면 전체다.
- 속도 버퍼 캡처(-Cmds "r.BufferVisualizationTarget Velocity,ShowFlag.VisualizeBuffer 1"이 run.log의 명령줄에 있으면): 픽셀 편차 = |R-128| + |G-128|.
  프레임 평균 편차가 앞뒤 프레임 중 큰 값 × 문턱(기본 1.3)을 넘으면 튄 프레임이다. 확대 캡처(-LabDiagFov=30)에서 쓴다. P1 시야에서는 8비트 시각화의 값이 작아 판정력이 없다.
- 색 캡처: 평균 휘도(0-255)가 앞뒤 두 프레임보다 모두 문턱(기본 0.6)을 넘게 높거나 모두 낮으면 튄 프레임이다(Worklog/M5.md "미니언 깜빡임 진단"의 휘도 튐, 행을 고를 수 있다).
- run.log의 lab_flash(-LabFlashDiag)에서 ISKM 컴포넌트마다의 트랙 수를 읽어, 튄 프레임에 트랙 수가 바뀐 컴포넌트가 있었는지 함께 센다
  (트랙 수가 바뀐 프레임에 렌더러가 그 컴포넌트의 뼈대 버퍼를 다시 할당한다, engine-notes.md 42절).
Pillow만 쓴다."""
import os, re, sys
from PIL import Image, ImageChops, ImageStat

SRC = sys.argv[1]
log = open(os.path.join(SRC, "run.log"), encoding="utf-8", errors="replace").read()
velocity = "BufferVisualizationTarget Velocity" in log
THRESH = float(sys.argv[2]) if len(sys.argv) > 2 else (1.3 if velocity else 0.6)
ROWS = tuple(int(v) for v in sys.argv[3].split(":")) if len(sys.argv) > 3 else None

fs = sorted(int(f[1:7]) for f in os.listdir(SRC) if f.startswith("f") and f.endswith(".png"))


def load(f):
    im = Image.open(os.path.join(SRC, "f%06d.png" % f)).convert("RGB")
    if ROWS:
        im = im.crop((0, ROWS[0], im.width, ROWS[1]))
    return im.resize((im.width // 2, im.height // 2), Image.BOX)


def metric_of(f):
    im = load(f)
    if velocity:
        r, g, _ = im.split()
        c = Image.new("L", r.size, 128)
        return ImageStat.Stat(ImageChops.add(ImageChops.difference(r, c), ImageChops.difference(g, c), scale=1.0)).mean[0]
    return ImageStat.Stat(im.convert("L")).mean[0]


metric = {f: metric_of(f) for f in fs}

tracks = {}
for m in re.finditer(r"lab_flash frame=(\d+) ([^\r\n]*)", log):
    d = {}
    for kv in m.group(2).split():
        k, v = kv.split("=", 1)
        if "/" in v:
            d[k] = int(v.split("/")[0])
    tracks[int(m.group(1))] = d


def grew(f):
    a, b = tracks.get(f, {}), tracks.get(f + 1, {})
    return [k for k in b if k in a and a[k] != b[k]]


def is_spike(f):
    if f - 1 not in metric or f + 1 not in metric:
        return False
    if velocity:
        return metric[f] > THRESH * max(metric[f - 1], metric[f + 1])
    # 앞뒤 두 프레임 모두와 같은 쪽으로 문턱보다 벌어져야 한다(한 단계 오르고 머무는 계단은 빼다)
    d1, d2 = metric[f] - metric[f - 1], metric[f] - metric[f + 1]
    return d1 * d2 > 0 and min(abs(d1), abs(d2)) > THRESH


spikes = [f for f in fs if is_spike(f)]
print("mode", "velocity" if velocity else "color", "frames", len(fs), "spikes", len(spikes), "threshold", THRESH, "rows", ROWS or "all")
if tracks:
    print("spikes with a track count change in that frame %d/%d, all frames %d/%d" % (
        sum(1 for f in spikes if grew(f)), len(spikes), sum(1 for f in fs if grew(f)), len(fs)))
    print("track counts at the last frame", tracks[max(tracks)])
for f in spikes:
    print("spike frame=%d metric=%.3f prev=%.3f next=%.3f grew=%s" % (f, metric[f], metric[f - 1], metric[f + 1], ",".join(grew(f))))
