const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 汇集窄方法摘要、固定跳表及已消费的旧输入证据；不导出指令或素材。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),cp=archivePaths.require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(archivePaths.workDir,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(path.join(root,p)),json=p=>JSON.parse(read(p));
const dll=read('DungeonVillageEXE/GameAssembly.dll');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a')throw Error('DLL identity');
const methods=json('work/persistence-replay-analysis/exe/methods.json');
function bytes(va,n){const rva=va-methods.base,s=methods.sections.find(s=>rva>=s.rva&&rva+n<=s.rva+s.rawSize);if(!s)throw Error('PE range');return dll.subarray(s.raw+rva-s.rva,s.raw+rva-s.rva+n);}
const run=(file,args=[])=>cp.execFileSync(process.execPath,[path.join(archivePaths.workDir,file),...args],{encoding:'utf8',maxBuffer:1024*1024});
const windows=run('inspect.cjs',['manifest']).trim().split('\n').map(JSON.parse);
const unique=[...new Set(windows.map(w=>w.rva))].map(rva=>windows.filter(w=>w.rva===rva).sort((a,b)=>b.bytes-a.bytes)[0]);
const jump=bytes(0x1023cea0,25*4),targets=Array.from({length:25},(_,id)=>({id,va:jump.readUInt32LE(id*4)}));
if(targets[0].va!==0x1023c940||targets[3].va!==0x1023ca42||targets.some(t=>t.va<0x1023c820||t.va>=0x1023cea0))throw Error('touch jump table');
const stub=bytes(0x101c9750,3);if(stub.toString('hex')!=='c20000'||!methods.methods.some(m=>m.va===0x101c9750))throw Error('shared empty method');
const old=read('work/steam-input-closure/disassembly.json');
if(sha(old)!=='230701bd97189df0640a1b4eb70c6eaeffbf593d4e460a46d7da9a068f76e7ef')throw Error('old disassembly identity');
const add=JSON.parse(old).find(x=>x.method.rva===0x23e010);
if(add.size!==6752||sha(bytes(add.method.va,add.size))!==add.sha256)throw Error('old _addTouch bytes');
const prior=json('work/steam-startup-resource-map/EVIDENCE.json');
const dir=prior.directories.find(x=>x.group==='common'&&x.file==='img.inf');
if(!dir.steam_rows.includes('177\tsaveload.gif'))throw Error('prior image ID');
const coverage=json('assets/IMAGE_COVERAGE.json');
const images=coverage.records.filter(r=>r.group==='common'&&r.entry&&r.entry.split('/').pop()==='saveload.png').map(r=>({id:r.id,entry:r.entry,bytes:r.bytes,sha256:r.sha256,width:r.width,height:r.height,rgba_sha256:r.rgba_sha256}));
if(images.length!==11)throw Error('inventory image variants');
const literals=JSON.parse(run('literals.cjs')),icons=JSON.parse(run('icon-data.cjs'));
const evidence={scope:'Steam固定标题菜单与选档静态合同；无窗口认证',dll_sha256:sha(dll),method_index_sha256:sha(read('work/persistence-replay-analysis/exe/methods.json')),
 windows,unique_methods:unique.length,unique_method_bytes:unique.reduce((n,w)=>n+w.bytes,0),overlap_note:'同一方法多次前缀只计最长一次；范围可含对齐或跳表，不等于业务指令字节。',
 touch_dispatch:{va:0x1023cea0,bytes:jump.length,sha256:sha(jump),targets},shared_stub:{va:0x101c9750,bytes:3,hex:stub.toString('hex'),instruction:'ret 0',sha256:sha(stub)},
 reused_input:{path:'../steam-input-closure/disassembly.json',sha256:sha(old),method:add.method,bytes:add.size,sha256_method:add.sha256,consumed_anchors:['0x1023E0A8','0x1023EA26','0x1023F1F0','0x1023F45E','0x1023F5CC'],note:'复用既有注册原点、id分流及空option消费；不复制旧指令文件。'},
 image_reference:{note:'复用已核资源清单与目录证据，本工具不重新解密资源归档；177为saveload.gif逻辑目录名，实际成员为PNG。',evidence_sha256:sha(read('work/steam-startup-resource-map/EVIDENCE.json')),coverage_sha256:sha(read('assets/IMAGE_COVERAGE.json')),directory_sha256:dir.steam_sha256,images},
 scripts:['inspect.cjs','literals.cjs','icon-data.cjs','audit.cjs'].map(p=>({path:p,bytes:fs.statSync(path.join(archivePaths.workDir,p)).size,sha256:sha(fs.readFileSync(path.join(archivePaths.workDir,p)))}))};
for(const [file,value] of [['LITERALS.json',literals],['ICONS.json',icons],['EVIDENCE.json',evidence]])fs.writeFileSync(path.join(archivePaths.workDir,file),JSON.stringify(value,null,2)+'\n');
console.log(JSON.stringify({unique_methods:evidence.unique_methods,unique_method_bytes:evidence.unique_method_bytes,jump_targets:targets.length,save_icon_variants:images.length,files:3}));
