const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 只读固定DEX，复算任务选择方法的字段、分支与原表分组；不恢复旧反编译文件。
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
const wanted='Lc/n;::f(I)Lc/k;',m=all.find(m=>m.method===wanted);
if(!m||m.method_index!==1008||m.code_offset!==546136||m.instruction_units!==540)throw Error('选择器身份');
const names={0x32:'if-eq',0x33:'if-ne',0x34:'if-lt',0x35:'if-ge',0x36:'if-gt',0x37:'if-le',0x38:'if-eqz',0x39:'if-nez',0x3a:'if-ltz',0x3b:'if-gez',0x3c:'if-gtz',0x3d:'if-lez'};
const instructions=m.instructions.map(i=>{const word=parseInt(i.words[0],16),op=word&255,extra={};
 if(names[op]){extra.condition=names[op];extra.registers=op<=0x37?[(word>>8)&15,word>>12]:[word>>8];}
 else if(i.field)extra.registers=op<=95?[(word>>8)&15,word>>12]:[word>>8];
 else if(op===1||op===7)extra.move={to:(word>>8)&15,from:word>>12};
 return {...i,...extra};});
const windowSpecs=[['entry_mode',0,39],['current_special_scan',39,81],['special_progress',81,236],
 ['ordinary_explore',236,335],['replay_battle',335,449],['ordinary_battle',449,540]];
const windows=windowSpecs.map(([name,begin,end])=>({name,begin_pc:begin,end_pc_exclusive:end,instructions:instructions.filter(i=>i.pc>=begin&&i.pc<end)}));
const branches=instructions.filter(i=>i.target!==undefined);const pcs=new Set(instructions.map(i=>i.pc));
for(const br of branches)if(!pcs.has(br.target))throw Error('分支目的地不是指令边界');
const at=pc=>instructions.find(i=>i.pc===pc),checks=[];
function check(name,pcs,ok){if(!ok)throw Error(name);checks.push({name,pcs,verified:true});}
check('current_stage_mismatch_skips', [58,537,538],at(58).condition==='if-ne'&&at(58).target===537&&at(538).target===70);
check('completion_t_read_overwritten_by_flags', [60,62],at(60).field==='La/m;::t:I'&&at(62).field==='La/m;::o:I'&&at(60).registers[0]===5&&at(62).registers[0]===5);
check('normal_mode_boss_requires_G500_and_kind1', [75,77,79,81],at(75).field==='Lc/n;::G:I'&&at(77).literal===500&&at(79).condition==='if-lt'&&at(79).target===221&&at(81).condition==='if-ne'&&at(81).target===221);
check('unbeaten_monster_v_returns_current_boss', [91,93,96],at(91).field==='La/k;::v:I'&&at(93).condition==='if-gtz'&&at(93).target===97&&at(96).target===34);
check('boss_cap_only_when_above_first', [169,171],at(169).condition==='if-le'&&at(169).target===173&&at(171).field==='La/k;::v:I'&&at(171).access==='iput');
check('strict_minimum_preserves_earlier_tie', [203,533,535],at(203).condition==='if-ge'&&at(203).target===533&&at(535).target===208);
check('rare_path_zeros_w_then_bypasses_normal_draw', [302,305,323],at(302).field==='Lc/n;::w:I'&&at(302).access==='iput'&&at(305).target===34&&at(323).invoke==='Lc/d;::a(I)I');
check('replay_ticket40_and_event60', [339,341,345,351],at(339).literal===40&&at(341).condition==='if-ge'&&at(341).target===449&&at(345).literal===60&&at(351).target===449);
check('replay_uses_stage_then_monster_y_cap6', [389,399,401,414,415],at(389).condition==='if-gt'&&at(389).target===417&&at(399).field==='La/k;::y:Z'&&at(401).condition==='if-eqz'&&at(414).literal===6&&at(415).target===421);
check('ordinary_battle_same_stage_not_bad_Java_neq',[479,483,491,500],at(479).condition==='if-ne'&&at(479).target===507&&at(483).condition==='if-ne'&&at(491).condition==='if-nez'&&at(500).condition==='if-nez');
const callers=all.flatMap(c=>c.instructions.flatMap((i,n)=>i.invoke===wanted?[{method:c.method,method_index:c.method_index,
 code_offset:c.code_offset,instructions_sha256:c.instructions_sha256,pc:i.pc,context:c.instructions.slice(Math.max(0,n-22),n+12)}]:[]));
const tableSource='data/world/questData.txt',rows=fs.readFileSync(path.join(root,tableSource),'utf8').trimEnd().split(/\r?\n/).map(line=>line.split('\t'));
if(rows.length!==81||rows.some(r=>r.length!==17))throw Error('81任务原表结构');
const definitions=rows.map(r=>({id:Number(r[0]),kind:Number(r[3]),chapter:Number(r[5]),monster:Number(r[11]),flags:Number(r[16])}));
const categories=[['boss',d=>d.flags&2],['replay',d=>d.flags&4],['rare_exploration',d=>d.flags&8],
 ['ordinary_exploration',d=>d.kind===0&&!(d.flags&14)],['ordinary_battle',d=>d.kind===1&&!(d.flags&14)]];
const definitionGroups=categories.map(([name,predicate])=>({name,definitions:definitions.filter(predicate)}));
if(definitionGroups.map(g=>g.definitions.length).join(',')!=='6,30,6,27,12')throw Error('定义分组变化');
const sourcePaths=['work/decompiled/sources/c/n.java','work/decompiled/sources/b/c.java','work/decompiled/sources/c/k.java',
 'data/world/questData.txt','example/src/world_task_creation.cpp','example/src/world_calendar_tasks.cpp',
 'example/src/world_dungeon.cpp','prototype/src/startup_world_runtime_tasks.cpp'];
const sources=sourcePaths.map(p=>{const data=fs.readFileSync(path.join(root,p));return {path:p,bytes:data.length,sha256:sha(data)};});
const result={version:1,date:'2026-10-09',source,dex_sha256:sha(b),method:{method:m.method,method_index:m.method_index,
 code_offset:m.code_offset,instruction_units:m.instruction_units,instructions_sha256:m.instructions_sha256},
 windows,branches:branches.map(i=>({pc:i.pc,condition:i.condition,registers:i.registers,target:i.target})),checks,callers,
 definition_groups:definitionGroups,sources,
 limitation:'固定APK选择器有限方法静态证据；不恢复缺失UserData fallback，不证明自然通关或Steam同实现。'};
const json=JSON.stringify(result,null,2)+'\n';if(Buffer.byteLength(json)>96*1024)throw Error('输出预算');
fs.writeFileSync(path.join(archivePaths.workDir,'DEX_SELECTOR.json'),json);
console.log(JSON.stringify({method:m.method,units:m.instruction_units,branches:branches.length,bytes:Buffer.byteLength(json)}));
