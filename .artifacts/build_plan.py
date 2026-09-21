from pathlib import Path
import json
import re
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.enum.style import WD_STYLE_TYPE
from docx.enum.text import WD_ALIGN_PARAGRAPH


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'deliverables' / '\u6a0a\u9646\u65ed_\u9762\u5411\u5bf9\u8c61\u8bfe\u8bbe\u5de5\u7a0b\u8ba1\u5212.md'
TARGET = SOURCE.with_name(SOURCE.stem + '_ARC内存池.docx')
META = ROOT / '.artifacts' / 'document_audit.json'
FONT_CN = '\u5b8b\u4f53'
FONT_HEAD = '\u9ed1\u4f53'
WIDTH = 9360
doc = Document()
section = doc.sections[0]
section.page_width = Inches(8.5)
section.page_height = Inches(11)
section.top_margin = section.bottom_margin = Inches(1)
section.left_margin = section.right_margin = Inches(1)
section.header_distance = section.footer_distance = Inches(.492)
for grid in list(section._sectPr.findall(qn('w:docGrid'))):
    section._sectPr.remove(grid)


def element(tag, **attrs):
    item = OxmlElement('w:' + tag)
    for key, value in attrs.items():
        item.set(qn('w:' + key), str(value))
    return item


def style(name, size, before=0, after=6, line=1.25,
          color='202124', bold=False, cn=FONT_CN, latin='Calibri'):
    if name not in doc.styles:
        doc.styles.add_style(name, WD_STYLE_TYPE.PARAGRAPH)
    item = doc.styles[name]
    item.font.name = latin
    item.font.size = Pt(size)
    item.font.bold = bold
    item.font.italic = False
    item.font.color.rgb = RGBColor.from_string(color)
    item.element.get_or_add_rPr().rFonts.set(qn('w:eastAsia'), cn)
    fmt = item.paragraph_format
    fmt.space_before = Pt(before)
    fmt.space_after = Pt(after)
    fmt.line_spacing = line
    fmt.widow_control = True
    properties = item.element.get_or_add_pPr()
    properties.append(element('snapToGrid', val=0))
    for border in list(properties.findall(qn('w:pBdr'))):
        properties.remove(border)
    return item


style('Normal', 11)
style('Title', 23, 0, 8, 1.15, '142B3D', True, FONT_HEAD)
style('Subtitle', 14, 0, 12, 1.2, '52616B', False, FONT_HEAD)
style('Heading 1', 16, 18, 10, 1.25, '2E74B5', True, FONT_HEAD)
style('Heading 2', 13, 14, 7, 1.25, '2E74B5', True, FONT_HEAD)
style('Heading 3', 12, 10, 5, 1.25, '1F4D78', True, FONT_HEAD)
style('Table Text', 9.5, 0, 2, 1.1)
style('Table Head', 9.5, 0, 2, 1.1, '1F4D78', True, FONT_HEAD)
style('Prompt', 10.5, 0, 6, 1.25, '202124')
style('List Bullet', 11, 0, 4, 1.25)
style('Table Citation', 9, 4, 4, 1.1, '52616B')
style('Header', 8, 0, 0, 1, '68737A')
style('Footer', 8, 0, 0, 1, '68737A')
for name in ['Normal', 'List Bullet', 'Prompt']:
    doc.styles[name].font.size = Pt(10.5)
    doc.styles[name].paragraph_format.line_spacing = Pt(14.5)
    doc.styles[name].paragraph_format.space_after = Pt(5 if name != 'List Bullet' else 4)
for name in ['Table Text', 'Table Head']:
    doc.styles[name].paragraph_format.line_spacing = Pt(12)
doc.styles['Prompt'].paragraph_format.line_spacing = Pt(13.5)
doc.styles['Prompt'].paragraph_format.space_after = Pt(4)
doc.styles['Heading 2'].font.size = Pt(12)
doc.styles['Heading 2'].paragraph_format.line_spacing = Pt(16)
doc.styles['Heading 2'].paragraph_format.space_before = Pt(10)
doc.styles['Heading 2'].paragraph_format.space_after = Pt(5)
for name in ['Title', 'Subtitle', 'Heading 1', 'Heading 2', 'Heading 3']:
    doc.styles[name].paragraph_format.keep_with_next = True

