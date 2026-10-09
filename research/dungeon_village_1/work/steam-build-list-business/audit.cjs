// raw21初始化/确认业务的有限源证据，保留空行和特殊模式差别，不改维护实现。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(__dirname,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex'),read=p=>fs.readFileSync(path.join(root,p));
const dll=read('DungeonVillageEXE/GameAssembly.dll'),meta=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const ix=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
function bytes(va,n){const r=va-ix.base,s=ix.sections.find(s=>r>=s.rva&&r+n<=s.rva+s.rawSize);if(!s)throw Error('PE range');return dll.subarray(s.raw+r-s.rva,s.raw+r-s.rva+n);}
const methods=cp.execFileSync(process.execPath,[path.join(__dirname,'inspect.cjs'),'manifest'],{encoding:'utf8'}).trim().split('\n').map(JSON.parse);
if(methods.length!==8||methods.reduce((n,m)=>n+m.bytes,0)!==30276)throw Error('fixed method budget');
const anchors=[];
function call(va,target,label){const b=bytes(va,5);if(b[0]!==0xe8||va+5+b.readInt32LE(1)!==target)throw Error(label);anchors.push({va,bytes:5,sha256:sha(b),target,label});}
function fixed(va,hex,label){const b=Buffer.from(hex,'hex');if(!bytes(va,b.length).equals(b))throw Error(label);anchors.push({va,bytes:b.length,sha256:sha(b),label});}
for(const [va,target,label] of [
 [0x103147ec,0x10809c10,'construct each category Vector'],[0x103148df,0x10837c30,'FLAG_BUILDABLE mask4'],
 [0x103149e3,0x10656290,'insert destruct at1'],[0x103149fa,0x102c9470,'event flag32 controls move'],
 [0x10314a61,0x10656290,'insert move at2'],[0x10314a78,0x102ef490,'Update_residentFlg(false)'],
 [0x10314afa,0x10320d60,'initial selected definition soft labels'],[0x103259a8,0x107e5820,'draw before frame gate'],
 [0x10325ae0,0x10748250,'up repeat'],[0x10325b05,0x10748250,'down repeat'],
 [0x10325b5b,0x10748250,'left repeat'],[0x10325b7e,0x10748250,'right repeat'],
 [0x10325c92,0x10320d60,'new selected ID changes soft labels'],[0x10325cad,0x10747e60,'logical confirm pulse'],
 [0x10325ccd,0x10259b10,'cancel soft label2'],[0x10324952,0x107e3a20,'cancel Pop'],
 [0x10325ced,0x10259b10,'detail soft label7'],[0x10325d43,0x1032ead0,'construct raw74 detail'],
 [0x10325d65,0x107e3a70,'push raw74'],[0x10325df6,0x10221360,'ordinary quote GetBuildCost'],
 [0x10323044,0x10255700,'insufficient cash ExecEvent11'],[0x10325ee0,0x102202a0,'ordinary/road ChangeTBMode'],
 [0x10325f34,0x102f6360,'ordinary ChangeState BUILD'],[0x10325f5a,0x107e4a50,'ordinary ChangeCurrentForm'],
 [0x10325f77,0x102e6df0,'refresh remaining build NEW'],[0x103260f4,0x10315de0,'first-time helper talk'],
 [0x10326130,0x102202a0,'move ChangeTBMode6'],[0x10326153,0x102f6360,'move BUILD'],
 [0x10326179,0x107e4a50,'move ChangeCurrentForm'],[0x103261c4,0x102202a0,'destruct ChangeTBMode3'],
 [0x103261e7,0x102f6360,'destruct BUILD'],[0x1032620d,0x107e4a50,'destruct ChangeCurrentForm'],
 [0x103262be,0x107e3bf0,'destruct explicit soft labels'],[0x102202e6,0x10222b90,'mode change resets help text'],
 [0x10316335,0x10320d60,'touch selection refresh'],[0x10320e5e,0x107e3bf0,'types2/3/13 detail label']
])call(va,target,label);
fixed(0x103148b4,'837e1000','nonzero BaseData state');
fixed(0x103148ef,'837e240c','housing type12');fixed(0x103148f5,'83be8800000000','positive housing stock prerequisite');
fixed(0x1031490c,'8b4e38','definition category field');fixed(0x103259b0,'83bba000000003','frame gate3 after draw');
fixed(0x10325a65,'be04000000','minimum selectable upper bound4');
fixed(0x10325b40,'f7fe','up-wrap selected modulo upper-bound');
fixed(0x10325bce,'83c005','five-row follow window');
fixed(0x10325e06,'bf2c010000','move quote300');fixed(0x10325f07,'897828','ordinary writes selected definition ID');
fixed(0x10325f12,'c7403000000000','ordinary resets inversion0');fixed(0x10325f5f,'c6461800','ordinary clears BaseData NEW');
fixed(0x10325d4e,'c786b800000001000000','detail value1=1');
fixed(0x102202e3,'89412c','ChangeTBMode only mode scalar');fixed(0x1032612e,'6a06','move mode6');fixed(0x103261c2,'6a03','destruct mode3');
const lo=meta.readUInt32LE(8),ln=meta.readUInt32LE(12),dt=meta.readUInt32LE(16),dn=meta.readUInt32LE(20);
const strings=[0x110eb510,0x110f13d0].map(va=>{const v=bytes(va,4).readUInt32LE(),i=(v>>>1)&0xfffffff;if(v>>>29!==5||i*8+8>ln)throw Error('literal usage');const n=meta.readUInt32LE(lo+i*8),at=meta.readUInt32LE(lo+i*8+4);if(n>1024||at+n>dn)throw Error('literal size');return {va,index:i,text:meta.toString('utf8',dt+at,dt+at+n)};});
const tablePath='work/exe-assessment/extracted/xls/Japanese.lproj/tenantData.txt',tableBytes=read(tablePath),rows=tableBytes.toString('utf8').trimEnd().split(/\r?\n/).map(x=>x.split('\t'));
if(rows.length!==85||rows.some(r=>r.length!==36))throw Error('table denominator');
const candidates=rows.filter(r=>(Number(r[35])&4)!==0).map(r=>({id:Number(r[0]),type:Number(r[3]),category:Number(r[8]),flags:Number(r[35])}));
const categories=[0,1,2].map(c=>candidates.filter(r=>r.category===c).length);
if(candidates.length!==50||JSON.stringify(categories)!=='[13,25,12]'||JSON.stringify(candidates.filter(r=>r.type===12).map(r=>r.id))!=='[25,27]')throw Error('definition filter candidate denominators');
// 与上一包已从Steam归档独立核出的原表身份相接，不将运行时state/haveNum伪造成表初值。
const mapResources=JSON.parse(read('work/steam-mapchip-patterns/RESOURCES.json'));
if(mapResources.tables['tenantData.txt'].sha256!==sha(tableBytes))throw Error('Steam original table identity');
const reused=['work/steam-input-closure/README.md','work/steam-build-list-render/EVIDENCE.json','work/steam-interaction-analysis/disassembly.txt','work/decompiled/sources/b/g.java'].map(p=>({path:p,bytes:read(p).length,sha256:sha(read(p))}));
if(reused[3].sha256!=='0eecb47f1996abc34e6fa1ffcbbeeb40d2a597f46168a2583833ed6fe1e33081')throw Error('APK identity');
const returnWindow={start:0x10328427,bytes:0x13,sha256:sha(bytes(0x10328427,0x13)),meaning:'shared true return; reused named Update evidence'};
const out={scope:'Steam raw21 Init/Update local business and shared touch qualification; no C++ changes',sources:{dll:sha(dll),metadata:sha(meta)},methods,unique_method_bytes:30276,branches:{init:{start:0x10314793,end:0x10314b0b,bytes:888},update:{start:0x1032599a,end:0x103262d9,bytes:2367}},anchors,strings,reused,returnWindow,table_filter:{path:tablePath,sha256:sha(tableBytes),candidates,category_counts:categories,qualification:'Only definition FLAG_BUILDABLE candidates; live state!=0 and housing inventory>0 still required; not current unlock list'},
 contracts:{categories:3,visible_rows:5,empty_selection_sentinel:-99,maximum_selection:'max(count-1,4)',frame_gate:'draw before checking frame>=3',moves:{destruct:{id:-1,mode:3,writes_build_id:false,writes_inversion:false},move:{id:-2,mode:6,quote:300,writes_build_id:false,writes_inversion:false},road:{type:6,mode:1,writes_build_id:true,inversion:0},ordinary:{flag:4,mode:0,writes_build_id:true,inversion:0}},detail:{raw:74,value1:1},debit_here:false},
 limitations:['Cannot replace original potentially invalid private state with permissive map access','No full ChangeCurrentForm/Pop parent-stack lifecycle or OS event certification','Markers can bind empty registered rows; confirm range gate remains required','Special move/destruct branches do not overwrite selected definition or orientation here','State update and DrawAllForms are not side-effect-free UI queries'],scripts:['inspect.cjs','audit.cjs'].map(p=>{const b=fs.readFileSync(path.join(__dirname,p));return {path:p,bytes:b.length,sha256:sha(b)};})};
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(out,null,2)+'\n');console.log(JSON.stringify({methods:8,bytes:30276,anchors:anchors.length,init_branch_bytes:888,update_branch_bytes:2367}));
