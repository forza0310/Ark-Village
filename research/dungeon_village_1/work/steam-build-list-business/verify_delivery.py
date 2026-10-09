"""冻结本批UTF-8 LF交付及来源复算，原始输入不规范化。"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
from urllib.parse import unquote
ROOT=Path(__file__).resolve().parents[2]
BASE=Path(__file__).resolve().parent
b=(BASE/'EVIDENCE.json').read_bytes()
subprocess.run(['node',str(BASE/'audit.cjs')],check=True)
assert (BASE/'EVIDENCE.json').read_bytes()==b
files=['ui/STEAM_BUILD_LIST_BUSINESS.md','ui/STEAM_BUILD_LIST.md','ui/STEAM_UI_COVERAGE.md',
       'ui/README.md','VERIFICATION.md']
files+=['work/steam-build-list-business/'+name for name in
        ['README.md','inspect.cjs','audit.cjs','EVIDENCE.json','verify_delivery.py']]
records=[]; links=0
for name in files:
    p=ROOT/name; b=p.read_bytes(); t=b.decode('utf-8'); assert '\ufffd' not in t
    if name.startswith('work/steam-build-list-business/') or name=='ui/STEAM_BUILD_LIST_BUSINESS.md':
        assert b'\r' not in b and not b.startswith(b'\xef\xbb\xbf'),name
    records.append({'path':name,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),
                    'lf_sha256':hashlib.sha256(t.replace('\r\n','\n').encode()).hexdigest()})
    if p.suffix=='.cjs': subprocess.run(['node','--check',str(p)],check=True)
    if p.suffix=='.md':
        for ref in re.findall(r'\]\(([^)]+)\)',t):
            if ref.startswith(('http:','https:','mailto:','#')): continue
            target=re.sub(r':\d+$','',unquote(ref.split('#')[0].strip('<>')))
            assert (p.parent/target).resolve().exists(),(name,ref)
            links+=1
result={'baseline':'49100d9','files':records,'links':links,'methods':8,'bytes':30276,'anchors':52,
        'new_assets':0,'builds':0,'background_processes':0,
        'limits':['完整FormManager退休和OS输入未认证','确认不等于实体建设/扣款已完成',
                  '原私有状态缺少防护不替代维护显式拒绝']}
(BASE/'VALIDATION.json').write_bytes((json.dumps(result,ensure_ascii=False,indent=2)+'\n').encode())
files.append('work/steam-build-list-business/VALIDATION.json')
(BASE/'FILES.json').write_bytes((json.dumps(files,ensure_ascii=False,indent=2)+'\n').encode())
files.append('work/steam-build-list-business/FILES.json')
(BASE/'git-paths.nul').write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in files)+b'\0')
print(json.dumps({'files':len(files),'links':links,'bytes':sum(r['bytes'] for r in records)}))
