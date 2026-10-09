"""冻结本批静态交付清单；复算仅限已登记来源脚本，不构建或读取玩家档。"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[2]
BASE = Path(__file__).resolve().parent
paths = [
    'ui/STEAM_WINDOW_FRAME.md', 'ui/STEAM_BUILD_LIST.md', 'ui/STEAM_UI_COVERAGE.md',
    'ui/README.md', 'rules/PERSISTENCE.md', 'stages/README.md',
    'verification/PRODUCT_REQUESTS.md', 'VERIFICATION.md',
    'work/steam-ui-directory-delivery/README.md', 'work/steam-ui-directory-delivery/audit.py',
]
for folder, names in [
    ('steam-window-frame-contract', ['README.md', 'inspect.cjs', 'audit.cjs', 'EVIDENCE.json']),
    ('steam-build-list-render', ['README.md', 'inspect.cjs', 'audit.cjs', 'EVIDENCE.json']),
    ('interrupt-directory-lifecycle', ['README.md', 'audit.cjs', 'EVIDENCE.json']),
]:
    evidence = ROOT / 'work' / folder / 'EVIDENCE.json'
    before = evidence.read_bytes()
    subprocess.run(['node', str(evidence.parent / 'audit.cjs')], check=True)
    assert evidence.read_bytes() == before, '证据复算改变已有文件'
    paths.extend('work/' + folder + '/' + name for name in names)
records, links = [], 0
for name in paths:
    p = ROOT / name
    b = p.read_bytes()
    t = b.decode('utf-8-sig')
    assert '\ufffd' not in t, name
    records.append({'path': name, 'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest(),
                    'lf_sha256': hashlib.sha256(t.replace('\r\n', '\n').encode()).hexdigest()})
    if p.suffix == '.cjs':
        subprocess.run(['node', '--check', str(p)], check=True)
    if p.suffix == '.md':
        for link in re.findall(r'\]\(([^)]+)\)', t):
            if link.startswith(('http:', 'https:', 'mailto:', '#')):
                continue
            target = re.sub(r':\d+$', '', unquote(link.split('#')[0].strip('<>')))
            resolved = (p.parent / target).resolve()
            assert resolved.exists() or resolved == BASE / 'VALIDATION.json', (name, link)
            links += 1
result = {'baseline': '25f8f96', 'qualification': '有界静态来源与文档审计；无新增游戏回归或窗口',
          'files': records, 'links': links, 'new_asset_bytes': 0,
          'limits': ['完整建设pattern和业务更新另验', '中文实际字形与OS输入未知',
                     '原自动中断未实现', '目录无清除结论仅限已追踪入口',
                     '收费缓存修复和在途12月回放不在本批'],
          'bytes': sum(r['bytes'] for r in records)}
(BASE / 'VALIDATION.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
all_paths = paths + ['work/steam-ui-directory-delivery/VALIDATION.json']
(BASE / 'FILES.json').write_text(json.dumps(all_paths, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
all_paths.append('work/steam-ui-directory-delivery/FILES.json')
(BASE / 'git-paths.nul').write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in all_paths)+b'\0')
print(json.dumps({'files': len(all_paths), 'links': links, 'bytes': result['bytes']}))
