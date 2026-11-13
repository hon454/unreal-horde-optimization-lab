# Builds the one-page results site in Site/ (GitHub Pages).
# Run from anywhere: py -3 -I Scripts/build-site.py
# Writes Site/data.js (window.SITE_DATA) and copies the step clips to Site/assets/stepN.webp.
# Every number on the page comes from these files; the page itself holds no measured value:
#   Measurements/Full2/summary.csv           ladder (all horde sizes, P1) and ablation rows (10000, P1)
#   Docs/figures/full2/full-n-at-60fps.csv   max entities at 60 fps (n_p95)
#   Measurements/Pool1/timeline.csv          entity pool on/off over 16 minutes (one run each, 1-minute bins)
#   Scripts/full-conditions.json             resolution and seed of the measurement
#   Posts/NN-*/README.md                     post titles and the first paragraph of "한 문장으로"
# Settings shown in the text (actor cap, actor distance, far distance) come from summary.csv columns.
# Output is deterministic: running it twice gives the same bytes.
import csv, glob, json, math, os, re, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
SITE = os.path.join(ROOT, "Site")
ASSETS = os.path.join(SITE, "assets")
BATCH = "Full2"
PRESET = "P1"
N0 = 10000
TARGET_MS = 1000.0 / 60.0
REPO = "https://github.com/hon454/unreal-horde-optimization-lab"

# Step number -> condition, short name, group. Names and groups follow Scripts/plot-readme.py.
STEPS = [
    (1, "L1-full", "액터 군중"),
    (2, "L2-mass", "Mass 로직"),
    (3, "L3-vatcull", "ISM + VAT"),
    (4, "L4-mass", "액터 하이드레이션"),
    (5, "L5-m4offpool", "Simulation LOD"),
    (6, "L6-m5bounds", "ISKM"),
    (7, "L7-q2crowd", "Niagara 배경 무리"),
    (8, "L8-q4final", "품질 올리기"),
]
SPEND_FROM = 7
GROUPS = {"earn": "기법을 더해 프레임 시간을 버는 단계", "spend": "번 시간을 품질에 쓰는 단계"}


def changed_text(num, s):
    """What the step changed, in one or two sentences. Settings come from the summary row s(cond)."""
    far = fmt_int(s(STEPS[2][1], "far_m"))
    if num == 1:
        return "모든 개체가 `ACharacter` 액터이고 AnimBP로 움직인다. 이후 단계를 견주는 기준선이다."
    if num == 2:
        return ("이동·피해·사망 로직을 Mass Entity의 Fragment와 Processor로 옮겼다. "
                "표현은 애니메이션 없는 ISM 하나로 바꿨다.")
    if num == 3:
        return (f"인스턴스에 VAT로 걷기·피격·죽기 애니메이션을 붙이고, {far}m 밖 개체는 그리지 않는다.")
    if num == 4:
        hm, cap = fmt_int(s(STEPS[3][1], "hydrate_m")), fmt_int(s(STEPS[3][1], "actor_cap"))
        return (f"오브에서 {hm}m 안의 개체(상한 {cap})를 VAT 인스턴스에서 AnimBP로 움직이는 액터로 바꿨다. "
                "액터 메시는 `USkeletalMeshComponentBudgeted`로 바꾸고 애니메이션 예산을 2ms로 두었다.")
    if num == 5:
        return (f"카메라에서 먼 개체의 로직을 덜 자주 돌리고, {far}m 밖 개체는 엔진 이동·조향 대신 자체 프로세서로 옮겼다. "
                "죽은 개체의 엔티티는 파괴하지 않고 풀에 두었다가 다시 쓴다.")
    if num == 6:
        hm = fmt_int(s(STEPS[3][1], "hydrate_m"))
        return (f"{hm}~{far}m 개체를 VAT 인스턴스에서 ISKM으로 바꾸고, ISKM 컴포넌트의 바운드를 고정했다.")
    if num == 7:
        return (f"그리지 않던 {far}m 밖을 Niagara 메시 입자로 채웠다. 입자는 실제 엔티티의 자리에 그린다. "
                "앞 단계들이 번 시간을 화면의 품질에 쓰는 단계다.")
    if num == 8:
        a, b = STEPS[6][1], STEPS[7][1]
        return (f"액터로 그리는 거리를 {fmt_int(s(a, 'hydrate_m'))}m에서 {fmt_int(s(b, 'hydrate_m'))}m로, "
                f"액터 상한을 {fmt_int(s(a, 'actor_cap'))}에서 {fmt_int(s(b, 'actor_cap'))}으로 올리고, 80m 밖 미니언의 그림자를 껐다. "
                "남은 시간을 가까운 개체의 품질에 쓰는 단계다.")
    raise ValueError(num)


