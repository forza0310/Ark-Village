// CLI恢复边界修正后的定向复验；不重复旧世界/计分长前缀。
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const executable=path.join(root,'work/release/bin/dungeon_village_startup_application_tests.exe');
const owned=fs.mkdtempSync(path.join(__dirname,'title-process-'));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function need(ok,message){if(!ok)throw Error(message);}
let passed=false;
try{
  const dirs=[0,1,2].map(i=>path.join(owned,'process-'+i));
  dirs.forEach(p=>fs.mkdirSync(p));
  const traces=dirs.map(p=>path.join(p,'trace.jsonl'));
  const snapshot=path.join(dirs[0],'title600.avra');
  function run(i,mode){
    const p=cp.spawnSync(executable,['title-background-replay-v2','--work-dir',dirs[i],mode,snapshot,'--trace-file',traces[i]],
      {encoding:'utf8',timeout:60000,windowsHide:true,maxBuffer:1024*1024});
    need(!p.error&&p.status===0,'标题进程失败 '+JSON.stringify(p));return p.stdout.trim();
  }
  const outputs=[run(0,'--save-file')],source=fs.readFileSync(snapshot);
  const rows=fs.readFileSync(traces[0],'utf8').trimEnd().split('\n');
  need(rows.length===680&&rows.every((s,i)=>JSON.parse(s).next_frame===i+1),'连续1..680请求');
  const tail=Buffer.from(rows.slice(600).join('\n')+'\n');
  for(let i=1;i<3;i++){
    outputs.push(run(i,'--load-file'));
    need(tail.equals(fs.readFileSync(traces[i])),'双恢复完整JSONL尾段一致');
    need(source.equals(fs.readFileSync(snapshot)),'源快照不变');
    need(outputs[i]===outputs[0],'终点一致');
  }
  const previous=fs.readFileSync(path.join(__dirname,'replay.log'),'utf8');
  const original=JSON.parse(previous.slice(previous.indexOf('{\n  "scenario":'))).title_presentation_replay;
  need(hash(source)===original.snapshot_sha256&&hash(tail)===original.trace_sha256&&outputs[0]===original.terminal_output,
       'CLI边界修正不改变已认证字节/轨迹');
  const result={date:'2026-10-09',qualification:'title_presentation_replay',process_count:3,
    capture_next_frame:600,stop_at:680,tail_frames:80,snapshot_bytes:source.length,
    snapshot_sha256:hash(source),trace_bytes:tail.length,trace_sha256:hash(tail),terminal_output:outputs[0]};
  fs.writeFileSync(path.join(__dirname,'title-replay-verified.json'),JSON.stringify(result,null,2)+'\n');
  console.log(JSON.stringify(result));passed=true;
} finally {
  // 只回收本脚本创建且核对位于本交付目录中的临时目录，失败留现场。
  need(path.dirname(owned)===__dirname,'临时目录边界');
  if(passed)fs.rmSync(owned,{recursive:true});
}
