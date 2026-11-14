# C. Document check right after a file edit (PostToolUse on Edit, MultiEdit and Write).
#   - Markdown: a "~" that is not written as "\~" (Scripts/check-tilde.py)
#   - Docs/STATUS.md: larger than 10 KB
#   - README.md and Posts/**/README.md: batch names, condition labels and schedule words
#     that AGENTS.md "문서와 글" keeps out of public documents
#   - Scripts/*.ps1: no UTF-8 BOM. The hook adds the BOM itself.
# A problem goes back to Claude (exit 2). The edit itself stays.
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _lab import ROOT, block, context, read_input, rel, tilde_hits  # noqa: E402

STATUS_LIMIT = 10 * 1024
BOM = b'\xef\xbb\xbf'
STRIP = [re.compile(r'^\s*(```|~~~).*?^\s*\1', re.S | re.M),  # fenced code
         re.compile(r'(`+).*?\1'),                            # code span
         re.compile(r'<!--.*?-->', re.S),                     # HTML comment
         re.compile(r'\]\([^)]*\)'),                          # link target
         re.compile(r'<[^>\n]+>')]                            # HTML tag or autolink
SCHEDULE = re.compile(r'\d+\s*일차|\d+\s*주\s*안에')
LABEL = re.compile(r'\b[A-Z][A-Za-z]*\d+-[a-z]+\b')  # condition labels such as L1-full


def batch_names():
    return sorted(p.name for p in (ROOT / 'Measurements').iterdir() if p.is_dir())


def public_doc_hits(path):
    text = (ROOT / path).read_text(encoding='utf-8')
    for pat in STRIP:
        text = pat.sub(lambda m: re.sub(r'[^\n]', ' ', m.group(0)), text)
    names = re.compile(r'(?<![\w/.-])(%s)(?![\w/.-])' % '|'.join(map(re.escape, batch_names())))
    hits = []
    for no, line in enumerate(text.splitlines(), 1):
        found = names.findall(line) + LABEL.findall(line) + SCHEDULE.findall(line)
        if found:
            hits.append(f'{path}:{no}: {", ".join(sorted(set(found)))}')
    return hits


def main():
    data = read_input()
    path = rel((data.get('tool_input') or {}).get('file_path') or '')
    if not path or not (ROOT / path).is_file():
        return
    problems, notes = [], []

    if path.endswith('.md'):
        hits = tilde_hits([path])
        if hits:
            problems.append('"~"를 "\\~"로 쓰지 않은 줄이 있다. 파일 편집 도구로 고친다:')
            problems += ['  ' + h for h in hits[:20]]

    if path == 'Docs/STATUS.md':
        size = (ROOT / path).stat().st_size
        if size > STATUS_LIMIT:
            problems.append(f'Docs/STATUS.md가 {size} B로 10 KB({STATUS_LIMIT} B)를 넘는다. '
                            '끝낸 작업의 경위를 Worklog로 옮기고 한 줄만 남긴다.')

    if path == 'README.md' or re.fullmatch(r'Posts/(.+/)?README\.md', path):
        hits = public_doc_hits(path)
        if hits:
            problems.append('공개 문서에 묶음 이름·조건 라벨·일정 표현이 있다. 측정을 평이한 말로 쓴다:')
            problems += ['  ' + h for h in hits[:20]]

    if re.fullmatch(r'Scripts/.+\.ps1', path):
        raw = (ROOT / path).read_bytes()
        if not raw.startswith(BOM):
            (ROOT / path).write_bytes(BOM + raw)
            notes.append(f'{path}에 UTF-8 BOM을 붙였다(AGENTS.md "구조와 이름"). '
                         '다음에 고치기 전에 파일을 다시 읽는다.')

    if problems:
        block([f'[doc-check] {path}'] + problems + notes)
    if notes:
        context('PostToolUse', '[doc-check] ' + ' '.join(notes))


main()
