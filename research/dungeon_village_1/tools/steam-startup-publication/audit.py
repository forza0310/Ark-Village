"""只读盘点启动发布最小集合，写本目录方案摘要；不提取或发布原素材。"""
# 读取按现存工具／归档位置解析历史路径；本轮候选输出只留在本地work。
import sys as _archive_sys
from pathlib import Path as _ArchivePath
_archive_sys.dont_write_bytecode = True
_archive_sys.path.insert(0, str(_ArchivePath(__file__).resolve().parents[1]))
from work_archive_paths import archive_input, archive_work, archive_output
from pathlib import Path
import hashlib
import json
import struct

ROOT = Path(__file__).resolve().parents[2]
HERE = archive_work(__file__)
sha = lambda b: hashlib.sha256(b).hexdigest()


def read_json(rel):
    return json.loads((archive_input(ROOT / rel)).read_text(encoding='utf-8'))


coverage = read_json('assets/IMAGE_COVERAGE.json')
prior = read_json('work/steam-startup-resource-map/EVIDENCE.json')
records = coverage['records']
selected = {m['steam'] for m in prior['mapping']}
assert len(selected) == 20
selected.add(prior['steam_title_extra']['id'])
selected.update(r['id'] for r in records if r.get('source') == 'EXE' and r.get('group') == 'common'
                and (r.get('entry', '').split('/')[-1] == 'saveload.png' or r.get('entry') == 'finger_l.seb'))
assert len(selected) == 32
selected.update(r['id'] for r in records if r.get('source') == 'EXE' and r.get('group') in ('common', 'event', 'title')
                and r.get('entry') in ('img.inf', 'seb.inf'))
assert len(selected) == 38
items = []
for ident in sorted(selected):
    rec = next(r for r in records if r['id'] == ident)
    candidate = ROOT / 'assets/original' / rec['group'] / rec['entry']
    existing = archive_input(candidate).is_file() and sha(archive_input(candidate).read_bytes()) == rec['sha256']
    item = {k: rec[k] for k in ('id', 'container', 'group', 'entry', 'kind', 'bytes', 'sha256')}
    for k in ('width', 'height', 'rgba_sha256'):
        if k in rec:
            item[k] = rec[k]
    item['publication'] = 'existing_exact_alias' if existing else 'new_steam_payload_required'
    item['proposed_published_path'] = candidate.relative_to(ROOT).as_posix() if existing else 'assets/steam-startup/original/' + rec['group'] + '/' + rec['entry']
    if rec['kind'] == 'SEB':
        # 七项均与现有APK副本逐字节一致，才可用现有字节核真实图片引用。
        assert existing
        data = archive_input(candidate).read_bytes()
        layers, frames = struct.unpack_from('>hh', data)
        assert 0 < layers <= 16 and 0 < frames <= 100
        at, refs = 4, set()
        for _ in range(layers):
            count, tag = struct.unpack_from('>hh', data, at)
            at += 4
            assert 0 <= count <= 100
            for _ in range(count):
                row = struct.unpack_from('>10h', data, at)
                at += 20
                if row[1] >= 0:
                    refs.add(row[1])
        assert at == len(data)
        directory = next(r for r in records if r.get('source') == 'EXE' and r.get('group') == rec['group'] and r.get('entry') == 'img.inf')
        targets = [next(t for t in directory['index_targets'] if t['index'] == i) for i in sorted(refs)]
        assert all(t['record'] in selected for t in targets)
        item['image_refs'] = targets
        item['frames'] = frames
    items.append(item)
summary = dict(logical_entries=len(items), payload_entries=sum(i['kind'] != 'INF' for i in items),
               png=sum(i['kind'] == 'image' for i in items), seb=sum(i['kind'] == 'SEB' for i in items),
               inf=sum(i['kind'] == 'INF' for i in items), bytes=sum(i['bytes'] for i in items),
               existing_aliases=sum(i['publication'] == 'existing_exact_alias' for i in items),
               required_new_entries=sum(i['publication'] != 'existing_exact_alias' for i in items),
               required_new_bytes=sum(i['bytes'] for i in items if i['publication'] != 'existing_exact_alias'))
out = dict(status='proposal_only_not_published', scope='启动图块与选档；不含完整标题人物、字体、音频或通关皮肤',
           coverage_sha256=sha((archive_input(ROOT / 'assets/IMAGE_COVERAGE.json')).read_bytes()),
           prior_evidence_sha256=sha((archive_input(ROOT / 'work/steam-startup-resource-map/EVIDENCE.json')).read_bytes()),
           summary=summary, items=items,
           directory_contract='INF作为原始证据保留；本最小包不填满所有目录引用。产品只依清单中明确发布的逻辑条目加载，不把INF当全组已发布。')
(archive_output(HERE / 'MANIFEST_DRAFT.json')).write_text(json.dumps(out, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(json.dumps(summary))
