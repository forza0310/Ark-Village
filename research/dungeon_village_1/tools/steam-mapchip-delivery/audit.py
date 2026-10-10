"""复算Steam图块静态来源；既有证据必须逐字节保持，不重签变化的输入。"""
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
SOURCE = ROOT / 'work/steam-mapchip-patterns'
frozen = {name: (archive_input(SOURCE / name)).read_bytes() for name in ['ARRAYS.json', 'RESOURCES.json', 'EVIDENCE.json']}
arrays = subprocess.check_output(['node', str(archive_input(SOURCE / 'arrays.cjs')), 'tenant'])
assert json.loads(arrays) == json.loads(frozen['ARRAYS.json'])
subprocess.run([_archive_sys.executable, '-X', 'utf8', '-B', str(archive_input(SOURCE / 'resources.py'))], check=True)
subprocess.run(['node', str(archive_input(SOURCE / 'audit.cjs'))], check=True)
# 子进程新结果位于work；复核候选，不能重读verification制造恒真比较。
for name in ['RESOURCES.json', 'EVIDENCE.json']:
    assert (SOURCE / name).read_bytes() == frozen[name], name
files = ['ui/STEAM_MAPCHIP_PATTERNS.md', 'ui/STEAM_BUILD_LIST.md', 'ui/README.md',
         'ui/STEAM_UI_COVERAGE.md', 'VERIFICATION.md',
         'work/steam-mapchip-delivery/README.md', 'work/steam-mapchip-delivery/audit.py']
files += ['work/steam-mapchip-patterns/' + name for name in
          ['README.md', 'inspect.cjs', 'arrays.cjs', 'ARRAYS.json', 'resources.py',
           'RESOURCES.json', 'audit.cjs', 'EVIDENCE.json']]
records, links = [], 0
for name in files:
    p = ROOT / name
    b = archive_input(p).read_bytes()
    t = b.decode('utf-8-sig')
    assert '\ufffd' not in t
    records.append({'path': name, 'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest(),
                    'lf_sha256': hashlib.sha256(t.replace('\r\n', '\n').encode()).hexdigest()})
    if p.suffix == '.cjs':
        subprocess.run(['node', '--check', str(archive_input(p))], check=True)
    if p.suffix == '.md':
        for ref in re.findall(r'\]\(([^)]+)\)', t):
            if ref.startswith(('http:', 'https:', 'mailto:', '#')):
                continue
            target = re.sub(r':\d+$', '', unquote(ref.split('#')[0].strip('<>')))
            resolved = (p.parent / target).resolve()
            assert archive_input(resolved).exists() or resolved == BASE / 'VALIDATION.json', (name, ref)
            links += 1
result = {'baseline': 'aa87318', 'files': records, 'links': links,
          'source': {'methods': 8, 'bytes': 10685, 'anchors': 19, 'definitions': 85,
                     'sprites': 87, 'resources': 194},
          'new_asset_bytes': 0, 'builds': 0, 'background_processes': 0,
          'limits': ['完整语言覆盖/资源动态安装未闭合', '旋转准入不由数组分母推出',
                     '原资源越界和缺图保留', '未运行原程序窗口']}
(archive_output(BASE / 'VALIDATION.json')).write_bytes((json.dumps(result, ensure_ascii=False, indent=2)+'\n').encode())
files += ['work/steam-mapchip-delivery/VALIDATION.json']
(archive_output(BASE / 'FILES.json')).write_bytes((json.dumps(files, ensure_ascii=False, indent=2)+'\n').encode())
files += ['work/steam-mapchip-delivery/FILES.json']
(archive_output(BASE / 'historical-paths.nul')).write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in files)+b'\0')
print(json.dumps({'files': len(files), 'links': links, 'bytes': sum(f['bytes'] for f in records)}))
