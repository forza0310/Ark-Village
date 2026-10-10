"""只读校验外部窗口反馈；原图和私人文本不复制进交付。"""
from pathlib import Path
import hashlib
import json
import struct

BASE = Path(__file__).resolve().parent
ROOT = BASE.parent.parent
SOURCE = ROOT / 'work/window-restore-observation/20261010-075500-skin-observation'


def jpeg_size(data):
    if data[:2] != b'\xff\xd8':
        raise ValueError('不是JPEG原字节')
    at = 2
    while at < len(data):
        if data[at] != 255:
            raise ValueError('JPEG标记缺失')
        while data[at] == 255:
            at += 1
        marker = data[at]
        at += 1
        if marker in [0xD8, 0xD9] or 0xD0 <= marker <= 0xD7:
            continue
        size = struct.unpack_from('>H', data, at)[0]
        if marker in [0xC0, 0xC1, 0xC2]:
            height, width = struct.unpack_from('>HH', data, at + 3)
            return width, height
        at += size
    raise ValueError('JPEG尺寸缺失')


raw = (SOURCE / 'EVIDENCE.json').read_bytes()
manifest = json.loads(raw)
records = []
for entry in manifest['files']:
    file = (SOURCE / entry['path']).resolve()
    if not file.is_relative_to(SOURCE.resolve()):
        raise ValueError('清单路径越界')
    data = file.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if len(data) != entry['bytes'] or digest != entry['sha256']:
        raise ValueError('源清单失配 ' + entry['path'])
    record = {key: entry[key] for key in ['path', 'bytes', 'sha256', 'actual_format']}
    if entry['actual_format'] == 'JPEG':
        width, height = jpeg_size(data)
        if (width, height) != (entry['width'], entry['height']):
            raise ValueError('原图尺寸失配')
        record.update(width=width, height=height)
    records.append(record)
images = [r for r in records if r['actual_format'] == 'JPEG']
actual = {p.relative_to(SOURCE).as_posix() for p in SOURCE.rglob('*') if p.is_file()}
if actual != {r['path'] for r in records} | {'EVIDENCE.json'}:
    raise ValueError('未登记或缺失文件')
if len(images) != 11 or sum(r['bytes'] for r in images) != 2476775:
    raise ValueError('图片规模失配')
result = dict(source=SOURCE.relative_to(ROOT).as_posix(), source_manifest_sha256=hashlib.sha256(raw).hexdigest(),
              files=records, manifested_files=len(records), original_images=len(images),
              original_image_bytes=sum(r['bytes'] for r in images), coverage=manifest['coverage'],
              inspected_by_main=['A01', 'A02', 'B06', 'Z01'],
              archive_policy='原图含村名，只留原本地work；本包不复制图片、不新增正式S编号',
              process_status='仅接收历史PID5348交还用户的记录，未查当前进程、未发送窗口输入或结束它',
              limitations=['EXE路径/可见2.56有报告；本次未提供DLL/metadata hash，不认证与固定样本字节相同',
                           '有限JPEG不认证字体文件/精确RGB/逻辑坐标/随机流/内部暂停',
                           '经营三类详情和计分未覆盖；未读写原档'])
(BASE / 'VALIDATION.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
print(json.dumps(dict(files=len(records), images=len(images), bytes=result['original_image_bytes'])))
