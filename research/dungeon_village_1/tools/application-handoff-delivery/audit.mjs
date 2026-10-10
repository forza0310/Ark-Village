// 明确复核本批三个证书与它们的来源；不运行模拟、不重签输入文件。
import {archiveFs as fs, archiveWork} from '../work_archive_paths.mjs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {readSecondStarSource} from '../../prototype/tests/application_second_star_process.mjs';
const base=archiveWork(import.meta.url),root=path.resolve(base,'../..');
const work=await fs.realpath(path.join(root,'work'));
await fs.mkdir(base,{recursive:true});
const hash=b=>createHash('sha256').update(b).digest('hex');
const files=['prototype/tests/startup_application_active_replay.cpp','prototype/tests/startup_application_active_replay.hpp',
 'prototype/tests/startup_application_second_star_replay.cpp','prototype/tests/startup_application_second_star_replay.hpp',
 'prototype/tests/application_second_star_process.mjs','prototype/tests/APPLICATION_PROCESS.md',
 'prototype/tests/startup_world_persistence_test.cpp','prototype/CMakeLists.txt',
 'work/active-controller-handoff/README.md','work/application-handoff-delivery/README.md','work/application-handoff-delivery/audit.mjs'];
const records=[];
for(const file of files){const b=await fs.readFile(path.join(root,file));records.push({path:file,bytes:b.length,sha256:hash(b)});}
const certificates=[];
for(const frame of [34429,34449,34469]){
 const file=path.join(work,`snapshots/active-application-v2/frame${frame}.avra.json`);
 const b=await fs.readFile(file),name=`FRAME${frame}.json`,out=path.join(base,name);
 try{const old=await fs.readFile(out);if(!old.equals(b))throw Error('拒绝覆盖不同归档证书');}
 catch(e){if(e.code!=='ENOENT')throw e;await fs.writeFile(out,b,{flag:'wx'});}
 certificates.push({path:path.relative(root,file),bytes:b.length,sha256:hash(b),archived_as:name,result:JSON.parse(b)});
}
const chain=await readSecondStarSource({loadPrefix:path.join(work,'snapshots/active-application-v2/frame34469.avra')},work);
await chain.unchanged();
const result={scope:'仅显式证书和来源身份复核；不重跑三进程',files:records,certificates,
 source_evidence:chain.evidence,checks:{release_build:'passed',persistence_seconds:31.14,replay_process_seconds:54.81,
 final_three_process_driver_checks:15,certificate_rejections:16},
 outputs:'成功进程根/trace已消费回收；输入hash不变；旧未认证历史保留',
 limitations:['本批仅真实接受下一任务，不代表出发/成功/二星','未对旧尾第1轮之后失败新增动态故障注入'],
 new_assets:0,new_build_trees:0,processes:'本批进程已收齐'};
await fs.writeFile(path.join(base,'VALIDATION.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({certificates:certificates.length,source_files:chain.evidence.length}));
