// 独立复核派生数组中的原指令常量和Steam资源摘要；不再次解释cctor、不写原输入。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
const methods=require('../persistence-replay-analysis/exe/methods.json');
const h=require('./human.json'),w=require('./weapon.json'),resources=require('./RESOURCES.json');
if(sha(dll)!==h.source.dll||h.source.dll!==w.source.dll)throw Error('source');
const walk=w.targets.find(t=>t.name==='WEAPON_WALK_POS'),byId=new Map(walk.arrays.map(a=>[a.id,a]));
const leafWrites=[];
function visit(id,indices=[]){const a=byId.get(id);if(!a)throw Error('array reference');for(const write of a.writes){if(typeof write.value==='number'){
 const va=write.at,rva=va-methods.base,s=methods.sections.find(s=>rva>=s.rva&&rva+7<=s.rva+s.rawSize);if(!s)throw Error('write mapping');const offset=s.raw+rva-s.rva,b=dll.subarray(offset,offset+7);
 // 正常行走64对坐标均为 mov dword ptr [ebx+disp8],imm32；不借抽象解释器读数。
 if(b[0]!==0xc7||b[1]!==0x43||b[2]!==0x10+write.index*4||b.readInt32LE(3)!==write.value)throw Error('independent immediate mismatch');
 leafWrites.push({indices:[...indices,write.index],va,offset,value:b.readInt32LE(3),sha256:sha(b)});
 }else visit(write.value.array,[...indices,write.index]);}}
visit(walk.publication.array);
if(leafWrites.length!==128)throw Error('walk coordinate coverage');
const get=(g,n)=>resources.identities.find(r=>r.group===g&&r.name===n);
const bodies=resources.selected_seb.filter(s=>s.group==='human');
for(const s of bodies){if(s.layers!==1||s.parts.length!==4)throw Error('body shape');for(const p of s.parts){const x=[0,18,0,36][p.frame];if(JSON.stringify(p.crop)!==JSON.stringify([x,24*s.index,18,24])||JSON.stringify(p.offset)!=='[-9,-24]'||JSON.stringify(p.flip)!=='[0,0]')throw Error('body frame');}}
const weaponSprites=resources.selected_seb.filter(s=>s.group==='weapon');
for(const s of weaponSprites){const style=Math.floor(s.index/4),facing=s.index%4,[width,height]=[[21,25],[21,25],[26,28],[32,35]][style];if(s.layers!==1||s.parts.length!==1||JSON.stringify(s.parts[0].crop)!==JSON.stringify([0,height*facing,width,height])||JSON.stringify(s.parts[0].offset)!=='[0,0]')throw Error('weapon frame');}
const artifacts=['arrays.cjs','human.json','weapon.json','resources.py','RESOURCES.json','inspect.cjs'].map(name=>{const b=fs.readFileSync(path.join(__dirname,name));return {name,bytes:b.length,sha256:sha(b)};});
const out={date:'2026-10-09',qualification:'独立机器码立即数复核128项；Steam SEB结构独立期望抽查；未做窗口或C++测试',sources:h.source,methods:[{name:h.method.type,begin:h.method.va,end:h.inspected_end,bytes:h.inspected_bytes,sha256:h.window_sha256},{name:w.method.type,begin:w.method.va,end:w.inspected_end,bytes:w.inspected_bytes,sha256:w.window_sha256}],helpers:w.helpers,independent_walk_immediates:leafWrites,resources:{identity_count:resources.identities.length,identical_to_apk:resources.identities.filter(r=>r.apk_bytes_equal).length,body_frame_checks:16,weapon_frame_checks:16,shadow:resources.selected_seb.find(s=>s.group==='common')},artifacts,limits:['success allocation and array type-check path only','no general interpreter or actual program execution','full effects, shared scratch reset and title lifecycle remain separate','resource byte equality does not certify entire Steam UI']};
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(out,null,2)+'\n');console.log(JSON.stringify({independent_coordinates:leafWrites.length,body_frames:16,weapon_frames:16,resources:out.resources.identity_count,bytes:artifacts.reduce((a,x)=>a+x.bytes,0)}));
