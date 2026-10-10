"""复核固定Steam图块、两张差异图片及表关联；只写摘要，不导出原资源。"""
# 读取按现存工具／归档位置解析历史路径；本轮候选输出只留在本地work。
import sys as _archive_sys
from pathlib import Path as _ArchivePath
_archive_sys.dont_write_bytecode = True
_archive_sys.path.insert(0, str(_ArchivePath(__file__).resolve().parents[1]))
from work_archive_paths import archive_input, archive_work, archive_output
from pathlib import Path
import ast
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
OUT = archive_work(__file__) / 'RESOURCES.json'
sha = lambda b: hashlib.sha256(b).hexdigest()
checks = 0


def need(ok, reason):
    global checks
    if not ok:
        raise ValueError(reason)
    checks += 1


coverage_path = ROOT / 'assets/IMAGE_COVERAGE.json'
coverage = json.loads(archive_input(coverage_path).read_text(encoding='utf-8'))
reader_path = ROOT / 'tools/scripts/image_coverage.py'
# 仅复用已交付的纯读取函数，不执行原脚本的全量遍历或写入入口。
tree = ast.parse(archive_input(reader_path).read_text(encoding='utf-8'))
selected = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in ('archive', 'image_info')]
need(len(selected) == 2, 'existing pure readers')
import zlib
ns = dict(struct=struct, zlib=zlib, need=need, sha=sha, png_cache={})
exec(compile(ast.Module(body=selected, type_ignores=[]), str(reader_path), 'exec'), ns)
key_node = next(n for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'key' for t in n.targets))
key = eval(compile(ast.Expression(key_node.value), str(reader_path), 'eval'), {'struct': struct})
records = coverage['records']
by_id = {r['id']: r for r in records}
data_root = ROOT / 'DungeonVillageEXE/KairoGames_Data'
files = {}
inputs = []


def read_container(rel):
    if rel not in files:
        item = next(c for c in coverage['containers'] if c['kind'] == 'UnitySerializedFile' and c['path'] == rel)
        raw = (archive_input(data_root / rel)).read_bytes()
        need(len(raw) == item['bytes'] and sha(raw) == item['sha256'], 'container identity: ' + rel)
        inputs.append(dict(path=rel, bytes=len(raw), sha256=sha(raw)))
        files[rel] = raw
    return files[rel]


def object_bytes(record):
    raw = read_container(record['container'])
    start, size = record['offset'], record['bytes']
    need(0 <= start <= len(raw) - size, 'object bounds')
    obj = raw[start:start + size]
    need(sha(obj) == record['sha256'], 'object identity')
    return obj


def object_name(obj):
    length = struct.unpack_from('<I', obj)[0]
    need(length <= len(obj)-4, 'object name bounds')
    return obj[4:4+length].decode('utf-8'), (4+length+3)&~3


groups = {}
objects = []
for group in ('common', 'image', 'xls'):
    rec = next(r for r in records if r.get('kind') == 'TextAsset' and r.get('name') == group)
    obj = object_bytes(rec)
    name, at = object_name(obj)
    size = struct.unpack_from('<I', obj, at)[0]
    payload = obj[at+4:at+4+size]
    need(name == group and len(payload) == size and sha(payload) == rec['payload_sha256'], 'TextAsset payload')
    decoded = bytes(v ^ key[i % len(key)] for i, v in enumerate(payload))
    _, entries = ns['archive'](decoded)
    groups[group] = {name: raw for name, raw, flags in entries}
    objects.append(dict(id=rec['id'], name=name, offset=rec['offset'], bytes=rec['bytes'], sha256=rec['sha256'], payload_sha256=sha(payload)))

# 只发布路径、裁片和字段对应摘要，不导出PNG、原表或完整归档。
def image_map(group):
    return {int(p[0]): p[1].replace('.gif', '.png') for line in groups[group]['img.inf'].decode('utf-8').splitlines() if (p := line.split('\t')) and p[0]}


def sprite_map(group):
    return [line.strip() for line in groups[group]['seb.inf'].decode('utf-8').splitlines() if line.strip()]


imaps = {g: image_map(g) for g in ('common', 'image')}
smaps = {g: sprite_map(g) for g in ('common', 'image')}
identities = {}


def identity(group, name):
    key = group + '/' + name
    if key not in identities:
        raw = groups[group][name]
        record = next(r for r in records if r.get('source') == 'EXE' and r.get('group') == group and r.get('entry') == name)
        need(sha(raw) == record['sha256'], 'entry identity ' + key)
        apk_path = ROOT / 'assets/original' / group / name
        apk = archive_input(apk_path).read_bytes() if archive_input(apk_path).exists() else None
        result = dict(group=group, name=name, source_id=record['id'], bytes=len(raw), sha256=sha(raw), apk_sha256=sha(apk) if apk else None, apk_bytes_equal=raw == apk)
        if name.endswith('.png'):
            result.update(ns['image_info'](raw))
            if apk:
                ai = ns['image_info'](apk)
                result['apk_rgba_sha256'] = ai['rgba_sha256']
                result['apk_pixels_equal'] = result['rgba_sha256'] == ai['rgba_sha256']
        identities[key] = result
    return key


sprites = {}


