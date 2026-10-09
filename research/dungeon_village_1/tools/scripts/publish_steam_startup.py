"""发布已核Steam启动／选档白名单；--check只读，异内容预检拒绝，APK原包不写入。"""
from pathlib import Path
import argparse
import ast
import hashlib
import json
import os
import struct
import sys
import zlib

ROOT = Path(__file__).resolve().parents[2]
CONTAINER_HASH = '75a7ac65688841505603393e3b013f26cd581d74f829aec9de3a55c73c21271b'
sha = lambda b: hashlib.sha256(b).hexdigest()
SELECTED = {
    'title': ['title00.png', 'title_window.png', 'title_logo.png', 'English.lproj/title_logo.png',
              'title_cursor.png', 'title_grass.png', 'upper.png', 'img.inf', 'seb.inf'],
    'event': ['event_backGlad.png', 'event_medelCelemony_back2.png', 'img.inf', 'seb.inf'],
    'common': ['wnd_back.png', 'wnd_bar.png', 'wnd_conner.png', 'arrow01.png', 'arrow02.png', 'finger_r.png',
               'wnd_back.seb', 'wnd_bar.seb', 'wnd_conner.seb', 'arrow01.seb', 'arrow02.seb', 'finger_r.seb',
               'finger_l.seb', 'saveload.png', 'de/saveload.png', 'es/saveload.png', 'fr/saveload.png',
               'hi/saveload.png', 'it/saveload.png', 'pt/saveload.png', 'ru/saveload.png', 'tr/saveload.png',
               'zh-CN/saveload.png', 'img.inf', 'seb.inf'],
}


def need(ok, message):
    if not ok:
        raise ValueError(message)


def readers():
    """复用已核纯读取函数，禁止执行全图像盘点脚本的遍历／写入入口。"""
    p = ROOT / 'tools/scripts/image_coverage.py'
    tree = ast.parse(p.read_text(encoding='utf-8'))
    functions = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in ('archive', 'image_info')]
    need(len(functions) == 2, '纯读取器缺失')
    ns = dict(struct=struct, zlib=zlib, need=need, sha=sha, png_cache={})
    exec(compile(ast.Module(body=functions, type_ignores=[]), str(p), 'exec'), ns)
    key_node = next(n for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'key' for t in n.targets))
    key = eval(compile(ast.Expression(key_node.value), str(p), 'eval'), {'struct': struct})
    return ns, key


def usage(group, entry):
    if entry.endswith('.inf'):
        return ['完整原始目录证据；仅可加载本清单显式选中项，禁止默认整组加载']
    if group == 'title':
        return ['Steam标题背景／菜单／Logo层；语言变体选择仍由实际语言目录决定']
    if group == 'event':
        return ['纪录背景与raw91新局裁片'] if entry == 'event_backGlad.png' else ['raw17通关计分背景']
    if entry.endswith('saveload.png'):
        return ['Steam手动／中断两行图标；177号图的两个已核IMG_SL裁片']
    if entry.startswith('wnd_'):
        return ['公共木框：标题选档、纪录、raw91新局与raw17计分']
    return ['菜单／选档／纪录翻页与选择指示；具体SEB帧按页面合同']


def seb_refs(data):
    need(len(data) >= 4, 'SEB头截断')
    layers, frames = struct.unpack_from('>hh', data)
    need(0 < layers <= 16 and 0 < frames <= 100, 'SEB计数')
    at, refs = 4, set()
    for _ in range(layers):
        need(at + 4 <= len(data), 'SEB层截断')
        count, tag = struct.unpack_from('>hh', data, at)
        at += 4
        need(0 <= count <= 100 and at + count * 20 <= len(data), 'SEB记录越界')
        for _ in range(count):
            row = struct.unpack_from('>10h', data, at)
            at += 20
            if row[1] >= 0:
                refs.add(row[1])
    need(at == len(data), 'SEB尾随数据')
    return frames, sorted(refs)


