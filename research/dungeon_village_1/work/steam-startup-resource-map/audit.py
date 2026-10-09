"""只读复算启动图块对应及 Unity Font 内嵌 sfnt；不导出游戏图片或字体。"""
from pathlib import Path
import ast
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent / 'EVIDENCE.json'
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
for group in ('title', 'event', 'common'):
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

names = {
    'title': ['title00.png', 'title_window.png', 'title_logo.png', 'title_cursor.png', 'title_grass.png'],
    'event': ['event_backGlad.png', 'event_medelCelemony_back2.png'],
    'common': ['wnd_back.png', 'wnd_bar.png', 'wnd_conner.png', 'arrow01.png', 'arrow02.png', 'finger_r.png', 'wnd_back.seb', 'wnd_bar.seb', 'wnd_conner.seb', 'arrow01.seb', 'arrow02.seb', 'finger_r.seb'],
}
mapping = []
directories = []
for group in groups:
    for filename in ('img.inf', 'seb.inf'):
        if filename not in groups[group]:
            continue
        original = (ROOT / 'assets/original' / group / filename).read_bytes()
        steam = groups[group][filename]
        def rows(data):
            result = []
            for line in data.decode('utf-8').splitlines():
                line = line.strip()
                if line:
                    result.append(line)
            return result
        directories.append(dict(group=group, file=filename, apk_sha256=sha(original),
                                steam_sha256=sha(steam), bytes_equal=original == steam,
                                apk_rows=rows(original), steam_rows=rows(steam)))
for group, requested in names.items():
    for name in requested:
        apk_path = ROOT / 'assets/original' / group / name
        apk = apk_path.read_bytes()
        apk_rec = by_id['APK:assets/' + group + '.dat:' + name]
        need(sha(apk) == apk_rec['sha256'], 'APK published identity')
        variants = [(entry, b) for entry, b in groups[group].items() if entry.rsplit('/', 1)[-1] == name]
        need(variants, 'Steam variant exists')
        for entry, b in variants:
            rec = next(r for r in records if r.get('source') == 'EXE' and r.get('group') == group and r.get('entry') == entry)
            need(sha(b) == rec['sha256'], 'Steam extracted entry identity')
            item = dict(apk=apk_rec['id'], steam=rec['id'], group=group, entry=entry,
                        apk_sha256=sha(apk), steam_sha256=sha(b), bytes_equal=apk == b,
                        consumer_equivalence='not_certified')
            if name.endswith('.png'):
                a, s = ns['image_info'](apk), ns['image_info'](b)
                need(a.get('rgba_sha256') and s.get('rgba_sha256'), 'RGBA decoder coverage')
                need(s['rgba_sha256'] == rec['rgba_sha256'], 'Steam RGBA agrees published inventory')
                item.update(apk_size=[a['width'], a['height']], steam_size=[s['width'], s['height']],
                            apk_rgba_sha256=a['rgba_sha256'], steam_rgba_sha256=s['rgba_sha256'],
                            pixels_equal=(a['width'], a['height'], a['rgba_sha256']) == (s['width'], s['height'], s['rgba_sha256']))
            mapping.append(item)


