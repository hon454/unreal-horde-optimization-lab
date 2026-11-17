# Builds the one-page results site in Site/ (GitHub Pages).
# Run from anywhere: py -3 -I Scripts/build-site.py            (writes Site/data.js and Site/assets/stepN.webp)
#                    py -3 -I Scripts/build-site.py --out <dir> (writes the same files under <dir>, for a check that leaves Site/ alone)
# Writes data.js (window.SITE_DATA) and copies the step clips (Docs/figures/steps/step<N>-*.webp) to assets/stepN.webp.
# Every number on the page comes from these files; the page itself holds no measured value:
#   Measurements/Full4/summary.csv           ladder (all horde sizes, P1) and technique-contribution rows (10000, P1)
#   Measurements/Full4/equivalence.csv       points that do not do the same work (UNMET), marked in by_n
#   Docs/figures/full4/full-n-at-60fps.csv   max N (n_p95), steps 1 and 2 only (W14 decision 5)
#   Measurements/Pool2/timeline.csv          entity pool on/off over a 960 s window (reference batch, one run each, 1-minute bins)
#   Scripts/full-conditions.json             resolution and seed, step roles, condition groups
#   Posts/NN-*/README.md                     titles and the first paragraph of "한 문장으로" of the eight posts (missing posts get a placeholder)
# Public names, row names and "distinguishable" come from Scripts/plot-readme.py and Scripts/plot.py, so the site, the README figures
# and the batch figures say the same thing. The data carries no batch name and no condition label.
# Output is deterministic: running it twice gives the same bytes.
import argparse
import csv
import glob
import importlib
import json
import math
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import plot  # noqa: E402  distinguishable(), full_unmet(), BUDGET_MS
pr = importlib.import_module("plot-readme")  # STEP_TEXT, ABL_TEXT, ABL_MIXED, ROLE_TEXT, row_notes(), fmt()

BATCH = pr.BATCH
POOL_BATCH = pr.POOL_BATCH
PRESET, SPEED = pr.PRESET, pr.SPEED
N0 = pr.N
TARGET_MS = plot.BUDGET_MS
REPO = "https://github.com/hon454/unreal-horde-optimization-lab"
DOC = pr.DOC
CFGS = pr.CFGS

STEPS = [(n, f"{SPEED}-s{n}", pr.STEP_TEXT[n][0], role) for n, role in pr.STEPS]
MAXN_STEPS = (1, 2)  # W14 decision 5: max N only for the steps that do not hold 16.7 ms at 10000
SHADOW_M = 80        # Representation LOD Medium/Low edge; step 8 turns minion shadows off beyond it (measurement.md "최종 구성의 거리 밴드")
FAR_STREAM_M = 500   # map radius: entity-driven particles inside it, off-map records outside it (code-tour.md, ADR-0020)
# The eight posts (V2-w14-plan.md 3.2, W14 decision 1) and the post that covers each step.
POSTS = [(0, "00-goals-and-method"), (1, "01-actor-baseline"), (2, "02-mass-engine-path"), (3, "03-separation"),
         (4, "04-quality-by-distance"), (5, "05-cost-cuts-and-contributions"), (6, "06-engine-findings"), (7, "07-summary")]
STEP_POST = {1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 4, 7: 4, 8: 5}
GROUPS = dict(pr.ROLE_TEXT)  # keys are the condition table's roles: start, earn (cuts cost), quality (adds representation)
METRICS = ["frame_p50_ms", "frame_p95_ms", "game_p50_ms", "render_p50_ms", "gpu_p50_ms"]
BOT_NAME = pr.BOT_NAME
ABL_GROUP_ORDER = ("dist", "same", "sub")

