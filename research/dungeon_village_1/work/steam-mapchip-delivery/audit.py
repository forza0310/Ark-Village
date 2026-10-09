"""复算Steam图块静态来源；既有证据必须逐字节保持，不重签变化的输入。"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[2]
BASE = Path(__file__).resolve().parent
SOURCE = ROOT / 'work/steam-mapchip-patterns'
frozen = {name: (SOURCE / name).read_bytes() for name in ['ARRAYS.json', 'RESOURCES.json', 'EVIDENCE.json']}
arrays = subprocess.check_output(['node', str(SOURCE / 'arrays.cjs'), 'tenant'])
assert json.loads(arrays) == json.loads(frozen['ARRAYS.json'])
subprocess.run(['python', str(SOURCE / 'resources.py')], check=True)
subprocess.run(['node', str(SOURCE / 'audit.cjs')], check=True)
for name, original in frozen.items():
    assert (SOURCE / name).read_bytes() == original, name
files = ['ui/STEAM_MAPCHIP_PATTERNS.md', 'ui/STEAM_BUILD_LIST.md', 'ui/README.md',
         'ui/STEAM_UI_COVERAGE.md', 'VERIFICATION.md',
         'work/steam-mapchip-delivery/README.md', 'work/steam-mapchip-delivery/audit.py']
files += ['work/steam-mapchip-patterns/' + name for name in
          ['README.md', 'inspect.cjs', 'arrays.cjs', 'ARRAYS.json', 'resources.py',
           'RESOURCES.json', 'audit.cjs', 'EVIDENCE.json']]
records, links = [], 0
for name in files:
    p = ROOT / name
    b = p.read_bytes()
    t = b.decode('utf-8-sig')
    assert '\ufffd' not in t
    records.append({'path': name, 'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest(),
                    'lf_sha256': hashlib.sha256(t.replace('\r\n', '\n').encode()).hexdigest()})
    if p.suffix == '.cjs':
        subprocess.run(['node', '--check', str(p)], check=True)
    if p.suffix == '.md':
        for ref in re.findall(r'\]\(([^)]+)\)', t):
            if ref.startswith(('http:', 'https:', 'mailto:', '#')):
                continue
            target = re.sub(r':\d+$', '', unquote(ref.split('#')[0].strip('<>')))
            resolved = (p.parent / target).resolve()
            assert resolved.exists() or resolved == BASE / 'VALIDATION.json', (name, ref)
            links += 1
result = {'baseline': 'aa87318', 'files': records, 'links': links,
          'source': {'methods': 8, 'bytes': 10685, 'anchors': 19, 'definitions': 85,
                     'sprites': 87, 'resources': 194},
          'new_asset_bytes': 0, 'builds': 0, 'background_processes': 0,
          'limits': ['完整语言覆盖/资源动态安装未闭合', '旋转准入不由数组分母推出',
                     '原资源越界和缺图保留', '未运行原程序窗口']}
(BASE / 'VALIDATION.json').write_bytes((json.dumps(result, ensure_ascii=False, indent=2)+'\n').encode())
files += ['work/steam-mapchip-delivery/VALIDATION.json']
(BASE / 'FILES.json').write_bytes((json.dumps(files, ensure_ascii=False, indent=2)+'\n').encode())
files += ['work/steam-mapchip-delivery/FILES.json']
(BASE / 'git-paths.nul').write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in files)+b'\0')
print(json.dumps({'files': len(files), 'links': links, 'bytes': sum(f['bytes'] for f in records)}))
