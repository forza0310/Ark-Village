// 原图发布与动态资源安装/旋转准入的具名静态证据；不运行原游戏。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(__dirname,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex'),read=p=>fs.readFileSync(path.join(root,p));
const dll=read('DungeonVillageEXE/GameAssembly.dll'),meta=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const idx=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
function bytes(va,n){const r=va-idx.base,s=idx.sections.find(s=>r>=s.rva&&r+n<=s.rva+s.rawSize);if(!s)throw Error('PE range');return dll.subarray(s.raw+r-s.rva,s.raw+r-s.rva+n);}
const methods=cp.execFileSync(process.execPath,[path.join(__dirname,'inspect.cjs'),'manifest'],{encoding:'utf8'}).trim().split('\n').map(JSON.parse);
const anchors=[];
function call(va,target,label){const b=bytes(va,5);if(b[0]!==0xe8||va+5+b.readInt32LE(1)!==target)throw Error(label);anchors.push({va,bytes:5,sha256:sha(b),target,label});}
function fixed(va,hex,label){const b=Buffer.from(hex,'hex');if(!bytes(va,b.length).equals(b))throw Error(label);anchors.push({va,bytes:b.length,sha256:sha(b),label});}
for(const [va,target,label] of [
 [0x102fa99d,0x107ae1d0,'dispose previous mapchip manager'],[0x102fa9d2,0x107b2610,'construct replacement manager'],
 [0x102faa05,0x10727e00,'read resource record1/2'],[0x102faa1b,0x107b0b20,'load mapchip bytes'],
 [0x102fa084,0x107ae1d0,'finish disposes mapchip manager'],[0x10727e93,0x10727ef0,'static record read delegates instance'],
 [0x10727fd4,0x1072aa70,'resource record reads media3'],[0x1072ab36,0x1072a760,'storage static delegates instance'],
 [0x1072a9a5,0x10717b70,'media3 reads asset data'],[0x10717d01,0x1078b340,'GetData waits UI callback'],
 [0x1072dc60,0x10717050,'asset candidates from GetAccessFilesList'],[0x1072dc9a,0x10718140,'first successful asset candidate'],
 [0x1072da3f,0x10c12c90,'first Unity Resources.Load attempt'],[0x1072db39,0x10c12c90,'extension-stripped Unity retry'],
 [0x1072d8d6,0x10c169d0,'TextAsset byte copy'],[0x1072d8ef,0x10c12d50,'success unloads TextAsset'],
 [0x1072d924,0x10c12d50,'exception path unloads TextAsset'],[0x10717dfd,0x10841d60,'conditional dat decode'],
 [0x107b0b9b,0x1084b2e0,'JarInflater byte constructor'],[0x107b0bac,0x107aefd0,'resource LoadReady'],
 [0x107b0bbd,0x107af930,'resource LoadStart'],[0x107b0bd6,0x1084a2a0,'success closes inflater'],
 [0x107b0c0e,0x1084a2a0,'exception closes inflater'],
 [0x102f659d,0x10837c30,'FLAG_INVERSION mask check'],[0x102f6648,0x107e3bf0,'set build soft labels'],
 [0x10259b1d,0x10259910,'default extra key0'],[0x102599f2,0x10747e60,'matching left label pulse'],
 [0x10259ab6,0x10747e60,'matching right label pulse'],[0x1030473c,0x10259b10,'game update checks rotate label9']
])call(va,target,label);
fixed(0x102fa9e2,'8d4320','resMapChip field address');fixed(0x102fa9e5,'8938','publish manager before load');
fixed(0x102faa01,'6a026a01','record IDs2 then1');fixed(0x102fa09d,'c70000000000','finish clears mapchip reference');
fixed(0x102f659a,'6a20','inversion mask32');fixed(0x102f65fa,'ff7034','soft label index9');
fixed(0x1030476f,'2b4130894130','toggle inversion1-old');
fixed(0x10258d14,'8b7014','RSFILENAMESS outer index1');fixed(0x10258d36,'897008','install RecordStore resource names');
fixed(0x10267cbc,'8908','RSFILENAMESS1 index2 string store');fixed(0x10267f5e,'8930','publish resource subarray at index1');
fixed(0x10267f70,'897838','publish RSFILENAMESS');
const lo=meta.readUInt32LE(8),ln=meta.readUInt32LE(12),data=meta.readUInt32LE(16),dn=meta.readUInt32LE(20);
function literal(va,expected){const v=bytes(va,4).readUInt32LE(),i=(v>>>1)&0xfffffff;if(v>>>29!==5||i*8+8>ln)throw Error('literal reference');const n=meta.readUInt32LE(lo+i*8),at=meta.readUInt32LE(lo+i*8+4);if(n>128||at+n>dn)throw Error('literal size');const text=meta.toString('utf8',data+at,data+at+n);if(text!==expected)throw Error('literal value');return {va,index:i,text};}
const literals=[literal(0x110f4690,'image'),literal(0x111037bc,'.dat'),literal(0x110fb91c,'')];
const rows=read('work/exe-assessment/extracted/xls/Japanese.lproj/tenantData.txt').toString('utf8').trimEnd().split(/\r?\n/).map(r=>r.split('\t'));
if(rows.length!==85||rows.some(r=>r.length!==36))throw Error('tenant table');
const inversion=rows.filter(r=>(Number(r[35])&32)!==0).map(r=>Number(r[0]));
const buildInversion=rows.filter(r=>(Number(r[35])&36)===36).map(r=>Number(r[0]));
const mapEvidence=JSON.parse(read('work/steam-mapchip-patterns/RESOURCES.json'));
if(inversion.length!==57||buildInversion.length!==46)throw Error('flags denominator');
for(const id of buildInversion){const row=mapEvidence.joins.find(r=>r.tenant_id===id);if(!row||row.directions.some(d=>d.pieces.some(p=>p.resolution!=='exact'||!p.crops_inside_images)))throw Error('buildable inversion resource mismatch');}
const publishOutput=cp.execFileSync('python',['-B',path.join(root,'tools/scripts/publish_steam_build_common.py'),'--check'],{encoding:'utf8'}).trim();
const published=JSON.parse(publishOutput);if(published.new_payload_bytes!==1213||published.created!==0)throw Error('published assets check');
const reused=[['work/headless-save-audit-analysis/steam-static/disassembly.json',[[0x10258cf5,0x10258d4a],[0x10267bf1,0x10267f84]]],['work/restore-behavior-analysis/steam-calendar/calendar-disassembly.json',[[0x103046dc,0x1030477a]]]].map(([p,windows])=>({path:p,sha256:sha(read(p)),windows:windows.map(([start,end])=>({start,end,bytes:end-start,sha256:sha(bytes(start,end-start))}))}));
const files=['work/steam-build-resource-install/inspect.cjs','tools/scripts/publish_steam_build_common.py','tools/scripts/publish_steam_startup.py','assets/steam-build-common/MANIFEST.json','assets/steam-build-common/original/common/tenant_resident.png','assets/steam-build-common/original/common/number05.png'].map(p=>{const b=read(p);return {path:p,bytes:b.length,sha256:sha(b)};});
const result={scope:'Steam mapchip manager install/retirement and build soft-label rotation qualification; static only',sources:{dll:sha(dll),metadata:sha(meta)},methods,method_count:methods.length,method_bytes:methods.reduce((n,m)=>n+m.bytes,0),anchors,literals,reused,flags:{inversion_mask:32,buildable_mask:4,inversion_definitions:inversion,buildable_inversion_definitions:buildInversion,both_directions_exact_for_buildable_inversion:true},published,files,limits:['No full language fallback order or runtime active-language certification','No claim all definitions are unlocked/buildable at current date','No atomic replacement in original GameForm.Init; published manager precedes Load','ResourceManager Dispose internals/GPU lifetime and all asynchronous loading remain separate','No game, saves, product modification, new build cache or window']};
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({methods:result.method_count,method_bytes:result.method_bytes,anchors:anchors.length,new_image_bytes:1213,rotatable:57,buildable_rotatable:46}));
