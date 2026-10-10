const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 只审核本批文档、离线引用与派生清单，不运行游戏／长测或窗口服务。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const root=path.resolve(archivePaths.workDir,'../..'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const documents=['ui/STARTUP_SKIN.md','ui/README.md','ui/examples/README.md',
 'stages/in-progress/APPLICATION_REPLAY_DESIGN.md','stages/README.md','stages/COMPREHENSIVE_RESEARCH.md',
 'VERIFICATION.md','work/startup-skin-contract/README.md','work/natural-clear-route/README.md',
 'work/startup-window-observation/RESULT.md'];
let links=0;const missing=[];
for(const file of documents){
 const full=path.join(root,file),text=fs.readFileSync(full,'utf8');
 if(text.includes('\ufffd'))throw Error('文档替换字符 '+file);
 for(const m of text.matchAll(/\[[^\]]*\]\(([^)\s]+)\)/g)){
  const link=m[1].split('#')[0].replace(/:\d+$/,'');
  if(!link||/^[a-z]+:/i.test(link))continue;
  ++links;if(!fs.existsSync(path.resolve(path.dirname(full),decodeURIComponent(link))))missing.push({file,link});
 }
}
const html=fs.readFileSync(path.join(root,'ui/examples/startup-sources.html'),'utf8');
let htmlReferences=0;
for(const m of html.matchAll(/(?:src|href)="([^"]+)"/g)){
 ++htmlReferences;
 if(!fs.existsSync(path.resolve(root,'ui/examples',m[1])))throw Error('素材页引用缺失 '+m[1]);
}
const directories=['work/startup-skin-contract','work/natural-clear-route','work/startup-window-observation'];
const artifacts=[];
for(const dir of directories)
 for(const e of fs.readdirSync(path.join(root,dir),{withFileTypes:true})){
  if(!e.isFile())throw Error('本批取证目录不应含缓存子目录 '+dir);
  const file=dir+'/'+e.name,b=fs.readFileSync(path.join(root,file));artifacts.push({file,bytes:b.length,sha256:hash(b)});
 }
const result={date:'2026-10-08',base:'9ec4c1a',qualification_zh:'静态研究／设计审计；无新运行时验收或原窗口动态',
 markdown_files:documents.length,links,missing_links:missing,html_references:htmlReferences,artifacts,
 artifact_bytes:artifacts.reduce((n,f)=>n+f.bytes,0),ui_resources_copied:0,new_build_trees:0,
 window_channel:{attempts:2,error:'Computer Use native pipe is unavailable (os error 2)',input_sent:false},
 browser_channel:{apps:0,browsers:0},pending_decision:'APPLICATION_REPLAY_DESIGN.md：单容器、隔离无覆盖恢复与第一版回放范围'};
fs.writeFileSync(path.join(archivePaths.workDir,'VALIDATION.json'),JSON.stringify(result,null,2)+'\n');
if(missing.length)throw Error(JSON.stringify(missing));
console.log(JSON.stringify({links,htmlReferences,artifact_bytes:result.artifact_bytes,missing_links:missing}));
