# Finds "~" that is not written as "\~" in the git-tracked Markdown files.
# GFM draws the text between two "~" of one paragraph as strikethrough (AGENTS.md "문서와 글").
# Code spans, fenced code blocks, HTML comments and link targets are skipped.
# Run from the repository root: py -3 -I Scripts/check-tilde.py [file ...]
# Exit code 1 when a hit is found, so it can run before a commit.
import re
import subprocess
import sys

FENCE = re.compile(r'^\s*(```|~~~)')
SKIP = [
    re.compile(r'<!--.*?-->'),        # HTML comment on one line
    re.compile(r'(`+).*?\1'),         # code span
    re.compile(r'\]\([^)]*\)'),       # link or image target
    re.compile(r'<https?://[^>]*>'),  # autolink
    re.compile(r'https?://\S+'),      # bare URL
]
TILDE = re.compile(r'(?<!\\)~')
sys.stdout.reconfigure(encoding='utf-8')

files = sys.argv[1:] or subprocess.run(
    ['git', 'ls-files', '*.md'], capture_output=True, text=True, encoding='utf-8').stdout.split()

hits = 0
for path in files:
    in_fence = False
    in_comment = False
    with open(path, encoding='utf-8') as f:
        for no, line in enumerate(f, 1):
            if FENCE.match(line):
                in_fence = not in_fence
                continue
            if in_fence:
                continue
            if in_comment:
                if '-->' not in line:
                    continue
                line = line.split('-->', 1)[1]
                in_comment = False
            for pat in SKIP:
                line = pat.sub('', line)
            if '<!--' in line:
                line = line.split('<!--', 1)[0]
                in_comment = True
            if TILDE.search(line):
                hits += 1
                print(f'{path}:{no}: {line.strip()[:100]}')

print(f'{hits} unescaped "~" in {len(files)} files')
sys.exit(1 if hits else 0)
