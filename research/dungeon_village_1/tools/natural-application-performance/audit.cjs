const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 窄投影等价、条件微基准及当前12月证书审计；不把不同权限环境墙钟当加速比例。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const root=path.resolve(archivePaths.workDir,'../..'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
let checks=0;const need=(ok,m)=>{checks++;if(!ok)throw Error(m);};
const read=p=>fs.readFileSync(path.join(root,p));
const before=read('work/natural-application-performance/before/tail.jsonl');
const after=read('work/natural-application-performance/after/tail.jsonl');
need(before.equals(after)&&before.toString().trimEnd().split('\n').length===20,'600轮前后最后20轮完整trace');
const summaries=['before','after'].map(n=>{
    const rows=read('work/natural-application-performance/'+n+'.log').toString().split('\n').filter(s=>s.startsWith('application-natural-summary '));
    need(rows.length===1,'终点唯一');return JSON.parse(rows[0].slice('application-natural-summary '.length));});
for(const k of ['digest','completed_frame','next_frame','random','sound_count','sound_hash'])need(summaries[0][k]===summaries[1][k],'终点 '+k);
const bench=read('work/natural-application-performance/benchmark.log').toString();
const samples=bench.split('\n').filter(s=>/^\d+,/.test(s)).map(s=>s.split(','));
need(samples.length===7&&samples.every((r,i)=>Number(r[0])===i+1&&r[1]===(i%2?'narrow-first':'full-first')&&r[2]==='1000'&&r[5]===r[6]),'交替七对微基准');
need(bench.includes('unchanged=true'),'微基准输入不变');
const median=a=>a.sort((a,b)=>a-b)[3],full=median(samples.map(r=>Number(r[3]))),narrow=median(samples.map(r=>Number(r[4])));
const worldLog=read('work/natural-application-performance/world-replay.log').toString();
const world=JSON.parse(worldLog.slice(worldLog.indexOf('{\n  "scenario":')));
need(world.trace_sha256==='ddebdf802e54a6dbec103994b7be0e8e35ae0f90e996ba63e08c39e412905ff1','旧world黄金尾段');
for(const [file,count] of [['ctest.log',3],['ctest-benchmark.log',1]])
    need(read('work/natural-application-performance/'+file).toString().includes('100% tests passed out of '+count),'标准检查 '+file);
const prefix='work/snapshots/natural-application-v2/month12.avra',snapshot=read(prefix),cert=JSON.parse(read(prefix+'.json'));
need(cert.snapshot_sha256===hash(snapshot)&&cert.snapshot_bytes===snapshot.length&&snapshot.readUInt32LE(12)===3,'当前12月档身份');
need(cert.capture_frame===19295&&cert.capture_months===12&&cert.tail_frames===20&&cert.process_count===3,'12月三路资格');
const source=JSON.parse(read('work/snapshots/natural-application-v2/month1.avra.json'));
need(cert.source_prefix.snapshot_sha256===source.snapshot_sha256,'从当前首月认证接续');
const model=read('prototype/src/startup_application_replay_fields.json');need(cert.application_schema===hash(model),'应用身份未改');
const outputs=[];
for(const f of fs.readdirSync(archivePaths.workDir))if(/\.log$|seconds\.txt$/.test(f)){
    const b=fs.readFileSync(path.join(archivePaths.workDir,f));outputs.push({file:f,bytes:b.length,sha256:hash(b)});}
const result={date:'2026-10-09',baseline:'464becc',checks,trace_sha256:hash(before),terminal:summaries[1],
    microbenchmark:{qualification:'conditional_projection_only',pairs:7,iterations:1000,full_median_ms:full,narrow_median_ms:narrow,
        reduction_percent:(1-narrow/full)*100,not_full_game_speedup:true},current_month12:cert,outputs,
    resource_boundary:'20行trace，临时系统副本已消费；现金/任务/审计历史仍可增长，无新构建树',
    limits:['前后600轮的权限环境不同，其10.71/21.77秒不用于速度比较','12月当前语义已认证，尚非自然通关','不修改产品/原表/Owner布局/旧黄金']};
fs.writeFileSync(path.join(archivePaths.workDir,'VALIDATION.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({checks,trace_sha256:result.trace_sha256,microbenchmark:result.microbenchmark,
    snapshot_bytes:snapshot.length,tail_seconds:cert.process_wall_seconds.slice(1)}));
