"""벤치마크 CSV를 마일스톤 측정 결과로 정리한다.

Saved/Profiling/CSV/<라벨>.csv(run-bench.ps1이 패키지에서 가져온 것)를 읽어
Measurements/<M>/runs.csv에 실행마다 한 행을 더하고, summary.csv를 조건별 중앙값·최소·최대로 다시 쓴다.
라벨 형식은 <M>-<variant>-N<n>-<preset><tag>-r<i>이고, r0(묶음의 첫 실행, 셰이더 캐시)은 summary에서 뺀다.
태그 t2*는 2종(-LabTypes=2) 묶음 2로 summary에 <variant>-t2 조건으로 따로 쓰고, 태그 aa*는 묶음 끝 A/A, ab*는 선행 A/B, aff*/noaff*는 어피니티 진단, start*는 묶음 시작 확인, ab2*는 코드 A/B(D안)으로 summary에 <variant>-aa / -ab 조건으로 따로 쓰고, trace*는 summary에서 뺀다.
열 정의는 Docs/Guides/measurement.md "지표". 표준 라이브러리만 쓴다.

M3b부터: frame_p95_ms(대표 지표), bottleneck(프레임·Game·Render·GPU p50 중 최댓값의 이름), build(CSV 메타데이터)를 더한다.
기존 runs.csv 행은 건드리지 않고 새 라벨만 더한다. 원본 CSV로 모든 행을 다시 계산하려면 --rebuild.
M4부터: Simulation LOD 열을 더한다(없으면 빈칸이라 M3 이하는 그대로다). CSV 스탯 Lab/SimTick<High|Medium|Low>(그 프레임에 접근 로직이 돈 개체 수),
Lab/SimBand<...>(밴드 전체 걷는 개체 수)의 평균과, 메타데이터 LabSimLod(예 `80/200m tick=0/0.50/1.00 avoid=0`)의 설정 간격,
E4 (d) `m4offmove`(LabSimLod 끝 ` offmove=1`)는 Lab/SimTickOff·SimBandOff·OffMoveCount·OffMove 열도 읽고 실제 틱 주기를 Off 열로 셈한다(sim_offmove, cfg_tick_off, sim_tick_off, sim_band_off, real_period_off, off_move_count, off_move_ms). 태그 e4d/e4dstart/e4daa는 E4 (d) 묶음(조건 이름 <변형>-e4d 등).
실제 틱 주기(초) = 평균 프레임 시간(초) x 평균 SimBand / 평균 SimTick을 밴드마다 runs.csv·summary.csv의 맨 끝 열에 쓴다(Worklog/M4.md).
실행: py -3 -I Scripts/summarize.py --milestone M0 [--pc 1] [--rebuild]
"""
import argparse
import csv
import re
import statistics
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CSV_DIR = ROOT / "Saved" / "Profiling" / "CSV"
LABEL_RE = re.compile(r"^(?P<m>M\d+b?)-(?P<variant>[a-z][a-z0-9]*)-N(?P<n>\d+)-(?P<preset>P\d)(?P<tag>[a-z0-9]*)-r(?P<run>\d+)$")

# 엔진 CSV 열(LaunchEngineLoop.cpp:1518-1525)과 후보 이름
COLS = {
    "frame": ["FrameTime"],
    "game": ["GameThreadTime"],
    "render": ["RenderThreadTime"],
    "gpu": ["GPUTime"],
    "draw_calls": ["RHI/DrawCalls", "DrawCalls"],
    "mem": ["PhysicalUsedMB", "Memory/PhysicalUsedMB"],
}
BANDS = ["High", "Medium", "Low"]
# M4 Simulation LOD 열(맨 끝에 붙인다). sim_lod = 메타데이터 LabSimLod, cfg_tick_* = 설정 간격(초), sim_tick_*/sim_band_* = 프레임 평균 개체 수,
# real_period_* = 실제 틱 주기(초)
SIM_FIELDS = (["sim_lod"] + [f"cfg_tick_{b.lower()}" for b in BANDS] + [f"sim_tick_{b.lower()}" for b in BANDS]
              + [f"sim_band_{b.lower()}" for b in BANDS] + [f"real_period_{b.lower()}" for b in BANDS])