def sprite(group, index):
    key = group + ':' + str(index)
    if key in sprites:
        return sprites[key]
    name = smaps[group][index]
    raw = groups[group][name]
    identity(group, name)
    at = 0
    def read():
        nonlocal at
        need(at + 2 <= len(raw), 'SEB word')
        value = struct.unpack_from('>h', raw, at)[0]
        at += 2
        return value
    layers, frames = read(), read()
    need(0 < layers <= 16 and 0 < frames < 10000, 'SEB dimensions')
    parts = []
    for layer in range(layers):
        count, tag = read(), read()
        need(0 <= count <= 10000, 'SEB part budget')
        for _ in range(count):
            values = [read() for _ in range(10)]
            frame, image, x, y, w, h, ox, oy, flipx, flipy = values
            need(frame >= 0, 'SEB negative frame')
            ikey = identity(group, imaps[group][image]) if image in imaps[group] else None
            info = identities[ikey] if ikey else None
            crop_valid = info is not None and x >= 0 and y >= 0 and w > 0 and h > 0 and x+w <= info['width'] and y+h <= info['height']
            parts.append(dict(layer=layer, tag=tag, frame=frame, image=image, resource=ikey, crop=[x,y,w,h], crop_inside_image=crop_valid, offset=[ox,oy], flip=[flipx,flipy]))
    need(at == len(raw), 'SEB trailing bytes')
    outside_nominal = [p['frame'] for p in parts if p['frame'] >= frames]
    frozen = next(r for r in records if r.get('source')=='EXE' and r.get('group')==group and r.get('entry')==name)
    need(outside_nominal == frozen['keyframes_outside_nominal_count'], 'nominal frame diagnostics changed')
    result = dict(group=group, index=index, resource=group+'/'+name, nominal_frames=frames, layers=layers, keyframes_outside_nominal_count=outside_nominal, parts=parts)
    sprites[key] = result
    return result


tables = {}
for name in ('tenantData.txt', 'mapchip_main.txt'):
    raw = groups['xls']['Japanese.lproj/' + name]
    prior = (archive_input(ROOT/'work/exe-assessment/extracted/xls/Japanese.lproj'/name)).read_bytes()
    need(raw == prior, 'existing Steam table alias')
    rows = [line.split('\t') for line in raw.decode('utf-8-sig').splitlines()]
    need(len(rows) == 85 and all(len(row) == (36 if name == 'tenantData.txt' else 7) for row in rows), 'table dimensions')
    tables[name] = dict(path='xls/Japanese.lproj/'+name, bytes=len(raw), sha256=sha(raw), rows=rows)
tenant_rows = tables['tenantData.txt']['rows']
map_rows = tables['mapchip_main.txt']['rows']
tenants = {int(row[0]): row for row in tenant_rows}
maps = {int(row[0]): row for row in map_rows}
need(len(tenants) == 85 and len(maps) == 85, 'unique table ids')
arrays = json.loads((archive_input(archive_work(__file__)/'ARRAYS.json')).read_text(encoding='utf-8'))
frames, addresses = (t['values'] for t in arrays['targets'])
joins = []
for tid, tr in tenants.items():
    mid = int(tr[9]); mr = maps[mid]
    owner = int(mr[5]); reverse = tenants[owner]
    pattern = int(reverse[10]); kind = int(reverse[3]); seb_id = int(mr[3])
    need(0 <= pattern < 3, 'pattern range')
    sr = sprite('image', seb_id)
    directions = []
    for orientation in (0,1):
        pieces = []
        for i, (u,v) in enumerate(addresses[pattern][orientation]):
            frame = (1 if orientation == 1 else 11) if kind == 6 else frames[pattern][orientation][i]
            parts = [j for j,p in enumerate(sr['parts']) if p['frame'] == frame]
            outside = all(not (min(p['frame'] for p in sr['parts'] if p['layer']==layer) <= frame <= max(p['frame'] for p in sr['parts'] if p['layer']==layer)) for layer in range(sr['layers']))
            # 原SEB允许各层帧范围外返回null；不是给无原帧的方向补造资源。
            need(bool(parts) or outside, 'interpolated frame requires separate contract '+str((tid,mid,orientation,frame)))
            need(all(sr['parts'][j]['resource'] is not None for j in parts), 'requested missing image '+str((tid,mid,orientation,frame)))
            pieces.append(dict(frame=frame, address=[u,v], anchor_offset=[30*(u+v),15*(u-v)], part_indices=parts, crops_inside_images=all(sr['parts'][j]['crop_inside_image'] for j in parts), resolution='exact' if parts else 'outside_all_layer_ranges'))
        directions.append(dict(orientation=orientation, pieces=pieces))
    joins.append(dict(tenant_id=tid, mapchip_id=mid, reverse_tenant_id=owner, caller_pattern=int(tr[10]), used_pattern=pattern, reverse_type=kind, seb_index=seb_id, seb=sr['resource'], directions=directions))
common = [sprite('common', i) for i in (12,81)]
for g in ('common','image'):
    identity(g,'img.inf'); identity(g,'seb.inf')
for item in tables.values():
    item.pop('rows')
result = dict(scope='Steam mapchip pattern joins and two differing common image consumers; static only', coverage_sha256=sha(archive_input(coverage_path).read_bytes()), reader_sha256=sha(archive_input(reader_path).read_bytes()), containers=inputs, objects=objects, tables=tables, arrays_sha256=sha((archive_input(archive_work(__file__)/'ARRAYS.json')).read_bytes()), identities=list(identities.values()), sprites=list(sprites.values()), joins=joins, checks=checks, limits=['Resource group installation and all language overrides are separate contracts','Definition mapping does not prove unlock or natural building completion','No current game process, original save, image export or build'])
archive_output(OUT).write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(checks=checks,joins=len(joins),sprites=len(sprites),resources=len(identities),differences=[k for k,v in identities.items() if not v['apk_bytes_equal']],reverse_differences=[j['tenant_id'] for j in joins if j['tenant_id'] != j['reverse_tenant_id']]),ensure_ascii=False))

