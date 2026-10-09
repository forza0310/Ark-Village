// 复核固定方法、原资源及已有调用窗口；仅保存最小来源摘要，不复制反编译实现。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(__dirname,'../..'),read=p=>fs.readFileSync(path.join(root,p));
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=read('DungeonVillageEXE/GameAssembly.dll'),metadata=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(metadata)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const index=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
const bytes=(va,n)=>{const r=va-index.base,s=index.sections.find(s=>r>=s.rva&&r+n<=s.rva+s.rawSize);if(!s)throw Error('PE data range');return dll.subarray(s.raw+r-s.rva,s.raw+r-s.rva+n);};
const run=(p,args)=>cp.execFileSync(process.execPath,[path.join(__dirname,p),...args],{encoding:'utf8',maxBuffer:1024*1024});
const methods=run('inspect.cjs',['manifest']).trim().split('\n').map(JSON.parse);
if(methods.length!==19 || methods.reduce((n,m)=>n+m.bytes,0)>16384)throw Error('fixed method budget');
const assertions=[];
function call(va,target,label){const b=bytes(va,5);if(b[0]!==0xe8||va+5+b.readInt32LE(1)!==target)throw Error(label);assertions.push({va,bytes:5,sha256:sha(b),call_target:target,label});}
for(const [va,target,label] of [
 [0x102521b0,0x10763590,'window outer outline'],[0x1025221f,0x10763590,'window inner outline'],
 [0x102522cf,0x10760c50,'40-wide wood tile'],[0x1025234a,0x10760c50,'title strip'],
 [0x10252396,0x1077e930,'title integer measurement'],[0x102523cd,0x10766290,'title shadow'],
 [0x102523ec,0x1077e930,'title second integer measurement'],[0x10252423,0x10766290,'title foreground'],
 [0x1024b12f,0x1076a990,'box fill'],[0x1024b198,0x10763590,'box outer outline'],
 [0x1024b20b,0x10763590,'box inner outline'],[0x1024b3cf,0x107bc710,'standard corner frame0'],
 [0x1024b275,0x102516f0,'white corner explicit image frame0'],
 [0x10763696,0x1076f080,'outline expanded outer quad'],[0x10763704,0x1076f080,'outline inset inner quad'],
 [0x1076aa83,0x1076f080,'fill original-sized quad'],[0x1075dfb9,0x107712c0,'ClipRect push before intersect'],
 [0x1075e152,0x1077bc40,'ClipRect transformed intersection'],[0x10773c22,0x1077bc40,'SetClip replacement'],
 [0x1077062a,0x1077bc40,'PopClip reinstalls saved floats'],[0x1077566d,0x10c35d90,'GUI previous group end'],
 [0x10775776,0x10c32120,'GUI clip group begin'],
 [0x1020f867,0x1075df70,'selector temporary clip'],[0x1020f86f,0x10770590,'selector clip popped before label'],
 [0x1020f8ad,0x107678c0,'selector type label after PopClip'],
 [0x103570af,0x10773c90,'answer resets explicit SC_WINDOW_BLACK'],
 [0x103570f6,0x107678c0,'answer point text after color reset']
])call(va,target,label);
const floats=[0x10dde55c,0x10dde54c].map(va=>({va,hex:bytes(va,4).toString('hex'),value:bytes(va,4).readFloatLE(),sha256:sha(bytes(va,4))}));
if(floats[0].hex!=='0000803f' || floats[1].hex!=='0ad7233c')throw Error('outline/clip float identity');
const prior_specs=[['steam-title-menu-contract','EVIDENCE.json'],['steam-save-menu-contract','EVIDENCE.json'],['steam-gltext-lifecycle','EVIDENCE.json'],['facility-animation-closure','disassembly.json']];
const reused=prior_specs.map(([dir,name])=>{const p='work/'+dir+'/'+name,b=read(p);return {path:'../'+dir+'/'+name,bytes:b.length,sha256:sha(b)};});
const colors=[
 {field:'SC_WINDOW_TITLE',offset:0x60,rgb:[247,253,247],start:0x1032bbd5,end:0x1032bc08},
 {field:'SC_WINDOW_TITLE2',offset:0x64,rgb:[44,54,105],start:0x1032bbfd,end:0x1032bc21},
 {field:'SC_WINDOW_BLACK',offset:0x68,rgb:[92,51,31],start:0x1032bc13,end:0x1032bc3a}
].map(x=>({...x,bytes:x.end-x.start,sha256:sha(bytes(x.start,x.end-x.start))}));
const caller_windows=[
 [0x1020f824,0x1020f8b2,'type label RGB60/100/200; ClipRect then immediate PopClip; anchor2'],
 [0x1020f955,0x1020f9c8,'selected empty manual RGB200/200/255 at85,R+14,82,14'],
 [0x1020fb0f,0x1020fb50,'empty manual text RGB30/30/30 anchor2'],
 [0x1020ff0e,0x1020ff51,'empty interrupt text RGB30/30/30 anchor2'],
 [0x1021025c,0x10210321,'visible slot name RGB30/30/30 TextLayout'],
 [0x1021037a,0x102105f9,'date/cash RGB30/30/30; cash anchor4'],
 [0x1035708d,0x103570fb,'both answers explicit SC_WINDOW_BLACK, anchor2']
].map(([va,end,label])=>({va,end,bytes:end-va,sha256:sha(bytes(va,end-va)),label}));
const coverage=JSON.parse(read('assets/IMAGE_COVERAGE.json')),records=coverage.records.filter(r=>r.source==='EXE'&&r.group==='common');
const resources=[];
for(const [kind,id,name] of [['image',28,'wnd_back.png'],['image',29,'wnd_bar.png'],['image',30,'wnd_conner.png'],['image',121,'wnd_conner01.png'],['sprite',6,'wnd_conner.seb']]) {
 const record=records.find(r=>r.entry===name),inf=records.find(r=>r.entry===(kind==='image'?'img.inf':'seb.inf'));
 if(!record||!inf.index_targets.some(t=>t.index===id&&t.filename===name&&t.record===record.id))throw Error('resource index '+name);
 const source=read('assets/original/common/'+name);if(sha(source)!==record.sha256)throw Error('Steam original alias '+name);
 const result={kind,index:id,id:record.id,path:'../../assets/original/common/'+name,bytes:source.length,sha256:record.sha256,width:record.width,height:record.height};
 if(kind==='sprite') {
  const count=source.readInt16BE(4);if(source.readInt16BE(0)!==1||8+count*20!==source.length)throw Error('single layer SEB');
  result.frames=source.readInt16BE(2);result.parts=Array.from({length:count},(_,i)=>Array.from({length:10},(_,j)=>source.readInt16BE(8+i*20+j*2)));
 }
 resources.push(result);
}
const dump=read('work/persistence-replay-analysis/exe/dumper/dump.cs'),lines=dump.toString('utf8').split(/\r?\n/);
const fields=lines.map((text,i)=>({line:i+1,text:text.trim()})).filter(x=>
 (x.line>=37775&&x.line<=37820&&/clip|guiGroup|guiBegin|Matrix/.test(x.text)) ||
 (x.line>=225550&&x.line<=225565&&/SC_WINDOW/.test(x.text)));
const result={scope:'Steam window helper expansion, inherited clipping and startup missing-style consumers; static only',dll_sha256:sha(dll),metadata_sha256:sha(metadata),methods,method_count:methods.length,method_bytes:methods.reduce((n,m)=>n+m.bytes,0),assertions,floats,colors,caller_windows,reused,resources,field_source:{path:'../persistence-replay-analysis/exe/dumper/dump.cs',sha256:sha(dump),fields},limitations:['No live CJK glyph identity or OS/DPI mapping','No proof of active clipping backend or all Matrix/Shader internals','Original helper lacks the stricter C++ dimensions guards','No C++ or frozen asset-package mutation'],scripts:['inspect.cjs','audit.cjs'].map(p=>{const b=fs.readFileSync(path.join(__dirname,p));return {path:p,bytes:b.length,sha256:sha(b)};})};
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({methods:result.method_count,method_bytes:result.method_bytes,call_anchors:assertions.length,resources:resources.length,floats:floats.map(f=>f.value)}));