# E4 (d) 변형 m4offmove: 200m 밖이 Simulation LOD의 Off 밴드다. 이 변형에만 있는 열(없으면 빈칸). cfg_tick_off는 메타데이터의 Low 간격(코드가 Off 간격을 Low와 같게 둔다)
SIM_FIELDS += ["sim_offmove", "cfg_tick_off", "sim_tick_off", "sim_band_off", "real_period_off", "off_move_count", "off_move_ms"]
RUN_FIELDS = ["label", "milestone", "variant", "n", "preset", "seed", "run", "pc", "frames",
              "game_p50_ms", "game_p99_ms", "render_p50_ms", "gpu_p50_ms", "frame_p50_ms", "frame_p95_ms", "frame_p99_ms",
              "hitch_abs", "hitch_rel", "draw_calls_p50", "mem_total_mb", "bottleneck", "build",
              "types", "types_assets", "large_actor_frames", "large_actor_skips", "large_skip_ratio", "final", "fg_ok"] + SIM_FIELDS
CORE = ["game_p50_ms", "game_p99_ms", "render_p50_ms", "gpu_p50_ms", "frame_p50_ms", "frame_p95_ms", "frame_p99_ms",
        "hitch_abs", "hitch_rel", "draw_calls_p50", "mem_total_mb"]
# 병목 후보: p50 열 이름 -> 표시 이름 (measurement.md "N@16.7ms": 프레임·Game·Render·GPU p50 중 최댓값)
BOTTLENECK_COLS = [("game_p50_ms", "game"), ("render_p50_ms", "render"), ("gpu_p50_ms", "gpu")]
# 게임이 CSV_METADATA로 쓰는 빌드 키(소문자). 키 이름은 Docs/Worklog/M3b.md. 없으면 build는 빈칸.
BUILD_KEYS = ["labbuild", "labcvars", "labdriver"]


def percentile(values, q):
    if not values:
        return ""
    s = sorted(values)
    k = (len(s) - 1) * q
    lo, hi = int(k), min(int(k) + 1, len(s) - 1)
    return round(s[lo] + (s[hi] - s[lo]) * (k - lo), 3)


def bottleneck(row):
    best, name = None, ""
    for col, label in BOTTLENECK_COLS:
        v = row.get(col, "")
        if v == "" or v is None:
            continue
        if best is None or float(v) > best:
            best, name = float(v), label
    return name


def types_fields(meta):
    """2종(M3b) 메타데이터: LabTypes, LabTypesAssets(ok/substituted/-), LabLargeActorFrames/Skips. 없으면 빈칸(옛 CSV)."""
    fr, sk = meta.get("lablargeactorframes", ""), meta.get("lablargeactorskips", "")
    try:
        ratio = round(int(sk) / int(fr), 4) if int(fr) > 0 else ""
    except ValueError:
        ratio = ""
    return {"types": meta.get("labtypes", ""), "types_assets": meta.get("labtypesassets", ""),
            "large_actor_frames": fr, "large_actor_skips": sk, "large_skip_ratio": ratio}


def sim_lod_fields(header, data, meta, frame):
    """M4 Simulation LOD 열. 스탯 열이나 LabSimLod가 없으면(M3 이하) 모두 빈칸. LabSimLod가 `-`여도 빈칸."""
    out = {k: "" for k in SIM_FIELDS}
    lod = meta.get("labsimlod", "")
    if lod and lod != "-":
        out["sim_lod"] = lod
        m = re.search(r"tick=([0-9.]+)/([0-9.]+)/([0-9.]+)", lod)
        if m:
            for b, v in zip(BANDS, m.groups()):
                out[f"cfg_tick_{b.lower()}"] = float(v)
    frame_s = statistics.fmean(frame) / 1000.0 if frame else None
    if lod and " offmove=1" in lod:
        out["sim_offmove"] = 1
        if out["cfg_tick_low"] != "":
            out["cfg_tick_off"] = out["cfg_tick_low"]
        tick_off = column(header, data, ["Lab/SimTickOff"])
        band_off = column(header, data, ["Lab/SimBandOff"])
        cnt = column(header, data, ["Lab/OffMoveCount"])
        ms = column(header, data, ["Lab/OffMove"])
        if tick_off:
            out["sim_tick_off"] = round(statistics.fmean(tick_off), 2)
        if band_off:
            out["sim_band_off"] = round(statistics.fmean(band_off), 2)
        if tick_off and band_off and frame_s and statistics.fmean(tick_off) > 0:
            out["real_period_off"] = round(frame_s * statistics.fmean(band_off) / statistics.fmean(tick_off), 4)
        if cnt:
            out["off_move_count"] = round(statistics.fmean(cnt), 2)
        if ms:
            out["off_move_ms"] = round(statistics.fmean(ms), 4)
    for b in BANDS:
        tick = column(header, data, [f"Lab/SimTick{b}"])
        band = column(header, data, [f"Lab/SimBand{b}"])
        mt = statistics.fmean(tick) if tick else None
        mb = statistics.fmean(band) if band else None
        if mt is not None:
            out[f"sim_tick_{b.lower()}"] = round(mt, 2)
        if mb is not None:
            out[f"sim_band_{b.lower()}"] = round(mb, 2)
        if frame_s and mt and mb:
            out[f"real_period_{b.lower()}"] = round(frame_s * mb / mt, 4)
    return out


