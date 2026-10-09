"""发布Steam建设目录已核差异图98／103；原始两SEB只做同字节引用，--check只读。"""
from pathlib import Path
import argparse
import json
import struct
import sys

sys.dont_write_bytecode = True
import publish_steam_startup as shared

ROOT = Path(__file__).resolve().parents[2]
sha, need = shared.sha, shared.need
IMAGES = {
    'tenant_resident.png': (98, 50, 15, '1a02c85d1b51db3da5789a6469d60bcf72a44de346f5ec251e9bb56169adb7c7'),
    'number05.png': (103, 100, 21, '9d94dd8c289f00459f4d9e7586942b5d60c130c5da381fa08b087e5194512b6d'),
}
SPRITES = {'tenant_resident.seb': (81, 98), 'number05.seb': (12, 103)}


def planned_payloads():
    """固定common对象内四项核验；只有两张白名单PNG可成为新素材文件。"""
    coverage = json.loads((ROOT/'assets/IMAGE_COVERAGE.json').read_text(encoding='utf-8'))
    records = coverage['records']
    raw = (ROOT/'DungeonVillageEXE/KairoGames_Data/resources.assets').read_bytes()
    need(sha(raw) == shared.CONTAINER_HASH, '固定resources.assets身份不同')
    obj = next(r for r in records if r['id'] == 'EXE:resources.assets:pathID=1148')
    need(obj['name'] == 'common' and obj['offset'] == 8314752 and obj['bytes'] == 1021784, '固定common对象边界')
    data = raw[obj['offset']:obj['offset']+obj['bytes']]
    need(sha(data) == obj['sha256'], 'Unity对象hash')
    name_len = struct.unpack_from('<I', data)[0]
    at = (4+name_len+3)&~3
    need(data[4:4+name_len] == b'common' and at+4 <= len(data), 'common对象名字')
    size = struct.unpack_from('<I', data, at)[0]
    encrypted = data[at+4:at+4+size]
    need(len(encrypted) == size and sha(encrypted) == obj['payload_sha256'], 'common载荷hash')
    ns, key = shared.readers()
    _, unpacked = ns['archive'](bytes(v ^ key[i % len(key)] for i,v in enumerate(encrypted)))
    archive = {name:(b,flags) for name,b,flags in unpacked}
    payloads, result = {}, []
    for entry in (*IMAGES, *SPRITES):
        b, flags = archive[entry]
        need(flags == 0, '不支持压缩条目')
        rec = next(r for r in records if r.get('source')=='EXE' and r.get('group')=='common' and r.get('entry')==entry)
        need(sha(b) == rec['sha256'] and len(b) == rec['bytes'], '白名单条目身份')
        item = dict(id=rec['id'], group='common', entry=entry, kind=rec['kind'], bytes=len(b), sha256=sha(b), source_container=rec['container'])
        apk_path = ROOT/'assets/original/common'/entry
        apk = apk_path.read_bytes()
        item['apk_sha256'] = sha(apk)
        if entry in IMAGES:
            slot,w,h,digest = IMAGES[entry]
            info, apk_info = ns['image_info'](b), ns['image_info'](apk)
            need(sha(b)==digest and (info['width'],info['height'])==(w,h), '两图固定身份/尺寸')
            need(info['rgba_sha256']==rec['rgba_sha256'] and info['rgba_sha256']!=apk_info['rgba_sha256'] and b!=apk, '两图必须保持真实像素差异')
            name = 'original/common/'+entry
            payloads[name] = b
            item.update(storage='published', file=name, index_kind='img.inf', index=slot,
                        width=w,height=h,rgba_sha256=info['rgba_sha256'],apk_rgba_sha256=apk_info['rgba_sha256'],
                        apk_bytes_equal=False,apk_pixels_equal=False,validation=info['validation'])
        else:
            slot,image = SPRITES[entry]
            need(b==apk, 'SEB别名必须与Steam逐字节相同')
            frames,refs=shared.seb_refs(b)
            need(refs==[image], 'SEB图片依赖')
            count=struct.unpack_from('>h',b,4)[0]
            parts=[dict(frame=v[0],image=v[1],crop=list(v[2:6]),offset=list(v[6:8]),flip=list(v[8:10])) for i in range(count) if (v:=struct.unpack_from('>10h',b,8+20*i))]
            need(struct.unpack_from('>h',b)[0]==1 and 8+count*20==len(b), '两SEB单层边界')
            item.update(storage='alias',file='../original/common/'+entry,index_kind='seb.inf',index=slot,
                        apk_bytes_equal=True,frames=frames,image_refs=refs,parts=parts)
        directory=next(r for r in records if r.get('source')=='EXE' and r.get('group')=='common' and r.get('entry')==item['index_kind'])
        need(any(t['index']==item['index'] and t['record']==rec['id'] for t in directory['index_targets']), '原INF槽对应')
        resident=entry.startswith('tenant_resident')
        item['consumer']=dict(contract='../../ui/STEAM_MAPCHIP_PATTERNS.md',page='Steam raw21',
                             qualification='type13且存在入住者' if resident else 'type12住宅数量',
                             call='common SEB81 frame0 lineNo=-1' if resident else 'Draw_multiValue common SEB12',
                             anchor='(J+42,K+40+37*r)' if resident else '(J+75,K+47+37*r)')
        result.append(item)
    need(len(payloads)==2 and sum(map(len,payloads.values()))==1213, '两图规模预算')
    manifest=dict(format='ark-research-steam-build-common-1',source=dict(container='DungeonVillageEXE/KairoGames_Data/resources.assets',
                  sha256=shared.CONTAINER_HASH,object={k:obj[k] for k in ('id','name','offset','bytes','sha256','payload_sha256')}),
                  logical_count=4,new_payload_count=2,alias_count=2,new_payload_bytes=1213,total_logical_bytes=sum(r['bytes'] for r in result),
                  scope='Steam建设目录差异图片98/103及同字节SEB81/12引用；不覆盖既有APK或Steam启动包',
                  path_base='file相对本清单目录，alias与published均须逐字节核SHA；产品运行不读取Unity容器或research/work',
                  language_contract='固定common根条目；不冒称已认证所有运行时语言替换或fallback',records=result)
    payloads['MANIFEST.json']=(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n').encode('utf-8')
    payloads['.gitattributes']=b'original/** -text\n'
    return payloads,manifest


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    payloads,manifest=planned_payloads()
    directory=ROOT/'assets/steam-build-common'
    if args.check:
        shared.verify_existing(directory,payloads,True)
        created=0
    else:
        created=shared.publish(directory,payloads)
    print(json.dumps(dict(mode='check' if args.check else 'publish',logical_count=4,new_payload_count=2,
                          alias_count=2,new_payload_bytes=1213,created=created,manifest_sha256=sha(payloads['MANIFEST.json'])),ensure_ascii=False))


if __name__=='__main__':
    main()