def planned_payloads():
    """从固定Unity对象重新读取，只发布38逻辑身份；不读取work。"""
    coverage = json.loads((ROOT / 'assets/IMAGE_COVERAGE.json').read_text(encoding='utf-8'))
    records = coverage['records']
    raw = (ROOT / 'DungeonVillageEXE/KairoGames_Data/resources.assets').read_bytes()
    need(sha(raw) == CONTAINER_HASH, 'resources.assets身份不同')
    ns, key = readers()
    payloads, result, inputs = {}, [], []
    selected_ids = {r['id'] for r in records if r.get('source') == 'EXE' and r.get('group') in SELECTED
                    and r.get('entry') in SELECTED[r['group']]}
    need(len(selected_ids) == 38, '固定白名单数量不符')
    for group, entries in SELECTED.items():
        obj = next(r for r in records if r.get('kind') == 'TextAsset' and r.get('name') == group)
        need(obj['container'] == 'resources.assets', '对象容器不符')
        start, count = obj['offset'], obj['bytes']
        need(0 <= start <= len(raw) - count, 'Unity对象越界')
        data = raw[start:start+count]
        need(sha(data) == obj['sha256'], 'Unity对象身份不同')
        name_len = struct.unpack_from('<I', data)[0]
        at = (4 + name_len + 3) & ~3
        need(data[4:4+name_len].decode('utf-8') == group and at+4 <= len(data), 'TextAsset名字／边界')
        size = struct.unpack_from('<I', data, at)[0]
        encrypted = data[at+4:at+4+size]
        need(len(encrypted) == size and sha(encrypted) == obj['payload_sha256'], 'TextAsset载荷身份')
        decoded = bytes(v ^ key[i % len(key)] for i, v in enumerate(encrypted))
        fmt, unpacked = ns['archive'](decoded)
        archive = {name: (b, flags) for name, b, flags in unpacked}
        inputs.append({k: obj[k] for k in ('id', 'name', 'offset', 'bytes', 'sha256', 'payload_sha256')})
        for entry in entries:
            need(entry in archive and archive[entry][1] == 0, '白名单条目或压缩标志不同')
            b = archive[entry][0]
            rec = next(r for r in records if r['id'] in selected_ids and r['group'] == group and r['entry'] == entry)
            need(len(b) == rec['bytes'] and sha(b) == rec['sha256'], '条目字节不符：'+entry)
            item = dict(id=rec['id'], group=group, entry=entry, kind=rec['kind'], bytes=len(b), sha256=sha(b),
                        source_container=rec['container'], usages=usage(group, entry))
            original = ROOT / 'assets/original' / group / entry
            if original.is_file() and original.read_bytes() == b:
                item.update(storage='alias', file='../original/'+group+'/'+entry)
            else:
                name = 'original/'+group+'/'+entry
                payloads[name] = b
                item.update(storage='published', file=name)
            if rec['kind'] == 'image':
                info = ns['image_info'](b)
                need(info and info.get('rgba_sha256') == rec['rgba_sha256'], 'PNG像素核验失败')
                item.update({k: info[k] for k in ('width', 'height', 'rgba_sha256', 'validation')})
            elif rec['kind'] == 'SEB':
                frames, refs = seb_refs(b)
                directory = next(r for r in records if r.get('source') == 'EXE' and r.get('group') == group and r.get('entry') == 'img.inf')
                targets = [next(t for t in directory['index_targets'] if t['index'] == i) for i in refs]
                need(all(t['record'] in selected_ids for t in targets), 'SEB引用缺少白名单图片')
                item.update(frames=frames, image_refs=targets)
            result.append(item)
    need(len(payloads) == 17 and sum(i['storage'] == 'alias' for i in result) == 21, '发布／别名规模变化')
    manifest = dict(format='ark-research-steam-startup-1', source=dict(container='DungeonVillageEXE/KairoGames_Data/resources.assets',
                    sha256=CONTAINER_HASH, objects=inputs), logical_count=38, new_payload_count=17, alias_count=21,
                    total_logical_bytes=sum(i['bytes'] for i in result), new_payload_bytes=sum(map(len, payloads.values())),
                    scope='Steam固定启动图块与选档；非完整标题人物／字体／音频／通关皮肤',
                    path_base='file相对本MANIFEST所在目录；alias须核同一SHA，不允许运行时读取原容器或work',
                    directory_contract='INF保留完整原目录，但当前仅发布records显式列出的项；禁止默认整组加载。七SEB正图片引用完整。',
                    language_contract='保留原语言路径；不指定未核的中文运行时fallback。相同hash语言条目保留各自来源及独立逻辑用途。',
                    contracts=['../../ui/STEAM_TITLE_DRAW.md', '../../ui/STEAM_TITLE_MENU.md', '../../ui/STARTUP_SKIN.md'],
                    records=result)
    payloads['MANIFEST.json'] = (json.dumps(manifest, ensure_ascii=False, indent=2)+'\n').encode('utf-8')
    return payloads, manifest


def verify_existing(directory, payloads, require_all):
    """任何写入前核整棵目标；不接受联接、符号链接或未登记多余文件。"""
    directory = directory.absolute()
    need(directory.is_relative_to(ROOT) and directory.resolve() == directory, '目标必须为研究根内普通路径')
    if directory.exists():
        need(directory.is_dir(), '目标不是目录')
        for p in directory.rglob('*'):
            need(not p.is_symlink() and p.resolve() == p.absolute(), '目标含符号链接／联接')
            if p.is_file():
                need(p.relative_to(directory).as_posix() in set(payloads) | {'README.md'}, '目标存在未登记文件')
    for name, b in payloads.items():
        target = directory / name
        need(target.resolve() == target.absolute(), '目标路径跳转')
        if target.exists():
            need(target.is_file() and target.read_bytes() == b, '已有内容不同，拒绝覆盖：'+name)
        else:
            need(not require_all, '缺少发布文件：'+name)


def publish(directory, payloads):
    verify_existing(directory, payloads, False)
    created, made_dirs = [], []
    try:
        for name, b in payloads.items():
            target = directory / name
            missing, parent = [], target.parent
            while not parent.exists():
                missing.append(parent)
                parent = parent.parent
            for parent in reversed(missing):
                parent.mkdir()
                made_dirs.append(parent)
            if target.exists():
                need(target.is_file() and target.read_bytes() == b, '并发目标变化：'+name)
                continue
            with target.open('xb') as out:
                created.append((target, b))
                out.write(b)
                out.flush()
                os.fsync(out.fileno())
        verify_existing(directory, payloads, True)
    except BaseException:
        for target, b in reversed(created):
            if target.is_file() and not target.is_symlink() and target.read_bytes() == b:
                target.unlink()
        for parent in reversed(made_dirs):
            if parent.is_dir() and not any(parent.iterdir()):
                parent.rmdir()
        raise
    return len(created)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    payloads, manifest = planned_payloads()
    directory = ROOT / 'assets/steam-startup'
    if args.check:
        verify_existing(directory, payloads, True)
        created = 0
    else:
        created = publish(directory, payloads)
    print(json.dumps(dict(mode='check' if args.check else 'publish', logical_count=38, new_payload_count=17,
                         aliases=21, created=created, new_payload_bytes=manifest['new_payload_bytes'],
                         manifest_sha256=sha(payloads['MANIFEST.json'])), ensure_ascii=False))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, KeyError, StopIteration, struct.error) as e:
        print('Steam启动素材发布拒绝：'+str(e), file=sys.stderr)
        raise SystemExit(1)
