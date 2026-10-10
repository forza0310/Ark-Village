const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// Steam升级81：源字节/计数表/素材引用核对；不执行游戏，不改旧证据。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),cp=archivePaths.require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(archivePaths.workDir,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex'),read=p=>fs.readFileSync(path.join(root,p));
const dll=read('DungeonVillageEXE/GameAssembly.dll'),meta=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source');
const ix=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
function bytes(va,n){const r=va-ix.base,s=ix.sections.find(s=>r>=s.rva&&r+n<=s.rva+s.rawSize);if(!s)throw Error('PE bounds');return dll.subarray(s.raw+r-s.rva,s.raw+r-s.rva+n);}
const methods=cp.execFileSync(process.execPath,[path.join(archivePaths.workDir,'inspect.cjs'),'manifest'],{encoding:'utf8'}).trim().split('\n').map(JSON.parse);
if(methods.length!==12||methods.filter(x=>!x.reused).reduce((a,x)=>a+x.bytes,0)!==6352)throw Error('budget');
const anchors=[];
function call(va,target,label){const b=bytes(va,5);if(b[0]!==0xe8||va+5+b.readInt32LE(1)!==target)throw Error(label);anchors.push({va,target,label,bytes:5,sha256:sha(b)});}
function fixed(va,hex,label){const b=Buffer.from(hex,'hex');if(!bytes(va,b.length).equals(b))throw Error(label);anchors.push({va,label,bytes:b.length,sha256:sha(b)});}
for(const [va,target,label] of [
 [0x1030f10c,0x10223920,'Init commits shared level and display values'],[0x10223955,0x10224cd0,'refresh old definition params'],[0x10223c21,0x10221430,'old level user threshold'],[0x10223c4c,0x10224cd0,'refresh new level params'],[0x1022402c,0x10224cd0,'final definition refresh'],
 [0x1031a684,0x1025f8f0,'phase0 frame1 jingle20'],[0x1031a6a2,0x10747e60,'confirm pulse'],[0x1031a76c,0x107e3a20,'final confirm Pop'],
 [0x1034e3e3,0x10252050,'222x170 window'],[0x1034e40b,0x1024b010,'box15 139 224 202'],[0x1034e478,0x102a4780,'phase0 reveal height'],[0x1034e4ac,0x1075df70,'message clip'],[0x1034e7ae,0x10770590,'message clip retired'],
 [0x1034e5a1,0x1077e930,'measure level text'],[0x1034e5ac,0x1077e930,'measure translated prefix'],[0x1034e5b9,0x1077e930,'measure translated suffix'],[0x1034e65b,0x1024fcb0,'nonJapanese level SEB16'],[0x1034e759,0x1024fcb0,'Japanese level SEB16'],
 [0x1034e809,0x1030a180,'phase0 cursor at212 204'],[0x1034e828,0x102d7cd0,'shared strength renderer dispCombi false'],
 [0x1034e87d,0x102a43f0,'actors jump distance6 time20'],[0x1034e913,0x102516f0,'left shadow mini80 image93'],[0x1034e946,0x1030d8c0,'left character5'],[0x1034e9a7,0x102516f0,'right shadow'],[0x1034e9be,0x1030d8c0,'right character6'],
 [0x102d7d7c,0x10760a00,'event image14 background'],[0x102d7dce,0x10760a00,'event image16 mini background'],[0x102d7dfd,0x1075df70,'mapchip clip80 74 80 64'],[0x102d7e1d,0x1021b9c0,'mapchip2 center120 105 direction0'],[0x102d7e25,0x10770590,'mapchip clip retirement'],
 [0x102d8080,0x102e2b90,'nonJapanese label font9'],[0x102d80e4,0x102c9380,'restore label font'],[0x102d83bc,0x102a4620,'special count interpolation'],[0x102d84b4,0x102a43f0,'delta hop distance8 time12'],
 [0x102d851b,0x10221570,'current displayed value MAX check'],[0x102d85e4,0x1024fcb0,'quality charm values SEB15'],[0x102d8625,0x10255210,'price value money SEB15'],[0x102d8681,0x10255410,'quality charm delta SEB12'],[0x102d86c9,0x10255410,'price delta SEB12'],[0x102d8733,0x107bc710,'price delta currency frame20'],[0x102d87bc,0x102cfe40,'phase1 cursor uses frame minus55'],
 [0x1030a1ef,0x102516f0,'phase0 arrow01 frame0'],[0x102cfee0,0x102516f0,'phase1 arrow01 frame0'],[0x1030db5d,0x102516f0,'mini character selected frame'],
 [0x103477f2,0x10896a40,'touch flag2'],[0x10347816,0x1023a130,'touch component2'],[0x102a46f7,0x1024a080,'count interpolation effective endpoint clamp']
])call(va,target,label);
fixed(0x1032a4d9,'6a02','81 zero initialized soft label pair');fixed(0x1032a50f,'8d8654010000','softLabels outer index81');
fixed(0x1032c3cf,'c7411028000000','phase0 gate40');fixed(0x1032c3e0,'c7411432000000','phase0 cursor50');fixed(0x1032c3f0,'8988c4000000','publish TIME1 c4');fixed(0x1032c489,'83c00a','phase1 second value last item time plus10');fixed(0x1032c498,'8990c8000000','publish TIME2 c8');
fixed(0x1031a760,'c6809800000000','clear shared isLvUp flag only at close');fixed(0x1031a810,'c780d800000001000000','confirm phase0 becomes phase1');fixed(0x1031a81d,'c780a000000000000000','phase transition resets frame not frame2');
fixed(0x10223c2b,'89b794000000','deduct old threshold from cumulative users');fixed(0x10223c46,'898784000000','commit shared definition level');
fixed(0x10002640,'558bec8b45083b410c73088b4481105dc20400','direct called array helper reads requested index; callers push0');
const prior=JSON.parse(read('work/facility-animation-closure/animation-tables.json')),time=prior.tables.find(t=>t.name==='TENANTITEM_ANIME_TIME2');
const tb=meta.subarray(time.dataOffset,time.dataOffset+time.length*4);if(sha(tb)!==time.bytes_sha256||JSON.stringify(Array.from({length:time.length},(_,i)=>tb.readInt32LE(i*4)))!=='[2,32,34,49,55]')throw Error('original shared timing');
const timings={item_time2:{values:time.values,data_offset:time.dataOffset,sha256:sha(tb)},upgrade_time1:[40,50],upgrade_time2:[time.values.at(-1),time.values.at(-1)+10]};
const lo=meta.readUInt32LE(8),ln=meta.readUInt32LE(12),dt=meta.readUInt32LE(16),dn=meta.readUInt32LE(20);
const strings=[0x110f410c,0x110ea1a8,0x110ea494,0x110f40e4,0x110fe208,0x110f1a58,0x110fc7f8].map(va=>{const v=bytes(va,4).readUInt32LE(),index=(v>>>1)&0xfffffff;if(v>>>29!==5||index*8+8>ln)throw Error('literal usage');const n=meta.readUInt32LE(lo+index*8),at=meta.readUInt32LE(lo+index*8+4);if(n>512||at+n>dn)throw Error('literal bounds');return {va,index,text:meta.toString('utf8',dt+at,dt+at+n)};});
const coverage=JSON.parse(read('assets/IMAGE_COVERAGE.json')),resources=[];
for(const [group,kind,ids] of [['common','image',[28,29,30,72,93,103,105,106,129]],['common','sprite',[2,6,12,15,16,80]],['event','image',[14,16]]]){
const records=coverage.records.filter(r=>r.source==='EXE'&&r.group===group),inf=records.find(r=>r.entry===(kind==='image'?'img.inf':'seb.inf'));
for(const id of ids){const link=inf.index_targets.find(t=>t.index===id),r=records.find(x=>x.id===link.record),file=(group==='common'&&kind==='image'&&id===103?'assets/steam-build-common/original/common/':group==='common'&&kind==='image'&&id===105?'assets/steam-facility-common/original/common/':'assets/original/'+group+'/')+link.filename,b=read(file);if(sha(b)!==r.sha256)throw Error('Steam alias '+file);
const entry={group,kind,index:id,source:r.id,path:file,bytes:b.length,sha256:sha(b),width:r.width,height:r.height};
if(kind==='sprite'){const count=b.readInt16BE(4);if(b.readInt16BE(0)!==1||8+count*20!==b.length)throw Error('SEB');entry.frames=b.readInt16BE(2);entry.parts=Array.from({length:count},(_,i)=>Array.from({length:10},(_,j)=>b.readInt16BE(8+i*20+j*2)));}resources.push(entry);
}}
for(const r of resources.filter(x=>x.kind==='sprite'))for(const p of r.parts){const im=resources.find(x=>x.group===r.group&&x.kind==='image'&&x.index===p[1]);if(!im||p[2]<0||p[3]<0||p[2]+p[4]>im.width||p[3]+p[5]>im.height)throw Error('SEB dependency/crop');}
const floats=[0x10dde62c,0x10dde528].map(va=>({va,value:bytes(va,4).readFloatLE(),hex:bytes(va,4).toString('hex')}));
const reused=['work/facility-animation-closure/animation-tables.json','work/steam-facility-detail/EVIDENCE.json','work/steam-facility-labels/EVIDENCE.json','work/persistence-replay-analysis/exe/dumper/dump.cs','ui/EQUIPMENT_FACILITY_RENDER.md'].map(p=>({path:p,bytes:read(p).length,sha256:sha(read(p))}));
const branches=[[0x1034e3c2,0x1034e9cb,'raw81 full local drawing'],[0x1031a662,0x1031a887,'raw81 input'],[0x1030f0e8,0x1030f11d,'raw81 Init2']].map(([start,end,label])=>({start,end,label,bytes:end-start,sha256:sha(bytes(start,end-start))}));
const colors=[[0x1032bc13,0x1032bc3a,'SC_WINDOW_BLACK',[92,51,31]],[0x1032bc86,0x1032bcc0,'SC_WINDOW_BLUE',[0,100,255]]].map(([start,end,name,rgb])=>({start,end,name,rgb,sha256:sha(bytes(start,end-start))}));
const out={scope:'Steam raw81 exact local drawing/inputs and shared level consumer; static only',sources:{dll:sha(dll),metadata:sha(meta)},methods,new_method_bytes:6352,direct_helper_bytes:19,branches,anchors,timings,strings,resources,floats,colors,reused,
 contract:{raw:81,softLabels:[0,0],window:[222,170,0],box:[15,139,224,202],jingle:20,confirmed_close_clears_definition_flag:true,level_committed_at:'Init2',phase0_frame_for_strength:-1,dispCombi:false,touch_component:2,touch_flags:2},
 limits:['No original window/pixel validation or maintained implementation','Full popup retirement and OS handling not expanded','Raw timing slots are not seconds','Original static display arrays not a maintained transactional Owner','countAnime is not ordinary lerp'],scripts:['inspect.cjs','audit.cjs'].map(p=>{const b=fs.readFileSync(path.join(archivePaths.workDir,p));if(b[0]===0xef||b.includes(13))throw Error('LF no BOM');return {path:p,bytes:b.length,sha256:sha(b)};})};
fs.writeFileSync(path.join(archivePaths.workDir,'EVIDENCE.json'),JSON.stringify(out,null,2)+'\n');console.log(JSON.stringify({new_bytes:6352,anchors:anchors.length,resources:resources.length,timings,strings,floats}));