def sfnt_info(obj, at, length):
    data = obj[at:at+length]
    need(len(data) == length and data[:4] == b'\0\1\0\0', 'TrueType scaler')
    count = struct.unpack_from('>H', data, 4)[0]
    need(1 <= count <= 128 and 12+16*count <= length, 'sfnt directory')
    tables = {}
    for i in range(count):
        tag, checksum, off, size = struct.unpack_from('>4sIII', data, 12+16*i)
        tag = tag.decode('ascii')
        need(tag not in tables and off >= 12+16*count and off+size <= length, 'sfnt table bounds')
        payload = bytearray(data[off:off+size])
        if tag == 'head':
            need(size >= 54, 'head length')
            payload[8:12] = b'\0'*4
        payload.extend(b'\0' * ((-len(payload)) % 4))
        actual = sum(struct.unpack('>' + 'I'*(len(payload)//4), payload)) & 0xffffffff
        need(actual == checksum, 'sfnt checksum: ' + tag)
        tables[tag] = dict(offset=off, bytes=size, sha256=sha(data[off:off+size]))
    need(all(a['offset']+a['bytes'] <= b['offset'] for a, b in zip(sorted(tables.values(), key=lambda x:x['offset']), sorted(tables.values(), key=lambda x:x['offset'])[1:])), 'sfnt table nonoverlap')
    def table(tag):
        t=tables[tag];return data[t['offset']:t['offset']+t['bytes']]
    nt = table('name')
    fmt, n, strings = struct.unpack_from('>3H', nt)
    need(fmt in (0,1) and 6+n*12 <= len(nt) and strings <= len(nt), 'font name directory')
    names_out=[]
    for i in range(n):
        platform, encoding, language, name_id, size, off = struct.unpack_from('>6H', nt, 6+12*i)
        need(strings+off+size <= len(nt), 'font name bounds')
        if name_id not in (1,2,4,6):continue
        b=nt[strings+off:strings+off+size]
        if platform in (0,3):value=b.decode('utf-16-be')
        elif platform == 1 and encoding == 0:value=b.decode('mac_roman')
        else:continue
        names_out.append(dict(platform=platform, encoding=encoding, language=language, name_id=name_id, value=value))
    return dict(offset_within_object=at, bytes=length, sha256=sha(data), table_count=count, tables=tables,
                names=names_out, units_per_em=struct.unpack_from('>H',table('head'),18)[0],
                glyph_count=struct.unpack_from('>H',table('maxp'),4)[0],
                validation='sfnt_directory_bounds_nonoverlap_table_checksums_names_not_runtime_selection_or_rasterization')


fonts=[]
for rec in records:
    if rec.get('class_id') != 128:continue
    obj=object_bytes(rec)
    name, q=object_name(obj)
    info=dict(id=rec['id'], object_name=name, object_sha256=sha(obj), object_bytes=len(obj))
    if rec['container']=='resources.assets':
        # 本样本四个Font的payload位于对齐名称后68字节，前4字节为明确长度。
        # 这里依靠sfnt完整目录/校验和认证载荷，不声称其余Unity字段已全部解码。
        at=q+68
        size=struct.unpack_from('<I',obj,at-4)[0]
        need(at+size<=len(obj), 'embedded font bounds')
        info['sfnt']=sfnt_info(obj,at,size)
        info['unparsed_tail_bytes']=len(obj)-at-size
    else:
        info['sfnt']=None
        info['limitation']='Unity builtin Arial object only; no embedded font payload claimed'
    fonts.append(info)

upper=groups['title']['upper.png']
upper_rec=next(r for r in records if r.get('source')=='EXE' and r.get('group')=='title' and r.get('entry')=='upper.png')
need(sha(upper)==upper_rec['sha256'], 'Steam extra title upper identity')
upper_info=ns['image_info'](upper)
need(upper_info['rgba_sha256']==upper_rec['rgba_sha256'], 'Steam extra title upper RGBA')

evidence=dict(schema=1, scope='startup resource identity and embedded font payload only',
              coverage_sha256=sha(coverage_path.read_bytes()), reader_sha256=sha(reader_path.read_bytes()),
              script_sha256=sha(Path(__file__).read_bytes()), containers=inputs, textasset_objects=objects,
              mapping=mapping, directories=directories, fonts=fonts,
              steam_title_extra=dict(id=upper_rec['id'], sha256=sha(upper), **upper_info),
              summary=dict(mapping_rows=len(mapping),
              png_rows=sum('pixels_equal' in x for x in mapping), byte_equal=sum(x['bytes_equal'] for x in mapping),
              pixel_equal=sum(x.get('pixels_equal',False) for x in mapping), font_objects=len(fonts),
              embedded_fonts=sum(x['sfnt'] is not None for x in fonts), checks=checks),
              limits=['Steam Draw consumers and language selection not certified', 'No game/window/save access',
                      'No texture atlas rasterization or font fallback certification', 'No raw asset/font export'])
text=json.dumps(evidence,ensure_ascii=False,indent=2)+'\n'
if '--check' in sys.argv:
    need(OUT.read_text(encoding='utf-8')==text, 'evidence drift')
else:
    OUT.write_text(text,encoding='utf-8',newline='\n')
print(json.dumps(evidence['summary'],ensure_ascii=False))
