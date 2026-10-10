const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 固定Steam两个cctor的成功分配路径静态读数；不执行IL2CPP或游戏，不输出指令全文。
// 只解释显式数组构造与发布；遇未知算术/调用/跳转拒绝，目标字段齐全后立即停止。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const root=path.resolve(archivePaths.workDir,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.join(root,'DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const index=archivePaths.require('../persistence-replay-analysis/exe/methods.json');
const iced=archivePaths.require('../local-tools/iced-x86-1.21.0/package');
const mode=process.argv[2];
const plans={human:{type:'game.Character2',rva:0x293560,end:0x29d2e0,slot:0x110f39bc,targets:{0x58:'HUMAN_ANIME_SEB',0x5c:'HUMAN_ANIME_SEB_F',0x60:'WEAPON_ANIME_F',0x70:'body_off'}},weapon:{type:'data.WeaponData',rva:0x229e40,end:0x2317e0,slot:0x110e7d8c,targets:{0xc:'WEAPON_WALK_POS',0x1c:'WEAPON_SEB'}}};
if(process.argv.length!==3||!plans[mode])throw Error('human or weapon required');
const plan=plans[mode],method=index.methods.find(m=>m.type===plan.type&&m.rva===plan.rva&&m.signature==='private static void .cctor() { }');
if(!method||index.methods.some(m=>m.rva>plan.rva&&m.rva<plan.end)||!index.methods.some(m=>m.rva===plan.end))throw Error('method boundary');
const section=index.sections.find(s=>plan.rva>=s.rva&&plan.end<=s.rva+s.rawSize);
if(!section||section.raw+section.rawSize>dll.length||method.offset!==section.raw+plan.rva-section.rva)throw Error('PE mapping');
const helpers=[{va:0x100025d0,caller:0x1022f38a,bytes:128,sha256:'7c263f945da3fd342ba8216fa1125d1a98b4f1a7559481b92663d63f5a5045bb'},{va:0x10002bb0,caller:0x1022f397,bytes:96,sha256:'8aaed229605431e5968b8fa65c93468c59ec947a4980c153e159ef24463c90f6'},{va:0x10002be0,caller:0x1022f3df,bytes:64,sha256:'3ddb887fe18d84f7d27699f714ea7a5cb63e86a370e5935a8be08e17542b58cc'}];
for(const h of helpers){const s=index.sections.find(s=>h.va-index.base>=s.rva&&h.va-index.base+h.bytes<=s.rva+s.rawSize);if(!s)throw Error('helper mapping');h.offset=s.raw+h.va-index.base-s.rva;if(sha(dll.subarray(h.offset,h.offset+h.bytes))!==h.sha256)throw Error('helper identity');}
const max=plan.end-plan.rva;if(max>41000)throw Error('fixed source budget');
function table(at,stride){const p=meta.readUInt32LE(at),n=meta.readUInt32LE(at+4);if(n%stride||p+n>meta.length)throw Error('metadata table');return {p,n:n/stride};}
const refs=table(184,8),types=table(160,88),fields=table(96,12),defs=table(64,12),data=table(72,1),strings=table(24,1);
function peSlot(va){const s=index.sections.find(s=>va-index.base>=s.rva&&va-index.base+4<=s.rva+s.rawSize);if(!s)throw Error('slot');const offset=s.raw+va-index.base-s.rva;return {offset,encoded:dll.readUInt32LE(offset)};}
const blobEvidence=[];
function blob(slot,length,call){const entry=peSlot(slot),v=entry.encoded;if(v>>>29!==4||!(v&1))throw Error('field ref tag');const ri=(v&0x1fffffff)>>>1;if(ri>=refs.n)throw Error('field ref');const rp=refs.p+ri*8,ti=meta.readInt32LE(rp),local=meta.readInt32LE(rp+4),tp=types.p+4129*88;
 if(4129>=types.n||meta.readInt32LE(tp+8)!==ti||local<0)throw Error('field type');const fi=meta.readInt32LE(tp+32)+local;if(fi<0||fi>=fields.n)throw Error('field');const fp=fields.p+fi*12,ni=meta.readUInt32LE(fp);if(ni>=strings.n)throw Error('name');const np=strings.p+ni,ne=meta.indexOf(0,np);if(ne<np||ne>=strings.p+strings.n)throw Error('name end');const name=meta.toString('utf8',np,ne);const found=[];for(let k=0;k<defs.n;k++)if(meta.readInt32LE(defs.p+k*12)===fi)found.push(defs.p+k*12);if(found.length!==1)throw Error('default');const di=meta.readInt32LE(found[0]+8),n=length*4;if(di<0||di+n>data.n||length>512)throw Error('data');const b=meta.subarray(data.p+di,data.p+di+n);if(sha(b).toUpperCase()!==name)throw Error('blob name hash');const values=Array.from({length},(_,i)=>b.readInt32LE(i*4));blobEvidence.push({call,slot,...entry,reference:ri,field:fi,field_offset:fp,default_offset:found[0],data_offset:data.p+di,length,sha256:sha(b),values});return values;}
