# B. Measurement guard (PreToolUse on Bash and PowerShell).
# Enforces AGENTS.md "여러 세션": one checkout, many sessions.
#   - While the Test package runs (a benchmark or a capture), block builds, packaging,
#     the editor, Development runs and a second benchmark. They would pollute the numbers.
#   - While a build or packaging runs (UnrealBuildTool, AutomationTool), block another one.
#     Both write to the same Binaries/.
# Put LAB_ALLOW_CONCURRENT=1 in the command to skip this guard on purpose.
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _lab import BENCH_EXE, BUILD_EXES, block, processes, read_input  # noqa: E402

BUILD = re.compile(r'Scripts[/\\](build|package)\.ps1|\bBuild\.bat\b|\bRunUAT\b|UnrealBuildTool', re.I)
EDITOR = re.compile(r'UnrealEditor(-Cmd)?(\.exe)?\b', re.I)
DEV_RUN = re.compile(r'Scripts[/\\]run-dev\.ps1', re.I)
BENCH = re.compile(r'Scripts[/\\](run-(bench|full|budget|mgr|pool|q4|transition)|capture-full)\.ps1'
                   r'|HordeOptLab-Win64-Test', re.I)


def main():
    cmd = (read_input().get('tool_input') or {}).get('command') or ''
    if 'LAB_ALLOW_CONCURRENT=1' in cmd:
        return
    kinds = [name for name, pat in (('빌드·패키징', BUILD), ('에디터', EDITOR),
                                    ('Development 실행', DEV_RUN), ('벤치마크', BENCH)) if pat.search(cmd)]
    if not kinds:
        return
    running = processes()
    if BENCH_EXE in running:
        block([f'[bench-guard] Test 패키지 벤치마크가 돌고 있어 이 명령({"·".join(kinds)})을 막았다.',
               '  측정이 오염된다(AGENTS.md "여러 세션"). 벤치마크가 끝난 뒤 다시 실행한다.',
               '  돌고 있는 세션은 Docs/STATUS.md "진행 중인 세션"에서 확인한다.'])
    if '빌드·패키징' in kinds and running & BUILD_EXES:
        names = ', '.join(sorted(running & BUILD_EXES))
        block([f'[bench-guard] 다른 빌드·패키징이 돌고 있다({names}).',
               '  같은 Binaries/를 덮어쓰므로 한 번에 한 세션만 빌드한다. 끝난 뒤 다시 실행한다.'])


main()
