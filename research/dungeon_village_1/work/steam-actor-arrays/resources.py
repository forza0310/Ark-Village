"""只读复算启动图块对应及 Unity Font 内嵌 sfnt；不导出游戏图片或字体。"""
from pathlib import Path
import ast
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent / 'RESOURCES.json'
sha = lambda b: hashlib.sha256(b).hexdigest()
checks = 0


def need(ok, reason):
    global checks
    if not ok:
        raise ValueError(reason)
    checks += 1


coverage_path = ROOT / 'assets/IMAGE_COVERAGE.json'
coverage = json.loads(coverage_path.read_text(encoding='utf-8'))
reader_path = ROOT / 'tools/scripts/image_coverage.py'
# 仅复用已交付的纯读取函数，不执行原脚本的全量遍历或写入入口。
tree = ast.parse(reader_path.read_text(encoding='utf-8'))
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
        raw = (data_root / rel).read_bytes()
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
for group in ('human', 'weapon', 'common'):
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

# ??????Steam?????APK?????????????
def img_map(group):
    return {int(line.split('\t')[0]):line.split('\t')[1].replace('.gif','.png') for line in groups[group]['img.inf'].decode('utf-8').splitlines() if line.strip()}
def seb_map(group):
    return [line.strip() for line in groups[group]['seb.inf'].decode('utf-8').splitlines() if line.strip()]
image_maps={g:img_map(g) for g in groups}
sprite_maps={g:seb_map(g) for g in groups}
def asset(group,name):
    raw=groups[group][name]
    rec=next(r for r in records if r.get('resource_group')==group and r.get('source')=='EXE' and r.get('entry')==name)
    need(sha(raw)==rec['sha256'],'archive entry frozen identity')
    original=ROOT/'assets/original'/group/name
    apk=original.read_bytes() if original.exists() else None
    out=dict(group=group,name=name,steam_id=rec['id'],bytes=len(raw),sha256=sha(raw),apk_sha256=sha(apk) if apk is not None else None,apk_bytes_equal=raw==apk)
    if name.endswith('.png'):
        out.update(ns['image_info'](raw))
    return out
identities=[]
for g in groups:
    chosen=['img.inf','seb.inf']
    if g in ('human','weapon'):
        chosen+=list(image_maps[g].values())
        chosen+=sprite_maps[g][:4 if g=='human' else 16]
    else:
        chosen+=[image_maps[g][3],sprite_maps[g][25]]
    identities.extend(asset(g,n) for n in dict.fromkeys(chosen))
parts=[]
def seb(group,index,wanted):
    name=sprite_maps[group][index];raw=groups[group][name];at=0
    def read():
        nonlocal at
        need(at+2<=len(raw),'SEB number bound');v=struct.unpack_from('>h',raw,at)[0];at+=2;return v
    layers,frames=read(),read();need(0<layers<=16 and frames>0,'SEB header')
    selected=[]
    for layer in range(layers):
        count,tag=read(),read();need(0<=count<=10000,'SEB records')
        for _ in range(count):
            v=[read() for _ in range(10)]
            if v[0] in wanted:
                selected.append(dict(layer=layer,frame=v[0],image=v[1],crop=v[2:6],offset=v[6:8],flip=v[8:10],tag=tag))
    need(at==len(raw),'SEB trailing')
    for frame in wanted:need(any(p['frame']==frame for p in selected),'SEB frame exists')
    row=dict(group=group,index=index,name=name,layers=layers,frames=frames,parts=selected);parts.append(row);return row
body=[seb('human',i,range(4)) for i in range(4)]
weapons=[seb('weapon',i,[0]) for i in range(16)]
shadow=seb('common',25,[0])
for group,sprites in [('human',body),('weapon',weapons),('common',[shadow])]:
    # ???????????????????????????????????
    images=list(image_maps[group]) if group!='common' else [3]
    for img in images:
        info=ns['image_info'](groups[group][image_maps[group][img]])
        for sprite in sprites:
            # ????????????????????????SEB??????????
            if group=='weapon':continue
            for p in sprite['parts']:
                x,y,w,h=p['crop'];need(x>=0 and y>=0 and w>0 and h>0 and x+w<=info['width'] and y+h<=info['height'],'sprite bounds')
result=dict(qualification='Steam???????APK????????????????',coverage_sha256=sha(coverage_path.read_bytes()),reader_sha256=sha(reader_path.read_bytes()),containers=inputs,objects=objects,image_indices={g:dict(m) for g,m in image_maps.items() if g!='common'},identities=identities,selected_seb=parts,checks=checks,limits=['weapon definition to image/style pairing not re-decoded in this tool','resource equality does not establish identical schedule or hidden effects'])
OUT.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(checks=checks,identities=len(identities),equal=sum(x['apk_bytes_equal'] for x in identities),selected_seb=len(parts),images={g:len(image_maps[g]) for g in ('human','weapon')},differences=[x['group']+'/'+x['name'] for x in identities if not x['apk_bytes_equal']]),ensure_ascii=False))
