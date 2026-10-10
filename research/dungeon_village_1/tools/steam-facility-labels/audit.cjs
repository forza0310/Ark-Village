const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// raw74软键静态表与实际确认/取消消费者：有界摘要，不操作游戏或旧证据。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),cp=archivePaths.require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(archivePaths.workDir,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex'),read=p=>fs.readFileSync(path.join(root,p));
const dll=read('DungeonVillageEXE/GameAssembly.dll'),meta=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const ix=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
function bytes(va,n){const r=va-ix.base,s=ix.sections.find(s=>r>=s.rva&&r+n<=s.rva+s.rawSize);if(!s)throw Error('PE range');return dll.subarray(s.raw+r-s.rva,s.raw+r-s.rva+n);}
const reused=[];
for(const [file,rvas] of [['facility-animation-closure',[0x3285e0]],['steam-interaction-analysis',[0x312460,0x317940]]]){
const p='work/'+file+'/disassembly.json',b=read(p),a=JSON.parse(b),methods=[];
for(const rva of rvas){const m=a.find(x=>x.method.rva===rva);if(!m||sha(bytes(m.method.va,m.size))!==m.sha256)throw Error('cached method identity');methods.push({method:m.method,bytes:m.size,sha256:m.sha256});}
reused.push({path:p,bytes:b.length,sha256:sha(b),methods});
}
const prefix=JSON.parse(cp.execFileSync(process.execPath,[path.join(archivePaths.workDir,'inspect.cjs'),'manifest'],{encoding:'utf8'}));
const helpers=['softDefault','softKey'].map(key=>JSON.parse(cp.execFileSync(process.execPath,[path.join(root,'work/steam-build-resource-install/inspect.cjs'),'manifest'],{encoding:'utf8'}).trim().split('\n').find(x=>JSON.parse(x).key===key)));
const anchors=[];
function fixed(va,hex,label){const b=Buffer.from(hex,'hex');if(!bytes(va,b.length).equals(b))throw Error(label);anchors.push({va,label,bytes:b.length,sha256:sha(b)});}
function call(va,target,label){const b=bytes(va,5);if(b[0]!==0xe8||va+5+b.readInt32LE(1)!==target)throw Error(label);anchors.push({va,target,label,bytes:5,sha256:sha(b)});}
fixed(0x10328a07,'6a65','outer softLabels array length101');
fixed(0x1032a27c,'6a02','new raw74 zero-filled int[2]');
fixed(0x1032a2a0,'c7471402000000','raw74 element1=2; element0 remains default0');
fixed(0x1032a2bd,'837e0c4a','index74 bounds');
fixed(0x1032a2c7,'8d8638010000','outer slot74 address0x10+74*4');
fixed(0x1032aae6,'8930','publish SubForm static offset0');
fixed(0x102685ff,'6a0f','AppData soft label string table15');
fixed(0x10268619,'8b151cb90f11','label0 empty literal');
fixed(0x10268649,'8d4610','label0 element address');
fixed(0x10268694,'8b0d206e0f11','label2 original return literal');
fixed(0x102686c7,'8d4618','label2 element address');
fixed(0x102689d0,'89704c','publish AppData static offset4c');
for(const [a,t,s] of [[0x103126ed,0x107e3bf0,'Init sets left/right from tables, soft3/4 null'],
 [0x1031b597,0x10747e60,'raw74 actual confirm checks pulse100000'],[0x1031f653,0x10259b10,'raw74 cancellation label2'],[0x1031f668,0x107e3a20,'raw74 cancel Pop'],
 [0x10259b1d,0x10259910,'one-arg soft helper passes key0'],[0x102599b7,0x108e7fa0,'left displayed label text equality'],[0x10259a7f,0x108e7fa0,'right displayed label text equality'],
 [0x102599f2,0x10747e60,'matching left consumes pulse200000'],[0x10259ab6,0x10747e60,'matching right consumes pulse400000']])call(a,t,s);
const lo=meta.readUInt32LE(8),ln=meta.readUInt32LE(12),dt=meta.readUInt32LE(16),dn=meta.readUInt32LE(20);
const strings=[0x110fb91c,0x110f6e20].map(va=>{const v=bytes(va,4).readUInt32LE(),index=(v>>>1)&0xfffffff;if(v>>>29!==5||index*8+8>ln)throw Error('literal');const n=meta.readUInt32LE(lo+index*8),at=meta.readUInt32LE(lo+index*8+4);if(n>64||at+n>dn)throw Error('literal bounds');return {va,index,text:meta.toString('utf8',dt+at,dt+at+n)};});
if(strings[0].text!==''||strings[1].text!=='戻る')throw Error('raw labels');
const windows=[[0x1032a27c,0x1032a2d6,'raw74 int pair and publication in outer array'],[0x1032aadd,0x1032aaf5,'outer array published to SubForm static0'],[0x10312643,0x103126f2,'Init maps pair through AppData strings then SetSoftLabel'],[0x102685ff,0x102686d3,'label string elements0 through2'],[0x102689c7,0x102689e4,'string table published to AppData4c']].map(([start,end,label])=>({start,end,label,bytes:end-start,sha256:sha(bytes(start,end-start))}));
const out={scope:'Steam raw74 initial left blank/right return and logical input qualification; not OS/window certification',sources:{dll:sha(dll),metadata:sha(meta)},new_prefix:prefix,reused,helpers,windows,anchors,strings,
 contract:{raw:74,softLabels:[0,2],left:'',right:'戻る',soft3:null,soft4:null,confirm_pulse:0x100000,return_pulse:0x400000,raw74_queries_label0:false},
 limits:['No entire framework lifecycle or physical hit rectangle investigation','Actual translated screen label/focus/platform mapping remains separate','Empty left soft label is not an inferred confirm button','No frozen evidence or C++ mutation'],scripts:['inspect.cjs','audit.cjs'].map(p=>{const b=fs.readFileSync(path.join(archivePaths.workDir,p));if(b[0]===0xef||b.includes(13))throw Error('LF no BOM');return {path:p,bytes:b.length,sha256:sha(b)};})};
fs.writeFileSync(path.join(archivePaths.workDir,'EVIDENCE.json'),JSON.stringify(out,null,2)+'\n');console.log(JSON.stringify({new_bytes:prefix.bytes,anchors:anchors.length,raw74:out.contract}));
