"""核对本批固定交付清单、修后检查与自然短前缀；不运行游戏或改原档。"""
from pathlib import Path
from urllib.parse import unquote
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[2]
BASE = Path(__file__).resolve().parent


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


files = [ROOT / p for p in json.loads((BASE / 'FILES.json').read_text(encoding='utf-8'))]
assert len(files) == len(set(files))
assert all(p.resolve().is_relative_to(ROOT) and p.is_file() for p in files)
links = 0
for p in files:
    if p.suffix != '.md':
        continue
    for ref in re.findall(r'\]\(([^)]+)\)', p.read_text(encoding='utf-8-sig')):
        if ref.startswith(('http:', 'https:', 'mailto:', '#')):
            continue
        target = re.sub(r':\d+(?:[-–]\d+)?$', '', unquote(ref.split('#')[0].strip('<>')))
        resolved = (p.parent / target).resolve()
        assert resolved.exists() or resolved == BASE / 'VALIDATION.json', (p, ref)
        links += 1

# 首轮的失败完整保留；同名检查必须由后续真正通过的执行覆盖。
results = {}
logs = ['ctest.log', 'ctest-replay-fix.log', 'ctest-tasks-final.log',
        'ctest-sinks-final.log', 'ctest-task-flow-sink.log']
for name in logs:
    content = (BASE / name).read_text(encoding='utf-8')
    for match in re.finditer(r'Test\s+#\d+:\s+(\S+)\s+\.+\s+(Passed|\*\*\*Failed)\s+([\d.]+) sec', content):
        test, status, seconds = match.groups()
        results[test] = {'status': status, 'seconds': float(seconds), 'log': name}
assert len(results) == 22, len(results)
assert all(r['status'] == 'Passed' for r in results.values()), results
assert '100% tests passed' in (BASE / logs[-1]).read_text(encoding='utf-8')

before = (BASE / 'task-flow-before-sink.log').read_text(encoding='utf-8')
after = (BASE / 'task-flow-after-sink.log').read_text(encoding='utf-8')
old = re.search(r'^task flow summary .+$', before, re.M).group()
new = re.search(r'^task flow summary .+$', after, re.M).group()
assert re.sub(r' audio_count=\d+ audio_peak=\d+', '', new) == old
audio = re.search(r'audio_count=(\d+) audio_peak=(\d+)', new)
assert audio and int(audio[1]) > 0 and int(audio[2]) > 0

certificates = []
for name in ('frame420', 'month1'):
    p = ROOT / 'work/snapshots/natural-application-audio-v1' / (name + '.avra.json')
    cert = json.loads(p.read_text(encoding='utf-8'))
    snapshot = p.with_suffix('')
    assert sha(snapshot) == cert['snapshot_sha256']
    assert snapshot.stat().st_size == cert['snapshot_bytes']
    assert cert['application_schema'] == 'a1d7f1b59376e72f04e3cfa1df064e656bad3a91fd27fb1256c53c0b5e1463bf'
    assert cert['world_schema'] == '7f33851d8b0430afbb6234595ab01d2190f29959515d216e6da64b1919dbf440'
    assert cert['controller'] == 'application-natural-clear-v2'
    assert cert['process_count'] == 3
    assert cert['tail_frames'] == cert['terminal']['trace_rows'] == 20
    assert cert['stop_at'] == cert['capture_frame'] + 20
    if cert['source_prefix']:
        source = cert['source_prefix']
        assert sha(Path(source['file'])) == source['snapshot_sha256']
        assert sha(Path(source['certificate_file'])) == source['certificate_sha256']
    certificates.append(cert)

release = [p for p in (ROOT / 'work/release').rglob('*') if p.is_file()]
assert not (ROOT / 'work/release/prototype/owner-codec-coverage.json').exists()
scope = []
for p in sorted(files):
    data = p.read_bytes()
    item = {'path': p.relative_to(ROOT).as_posix(), 'bytes': len(data),
            'sha256': hashlib.sha256(data).hexdigest()}
    if p.suffix != '.raw':
        item['utf8_lf_sha256'] = hashlib.sha256(data.decode('utf-8-sig').replace('\r\n', '\n').encode()).hexdigest()
    scope.append(item)
report = {
    'date': '2026-10-09', 'baseline': 'e73bb31',
    'qualification': '世界typed声音、新遭遇通知与短回放；Steam菜单／语言静态合同',
    'world_semantics': 3, 'application_semantics': 5,
    'tests': results, 'natural_task_summary': new,
    'natural_certificates': certificates,
    'retired': ['世界语义1/2与应用语义1–4', '旧12/24月及初始化中间候选的接续资格'],
    'codec_ast_retired_bytes': 36380676,
    'release_files': len(release), 'release_bytes': sum(p.stat().st_size for p in release),
    'links': links, 'scope_files': scope,
    'limits': ['没有新增原窗口观察、音频设备或原档操作',
               '完整Steam标题控制器和字体运行选择仍缺',
               '自然五星/BOSS/180月通关未认证',
               '重建另一遭遇的重复发声未独立自然认证',
               '现金/任务/检查点历史仍可合法增长']}
(BASE / 'VALIDATION.json').write_bytes((json.dumps(report, ensure_ascii=False, indent=2) + '\n').encode())
print(json.dumps({'files': len(scope), 'links': links, 'tests': len(results),
                  'bytes': sum(p['bytes'] for p in scope),
                  'release_bytes': report['release_bytes']}, ensure_ascii=False))
