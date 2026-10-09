"""已核两图与装载合同的交付白名单；不构建、不读取玩家档。"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
from urllib.parse import unquote
ROOT = Path(__file__).resolve().parents[2]
BASE = Path(__file__).resolve().parent
before = (BASE/'EVIDENCE.json').read_bytes()
subprocess.run(['node', str(BASE/'audit.cjs')], check=True)
assert (BASE/'EVIDENCE.json').read_bytes() == before
files = ['assets/steam-build-common/'+p for p in ['README.md', 'MANIFEST.json', '.gitattributes',
          'original/common/tenant_resident.png', 'original/common/number05.png']]
files += ['tools/scripts/publish_steam_build_common.py', 'ui/STEAM_BUILD_RESOURCE_INSTALL.md',
          'ui/README.md', 'ui/STEAM_BUILD_LIST.md', 'ui/STEAM_UI_COVERAGE.md',
          'verification/PRODUCT_REQUESTS.md', 'VERIFICATION.md']
files += ['work/steam-build-resource-install/'+p for p in
          ['README.md', 'inspect.cjs', 'audit.cjs', 'EVIDENCE.json', 'verify_delivery.py']]
records, links = [], 0
for name in files:
    p=ROOT/name; b=p.read_bytes()
    row={'path':name,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
    if p.suffix != '.png':
        t=b.decode('utf-8-sig'); assert '\ufffd' not in t
        row['utf8_lf_sha256']=hashlib.sha256(t.replace('\r\n','\n').encode()).hexdigest()
        if p.suffix=='.md':
            for ref in re.findall(r'\]\(([^)]+)\)',t):
                if ref.startswith(('http:','https:','mailto:','#')): continue
                target=re.sub(r':\d+$','',unquote(ref.split('#')[0].strip('<>')))
                assert (p.parent/target).resolve().exists(), (name,ref)
                links+=1
    if p.suffix=='.cjs': subprocess.run(['node','--check',str(p)],check=True)
    records.append(row)
result={'baseline':'01f5996','files':records,'links':links,'new_image_bytes':1213,
        'logical_resources':4,'alias_count':2,'builds':0,'background_processes':0,
        'limits':['全语言候选顺序和GPU退休未闭合','原窗口和列表Init/Update仍单列',
                  '46定义双向有效不等于当前全部解锁']}
(BASE/'VALIDATION.json').write_bytes((json.dumps(result,ensure_ascii=False,indent=2)+'\n').encode())
files+=['work/steam-build-resource-install/VALIDATION.json']
(BASE/'FILES.json').write_bytes((json.dumps(files,ensure_ascii=False,indent=2)+'\n').encode())
files+=['work/steam-build-resource-install/FILES.json']
(BASE/'git-paths.nul').write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in files)+b'\0')
print(json.dumps({'files':len(files),'links':links,'bytes':sum(r['bytes'] for r in records)}))