# Explicit numbering keeps wrapped list content aligned with its text.
numbering = doc.part.numbering_part.element
abstract = element('abstractNum', abstractNumId=90)
abstract.append(element('multiLevelType', val='singleLevel'))
lvl = element('lvl', ilvl=0)
lvl.append(element('start', val=1))
lvl.append(element('numFmt', val='bullet'))
lvl.append(element('lvlText', val='\u2022'))
lvl.append(element('lvlJc', val='left'))
ppr = element('pPr')
tabs = element('tabs')
tabs.append(element('tab', val='num', pos=540))
ppr.append(tabs)
ppr.append(element('ind', left=540, hanging=271))
ppr.append(element('spacing', before=0, after=80, line=300, lineRule='auto'))
lvl.append(ppr)
abstract.append(lvl)
numbering.append(abstract)
num = element('num', numId=90)
num.append(element('abstractNumId', val=90))
numbering.append(num)

header = section.header.paragraphs[0]
header.style = doc.styles['Header']
header.text = '\u6a0a\u9646\u65ed | \u9762\u5411\u5bf9\u8c61\u7a0b\u5e8f\u8bfe\u7a0b\u8bbe\u8ba1 | \u5de5\u7a0b\u8ba1\u5212 V1.0'
footer = section.footer.paragraphs[0]
footer.style = doc.styles['Footer']
footer.alignment = WD_ALIGN_PARAGRAPH.RIGHT
footer.add_run('\u7b2c ')
for instruction in ['PAGE', 'NUMPAGES']:
    field = element('fldSimple', instr=instruction)
    run = element('r')
    value = element('t')
    value.text = '1'
    run.append(value)
    field.append(run)
    footer._p.append(field)
    footer.add_run(' / ' if instruction == 'PAGE' else ' \u9875')

doc.core_properties.title = '\u6a0a\u9646\u65ed - \u9762\u5411\u5bf9\u8c61\u8bfe\u8bbe\u5de5\u7a0b\u8ba1\u5212'
doc.core_properties.subject = '\u6a21\u62df\u5373\u65f6\u901a\u4fe1\u7cfb\u7edf\u5b9e\u73b0'
doc.core_properties.author = '\u6a0a\u9646\u65ed'
doc.core_properties.keywords = 'C++17, FanLX, LRU, TCP, OOP'


def add_text(text, style_name='Normal'):
    paragraph = doc.add_paragraph(text, style_name)
    paragraph.paragraph_format.keep_together = False
    return paragraph


def add_table(rows):
    count = len(rows[0])
    if count == 2:
        widths = [2100, 7260]
    elif count == 4:
        widths = [2650, 1450, 2900, 2360]
    else:
        widths = [2850, 3750, 2760]
        if rows[0][0] in ['\u6b65\u9aa4', '\u7f16\u53f7 / \u539f\u6587\u4f4d\u7f6e']:
            widths = [2050, 4620, 2690]
    table = doc.add_table(rows=len(rows), cols=count)
    table.autofit = False
    props = table._tbl.tblPr
    for tag in ['tblW', 'tblInd', 'tblCellMar', 'tblBorders']:
        for old in list(props.findall(qn('w:' + tag))):
            props.remove(old)
    props.append(element('tblW', w=WIDTH, type='dxa'))
    props.append(element('tblInd', w=120, type='dxa'))
    margins = element('tblCellMar')
    for edge, value in [('top', 80), ('bottom', 80), ('start', 120), ('end', 120)]:
        margins.append(element(edge, w=value, type='dxa'))
    props.append(margins)
    borders = element('tblBorders')
    for edge in ['top', 'left', 'bottom', 'right', 'insideH', 'insideV']:
        borders.append(element(edge, val='single', sz=4, color='C9D2D9'))
    props.append(borders)
    grid = table._tbl.tblGrid
    for child in list(grid):
        grid.remove(child)
    for width in widths:
        grid.append(element('gridCol', w=width))
    for row_index, row in enumerate(table.rows):
        row._tr.get_or_add_trPr().append(element('cantSplit'))
        if row_index == 0:
            row._tr.get_or_add_trPr().append(element('tblHeader'))
        for index, cell in enumerate(row.cells):
            cell.width = Inches(widths[index] / 1440)
            cell._tc.get_or_add_tcPr().tcW.set(qn('w:w'), str(widths[index]))
            cell._tc.get_or_add_tcPr().tcW.set(qn('w:type'), 'dxa')
            cell.text = rows[row_index][index]
            cell.paragraphs[0].style = doc.styles['Table Head' if row_index == 0 else 'Table Text']
            if row_index == 0:
                cell._tc.get_or_add_tcPr().append(element('shd', fill='E8EEF5'))
                cell.paragraphs[0].paragraph_format.keep_with_next = True
    after = doc.add_paragraph()
    after.paragraph_format.space_before = Pt(0)
    after.paragraph_format.space_after = Pt(3)
    after.paragraph_format.line_spacing = 1
    after.add_run().font.size = Pt(1)


