const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 本批集中审核：仅运行研究CLI，显式项目内临时文件；不访问原游戏或玩家存档。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),{spawnSync}=archivePaths.require('child_process');
const root=path.resolve(archivePaths.workDir,'../..'), build=path.join(root,'work/release');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
let checks=0;
const check=(yes,why)=>{++checks;if(!yes)throw Error(why);};
const sandbox=fs.mkdtempSync(path.join(archivePaths.workDir,'cli-fixture-'));
const executable=path.join(build,'bin/dungeon_village_startup_application_cli.exe');
const files=['system.avr','world0.avr','world1.avr'].map(f=>path.join(sandbox,f));
const args=['--system',files[0],'--slot0',files[1],'--slot1',files[2],'--seed','42','--title-presentation'];
const artifacts=[];
try {
  const run=(name,input,parameters=args)=>{
    const r=spawnSync(executable,parameters,{input,encoding:'utf8',timeout:30000,maxBuffer:1024*1024});
    if(r.error)throw r.error;
    const output=(r.stdout+r.stderr).replace(/\r\n/g,'\n').replace(/[ \t]+$/gm,'').trimEnd()+'\n';
    fs.writeFileSync(path.join(archivePaths.workDir,name+'.log'),output);
    artifacts.push({file:name+'.log',sha256:sha(Buffer.from(output)),bytes:Buffer.byteLength(output)});
    return r;
  };
  const first=run('cli-create','records\nnext\nprevious\ntitle\nnew 0\nvillage 审核村\nname 自定义主角\nsex 1\ncancel\nnew 0\nstart\nsave\nquit\n');
  check(first.status===0&&!first.stdout.includes('拒绝：'),'CLI配置/取消/开始/保存链');
  check(first.stdout.includes('自定义主角')&&first.stdout.includes('审核村'),'中文名称消费者输出');
  check(fs.existsSync(files[0])&&fs.existsSync(files[1])&&!fs.existsSync(files[2]),'显式系统与单世界文件范围');
  const old=fs.readFileSync(files[1]);
  check(old.subarray(0,8).toString()==='AVRSAVE1','世界magic');
  check(fs.readFileSync(files[0]).subarray(0,8).toString()==='AVRSYS01','系统magic');
  const second=run('cli-reopen','load 0\nview\ntitle\nnew 0\noverwrite yes\nname 新主角\nstart\nquit\n');
  check(second.status===0&&!second.stdout.includes('拒绝：'),'独立进程加载及覆盖询问');
  check(fs.readFileSync(files[1]).equals(old),'开始及退出不覆盖旧世界文件');
  const bad=run('cli-denials','new 2\nstep 10001\nsex 4\nquit\n');
  check((bad.stdout.match(/拒绝：/g)||[]).length===3,'输入范围明确拒绝');
  const absent=run('cli-missing-paths','',[]);
  check(absent.status===2,'没有显式路径不得启动');
  const tree=directory=>{
    let bytes=0,count=0,pending=[];
    for(const ent of fs.readdirSync(directory,{withFileTypes:true})){
      const full=path.join(directory,ent.name);
      if(ent.isSymbolicLink())continue;
      if(ent.isDirectory()){const r=tree(full);bytes+=r.bytes;count+=r.files;pending.push(...r.pending);}
      else {bytes+=fs.statSync(full).size;++count;if(/\.pending|\.tmp\./.test(ent.name))pending.push(path.relative(root,full));}
    }
    return {bytes,files:count,pending};
  };
  const schema=sha(fs.readFileSync(path.join(root,'prototype/src/startup_world_codec_fields.json')));
  check(schema==='a1ca189f3b9290b18390812af91f601eaac9f295154ab1de3601590d0fbda2d7','新profile schema独立身份');
  const records={date:'2026-10-08',checks,schema,scope_zh:'研究CLI及资源审计，不是原窗口／产品／自然16年路线',
    logs:artifacts,release_tree:tree(build),system_file:{bytes:fs.statSync(files[0]).size,sha256:sha(fs.readFileSync(files[0]))},
    world_file:{bytes:old.length,sha256:sha(old)},temporary_files:'仅本脚本独占目录，finally回收'};
  check(records.release_tree.pending.length===0,'Release树无未完成替换');
  records.checks=checks;
  fs.writeFileSync(path.join(archivePaths.workDir,'VALIDATION.json'),JSON.stringify(records,null,2)+'\n');
  console.log(JSON.stringify({checks,release_tree:records.release_tree,schema}));
} finally {
  const relative=path.relative(archivePaths.workDir,sandbox);
  if(!relative||relative.startsWith('..')||path.isAbsolute(relative))throw Error('拒绝越界清理');
  fs.rmSync(sandbox,{recursive:true,force:true});
}
