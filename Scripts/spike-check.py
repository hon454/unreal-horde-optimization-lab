"""시험 실행 묶음(Spike<n>) 실행 하나의 CSV 메타데이터를 기대값과 대조한다(run-spike.ps1이 부른다).

실행: py -3 -I Scripts/spike-check.py <CSV> LabOrbit=r57.3/v3/h6/aids1 LabPreset=P3 LabTechSimLod=1 ...
키는 대소문자를 가리지 않는다. 어긋나면 종료 코드 1. 마지막 줄에 frames, frame_p50, frame_p95를 낸다(러너가 읽는다).
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from summarize import column, percentile, read_profile  # noqa: E402


def main():
    if len(sys.argv) < 2:
        print("usage: spike-check.py <CSV> [Key=Value ...]")
        return 2
    header, data, meta = read_profile(Path(sys.argv[1]))
    bad = 0
    for pair in sys.argv[2:]:
        key, _, want = pair.partition("=")
        got = meta.get(key.lower(), "<none>")
        if got != want:
            bad += 1
            print(f"MISMATCH {key}: want {want} got {got}")
    frame = column(header, data, ["FrameTime"])
    print(f"check={'FAIL' if bad else 'ok'} frames={len(frame)} frame_p50={percentile(frame, 0.5)} frame_p95={percentile(frame, 0.95)}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
