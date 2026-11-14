# Shared helpers for the Claude Code hooks in this folder (see Docs/Guides/harness.md).
# Each hook reads one JSON object from stdin and answers through its exit code:
# 0 = allow, 2 = block (PreToolUse) or send the stderr text back to Claude (PostToolUse).
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

sys.stdout.reconfigure(encoding='utf-8')
sys.stderr.reconfigure(encoding='utf-8')


def read_input():
    raw = sys.stdin.buffer.read().decode('utf-8', errors='replace')
    try:
        return json.loads(raw) if raw.strip() else {}
    except json.JSONDecodeError:
        return {}


def block(lines):
    """Stop the tool call (PreToolUse) or hand the problem back to Claude (PostToolUse)."""
    print('\n'.join(lines), file=sys.stderr)
    sys.exit(2)


def context(event, text):
    """Add text to Claude's context without blocking."""
    print(json.dumps({'hookSpecificOutput': {'hookEventName': event, 'additionalContext': text}},
                     ensure_ascii=False))
    sys.exit(0)


def git(*args):
    r = subprocess.run(['git', *args], cwd=ROOT, capture_output=True, text=True, encoding='utf-8')
    return r.stdout


def processes():
    """Lower-case image names of the running processes (Windows tasklist)."""
    try:
        r = subprocess.run(['tasklist', '/FO', 'CSV', '/NH'], capture_output=True, text=True,
                           encoding='utf-8', errors='replace', timeout=10)
    except (OSError, subprocess.TimeoutExpired):
        return set()
    return {line.split('","')[0].strip('"').lower() for line in r.stdout.splitlines() if line}


# Process names that mean a measurement, a build or the editor is running.
BENCH_EXE = 'hordeoptlab-win64-test.exe'
BUILD_EXES = {'unrealbuildtool.exe', 'automationtool.exe'}
EDITOR_EXE = 'unrealeditor.exe'


def rel(path):
    """Repository-relative POSIX path, or None for a path outside the repository."""
    m = re.match(r'^/([a-zA-Z])/(.*)', path)  # Git Bash form /g/... -> G:/...
    if m:
        path = f'{m.group(1)}:/{m.group(2)}'
    try:
        return Path(path).resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return None


def tilde_hits(files):
    """Run Scripts/check-tilde.py on the files. Return its hit lines (empty list = clean)."""
    files = [f for f in files if (ROOT / f).is_file()]
    if not files:
        return []
    r = subprocess.run([sys.executable, '-I', 'Scripts/check-tilde.py', *files], cwd=ROOT,
                       capture_output=True, text=True, encoding='utf-8')
    if r.returncode == 0:
        return []
    return [l for l in r.stdout.splitlines() if not re.match(r'^\d+ unescaped', l)]
