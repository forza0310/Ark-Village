// 固定Steam raw74静态核对：直接调用、分支字节、原文字及已出版素材引用；不运行游戏。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(__dirname,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex'),read=p=>fs.readFileSync(path.join(root,p));
const dll=read('DungeonVillageEXE/GameAssembly.dll'),meta=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const ix=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
function bytes(va,n){const r=va-ix.base,s=ix.sections.find(s=>r>=s.rva&&r+n<=s.rva+s.rawSize);if(!s)throw Error('PE range');return dll.subarray(s.raw+r-s.rva,s.raw+r-s.rva+n);}
const methods=cp.execFileSync(process.execPath,[path.join(__dirname,'inspect.cjs'),'manifest'],{encoding:'utf8'}).trim().split('\n').map(JSON.parse);
if(methods.length!==7||methods.filter(m=>!m.reused).reduce((n,m)=>n+m.bytes,0)!==19792)throw Error('method budget');
const anchors=[];
function call(va,target,label){const b=bytes(va,5);if(b[0]!==0xe8||va+5+b.readInt32LE(1)!==target)throw Error(label);anchors.push({va,target,label,bytes:5,sha256:sha(b)});}
function fixed(va,hex,label){const b=Buffer.from(hex,'hex');if(!bytes(va,b.length).equals(b))throw Error(label);anchors.push({va,label,bytes:b.length,sha256:sha(b)});}
for(const [a,t,s] of [
 [0x1030f8de,0x10809c10,'Init2 new ary1'],[0x1030f93f,0x10221250,'resident catalogue'],[0x1030f970,0x10809c10,'instance ary2'],
 [0x10345454,0x10200ad0,'numbered facility title'],[0x1034547a,0x10252050,'220x168 window'],[0x1034549f,0x1024b010,'content box'],[0x103454b2,0x1030db80,'only ordinary instance arrows'],
 [0x10345618,0x1030c350,'facility type icon mode9'],[0x103456cd,0x107cabd0,'nonJapanese name layout'],
 [0x1034587d,0x102215b0,'preview base price'],[0x1034588e,0x102214b0,'instance price plus bonus'],[0x1034589b,0x10221570,'instance price max'],
 [0x103455cc,0x102214b0,'page2 upkeep plus bonus'],[0x10345917,0x10255210,'price/upkeep money helper'],
 [0x10345af9,0x10239e20,'neighbor rows without marker16'],[0x10345abc,0x107bf8c0,'selected neighbor finger sprite'],[0x10346081,0x102517b0,'page2 five row scrollbar'],
 [0x1034619a,0x1075df70,'preview clip'],[0x103461b0,0x1021b9c0,'centered mapchip2 inversion0'],[0x103461b8,0x10770590,'preview popclip'],
 [0x10346620,0x102215b0,'preview charm'],[0x10346631,0x102215b0,'preview quality'],[0x10346657,0x102214b0,'instance quality/charm plus bonus'],
 [0x103467c0,0x1030c350,'use effect icon7'],[0x1034685a,0x107bc710,'effect dots number08 frame14'],[0x1034690d,0x10221430,'next level users threshold'],
 [0x103469aa,0x102516f0,'remaining users unit number05 frame18'],[0x10346b0f,0x102e6b90,'home resident display request'],[0x10346b3a,0x102ccea0,'home resident weapon draw'],
 [0x1034722b,0x1030a2c0,'bottom instruction helper'],[0x10347258,0x10896a40,'preview touch flag2'],[0x1034727c,0x1023a130,'preview touch component2'],
 [0x1031b420,0x10748250,'page2 up'],[0x1031b457,0x10748250,'page2 down'],[0x1031b51b,0x10748250,'left toggles'],[0x1031b559,0x10748250,'right toggles independently'],
 [0x1031b597,0x10747e60,'confirm pulse'],[0x1031fc7d,0x107e3a20,'preview confirm Pop'],[0x1031f653,0x10259b10,'soft label2'],[0x1031f668,0x107e3a20,'cancel Pop'],
 [0x1031b67f,0x1032ead0,'ordinary75 or resident80'],[0x1031b6ba,0x107e3a70,'push with actual instance and definition'],[0x1031b6f0,0x1032ead0,'equipment79'],
 [0x1031b843,0x102f6360,'home action state6'],[0x1031b873,0x107e3a20,'home action Pop'],
 [0x10305688,0x102c4330,'scene selectedTenant definition'],[0x10305704,0x1032ead0,'inhabited home opens60'],[0x10305885,0x1032ead0,'ordinary scene opens74'],[0x10305904,0x107e3a70,'scene Push74'],[0x10305930,0x1032ead0,'pending upgrade opens81'],
 [0x1021bc6f,0x107bc710,'mapchip2 same pattern frame stream'],[0x1030dc3d,0x10239950,'left arrow component1 value16'],[0x1030dc9a,0x10239950,'right arrow component1 value18'],
 [0x1030c6f0,0x10760c50,'mode9 icon_tenantInfo crop'],[0x1030c7ee,0x10760c50,'mode7 icon_param00 crop']
])call(a,t,s);
fixed(0x1030569a,'80bf9800000000','upgrade flag before construction gate');
fixed(0x103056a7,'837b1c00','construction state0 gate');
fixed(0x103058fa,'c787b800000000000000','scene value1=0');
fixed(0x1031b5f2,'83b8b800000001','preview exact value1=1 confirm');
fixed(0x1031b5ff,'83b8d800000001','page2 confirm returns false');
fixed(0x10346920,'2b8694000000','remaining users subtract tenantUserNum');
const literalVAs=[0x110f767c,0x110f7654,0x110f75ac,0x110f8dc0,0x110f8b74,0x110f4cb4,0x110f3c74,0x110f5e64,0x110fa9c8,0x11102660,0x110f7e5c,0x110f79f0,0x110ea59c,0x110fedc8,0x11101adc,0x110f856c,0x110f8614,0x110fb8b0,0x110fa680,0x110f7f04,0x110ea7d8,0x110f5e38,0x110fb808,0x110f5e10,0x110f4f28,0x110f6374,0x110ea0cc,0x110eb828];
const lo=meta.readUInt32LE(8),ln=meta.readUInt32LE(12),dt=meta.readUInt32LE(16),dn=meta.readUInt32LE(20);
const strings=literalVAs.map(va=>{const v=bytes(va,4).readUInt32LE(),index=(v>>>1)&0xfffffff;if(v>>>29!==5||index*8+8>ln)throw Error('literal usage');const n=meta.readUInt32LE(lo+index*8),at=meta.readUInt32LE(lo+index*8+4);if(n>1024||at+n>dn)throw Error('literal size');return {va,index,text:meta.toString('utf8',dt+at,dt+at+n)};});
const coverage=JSON.parse(read('assets/IMAGE_COVERAGE.json')),records=coverage.records.filter(r=>r.source==='EXE'&&r.group==='common'),resources=[];
for(const [kind,ids] of [['image',[28,29,30,37,38,70,72,74,91,103,105,106,129,178]],['sprite',[3,6,12,15,16,21]]]){
const inf=records.find(r=>r.entry===(kind==='image'?'img.inf':'seb.inf'));
for(const id of ids){const item=inf.index_targets.find(x=>x.index===id),record=records.find(r=>r.id===item.record);if(!record)throw Error('index');
 if(kind==='image'&&[37,105].includes(id)){const expected={37:'2b99ce3fa12e25e4d1fab78e06b6cd0ada87f8e3f4bf68c94c8421d7172c22e0',105:'8914321921983ba8508b86db5c499cb580df1a5468860d354a09cede9d9f8361'};if(record.sha256!==expected[id]||record.apk_same_name_bytes_equal!==false||record.apk_same_name_pixels_equal!==false)throw Error('unpublished Steam icon identity');resources.push({kind,index:id,source:record.id,path:null,bytes:record.bytes,sha256:record.sha256,width:record.width,height:record.height,publication:'not published; APK same name is not a usable alias'});continue;}
 const file=(id===103&&kind==='image'?'assets/steam-build-common/original/common/':'assets/original/common/')+item.filename,b=read(file);if(sha(b)!==record.sha256)throw Error('published Steam alias '+file);
 const r={kind,index:id,source:record.id,path:file,bytes:b.length,sha256:sha(b),width:record.width,height:record.height};
 if(kind==='sprite'){const count=b.readInt16BE(4);if(b.readInt16BE(0)!==1||8+count*20!==b.length)throw Error('SEB structure');r.frames=b.readInt16BE(2);r.parts=Array.from({length:count},(_,i)=>Array.from({length:10},(_,j)=>b.readInt16BE(8+i*20+j*2)));}
 resources.push(r);
}}
for(const r of resources.filter(r=>r.kind==='sprite'))for(const p of r.parts){const image=resources.find(x=>x.kind==='image'&&x.index===p[1]);if(!image||p[2]<0||p[3]<0||p[2]+p[4]>image.width||p[3]+p[5]>image.height)throw Error('sprite image dependency/crop');}
const reused=['work/steam-build-list-business/EVIDENCE.json','work/steam-mapchip-patterns/EVIDENCE.json','work/steam-mapchip-patterns/RESOURCES.json','work/steam-window-frame-contract/EVIDENCE.json','work/steam-ui-coverage/methods.json','work/persistence-replay-analysis/exe/dumper/dump.cs'].map(p=>{const b=read(p);return {path:p,bytes:b.length,sha256:sha(b)};});
const branches=[[0x1030f8c3,0x1030fc58,'Init2 raw74'],[0x103453b8,0x1034728b,'complete local raw74 draw'],[0x1031b3b6,0x1031b88b,'raw74 input local body'],[0x10305670,0x10305985,'scene facility entry']].map(([start,end,label])=>({start,end,label,bytes:end-start,sha256:sha(bytes(start,end-start))}));
const out={scope:'Steam raw74 definition/instance entries, local drawing and inputs; original window and full platform rendering remain separate',sources:{dll:sha(dll),metadata:sha(meta)},methods,new_method_bytes:19792,branches,anchors,strings,resources,reused,
 contracts:{preview_value1:1,scene_value1:0,window:[220,168,0],box:[17,74,219,184],ordinary_map_x:23,other_map_x:71,mapchip2_inversion:0,mapchip2_pattern_offsets:[[-30,0],[-45,7],[-30,15]],neighbor_visible_rows:5,neighbor_marker16:false,paid_action_in_raw74:false},
 limitations:['No original runtime input or pixel comparison this batch','Soft label table74 exact strings not newly decoded','Housing drawing branch existence does not prove scene reachability; inhabited homes go60','SubForm array retirement follows framework; no retained-count bound claim','Original private payload lacks maintained strict rejection; no C++ change'],
 scripts:['inspect.cjs','audit.cjs'].map(p=>{const b=fs.readFileSync(path.join(__dirname,p));if(b[0]===0xef||b.includes(13))throw Error('scripts must UTF8 no BOM LF');return {path:p,bytes:b.length,sha256:sha(b)};})};
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(out,null,2)+'\n');console.log(JSON.stringify({methods:methods.length,new_bytes:out.new_method_bytes,anchors:anchors.length,resources:resources.length,strings},null,2));
