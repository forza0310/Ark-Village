"""发布Steam人物详情common图片88；固定白名单、独占创建，--check只读。"""
from pathlib import Path
import argparse
import json
import struct
import sys

sys.dont_write_bytecode = True
import publish_steam_startup as shared

ROOT = Path(__file__).resolve().parents[2]
sha, need = shared.sha, shared.need


def planned_payloads():
    """只在内存解固定common对象；出版原条目，保留APK不同像素证据。"""
    records = json.loads((ROOT/'assets/IMAGE_COVERAGE.json').read_text(encoding='utf-8'))['records']
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
    entries = [(b, flags) for name,b,flags in unpacked if name == 'wnd_ato.png']
    need(len(entries) == 1 and entries[0][1] == 0, '唯一未压缩目标条目')
    b = entries[0][0]
    rec = next(r for r in records if r['id'] == 'EXE:resources.assets:1148:common:wnd_ato.png')
    need(len(b) == 187 and sha(b) == rec['sha256'] == 'ffbf2c65ae896f069228161125df46291121c68555b184603346e745de81c056', '图片固定字节身份')
    inf = next(r for r in records if r.get('source') == 'EXE' and r.get('group') == 'common' and r.get('entry') == 'img.inf')
    need(any(t['index'] == 88 and t['record'] == rec['id'] for t in inf['index_targets']), 'INF图片槽88')
    apk = (ROOT/'assets/original/common/wnd_ato.png').read_bytes()
    info, apk_info = ns['image_info'](b), ns['image_info'](apk)
    need((info['width'], info['height']) == (10, 7), '图片尺寸')
    need(info['rgba_sha256'] == rec['rgba_sha256'] == '29274cf1a2912792877382b2ead6da4159777d99e79fa1f0924a93fbfc8eec2d', 'RGBA身份')
    need(b != apk and info['rgba_sha256'] != apk_info['rgba_sha256'], 'APK同名图片确有字节和像素差异')
    item = dict(id=rec['id'], group='common', entry='wnd_ato.png', kind='image', bytes=len(b), sha256=sha(b),
                source_container=rec['container'], storage='published', file='original/common/wnd_ato.png',
                index_kind='img.inf', index=88, width=10, height=7, rgba_sha256=info['rgba_sha256'],
                apk_file='../original/common/wnd_ato.png', apk_sha256=sha(apk), apk_width=apk_info['width'], apk_height=apk_info['height'],
                apk_rgba_sha256=apk_info['rgba_sha256'], apk_bytes_equal=False, apk_pixels_equal=False, validation=info['validation'],
                consumer=dict(contract='../../ui/STEAM_HUMAN_DETAIL.md', evidence='../../work/steam-human-detail/EVIDENCE.json',
                              page='Steam raw60 四页共用经验区', call='SubForm._draw2_1 DrawImage at VA 0x1033D92B',
                              anchor_non_japanese=[149,113], anchor_japanese=[148,112], full_image=True))
    manifest = dict(format='ark-research-steam-human-common-1', source=dict(container='DungeonVillageEXE/KairoGames_Data/resources.assets',
                    sha256=shared.CONTAINER_HASH, object={k:obj[k] for k in ('id','name','offset','bytes','sha256','payload_sha256')}),
                    logical_count=1, new_payload_count=1, alias_count=0, new_payload_bytes=187, total_logical_bytes=187,
                    scope='Steam人物详情固定common图片88；不是完整人物/字体/窗口/语言变体包',
                    path_base='file相对本清单；产品复制资源并核SHA，不读取Unity容器或research/work', records=[item])
    return {'original/common/wnd_ato.png':b, 'MANIFEST.json':(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n').encode('utf-8'),
            '.gitattributes':b'original/** -text\n'}, manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    payloads, manifest = planned_payloads()
    directory = ROOT/'assets/steam-human-common'
    if args.check:
        shared.verify_existing(directory, payloads, True)
        created = 0
    else:
        created = shared.publish(directory, payloads)
    print(json.dumps(dict(mode='check' if args.check else 'publish', logical_count=1, new_payload_count=1,
                         new_payload_bytes=187, created=created, manifest_sha256=sha(payloads['MANIFEST.json'])), ensure_ascii=False))


if __name__ == '__main__':
    main()
