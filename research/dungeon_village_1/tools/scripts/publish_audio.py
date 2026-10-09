"""发布固定APK的26个声音文件；--check只读复核，任何已有异内容目标均拒绝。"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
APK_HASH = '1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5'
sha = lambda data: hashlib.sha256(data).hexdigest()


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def vorbis_properties(data):
    """核Ogg页与Vorbis标识包；只给容器时长，不宣称PCM解码／听音／CRC已验。"""
    at, pages, stream, granule = 0, 0, None, None
    first, complete, eos = bytearray(), False, False
    while at < len(data):
        require(at + 27 <= len(data) and data[at:at+5] == b'OggS\0', 'Ogg页头无效')
        count = data[at+26]
        require(at+27+count <= len(data), 'Ogg分段表越界')
        segments = data[at+27:at+27+count]
        end = at+27+count+sum(segments)
        require(end <= len(data), 'Ogg页数据越界')
        serial, sequence = struct.unpack_from('<II', data, at+14)
        if stream is None:
            stream = serial
        require(serial == stream and sequence == pages, '仅支持已核单逻辑流与连续页号')
        payload = at+27+count
        for size in segments:
            if not complete:
                first.extend(data[payload:payload+size])
                complete = size < 255
            payload += size
        value = struct.unpack_from('<Q', data, at+6)[0]
        if value != 0xffffffffffffffff:
            granule = value
        eos = bool(data[at+5] & 4)
        at, pages = end, pages+1
    require(eos and complete and len(first) >= 30 and first[:7] == b'\x01vorbis', 'Vorbis标识或流尾无效')
    version, channels, rate = struct.unpack_from('<IBI', first, 7)
    require(version == 0 and 1 <= channels <= 8 and rate > 0 and granule is not None, 'Vorbis参数无效')
    return dict(codec='Vorbis', channels=channels, sample_rate=rate, pages=pages,
                terminal_granule=granule, granule_seconds=round(granule/rate, 6),
                verification='Ogg页边界／顺序、Vorbis标识及EOS；未解码PCM、播放或校验Ogg CRC')


def planned_payloads():
    """固定APK及清单决定发布内容，不读取work，不覆盖旧视觉素材。"""
    apk = ROOT/'maoxianmigongcun.apk'
    require(sha(apk.read_bytes()) == APK_HASH, 'APK身份不符')
    payloads, records = {}, []
    with zipfile.ZipFile(apk) as archive:
        names = archive.namelist()
        require(len(names) == len(set(names)), 'APK存在重复ZIP条目')
        source_manifest = archive.read('res/raw/snd.inf')
        rows = source_manifest.decode('utf-8').splitlines()
        require(len(rows) == 26 and all(row == row.strip() and row for row in rows), '实际声音清单须为26个非空无前后空格行')
        for slot, original_name in enumerate(rows):
            name = original_name.replace('.mld', '.ogg')
            require(name == name.lower() and name.endswith('.ogg') and '/' not in name and '\\' not in name and '\t' not in name, '非法声音文件名')
            require(name not in payloads, '重复声音文件名')
            entry = 'res/raw/'+name
            data = archive.read(entry)
            payloads[name] = data
            records.append(dict(id=slot, file=name, manifest_name=original_name,
                                source_zip_entry=entry, bytes=len(data), sha256=sha(data),
                                channel=0 if slot < 4 else 1,
                                channel_name='BGM' if slot < 4 else 'SE',
                                loop='completion_restart' if slot < 4 else 'none',
                                **vorbis_properties(data)))
        require({n for n in names if n.startswith('res/raw/') and n.endswith('.ogg')} == {r['source_zip_entry'] for r in records}, 'APK原始Ogg与26槽不一致')
    manifest = dict(format='ark-research-audio-1', source=dict(apk='maoxianmigongcun.apk', apk_sha256=APK_HASH,
                        index_entry='res/raw/snd.inf', index_bytes=len(source_manifest), index_sha256=sha(source_manifest)),
                    contract='../../ui/AUDIO_REQUESTS.md', count=len(records),
                    total_audio_bytes=sum(r['bytes'] for r in records),
                    scope='固定APK1.0.8原声音字节；独立于761项视觉发布；不等于Steam音频同一或运行消费者完成',
                    operations='通道与循环为初始化属性；BGM替换／普通播放／jingle须另保留真实调用语义',
                    records=records)
    payloads['MANIFEST.json'] = (json.dumps(manifest, ensure_ascii=False, indent=2)+'\n').encode('utf-8')
    return payloads, manifest


def verify_existing(directory, payloads, require_all):
    """先核全部既有目标再写；符号链接／目录联接与不同内容一律拒绝。"""
    if directory.exists():
        require(directory.is_dir() and directory.resolve() == directory.absolute(), '音频目标不是项目内普通目录')
        unexpected = {p.name for p in directory.iterdir()} - set(payloads) - {'README.md'}
        require(not unexpected, '目标有未登记项：'+', '.join(sorted(unexpected)))
    for name, data in payloads.items():
        target = directory/name
        require(not target.is_symlink(), '不允许音频目标符号链接：'+name)
        if target.exists():
            require(target.is_file() and target.resolve() == target.absolute(), '音频目标不是普通文件：'+name)
            require(target.read_bytes() == data, '已有内容不同，拒绝覆盖：'+name)
        else:
            require(not require_all, '缺少已发布文件：'+name)


def publish(directory, payloads):
    verify_existing(directory, payloads, require_all=False)
    created = []
    directory.mkdir(parents=False, exist_ok=True)
    try:
        for name, data in payloads.items():
            target = directory/name
            if target.exists():
                require(target.read_bytes() == data, '并发目标变化：'+name)
                continue
            # 独占创建，绝不以wb覆盖；异常只退休本进程已创建且身份仍一致的文件。
            with target.open('xb') as output:
                created.append((target, data))
                output.write(data)
                output.flush()
                os.fsync(output.fileno())
        verify_existing(directory, payloads, require_all=True)
    except BaseException:
        for target, data in reversed(created):
            if target.is_file() and not target.is_symlink() and target.read_bytes() == data:
                target.unlink()
        raise
    return len(created)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='只读验证已发布清单与26个音频，不创建或修改文件')
    args = parser.parse_args()
    payloads, manifest = planned_payloads()
    directory = ROOT/'assets/audio'
    if args.check:
        verify_existing(directory, payloads, require_all=True)
        created = 0
    else:
        created = publish(directory, payloads)
    print(json.dumps(dict(mode='check' if args.check else 'publish', count=manifest['count'],
                          audio_bytes=manifest['total_audio_bytes'], created=created,
                          manifest_sha256=sha(payloads['MANIFEST.json'])), ensure_ascii=False))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, zipfile.BadZipFile) as error:
        print('音频发布拒绝：'+str(error), file=sys.stderr)
        raise SystemExit(1)
