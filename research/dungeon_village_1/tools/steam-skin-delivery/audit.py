"""Steam局部皮肤批固定清单审计；不构建、不读原档、不纳入下批在途源码。"""
# 读取按现存工具／归档位置解析历史路径；本轮候选输出只留在本地work。
import sys as _archive_sys
from pathlib import Path as _ArchivePath
_archive_sys.dont_write_bytecode = True
_archive_sys.path.insert(0, str(_ArchivePath(__file__).resolve().parents[1]))
from work_archive_paths import archive_input, archive_work, archive_output
from pathlib import Path
from urllib.parse import unquote
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
BASE = archive_work(__file__)
files = [ROOT / s for s in json.loads((archive_input(BASE / 'FILES.json')).read_text(encoding='utf-8'))]
assert len(files) == len(set(files))
links = 0
scope = []
for p in files:
    assert p.resolve().is_relative_to(ROOT) and archive_input(p).is_file()
    data = archive_input(p).read_bytes()
    text = data.decode('utf-8-sig')
    scope.append({'path': p.relative_to(ROOT).as_posix(), 'bytes': len(data),
                  'sha256': hashlib.sha256(data).hexdigest(),
                  'utf8_lf_sha256': hashlib.sha256(text.replace('\r\n', '\n').encode()).hexdigest()})
    if p.suffix == '.cjs':
        subprocess.run(['node', '--check', str(archive_input(p))], check=True, capture_output=True)
    if p.suffix != '.md':
        continue
    for ref in re.findall(r'\]\(([^)]+)\)', text):
        if ref.startswith(('http:', 'https:', 'mailto:', '#')):
            continue
        target = unquote(ref.split('#')[0].strip('<>'))
        resolved = (p.parent / target).resolve()
        assert archive_input(resolved).exists() or resolved == BASE / 'VALIDATION.json', (p, ref)
        links += 1

log = (archive_input(BASE / 'ctest.log')).read_text(encoding='utf-8')
assert '100% tests passed out of 2' in log
assert 'Total Test time (real) =  22.18 sec' in log
font = json.loads((archive_input(ROOT / 'work/steam-font-backend-contract/EVIDENCE.json')).read_text(encoding='utf-8'))
gltext = json.loads((archive_input(ROOT / 'work/steam-gltext-lifecycle/EVIDENCE.json')).read_text(encoding='utf-8'))
release = [p for p in (ROOT / 'work/release').rglob('*') if archive_input(p).is_file()]
result = {
    'date': '2026-10-09', 'baseline': 'a522b7c',
    'qualification': 'Steam局部绘制计划及文字后端静态消费；不是原窗口或完整UI验收',
    'tests': {'configuration': 'research单Release动态树', 'count': 2, 'seconds': 22.18,
              'log': 'ctest.log', 'source_boundary': '系统2/标题控制器后继源码开始前完成，未将其在途修改算入'},
    'world_semantics': 3, 'application_semantics': 5, 'schema_changed': False,
    'new_asset_copies': 0, 'links': links,
    'font_scope': {'registered_methods': font['unique_registered_methods'],
                   'cleanup_helpers': font['direct_cleanup_helpers'], 'bytes': font['unique_bytes']},
    'gltext_evidence_sha256': hashlib.sha256((archive_input(ROOT / 'work/steam-gltext-lifecycle/EVIDENCE.json')).read_bytes()).hexdigest(),
    'release_files': len(release), 'release_bytes': sum(archive_input(p).stat().st_size for p in release),
    'files': scope,
    'limits': ['中文实际字形／原窗口及平台输入未测', '计划仍含窗口／箭头helper边界和未知字体样式',
               '原GLText消费不等于池内引用释放', '后继四目录设计已获批但本批未实现',
               '自然后期/通关与合法历史增长仍另验']}
(archive_output(BASE / 'VALIDATION.json')).write_bytes((json.dumps(result, ensure_ascii=False, indent=2) + '\n').encode())
print(json.dumps({'files': len(scope), 'bytes': sum(p['bytes'] for p in scope),
                  'links': links, 'release_bytes': result['release_bytes']}, ensure_ascii=False))