# What the configuration does instead, one sentence per row (glossary 5.2, commands.md "기법별 기여").
ABL_SUB = {
    "a8hyd": "끄면 액터 없이 모든 거리를 ISKM으로 그린다.",
    "a8bud": "끄면 액터 메시가 `USkeletalMeshComponent`이고 매 프레임 애니메이션을 평가한다.",
    "a8fix": "끄면 인스턴스와 액터가 바뀔 때의 전환 대책 다섯을 모두 끈다.",
    "a8hold": "끄면 바뀐 개체가 곧바로 앞의 표현으로 되돌아갈 수 있다.",
    "a8pool": "끄면 죽은 개체의 엔티티를 파괴하고 새로 만든다.",
    "a8sim": "끄면 80m 밖도 로직을 매 프레임 돌리고, 200m 밖도 엔진 이동·조향으로 옮긴다.",
    "a8iskm": "끄면 35~200m를 VAT 인스턴스로 그린다.",
    "a8bnd": "끄면 엔진이 매 프레임 모든 ISKM 인스턴스의 바운드를 합쳐 컴포넌트 바운드를 다시 만든다.",
    "a8far": "끄면 200m 밖 엔티티도 ISKM으로 맵 끝까지 그린다.",
    "a8fx": "끄면 200m 밖에 입자를 그리지 않는다.",
    "a8sep": "끄면 개체끼리 떼어 놓지 않는다.",
    "a8cap": "끄면 액터에 캡슐 콜리전이 없다.",
    "a8shd": "다시 켜면 80m 밖 인스턴스도 그림자를 그린다.",
    "a8fsb": "끄면 액터 메시가 애니메이션이 갱신될 때마다 피직스 에셋 바디로 바운드를 다시 셈한다.",
    "a8res": "끄면 ISKM 트랙을 미리 늘려 두지 않는다.",
    "a8fxf": "끄면 입자가 걸어갈 목표를 시작 때 한 번만 정한다.",
    "a8stp": "끄면 ISKM 바운드 상자가 카메라를 따라 옮겨지지 않고, 대신 궤도를 덮게 커진다.",
    "s8av": "분리 프로세서 대신 엔진 회피 Processor로 겹침을 막는다.",
}
assert set(ABL_SUB) == set(pr.ABL_TEXT)


def changed_text(num, s):
    """What the step changed, in one to three sentences. Settings come from the summary row s(step, column)."""
    hm, cap, far = fmt_int(s(2, "hydrate_m")), fmt_int(s(2, "actor_cap")), fmt_int(s(2, "far_m"))
    if num == 1:
        return ("모든 개체가 `ACharacter` 액터다. 캡슐끼리 막고 `UCharacterMovementComponent`의 스윕 이동으로 서로 겹치지 않으며, "
                "AnimBP로 움직인다. 이후 단계를 견주는 출발점이다.")
    if num == 2:
        return ("이동·피해·사망 로직을 Mass Entity의 Fragment와 Processor로 옮기고, 엔진의 이동·회피·표현 Processor를 그대로 썼다. "
                f"화면 안에서 카메라로부터 {hm}m 안의 개체(상한 {cap})는 액터, 나머지는 애니메이션 없는 정지 메시 인스턴스이고, "
                f"{far}m 밖은 그리지 않는다.")
    if num == 3:
        return ("엔진 회피를 직접 짠 분리 프로세서(`ULabSeparationProcessor`)로 바꾸고, 엔진의 방향 보간 Processor를 "
                "같은 계산을 병렬로 도는 사본으로 바꿨다. 표현은 단계 2와 같다.")
    if num == 4:
        hold = fmt_int(float(s(4, "rep_hold_ms")) / 1000)
        return (f"{hm}~{far}m 개체를 VAT 인스턴스로 그려 걷기·피격·죽기 애니메이션을 붙였다. "
                f"액터 메시를 `USkeletalMeshComponentBudgeted`로 바꾸고 예산을 {fmt_int(s(4, 'budget_ms'))}ms로 두었다. "
                f"인스턴스와 액터가 바뀔 때의 전환 대책 다섯을 켜고, 한 번 바뀐 개체가 {hold}초 안에 바로 전의 표현으로 되돌아가지 않게 붙잡는다.")
    if num == 5:
        med = s(5, "sim_lod").split("/")[0]
        return (f"카메라에서 {med}~{far}m 개체의 로직을 {float(s(5, 'cfg_tick_medium')):g}초 간격으로 돌리고, "
                f"{far}m 밖은 엔진 이동·조향 대신 자체 Processor가 {float(s(5, 'cfg_tick_low')):g}초 간격으로 옮긴다. "
                "죽은 개체의 엔티티는 파괴하지 않고 풀에 두었다가 다시 쓴다.")
    if num == 6:
        return (f"{hm}~{far}m 개체를 VAT 인스턴스에서 ISKM으로 바꾸고, ISKM 컴포넌트에 카메라를 따라 옮기는 고정 바운드를 넣었다. "
                "ISKM 트랙은 미리 늘려 둔다.")
    if num == 7:
        return (f"그리지 않던 {far}m 밖을 Niagara 메시 입자로 채웠다. {far}~{FAR_STREAM_M}m는 실제 엔티티의 자리에, "
                f"{FAR_STREAM_M}m 밖은 맵 밖 무리의 기록 자리에 입자를 그린다.")
    if num == 8:
        return (f"{SHADOW_M}m 밖 인스턴스의 그림자를 끄고, 액터 메시가 피직스 에셋 바디 대신 메시의 고정 바운드를 쓰게 했다"
                "(`bComponentUseFixedSkelBounds`).")
    raise ValueError(num)


