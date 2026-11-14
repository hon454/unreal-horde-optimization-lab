# D. Session brief (SessionStart).
# Puts the parts of Docs/STATUS.md that decide the next step into Claude's context,
# with what is running on this PC now. It automates step 1 of AGENTS.md "세션을 시작하면".
# Claude still reads the whole STATUS.md when the task needs it.
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _lab import BENCH_EXE, BUILD_EXES, EDITOR_EXE, ROOT, git, processes, read_input  # noqa: E402

SECTIONS = ('단계', '다음 할 일', '진행 중인 세션', '막힌 것', '사용자에게 요청한 일')
MAX_LINES = 25


def sections(text):
    out = {}
    for m in re.finditer(r'^## (.+?)\n(.*?)(?=^## |\Z)', text, re.S | re.M):
        out[m.group(1).strip()] = m.group(2).strip()
    return out


def main():
    data = read_input()
    lines = ['[session-brief] Docs/STATUS.md 발췌와 이 PC의 상태다. 작업 전에 확인한다.']

    running = processes()
    state = []
    if BENCH_EXE in running:
        state.append('Test 패키지 벤치마크가 돌고 있다. 빌드·에디터·패키징을 하지 않는다.')
    if running & BUILD_EXES:
        state.append('빌드·패키징이 돌고 있다(' + ', '.join(sorted(running & BUILD_EXES)) + ').')
    if EDITOR_EXE in running:
        state.append('Unreal 에디터가 열려 있다.')
    lines.append('실행 중: ' + (' '.join(state) if state else '벤치마크·빌드·에디터 없음.'))

    dirty = [l for l in git('status', '--porcelain').splitlines() if l]
    head = git('log', '-1', '--format=%h %s').strip()
    lines.append(f'git: HEAD {head}. 커밋하지 않은 변경 {len(dirty)}개.')

    status = ROOT / 'Docs' / 'STATUS.md'
    if status.is_file():
        found = sections(status.read_text(encoding='utf-8'))
        for name in SECTIONS:
            body = found.get(name)
            if not body:
                continue
            body_lines = body.splitlines()
            if len(body_lines) > MAX_LINES:
                body_lines = body_lines[:MAX_LINES] + [f'... ({len(body_lines) - MAX_LINES}줄 생략)']
            lines.append(f'\n## {name}\n' + '\n'.join(body_lines))
    if data.get('source') == 'compact':
        lines.append('\n대화가 압축됐다. 진행 중인 작업 단위와 Worklog 절을 다시 확인한다.')
    print('\n'.join(lines))


main()
