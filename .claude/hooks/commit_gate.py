# A. Commit gate (PreToolUse on Bash and PowerShell).
# Blocks "git commit" when it breaks an AGENTS.md rule:
#   - the subject is not Conventional Commits with an allowed type and a lower-case summary
#   - a staged path is one the repository must not hold (Saved/, Binaries/, Fab content, ...)
#   - a staged image or Unreal asset is not under Git LFS
#   - a staged Markdown file has a "~" that is not written as "\~" (Scripts/check-tilde.py)
# Files that the same command line adds with "git add" are checked too.
import re
import shlex
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _lab import block, git, read_input, tilde_hits  # noqa: E402

TYPES = ('feat', 'fix', 'docs', 'build', 'refactor', 'chore', 'bench')
SUBJECT = re.compile(r'^(%s)(\([a-z0-9][a-z0-9._/-]*\))?: [a-z0-9](?:[^\n]*[^.\s])?$' % '|'.join(TYPES))
FORBIDDEN = ('Saved/', 'Binaries/', 'Intermediate/', 'DerivedDataCache/', 'Packaged/',
             'Content/ParagonMinions/')
LFS_EXT = ('.png', '.jpg', '.jpeg', '.webp', '.gif', '.mp4', '.uasset', '.umap')

COMMIT = re.compile(r'\bgit\s+(?:-c\s+\S+\s+)*commit(?![-\w])')
HEREDOC = re.compile(r"<<-?\s*['\"]?(\w+)['\"]?[^\n]*\n(.*?)\n\s*\1", re.S)
PS_HERE = re.compile(r"(?:-m|--message)\s+@['\"]\r?\n(.*?)\r?\n['\"]@", re.S)
MSG = re.compile(r"(?:^|\s)(?:-m|--message)(?:=|\s+)(?:\"((?:[^\"\\]|\\.)*)\"|'([^']*)'|(\S+))")
SEP = re.compile(r'&&|\|\||;|\||\n')


def subject_of(cmd):
    m = HEREDOC.search(cmd) or PS_HERE.search(cmd)
    if m:
        body = m.group(m.lastindex)
    else:
        m = MSG.search(cmd)
        if not m:
            return None
        body = next(g for g in m.groups() if g is not None)
    return body.strip().splitlines()[0].strip() if body.strip() else ''


def added_paths(cmd):
    """Paths named by "git add" in the same command line, as (files, raw tokens)."""
    tokens = []
    for part in SEP.split(cmd):
        m = re.search(r'\bgit\s+add\b(.*)', part)
        if not m:
            continue
        try:
            tokens += [t for t in shlex.split(m.group(1)) if not t.startswith('-')]
        except ValueError:
            pass
    files = git('ls-files', '-m', '-o', '--exclude-standard', '--', *tokens).split('\n') if tokens else []
    return [f for f in files if f], tokens


def main():
    data = read_input()
    cmd = (data.get('tool_input') or {}).get('command') or ''
    if not COMMIT.search(cmd):
        return
    problems = []

    subject = subject_of(cmd)
    if subject is not None and not SUBJECT.match(subject):
        problems.append(f'커밋 제목이 규칙과 다르다: "{subject}"')
        problems.append(f'  형식: type(scope): summary. type은 {", ".join(TYPES)}. '
                        'summary는 영어 명령형 소문자로 시작하고 마침표로 끝내지 않는다.')

    staged = [f for f in git('diff', '--cached', '--name-only', '--diff-filter=ACMR').split('\n') if f]
    added, raw = added_paths(cmd)
    paths = sorted(set(staged) | set(added) | {t.replace('\\', '/') for t in raw})

    bad = [p for p in paths if p.startswith(FORBIDDEN)]
    if bad:
        problems.append('커밋하지 않는 경로가 들어 있다(AGENTS.md "저장소 용량"): ' + ', '.join(bad[:10]))

    binaries = [p for p in paths if p.lower().endswith(LFS_EXT) and Path(p).suffix]
    if binaries:
        attrs = git('check-attr', 'filter', '--', *binaries)
        not_lfs = [l.split(':')[0] for l in attrs.splitlines() if not l.endswith(': lfs')]
        if not_lfs:
            problems.append('Git LFS 밖의 바이너리다(.gitattributes 확인): ' + ', '.join(not_lfs[:10]))

    hits = tilde_hits([p for p in paths if p.endswith('.md')])
    if hits:
        problems.append('"~"를 "\\~"로 쓰지 않은 줄이 있다. 파일 편집 도구로 고친다:')
        problems += ['  ' + h for h in hits[:20]]

    if problems:
        block(['[commit-gate] 커밋을 막았다.'] + problems)


main()
