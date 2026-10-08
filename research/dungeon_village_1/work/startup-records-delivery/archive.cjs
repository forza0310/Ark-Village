// 汇总已执行结果，不重复运行CTest／CLI，不读取原程序或玩家档。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),{execFileSync}=require('child_process');
const root=path.resolve(__dirname,'../..'),repo=path.resolve(root,'../..');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=b=>(b[0]===255&&b[1]===254?b.toString('utf16le'):b.toString('utf8')).replace(/^\uFEFF/,'');
const normalize=s=>s.replace(/\r\n/g,'\n').replace(/[ \t]+$/gm,'').trimEnd()+'\n';
const logs=[];
for(const [source,dest] of [['startup-records-build.log','build.log'],['startup-records-ctest.log','ctest-initial.log'],['startup-records-persistence.log','ctest-final-persistence.log']]){
  const bytes=Buffer.from(normalize(read(fs.readFileSync(path.join(root,'work',source)))));
  fs.writeFileSync(path.join(__dirname,dest),bytes);
  logs.push({file:dest,bytes:bytes.length,sha256:sha(bytes)});
}
const git=(...args)=>execFileSync('git',['-c','safe.directory='+repo.replaceAll('\\','/'),...args],{cwd:repo,encoding:'utf8'}).trim().split(/\r?\n/).filter(Boolean);
const cliFile=path.join(__dirname,'VALIDATION.json'),cli=JSON.parse(fs.readFileSync(cliFile,'utf8'));
for(const log of cli.logs){
  const file=path.join(__dirname,log.file),original=fs.readFileSync(file),bytes=Buffer.from(normalize(read(original)));
  if(!bytes.equals(original))log.raw_capture={bytes:original.length,sha256:sha(original)};
  fs.writeFileSync(file,bytes);log.bytes=bytes.length;log.sha256=sha(bytes);
}
cli.log_policy_zh='归档文本统一UTF-8/LF、去行末空白，原始捕获身份按需另存；不修改测试结论';
fs.writeFileSync(cliFile,JSON.stringify(cli,null,2)+'\n');
const changed=[...git('diff','--name-only'),...git('diff','--cached','--name-only'),...git('ls-files','--others','--exclude-standard')].filter(f=>f.startsWith('research/')&&f.endsWith('.md'));
const extra=['work/endgame-records-contract/README.md','work/steam-build-list-contract/README.md','work/startup-records-profile/README.md','work/startup-records-delivery/README.md'].map(f=>path.relative(repo,path.join(root,f)));
let links=0;const missing=[];
for(const file of [...new Set([...changed,...extra])]){
  const full=path.join(repo,file),md=fs.readFileSync(full,'utf8');
  for(const m of md.matchAll(/\[[^\]]*\]\(([^)\s]+)\)/g)){
    const link=m[1].split('#')[0].replace(/:\d+$/,''); // 仓库源文件链接允许:行号。
    if(!link||/^[a-z]+:/i.test(link))continue;
    ++links;
    if(!fs.existsSync(path.resolve(path.dirname(full),decodeURIComponent(link))))missing.push({file,link});
  }
}
const tree=dir=>{
  let bytes=0,files=0,pending=[];
  for(const e of fs.readdirSync(dir,{withFileTypes:true})){
    const full=path.join(dir,e.name);
    if(e.isSymbolicLink())continue;
    if(e.isDirectory()){const t=tree(full);bytes+=t.bytes;files+=t.files;pending.push(...t.pending);}
    else {bytes+=fs.statSync(full).size;++files;if(/\.pending|\.tmp\./.test(e.name))pending.push(path.relative(root,full));}
  }
  return {bytes,files,pending};
};
const audit={date:'2026-10-08',scope_zh:'当前修改的文档链接与已执行日志／资源规模，不代替行为回归',logs,
  changed_markdown:changed.length,links,missing_links:missing,release_tree:tree(path.join(root,'work/release')),
  product_requests_sha256:sha(fs.readFileSync(path.join(repo,'docs/reference/RESEARCH_REQUESTS.md'))),
  retired_ast:{bytes:36315062,sha256:'01ed97674fbcef8608db8a0301484c975d4c7d52fabcc3ebe80b83d0ab54a6ba',reason_zh:'codec覆盖检查生成的本轮AST，检查成功后清理；与先前生成阶段AST分别记录'},
  ownership_zh:'计分完成退休三项controller引用；声音一次领取；标题最多原定义数且每次替换名单；现金/任务/审计历史仍可能合法增长'};
fs.writeFileSync(path.join(__dirname,'AUDIT.json'),JSON.stringify(audit,null,2)+'\n');
console.log(JSON.stringify(audit));
