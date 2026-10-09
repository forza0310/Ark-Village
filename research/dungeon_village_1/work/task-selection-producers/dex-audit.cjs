// 复用项目既有有界DEX读器的字段/方法表逻辑；全表仅查5个具名字段引用，不输出整程序反编译。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),zlib=require('zlib');
const root=path.resolve(__dirname,'../..'),source='work/magic-pot-analysis/classes.dex';
const b=fs.readFileSync(path.join(root,source)),sha=x=>crypto.createHash('sha256').update(x).digest('hex');
const expected=JSON.parse(fs.readFileSync(path.join(root,'work/magic-pot-analysis/DEX_M.json'),'utf8')).dex_sha256;
if(b.length>16*1024*1024||b.toString('ascii',0,4)!=='dex\n'||sha(b)!==expected)throw Error('固定DEX身份');
// 只在内存核对固定APK中的classes.dex，不再生成DEX副本。
const apkPath='maoxianmigongcun.apk',apk=fs.readFileSync(path.join(root,apkPath));
if(apk.length>128*1024*1024||sha(apk)!=='1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5')throw Error('固定APK身份');
let eocd=-1;
for(let p=apk.length-22;p>=Math.max(0,apk.length-65557);--p)if(apk.readUInt32LE(p)===0x06054b50&&p+22+apk.readUInt16LE(p+20)===apk.length){eocd=p;break;}
if(eocd<0)throw Error('ZIP目录缺失');
let zipPos=apk.readUInt32LE(eocd+16),dexEntry;
for(let i=0;i<apk.readUInt16LE(eocd+10);++i){
 if(apk.readUInt32LE(zipPos)!==0x02014b50)throw Error('ZIP目录签名');
 const n=apk.readUInt16LE(zipPos+28),extra=apk.readUInt16LE(zipPos+30),comment=apk.readUInt16LE(zipPos+32);
 if(apk.toString('utf8',zipPos+46,zipPos+46+n)==='classes.dex'){
  if(dexEntry)throw Error('重复DEX项');
  dexEntry={method:apk.readUInt16LE(zipPos+10),packed:apk.readUInt32LE(zipPos+20),unpacked:apk.readUInt32LE(zipPos+24),local:apk.readUInt32LE(zipPos+42)};
 }
 zipPos+=46+n+extra+comment;
}
if(!dexEntry||dexEntry.unpacked!==b.length||apk.readUInt32LE(dexEntry.local)!==0x04034b50)throw Error('DEX目录不一致');
const payload=dexEntry.local+30+apk.readUInt16LE(dexEntry.local+26)+apk.readUInt16LE(dexEntry.local+28);
if(payload+dexEntry.packed>apk.length)throw Error('ZIP载荷越界');
const compressed=apk.subarray(payload,payload+dexEntry.packed);
const unpacked=dexEntry.method===0?compressed:dexEntry.method===8?zlib.inflateRawSync(compressed,{maxOutputLength:b.length}):null;
if(!unpacked||!unpacked.equals(b))throw Error('APK的DEX与已提取证据不同');
const u16=o=>b.readUInt16LE(o),u32=o=>b.readUInt32LE(o);
function leb(o){let n=0,s=0;for(let i=0;i<5;i++){const c=b[o++];n|=(c&127)<<s;if(!(c&128))return[n>>>0,o];s+=7;}throw Error('uleb越界');}
const strings=Array.from({length:u32(56)},(_,i)=>{let[length,p]=leb(u32(u32(60)+4*i)),e=p;while(b[e]!==0){if(++e>=b.length)throw Error('string越界');}return b.toString('utf8',p,e);});
const types=Array.from({length:u32(64)},(_,i)=>strings[u32(u32(68)+4*i)]);
const protos=Array.from({length:u32(72)},(_,i)=>{const p=u32(76)+12*i,o=u32(p+8);return '('+(o?Array.from({length:u32(o)},(_,j)=>types[u16(o+4+2*j)]).join(''):'')+')'+types[u32(p+4)];});
const methods=Array.from({length:u32(88)},(_,i)=>{const p=u32(92)+8*i;return types[u16(p)]+'::'+strings[u32(p+4)]+protos[u16(p+2)];});
const fields=Array.from({length:u32(80)},(_,i)=>{const p=u32(84)+8*i;return types[u16(p)]+'::'+strings[u32(p+4)]+':'+types[u16(p+2)];});
const wanted=new Set(['La/m;::u:Ljava/util/Vector;','La/m;::v:I','La/m;::w:I','La/m;::x:Ljava/util/Vector;','La/k;::y:Z']);
if([...wanted].some(f=>!fields.includes(f)))throw Error('目标字段缺失');
const widths=new Map(),add=(n,ops)=>ops.forEach(o=>widths.set(o,n));
add(1,[0,1,4,7,10,11,12,13,14,15,16,17,18,29,30,33,39,40,...Array.from({length:21},(_,i)=>123+i),...Array.from({length:32},(_,i)=>176+i)]);
add(2,[2,5,8,19,21,22,25,26,28,31,32,34,35,41,...Array.from({length:12},(_,i)=>50+i),...Array.from({length:42},(_,i)=>68+i),...Array.from({length:5},(_,i)=>45+i),...Array.from({length:32},(_,i)=>144+i),...Array.from({length:19},(_,i)=>208+i)]);
add(3,[3,6,9,20,23,27,36,37,38,42,43,44,...Array.from({length:5},(_,i)=>110+i),...Array.from({length:5},(_,i)=>116+i)]);add(5,[24]);
function decode(code){
 const size=u32(code+12),start=code+16;if(size*2>1024*1024||start+size*2>b.length)throw Error('方法预算/越界');
 const words=Array.from({length:size},(_,j)=>u16(start+2*j)),out=[];
 for(let pc=0;pc<size;){
  const word=words[pc],op=word&255;let width=widths.get(op),detail;
  if(op===0&&word!==0){const tag=word>>8,count=words[pc+1];
   if(tag===1)width=4+count*2;else if(tag===2)width=2+count*4;
   else if(tag===3){const count32=words[pc+2]+words[pc+3]*65536;width=4+Math.ceil(count*count32/2);}
   else throw Error('未知payload');detail={payload:tag};
  }else if(op>=82&&op<=109)detail={field:fields[words[pc+1]],registers:op<=95?[(word>>8)&15,word>>12]:[word>>8],
   access:op>=103?'sput':op>=96?'sget':op>=89?'iput':'iget'};
  else if(op>=110&&op<=114||op>=116&&op<=120)detail={invoke:methods[words[pc+1]]};
  else if(op===18)detail={register:(word>>8)&15,literal:(word<<16)>>28};
  else if(op===19)detail={register:word>>8,literal:(words[pc+1]<<16)>>16};
  else if(op>=50&&op<=61)detail={target:pc+((words[pc+1]<<16)>>16)};
  else if(op===40)detail={target:pc+((word<<16)>>24)};
  else if(op===41)detail={target:pc+((words[pc+1]<<16)>>16)};
  if(!Number.isSafeInteger(width)||width<=0||pc+width>size)throw Error(`指令宽度/边界 ${op.toString(16)} pc=${pc}`);
  out.push({pc,op:op.toString(16).padStart(2,'0'),words:words.slice(pc,pc+Math.min(width,5)).map(x=>x.toString(16).padStart(4,'0')),...detail});
  pc+=width;
 }
 return {size,start,out};
}
const wantedCalls=new Set(['Lc/n;::d()V','Lc/n;::c()V','Ld/a;::a(ILjava/lang/String;)V','Ld/a;::a(IZ)V']);
const windows=new Map([
 ['La/m;::<clinit>()V',[[0,20]]],['La/k;::b()V',[[0,32]]],['Lc/b;::k()I',[[0,195]]],
 ['Lc/n;::d()V',[[0,24],[1190,1257]]],['Lc/n;::f(I)Lc/k;',[[0,40],[330,450]]],
 ['Lb/h;::a()Z',[[160,225]]],['Lb/h;::e()V',[[0,300]]],['Lb/h;::f()V',[[0,80]]]
]);
const hits=[],codeIndex=[],callers=[],methodWindows=[];let scanned=0,units=0;
for(let i=0;i<u32(96);i++){
 const def=u32(100)+32*i;let p=u32(def+24);if(!p)continue;const counts=[];
 for(let k=0;k<4;k++){let[n,q]=leb(p);counts.push(n);p=q;}
 for(const count of counts.slice(0,2)){let id=0;for(let j=0;j<count;j++){let[n,q]=leb(p);p=q;id+=n;const access=leb(p);p=access[1];}}
 for(const count of counts.slice(2)){let id=0;for(let k=0;k<count;k++){
  let[n,q]=leb(p);p=q;id+=n;const access=leb(p);p=access[1];const item=leb(p);p=item[1];const code=item[0];
  if(!code)continue;const decoded=decode(code);++scanned;units+=decoded.size;
  const matches=decoded.out.filter(r=>r.field&&wanted.has(r.field));
  const meta={method:methods[id],method_index:id,code_offset:code,instruction_units:decoded.size,
    instructions_sha256:sha(b.subarray(decoded.start,decoded.start+decoded.size*2))};
  for(const hit of decoded.out.filter(r=>wantedCalls.has(r.invoke))){const at=decoded.out.findIndex(r=>r.pc===hit.pc);
   callers.push({...meta,...hit,context:decoded.out.slice(Math.max(0,at-8),Math.min(decoded.out.length,at+9))});}
  if(windows.has(methods[id]))methodWindows.push({...meta,windows:windows.get(methods[id]).map(([begin,end])=>({
   begin_pc:begin,end_pc_exclusive:end,instructions:decoded.out.filter(r=>r.pc>=begin&&r.pc<end)}))});
  if(!matches.length)continue;
  codeIndex.push(meta);
  for(const hit of matches){const at=decoded.out.findIndex(r=>r.pc===hit.pc);
   hits.push({...meta,...hit,context:decoded.out.slice(Math.max(0,at-4),Math.min(decoded.out.length,at+7))});}
 }}
}
const sourcePaths=['data/world/questData.txt','work/decompiled/sources/a/m.java','work/decompiled/sources/a/k.java',
 'work/decompiled/sources/b/h.java','work/decompiled/sources/b/g.java','work/decompiled/sources/c/n.java',
 'work/decompiled/sources/c/b.java','work/decompiled/sources/d/a.java','example/src/encounter_ai.cpp',
 'example/src/encounter_creation.cpp','example/src/world_task_creation.cpp',
 'prototype/src/startup_world_runtime_tasks.cpp','prototype/src/startup_world_runtime.cpp',
 'prototype/include/dungeon_village_prototype/startup_world_runtime.hpp'];
