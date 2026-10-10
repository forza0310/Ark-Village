const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 只读固定DEX，登记标题目录初始化的字段及设施N调用链；不更改原证据或维护代码。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const root=path.resolve(archivePaths.workDir,'../..'),source='work/magic-pot-analysis/classes.dex';
const b=fs.readFileSync(path.join(root,source)),sha=x=>crypto.createHash('sha256').update(x).digest('hex');
if(sha(b)!=='b4386a0de612fe18196a390633607ef36c69c2a63881244832314e3bf4902d1a')throw Error('DEX身份');
const u16=o=>b.readUInt16LE(o),u32=o=>b.readUInt32LE(o);
function leb(o){let n=0,s=0;for(let k=0;k<5;k++){const c=b[o++];n|=(c&127)<<s;if(!(c&128))return[n>>>0,o];s+=7;}throw Error('uleb');}
const strings=Array.from({length:u32(56)},(_,i)=>{let[n,p]=leb(u32(u32(60)+4*i)),e=p;while(b[e])e++;return b.toString('utf8',p,e);});
const types=Array.from({length:u32(64)},(_,i)=>strings[u32(u32(68)+4*i)]);
const protos=Array.from({length:u32(72)},(_,i)=>{const p=u32(76)+12*i,o=u32(p+8);return '('+(o?Array.from({length:u32(o)},(_,j)=>types[u16(o+4+2*j)]).join(''):'')+')'+types[u32(p+4)];});
const methods=Array.from({length:u32(88)},(_,i)=>{const p=u32(92)+8*i;return types[u16(p)]+'::'+strings[u32(p+4)]+protos[u16(p+2)];});
const fields=Array.from({length:u32(80)},(_,i)=>{const p=u32(84)+8*i;return types[u16(p)]+'::'+strings[u32(p+4)]+':'+types[u16(p+2)];});
const widths=new Map(),add=(n,ops)=>ops.forEach(o=>widths.set(o,n));
add(1,[0,1,4,7,10,11,12,13,14,15,16,17,18,29,30,33,39,40,...Array.from({length:21},(_,i)=>123+i),...Array.from({length:32},(_,i)=>176+i)]);
add(2,[2,5,8,19,21,22,25,26,28,31,32,34,35,41,...Array.from({length:12},(_,i)=>50+i),...Array.from({length:42},(_,i)=>68+i),...Array.from({length:5},(_,i)=>45+i),...Array.from({length:32},(_,i)=>144+i),...Array.from({length:19},(_,i)=>208+i)]);
add(3,[3,6,9,20,23,27,36,37,38,42,43,44,...Array.from({length:5},(_,i)=>110+i),...Array.from({length:5},(_,i)=>116+i)]);add(5,[24]);
function decode(code){
 const size=u32(code+12),start=code+16;if(start+size*2>b.length)throw Error('code越界');
 const words=Array.from({length:size},(_,i)=>u16(start+2*i)),out=[];
 for(let pc=0;pc<size;){const word=words[pc],op=word&255;let width=widths.get(op),extra={};
  if(op===0&&word){const tag=word>>8,n=words[pc+1];if(tag===1)width=4+n*2;else if(tag===2)width=2+n*4;
   else if(tag===3)width=4+Math.ceil(n*(words[pc+2]+65536*words[pc+3])/2);else throw Error('payload');extra.payload=tag;}
  else if(op>=82&&op<=109)extra={field:fields[words[pc+1]],access:op>=103?'sput':op>=96?'sget':op>=89?'iput':'iget'};
  else if(op>=110&&op<=114||op>=116&&op<=120)extra.invoke=methods[words[pc+1]];
  else if(op===18)extra={register:(word>>8)&15,literal:(word<<16)>>28};
  else if(op===19)extra={register:word>>8,literal:(words[pc+1]<<16)>>16};
  else if(op>=50&&op<=61||op===41)extra.target=pc+((words[pc+1]<<16)>>16);
  else if(op===40)extra.target=pc+((word<<16)>>24);
  if(!width||pc+width>size)throw Error('指令宽度');
  out.push({pc,words:words.slice(pc,pc+Math.min(width,5)).map(w=>w.toString(16).padStart(4,'0')),...extra});pc+=width;
 }
 return {size,start,out};
}
const all=[];
for(let i=0;i<u32(96);++i){let p=u32(u32(100)+32*i+24);if(!p)continue;const counts=[];
 for(let n=0;n<4;++n){const [v,q]=leb(p);counts.push(v);p=q;}
 for(let n=0;n<counts[0]+counts[1];++n){p=leb(p)[1];p=leb(p)[1];}
 for(const count of counts.slice(2)){let id=0;for(let n=0;n<count;++n){let[v,q]=leb(p);id+=v;p=q;p=leb(p)[1];const [code,next]=leb(p);p=next;
  if(code){const d=decode(code);all.push({method:methods[id],method_index:id,code_offset:code,instruction_units:d.size,
   instructions_sha256:sha(b.subarray(d.start,d.start+d.size*2)),instructions:d.out});}
 }}
}
const nField='La/o;::N:I',reset='La/o;::g()V';
const selected=all.filter(m=>m.method===reset||m.method==='La/o;::a(Lc/m;La/o;)V');
const references=[];
for(const m of all){const {instructions,...meta}=m;for(let i=0;i<instructions.length;++i){const r=instructions[i];
 if(r.field===nField||r.invoke===reset)references.push({...meta,...r,context:instructions.slice(Math.max(0,i-5),i+6)});
}}
const startup=all.find(m=>m.method==='Lc/n;::d()V');
if(!startup||selected.length!==2)throw Error('指定方法身份');
const targets=new Set(startup.instructions.filter(i=>i.access==='sput'||i.access==='iput').map(i=>i.field));
for(const f of ['La/h;::D:Ljava/util/Vector;','La/o;::N:I','Lc/n;::ai:[[Lkairo/android/e/a/b;',
 'Lc/n;::aj:[[[I','Lc/n;::ak:[[[[I','La/k;::E:[I','La/k;::F:[I','La/e;::aj:I'])if(fields.includes(f))targets.add(f);