def fg_ok(label):
    """패키지 Saved/Lab/<라벨>.summary.txt 끝의 `fg=30/30`(측정 구간 포그라운드가 게임인 표본 수 / 전체 표본 수). 없으면 빈칸(옛 실행)."""
    p = ROOT / "Packaged" / "Test" / "Windows" / "HordeOptLab" / "Saved" / "Lab" / f"{label}.summary.txt"
    try:
        m = re.search(r"fg=(\d+/\d+)", p.read_text(encoding="utf-8", errors="replace"))
    except OSError:
        return ""
    return m.group(1) if m else ""


def build_string(meta):
    parts = [f"{k}={meta[k]}" for k in BUILD_KEYS if meta.get(k, "") != ""]
    return ";".join(parts)


def read_profile(path):
    """헤더, 프레임 행, 메타데이터를 돌려준다.

    엔진 CSV는 첫 행이 헤더(첫 열 EVENTS), 이어 프레임 행, 끝에 헤더 반복과 [키],값 쌍의 메타데이터 행이 온다.
    메타데이터 키는 소문자다(예: [labn],500).
    """
    with open(path, newline="", encoding="utf-8-sig") as f:
        rows = list(csv.reader(f))
    # 캡처 중에 새 스탯이 생기면 뒤쪽 행일수록 열이 늘어난다. 끝의 헤더([HasHeaderRowAtEnd])가 전체 열 목록이다.
    header = [r for r in rows if r and r[0] == "EVENTS"][-1]
    data, meta = [], {}
    for row in rows:
        if not row or row[0] == "EVENTS":
            continue
        if row[0].startswith("["):
            for i in range(0, len(row) - 1, 2):
                meta[row[i].strip("[]").lower()] = row[i + 1]
            continue
        try:
            float(row[1])
        except (ValueError, IndexError):
            continue
        data.append(row + [""] * (len(header) - len(row)))
    return header, data, meta


def column(header, data, names):
    for name in names:
        if name in header:
            i = header.index(name)
            out = []
            for r in data:
                try:
                    out.append(float(r[i]))
                except ValueError:
                    pass
            return out
    return []


