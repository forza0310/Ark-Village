"""首次主动任务成果与后继前缀交付审计，失败日志及源证书保持原字节。"""
from pathlib import Path
import hashlib
import json
import re
from urllib.parse import unquote
ROOT=Path(__file__).resolve().parents[2]; BASE=Path(__file__).resolve().parent
sha=lambda b:hashlib.sha256(b).hexdigest()
def text(p):
    b=p.read_bytes(); return b.decode('utf-16' if b.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
for frame,tail in [(7300,160),(10000,20)]:
    source=ROOT/f'work/snapshots/active-application-v1/frame{frame}.avra'; cert=source.with_suffix('.avra.json')
    c=json.loads(text(cert)); b=source.read_bytes()
    assert c['controller']=='application-active-progression-v1' and c['qualification']=='active_application_management_tail'
    assert c['snapshot_sha256']==sha(b) and c['snapshot_bytes']==len(b) and c['process_count']==3
    assert c['capture_frame']==frame and c['tail_frames']==tail and c['stop_at']==frame+tail
    assert c['terminal']['task_successes']==1 and c['terminal']['completed_activities']==2
    assert c['terminal']['rank']==0 and c['terminal']['terminal'] is False
    for key,digest in [('file','snapshot_sha256'),('certificate_file','certificate_sha256')]:
        p=Path(c['source_prefix'][key]).resolve()
        assert p.is_relative_to(ROOT/'work') and sha(p.read_bytes())==c['source_prefix'][digest]
    (BASE/f'FRAME{frame}.json').write_bytes(cert.read_bytes())
assert 'active frame=7348 months=4: active unknown raw=30 page=33' in text(BASE/'frame10000.log')
files=['prototype/tests/startup_application_active_replay.cpp','rules/PROGRESSION_ROUTES.md',
       'stages/README.md','stages/COMPREHENSIVE_RESEARCH.md','VERIFICATION.md',
       'work/active-progression-plan/ACTIVE_DRIVER.md','work/active-second-star-plan/README.md',
       'work/active-second-star-plan/EVIDENCE.json']
files+=['work/active-application-route/'+name for name in
        ['README.md','audit.py','build-task-results.log','frame10000.log','frame7300.log',
         'frame10000-retry.log','FRAME7300.json','FRAME10000.json','application-active-Yrygn6/process-0.log']]
records=[]; links=0
for name in files:
    p=ROOT/name;b=p.read_bytes();t=text(p);assert '\ufffd' not in t and '?'*4 not in t,name
    records.append({'path':name,'bytes':len(b),'sha256':sha(b),'lf_sha256':sha(t.replace('\r\n','\n').encode())})
    if p.suffix=='.md':
        for ref in re.findall(r'\]\(([^)]+)\)',t):
            if ref.startswith(('http:','https:','mailto:','#')):continue
            target=re.sub(r':\d+$','',unquote(ref.split('#')[0].strip('<>')));q=(p.parent/target).resolve()
            assert q.exists() or q==BASE/'VALIDATION.json',(name,ref)
            links+=1
result={'baseline':'e19b97b','files':records,'links':links,'world_semantics':4,'application_semantics':7,
        'schema_changed':False,'new_assets':0,'background_processes':0,
        'limits':['自然首星/BOSS/二星未完成','二星Controller交接仅方案未实现',
                  '合法历史增长不删除，128MiB预算不放宽','departed回执不等同所有自动出发次数']}
(BASE/'VALIDATION.json').write_bytes((json.dumps(result,ensure_ascii=False,indent=2)+'\n').encode())
files.append('work/active-application-route/VALIDATION.json')
(BASE/'FILES.json').write_bytes((json.dumps(files,ensure_ascii=False,indent=2)+'\n').encode())
files.append('work/active-application-route/FILES.json')
(BASE/'git-paths.nul').write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in files)+b'\0')
print(json.dumps({'files':len(files),'links':links,'bytes':sum(r['bytes'] for r in records)}))
