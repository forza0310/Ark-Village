const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 本批交付只读审计：来源检查由三个具名专题脚本完成；此处核输出、快照证书、链接和资源规模。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),cp=archivePaths.require('child_process');
const root=path.resolve(archivePaths.workDir,'../..'),repo=path.resolve(root,'../..');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
let checks=0;const need=(v,m)=>{++checks;if(!v)throw Error(m);};
const git=args=>cp.execFileSync('git',['-c','safe.directory=D:/code/Ark-Village',...args],{cwd:repo,encoding:'utf8'}).trim();
const read=p=>fs.readFileSync(path.join(root,p));
const inventory=directory=>{
  let files=0,bytes=0,pending=[];
  for(const e of fs.readdirSync(directory,{withFileTypes:true})){
    const p=path.join(directory,e.name);
    if(e.isDirectory()){const c=inventory(p);files+=c.files;bytes+=c.bytes;pending.push(...c.pending);}
    else {files++;bytes+=fs.statSync(p).size;if(/\.pending$|\.tmp\./.test(e.name))pending.push(path.relative(root,p));}
  }
  return {files,bytes,pending};
};
const certificate=JSON.parse(read('work/snapshots/application-clear-v1/CERTIFICATE.json'));
need(certificate.qualification==='conditional_raw17_application_replay','条件资格不得升级自然通关');
need(certificate.captures.length===4,'四个具名边界');
for(const entry of certificate.captures){
  const p='work/snapshots/application-clear-v1/'+entry.boundary+'.avra';
  const bytes=read(p);
  need(bytes.length===entry.snapshot_bytes&&hash(bytes)===entry.snapshot_sha256,'冻结快照身份：'+p);
  need(bytes.subarray(0,8).toString()==='AVRAPP01','应用容器身份');
  need(bytes.subarray(-64).toString()===hash(bytes.subarray(0,-64)),'整体容器摘要');
  need(entry.process_count===3&&entry.tail_frames>0,'三路有效尾段证书');
}
need(!git(['diff','HEAD','--','research/dungeon_village_1/data','research/dungeon_village_1/assets/original',
  'research/dungeon_village_1/prototype/src/startup_world_codec_fields.inc',
  'research/dungeon_village_1/prototype/src/startup_world_codec_fields.json']), '原表/原图/world schema不得修改');
const manifest=read('prototype/src/startup_application_replay_fields.json');
const schema=hash(manifest),fields=JSON.parse(manifest).fields;
need(fields.length===15 && new Set(fields.map(f=>f.name)).size===15,'应用15直接成员全部分类');
need(read('prototype/src/startup_application_replay_fields.inc').toString().includes(schema),'应用schema与规范清单一致');
const outputPaths=[];
for(const dir of ['work/application-replay-delivery','work/startup-frame-font-delivery'])
  for(const name of fs.readdirSync(path.join(root,dir)))
    if(/\.log$|\.png$/.test(name))outputPaths.push(dir+'/'+name);
const outputs=outputPaths.map(p=>{const b=read(p);return {path:p,bytes:b.length,sha256:hash(b)};});
const png=read('work/startup-frame-font-delivery/frame-pieces.png');
need(png.readUInt32BE(16)===760&&png.readUInt32BE(20)===930,'CPU窗框图尺寸');
const final=read('work/application-replay-delivery/ctest-final.log').toString();
need(final.includes('100% tests passed out of 7'),'最终7项CTest通过');
const text=read('work/application-replay-delivery/replay-final.log').toString();
const begin=text.indexOf('{\n  "scenario":');need(begin>=0,'实际进程runner证书输出');
const replay=JSON.parse(text.slice(begin));
need(replay.application_replay.captures.length===4,'最终runner含四个应用认证点');
need(replay.application_replay.terminal_output===certificate.terminal_output,'归档与实际runner终点一致');
const ignoredDocs=['work/application-replay-delivery/README.md','work/startup-frame-font-delivery/README.md',
  'work/startup-frame-font-research/README.md','work/steam-startup-resource-map/README.md',
  'work/title-schedule-handoff/README.md','work/window-restore-observation/README.md',
  'work/window-restore-observation/NEXT_SKIN_OBSERVATION_PROMPT.md',
  'work/window-restore-observation/SKIN_OBSERVATION_FEEDBACK_TEMPLATE.md'];
const changed=[...git(['diff','HEAD','--name-only','--','research']).split('\n'),
  ...git(['ls-files','--others','--exclude-standard','--','research']).split('\n')]
  .filter(p=>p.endsWith('.md')).map(p=>path.join(repo,p));
const docs=[...new Set([...changed,...ignoredDocs.map(p=>path.join(root,p))])];
let links=0;const missing=[];
const self=path.join(archivePaths.workDir,'VALIDATION.json');
for(const doc of docs)for(const m of fs.readFileSync(doc,'utf8').matchAll(/\[[^\]\r\n]*\]\(([^)\r\n]+)\)/g)){
  let target=m[1].replace(/^<|>$/g,'').split('#')[0];
  if(!target||/^(https?:|mailto:)/.test(target))continue;
  target=target.replace(/:\d+(?:-\d+)?$/,'');++links;
  const absolute=path.resolve(path.dirname(doc),target);
  if(absolute!==self&&!fs.existsSync(absolute))missing.push({doc:path.relative(root,doc),target});
}
need(missing.length===0,'文档断链：'+JSON.stringify(missing));
const release=inventory(path.join(root,'work/release'));
need(release.pending.length===0,'本轮构建树无待发布临时文件');
need(!fs.existsSync(path.join(root,'work/release/prototype/owner-codec-coverage.json')),'已消费36MB AST已退休');
const result={date:'2026-10-09',baseline:'4ef4a98',checks,documents:docs.length,links,
  application_schema:schema,world_schema:'a1ca189f3b9290b18390812af91f601eaac9f295154ab1de3601590d0fbda2d7',
  release,kept_snapshots:inventory(path.join(root,'work/snapshots/application-clear-v1')),
  replay_process_seconds:replay.process_wall_seconds,
  replay_process_seconds_sum:replay.process_wall_seconds.reduce((a,b)=>a+b,0),
  application_certificate:certificate,outputs,
  limits:['Windows实际验收；符号链接场景权限不足未跑','POSIX发布未在本机执行','当前只认证raw17条件短回放，非自然通关','完整Steam字体/皮肤/原标题人物Owner仍未完成']};
fs.writeFileSync(self,JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({checks,documents:docs.length,links,release,snapshots:result.kept_snapshots,
  process_seconds:result.replay_process_seconds_sum}));
