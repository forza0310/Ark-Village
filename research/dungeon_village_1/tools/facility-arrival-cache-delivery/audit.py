"""邻接缓存修复交付审计：固定白名单、真实检查与新旧证书分开，不启动长测。"""
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
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[2]
BASE = archive_work(__file__)
sha = lambda b: hashlib.sha256(b).hexdigest()
def read_text(p):
    b = archive_input(p).read_bytes()
    return b.decode('utf-16' if b.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')

# 不重新生成旧证书。只核源文件／证书链，再复制小证书供历史审计。
for directory, output, schema in [
    ('natural-application-menu-v1/month12', 'MONTH12_HISTORY.json',
     '309d5698d851da2d505796fa258316ff3dd3c1a00b99c6b1ead44784266f5ec0'),
    ('natural-application-economy-v1/month1', 'MONTH1.json',
     'bd1940f2adef221c3da305403518082e34fd5eb2b1756e478240f564725edf0c')]:
    source = ROOT / 'work/snapshots' / (directory + '.avra')
    certificate = source.with_suffix('.avra.json')
    c = json.loads(read_text(certificate))
    b = archive_input(source).read_bytes()
    assert len(b) == c['snapshot_bytes'] and sha(b) == c['snapshot_sha256']
    assert c['application_schema'] == schema and c['process_count'] == 3 and c['tail_frames'] == 20
    assert c['stop_at'] == c['capture_frame'] + 20
    if c['source_prefix']:
        s = c['source_prefix']
        for key, digest in [('file', 'snapshot_sha256'), ('certificate_file', 'certificate_sha256')]:
            p = Path(s[key]).resolve()
            assert p.is_relative_to(ROOT / 'work') and sha(archive_input(p).read_bytes()) == s[digest]
    (archive_output(BASE / output)).write_bytes(archive_input(certificate).read_bytes())

tests = {}
logs = ['ctest-first.log', 'ctest-final.log', 'ctest-application-final.log']
for name in logs:
    log = read_text(BASE / name)
    for line in log.splitlines():
        m = re.search(r'Test\s+#\d+:\s+(\S+)\s+\.+\s+(Passed|\*\*\*Failed)\s+([\d.]+) sec', line)
        if m:
            tests[m[1]] = {'passed': m[2] == 'Passed', 'seconds': float(m[3]), 'log': name}
assert len(tests) == 14 and all(t['passed'] for t in tests.values()), tests
assert '100% tests passed out of 8' in read_text(BASE / 'ctest-final.log')
assert '100% tests passed out of 1' in read_text(BASE / 'ctest-application-final.log')

files = [
    'prototype/src/startup_world_building.cpp', 'prototype/src/startup_world_persistence.cpp',
    'prototype/src/startup_application_replay.cpp', 'prototype/src/startup_application_replay_fields.inc',
    'prototype/src/startup_application_replay_fields.json', 'prototype/src/steam_startup_skin.cpp',
    'prototype/scripts/check_application_replay_fields.mjs',
    'prototype/tests/application_natural_process.mjs', 'prototype/tests/startup_world_building_test.cpp',
    'prototype/tests/startup_world_persistence_test.cpp', 'prototype/tests/startup_application_replay_checks.cpp',
    'prototype/tests/startup_application_replay_state_checks.cpp', 'prototype/tests/steam_startup_skin_checks.cpp',
    'prototype/APPLICATION_REPLAY.md', 'prototype/PERSISTENCE.md', 'prototype/STEAM_STARTUP_SKIN.md',
    'prototype/STARTUP_APPLICATION.md', 'prototype/APPLICATION_STORAGE.md', 'prototype/AUDIO_REQUESTS.md',
    'prototype/README.md', 'rules/FACILITY_USE.md', 'stages/README.md', 'stages/COMPREHENSIVE_RESEARCH.md',
    'verification/PRODUCT_REQUESTS.md', 'VERIFICATION.md',
    'work/facility-arrival-cache-review/README.md', 'work/facility-arrival-cache-review/EVIDENCE.json',
]
files += ['work/facility-arrival-cache-delivery/' + name for name in
          ['README.md', 'audit.py', 'build.log', 'build-final.log', *logs, 'month1.log',
           'MONTH1.json', 'MONTH12_HISTORY.json']]
assert len(files) == len(set(files))
records, links = [], 0
for name in files:
    p = ROOT / name
    b = archive_input(p).read_bytes()
    t = read_text(p)
    assert '\ufffd' not in t, name
    records.append({'path': name, 'bytes': len(b), 'sha256': sha(b),
                    'utf8_lf_sha256': sha(t.replace('\r\n', '\n').encode())})
    if p.suffix != '.md':
        continue
    for ref in re.findall(r'\]\(([^)]+)\)', t):
        if ref.startswith(('http:', 'https:', 'mailto:', '#')):
            continue
        target = re.sub(r':\d+$', '', unquote(ref.split('#')[0].strip('<>')))
        resolved = (p.parent / target).resolve()
        assert archive_input(resolved).exists() or resolved == BASE / 'VALIDATION.json', (name, ref)
        links += 1
release = [p for p in (ROOT / 'work/release').rglob('*') if archive_input(p).is_file()]
assert not (archive_input(ROOT / 'work/release/prototype/owner-codec-coverage.json')).exists()
result = {'baseline': 'e81520a', 'world_semantics': 4, 'application_semantics': 7,
          'system_format': 2, 'world_layout_changed': False, 'new_asset_bytes': 0,
          'tests': tests, 'links': links, 'files': records,
          'release_files': len(release), 'release_bytes': sum(archive_input(p).stat().st_size for p in release),
          'retired_ast_bytes': 36380676, 'processes': '本批构建/检查/双恢复均已收齐',
          'limits': ['条件到达不是自然寻路', '原窗口/完整皮肤未验', '修前12月不作当前前缀',
                     '128MiB预算不放宽，合法历史仍增长', '产品复验另算']}
(archive_output(BASE / 'VALIDATION.json')).write_bytes((json.dumps(result, ensure_ascii=False, indent=2)+'\n').encode())
files += ['work/facility-arrival-cache-delivery/VALIDATION.json']
(archive_output(BASE / 'FILES.json')).write_bytes((json.dumps(files, ensure_ascii=False, indent=2)+'\n').encode())
files += ['work/facility-arrival-cache-delivery/FILES.json']
(archive_output(BASE / 'historical-paths.nul')).write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in files)+b'\0')
print(json.dumps({'files': len(files), 'links': links, 'tests': len(tests),
                  'release_bytes': result['release_bytes']}))