const sources=sourcePaths.map(p=>{const bytes=fs.readFileSync(path.join(root,p));return {path:p,bytes:bytes.length,sha256:sha(bytes)};});
const rows=fs.readFileSync(path.join(root,'data/world/questData.txt'),'utf8').trimEnd().split(/\r?\n/).map(s=>s.split('\t'));
const ids=mask=>rows.filter(r=>(Number(r[16])&mask)!==0).map(r=>Number(r[0]));
if(rows.length!==81||ids(2).length!==6||ids(4).length!==30)throw Error('定义规模/flags列变化');
const report={version:2,date:'2026-10-09',source,dex_sha256:sha(b),dex_bytes:b.length,
 apk:{path:apkPath,sha256:sha(apk),bytes:apk.length,classes_dex_exact_match:true},
 authority:'固定DEX全体有代码方法的具名字段引用查找；不是全程序别名/反射/外部JNI数据流证明',
 method_definitions_scanned:scanned,instruction_units_scanned:units,targets:[...wanted],methods:codeIndex,hits,
 call_targets:[...wantedCalls],callers,method_windows:methodWindows,sources,
 definition_lists:{source:'data/world/questData.txt',flags_column_zero_based:16,u_flag2:ids(2),x_flag4:ids(4)}};
const encoded=JSON.stringify(report,null,2)+'\n';if(Buffer.byteLength(encoded)>256*1024)throw Error('限定证据输出预算');
fs.writeFileSync(path.join(__dirname,'DEX_FIELD_REFS.json'),encoded);
console.log(JSON.stringify({methods_scanned:scanned,matched_methods:codeIndex.length,hits:hits.length,callers:callers.map(x=>({method:x.method,pc:x.pc,invoke:x.invoke})),
 writes:hits.filter(h=>h.access.endsWith('put')).map(h=>({method:h.method,pc:h.pc,field:h.field})),bytes:Buffer.byteLength(encoded)}));