def alt_text(num, s):
    """What the clip shows (alt text), from the posts' image captions. The clip is always horde 10000, orb camera."""
    far = fmt_int(s(STEPS[2][1], "far_m"))
    hm = fmt_int(s(STEPS[3][1], "hydrate_m"))
    hm8 = fmt_int(s(STEPS[7][1], "hydrate_m"))
    body = {
        1: "모든 개체가 AnimBP로 움직이는 액터다.",
        2: "모든 개체가 애니메이션 없는 같은 자세로 움직인다.",
        3: f"오브 가까이의 개체도 VAT 인스턴스이고, {far}m 밖은 빈 땅이다.",
        4: f"{hm}m 안은 액터, {hm}~{far}m는 VAT 인스턴스다.",
        5: f"{hm}m 안은 액터, {hm}~{far}m는 VAT 인스턴스다. 4단계와 표현이 같다.",
        6: f"{hm}m 안은 액터, {hm}~{far}m는 ISKM이고, {far}m 밖은 빈 땅이다.",
        7: f"{hm}m 안은 액터이고, {far}m 밖은 입자 무리가 채운다.",
        8: f"{hm8}m 안은 액터이고, {far}m 밖은 입자 무리가 채운다.",
    }[num]
    return f"{num}단계 화면: 호드 총수 1만, 오브 시점 카메라. {body}"


ABLATIONS = [
    ("A8-hydrate", "가까운 개체를 액터로(하이드레이션)", "빼면 가까운 개체도 ISKM으로 그림"),
    ("A8-budget", "Animation Budget Allocator", "빼면 액터가 예산 없이 매 프레임 애니메이션을 평가"),
    ("A8-simlod", "Simulation LOD", "빼면 80m 밖도 접근·피해 로직을 매 프레임, 200m 밖도 엔진 이동·조향으로 옮김"),
    ("A8-bounds", "ISKM 컴포넌트의 고정 바운드", "빼면 엔진이 매 프레임 바운드를 다시 계산"),
    ("A8-fix", "액터 전환 대책 다섯 가지", "빼면 인스턴스와 액터가 바뀔 때의 보정을 모두 끔"),
    ("A8-pool", "엔티티 풀", "빼면 죽은 개체의 엔티티를 파괴하고 새로 만듦"),
    ("A8-iskm", "40~200m를 ISKM으로", "빼면 ISM + VAT로 그림"),
    ("A8-farshadow", "80m 밖 인스턴스 그림자 끔", "빼면 80m 밖도 그림자를 그림"),
    ("A8-farcull", "200m 밖 엔티티를 입자로", "빼면 입자 대신 ISKM으로 그림"),
    ("A8-crowdfx", "Niagara 배경 무리", "빼면 200m 밖에 아무것도 그리지 않음"),
]
BASE = "L8-q4final"
METRICS = ["frame_p50_ms", "frame_p95_ms", "game_p50_ms", "render_p50_ms", "gpu_p50_ms"]
BOT_NAME = {"game": "게임 스레드", "render": "렌더 스레드", "gpu": "GPU"}


def fmt(x, signed=False):
    """Three significant figures, as Scripts/plot-readme.py."""
    a = abs(x)
    s = f"{a:.0f}" if a >= 100 else f"{a:.1f}" if a >= 10 else f"{a:.2f}"
    if float(s) == 0:
        return s
    if signed:
        return ("+" if x > 0 else "−" if x < 0 else "±") + s
    return ("−" if x < 0 else "") + s


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


def sv(cond, col, n=N0):
    return float(S[cond, n][col])


def srange(cond, col, n=N0):
    return float(S[cond, n][col + "_max"]) - float(S[cond, n][col + "_min"])


def diff(a, b, col, n=N0):
    """b minus a, and whether it is larger than both run ranges (measurement.md "차이 판정")."""
    d = sv(b, col, n) - sv(a, col, n)
    return {"d": round(d, 3), "real": abs(d) > max(srange(a, col, n), srange(b, col, n)),
            "pct": round(100.0 * d / sv(a, col, n), 1)}


def counts_of(cond):
    return sorted(n for (c, n) in S if c == cond)


def metric_row(cond, n):
    r = S[cond, n]
    out = {"bottleneck": r["bottleneck"]}
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
            num = int(r["series"].split(" ", 1)[0])
            out[num] = {"n": int(r["n_p95"]), "above": r["note_p95"] == "above range"}
    return out