content = SOURCE.read_text(encoding='utf-8')
pages = content.split('<!-- pagebreak -->')
headings = []
for page_index, page in enumerate(pages):
    lines = page.strip().splitlines()
    in_code = False
    pending_break = page_index in {1, 15, 22}
    index = 0
    while index < len(lines):
        line = lines[index].strip()
        if not line:
            index += 1
            continue
        if line.startswith('```'):
            if in_code:
                paragraph.paragraph_format.keep_with_next = False
            in_code = not in_code
            index += 1
            continue
        if line.startswith('|') and not in_code:
            rows = []
            while index < len(lines) and lines[index].strip().startswith('|'):
                cells = [x.strip() for x in lines[index].strip().strip('|').split('|')]
                if not all(re.fullmatch(r':?-+:?', x) for x in cells):
                    rows.append(cells)
                index += 1
            add_table(rows)
            continue
        if in_code:
            paragraph = add_text(line, 'Prompt')
            paragraph.paragraph_format.keep_together = True
            paragraph.paragraph_format.keep_with_next = True
            pp = paragraph._p.get_or_add_pPr()
            pp.append(element('shd', fill='F4F6F9'))
            paragraph.paragraph_format.left_indent = Inches(.10)
            paragraph.paragraph_format.right_indent = Inches(.10)
        elif line.startswith('# '):
            paragraph = add_text(line[2:], 'Title')
        elif line.startswith('## '):
            name = 'Subtitle' if page_index == 0 else 'Heading 1'
            paragraph = add_text(line[3:], name)
            if name == 'Heading 1':
                headings.append(line[3:])
        elif line.startswith('### '):
            paragraph = add_text(line[4:], 'Heading 2')
        elif line.startswith('- '):
            paragraph = add_text(line[2:], 'List Bullet')
            props = paragraph._p.get_or_add_pPr()
            np = element('numPr')
            np.append(element('ilvl', val=0))
            np.append(element('numId', val=90))
            props.append(np)
        else:
            paragraph = add_text(line)
        if pending_break:
            paragraph.paragraph_format.page_break_before = True
            paragraph.paragraph_format.space_before = Pt(0)
            pending_break = False
        index += 1

doc.save(TARGET)

# Audit explicit geometry and coverage in the authored artifact.
assert len(headings) == 27
assert len(pages) == 28
for prefix, count in [('R', 33), ('P', 16), ('T', 16), ('S', 12)]:
    start = 0 if prefix in ['P', 'S'] else 1
    for n in range(start, count + start):
        assert f'{prefix}{n:02}' in content, (prefix, n)
for table in doc.tables:
    widths = [int(node.get(qn('w:w'))) for node in table._tbl.tblGrid]
    assert sum(widths) == WIDTH
    for row in table.rows:
        assert [int(c._tc.tcPr.tcW.w) for c in row.cells] == widths
audit = {
    'source': str(SOURCE), 'output': str(TARGET),
    'content_blocks': len(pages), 'headings': headings,
    'tables': len(doc.tables), 'characters': len(content),
    'preset': 'compact_reference_guide', 'header_pattern': 'memo_masthead',
    'overrides': {
        'cjk_fonts': 'SimSun body, SimHei headings; Calibri Latin',
        'table_text': '9.5pt / 1.10 lines / 0pt before / 2pt after',
        'prompt_text': '10.5pt / 1.25 lines / 0pt before / 6pt after / F4F6F9 fill',
        'title': '23pt / 1.15 lines / 0pt before / 8pt after / 142B3D',
        'page_start_heading': 'new pages at requirements, schedule, prompts; 0pt before',
        'table_spacer': '1pt empty paragraph, 3pt after',
        'academic_cjk_density': 'body/prompt/list 10.5pt exact 14.5pt; body/prompt after 5pt; H2 12pt exact 16pt before 10pt after 5pt; table exact 12pt',
        'prompt_density': '10.5pt exact 13.5pt; after 4pt; entire prompt stays together',
    },
    'coverage': {'R': 33, 'O': 2, 'E': 3, 'S': 12, 'T': 16, 'P': 16},
    'status': 'structural checks passed; render pending',
}
META.write_text(json.dumps(audit, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'output': str(TARGET), 'tables': len(doc.tables), 'content_blocks': len(pages)}, ensure_ascii=True))
