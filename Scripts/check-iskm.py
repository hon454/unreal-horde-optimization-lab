"""run-m5-all.ps1의 메타데이터 검사용. CSV의 Lab/IskmInstances 중앙값을 찍는다. 열이 없으면 'nocol', 중앙값이 0 이하면 종료 코드 1.
사용: py -3 -I Scripts/check-iskm.py <csv>"""
import importlib.util
import statistics
import sys
from pathlib import Path

spec = importlib.util.spec_from_file_location("summarize", Path(__file__).with_name("summarize.py"))
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
header, data, _ = mod.read_profile(sys.argv[1])
vals = mod.column(header, data, ["Lab/IskmInstances"])
if not vals:
    print("nocol")
    sys.exit(2)
med = statistics.median(vals)
print(f"{med:g}")
sys.exit(0 if med > 0 else 1)
