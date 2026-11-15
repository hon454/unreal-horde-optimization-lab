"""총괄 묶음의 판정 규칙 중 요약과 묶음 끝 검사가 함께 쓰는 것(리뷰 M4, Docs/Guides/measurement.md "느린 실행").

느린 실행: 한 조건의 원 실행(회차 1~6. 1~3은 첫 실행, 4~6은 실패 재측정) 가운데 Game p50과 Render p50이 둘 다 그 조건 원 실행 중앙값보다 3% 넘게 긴 실행.
러너가 느린 실행마다 새 회차 번호로 한 번 다시 잰다: 느린 실행을 회차 번호 순으로 세어 k번째(0부터)는 회차 7 + k(7~9).
다시 잰 실행이 느리지 않으면 느린 실행 대신 요약에 쓴다. 다시 잰 실행도 느리면 더 재지 않고 둘 다 표시하고 둘 다 요약에 넣는다.
표시(slow_mark): slow(다시 잰 실행이 아직 없음), slow-replaced(다시 잰 실행이 대신 쓰임, 요약에서 빠짐), slow-kept(다시 잰 실행도 느림, 둘 다 쓰임),
rerun(요약에 쓰이는 재측정), rerun-slow(느린 재측정).
표준 라이브러리만 쓴다.
"""
import statistics

SLOW_PCT = 0.03       # 조건 중앙값보다 3% 넘게
ORIG_RUN_MAX = 6      # 회차 1~3 첫 실행, 4~6 실패 재측정
RERUN_BASE = 7        # 느린 실행의 재측정 회차는 7부터 9까지
RERUN_RUN_MAX = 9
AA_RUN_MIN = 10       # 묶음 끝 A/A는 회차 11 이상


def _f(r, k):
    try:
        return float(r[k])
    except (KeyError, ValueError, TypeError):
        return None


def condition_medians(rows):
    """한 조건의 원 실행(회차 1~6)에서 Game p50과 Render p50의 중앙값. 원 실행이 3개 미만이면 판정하지 않는다(None)."""
    orig = [r for r in rows if 1 <= int(r["run"]) <= ORIG_RUN_MAX]
    if len(orig) < 3:
        return None
    g = [x for x in (_f(r, "game_p50_ms") for r in orig) if x is not None]
    n = [x for x in (_f(r, "render_p50_ms") for r in orig) if x is not None]
    if len(g) < 3 or len(n) < 3:
        return None
    return statistics.median(g), statistics.median(n)


def is_slow(r, med):
    g, n = _f(r, "game_p50_ms"), _f(r, "render_p50_ms")
    if g is None or n is None or not med:
        return False
    return g > med[0] * (1 + SLOW_PCT) and n > med[1] * (1 + SLOW_PCT)


def slow_marks(rows):
    """한 조건(A/A 제외)의 실행 행들에서 label -> slow_mark. 표시가 없는 실행은 들어 있지 않다."""
    med = condition_medians(rows)
    if not med:
        return {}
    flagged = sorted((r for r in rows if 1 <= int(r["run"]) <= ORIG_RUN_MAX and is_slow(r, med)), key=lambda r: int(r["run"]))
    reruns = {int(r["run"]): r for r in rows if RERUN_BASE <= int(r["run"]) <= RERUN_RUN_MAX}
    marks = {}
    for k, r in enumerate(flagged):
        rr = reruns.get(RERUN_BASE + k)
        if rr is None:
            marks[r["label"]] = "slow"
        elif is_slow(rr, med):
            marks[r["label"]] = "slow-kept"
            marks[rr["label"]] = "rerun-slow"
        else:
            marks[r["label"]] = "slow-replaced"
            marks[rr["label"]] = "rerun"
    return marks