def alt_text(num, s):
    """What the clip shows (alt text). The clip is always horde 10000, moving scene, orb camera."""
    hm, far = fmt_int(s(2, "hydrate_m")), fmt_int(s(2, "far_m"))
    body = {
        1: "모든 개체가 AnimBP로 움직이는 액터다.",
        2: f"{hm}m 안은 액터, {hm}~{far}m는 애니메이션 없는 정지 메시 인스턴스이고, {far}m 밖은 빈 땅이다.",
        3: "단계 2와 표현이 같다.",
        4: f"{hm}m 안은 액터, {hm}~{far}m는 VAT 인스턴스이고, {far}m 밖은 빈 땅이다.",
        5: "단계 4와 표현이 같다.",
        6: f"{hm}m 안은 액터, {hm}~{far}m는 ISKM이고, {far}m 밖은 빈 땅이다.",
        7: f"{hm}m 안은 액터, {hm}~{far}m는 ISKM이고, {far}m 밖은 입자 무리가 채운다.",
        8: f"단계 7과 표현이 같고, {SHADOW_M}m 밖 개체는 그림자가 없다.",
    }[num]
    return f"단계 {num} 화면: 호드 총수 1만, 이동 장면, 오브 시점 카메라. {body}"


def fmt(x, signed=False):
    """Three significant figures, as Scripts/plot-readme.py."""
    return pr.fmt(x, signed)


def fmt_int(x):
    """Rounded half up, as Math.round in Site/site.js (Python's round() rounds 5062.5 to 5062)."""
    return f"{int(math.floor(float(x) + 0.5)):,}"


def path(*p):
    return os.path.join(ROOT, *p)


