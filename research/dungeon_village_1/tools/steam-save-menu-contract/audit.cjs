const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 固定来源的小摘要：复用旧具名方法证据，不复制反汇编全文或运行DLL。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),cp=archivePaths.require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(archivePaths.workDir,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(path.join(root,p)),json=p=>JSON.parse(read(p));
const dll=read('DungeonVillageEXE/GameAssembly.dll'),metadata=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(metadata)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const index=json('work/persistence-replay-analysis/exe/methods.json');
function bytes(va,n){const rva=va-index.base,s=index.sections.find(s=>rva>=s.rva&&rva+n<=s.rva+s.rawSize);if(!s)throw Error('PE range');return dll.subarray(s.raw+rva-s.rva,s.raw+rva-s.rva+n);}
const reused=[];
for(const spec of [
 {file:'steam-interaction-analysis',sha:'005abbee8899f36c84db175973a7dcfeab6cb478b197e700a9773e8cc5effbcf',methods:[0x312460,0x321990,0x30e270,0x23c820,0x23cf10,0x3160b0,0x316360]},
 {file:'facility-animation-closure',sha:'4471a4e9ccb0327d6e07aa67cfb3038e9694a0b2e46dca8b157f85e3f69d9cfa',methods:[0x3285e0]}
]){const p='work/'+spec.file+'/disassembly.json',raw=read(p);if(sha(raw)!==spec.sha)throw Error('prior source');const old=JSON.parse(raw);reused.push({path:'../'+spec.file+'/disassembly.json',sha256:sha(raw),methods:spec.methods.map(rva=>{const m=old.find(m=>m.method.rva===rva);if(!m||sha(bytes(m.method.va,m.size))!==m.sha256)throw Error('prior method bytes');return {method:m.method,bytes:m.size,sha256:m.sha256};})});}
const run=(p,args=[])=>cp.execFileSync(process.execPath,[path.join(archivePaths.workDir,p),...args],{encoding:'utf8',maxBuffer:1024*1024});
const windows=run('inspect.cjs',['manifest']).trim().split('\n').map(JSON.parse),unique=[...new Set(windows.map(w=>w.rva))].map(rva=>windows.filter(w=>w.rva===rva).sort((a,b)=>b.bytes-a.bytes)[0]);
const floats=[0x10dde528,0x10dde538,0x10dde560,0x10dde574,0x10dde580,0x10dde678].map(va=>({va,hex:bytes(va,4).toString('hex'),value:bytes(va,4).readFloatLE(),sha256:sha(bytes(va,4))}));
const constants=[{va:0x1032bb1d,bytes:42,purpose:'菜单选中RGB(76,58,50)／非选中RGB(255,242,220)'},{va:0x1032bc13,bytes:44,purpose:'消息正文RGB(92,51,31)'},{va:0x1032bca2,bytes:54,purpose:'消息选中RGB(255,153,55)'},{va:0x103290b8,bytes:87,purpose:'raw20软标签[0,2]'},{va:0x1032db31,bytes:137,purpose:'非日文宽度[91,90,42]及WIDTH_DATA[1]绑定'},{va:0x10314b0b,bytes:48,purpose:'raw20菜单[25,1],[26,1],[27,1]原序'}].map(s=>({...s,sha256:sha(bytes(s.va,s.bytes))}));
const jump={va:0x1023cea0+9*4,target:bytes(0x1023cea0+9*4,4).readUInt32LE()};if(jump.target!==0x1023c9cd)throw Error('submenu dispatch');
const coverage=json('assets/IMAGE_COVERAGE.json');const resources=[];
for(const entry of ['menu.seb','menu.png','finger_r.seb','finger_l.seb','finger_r.png']){const r=coverage.records.find(r=>r.source==='EXE'&&r.group==='common'&&r.entry===entry),original=read('assets/original/common/'+entry);if(!r||sha(original)!==r.sha256)throw Error('Steam same-byte resource');const item={id:r.id,entry,bytes:r.bytes,sha256:r.sha256,published_alias:'../../assets/original/common/'+entry};if(entry.endsWith('.seb')){const count=original.readInt16BE(4);if(original.readInt16BE(0)!==1||8+count*20!==original.length)throw Error('single-layer SEB');item.frames=original.readInt16BE(2);item.parts=Array.from({length:count},(_,i)=>Array.from({length:10},(_,j)=>original.readInt16BE(8+i*20+j*2)));}else {item.width=r.width;item.height=r.height;item.rgba_sha256=r.rgba_sha256;}resources.push(item);}
const out={scope:'Steam raw20／确认页原序、输入、绘制与语言翻译有限消费者；无窗口实验',dll_sha256:sha(dll),metadata_sha256:sha(metadata),method_index_sha256:sha(read('work/persistence-replay-analysis/exe/methods.json')),windows,unique_new_methods:unique.length,unique_new_prefix_bytes:unique.reduce((s,w)=>s+w.bytes,0),reused,
 draw_boundary:{indexed_next_va:0x10383210,indexed_span_bytes:197616,consumed_prefix_end:0x103571c0,consumed_prefix_bytes:17312,limit:'索引下一具名方法不是真实函数边界；0x103571C0另起prologue，本批不据登记间距解码后续匿名函数。不宣称其他页面分支已逐项审读。'},floats,constants,submenu_dispatch:jump,resources,
 scripts:['inspect.cjs','literals.cjs','width-data.cjs','audit.cjs'].map(p=>({path:p,bytes:fs.statSync(path.join(archivePaths.workDir,p)).size,sha256:sha(fs.readFileSync(path.join(archivePaths.workDir,p)))}))};
for(const [name,data] of [['EVIDENCE.json',out],['LITERALS.json',JSON.parse(run('literals.cjs'))],['WIDTH.json',JSON.parse(run('width-data.cjs'))]])fs.writeFileSync(path.join(archivePaths.workDir,name),JSON.stringify(data,null,2)+'\n');
console.log(JSON.stringify({new_methods:out.unique_new_methods,new_prefix_bytes:out.unique_new_prefix_bytes,reused_methods:reused.reduce((s,r)=>s+r.methods.length,0),float_values:floats.map(f=>f.value),resources:resources.length}));
