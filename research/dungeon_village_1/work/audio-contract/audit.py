"""固定APK音频ID到原ZIP条目，只读校验及Ogg/Vorbis头读取；不导出或播放音频。"""
from pathlib import Path
import hashlib, json, struct, zipfile, subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
sha = lambda b: hashlib.sha256(b).hexdigest()
apk = ROOT / 'maoxianmigongcun.apk'
raw = apk.read_bytes()
assert sha(raw) == '1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5'
checks = 0


def need(v, message):
    global checks
    if not v:
        raise ValueError(message)
    checks += 1


def ogg_info(data):
    at = 0
    last = None
    pages = 0
    stream = None
    first_packet = bytearray()
    packet_complete = False
    end_flag = False
    while at < len(data):
        need(data[at:at+4] == b'OggS' and at+27 <= len(data), 'Ogg header')
        need(data[at+4] == 0, 'Ogg version')
        count = data[at+26]
        need(at+27+count <= len(data), 'Ogg segment table')
        sizes = data[at+27:at+27+count]
        end = at+27+count+sum(sizes)
        need(end <= len(data), 'Ogg page body')
        serial, seq = struct.unpack_from('<II', data, at+14)
        if stream is None:
            stream = serial
        need(stream == serial and seq == pages, 'single ordered logical stream')
        payload_at = at+27+count
        for size in sizes:
            if not packet_complete:
                first_packet.extend(data[payload_at:payload_at+size])
                packet_complete = size < 255
            payload_at += size
        granule = struct.unpack_from('<Q', data, at+6)[0]
        if granule != 0xffffffffffffffff:
            last = granule
        end_flag = bool(data[at+5] & 4)
        pages += 1
        at = end
    need(end_flag and packet_complete and first_packet[:7] == b'\x01vorbis', 'Vorbis identification and EOS')
    need(len(first_packet) >= 30, 'Vorbis identification size')
    version = struct.unpack_from('<I', first_packet, 7)[0]
    channels = first_packet[11]
    rate = struct.unpack_from('<I', first_packet, 12)[0]
    need(version == 0 and 1 <= channels <= 8 and rate > 0 and last is not None, 'Vorbis format')
    return dict(codec='Vorbis', channels=channels, sample_rate=rate, pages=pages,
                terminal_granule=last, granule_seconds=round(last/rate, 6),
                qualification='container/identification/EOS only; no decode, playback or CRC verification')


with zipfile.ZipFile(apk) as archive:
    snd = archive.read('res/raw/snd.inf')
    local_snd = ROOT/'work/decompiled/resources/res/raw/snd.inf'
    need(local_snd.read_bytes() == snd, 'snd.inf research copy equals APK ZIP')
    lines = [line.strip() for line in snd.decode('utf-8').splitlines() if line.strip()]
    need(len(lines) == 26 and all('\t' not in line and '/' not in line and '\\' not in line for line in lines), 'direct 26-row sound slots')
    records = []
    for slot, name in enumerate(lines):
        normalized = name.replace('.mld', '.ogg')
        need(normalized == normalized.lower() and normalized.endswith('.ogg'), 'actual raw name normalization')
        entry = 'res/raw/'+normalized
        data = archive.read(entry)
        local = ROOT/'work/decompiled/resources'/entry
        need(local.read_bytes() == data, 'audio research copy equals APK ZIP')
        records.append(dict(id=slot,manifest_name=name,entry=entry,local_path=str(local.relative_to(ROOT)).replace('\\','/'),bytes=len(data),sha256=sha(data),channel=0 if slot<4 else 1,loop='completion_restart' if slot<4 else 'none',**ogg_info(data)))
    all_ogg = {x for x in archive.namelist() if x.startswith('res/raw/') and x.endswith('.ogg')}
    need(all_ogg == {r['entry'] for r in records}, 'no unindexed raw Ogg')

windows = [
    ('loader-C-channel-loop','b/a.java',508,523),('resource-group-and-legacy-name-list','d/a.java',118,122),
    ('BGM-wrapper','d/a.java',3436,3446),('SE-wrapper','d/a.java',3787,3792),('jingle-wrapper','d/a.java',3936,3939),
    ('resource-name-conversion','kairo/a/a/a.java',30,32),('resource-manifest-and-id','kairo/android/ui/s.java',8,76),
    ('resource-row-allocation','kairo/android/ui/s.java',87,110),('resource-audio-dispatch','kairo/android/ui/s.java',195,231),
    ('resource-sound-directory','kairo/android/ui/s.java',333,352),('sound-fields-loop','kairo/android/ui/w.java',1,52),
    ('player-ports-and-controls','kairo/android/ui/SoundPlayer.java',9,185),('MediaPlayer-lifecycle','kairo/android/ui/y.java',37,192),
    ('jingle-mute-request','net/kairosoft/android/bouken_ja/Main.java',25,35),('jingle-wait-consumer','net/kairosoft/android/bouken_ja/Main.java',121,138),
    ('foreground-input-gate','kairo/android/a/b.java',179,202),('android-lifecycle','kairo/android/ui/IApplication.java',336,372),
    ('title-volume-bgm','b/h.java',293,299),('settings-volume','b/d.java',192,222),
    ('same-id5-jingle-a','b/g.java',4712,4725),('same-id5-jingle-b','b/g.java',4873,4886),
]
sources = []
for purpose, name, first, last in windows:
    b=(ROOT/'work/decompiled/sources'/name).read_bytes()
    text=b.decode('utf-8').splitlines()
    need(0<first<=last<=len(text), 'source window range '+name)
    sources.append(dict(purpose=purpose,path='work/decompiled/sources/'+name,first=first,last=last,source_sha256=sha(b),window_lf_sha256=sha('\n'.join(text[first-1:last]).encode())))
steam = subprocess.run(['node',str(OUT/'steam-inspect.cjs'),'manifest'],check=True,capture_output=True,text=True)
steam_windows = [json.loads(line) for line in steam.stdout.splitlines()]
result = dict(date='2026-10-09',qualification='APK source + exact ZIP audio identities; Steam limited method cross-check',apk_sha256=sha(raw),snd_inf_sha256=sha(snd),snd_inf_bytes=len(snd),audio=records,sources=sources,steam_method_windows=steam_windows,checks=checks,storage=dict(audio_copies_created=0,audio_bytes_examined=sum(r['bytes'] for r in records),audio_played=0,background_processes=0),limitations=['JADX sound-extension dispatch loop is structurally suspicious; no new DEX confirmation','Steam audio resource bytes/loading/channel/lifecycle not fully certified','No audio assets publication or maintained typed audio request interface','No C++ build or playback/window test'])
(OUT/'EVIDENCE.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(records=len(records),audio_bytes=sum(r['bytes'] for r in records),channels={'BGM':4,'SE':22},checks=checks,steam_windows=len(steam_windows)),ensure_ascii=False))
