# 죽기 클립 후보 비교 시트와 표를 만든다. 입력은 Scripts/dump-death-clips.py가 쓴 Saved/Lab/death_clips.json.
# 엔진 렌더링 없이 뼈 위치(컴포넌트 공간)를 선으로 그린다: 클립마다 한 줄, 시작·1/3·2/3·끝 프레임 네 칸. 회색 선은 레퍼런스 포즈, 가로선은 바닥(z=0).
# 메시 모양은 보이지 않고 자세(서 있는지 누웠는지)만 보인다. 실행: py -3 Scripts/death-clip-sheet.py  (Pillow 필요)
# 결과: Saved/Screenshots/Lab/death_clips_large_sheet.jpg, death_clips_small_sheet.jpg, 표는 표준 출력(마크다운).
import json
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "Saved", "Lab", "death_clips.json")
OUT = os.path.join(ROOT, "Saved", "Screenshots", "Lab")
DEATH_SEC = None  # 로직의 사망 시간 = 소형 Death_A 길이(LabCrowdSpawner.cpp). 아래에서 JSON으로 채운다
CELL_W, CELL_H, LABEL_W, PAD = 380, 300, 230, 12
YAW = math.radians(35.0)  # 비스듬히 본다(앞뒤와 좌우가 함께 보이게)


def find(names, keys):
    low = [n.lower() for n in names]
    for k in keys:
        for i, n in enumerate(low):
            if n == k:
                return i
    for k in keys:
        for i, n in enumerate(low):
            if k in n:
                return i
    return -1


def project(p, scale, ox, oy):
    x, y, z = p
    sx = x * math.cos(YAW) + y * math.sin(YAW)
    return ox + sx * scale, oy - z * scale


BODY = ["pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "head"] + [
    b + s for s in ("_l", "_r") for b in ("clavicle", "upperarm", "lowerarm", "hand", "thigh", "calf", "foot", "ball")]


def draw_pose(d, pts, parents, names, scale, ox, oy, color, width):
    # 몸통·팔·다리 뼈만 그린다(손가락, 무기, IK, 파편 뼈는 선이 길게 튀어 자세를 가린다)
    for i, par in enumerate(parents):
        if par >= 0 and names[i] in BODY and names[par] in BODY:
            d.line([project(pts[par], scale, ox, oy), project(pts[i], scale, ox, oy)], fill=color, width=width)


def main():
    global DEATH_SEC
    data = json.load(open(DATA, encoding="utf-8"))
    DEATH_SEC = next(c["length"] for c in data["small"]["clips"] if c["name"] == "Death_A")
    os.makedirs(OUT, exist_ok=True)
    try:
        font = ImageFont.truetype("malgun.ttf", 18)
        small_font = ImageFont.truetype("malgun.ttf", 14)
    except OSError:
        font = small_font = ImageFont.load_default()
    for unit in ("large", "small"):
        u = data[unit]
        names, parents, ref = u["bones"], u["parents"], u["ref"]
        pelvis = find(names, ["pelvis", "hips", "hip"])
        head = find(names, ["head"])
        root = 0
        ref_pelvis_z = ref[pelvis][2]
        ref_top = max(p[2] for i, p in enumerate(ref) if names[i] in BODY)
        scale = (CELL_H - 60) / (ref_top * 1.6)  # 공중으로 뜨는 클립이 칸 밖으로 나가지 않게 여유를 둔다
        rows = u["clips"]
        img = Image.new("RGB", (LABEL_W + 4 * CELL_W, 40 + len(rows) * CELL_H), "white")
        d = ImageDraw.Draw(img)
        for k, t in enumerate(["start", "1/3", "2/3", "end"]):
            d.text((LABEL_W + k * CELL_W + PAD, 10), t, fill="black", font=font)
        print("\n### %s (skeleton %s, pelvis bone %s, ref pelvis z %.0f cm, death time %.3f s)\n" % (unit, u["skeleton"].split(".")[-1], names[pelvis], ref_pelvis_z, DEATH_SEC))
        print("| clip | length s | keys | root motion flag | root bone moved cm | pelvis moved (horizontal) cm | end pelvis z cm | end pelvis / ref | end head z cm | end lowest bone z cm | hold at end pose s |")
        print("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
        for r, c in enumerate(rows):
            y0 = 40 + r * CELL_H
            d.line([(0, y0), (img.width, y0)], fill=(200, 200, 200), width=1)
            first, last = c["poses"][0], c["poses"][-1]
            root_move = math.dist(first[root], last[root])
            pel_move = math.hypot(last[pelvis][0] - first[pelvis][0], last[pelvis][1] - first[pelvis][1])
            hold = DEATH_SEC - c["length"]
            label = "%s\n%.2f s, %d keys\nend pelvis %.0f%%\nhold %.2f s" % (c["name"], c["length"], c["keys"], 100.0 * last[pelvis][2] / ref_pelvis_z, hold)
            d.multiline_text((PAD, y0 + PAD), label, fill="black", font=font, spacing=6)
            for k, pts in enumerate(c["poses"]):
                ox = LABEL_W + k * CELL_W + CELL_W / 2
                oy = y0 + CELL_H - 30
                d.line([(LABEL_W + k * CELL_W + 8, oy), (LABEL_W + (k + 1) * CELL_W - 8, oy)], fill=(120, 90, 60), width=2)  # 바닥
                draw_pose(d, ref, parents, names, scale, ox, oy, (205, 205, 205), 2)
                draw_pose(d, pts, parents, names, scale, ox, oy, (30, 30, 30), 4)
                px, py = project(pts[pelvis], scale, ox, oy)
                d.ellipse([px - 5, py - 5, px + 5, py + 5], fill=(200, 40, 40))
                if head >= 0:
                    hx, hy = project(pts[head], scale, ox, oy)
                    d.ellipse([hx - 5, hy - 5, hx + 5, hy + 5], fill=(40, 90, 200))
                d.text((LABEL_W + k * CELL_W + PAD, y0 + PAD), "frame %d" % c["frames"][k], fill=(90, 90, 90), font=small_font)
            print("| %s | %.3f | %d | %s | %.0f | %.0f | %.0f | %.2f | %s | %.0f | %.2f |" % (
                c["name"], c["length"], c["keys"], "on" if c["root_motion"] else "off", root_move, pel_move,
                last[pelvis][2], last[pelvis][2] / ref_pelvis_z, ("%.0f" % last[head][2]) if head >= 0 else "-",
                min(p[2] for i, p in enumerate(last) if names[i] in BODY), hold))
        path = os.path.join(OUT, "death_clips_%s_sheet.jpg" % unit)
        img.save(path, quality=90)
        print("\nsheet: " + path, file=sys.stderr)


main()
