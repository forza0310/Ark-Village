"""主动应用首批交付审计；当前证书与初验草稿分开，不覆盖旧轨迹身份。"""
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
sha=lambda b:hashlib.sha256(b).hexdigest()
def text(p):
    b=archive_input(p).read_bytes()
    return b.decode('utf-16' if b.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
for frame in [420,2000]:
    source=ROOT/f'work/snapshots/active-application-v1/frame{frame}.avra'
    cert=source.with_suffix('.avra.json'); c=json.loads(text(cert)); b=archive_input(source).read_bytes()
    assert c['controller']=='application-active-progression-v1' and c['qualification']=='active_application_management_tail'
    assert c['snapshot_sha256']==sha(b) and c['snapshot_bytes']==len(b)
    assert c['capture_frame']==frame and c['stop_at']==frame+20 and c['process_count']==3 and c['tail_frames']==20
    assert c['terminal']['terminal'] is False and c['terminal']['rank']==0 and int.from_bytes(b[12:16],'little')==7
    if c['source_prefix']:
        s=c['source_prefix']
        for key,digest in [('file','snapshot_sha256'),('certificate_file','certificate_sha256')]:
            p=Path(s[key]).resolve(); assert p.is_relative_to(ROOT/'work') and sha(archive_input(p).read_bytes())==s[digest]
    (archive_output(BASE/f'FRAME{frame}.json')).write_bytes(archive_input(cert).read_bytes())
assert 'startup_world_replay_process ...   Passed' in text(BASE/'ctest-final.log')
assert '100% tests passed out of 1' in text(BASE/'ctest-persistence-fixed.log')
files=['prototype/CMakeLists.txt','prototype/tests/startup_world_persistence_test.cpp',
       'prototype/tests/startup_application_active_replay.hpp','prototype/tests/startup_application_active_replay.cpp',
       'prototype/tests/application_active_process.mjs','prototype/tests/application_process_support.mjs',
       'prototype/tests/application_natural_process.mjs','prototype/tests/replay_file_test.mjs',
       'prototype/tests/APPLICATION_PROCESS.md','prototype/APPLICATION_REPLAY.md',
       'stages/README.md','stages/COMPREHENSIVE_RESEARCH.md','VERIFICATION.md','ui/README.md',
       'work/active-progression-plan/ACTIVE_DRIVER.md',
       'work/window-restore-observation/NEXT_BUILD_LIST_INPUT_PROMPT.md']
files+=['work/application-active-delivery/'+name for name in
        ['README.md','audit.py','build.log','build-final.log','build-check-directory.log',
         'ctest-first.log','ctest-final.log','ctest-persistence-fixed.log',
         'frame420.log','frame420-final.log','frame2000.log','FRAME420.json','FRAME2000.json']]
records=[]; links=0
for name in files:
    p=ROOT/name; b=archive_input(p).read_bytes(); t=text(p); assert '\ufffd' not in t,name
    records.append({'path':name,'bytes':len(b),'sha256':sha(b),'utf8_lf_sha256':sha(t.replace('\r\n','\n').encode())})
    if p.suffix=='.mjs':subprocess.run(['node','--check',str(archive_input(p))],check=True)
    if p.suffix=='.md':
        for ref in re.findall(r'\]\(([^)]+)\)',t):
            if ref.startswith(('http:','https:','mailto:','#')):continue
            target=re.sub(r':\d+$','',unquote(ref.split('#')[0].strip('<>')))
            q=(p.parent/target).resolve()
            assert archive_input(q).exists() or q==BASE/'VALIDATION.json',(name,ref)
            links+=1
release=[p for p in (ROOT/'work/release').rglob('*') if archive_input(p).is_file()]
result={'baseline':'7bc85f0','files':records,'links':links,'world_semantics':4,'application_semantics':7,
        'controller':'application-active-progression-v1','new_targets':0,'new_ctests':0,'new_assets':0,
        'tests':{'replay_process_seconds':54.24,'persistence_final_seconds':31.04,
                 'intermediate_failure':'固定测试目录重跑冲突；旧现场保留，独占目录修后通过'},
        'release_bytes':sum(archive_input(p).stat().st_size for p in release),'release_files':len(release),
        'processes':'本批构建/CTest/两段三进程均已退出；无新增AST缓存',
        'limits':['0星/未完成活动，短前缀不是首星或通关','52→53生命周期夹具不是自然活动认证',
                  '双文件证书发布非原子，崩溃仍可能留候选','合法历史增长与128MiB预算保持',
                  '原窗口提示词待外部结果，产品未改']}
(archive_output(BASE/'VALIDATION.json')).write_bytes((json.dumps(result,ensure_ascii=False,indent=2)+'\n').encode())
files.append('work/application-active-delivery/VALIDATION.json')
(archive_output(BASE/'FILES.json')).write_bytes((json.dumps(files,ensure_ascii=False,indent=2)+'\n').encode())
files.append('work/application-active-delivery/FILES.json')
(archive_output(BASE/'historical-paths.nul')).write_bytes(b'\0'.join(('research/dungeon_village_1/'+p).encode() for p in files)+b'\0')
print(json.dumps({'files':len(files),'links':links,'bytes':sum(r['bytes'] for r in records),
                  'release_bytes':result['release_bytes']}))
