// 仅核已命名局部的原常量、索引、调用及资源；不把40KiB静态构造器全部称为已研究。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
if(process.argv.length!==2)throw Error('no args');
const root=path.resolve(__dirname,'../..'),read=p=>fs.readFileSync(path.join(root,p)),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=read('DungeonVillageEXE/GameAssembly.dll'),meta=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'),ix=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('fixed sources');
const manifest=cp.execFileSync(process.execPath,[path.join(__dirname,'inspect.cjs'),'manifest'],{encoding:'utf8'}).trim().split('\n').map(JSON.parse);
function bytes(va,n){const rva=va-ix.base,s=ix.sections.find(s=>rva>=s.rva&&rva+n<=s.rva+s.rawSize);if(!s)throw Error('PE bounds');return dll.subarray(s.raw+rva-s.rva,s.raw+rva-s.rva+n);}
const iced=require('../local-tools/iced-x86-1.21.0/package'),instructions=new Map();
for(const m of manifest){const decoder=new iced.Decoder(32,bytes(ix.base+m.rva,m.bytes),iced.DecoderOptions.None);decoder.ip=BigInt(ix.base+m.rva);const formatter=new iced.Formatter(iced.FormatterSyntax.Intel);while(decoder.canDecode){const i=decoder.decode();if(i.isInvalid){i.free();break;}instructions.set(Number(i.ip),{va:Number(i.ip),length:i.length,text:formatter.format(i),target:i.isCallNear?Number(i.nearBranchTarget):null});i.free();}formatter.free();decoder.free();}
const anchors=[];
function exact(va,text){const i=instructions.get(va);if(!i||i.text!==text)throw Error('instruction '+va.toString(16)+' '+i?.text);anchors.push({...i,sha256:sha(bytes(va,i.length))});}
for(const [va,text] of [
 [0x102968da,'push 2'],[0x102968f4,'push 2'],[0x10296966,'push 2'],
 [0x1029691a,'mov dword ptr [edi+10h],0FFFFFFF2h'],[0x1029692b,'mov dword ptr [edi+14h],0FFFFFFE4h'],
 [0x1029698a,'mov dword ptr [edi+10h],5'],[0x1029699b,'mov dword ptr [edi+14h],0FFFFFFE4h'],
 [0x1029695a,'lea eax,[esi+10h]'],[0x1029695d,'mov [eax],edi'],[0x102969c2,'lea eax,[esi+14h]'],[0x102969c5,'mov [eax],edi'],
 [0x102969d7,'mov [eax+0A0h],esi'],[0x102d0309,'xor esi,esi'],[0x102d030b,'cmp eax,2'],[0x102d030e,'je short 102D0315h'],
 [0x102d0310,'cmp eax,3'],[0x102d0313,'jne short 102D031Ah'],[0x102d0315,'mov esi,1'],
 [0x102d035c,'mov eax,[eax+0A0h]'],[0x102d0373,'mov edx,[eax+esi*4+10h]'],[0x102d0389,'mov edx,[edx+10h]'],
 [0x102d0394,'mov eax,[eax+esi*4+10h]'],[0x102d03a2,'mov esi,[eax+14h]'],[0x102d03e2,'push 0'],
 [0x102d03e7,'lea eax,[esi+79h]'],[0x102d03ef,'add eax,edi'],[0x102d03f1,'add eax,[ebp-8]']])exact(va,text);