const registers=Object.fromEntries(['eax','ebx','ecx','edx','esi','edi','ebp','esp'].map(r=>[r,{kind:'unknown',name:r}]));
const stack=[],locals=new Map(),globals=new Map([[mode==='human'?0x111556b1:0x11155501,0]]),staticFields=new Map(),events=[],allocations=[];let flags={z:false,lt:false,gt:false,below:false},pc=method.va,steps=0,last=pc;
const number=s=>{const n=s.endsWith('h')?parseInt(s.slice(0,-1),16):Number(s);if(!Number.isFinite(n))throw Error('non-numeric literal '+s);return n;};
function location(s){s=s.replace(/^(?:byte|dword) ptr /,'');if(!s.startsWith('['))throw Error('memory syntax '+s);const x=s.slice(1,-1);if(/^[0-9A-F]+h$/.test(x))return {owner:globals,key:number(x),absolute:true};const m=x.match(/^([a-z]+)(?:([+-])([0-9A-F]+h|\d+))?$/);if(!m)throw Error('memory expression '+s);const off=m[2]?(m[2]==='-'?-1:1)*number(m[3]):0;if(m[1]==='ebp')return {owner:locals,key:off};const owner=registers[m[1]];if(!owner||typeof owner!=='object')throw Error('null/unknown address '+s);if(owner.kind==='address')return {owner:owner.owner,key:owner.key+off};return {owner,key:off};}
function read(s){if(Object.hasOwn(registers,s))return registers[s];if(/^(?:0x)?[0-9]+$/.test(s)||/^[0-9A-F]+h$/.test(s))return number(s);if(s.includes('[')){const {owner,key,absolute}=location(s);if(owner instanceof Map){if(owner.has(key))return owner.get(key);if(absolute){peSlot(key);const value={kind:'slot',va:key};owner.set(key,value);return value;}throw Error('unwritten local');}if(owner.kind==='slot'&&key===0x5c)return {kind:'static',slot:owner.va};if(owner.kind==='static'){if(!staticFields.has(owner.slot+':'+key))throw Error('unwritten static');return staticFields.get(owner.slot+':'+key);}if(owner.kind==='array'){if(key===0xc)return owner.values.length;if(key===0)return {kind:'arrayType'};if(key>=0x10&&(key-0x10)%4===0){const i=(key-0x10)/4;if(i>=owner.values.length)throw Error('array read bound');return owner.values[i];}}if(owner.kind==='arrayType'&&key===0x20)return {kind:'castType'};throw Error('unknown read '+s);}throw Error('unknown operand '+s);}
function scalar(v){if(typeof v==='number')return v;if(v&&typeof v==='object'&&v.kind!=='unknown')return 1;throw Error('unknown scalar');}
function write(s,v){if(Object.hasOwn(registers,s)){registers[s]=v;return;}const {owner,key}=location(s);if(owner instanceof Map){owner.set(key,v);return;}if(owner.kind==='static'){staticFields.set(owner.slot+':'+key,v);events.push({at:pc,slot:owner.slot,field_offset:key,array:v.kind==='array'?v.id:null});return;}if(owner.kind==='array'&&key>=0x10&&(key-0x10)%4===0){const i=(key-0x10)/4;if(i>=owner.values.length)throw Error('array write bound');owner.values[i]=v;owner.writes.push({at:pc,index:i,value:typeof v==='number'?(v|0):{array:v.id}});return;}throw Error('unknown write '+s);}
function plain(v){if(typeof v==='number')return v|0;if(v?.kind==='array')return v.values.map(plain);throw Error('target contains unknown data');}
const decoder=new iced.Decoder(32,dll.subarray(method.offset,method.offset+max),iced.DecoderOptions.None);decoder.ip=BigInt(method.va);const formatter=new iced.Formatter(iced.FormatterSyntax.Intel);let done=false;const history=[];
while(decoder.canDecode&&steps++<15000){const i=decoder.decode();pc=Number(i.ip);const text=formatter.format(i);last=pc+i.length;history.push({pc,text});if(history.length>12)history.shift();if(i.isInvalid)throw Error('decode '+pc.toString(16));const op=text.split(' ')[0],args=text.slice(op.length).trim().split(',');
 try{
 if(op==='mov')write(args[0],read(args[1]));
 else if(op==='lea'){const m=args[1].match(/^\[([a-z]+)\+([a-z]+)\*([1248])\]$/);if(m){const a=read(m[1]),b=read(m[2]);if(typeof a!=='number'||typeof b!=='number')throw Error('scaled address');registers[args[0]]=(a+b*Number(m[3]))|0;}else{const loc=location(args[1]);registers[args[0]]={kind:'address',...loc};}}
 else if(op==='push')stack.unshift(read(args[0]));
 else if(op==='pop')registers[args[0]]=stack.shift();
 else if(op==='add'&&args[0]==='esp'){const n=number(args[1]);if(n%4||stack.length<n/4)throw Error('stack');stack.splice(0,n/4);}
 else if(op==='sub'&&args[0]==='esp'){const n=number(args[1]);if(n%4)throw Error('stack');stack.unshift(...Array(n/4).fill(0));}
 else if(op==='add'){const v=registers[args[0]],n=read(args[1]);if(typeof n!=='number')throw Error('add operand');if(v&&typeof v==='object'&&v.kind!=='unknown')registers[args[0]]={kind:'address',owner:v,key:n};else if(typeof v==='number')registers[args[0]]=(v+n)|0;else throw Error('add destination');}
 else if(op==='shl'){const a=read(args[0]),b=number(args[1]);if(typeof a!=='number'||b<0||b>31)throw Error('shift');write(args[0],a<<b);}
 else if(op==='xor'&&args[0]===args[1])registers[args[0]]=0;
 else if(op==='test'){const a=scalar(read(args[0])),b=scalar(read(args[1]));flags={z:(a&b)===0,lt:false,gt:false,below:false};}
 else if(op==='cmp'){let a=read(args[0]);const av=scalar(a),b=scalar(read(args[1]));flags={z:av===b,lt:(av|0)<(b|0),gt:(av|0)>(b|0),below:(av>>>0)<(b>>>0)};}
 else if(op==='call'){
 const target=Number(i.nearBranchTarget);
 if(target===0x1010aa90){const n=stack[1];if(!Number.isInteger(n)||n<0||n>512)throw Error('allocation size');const a={kind:'array',id:allocations.length,at:pc,values:Array(n).fill(0),writes:[]};allocations.push(a);registers.eax=a;}
 else if(target===0x1096ae10){const a=stack[0],ref=stack[1];if(a?.kind!=='array'||ref?.kind!=='slot')throw Error('InitializeArray args');a.values=blob(ref.va,a.values.length,pc);a.blob_slot=ref.va;}
 else if(target===0x1010aa00)registers.eax=stack[0];
 else if(target===0x100025d0){if(stack[0]?.kind!=='array'||stack[1]?.kind!=='array')throw Error('type-check helper arguments');registers.eax=stack[1];}
 else if(target===0x10002bb0){const a=registers.ecx,n=stack[0],v=stack[1];if(a?.kind!=='array'||!Number.isInteger(n)||n<0||n>=a.values.length||v?.kind!=='array')throw Error('array-store helper arguments');a.values[n]=v;a.writes.push({at:pc,helper:target,index:n,value:{array:v.id}});registers.ecx={kind:'address',owner:a,key:0x10+n*4};registers.eax=v;stack.splice(0,2);}
 else if(target===0x10002be0){const a=registers.ecx,n=stack[0],v=stack[1];if(a?.kind!=='array'||!Number.isInteger(n)||n<0||n>=a.values.length||typeof v!=='number')throw Error('integer-store helper arguments');a.values[n]=v|0;a.writes.push({at:pc,helper:target,index:n,value:v|0});registers.edx=n;registers.eax=v;stack.splice(0,2);}
 else if(target!==0x1010aeb0&&target!==0x1010a9f0)throw Error('unknown call '+target.toString(16));
 }
 else if(/^j/.test(op)){if(op!=='jmp'&&!flags)throw Error('unknown flags');const f=flags||{};const takes={je:f.z,jne:!f.z,jz:f.z,jnz:!f.z,jbe:f.below||f.z,jae:!f.below,jl:f.lt,jg:f.gt,jmp:true};if(!Object.hasOwn(takes,op))throw Error('unknown branch');if(takes[op]){const target=Number(i.nearBranchTarget);if(target<method.va||target>=method.va+max)throw Error('branch boundary');decoder.position=target-method.va;decoder.ip=BigInt(target);}}
 else throw Error('unknown opcode '+op);
 if(['add','sub','shl','xor','call'].includes(op))flags=null;
 done=Object.keys(plan.targets).every(k=>staticFields.has(plan.slot+':'+Number(k)));
 }catch(e){console.error(JSON.stringify(history));throw Error('0x'+pc.toString(16)+' '+text+': '+e.message);}finally{i.free();}
 if(done)break;
}
if(!done)throw Error('target not reached');
const depths={HUMAN_ANIME_SEB:1,HUMAN_ANIME_SEB_F:2,WEAPON_ANIME_F:3,body_off:2,WEAPON_WALK_POS:4,WEAPON_SEB:1};
function validateTree(v,depth){if(depth===0){if(typeof v!=='number'||!Number.isInteger(v))throw Error('integer leaf');return;}if(v?.kind!=='array')throw Error('unfilled reference array');v.values.forEach(x=>validateTree(x,depth-1));}
const targets=Object.entries(plan.targets).map(([off,name])=>{const a=staticFields.get(plan.slot+':'+Number(off));validateTree(a,depths[name]);const relevant=new Set();function visit(v){if(v?.kind==='array'){relevant.add(v.id);v.values.forEach(visit);}}visit(a);return {name,offset:Number(off),publication:events.find(e=>e.slot===plan.slot&&e.field_offset===Number(off)),values:plain(a),arrays:allocations.filter(a=>relevant.has(a.id)).map(a=>({id:a.id,at:a.at,length:a.values.length,blob_slot:a.blob_slot,writes:a.writes}))};});
console.log(JSON.stringify({qualification:'固定cctor成功分配路径静态解释；不执行原游戏；不是任意IL解释器',mode,source:{dll:sha(dll),metadata:sha(meta)},method,rva_end:plan.end,inspected_end:last,inspected_bytes:last-method.va,window_sha256:sha(dll.subarray(method.offset,method.offset+last-method.va)),steps,targets,helpers:mode==='weapon'?helpers:[],blobs:blobEvidence.filter(b=>targets.some(t=>t.arrays.some(a=>a.blob_slot===b.slot)))},null,2));formatter.free();decoder.free();
