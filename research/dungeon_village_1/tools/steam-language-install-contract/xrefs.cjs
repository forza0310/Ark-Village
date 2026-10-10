const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 固定注册方法直接调用候选；仅解从已登记调用者入口至候选，不解整DLL。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const root=path.resolve(archivePaths.workDir,'../..'),base=path.join(root,'work/persistence-replay-analysis/exe');
const b=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll')),h=x=>crypto.createHash('sha256').update(x).digest('hex');
if(h(b)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a')throw Error('DLL identity');
const j=JSON.parse(fs.readFileSync(path.join(base,'methods.json'))),iced=archivePaths.require(path.join(root,'work/local-tools/iced-x86-1.21.0/package'));
const target=j.methods.find(m=>m.type==='kairo.unity.util.Language'&&m.signature==='public static void RegistTranslateTable(ThreadStart method) { }');
if(!target||target.va!==0x1081dc40)throw Error('target identity');
const methods=[...new Map(j.methods.map(m=>[m.va,m])).values()].sort((a,b)=>a.va-b.va),result=[];
for(const sec of j.sections.filter(s=>(s.flags&0x20000000)!==0)){
 if(sec.raw+sec.rawSize>b.length)throw Error('section range');
 for(let p=sec.raw;p+5<=sec.raw+sec.rawSize;p++){
  if(b[p]!==0xe8&&b[p]!==0xe9)continue;const va=j.base+sec.rva+p-sec.raw;
  if(va+5+b.readInt32LE(p+1)!==target.va)continue;
  const m=methods.findLast(m=>m.va<=va);if(!m||va-m.va>16384)throw Error('caller exceeds bounded review');
  const d=new iced.Decoder(32,b.subarray(m.offset,p+5),iced.DecoderOptions.None);d.ip=BigInt(m.va);let verified=false;
  while(d.canDecode){const i=d.decode();if(Number(i.ip)===va&&(i.isCallNear||i.isJmpShortOrNear)){verified=true;i.free();break;}i.free();}d.free();
  result.push({va,offset:p,caller:m,boundary_verified:verified,prefix_bytes:p+5-m.offset,prefix_sha256:h(b.subarray(m.offset,p+5))});
 }
}
console.log(JSON.stringify({dll_sha256:h(b),target,candidates:result,scope:'E8/E9 to fixed RegistTranslateTable; no indirect calls or inline-write completeness claim'},null,2));
