# Makes the frame-flow figure of Docs/Guides/code-tour.md in Docs/figures/code-tour/.
# Run from anywhere: py -3 -I Scripts/plot-code-tour.py
# No measured values: the order is read from the processor declarations in Source/HordeOptLab/
# (ExecuteInGroup, ExecuteAfter, ExecuteBefore, bRequiresGameThreadExecution) and the engine
# processors they refer to (Docs/Reference/engine-notes.md).
# Colours, font and card shape follow Scripts/plot-readme.py.
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
OUT = os.path.join(ROOT, "Docs", "figures", "code-tour")

FONT = "Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Segoe UI, sans-serif"
INK, SUB, LINE, BG, CARD = "#111827", "#4b5563", "#e5e7eb", "#ffffff", "#f8fafc"
BLUE, TEAL, PINK, AMBER, DARK, GRAY = "#2563eb", "#0d9488", "#db2777", "#b45309", "#374151", "#9ca3af"
W = 800

# Ladder steps 4, 7 and 8 spend earned frame time on quality (same split as plot-readme.py).
SPEND = {4, 7, 8}


def esc(t):
    return t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def svg(h, body):
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{h}" viewBox="0 0 {W} {h}" '
            f'font-family="{FONT}" font-size="16" fill="{INK}">',
            f'<rect x="0.5" y="0.5" width="{W-1}" height="{h-1}" rx="12" fill="{BG}" stroke="{LINE}"/>',
            '<defs><marker id="arr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" '
            f'orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="{SUB}"/></marker></defs>']
    return "\n".join(head + body + ["</svg>"]) + "\n"


def chips(o, right, y, steps):
    """Ladder-step chips, right-aligned at (right, y). Width from the label length (digits + '단계')."""
    x = right
    for s in reversed(steps):
        label = f"{s}단계"
        w = 18 + 9 * len(str(s)) + 32
        x -= w
        c = PINK if s in SPEND else BLUE
        o.append(f'<rect x="{x}" y="{y - 16}" width="{w}" height="22" rx="11" fill="{c}" fill-opacity="0.12" stroke="{c}"/>')
        o.append(f'<text x="{x + w / 2}" y="{y}" text-anchor="middle" font-size="16" font-weight="700" fill="{c}">{label}</text>')
        x -= 6


def card(o, x, y, w, head, lines, steps=(), gt=False):
    h = 44 + 22 * len(lines) + (22 if gt else 0)
    o.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" fill="{CARD}" stroke="{LINE}"/>')
    o.append(f'<text x="{x + 16}" y="{y + 28}" font-size="17" font-weight="700">{esc(head)}</text>')
    if steps:
        chips(o, x + w - 12, y + 27, steps)
    for i, ln in enumerate(lines):
        o.append(f'<text x="{x + 16}" y="{y + 54 + 22 * i}" font-size="16" fill="{SUB}">{esc(ln)}</text>')
    if gt:
        o.append(f'<text x="{x + 16}" y="{y + 54 + 22 * len(lines)}" font-size="16" font-weight="700" fill="{AMBER}">게임 스레드</text>')
    return h


def arrow(o, x1, y1, x2, y2):
    o.append(f'<path d="M{x1} {y1} L{x2} {y2}" stroke="{SUB}" stroke-width="1.6" fill="none" marker-end="url(#arr)"/>')