def summarize_run(path, pc):
    label = path.stem
    m = LABEL_RE.match(label)
    if not m:
        return None
    header, data, meta = read_profile(path)
    frame = column(header, data, COLS["frame"])
    p50 = percentile(frame, 0.5)
    row = {
        "label": label, "milestone": m["m"], "variant": m["variant"], "n": int(m["n"]),
        "preset": m["preset"], "seed": meta.get("labseed", ""), "run": int(m["run"]), "pc": pc,
        "frames": len(data),
        "game_p50_ms": percentile(column(header, data, COLS["game"]), 0.5),
        "game_p99_ms": percentile(column(header, data, COLS["game"]), 0.99),
        "render_p50_ms": percentile(column(header, data, COLS["render"]), 0.5),
        "gpu_p50_ms": percentile(column(header, data, COLS["gpu"]), 0.5),
        "frame_p50_ms": p50,
        "frame_p95_ms": percentile(frame, 0.95),
        "frame_p99_ms": percentile(frame, 0.99),
        "hitch_abs": sum(1 for v in frame if v > 50.0),
        "hitch_rel": sum(1 for v in frame if p50 != "" and v > 2 * p50),
        "draw_calls_p50": percentile(column(header, data, COLS["draw_calls"]), 0.5),
        "mem_total_mb": percentile(column(header, data, COLS["mem"]), 0.5),
        "build": build_string(meta),
    }
    row["bottleneck"] = bottleneck(row)
    row.update(types_fields(meta))
    row.update(sim_lod_fields(header, data, meta, frame))
    row["final"] = 1 if (m["tag"].endswith("fg") and not m["tag"].startswith("diag")) else ""  # 최종 묶음은 라벨 태그가 fg로 끝난다(2026-11-10 재측정). 옛 fin 묶음은 final=0
    row["fg_ok"] = fg_ok(label)
    return row


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--milestone", required=True)
    ap.add_argument("--rebuild", action="store_true", help="기존 행도 원본 CSV로 다시 계산한다(CSV가 없는 행은 그대로 둔다)")
    ap.add_argument("--pc", default="1", help="Docs/Reference/pc-specs.md의 행 번호")
    args = ap.parse_args()

    out_dir = ROOT / "Measurements" / args.milestone
    out_dir.mkdir(parents=True, exist_ok=True)
    runs_path = out_dir / "runs.csv"

    existing = {}
    if runs_path.exists():
        with open(runs_path, newline="", encoding="utf-8") as f:
            for r in csv.DictReader(f):
                existing[r["label"]] = r

    added = 0
    for path in sorted(CSV_DIR.glob(f"{args.milestone}-*.csv")):
        if path.stem in existing and not args.rebuild:
            continue
        row = summarize_run(path, existing[path.stem]["pc"] if path.stem in existing else args.pc)
        if row:
            added += path.stem not in existing
            existing[row["label"]] = row

    rows = sorted(existing.values(), key=lambda r: (r["variant"], int(r["n"]), r["preset"], int(r["run"])))
    with open(runs_path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=RUN_FIELDS)
        w.writeheader()
        for r in rows:
            r.setdefault("bottleneck", bottleneck(r))
            w.writerow({k: r.get(k, "") for k in RUN_FIELDS})

    groups = {}
    for r in rows:
        if int(r["run"]) == 0:
            continue
        tag = LABEL_RE.match(r["label"])["tag"]
        if r.get("types_assets", "") == "substituted":
            print(f"WARNING: {r['label']} LabTypesAssets=substituted (대형 에셋 대체, 스모크 전용) -> summary에서 뺀다")
            continue
        if tag.startswith("trace"):
            continue  # Insights 트레이스 실행은 비교에 쓰지 않는다(runs.csv에는 남는다)
        final = tag.endswith("fg") and not tag.startswith("diag")  # 최종 묶음(2026-11-10 재측정)
        legacy = tag.endswith("fin")  # 옛 묶음(final=0, 조건 이름은 -fin 그대로)
        btag = tag[:-2] if final else tag[:-3] if legacy else tag
        variant = r["variant"] + next((f"-{t}" for t in ("noaff", "aff", "diagbg", "diagfg", "fgtest", "e4dstart", "e4daa", "e4d", "start", "aa", "ab2", "ab", "t2") if btag.startswith(t)), "") + ("-fg" if final else "-fin" if legacy else "")  # 묶음 끝 A/A와 선행 A/B(ab)는 기준점과 따로 요약한다
        groups.setdefault((variant, int(r["n"]), r["preset"]), []).append(r)

    fields = ["variant", "n", "preset", "runs"]
    for c in CORE:
        fields += [c, f"{c}_min", f"{c}_max"]
    fields += ["bottleneck", "build", "types", "types_assets", "large_skip_ratio", "final", "fg_ok"]
    fields += ["sim_lod"] + [f for f in SIM_FIELDS if f != "sim_lod"]
    with open(out_dir / "summary.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for (variant, n, preset), rs in sorted(groups.items()):
            out = {"variant": variant, "n": n, "preset": preset, "runs": len(rs)}
            for c in CORE:
                vals = [float(r[c]) for r in rs if str(r[c]) != ""]
                out[c] = round(statistics.median(vals), 3) if vals else ""
                out[f"{c}_min"] = min(vals) if vals else ""
                out[f"{c}_max"] = max(vals) if vals else ""
            out["bottleneck"] = bottleneck(out)
            builds = sorted({r.get("build", "") for r in rs if r.get("build", "")})
            out["build"] = builds[0] if len(builds) == 1 else " // ".join(builds)
            out["types"] = "/".join(sorted({str(r.get("types", "")) for r in rs if str(r.get("types", "")) != ""}))
            out["types_assets"] = "/".join(sorted({str(r.get("types_assets", "")) for r in rs if str(r.get("types_assets", "")) != ""}))
            out["final"] = 1 if all(str(r.get("final", "")) == "1" for r in rs) else ""
            known = [str(r.get("fg_ok", "")) for r in rs if str(r.get("fg_ok", "")) != ""]
            out["fg_ok"] = f"{sum(1 for k in known if k.split('/')[0] == k.split('/')[1])}/{len(rs)}" if known else ""
            ratios = [float(r["large_skip_ratio"]) for r in rs if str(r.get("large_skip_ratio", "")) != ""]
            out["large_skip_ratio"] = round(statistics.median(ratios), 4) if ratios else ""
            lods = sorted({str(r.get("sim_lod", "")) for r in rs if str(r.get("sim_lod", "")) != ""})
            out["sim_lod"] = " // ".join(lods)
            for c in SIM_FIELDS:
                if c == "sim_lod":
                    continue
                vals = [float(r[c]) for r in rs if str(r.get(c, "")) != ""]
                out[c] = round(statistics.median(vals), 4) if vals else ""
            w.writerow(out)

    print(f"runs.csv: {len(rows)} rows (+{added}), summary.csv: {len(groups)} conditions -> {out_dir}")


if __name__ == "__main__":
    main()
