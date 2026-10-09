"""只读固定 Steam 语言容器；只输出索引、配置和语言头，不导出完整译文。"""
from pathlib import Path
import ast,hashlib,json,struct,zlib
ROOT=Path(__file__).resolve().parents[2]
sha=lambda b:hashlib.sha256(b).hexdigest()
def need(ok,reason):
    if not ok: raise ValueError(reason)
coverage=json.loads((ROOT/'assets/IMAGE_COVERAGE.json').read_text(encoding='utf-8'))
source=ROOT/'tools/scripts/image_coverage.py'
tree=ast.parse(source.read_text(encoding='utf-8'))
functions=[n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='archive']
need(len(functions)==1,'archive reader')
ns=dict(struct=struct,zlib=zlib,need=need,sha=sha)
exec(compile(ast.Module(body=functions,type_ignores=[]),str(source),'exec'),ns)
key_node=next(n for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='key' for t in n.targets))
key=eval(compile(ast.Expression(key_node.value),str(source),'eval'),{'struct':struct})
container=ROOT/'DungeonVillageEXE/KairoGames_Data/resources.assets'
raw=container.read_bytes()
c=next(c for c in coverage['containers'] if c['kind']=='UnitySerializedFile' and c['path']=='resources.assets')
need(len(raw)==c['bytes'] and sha(raw)==c['sha256'],'resource identity')
objects=[]
for name in ['language','language_pack_template_en','language_pack_template_ja','language_pack_format']:
    r=next(r for r in coverage['records'] if r.get('kind')=='TextAsset' and r.get('name')==name)
    obj=raw[r['offset']:r['offset']+r['bytes']]
    need(len(obj)==r['bytes'] and sha(obj)==r['sha256'],'object identity')
    size=struct.unpack_from('<I',obj)[0];at=(4+size+3)&~3
    need(obj[4:4+size].decode()==name,'object name')
    size=struct.unpack_from('<I',obj,at)[0];payload=obj[at+4:at+4+size]
    need(len(payload)==size and sha(payload)==r['payload_sha256'],'payload identity')
    decoded=bytes(v^key[i%len(key)] for i,v in enumerate(payload)) if name=='language' else payload
    entry=dict(id=r['id'],name=name,bytes=r['bytes'],sha256=r['sha256'],payload_bytes=size,payload_sha256=sha(payload),decoded_sha256=sha(decoded))
    if name=='language':
        _,entries=ns['archive'](decoded)
        entry['entries']=[]
        for filename,data,flags in entries:
            item=dict(name=filename,bytes=len(data),sha256=sha(data),flags=flags)
            text=data.decode('utf-8-sig')
            if filename=='config.inf': item['configuration']=text
            else: item['headers']=[s for s in text.splitlines() if s.startswith('@') or s.startswith('"@')]
            entry['entries'].append(item)
    elif name=='language_pack_format': entry['text']=decoded.decode('utf-8-sig')
    else: entry['header_count']=sum(1 for s in decoded.decode('utf-8-sig').splitlines() if s.startswith('@') or s.startswith('"@'))
    objects.append(entry)
print(json.dumps(dict(container=dict(path=container.relative_to(ROOT).as_posix(),bytes=len(raw),sha256=sha(raw)),objects=objects),ensure_ascii=False,indent=2))
