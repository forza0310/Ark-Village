"""管理／窗框批审计；保留主动世界与被动应用两种证书身份，不保存完整blob。"""
from pathlib import Path
import hashlib
import json
import re
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[2]
BASE = Path(__file__).resolve().parent
sha = lambda b: hashlib.sha256(b).hexdigest()
def text(p):
    b = p.read_bytes()
    return b.decode('utf-16' if b.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')

world = ROOT / 'work/active-progression-economy-v1'
blob = (world / 'prefix420.awr').read_bytes()
assert blob[:8] == b'AVRSAVE1' and int.from_bytes(blob[12:16], 'little') == 4
assert blob[-64:].decode() == sha(blob[:-64])
traces = [(world / name).read_bytes() for name in ['reference.trace', 'restore-a.trace', 'restore-b.trace']]
assert traces[0] == traces[1] == traces[2]
rows = traces[0].decode().splitlines()
assert len(rows) == 20 and [int(r.split()[0]) for r in rows] == list(range(421, 441))
for row in rows:
    fields = row.split()
    assert len(fields) == 4 and re.fullmatch('[0-9a-f]{64}', fields[1])
    assert all(re.fullmatch('(?:[0-9a-f]{2})+', value) for value in fields[2:])
for name in ['reference.log', 'restore-a.log', 'restore-b.log']:
    log = text(world / name)
    assert 'next_frame=441 frame=440' in log and 'checks passed' in log
active = {'qualification': 'natural_world_progression_tail', 'producer_revision': 'aa87318',
          'controller': 'natural-progression-expansion-v2', 'world_semantics': 4,
          'snapshot_path': 'work/active-progression-economy-v1/prefix420.awr',
          'snapshot_bytes': len(blob), 'snapshot_sha256': sha(blob), 'capture_frame': 420,
          'stop_at': 440, 'tail_frames': 20, 'process_count': 3,
          'trace_bytes': len(traces[0]), 'trace_sha256': sha(traces[0]),
          'terminal_observed': {'funds': 4400, 'humans': 1, 'tasks': 0, 'random_draws': 5},
          'limits': ['有真实面包房建设，不是首星已完成', '不是应用容器，不能互换被动应用Driver']}
(BASE / 'ACTIVE420.json').write_bytes((json.dumps(active, ensure_ascii=False, indent=2)+'\n').encode())
source = ROOT / 'work/snapshots/natural-application-economy-v1/month12.avra'
certificate = source.with_suffix('.avra.json')
c = json.loads(text(certificate)); b = source.read_bytes()
assert c['producer_revision'] == 'aa87318' and c['capture_months'] == 12 and c['tail_frames'] == 20
assert c['snapshot_bytes'] == len(b) and c['snapshot_sha256'] == sha(b) and c['process_count'] == 3
assert int.from_bytes(b[12:16], 'little') == 7
for key, digest in [('file', 'snapshot_sha256'), ('certificate_file', 'certificate_sha256')]:
    p = Path(c['source_prefix'][key]).resolve()
    assert p.is_relative_to(ROOT / 'work') and sha(p.read_bytes()) == c['source_prefix'][digest]
(BASE / 'MONTH12.json').write_bytes(certificate.read_bytes())
assert '100% tests passed out of 2' in text(BASE / 'ctest.log')
files = ['prototype/include/dungeon_village_prototype/startup_application.hpp',
         'prototype/include/dungeon_village_prototype/steam_startup_skin.hpp',
         'prototype/src/startup_application.cpp', 'prototype/src/steam_startup_skin.cpp',
         'prototype/tests/startup_application_actions_checks.cpp', 'prototype/tests/steam_startup_skin_checks.cpp',
         'prototype/STARTUP_APPLICATION.md', 'prototype/STEAM_STARTUP_SKIN.md',
         'stages/README.md', 'stages/COMPREHENSIVE_RESEARCH.md', 'verification/PRODUCT_REQUESTS.md',
         'VERIFICATION.md', 'work/active-progression-plan/README.md', 'work/active-progression-plan/EVIDENCE.json']
files += ['work/application-management-frame-delivery/'+name for name in
          ['README.md', 'audit.py', 'build.log', 'ctest.log', 'ACTIVE420.json', 'MONTH12.json']]
records, links = [], 0
for name in files:
    p = ROOT / name; b = p.read_bytes(); t = text(p)
    assert '\ufffd' not in t
    records.append({'path': name, 'bytes': len(b), 'sha256': sha(b),
                    'utf8_lf_sha256': sha(t.replace('\r\n', '\n').encode())})
    if p.suffix == '.md':
        for ref in re.findall(r'\]\(([^)]+)\)', t):
            if ref.startswith(('http:', 'https:', 'mailto:', '#')):
                continue
            target = re.sub(r':\d+$', '', unquote(ref.split('#')[0].strip('<>')))
            q = (p.parent / target).resolve()
            assert q.exists() or q == BASE / 'VALIDATION.json', (name, ref)
            links += 1
release = [p for p in (ROOT / 'work/release').rglob('*') if p.is_file()]
result = {'baseline': '6386986', 'files': records, 'links': links, 'tests': 2, 'seconds': 12.20,
          'world_semantics': 4, 'application_semantics': 7, 'schema_changed': False,
          'application_fields': [19, 43], 'new_asset_bytes': 0,
          'release_bytes': sum(p.stat().st_size for p in release), 'release_files': len(release),
          'processes': '本批构建/检查/主动三进程及被动12月三进程均已退出',
          'limits': ['高星/BOSS/自然通关未完成', 'Steam实际字体/后端/窗口未测',
                     '128MiB预算及合法历史增长仍有限制']}
(BASE / 'VALIDATION.json').write_bytes((json.dumps(result, ensure_ascii=False, indent=2)+'\n').encode())
files.append('work/application-management-frame-delivery/VALIDATION.json')
(BASE / 'FILES.json').write_bytes((json.dumps(files, ensure_ascii=False, indent=2)+'\n').encode())
files.append('work/application-management-frame-delivery/FILES.json')
(BASE / 'git-paths.nul').write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in files)+b'\0')
print(json.dumps({'files': len(files), 'links': links, 'active_snapshot_bytes': len(blob),
                  'month12_snapshot_bytes': c['snapshot_bytes'], 'release_bytes': result['release_bytes']}))