def load_summary():
    rows = {}
    with open(path("Measurements", BATCH, "summary.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if r["preset"] == PRESET:
                rows[r["variant"], int(r["n"])] = r
    return rows


S = load_summary()
UNMET, _ = plot.full_unmet(BATCH)


def sv(c, col, n=N0):
    return float(S[c, n][col])


def diff(a, b, col, n=N0):
    """b minus a, and whether it is distinguishable (plot.py distinguishable(): the change is at least 0.5 ms and larger than both run ranges)."""
    d = sv(b, col, n) - sv(a, col, n)
    return {"d": round(d, 3), "real": plot.distinguishable(S[b, n], S[a, n], col),
            "pct": round(100.0 * d / sv(a, col, n), 1)}


def counts_of(c):
    return sorted(n for (v, n) in S if v == c)


def metric_row(c, n):
    r = S[c, n]
    out = {"bottleneck": r["bottleneck"], "unmet": (c, n) in UNMET}
    for col in METRICS:
        out[col] = float(r[col])
        out[col + "_min"] = float(r[col + "_min"])
        out[col + "_max"] = float(r[col + "_max"])
    out["actors_mean"] = float(r["actors_mean"]) if r["actors_mean"] else None
    return out


def load_max60():
    out = {}
    with open(path("Docs", "figures", BATCH.lower(), "full-n-at-60fps.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            head = r["series"].split(" ", 1)[0]
            if head.isdigit() and int(head) in MAXN_STEPS:  # "1+ ..." is the s1opt sub-experiment, not a step
                assert r["n_p95"], r
                out[int(head)] = {"n": int(r["n_p95"]), "above": False}
    assert sorted(out) == list(MAXN_STEPS), out
    return out


def load_pool():
    keys = {"s8": "on", "a8pool": "off"}
    cols = ["frame_p50", "frame_p95", "gpu_mem_mean", "iskm_tracks_end"]
    data = {"on": [], "off": []}
    with open(path("Measurements", POOL_BATCH, "timeline.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            for cfg, k in keys.items():
                if f"-{SPEED}-{cfg}-N{N0}-{PRESET}-" in r["label"]:
                    data[k].append(r)
    out = {"minutes": [int(r["minute"]) for r in sorted(data["on"], key=lambda r: int(r["minute"]))]}
    for k in data:
        rows = sorted(data[k], key=lambda r: int(r["minute"]))
        assert [int(r["minute"]) for r in rows] == out["minutes"], k
        out[k] = {c: [float(r[c]) for r in rows] for c in cols}
    return out


def strip_md(t):
    t = t.replace("\\~", "~")
    t = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", t)  # links -> text
    t = t.replace("**", "")
    return t.strip()


def load_posts():
    """The eight posts. A post whose README.md is missing or has no "# NN 제목" line yet gets ready = false and empty text."""
    posts = []
    for num, folder in POSTS:
        readme = path("Posts", folder, "README.md")
        title, sentence = "", ""
        if os.path.exists(readme):
            with open(readme, encoding="utf-8") as f:
                lines = f.read().splitlines()
            m = re.match(r"#\s+(\d\d)\s+(.*)", lines[0]) if lines else None
            if m and int(m.group(1)) == num:
                title = strip_md(m.group(2))
                on = False
                for ln in lines[1:]:
                    if ln.startswith("## "):
                        if on:
                            break
                        on = ln.strip() == "## 한 문장으로"
                        continue
                    if on and ln.strip():
                        sentence = strip_md(ln)
                        break
        posts.append({"num": num, "title": title, "sentence": sentence, "ready": bool(title),
                      "url": f"{REPO}/blob/main/Posts/{folder}/README.md"})
    return posts


def load_conditions():
    args = {}
    for a in DOC["common_args"]:
        if "=" in a:
            k, v = a.lstrip("-").split("=", 1)
            args[k] = v
    return args


def ablation():
    """Every technique-contribution row and the sub-experiment, judged on frame p95 against the final configuration (W14 decision 6)."""
    base_c = f"{SPEED}-s8"
    base = {c: sv(base_c, c) for c in METRICS}
    names = DOC["groups"]["ablation"]["configs"] + DOC["groups"]["sub"]["configs"]
    rows = []
    for cfg in names:
        c = f"{SPEED}-{cfg}"
        r = S[c, N0]
        frame = diff(base_c, c, "frame_p95_ms")
        frame.update(lo=round(sv(c, "frame_p95_ms_min") - base["frame_p95_ms"], 3), hi=round(sv(c, "frame_p95_ms_max") - base["frame_p95_ms"], 3))
        group = "sub" if CFGS[cfg]["kind"] == "sub" else "dist" if frame["real"] else "same"
        rows.append({
            "key": cfg, "name": pr.ABL_TEXT[cfg], "sub": ABL_SUB[cfg], "group": group,
            "state": ("slower" if frame["d"] > 0 else "faster") if frame["real"] else "same",
            "frame": frame, "frame50": diff(base_c, c, "frame_p50_ms"),
            "game": diff(base_c, c, "game_p50_ms"), "gpu": diff(base_c, c, "gpu_p50_ms"),
            "bottleneck": r["bottleneck"], "mixed": pr.ABL_MIXED.get(cfg, ""),
            "notes": pr.row_notes(cfg, r, S[base_c, N0]),
        })
    rows.sort(key=lambda r: (ABL_GROUP_ORDER.index(r["group"]), -r["frame"]["d"]))
    return base, rows


def ablation_notes(by):
    """Sentences under the technique-contribution chart. Each claim is asserted against the data, so a new batch cannot keep a stale sentence."""
    far, hyd, iskm, shd, fx, av = (by[k] for k in ("a8far", "a8hyd", "a8iskm", "a8shd", "a8fx", "s8av"))
    abl = [r for r in by.values() if r["group"] != "sub"]
    assert max(abl, key=lambda r: r["frame"]["d"]) is far and far["frame"]["real"] and far["game"]["real"]
    assert hyd["bottleneck"] == "gpu" and hyd["frame"]["real"] and hyd["frame"]["d"] < 0 and hyd["game"]["real"] and hyd["game"]["d"] < 0
    assert iskm["gpu"]["real"] and iskm["gpu"]["d"] > 0 and iskm["frame"]["d"] < 0
    assert fx["gpu"]["real"] and fx["gpu"]["d"] < 0 and not fx["frame"]["real"]
    assert av["frame"]["real"] and av["frame"]["d"] > 0
    out = [
        f"200m 밖을 그리지 않는 일을 끄면 프레임 p95가 {fmt(far['frame']['d'])}ms 길어진다. 200m 밖 엔티티도 ISKM으로 맵 끝까지 그리고, 게임 스레드가 {fmt(far['game']['d'])}ms 는다.",
        f"하이드레이션을 끄면 게임 스레드가 {fmt(-hyd['game']['d'])}ms 줄고 병목이 GPU로 바뀐다. 프레임 p95는 {fmt(-hyd['frame']['d'])}ms 짧아진다.",
        f"ISKM을 VAT 인스턴스로 되돌리면 GPU 시간이 {fmt(iskm['gpu']['d'])}ms 늘고 프레임 p95는 {fmt(-iskm['frame']['d'])}ms 짧아진다. "
        "병목이 게임 스레드라 ISKM의 이점은 프레임 시간이 아니라 GPU 시간으로 남는다.",
        (f"80m 밖 그림자를 다시 켜면 GPU 시간이 {fmt(shd['gpu']['d'])}ms 늘고 프레임 p95는 {fmt(shd['frame']['d'])}ms 는다."
         if shd["frame"]["real"] and shd["gpu"]["real"] else
         f"80m 밖 그림자를 다시 켜도 프레임 p95와 GPU 시간이 최종 구성과 구별되지 않는다(GPU 시간 {fmt(shd['gpu']['d'], True)}ms, 0.5ms 아래)."
         if not shd["frame"]["real"] and not shd["gpu"]["real"] else
         f"80m 밖 그림자를 다시 켜면 GPU 시간이 {fmt(shd['gpu']['d'])}ms 늘지만 프레임 p95는 구별되지 않는다."
         if shd["gpu"]["real"] else
         f"80m 밖 그림자를 다시 켜면 프레임 p95가 {fmt(shd['frame']['d'])}ms 는다."),
        f"배경 무리를 끄면 GPU 시간이 {fmt(-fx['gpu']['d'])}ms 줄지만 프레임 p95는 최종 구성과 구별되지 않는다.",
        f"분리 프로세서 대신 엔진 회피를 쓰면 프레임 p95가 {fmt(av['frame']['d'])}ms 길어진다.",
    ]
    return out


def pool_notes(p):
    on, off = p["on"], p["off"]
    m1 = p["minutes"][-1]
    late = [b - a for a, b in zip(on["frame_p95"][-pr.POOL_LAST:], off["frame_p95"][-pr.POOL_LAST:])]
    rate = (on["iskm_tracks_end"][-1] - on["iskm_tracks_end"][1]) / (p["minutes"][-1] - p["minutes"][1])
    assert off["iskm_tracks_end"][-1] > 2 * on["iskm_tracks_end"][-1] and off["gpu_mem_mean"][-1] > off["gpu_mem_mean"][0]
    assert min(late) > 0
    return [
        f"최종 구성을 엔티티 풀을 켜고 끈 채 {m1}분씩 한 번 돌렸다. 다른 값과 따로 잰 참고 값이다.",
        "엔진은 움직이는 ISKM 인스턴스의 엔티티가 파괴될 때 그 애니메이션 트랙을 돌려받지 않는다"
        "(`MassVisualizationComponent.cpp`). 풀을 끄면 죽은 개체마다 엔티티를 파괴하므로 트랙이 샌다.",
        f"풀을 끄면 {m1}분 동안 ISKM 트랙이 {fmt_int(off['iskm_tracks_end'][0])}에서 {fmt_int(off['iskm_tracks_end'][-1])}로 늘고, "
        f"GPU 메모리가 {fmt_int(off['gpu_mem_mean'][0])}MB에서 {fmt_int(off['gpu_mem_mean'][-1])}MB로 는다.",
        f"마지막 {pr.POOL_LAST}분의 프레임 p95는 풀 끔이 {fmt(min(late))}–{fmt(max(late))}ms 길었다. "
        f"풀을 켜면 2분째부터 트랙이 분당 약 {fmt_int(round(rate, -1))}개만 는다. 실행이 한 번씩이라 범위는 없다.",
    ]


def copy_if_changed(src, dst):
    if os.path.exists(dst) and os.path.getsize(dst) == os.path.getsize(src):
        with open(src, "rb") as a, open(dst, "rb") as b:
            if a.read() == b.read():
                return False
    shutil.copyfile(src, dst)
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", help="folder to write data.js and assets/ into (default Site/). A check run uses a scratch folder and leaves Site/ alone")
    out_dir = ap.parse_args().out or path("Site")
    assets = os.path.join(out_dir, "assets")
    max60 = load_max60()
    posts = load_posts()
    post_by = {p["num"]: p for p in posts}
    args = load_conditions()
    pool = load_pool()
    base, ab_rows = ablation()
    by = {r["key"]: r for r in ab_rows}
    notes = ablation_notes(by)
    for r in ab_rows:
        del r["key"]  # the public data carries no condition label

    os.makedirs(assets, exist_ok=True)
    steps, copied = [], 0
    all_counts = set()
    s = lambda num, col: S[f"{SPEED}-s{num}", N0][col]
    for i, (num, c, name, role) in enumerate(STEPS):
        src = glob.glob(path("Docs", "figures", "steps", f"step{num}-*.webp"))
        assert len(src) == 1, (num, src)
        clip = f"step{num}.webp"
        copied += copy_if_changed(src[0], os.path.join(assets, clip))
        counts = counts_of(c)
        all_counts.update(counts)
        by_n = {str(n): metric_row(c, n) for n in counts}
        delta = {}
        if i > 0:
            prev = STEPS[i - 1][1]
            for n in counts:
                if (prev, n) in S:
                    delta[str(n)] = {k: diff(prev, c, col, n) for k, col in
                                     (("frame", "frame_p50_ms"), ("frame95", "frame_p95_ms"), ("game", "game_p50_ms"), ("gpu", "gpu_p50_ms"))}
                    delta[str(n)]["bottleneck_prev"] = S[prev, n]["bottleneck"]
        p95 = sv(c, "frame_p95_ms")
        post = post_by[STEP_POST[num]]
        steps.append({
            "num": num, "name": name, "group": role,
            "changed": changed_text(num, s),
            "learned": post["sentence"],
            "post": {"title": post["title"], "url": post["url"], "num": post["num"], "ready": post["ready"]},
            "clip": "assets/" + clip, "clip_mb": round(os.path.getsize(src[0]) / 1e6, 1),
            "alt": alt_text(num, s),
            "max60": max60.get(num),  # None for steps 3-8 (decision 5)
            "margin_ms": round(TARGET_MS - p95, 3) if p95 <= TARGET_MS else None,  # "16.7ms까지의 여유" at 10000
            "counts": counts, "by_n": by_n, "delta": delta,
        })

    base_c = f"{SPEED}-s8"
    build = S[base_c, N0]["build"].split(";")[0].split("=")[1][:7]
    l1, l8 = sv(STEPS[0][1], "frame_p50_ms"), sv(base_c, "frame_p50_ms")
    data = {
        "meta": {
            "repo": REPO,
            "release": f"{REPO}/releases/tag/test-package-{build}",
            "pc_specs": f"{REPO}/blob/main/Docs/Reference/pc-specs.md",
            "measurement_guide": f"{REPO}/blob/main/Docs/Guides/measurement.md",
            "summary_csv": f"{REPO}/blob/main/Measurements/{BATCH}/summary.csv",
            "build": build,
            "res": f"{args['ResX']}×{args['ResY']}",
            "seed": args["LabSeed"],
            "runs_per_condition": int(S[base_c, N0]["runs"]),
            "target_n": N0, "target_ms": TARGET_MS,
            "distinct_floor_ms": plot.FULL_DISTINCT_FLOOR_MS,
        },
        "headline": {"first_ms": l1, "final_ms": l8, "first_p95_ms": sv(STEPS[0][1], "frame_p95_ms"),
                     "final_p95_ms": sv(base_c, "frame_p95_ms"), "final_margin_ms": round(TARGET_MS - sv(base_c, "frame_p95_ms"), 3),
                     "first_max60": max60[1]["n"], "first_over_budget": round(l1 / TARGET_MS, 1), "speedup": round(l1 / l8, 1)},
        "groups": GROUPS,
        "counts": sorted(all_counts),
        "steps": steps,
        "ablation": {"base": base, "base_bottleneck": S[base_c, N0]["bottleneck"], "rows": ab_rows, "notes": notes},
        "pool": dict(pool, notes=pool_notes(pool)),
        "posts": posts,
        "bottleneck_names": BOT_NAME,
    }
    text = ("// Generated by Scripts/build-site.py from the final measurement CSVs. Do not edit.\n"
            "window.SITE_DATA = " + json.dumps(data, ensure_ascii=False, indent=1, sort_keys=True) + ";\n")
    out = os.path.join(out_dir, "data.js")
    old = open(out, encoding="utf-8").read() if os.path.exists(out) else None
    if old != text:
        with open(out, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
    missing = [p["num"] for p in posts if not p["ready"]]
    print(f"data.js {'written' if old != text else 'unchanged'} ({len(text.encode('utf-8')):,} bytes) in {out_dir}, "
          f"clips copied {copied}/{len(STEPS)}, headline {fmt(l1)} -> {fmt(l8)} ms (p95 {fmt(data['headline']['final_p95_ms'])}), "
          f"posts not ready: {', '.join(f'{n:02d}' for n in missing) or 'none'}")


if __name__ == "__main__":
    main()
