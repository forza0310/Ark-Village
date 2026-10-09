"""本批交付审计：只读源码/来源/快照，更新本目录验证摘要；不运行游戏。"""
from pathlib import Path
from urllib.parse import unquote
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[2]
BASE = Path(__file__).resolve().parent
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()

# 固定已完成交付清单；复查时不将未来git差异或其他智能体在途文件纳入。
files = {ROOT/p for p in json.loads((BASE/'FILES.json').read_text(encoding='utf-8'))}
assert all(p.resolve().is_relative_to(ROOT) for p in files)
links = 0
for p in files:
    if p.suffix != '.md':
        continue
    content = p.read_text(encoding='utf-8-sig')
    assert not re.search(r'2026-10-09\?{4}', content), f'中文损坏：{p}'
    for ref in re.findall(r'\]\(([^)]+)\)', content):
        if ref.startswith(('http:', 'https:', 'mailto:', '#')):
            continue
        target = re.sub(r':\d+(?:[-–]\d+)?$', '', unquote(ref.split('#')[0].strip('<>')))
        resolved = (p.parent/target).resolve()
        assert resolved.exists() or resolved == BASE/'VALIDATION.json', (p, ref)
        links += 1

certificates = []
for name in ('frame420', 'month1'):
    p = ROOT/'work/snapshots/natural-application-v4'/(name+'.avra.json')
    cert = json.loads(p.read_text(encoding='utf-8'))
    snapshot = p.with_suffix('')
    assert cert['snapshot_sha256'] == sha(snapshot) and cert['snapshot_bytes'] == snapshot.stat().st_size
    assert cert['application_schema'] == '77fcbdfefcb6e2ad562e95d081c6d9ba3a086612dab44aa49d72faca9dae84c2'
    assert cert['process_count'] == 3 and cert['tail_frames'] == cert['terminal']['trace_rows'] == 20
    assert cert['stop_at'] == cert['capture_frame'] + 20
    if cert['source_prefix']:
        source = cert['source_prefix']
        assert sha(Path(source['file'])) == source['snapshot_sha256']
        assert sha(Path(source['certificate_file'])) == source['certificate_sha256']
    certificates.append(cert)

test_log = (BASE/'ctest-final.log').read_text(encoding='utf-8')
assert '100% tests passed out of 12' in test_log
release = [p for p in (ROOT/'work/release').rglob('*') if p.is_file()]
assert not (ROOT/'work/release/prototype/owner-codec-coverage.json').exists()
scope = []
for p in sorted(files):
    data = p.read_bytes()
    item = {'path': p.relative_to(ROOT).as_posix(), 'bytes': len(data), 'sha256': sha(p)}
    if p.suffix in {'.md', '.json', '.py', '.cjs', '.mjs', '.cpp', '.hpp', '.inc', '.log', '.txt'}:
        item['utf8_lf_sha256'] = hashlib.sha256(data.decode('utf-8-sig').replace('\r\n', '\n').encode()).hexdigest()
    scope.append(item)
report = {'date': '2026-10-09', 'baseline': '2ea65e2',
          'qualification': '已核初始化修复、静态UI/音频来源、Steam素材；非自然完整通关',
          'world_semantics': 2, 'application_semantics': 4,
          'tests': {'configuration': 'research单Release动态树', 'count': 12, 'seconds': 120.23},
          'natural_certificates': certificates,
          'retired': ['世界语义1及应用语义1/2/3', 'natural-application-v3任务池已修但设施N未修候选'],
          'interrupted': '36月采样在29月停止；会话74721 exit1，研究测试进程只读复核已退出',
          'codec_ast_retired_bytes': 36315067,
          'release_files': len(release), 'release_bytes': sum(p.stat().st_size for p in release),
          'links': links, 'scope_files': scope,
          'limits': ['原窗口未新增观察', 'typed声音及新遭遇B2/通知24尚未实现',
                     '静态菜单合同不等于已维护可操作页面', '现金/任务/审计历史仍允许合法增长']}
(BASE/'VALIDATION.json').write_bytes((json.dumps(report, ensure_ascii=False, indent=2)+'\n').encode())
print(json.dumps({'files': len(scope), 'links': links, 'bytes': sum(p['bytes'] for p in scope),
                  'release_bytes': report['release_bytes']}, ensure_ascii=False))
