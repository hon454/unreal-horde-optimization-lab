"""조건 표(Scripts/full-conditions.json, v2)를 읽는 공용 코드. ablation-check.py, equivalence-check.py, summarize.py, plot.py가 쓴다.

라벨: <묶음>-<V0|V1>-<구성>-N<N>-P1-r<회차>. V0는 이동(궤도 속도 2m/s), V1은 정지(속도 0)다. 구성은 단계(s1~s8)나 빼 보기(a8*)이고
조건 이름은 "<V0|V1>-<구성>"이다(summary.csv의 variant와 cond 열). 트레이스(<묶음>trace-)와 촬영(<묶음>shot-)은 접두사가 달라 묶음 요약에 안 잡힌다.
표준 라이브러리만 쓴다.
"""
import json
import re
from pathlib import Path

CONDITIONS = Path(__file__).resolve().parent / "full-conditions.json"
LABEL_RE = re.compile(r"^(?P<batch>[A-Za-z][A-Za-z0-9]*?)(?P<kind>trace|traceser|shot)?-(?P<speed>V\d+)-(?P<cfg>[A-Za-z0-9]+)-N(?P<n>\d+)-P(?P<preset>\d)-r(?P<run>\d+)$")


def load():
    return json.loads(CONDITIONS.read_text(encoding="utf-8"))


def configs(doc=None):
    """구성 이름 -> 구성(expect는 base의 expect 위에 덮어쓴 값, args는 base의 args를 앞에 붙이지 않고 구성의 것 그대로)."""
    doc = doc or load()
    raw = {c["name"]: c for c in doc["configs"]}
    out = {}
    for name, c in raw.items():
        exp = dict(raw[c["base"]]["expect"]) if c.get("base") else {}
        exp.update(c["expect"])
        out[name] = dict(c, expect=exp)
    return out


def parse_label(label):
    m = LABEL_RE.match(label)
    if not m:
        return None
    d = m.groupdict()
    d["n"] = int(d["n"])
    d["run"] = int(d["run"])
    d["kind"] = d["kind"] or ""
    return d


def speed_info(doc, speed_key):
    return doc["scenario"]["speeds"][speed_key]


def window_text(doc, speed_key, measure_sec=None):
    sp = speed_info(doc, speed_key)
    return f'{sp["warmup_sec"]}+{measure_sec or doc["scenario"]["measure_sec"]}'


def expected(doc, cfgs, cfg_name, speed_key, n=None, label=None, build12=None, shot=False, serial=False, measure_sec=None, preset=None):
    """한 실행의 기대 메타데이터(키 소문자 -> 값). n, label, build12가 None이면 그 항목은 대조하지 않는다. preset(라벨의 P칸, 1|2)이 있으면 LabPreset을 P<preset>으로 둔다."""
    c = cfgs[cfg_name]
    sc = doc["scenario"]
    exp = {k.lower(): v for k, v in doc["common_expect"].items()}
    exp.update({k.lower(): v for k, v in c["expect"].items()})
    if preset:
        exp["labpreset"] = f"P{preset}"
    exp["laborbit"] = f'r{sc["radius_m"]}/v{speed_info(doc, speed_key)["mps"]}/h{sc["height_m"]}'
    exp["labwindow"] = window_text(doc, speed_key, measure_sec)
    if n:
        exp.update({"labn": str(n), "labalive": str(n), "labalivemin": str(n), "labalivemax": str(n)})
    if label:
        exp["lablabel"] = label
    if build12:
        exp["labbuild"] = build12
    if serial:
        exp["~laboverrides"] = "fully_parallel=0"  # "~"로 시작하는 키는 값이 들어 있는지만 본다
    if shot:
        exp["labhud"] = "1"
        exp.pop("labshotat", None)
    return exp