const call=instructions.get(0x102d03f9);if(!call||call.target!==0x102516f0)throw Error('DrawSeb consumer');anchors.push({...call,sha256:sha(bytes(call.va,call.length))});
const initRange=[0x102968da,0x102969f0],consumerRange=[0x102d02d0,0x102d0401];
if([...instructions.values()].some(i=>i.va>=initRange[0]&&i.va<initRange[1]&&i.target===0x1096ae10))throw Error('unexpected blob initializer');
const values=[[bytes(0x1029691a,7).readInt32LE(3),bytes(0x1029692b,7).readInt32LE(3)],[bytes(0x1029698a,7).readInt32LE(3),bytes(0x1029699b,7).readInt32LE(3)]];
if(JSON.stringify(values)!=='[[-14,-28],[5,-28]]')throw Error('offset constants');
const dumpPath='work/persistence-replay-analysis/exe/dumper/dump.cs',dump=read(dumpPath).toString('utf8');
if(!dump.includes('public static readonly int[][] OFF_EFPINCE; // 0xA0'))throw Error('named static field mapping');
const oldPath='work/steam-human-presentation/EVIDENCE.json',old=JSON.parse(read(oldPath));
if(old.reused_portrait.sha256!==manifest.find(x=>x.key==='portrait').sha256)throw Error('portrait evidence mismatch');
const resourceRecords=old.resources.filter(r=>(r.kind==='image'&&r.index===22)||(r.kind==='sprite'&&r.index===39));
if(resourceRecords.length!==2)throw Error('resource reference count');
for(const r of resourceRecords)if(sha(read(r.path))!==r.sha256)throw Error('resource '+r.path);
const sprite=resourceRecords.find(r=>r.kind==='sprite'),frame0=sprite.parts.filter(p=>p[0]===0);
if(frame0.length!==1||JSON.stringify(frame0[0])!=='[0,22,0,0,11,13,0,0,0,0]')throw Error('pinch frame0 source/offset');
const apk=[];
for(const [p,first,last,needles] of [
 ['work/decompiled/sources/c/b.java',141,141,['public static final int[][] bD = {new int[]{-14, -28}, new int[]{5, -28}};']],
 ['work/decompiled/sources/c/n.java',1645,1660,['int i10 = eVar.D[2] != 1 ? -12 : 0;','char c2 = (i9 == 2 || i9 == 3) ? (char) 1 : (char) 0;','b.bD[c2][0] + iA + i10','b.bD[c2][1] + 121']]]){
 const b=read(p),lines=b.toString('utf8').split(/\r?\n/),selected=lines.slice(first-1,last).join('\n');for(const n of needles)if(!selected.includes(n))throw Error('APK local crosscheck '+n);
 apk.push({path:p,first,last,source_sha256:sha(b),window_sha256:sha(Buffer.from(selected)),grade:'generated Java local crosscheck; no new DEX instruction audit'});
}
const scripts=['inspect.cjs','audit.cjs'].map(p=>{const b=fs.readFileSync(path.join(__dirname,p));if(b[0]===239||b.includes(13))throw Error('script UTF8 LF');return {path:p,bytes:b.length,sha256:sha(b)};});
const evidence={scope:'Steam raw60 OFF_EFPINCE static offsets and direction consumer; no whole-world pinch equivalence',sources:{dll_sha256:sha(dll),metadata_sha256:sha(meta),method_index_sha256:sha(read('work/persistence-replay-analysis/exe/methods.json')),dump_sha256:sha(read(dumpPath))},methods:manifest.map(m=>({...m,identity_only:m.key==='cctor',reused:m.key==='portrait'})),windows:[{key:'offset_initializer',begin:initRange[0],end:initRange[1],bytes:initRange[1]-initRange[0],sha256:sha(bytes(initRange[0],initRange[1]-initRange[0]))},{key:'portrait_pinch_consumer',begin:consumerRange[0],end:consumerRange[1],bytes:consumerRange[1]-consumerRange[0],sha256:sha(bytes(consumerRange[0],consumerRange[1]-consumerRange[0])),reused:true}],anchors,field:{owner:'game.Character2',name:'OFF_EFPINCE',static_offset:160,shape:[2,2],initialization:'four signed int32 immediates, not RuntimeFieldHandle/InitializeArray',values},consumer:{row1_directions:[2,3],other_direction_row:0,base_y:121,sprite:39,image:22,frame:0,derived_y:93,x_expression:'MOVE_DATA interpolated x + house offset + OFF_EFPINCE[row][0]'},resources:resourceRecords,reused:{path:oldPath,sha256:sha(read(oldPath))},apk_crosscheck:apk,scripts,limitations:['No original window pixel/click acceptance','No whole Character2 static constructor semantic coverage','APK evidence here is Java-local, not new DEX audit','This fixes raw60 portrait pinch placement, not all world Character2 effects','GetAnimeIndex and body/weapon tree remain separate']};
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(evidence,null,2)+'\n');console.log(JSON.stringify({initializer_bytes:278,reused_consumer_bytes:305,anchors:anchors.length,offsets:values,resources:resourceRecords.length,apk_windows:apk.length}));
