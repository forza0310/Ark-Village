const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 本阶段收口：当前语义3与修前诊断分别核身份，未验性能源码不纳入本阶段认证。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),cp=archivePaths.require('child_process');
const root=path.resolve(archivePaths.workDir,'../..'),repo=path.resolve(root,'../..');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(path.join(root,p));
const git=a=>cp.execFileSync('git',['-c','safe.directory=D:/code/Ark-Village',...a],{cwd:repo,encoding:'utf8'}).trim();
let checks=0;const need=(ok,m)=>{checks++;if(!ok)throw Error(m);};
function inventory(dir){let files=0,bytes=0,pending=[];for(const e of fs.readdirSync(dir,{withFileTypes:true})){
    const p=path.join(dir,e.name);if(e.isDirectory()){const c=inventory(p);files+=c.files;bytes+=c.bytes;pending.push(...c.pending);}
    else{files++;bytes+=fs.statSync(p).size;if(/\.pending$|\.tmp\./.test(e.name))pending.push(p);}}
    return {files,bytes,pending};}
const model=read('prototype/src/startup_application_replay_fields.json'),manifest=JSON.parse(model);
need(manifest.semantics===3&&manifest.fields.length===16&&manifest.nested.reduce((n,t)=>n+t.fields.length,0)===11,'语义3/16+11');
need(read('prototype/src/startup_application_replay_fields.inc').toString().includes(hash(model)),'应用规范schema');
need(!git(['diff','HEAD','--','research/dungeon_village_1/data','research/dungeon_village_1/assets/original',
    'research/dungeon_village_1/prototype/src/startup_world_codec_fields.inc',
    'research/dungeon_village_1/prototype/src/startup_world_codec_fields.json']),'world字段/原表/素材不变');
const current=[];
for(const name of ['frame420','month1']){
    const prefix='work/snapshots/natural-application-v2/'+name+'.avra';
    const bytes=read(prefix),cert=JSON.parse(read(prefix+'.json'));
    need(bytes.length===cert.snapshot_bytes&&hash(bytes)===cert.snapshot_sha256&&bytes.readUInt32LE(12)===3,'当前自然档 '+name);
    need(cert.application_schema===hash(model)&&cert.process_count===3&&cert.tail_frames===20&&
        cert.stop_at===cert.capture_frame+20,'自然尾段证书 '+name);
    current.push(cert);
}
need(current[0].capture_frame===420&&current[1].capture_frame===1634&&current[1].capture_months===1,'实际捕获边界');
need(current[1].source_prefix.snapshot_sha256===current[0].snapshot_sha256,'首月复用当前420源');
const retired=[];
for(const name of ['frame420','month1','month12']){
    const prefix='work/snapshots/natural-application-v1/'+name+'.avra';
    const bytes=read(prefix),cert=JSON.parse(read(prefix+'.json'));
    need(bytes.length===cert.snapshot_bytes&&hash(bytes)===cert.snapshot_sha256&&bytes.readUInt32LE(12)===2,'历史诊断身份 '+name);
    retired.push({path:prefix,bytes:bytes.length,sha256:hash(bytes),status:'修前诊断，当前语义3拒绝'});
}
const clear=JSON.parse(read('work/snapshots/application-clear-v3/CERTIFICATE.json'));
need(clear.captures.length===4&&clear.qualification==='conditional_raw17_application_replay','当前四计分条件档');
for(const c of clear.captures){const b=read('work/snapshots/application-clear-v3/'+c.boundary+'.avra');
    need(hash(b)===c.snapshot_sha256&&b.length===c.snapshot_bytes&&b.readUInt32LE(12)===3,'条件档 '+c.boundary);}
const log=read('work/natural-application-delivery/replay-final.log').toString();
const replay=JSON.parse(log.slice(log.indexOf('{\n  "scenario":')));
need(replay.trace_sha256==='ddebdf802e54a6dbec103994b7be0e8e35ae0f90e996ba63e08c39e412905ff1','旧世界黄金尾段');
need(replay.natural_application_replay.snapshot_sha256===current[0].snapshot_sha256,'现有runner实际接线');
need(read('work/natural-application-delivery/ctest-final.log').toString().includes('100% tests passed out of 6'),'六项受影响检查');
const png=read('work/title-actor-skin/actor-pieces.png');
need(png.readUInt32BE(16)===760&&png.readUInt32BE(20)===1230,'实际查看的CPU图');
const sourceSkin=JSON.parse(read('work/title-actor-skin/EVIDENCE.json'));
const docs=[...new Set([...git(['diff','HEAD','--name-only','--','research']).split('\n').filter(p=>p.endsWith('.md')).map(p=>path.join(repo,p)),
    ...['ui/STEAM_TITLE_DRAW.md','ui/TITLE_ACTOR_SKIN.md','work/natural-application-delivery/README.md',
        'work/title-actor-skin/README.md','work/steam-title-draw/README.md'].map(p=>path.join(root,p))])];
let links=0;const missing=[];
for(const doc of docs)for(const m of fs.readFileSync(doc,'utf8').matchAll(/\[[^\]\r\n]*\]\(([^)\r\n]+)\)/g)){
    const target=m[1].replace(/^<|>$/g,'').split('#')[0].replace(/:\d+(?:-\d+)?$/,'');
    if(!target||/^(https?:|mailto:)/.test(target))continue;links++;
    const p=path.resolve(path.dirname(doc),target);
    if(p!==path.join(archivePaths.workDir,'VALIDATION.json')&&!fs.existsSync(p))missing.push({doc,target});}
need(!missing.length,'断链 '+JSON.stringify(missing));
const release=inventory(path.join(root,'work/release'));
need(!release.pending.length&&!fs.existsSync(path.join(root,'work/release/prototype/owner-codec-coverage.json')),'临时发布/AST清理');
const outputs=[];
for(const dir of ['work/natural-application-delivery','work/title-actor-skin','work/steam-title-draw'])
    for(const name of fs.readdirSync(path.join(root,dir)))if(/\.(log|png|json)$/.test(name)&&name!=='VALIDATION.json'){
        const b=read(dir+'/'+name);outputs.push({path:dir+'/'+name,bytes:b.length,sha256:hash(b)});}
const result={date:'2026-10-09',baseline:'327fcc9',checks,links,application_schema:hash(model),
    current_natural:current,retired_natural:retired,current_clear:clear,release,outputs,
    natural_snapshots:inventory(path.join(root,'work/snapshots/natural-application-v2')),
    conditional_snapshots:inventory(path.join(root,'work/snapshots/application-clear-v3')),
    actor_source_checks:sourceSkin.checks??null,
    limits:['语义3当前只认证420/首月，修前12月不冒充修后','CPU人物图是基础图层，不是完整窗口','独立性能窄投影仍在途，未纳入本批二进制结果','原程序/实时原档/产品/远程推送未操作']};
fs.writeFileSync(path.join(archivePaths.workDir,'VALIDATION.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({checks,links,release,natural:result.natural_snapshots,conditional:result.conditional_snapshots}));
