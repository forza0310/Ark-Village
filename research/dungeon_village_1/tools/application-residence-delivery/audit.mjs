// 只读复核已产生的住宅证书；隔离副本变异只测试证书拒绝，不修改来源或重跑模拟。
import {archiveFs as fs, archiveWork} from '../work_archive_paths.mjs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {readResidenceSource} from '../../prototype/tests/application_residence_process.mjs';
const base=archiveWork(import.meta.url),root=path.resolve(base,'../..');
const work=await fs.realpath(path.join(root,'work')),snap=path.join(work,'snapshots/active-residence-v1');
await fs.mkdir(base,{recursive:true});
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(ok,why)=>{if(!ok)throw Error(why);};
const records=[];
const sources=new Map();
for(const name of ['frame34786-bound','frame35000','frame38820-bound']){
 const file=path.join(snap,name+'.avra'),checked=await readResidenceSource({loadPrefix:file},work);
 const raw=await fs.readFile(file+'.json'),c=JSON.parse(raw),out=path.join(base,name+'.json');
 try{need((await fs.readFile(out)).equals(raw),'不能覆盖不同归档证书');}
 catch(e){if(e.code!=='ENOENT')throw e;await fs.writeFile(out,raw,{flag:'wx'});}
 await checked.unchanged();for(const e of checked.evidence)sources.set(e.file,e);
 records.push({name,certificate_sha256:hash(raw),result:c});
}
const gift=records[0].result,first=records[1].result,last=records[2].result;
need(gift.terminal.capture_gift_pending===true&&gift.tail_receipts.gifts_committed===2,'真实待付款尾段未闭合');
need(first.tail_receipts.completed_houses===1&&first.terminal.milestones.first_home_frame===35202,'首住宅真实完工缺失');
need(last.terminal.terminal===true&&last.terminal.completed_houses===4&&
 last.terminal.completed_business_facilities===10&&last.terminal.task_successes===7,'最终四宅十店/任务结局缺失');
const mutations=[
 ['qualification',c=>c.qualification='wrong'],['process_count',c=>c.process_count=2],
 ['seed',c=>c.seed=2],['snapshot_hash',c=>c.snapshot_sha256='0'.repeat(64)],
 ['capture_frame',c=>++c.capture_frame],['producer',c=>c.producer_revision='wrong'],
 ['dataset',c=>c.dataset='0'.repeat(64)],['section_bytes',c=>c.section_bytes=[]],
 ['source_certificate',c=>c.source_prefix.certificate_sha256='0'.repeat(64)],
 ['origin',c=>++c.handoff.origin_metadata.next_command],
 ['completed_count',c=>++c.terminal.completed_houses],
 ['duplicate_home',c=>c.terminal.homes[1].human=c.terminal.homes[0].human],
 ['future_home',c=>c.terminal.homes[0].completed_frame=c.stop_at+1],
 ['capture_receipt',c=>++c.capture_receipts.gifts_committed],
 ['tail_receipt',c=>++c.tail_receipts.homes_admitted],
 ['tail_commands',c=>++c.tail_active_command_count],
 ['cumulative_commands',c=>++c.terminal.active_command_count],
 ['terminal_milestone',c=>++c.milestones.first_home_frame],
 ['history_limit',c=>c.uncertified_history={}],
 ['summary_capture',c=>++c.terminal.capture_receipts.completed_houses]
];
const temporary=await fs.mkdtemp(path.join(work,'residence-certificate-audit-'));
let complete=false;
try{
 const copy=path.join(temporary,'candidate.avra');
 await fs.copyFile(path.join(snap,'frame38820-bound.avra'),copy);
 for(const [name,mutate] of mutations){
  const c=structuredClone(last);mutate(c);await fs.writeFile(copy+'.json',JSON.stringify(c)+'\n');
  let rejected=false;try{await readResidenceSource({loadPrefix:copy},work);}catch{rejected=true;}
  need(rejected,'证书变异未拒绝 '+name);
 }
 complete=true;
}finally{
 if(complete){need(path.dirname(temporary)===work,'临时根越界');await fs.rm(temporary,{recursive:true});}
}
const files=['prototype/tests/startup_application_residence_replay.cpp','prototype/tests/startup_application_residence_replay.hpp',
 'prototype/tests/application_residence_process.mjs','prototype/tests/application_progression_evidence.mjs',
 'prototype/tests/startup_application_second_star_replay.cpp','prototype/tests/startup_application_second_star_replay.hpp',
 'prototype/tests/application_second_star_process.mjs','prototype/tests/APPLICATION_PROCESS.md',
 'prototype/tests/startup_world_persistence_test.cpp','prototype/CMakeLists.txt',
 'work/second-star-residence-plan/README.md','work/application-residence-delivery/README.md','work/application-residence-delivery/audit.mjs'];
