// 本批只读核来源、当前应用身份、回放证书、文档及规模；不读取原存档。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
const root=path.resolve(__dirname,'../..'),repo=path.resolve(root,'../..');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(path.join(root,p));
let checks=0;const need=(v,m)=>{++checks;if(!v)throw Error(m);};
const git=args=>cp.execFileSync('git',['-c','safe.directory=D:/code/Ark-Village',...args],{cwd:repo,encoding:'utf8'}).trim();
function inventory(dir){
  let files=0,bytes=0,pending=[];
  for(const e of fs.readdirSync(dir,{withFileTypes:true})){
    const p=path.join(dir,e.name);
    if(e.isDirectory()){const c=inventory(p);files+=c.files;bytes+=c.bytes;pending.push(...c.pending);}
    else {files++;bytes+=fs.statSync(p).size;if(/\.pending$|\.tmp\./.test(e.name))pending.push(path.relative(root,p));}
  }
  return {files,bytes,pending};
}
const evidence=JSON.parse(read('work/title-owner-contract/EVIDENCE.json'));
for(const e of evidence.sources){
  const b=read(e.path);need(b.length===e.bytes&&hash(b)===e.source_sha256,'来源身份 '+e.path);
  const lines=b.toString('utf8').replace(/\r\n/g,'\n').split('\n');
  const window=lines.slice(e.first-1,e.last).join('\n')+'\n';
  need(hash(window)===e.window_lf_sha256,'来源行窗 '+e.path+':'+e.first);
}
const manifest=read('prototype/src/startup_application_replay_fields.json');
const model=JSON.parse(manifest),schema=hash(manifest);
need(model.semantics===2&&model.fields.length===16&&model.nested.reduce((n,t)=>n+t.fields.length,0)===11,'16+11字段语义2');
need(read('prototype/src/startup_application_replay_fields.inc').toString().includes(schema),'schema清单一致');
need(!git(['diff','HEAD','--','research/dungeon_village_1/data','research/dungeon_village_1/assets/original',
  'research/dungeon_village_1/prototype/src/startup_world_codec_fields.inc',
  'research/dungeon_village_1/prototype/src/startup_world_codec_fields.json']),'原表/原图/world schema不变');
const text=read('work/title-owner-delivery/replay.log').toString();
const begin=text.indexOf('{\n  "scenario":');need(begin>=0,'进程runner证书');
const replay=JSON.parse(text.slice(begin));
need(replay.trace_sha256==='ddebdf802e54a6dbec103994b7be0e8e35ae0f90e996ba63e08c39e412905ff1','原世界尾段不变');
need(replay.title_presentation_replay.tail_frames===80&&replay.title_presentation_replay.process_count===3,'标题三路80请求');
const certificate=JSON.parse(read('work/snapshots/application-clear-v2/CERTIFICATE.json'));
need(certificate.qualification==='conditional_raw17_application_replay'&&certificate.captures.length===4,'计分条件资格');
for(const e of certificate.captures){
  const b=read('work/snapshots/application-clear-v2/'+e.boundary+'.avra');
  need(b.length===e.snapshot_bytes&&hash(b)===e.snapshot_sha256,'条件档身份 '+e.boundary);
  need(b.readUInt32LE(12)===2&&b.subarray(-64).toString()===hash(b.subarray(0,-64)),'语义2完整容器');
}
need(read('work/title-owner-delivery/ctest-verified.log').toString().includes('100% tests passed'),'修复后主责通过');
const changed=git(['diff','HEAD','--name-only','--','research']).split('\n').filter(p=>p.endsWith('.md')).map(p=>path.join(repo,p));
const extra=['work/title-owner-delivery/README.md','work/title-owner-contract/README.md','work/natural-application-route/README.md'];
const docs=[...new Set([...changed,...extra.map(p=>path.join(root,p))])];
let links=0;const missing=[];
for(const doc of docs)for(const m of fs.readFileSync(doc,'utf8').matchAll(/\[[^\]\r\n]*\]\(([^)\r\n]+)\)/g)){
  const target=m[1].replace(/^<|>$/g,'').split('#')[0].replace(/:\d+(?:-\d+)?$/,'');
  if(!target||/^(https?:|mailto:)/.test(target))continue;
  links++;const absolute=path.resolve(path.dirname(doc),target);
  if(absolute!==path.join(__dirname,'VALIDATION.json')&&!fs.existsSync(absolute))missing.push({doc,target});
}
need(!missing.length,'断链 '+JSON.stringify(missing));
const release=inventory(path.join(root,'work/release'));
need(!release.pending.length,'无待发布临时文件');
need(!fs.existsSync(path.join(root,'work/release/prototype/owner-codec-coverage.json')),'已消费AST清理');
const outputs=fs.readdirSync(__dirname).filter(p=>/\.log$|^title-replay-verified\.json$/.test(p)).map(p=>{
  const b=fs.readFileSync(path.join(__dirname,p));return {path:p,bytes:b.length,sha256:hash(b)};
});
const result={date:'2026-10-09',baseline:'89f157c',checks,source_windows:evidence.sources.length,
  documents:docs.length,links,application_schema:schema,release,
  kept_snapshots:inventory(path.join(root,'work/snapshots/application-clear-v2')),
  title_certificate:replay.title_presentation_replay,application_certificate:certificate,outputs,
  limits:['固定APK静态及Windows C++维护验证，不代表Steam动态','完整h/j/o输入、身体裁片和自然Driver尚未实现','历史账本与任务可增长，无永久有界承诺']};
fs.writeFileSync(path.join(__dirname,'VALIDATION.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({checks,links,release,snapshots:result.kept_snapshots}));
