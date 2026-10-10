const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 通关纪录的有限只读证据；不运行游戏、不保存反编译全文。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),assert=archivePaths.require('assert/strict');
const root=path.resolve(archivePaths.workDir,'../..'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(path.join(root,p));
const dll=read('DungeonVillageEXE/GameAssembly.dll'),index=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
assert.equal(hash(dll),'9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a');
const pe=dll.readUInt32LE(0x3c);assert.equal(dll.readUInt32LE(pe),0x4550);assert.equal(dll.readUInt16LE(pe+4),0x14c);
const opt=pe+24;assert.equal(dll.readUInt16LE(opt),0x10b);const base=dll.readUInt32LE(opt+28);
const sections=Array.from({length:dll.readUInt16LE(pe+6)},(_,i)=>{const p=opt+dll.readUInt16LE(pe+20)+40*i;return {rva:dll.readUInt32LE(p+12),bytes:dll.readUInt32LE(p+16),offset:dll.readUInt32LE(p+20)};});
const starts=[...new Set(index.methods.map(m=>m.va))].sort((a,b)=>a-b),names=new Map(index.methods.map(m=>[m.va,m.type+'::'+m.signature]));
const wanted=[['main.AppData','public long AddMoney(int add, int recordId) { }'],['game.UserData','public void SetClearPoint() { }'],['form.SubForm','public void Update_clearPoint() { }']];
const methods=wanted.map(([type,signature])=>{const m=index.methods.find(v=>v.type===type&&v.signature===signature);assert(m);const end=starts.find(v=>v>m.va),bytes=end-m.va,section=sections.find(s=>m.rva>=s.rva&&m.rva<s.rva+s.bytes);assert(section);assert.equal(m.va,base+m.rva);assert.equal(m.offset,section.offset+m.rva-section.rva);assert(Number.isSafeInteger(bytes)&&bytes>0&&bytes<=4096);assert(m.offset+bytes<=dll.length&&m.rva+bytes<=section.rva+section.bytes);return {...m,bytes,sha256:hash(dll.subarray(m.offset,m.offset+bytes)),range_kind:'至下一登记方法起点的有界范围，可能含对齐填充；不称精确函数长度'};});
assert(methods.reduce((n,m)=>n+m.bytes,0)<=8192);
const iced=archivePaths.require(path.join(root,'work/local-tools/iced-x86-1.21.0/package'));
const args=process.argv.slice(2);
function decode(m,visit){const d=new iced.Decoder(32,dll.subarray(m.offset,m.offset+m.bytes),0);d.ip=BigInt(m.va);let n=0;while(d.canDecode){const ins=d.decode();try{visit(ins,n++);}finally{ins.free();}}d.free();}
if(args[0]==='--inspect'){
 const m=methods.find(x=>x.rva===Number(args[1])),start=Number(args[2]||0),count=Number(args[3]||80);assert(m);assert(Number.isSafeInteger(start)&&start>=0);assert(Number.isSafeInteger(count)&&count>0&&count<=100);
 const f=new iced.Formatter(iced.FormatterSyntax.Intel);decode(m,(i,n)=>{if(n>=start&&n<start+count)console.log(n,'0x'+Number(i.ip).toString(16),f.format(i),i.isCallNear?names.get(Number(i.nearBranchTarget))||'':'');});f.free();process.exit(0);
}
const windows=[
 ['categories','c/n.java',130,130,'六类显示名及顺序'],['score_inputs','c/n.java',2291,2335,'人气/任务完成/已开放设施种类/已开放人物全职业M/在籍住宅实例/已开放人物努力u'],
 ['score_init','b/g.java',10635,10641,'17初始化停背景音乐、捕获旧最高分及计算六类'],['score_update','b/g.java',6835,6923,'阶段阈值、恰到阈值累计一次、确认推进及严格最高分提交'],
 ['thresholds','b/g.java',173,173,'阶段门槛75/75/45/45/65/45/65/9999'],['rank_labels','b/g.java',129,129,'奖杯档位文本'],
 ['cash_record','d/a.java',3983,3991,'P14为0且新余额严格超过系统纪录才更新J.c2和村名，不在此保存系统'],
 ['clear_trigger','b/c.java',195,205,'月变化且原年15月3时：事件3、推raw17、置P14和J9、继承快照C'],
 ['date_display','b/c.java',674,686,'原年和月字段各加1作为显示值；结局触发为显示16年4月']
];
const sources=new Map(),apk=windows.map(([key,file,first,last,summary])=>{const p='work/decompiled/sources/'+file,b=read(p),lines=b.toString('utf8').split(/\r?\n/);assert(first>0&&last>=first&&last<=lines.length);sources.set(p,{path:p,bytes:b.length,sha256:hash(b)});return {key,path:p,first,last,window_lf_sha256:hash(Buffer.from(lines.slice(first-1,last).join('\n'))),summary};});
const steam=methods.map(m=>{const calls=[];decode(m,(i)=>{if(i.isCallNear){const target=Number(i.nearBranchTarget);if(names.has(target))calls.push({va:'0x'+Number(i.ip).toString(16),target:'0x'+target.toString(16),name:names.get(target)});}});return {...m,calls};});
// 用户本轮已明确授权；直接固定三精确DEX签名，不改写或执行其它分析器。
const dex=read('work/magic-pot-analysis/classes.dex');
assert.equal(hash(dex),'b4386a0de612fe18196a390633607ef36c69c2a63881244832314e3bf4902d1a');
assert(dex.length<16*1024*1024);
const u16=o=>dex.readUInt16LE(o),u32=o=>dex.readUInt32LE(o);
function leb(o){let n=0,s=0;for(let k=0;k<5;k++){assert(o<dex.length);const c=dex[o++];n|=(c&127)<<s;if(!(c&128))return[n>>>0,o];s+=7;}throw Error('uleb边界');}
const strings=Array.from({length:u32(56)},(_,i)=>{let[,p]=leb(u32(u32(60)+4*i)),end=p;while(dex[end]!==0){assert(++end<dex.length);}return dex.toString('utf8',p,end);});
const types=Array.from({length:u32(64)},(_,i)=>strings[u32(u32(68)+4*i)]);
const protos=Array.from({length:u32(72)},(_,i)=>{const at=u32(76)+12*i,list=u32(at+8);return '('+(list?Array.from({length:u32(list)},(_,j)=>types[u16(list+4+j*2)]).join(''):'')+')'+types[u32(at+4)];});
const dexMethods=Array.from({length:u32(88)},(_,i)=>{const at=u32(92)+8*i;return types[u16(at)]+'::'+strings[u32(at+4)]+protos[u16(at+2)];});
const dexFields=Array.from({length:u32(80)},(_,i)=>{const at=u32(84)+8*i;return types[u16(at)]+'::'+strings[u32(at+4)]+':'+types[u16(at+2)];});
const targets=new Set(['Lc/n;::B()V','Lb/g;::i()V','Ld/a;::f(II)J']);
const dexItems=[];
for(let i=0;i<u32(96);++i){
 const def=u32(100)+i*32;if(!['Lc/n;','Lb/g;','Ld/a;'].includes(types[u32(def)]))continue;
 let p=u32(def+24);if(!p)continue;const counts=[];
 for(let k=0;k<4;++k){let[n,q]=leb(p);p=q;counts.push(n);}
 for(let k=0;k<counts[0]+counts[1];++k)for(let z=0;z<2;++z)p=leb(p)[1];
 for(const count of counts.slice(2)){
  let id=0;for(let k=0;k<count;++k){let[n,q]=leb(p);p=q;id+=n;const access=leb(p);p=access[1];const code=leb(p);p=code[1];
   if(!targets.has(dexMethods[id]))continue;
   const at=code[0],size=u32(at+12);assert(at>0&&size>0&&size*2<=16384&&at+16+size*2<=dex.length);
   dexItems.push({method:dexMethods[id],code_offset:at,instruction_units:size,instructions_sha256:hash(dex.subarray(at+16,at+16+size*2)),
    words:Array.from({length:size},(_,j)=>u16(at+16+2*j))});
  }
 }
}
assert.equal(dexItems.length,3);assert(dexItems.reduce((n,m)=>n+m.instruction_units*2,0)<=16384);
const widths=new Map(),add=(n,ops)=>ops.forEach(o=>widths.set(o,n));
add(1,[0,1,4,7,10,11,12,13,14,15,16,17,18,29,30,33,39,40,...Array.from({length:21},(_,i)=>123+i),...Array.from({length:32},(_,i)=>176+i)]);
add(2,[2,5,8,19,21,22,25,26,28,31,32,34,35,41,...Array.from({length:12},(_,i)=>50+i),...Array.from({length:42},(_,i)=>68+i),...Array.from({length:5},(_,i)=>45+i),...Array.from({length:32},(_,i)=>144+i),...Array.from({length:19},(_,i)=>208+i)]);
add(3,[3,6,9,20,23,27,36,37,38,42,43,44,...Array.from({length:5},(_,i)=>110+i),...Array.from({length:5},(_,i)=>116+i)]);add(5,[24]);
for(const item of dexItems){
 const refs=[];for(let pc=0;pc<item.words.length;){
  const word=item.words[pc],op=word&255,width=widths.get(op);assert(width&&!(op===0&&word!==0));assert(pc+width<=item.words.length);
  if(op>=82&&op<=109)refs.push({pc,field:dexFields[item.words[pc+1]]});
  if(op>=110&&op<=114||op>=116&&op<=120)refs.push({pc,method:dexMethods[item.words[pc+1]]});
  if([0x81,0x85,0x88,0xa8,0xb0,0xbb,0xc8,0xd3].includes(op))refs.push({pc,operation:'0x'+op.toString(16)});
  if(op===18)refs.push({pc,literal:(word<<16)>>28});
  if(op===19)refs.push({pc,literal:(item.words[pc+1]<<16)>>16});
  if(op===21)refs.push({pc,bits_hex:'0x'+(item.words[pc+1]<<16>>>0).toString(16)});
  if(op===25)refs.push({pc,wide_high16_hex:'0x'+item.words[pc+1].toString(16)});
  pc+=width;
 }item.references=refs;delete item.words;
}
// 只保存系数与合同摘要，不复制机器码／原计分实现。
const scoreCoefficients=[[0,0x10dde574,5],[1,0x10dde664,300],[2,0x10dde668,500],[3,0x10dde5d4,50],[4,0x10dde66c,1000],[5,0x10dde580,10]].map(([slot,va,value])=>{
 const rva=va-base,s=sections.find(x=>rva>=x.rva&&rva+4<=x.rva+x.bytes);assert(s);const offset=s.offset+rva-s.rva;assert(offset+4<=dll.length);assert.equal(dll.readFloatLE(offset),value);return {slot,va:'0x'+va.toString(16),offset,value,float_bytes_sha256:hash(dll.subarray(offset,offset+4))};
});
const scoreDex=dexItems.find(x=>x.method==='Lc/n;::B()V');
for(const op of ['0x81','0x85','0x88','0xc8'])assert.equal(scoreDex.references.filter(x=>x.operation===op).length,6);
const floatSample={count:16777217,coefficient:5,float32_count:Math.fround(16777217),float32_product:Math.fround(Math.fround(16777217)*5),integer_product:16777217*5};
assert.equal(floatSample.float32_product,83886080);assert.notEqual(floatSample.float32_product,floatSample.integer_product);
const contract={
 score_conversion:'int32统计→signed64行count→binary32→binary32乘系数→signed64向零截断；整数和可能回绕，维护拒绝溢出须标策略',
 score_fields:['世界人气','任务完成数','已开放kind2/3设施定义种数','已开放人物全部M之和','在籍kind12住宅实例数','已开放人物努力u之和'],
 minimum_read_model:['六行count/score','bo阶段0..7','bp阶段计数','bq行0..6','bs累计','bt初始化捕获旧最高','世界P14资格','系统long0/text0/int10','系统long2/text1'],
 clear_trigger:'APK月变化且raw年15月3：事件3(args15)→Push raw17→P14=1→J9=1→继承C；原日期显示各+1',
 stage_thresholds:[75,75,45,45,65,45,65,9999],
 score_once:'仅bp从未满增至stage3门槛时累加当行一次；stage6自动一次收尾、事件6先于新高事件4或非新高事件5、stage7无领取分支',
 record_guard:'严格高于初始化捕获旧分才改long0/text0/int10；新高奖杯>=100000:4、>=75000:3、>=50000:2、其它:1；收尾请求SaveSystem',
 cash_guard:'更新资金/账本后，仅P14==0且余额严格超过系统long2才更新long2/text1；AddMoney不保存系统；原加法可能signed64回绕',
 steam_fast_difference:'资格静态bool+0x83真且held0x40可拉满未满阶段计数；stage3可用held替代确认，stage0仍只用pulse1048576；资格语义和默认值未核',
 steam_boundaries:['未解码数值转换runtime helper，极限NaN/溢出全域等价未认证','未认证真实结局窗口、绘制、BGM文件或全部自然后期路线']
};
const out={dll_sha256:hash(dll),method_index_sha256:hash(read('work/persistence-replay-analysis/exe/methods.json')),tool:'iced-x86 1.21.0',apk,sources:[...sources.values()],steam,score_coefficients:scoreCoefficients,float32_sample:floatSample,contract,
 dex_sha256:hash(dex),dex_methods:dexItems,range_bytes:steam.reduce((n,m)=>n+m.bytes,0),authorization:'2026-10-08用户明确可以继续研究后，固定六方法只读有限交叉',scope:'计分输入与纪录写入有限静态交叉；不认证原窗口或维护系统档'};
fs.writeFileSync(path.join(archivePaths.workDir,'EVIDENCE.json'),JSON.stringify(out,null,2)+'\n');console.log(JSON.stringify({windows:apk.length,lines:windows.reduce((n,w)=>n+w[3]-w[2]+1,0),methods:steam.length,range_bytes:out.range_bytes}));
