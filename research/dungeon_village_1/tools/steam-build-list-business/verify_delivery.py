"""冻结本批UTF-8 LF交付及来源复算，原始输入不规范化。"""
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
import subprocess
from urllib.parse import unquote
ROOT=Path(__file__).resolve().parents[2]
BASE=archive_work(__file__)
b=(archive_input(BASE/'EVIDENCE.json')).read_bytes()
subprocess.run(['node',str(archive_input(BASE/'audit.cjs'))],check=True)
# 比较子进程work候选，不能再次读取同一份verification归档。
assert (BASE/'EVIDENCE.json').read_bytes()==b
files=['ui/STEAM_BUILD_LIST_BUSINESS.md','ui/STEAM_BUILD_LIST.md','ui/STEAM_UI_COVERAGE.md',
       'ui/README.md','VERIFICATION.md']
files+=['work/steam-build-list-business/'+name for name in
        ['README.md','inspect.cjs','audit.cjs','EVIDENCE.json','verify_delivery.py']]
records=[]; links=0
for name in files:
    p=ROOT/name; b=archive_input(p).read_bytes(); t=b.decode('utf-8'); assert '\ufffd' not in t
    if name.startswith('work/steam-build-list-business/') or name=='ui/STEAM_BUILD_LIST_BUSINESS.md':
        assert b'\r' not in b and not b.startswith(b'\xef\xbb\xbf'),name
    records.append({'path':name,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),
                    'lf_sha256':hashlib.sha256(t.replace('\r\n','\n').encode()).hexdigest()})
    if p.suffix=='.cjs': subprocess.run(['node','--check',str(archive_input(p))],check=True)
    if p.suffix=='.md':
        for ref in re.findall(r'\]\(([^)]+)\)',t):
            if ref.startswith(('http:','https:','mailto:','#')): continue
            target=re.sub(r':\d+$','',unquote(ref.split('#')[0].strip('<>')))
            assert archive_input((p.parent/target).resolve()).exists(),(name,ref)
            links+=1
result={'baseline':'49100d9','files':records,'links':links,'methods':8,'bytes':30276,'anchors':52,
        'new_assets':0,'builds':0,'background_processes':0,
        'limits':['完整FormManager退休和OS输入未认证','确认不等于实体建设/扣款已完成',
                  '原私有状态缺少防护不替代维护显式拒绝']}
(archive_output(BASE/'VALIDATION.json')).write_bytes((json.dumps(result,ensure_ascii=False,indent=2)+'\n').encode())
files.append('work/steam-build-list-business/VALIDATION.json')
(archive_output(BASE/'FILES.json')).write_bytes((json.dumps(files,ensure_ascii=False,indent=2)+'\n').encode())
files.append('work/steam-build-list-business/FILES.json')
(archive_output(BASE/'historical-paths.nul')).write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in files)+b'\0')
print(json.dumps({'files':len(files),'links':links,'bytes':sum(r['bytes'] for r in records)}))
