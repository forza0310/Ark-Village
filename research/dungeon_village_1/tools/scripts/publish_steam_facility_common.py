"""发布Steam设施详情差异图37／105；SEB15沿用同字节引用，--check只读。"""
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
    'icon_param00.png': (37, 112, 32, '2b99ce3fa12e25e4d1fab78e06b6cd0ada87f8e3f4bf68c94c8421d7172c22e0'),
    'number08.png': (105, 100, 21, '8914321921983ba8508b86db5c499cb580df1a5468860d354a09cede9d9f8361'),
}


def planned_payloads():
    """只在内存解固定common对象，拒绝身份变化与未经登记的目标。"""
    coverage = json.loads((ROOT/'assets/IMAGE_COVERAGE.json').read_text(encoding='utf-8'))
    records = coverage['records']
    raw = (ROOT/'DungeonVillageEXE/KairoGames_Data/resources.assets').read_bytes()
    need(sha(raw) == shared.CONTAINER_HASH, '固定resources.assets身份不同')
    obj = next(r for r in records if r['id'] == 'EXE:resources.assets:pathID=1148')
    need(obj['name'] == 'common' and obj['offset'] == 8314752 and obj['bytes'] == 1021784, 'common对象边界')
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
    for entry in (*IMAGES, 'number08.seb'):
        b, flags = archive[entry]
        need(flags == 0, '不支持压缩条目')
        rec = next(r for r in records if r.get('source')=='EXE' and r.get('group')=='common' and r.get('entry')==entry)
        need(sha(b) == rec['sha256'] and len(b) == rec['bytes'], '白名单条目身份')
        apk = (ROOT/'assets/original/common'/entry).read_bytes()
        item = dict(id=rec['id'], group='common', entry=entry, kind=rec['kind'], bytes=len(b),
                    sha256=sha(b), source_container=rec['container'], apk_sha256=sha(apk))
        if entry in IMAGES:
            slot,w,h,digest = IMAGES[entry]
            info, apk_info = ns['image_info'](b), ns['image_info'](apk)
            need(sha(b)==digest and (info['width'],info['height'])==(w,h), '两图固定身份/尺寸')
            need(info['rgba_sha256']==rec['rgba_sha256'] and info['rgba_sha256']!=apk_info['rgba_sha256'] and b!=apk, '实际像素差异')
            name = 'original/common/'+entry
            payloads[name] = b
            item.update(storage='published',file=name,index_kind='img.inf',index=slot,width=w,height=h,
                        rgba_sha256=info['rgba_sha256'],apk_rgba_sha256=apk_info['rgba_sha256'],
                        apk_bytes_equal=False,apk_pixels_equal=False,validation=info['validation'])
        else:
            need(b==apk, 'SEB15别名必须与Steam逐字节相同')
            frames,refs=shared.seb_refs(b)
            need(frames==21 and refs==[105], 'SEB15帧与图片依赖')
            count=struct.unpack_from('>h',b,4)[0]
            need(struct.unpack_from('>h',b)[0]==1 and 8+count*20==len(b), '单层SEB边界')
            parts=[]
            for i in range(count):
                v=struct.unpack_from('>10h',b,8+20*i)
                need(v[2]>=0 and v[3]>=0 and v[2]+v[4]<=100 and v[3]+v[5]<=21, 'SEB15裁片范围')
                parts.append(dict(frame=v[0],image=v[1],crop=list(v[2:6]),offset=list(v[6:8]),flip=list(v[8:10])))
            item.update(storage='alias',file='../original/common/'+entry,index_kind='seb.inf',index=15,
                        apk_bytes_equal=True,frames=frames,image_refs=refs,parts=parts)
        directory=next(r for r in records if r.get('source')=='EXE' and r.get('group')=='common' and r.get('entry')==item['index_kind'])
        need(any(t['index']==item['index'] and t['record']==rec['id'] for t in directory['index_targets']), 'INF槽对应')
        icon=entry=='icon_param00.png'
        item['consumer']=dict(contract='../../ui/STEAM_FACILITY_DETAIL.md',page='Steam raw74',
             qualification='普通设施页0使用效果' if icon else '普通设施两页数字、装备种类及使用效果点数',
             call='Draw_icon mode7; image37 source y16,16x16; type5 uses x96, other x16*(type%10)' if icon else 'common SEB15; DrawNumImage / Draw_money; effect frame14',
             anchor='(136,127+17*i)' if icon else '数值位置见合同；效果点(192-8*j,130+17*i)')
        result.append(item)
    need(len(payloads)==2 and sum(map(len,payloads.values()))==2438, '两图规模预算')
    manifest=dict(format='ark-research-steam-facility-common-1',source=dict(container='DungeonVillageEXE/KairoGames_Data/resources.assets',
             sha256=shared.CONTAINER_HASH,object={k:obj[k] for k in ('id','name','offset','bytes','sha256','payload_sha256')}),
             logical_count=3,new_payload_count=2,alias_count=1,new_payload_bytes=2438,total_logical_bytes=sum(r['bytes'] for r in result),
             scope='Steam设施详情差异图片37/105及同字节SEB15引用；不修改旧APK/Steam包',
             path_base='file相对清单目录；所有published/alias均须核SHA；产品运行不读取Unity或research/work',
             language_contract='固定common根条目；不冒称所有运行时语言替换已认证',records=result)
    payloads['MANIFEST.json']=(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n').encode('utf-8')
    payloads['.gitattributes']=b'original/** -text\n'
    return payloads,manifest


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    payloads,manifest=planned_payloads()
    directory=ROOT/'assets/steam-facility-common'
    if args.check:
        shared.verify_existing(directory,payloads,True)
        created=0
    else:
        created=shared.publish(directory,payloads)
    print(json.dumps(dict(mode='check' if args.check else 'publish',logical_count=3,new_payload_count=2,
                         alias_count=1,new_payload_bytes=2438,created=created,
                         manifest_sha256=sha(payloads['MANIFEST.json'])),ensure_ascii=False))


if __name__=='__main__':
    main()
