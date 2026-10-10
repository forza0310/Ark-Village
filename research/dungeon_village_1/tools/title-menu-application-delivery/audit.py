"""系统2／应用6批固定白名单审计；不运行游戏、重建或访问原档。"""
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
def sha(p):
    return hashlib.sha256(archive_input(p).read_bytes()).hexdigest()

files = [ROOT / p for p in json.loads((archive_input(BASE / 'FILES.json')).read_text(encoding='utf-8'))]
assert len(files) == len(set(files))
scope = []
links = 0
for p in files:
    assert p.resolve().is_relative_to(ROOT) and archive_input(p).is_file(), p
    data = archive_input(p).read_bytes()
    text = data.decode('utf-8-sig')
    scope.append({'path': p.relative_to(ROOT).as_posix(), 'bytes': len(data),
                  'sha256': hashlib.sha256(data).hexdigest(),
                  'utf8_lf_sha256': hashlib.sha256(text.replace('\r\n', '\n').encode()).hexdigest()})
    if p.suffix == '.mjs':
        subprocess.run(['node', '--check', str(archive_input(p))], capture_output=True, check=True)
    if p.suffix != '.md':
        continue
    for ref in re.findall(r'\]\(([^)]+)\)', text):
        if ref.startswith(('https:', 'http:', 'mailto:', '#')):
            continue
        target = re.sub(r':\d+(?:[-–]\d+)?$', '', unquote(ref.split('#')[0].strip('<>')))
        resolved = (p.parent / target).resolve()
        assert archive_input(resolved).exists() or resolved == BASE / 'VALIDATION.json', (p, ref)
        links += 1

log = (archive_input(BASE / 'ctest-final.log')).read_text(encoding='utf-8')
assert '100% tests passed out of 5' in log and '103.30 sec' in log
tests = [{'name': m[1], 'seconds': float(m[2])} for m in
         re.finditer(r'Test\s+#\d+:\s+(\S+)\s+\.+\s+Passed\s+([\d.]+) sec', log)]
assert len(tests) == 5
certificates = []
for name in ('frame420', 'month1'):
    p = ROOT / 'work/snapshots/natural-application-menu-v1' / (name + '.avra.json')
    cert = json.loads(archive_input(p).read_text(encoding='utf-8'))
    snap = p.with_suffix('')
    assert sha(snap) == cert['snapshot_sha256'] and archive_input(snap).stat().st_size == cert['snapshot_bytes']
    assert cert['controller'] == 'application-natural-clear-v3'
    assert cert['application_schema'] == '309d5698d851da2d505796fa258316ff3dd3c1a00b99c6b1ead44784266f5ec0'
    assert cert['world_schema'] == '7f33851d8b0430afbb6234595ab01d2190f29959515d216e6da64b1919dbf440'
    assert cert['process_count'] == 3 and cert['tail_frames'] == cert['terminal']['trace_rows'] == 20
    assert cert['stop_at'] == cert['capture_frame'] + 20 and cert['section_bytes']['6'] == 4
    if cert['source_prefix']:
        source = cert['source_prefix']
        assert sha(Path(source['file'])) == source['snapshot_sha256']
        assert sha(Path(source['certificate_file'])) == source['certificate_sha256']
    certificates.append(cert)
menu = json.loads((archive_input(BASE / 'MENU_REPLAY.json')).read_text(encoding='utf-8'))
assert menu['process_count'] == 3 and menu['trace_rows'] == 18 and menu['command_count'] == 17
assert menu['terminal']['audio_count'] == 6 and menu['terminal']['blob_count'] == 1
assert menu['terminal']['random'] == 0
assert not (archive_input(ROOT / 'work/release/prototype/owner-codec-coverage.json')).exists()
release = [p for p in (ROOT / 'work/release').rglob('*') if archive_input(p).is_file()]
own_outputs = [p for p in BASE.rglob('*') if archive_input(p).is_file()]
residuals = [p for p in BASE.rglob('*') if p.name.startswith(('.avr.tmp.', '.avrapp-stage.'))]
assert not residuals, residuals
record = {
    'date': '2026-10-09', 'baseline': '4928cc9',
    'qualification': '系统2四目录、标题菜单、应用6完整文件视图及短跨进程认证',
    'system_version': 2, 'application_semantics': 6, 'world_semantics': 3,
    'application_fields': 19, 'nested_application_fields': 43,
    'tests': tests, 'test_seconds': 103.30,
    'natural_certificates': certificates, 'menu_certificate': menu,
    'new_assets': 0, 'codec_ast_retired_bytes': 36380676,
    'links': links, 'release_files': len(release), 'release_bytes': sum(archive_input(p).stat().st_size for p in release),
    'delivery_local_files': len(own_outputs), 'delivery_local_bytes': sum(archive_input(p).stat().st_size for p in own_outputs),
    'files': scope,
    'retired': ['系统1/应用1–5及标题内存v2', '旧自然应用audio-v1及v1–v4前缀接续资格'],
    'limits': ['原自动中断轮内产生者/raw14未实现', 'Windows文件发布，symlink权限场景未覆盖',
               '无新原窗口观察/完整Steam调度认证', '17命令无日期推进，不替代自然通关',
               '合法历史及失败清理债务仍可增长，wire预算不承诺进程RSS上限']}
(archive_output(BASE / 'VALIDATION.json')).write_bytes((json.dumps(record, ensure_ascii=False, indent=2) + '\n').encode())
print(json.dumps({'files': len(scope), 'links': links, 'bytes': sum(p['bytes'] for p in scope),
                  'tests': len(tests), 'release_bytes': record['release_bytes']}, ensure_ascii=False))
