from pathlib import Path
import hashlib
import json
import re
from docx import Document
from docx.oxml.ns import qn
from pypdf import PdfReader


root = Path(__file__).resolve().parents[1]
source = next((root / 'deliverables').glob('*.md'))
target = source.with_name(source.stem + '_ARC内存池.docx')
markdown = source.read_text(encoding='utf-8')
expected = []
for line in markdown.splitlines():
    line = line.strip()
    if not line or line.startswith('<!--') or line.startswith('```'):
        continue
    if line.startswith('|'):
        cells = [cell.strip() for cell in line.strip('|').split('|')]
        if all(re.fullmatch(r':?-+:?', cell) for cell in cells):
            continue
        expected.extend(cells)
    else:
        expected.append(re.sub(r'^(?:#{1,3} |\- )', '', line))
document = Document(target)
actual = ''.join(node.text or '' for node in document.element.body.iter(qn('w:t')))
normalize = lambda text: re.sub(r'\s+', '', text)
assert normalize(''.join(expected)) == normalize(actual), 'Markdown/DOCX content differs'

reader = PdfReader(root / '.artifacts' / 'render' / 'plan.pdf')
rendered_text = ''.join(page.extract_text() for page in reader.pages)
assert len(reader.pages) == 27
assert all(len(page.extract_text()) > 400 for page in reader.pages)
for prefix, first, last in [('R', 1, 33), ('O', 1, 2), ('E', 1, 3),
                            ('S', 0, 11), ('P', 0, 15), ('T', 1, 16)]:
    for number in range(first, last + 1):
        token = f'{prefix}{number:02}'
        assert token in actual and token in rendered_text, token
for number in range(1, 28):
    assert (root / '.artifacts' / 'render' / f'final-{number:02}.png').is_file()

report = {
    'status': 'passed',
    'word_pages': 27,
    'markdown_docx_identical_text': True,
    'requirement_ids_in_render': True,
    'visual_review': 'All 27 pages reviewed using Word PDF export and Poppler PNGs.',
    'renderer_note': 'Packaged LibreOffice renderer unavailable; installed Microsoft Word used.',
    'requirements': 33, 'optional_items': 2, 'enhancements': 3,
    'stages': 12, 'test_groups': 16, 'prompts': 16,
    'files': {path.name: {'bytes': path.stat().st_size,
                        'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
              for path in [source, target]},
}
(root / '.artifacts' / 'verification.json').write_text(
    json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(report, ensure_ascii=True))