const identities=[];for(const file of files){const b=await fs.readFile(path.join(root,file));identities.push({path:file,bytes:b.length,sha256:hash(b)});}
const logs={};for(const name of ['ctest-final.log','ctest-bound.log','gift-bound60.log','ready-bound20.log']){
 const b=await fs.readFile(path.join(work,'application-residence-replay',name));
 const text=b[0]===0xff&&b[1]===0xfe?b.subarray(2).toString('utf16le'):b.toString('utf8');
 if(name.startsWith('ctest'))need(text.includes('100% tests passed'),'检查未通过 '+name);
 logs[name]={bytes:b.length,sha256:hash(b)};
}
const resourceNames=['live_actors','retired_actors','facilities','pages','page_payloads','effects','live_encounters',
 'retired_encounters','retained_tasks','continuations','checkpoints','cash_entries'];
const sizes={};for(const name of ['release','snapshots/active-residence-v1']){
 let count=0,bytes=0;const scan=async d=>{for(const e of await fs.readdir(d,{withFileTypes:true})){
  const p=path.join(d,e.name);if(e.isDirectory())await scan(p);else if(e.isFile()){++count;bytes+=(await fs.stat(p)).size;}
 }};await scan(path.join(work,name));sizes[name]={files:count,bytes};
}
for(const e of sources.values()){const b=await fs.readFile(e.file);need(b.length===e.bytes&&hash(b)===e.sha256,'来源最终改变');}
const report={scope:'住宅有限策略及三路短尾认证，不认证原游戏动态或二星',files:identities,
 certificates:records.map(({name,certificate_sha256,result:c})=>({name,certificate_sha256,capture_frame:c.capture_frame,
 stop_at:c.stop_at,snapshot_bytes:c.snapshot_bytes,snapshot_sha256:c.snapshot_sha256,process_wall_seconds:c.process_wall_seconds,
 tail_receipts:c.tail_receipts,terminal:c.terminal.terminal})),
 source_evidence:[...sources.values()],certificate_mutations_rejected:mutations.map(x=>x[0]),logs,sizes,
 resources:Object.fromEntries(resourceNames.map((n,i)=>[n,{current:last.terminal.resources[i],peak:last.terminal.peaks[i]}])),
 gift_entry_checks:{normal:13,pending:25,extra_pending_rejections:12,initialized65_parent_child_rejections:2},
 outputs:'成功三路临时目录/trace消费后回收；原失败现场、来源证书及有引用快照保留',
 limitations:['20000轮候选之前历史未认证','现金/任务/完整审计历史可合法增长，不宣称永久有界',
 '待付款回放诊断失败及加固前证书保留，旧证书不重签','二星800人气/12成功、活动30、自然通关仍待后续'],
 new_assets:0,new_build_trees:0};
await fs.writeFile(path.join(base,'VALIDATION.json'),JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify({certificates:records.length,source_files:sources.size,rejections:mutations.length,sizes}));