def frame_flow():
    o = []
    o.append(f'<text x="32" y="46" font-size="22" font-weight="700">최종 구성의 한 프레임</text>')
    o.append(f'<text x="32" y="74" font-size="16" fill="{SUB}">화살표는 코드에 선언된 실행 순서다. 칩은 그 코드가 들어온 사다리 단계다.</text>')

    # Actor ticks in the same tick group as the Mass PrePhysics phase.
    y = 98
    h = card(o, 32, y, W - 64, "액터 틱(틱 그룹 PrePhysics)",
             ["ALabOrb: 이펙트 · ALabCameraRig · ALabGameMode: CSV 기록",
              "Mass 처리와 같은 틱 그룹이고 둘 사이의 순서 선언은 없다"])
    y += h + 26

    # Mass PrePhysics frame.
    fy = y
    o.append(f'<text x="24" y="{fy + 4}" font-size="17" font-weight="700">Mass 처리(PrePhysics 단계). 시작할 때 오브 위치·카메라·저격 요청을 받는다</text>')
    y = fy + 22
    lx, rx, cw = 44, 410, 346
    h = card(o, lx, y, rx + cw - lx, "1 거리 판정(LOD 그룹)",
             ["UMassLODCollectorProcessor 다음에 UMassSimulationLODProcessor와",
              "UMassVisualizationLODProcessor. 두 LOD 계산 사이의 순서는 없다"], steps=(2, 5))
    top = y + h
    y = top + 34
    arrow(o, lx + cw / 2, top, lx + cw / 2, y - 2)
    arrow(o, rx + cw / 2, top, rx + cw / 2, y - 2)

    # Left column: logic.
    ly = y
    h = card(o, lx, ly, cw, "2 접근과 조향", ["ULabApproachProcessor(Tasks)", "다음 UMassSteerToMoveTargetProcessor"], steps=(2,))
    arrow(o, lx + cw / 2, ly + h, lx + cw / 2, ly + h + 30)
    ly += h + 32
    h = card(o, lx, ly, cw, "3 이동(Movement)", ["엔진 이동·회전", "ULabOffMoveProcessor: 200m 밖"], steps=(2, 5))
    arrow(o, lx + cw / 2, ly + h, lx + cw / 2, ly + h + 30)
    ly += h + 32
    h = card(o, lx, ly, cw, "4 Lab 로직(LabLogic)", ["ULabOrbContactProcessor: 범위 피해", "다음 ULabOrbSnipeProcessor: 저격",
                                                     "다음 ULabMinionStateProcessor"], steps=(2,))
    ly += h

    # Right column: representation.
    ry = y
    h = card(o, rx, ry, cw, "5 표현 전환", ["UMassVisualizationProcessor", "액터 · ISKM · 그리지 않음"], steps=(3, 8))
    arrow(o, rx + cw / 2, ry + h, rx + cw / 2, ry + h + 30)
    ry += h + 32
    h = card(o, rx, ry, cw, "6 ISKM과 입자 갱신", ["ULabIskmProcessor", "ULabCrowdFxGatherProcessor", "둘 사이의 순서는 없다"],
             steps=(6, 7), gt=True)
    arrow(o, rx + cw / 2, ry + h, rx + cw / 2, ry + h + 30)
    ry += h + 32
    h = card(o, rx, ry, cw, "7 액터 동기화", ["ULabActorSyncProcessor"], steps=(4,), gt=True)
    ry += h

    y = max(ly, ry) + 32
    o.append(f'<text x="44" y="{y}" font-size="16" font-weight="700" fill="{AMBER}">왼쪽 열(로직)과 오른쪽 열(표현) 사이에는 순서 선언이 없다.</text>')
    o.append(f'<text x="44" y="{y + 24}" font-size="16" fill="{SUB}">태그 변경과 풀·재생성은 지연 명령이라 명령 버퍼를 비울 때 적용된다.</text>')
    o.append(f'<text x="44" y="{y + 48}" font-size="16" fill="{SUB}">ULabNearOrbCountProcessor(오브 30m 안 개체 수)는 순서 선언이 없다.</text>')
    y += 70
    o.insert(len(o), f'<rect x="24" y="{fy + 12}" width="{W - 48}" height="{y - fy - 12}" rx="12" fill="none" '
                     f'stroke="{GRAY}" stroke-dasharray="6 5"/>')
    y += 26

    h = card(o, 32, y, W - 64, "PostPhysics 단계가 끝날 때",
             ["엔진 EndVisualChanges가 쌓인 위치와 애니 데이터를 ISKM 컴포넌트에 올린다"])
    y += h
    return svg(y + 32, o)


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, "frame-flow.svg"), "w", encoding="utf-8", newline="\n") as f:
        f.write(frame_flow())