const referenceIndex=[...targets].map(field=>({field,references:all.flatMap(m=>m.instructions.filter(i=>i.field===field)
 .map(i=>({method:m.method,pc:i.pc,access:i.access})))}));
const sourcePaths=['work/decompiled/sources/c/n.java','work/decompiled/sources/a/o.java','work/decompiled/sources/a/e.java',
 'work/decompiled/sources/a/k.java','work/decompiled/sources/b/c.java','work/decompiled/sources/c/h.java',
 'data/original/tenantData.txt','prototype/scripts/compile_startup_world.mjs',
 'prototype/include/dungeon_village_prototype/startup_world_projection.hpp','prototype/src/startup_world_projection.cpp',
 'prototype/src/startup_world_runtime.cpp','prototype/src/startup_world_runtime_tasks.cpp',
 'prototype/src/startup_world_runtime_calendar.cpp','prototype/src/startup_world_building.cpp',
 'prototype/src/startup_world_routes.cpp','example/src/human_growth.cpp','example/src/actor_housekeeping.cpp',
 'example/src/world_facility_update.cpp','example/src/world_calendar_maintenance.cpp'];
const sources=sourcePaths.map(p=>{const data=fs.readFileSync(path.join(root,p));return {path:p,bytes:data.length,sha256:sha(data)};});
const result={version:1,date:'2026-10-09',source,dex_sha256:sha(b),methods_scanned:all.length,
 startup_method:{method:startup.method,code_offset:startup.code_offset,instruction_units:startup.instruction_units,
  instructions_sha256:startup.instructions_sha256},
 startup_direct_writes:startup.instructions.filter(i=>i.access==='sput'||i.access==='iput'),
 startup_calls:startup.instructions.filter(i=>i.invoke),facility_n_references:references,facility_n_methods:selected,
 startup_reference_index:referenceIndex,sources,
 limits:'具名直接指令扫描；不是别名/反射/JNI穷尽；启动列表后续消费者按责任矩阵局部核对。'};
const json=JSON.stringify(result,null,2)+'\n';if(Buffer.byteLength(json)>128*1024)throw Error('输出预算');
fs.writeFileSync(path.join(archivePaths.workDir,'DEX_STARTUP.json'),json);
console.log(JSON.stringify({methods:all.length,writes:result.startup_direct_writes.length,n_refs:references.length,
 reset_callers:references.filter(r=>r.invoke===reset).map(r=>({method:r.method,pc:r.pc})),bytes:Buffer.byteLength(json)}));
