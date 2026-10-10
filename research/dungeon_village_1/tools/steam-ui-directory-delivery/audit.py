"""冻结本批静态交付清单；复算仅限已登记来源脚本，不构建或读取玩家档。"""
# 读取按现存工具／归档位置解析历史路径；本轮候选输出只留在本地work。
import sys as _archive_sys
from pathlib import Path as _ArchivePath
_archive_sys.dont_write_bytecode = True
_archive_sys.path.insert(0, str(_ArchivePath(__file__).resolve().parents[1]))
from work_archive_paths import archive_input, archive_work, archive_output
# historical-paths.nul仅保留旧路径的历史身份参考，禁止用于Git暂存。
from pathlib import Path
import hashlib
import json
import re
import subprocess
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[2]
BASE = archive_work(__file__)
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
    before = archive_input(evidence).read_bytes()
    subprocess.run(['node', str(archive_input(evidence.parent / 'audit.cjs'))], check=True)
    # 子进程只写work候选，必须把它与既有归档比较，而非重复读取归档。
    assert evidence.read_bytes() == before, '候选复算与历史证据不同'
    paths.extend('work/' + folder + '/' + name for name in names)
records, links = [], 0
for name in paths:
    p = ROOT / name
    b = archive_input(p).read_bytes()
    t = b.decode('utf-8-sig')
    assert '\ufffd' not in t, name
    records.append({'path': name, 'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest(),
                    'lf_sha256': hashlib.sha256(t.replace('\r\n', '\n').encode()).hexdigest()})
    if p.suffix == '.cjs':
        subprocess.run(['node', '--check', str(archive_input(p))], check=True)
    if p.suffix == '.md':
        for link in re.findall(r'\]\(([^)]+)\)', t):
            if link.startswith(('http:', 'https:', 'mailto:', '#')):
                continue
            target = re.sub(r':\d+$', '', unquote(link.split('#')[0].strip('<>')))
            resolved = (p.parent / target).resolve()
            assert archive_input(resolved).exists() or resolved == BASE / 'VALIDATION.json', (name, link)
            links += 1
result = {'baseline': '25f8f96', 'qualification': '有界静态来源与文档审计；无新增游戏回归或窗口',
          'files': records, 'links': links, 'new_asset_bytes': 0,
          'limits': ['完整建设pattern和业务更新另验', '中文实际字形与OS输入未知',
                     '原自动中断未实现', '目录无清除结论仅限已追踪入口',
                     '收费缓存修复和在途12月回放不在本批'],
          'bytes': sum(r['bytes'] for r in records)}
(archive_output(BASE / 'VALIDATION.json')).write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
all_paths = paths + ['work/steam-ui-directory-delivery/VALIDATION.json']
(archive_output(BASE / 'FILES.json')).write_text(json.dumps(all_paths, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
all_paths.append('work/steam-ui-directory-delivery/FILES.json')
(archive_output(BASE / 'historical-paths.nul')).write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in all_paths)+b'\0')
print(json.dumps({'files': len(all_paths), 'links': links, 'bytes': result['bytes']}))
