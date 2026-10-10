"""X4交付审计，仅核已用来源与已完成检查，不重跑游戏。"""
# 读取按现存工具／归档位置解析历史路径；本轮候选输出只留在本地work。
import sys as _archive_sys
from pathlib import Path as _ArchivePath
_archive_sys.dont_write_bytecode = True
_archive_sys.path.insert(0, str(_ArchivePath(__file__).resolve().parents[1]))
from work_archive_paths import archive_input, archive_work, archive_output
# historical-paths.nul仅保留旧路径的历史身份参考，禁止用于Git暂存。
from pathlib import Path
import hashlib
import json
import re
from urllib.parse import unquote
ROOT=Path(__file__).resolve().parents[2]; BASE=archive_work(__file__)
def text(p):
    b=archive_input(p).read_bytes();return b.decode('utf-16' if b.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
sha=lambda b:hashlib.sha256(b).hexdigest()
assert '100% tests passed out of 1' in text(BASE/'ctest.log')
assert '0.61 sec' in text(BASE/'ctest.log')
assets=[]
for name,digest in [('eff_coin.png','64809d749638a027339c2fccbf712c937e5744e3de54f5bb9ec3fd8ae5aaa8ae'),
                    ('eff_coin.seb','7a1c24539389ceabb23e875255e5a2344e5616e52a91faccde3a1de350b084d4')]:
    p=ROOT/'assets/original/common'/name;b=archive_input(p).read_bytes();assert sha(b)==digest
    assets.append({'path':p.relative_to(ROOT).as_posix(),'bytes':len(b),'sha256':digest})
files=['prototype/include/dungeon_village_prototype/startup_world_visuals.hpp',
       'prototype/src/startup_world_visuals.cpp','prototype/tests/startup_world_visuals_test.cpp',
       'ui/COMBAT_RENDER.md','ui/README.md','verification/PRODUCT_REQUESTS.md','VERIFICATION.md',
       'work/ui-next-consumers/README.md']
files+=['work/coin-effect-delivery/'+n for n in ['README.md','audit.py','build.log','ctest.log']]
records=[];links=0
for name in files:
    p=ROOT/name;b=archive_input(p).read_bytes();t=text(p);assert '\ufffd' not in t
    records.append({'path':name,'bytes':len(b),'sha256':sha(b),'lf_sha256':sha(t.replace('\r\n','\n').encode())})
    if p.suffix=='.md':
        for ref in re.findall(r'\]\(([^)]+)\)',t):
            if ref.startswith(('http:','https:','mailto:','#')):continue
            target=re.sub(r':\d+$','',unquote(ref.split('#')[0].strip('<>')));q=(p.parent/target).resolve()
            assert archive_input(q).exists() or q==BASE/'VALIDATION.json',(name,ref)
            links+=1
result={'baseline':'87023c3','files':records,'links':links,'reused_assets':assets,'new_asset_bytes':0,
        'tests':1,'seconds':0.61,'owner_or_schema_changed':False,'background_processes':0,
        'limits':['仅固定APK局部合同','全场景层序/Steam具体消费者/原动态另验']}
(archive_output(BASE/'VALIDATION.json')).write_bytes((json.dumps(result,ensure_ascii=False,indent=2)+'\n').encode())
files.append('work/coin-effect-delivery/VALIDATION.json')
(archive_output(BASE/'FILES.json')).write_bytes((json.dumps(files,ensure_ascii=False,indent=2)+'\n').encode())
files.append('work/coin-effect-delivery/FILES.json')
(archive_output(BASE/'historical-paths.nul')).write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in files)+b'\0')
print(json.dumps({'files':len(files),'links':links,'reused_asset_bytes':sum(a['bytes'] for a in assets)}))