def load_pool():
    keys = {"L8-q4final": "on", "A8-pool": "off"}
    cols = ["frame_p50", "gpu_mem_mean", "iskm_tracks_end"]
    data = {"on": [], "off": []}
    with open(path("Measurements", "Pool1", "timeline.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            for cond, k in keys.items():
                if f"-{cond}-N{N0}-{PRESET}-" in r["label"]:
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
    posts = []
    for d in sorted(glob.glob(path("Posts", "[0-9][0-9]-*"))):
        readme = os.path.join(d, "README.md")
        if not os.path.exists(readme):
            continue
        with open(readme, encoding="utf-8") as f:
            lines = f.read().splitlines()
        m = re.match(r"#\s+(\d\d)\s+(.*)", lines[0])
        assert m, readme
        sentence, on = "", False
        for ln in lines[1:]:
            if ln.startswith("## "):
                if on:
                    break
                on = ln.strip() == "## 한 문장으로"
                continue
            if on and ln.strip():
                sentence = ln.strip()
                break
        name = os.path.basename(d)
        posts.append({"num": int(m.group(1)), "title": strip_md(m.group(2)), "sentence": strip_md(sentence),
                      "url": f"{REPO}/blob/main/Posts/{name}/README.md"})
    return posts


def load_conditions():
    with open(path("Scripts", "full-conditions.json"), encoding="utf-8") as f:
        j = json.load(f)
    args = {}
    for a in j["common_args"]:
        if "=" in a:
            k, v = a.lstrip("-").split("=", 1)
            args[k] = v
    return args


def ablation():
    base = {c: sv(BASE, c) for c in METRICS}
    rows = []
    for cond, name, sub in ABLATIONS:
        rows.append({
            "name": name, "sub": sub,
            "frame": diff(BASE, cond, "frame_p50_ms"),
            "game": diff(BASE, cond, "game_p50_ms"),
            "gpu": diff(BASE, cond, "gpu_p50_ms"),
            "bottleneck": S[cond, N0]["bottleneck"],
        })
    rows.sort(key=lambda r: -r["frame"]["d"])
    return base, rows


def ablation_notes(rows_by):
    """Sentences under the ablation chart. The claims are asserted against the data."""
    fc, hy, ik, fs, cf = (rows_by[k] for k in ("A8-farcull", "A8-hydrate", "A8-iskm", "A8-farshadow", "A8-crowdfx"))
    top = max(rows_by.values(), key=lambda r: r["frame"]["d"])
    assert top is fc and fc["frame"]["real"] and fc["game"]["real"]
    assert hy["bottleneck"] == "gpu" and hy["game"]["d"] < 0 and hy["frame"]["d"] < 0 and -hy["frame"]["d"] < -hy["game"]["d"]
    assert ik["gpu"]["real"] and ik["gpu"]["d"] > 0 and ik["game"]["d"] < 0
    assert fs["gpu"]["real"] and fs["gpu"]["d"] > 0 and not fs["frame"]["real"]
    assert cf["frame"]["d"] < 0
    return [
        f"200m 밖을 입자 대신 ISKM으로 그리면 게임 스레드가 {fmt(fc['game']['d'])}ms 늘어 프레임 시간이 가장 크게 는다.",
        f"액터 하이드레이션을 빼면 게임 스레드는 {fmt(-hy['game']['d'])}ms 줄지만 병목이 GPU로 바뀌어 프레임 시간은 {fmt(-hy['frame']['d'])}ms만 준다.",
        f"ISKM을 VAT로 되돌리면 GPU 시간이 {fmt(ik['gpu']['d'])}ms 늘고 게임 스레드는 {fmt(-ik['game']['d'])}ms 준다. "
        "병목이 게임 스레드라 ISKM의 이점은 프레임 시간이 아니라 GPU 시간으로 남는다.",
        f"80m 밖 그림자를 다시 켜면 GPU 시간만 {fmt(fs['gpu']['d'])}ms 늘고 프레임 시간은 구별되지 않는다.",
        f"Niagara 배경 무리를 아예 끄면 프레임 시간이 {fmt(-cf['frame']['d'])}ms 짧아질 뿐이다.",
    ]


def pool_notes(p):
    on, off = p["on"], p["off"]
    m1 = p["minutes"][-1]
    assert off["iskm_tracks_end"][-1] > on["iskm_tracks_end"][-1] and off["frame_p50"][-1] > off["frame_p50"][0]
    return [
        f"최종 구성을 엔티티 풀을 켜고 끈 채 {m1}분씩 한 번 돌렸다. 60초 측정에서는 둘이 구별되지 않았다.",
        "엔진은 움직이는 ISKM 인스턴스의 엔티티가 파괴될 때 그 애니메이션 트랙을 돌려받지 않는다"
        "(`MassVisualizationComponent.cpp`). 풀을 끄면 죽은 개체마다 엔티티를 파괴하므로 트랙이 샌다.",
        f"풀을 끄면 {m1}분 동안 ISKM 트랙이 {fmt_int(off['iskm_tracks_end'][-1])}까지 늘고, "
        f"GPU 메모리가 {fmt_int(off['gpu_mem_mean'][0])}MB에서 {fmt_int(off['gpu_mem_mean'][-1])}MB로, "
        f"프레임 시간 중앙값이 {fmt(off['frame_p50'][0])}ms에서 {fmt(off['frame_p50'][-1])}ms로 는다.",
        f"풀을 켜면 트랙이 {fmt_int(on['iskm_tracks_end'][-1])}에서 멈추고 메모리와 프레임 시간이 평평하다. 실행이 한 번씩이라 범위는 없다.",
    ]


def copy_if_changed(src, dst):
    if os.path.exists(dst) and os.path.getsize(dst) == os.path.getsize(src):
        with open(src, "rb") as a, open(dst, "rb") as b:
            if a.read() == b.read():
                return False
    shutil.copyfile(src, dst)
    return True


def main():
    max60 = load_max60()
    posts = load_posts()
    post_by = {p["num"]: p for p in posts}
    args = load_conditions()
    pool = load_pool()
    base, ab_rows = ablation()
    rows_by = {cond: next(r for r in ab_rows if r["name"] == name) for cond, name, _ in ABLATIONS}

    os.makedirs(ASSETS, exist_ok=True)
    steps, copied = [], 0
    all_counts = set()
    s = lambda cond, col: S[cond, N0][col]
    for i, (num, cond, name) in enumerate(STEPS):
        src = glob.glob(path("Docs", "figures", "steps", f"step{num}-*.webp"))
        assert len(src) == 1, (num, src)
        clip = f"step{num}.webp"
        copied += copy_if_changed(src[0], os.path.join(ASSETS, clip))
        counts = counts_of(cond)
        all_counts.update(counts)
        by_n = {str(n): metric_row(cond, n) for n in counts}
        delta = {}
        if i > 0:
            prev = STEPS[i - 1][1]
            for n in counts:
                if (prev, n) in S:
                    delta[str(n)] = {k: diff(prev, cond, col, n) for k, col in
                                     (("frame", "frame_p50_ms"), ("game", "game_p50_ms"), ("gpu", "gpu_p50_ms"))}
                    delta[str(n)]["bottleneck_prev"] = S[prev, n]["bottleneck"]
        post = post_by[num]
        steps.append({
            "num": num, "name": name,
            "group": "spend" if num >= SPEND_FROM else "earn",
            "changed": changed_text(num, s),
            "learned": post["sentence"],
            "post": {"title": post["title"], "url": post["url"], "num": num},
            "clip": "assets/" + clip, "clip_mb": round(os.path.getsize(src[0]) / 1e6, 1),
            "alt": alt_text(num, s),
            "max60": max60[num], "counts": counts, "by_n": by_n, "delta": delta,
        })

    build = S[BASE, N0]["build"].split(";")[0].split("=")[1][:7]
    l1, l8 = sv(STEPS[0][1], "frame_p50_ms"), sv(BASE, "frame_p50_ms")
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
            "runs_per_condition": int(S[BASE, N0]["runs"]),
            "target_n": N0, "target_ms": round(TARGET_MS, 3),
        },
        "headline": {"first_ms": l1, "final_ms": l8, "first_p95_ms": sv(STEPS[0][1], "frame_p95_ms"),
                     "final_p95_ms": sv(BASE, "frame_p95_ms"), "first_max60": max60[1]["n"],
                     "first_over_budget": round(l1 / TARGET_MS, 1), "speedup": round(l1 / l8, 1)},
        "groups": GROUPS,
        "counts": sorted(all_counts),
        "steps": steps,
        "ablation": {"base": base, "base_bottleneck": S[BASE, N0]["bottleneck"], "rows": ab_rows,
                     "notes": ablation_notes(rows_by)},
        "pool": dict(pool, notes=pool_notes(pool)),
        "posts": posts,
        "bottleneck_names": BOT_NAME,
    }
    text = ("// Generated by Scripts/build-site.py from the final measurement CSVs. Do not edit.\n"
            "window.SITE_DATA = " + json.dumps(data, ensure_ascii=False, indent=1, sort_keys=True) + ";\n")
    out = os.path.join(SITE, "data.js")
    old = open(out, encoding="utf-8").read() if os.path.exists(out) else None
    if old != text:
        with open(out, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
    print(f"data.js {'written' if old != text else 'unchanged'} ({len(text.encode('utf-8')):,} bytes), "
          f"clips copied {copied}/{len(STEPS)}, headline {fmt(l1)} -> {fmt(l8)} ms")


if __name__ == "__main__":
    main()
