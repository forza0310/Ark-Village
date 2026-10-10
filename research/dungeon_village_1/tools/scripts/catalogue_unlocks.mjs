// 只读冻结表与现有定义索引，派生入口索引；不运行游戏或复制定义大表。
import fs from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
// 从tools/scripts显式定位研究根；复算只生成work候选，发布由维护者核验后完成。
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const outputPath=path.join(root,'work/catalogue-unlock-producers/EVIDENCE.json');
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(v,m)=>{if(!v)throw Error(m);};
const sources=new Map();
async function read(p){const b=await fs.readFile(path.join(root,p));sources.set(p,{path:p,bytes:b.length,sha256:hash(b)});return b;}
const index=JSON.parse(await read('data/PROGRESSION_CONTENT_INDEX.json'));
const encoded=await read('work/original-save-analysis/apk/xls.dat');
need(hash(encoded)==='8baacbb181dcd4eb18435eb39938ef2ad21d7dee9db27654b07c3428f4739958','冻结xls身份变化');
// 复用既有progression-content-audit的已核归档格式，仅在内存中取表。
const key=Buffer.alloc(44);
[-1387743643,321849466,-380916995,1114766278,1209944503,138008561,-893766998,
 -1242421477,-1230126924,626883230,1684377624].forEach((v,i)=>key.writeInt32LE(v,i*4));
const archive=Buffer.from(encoded);for(let i=0;i<archive.length;i++)archive[i]^=key[i%44];
let at=0;const u32=()=>{need(at+4<=archive.length,'归档整数越界');const n=archive.readUInt32BE(at);at+=4;return n;};
u32();const payload=u32(),count=u32();need(count>0&&count<1000,'归档数量越界');
const names=[];for(let i=0;i<count;i++){const n=u32();need(n>0&&n<4096&&at+n<=archive.length,'归档名越界');names.push(archive.toString('utf8',at,at+n));at+=n;}
const offsets=names.map(u32),sizes=names.map(u32),flags=archive.subarray(at,at+count);at+=count;
need(at+payload===archive.length,'归档载荷不符');
const tables=new Map();
for(const t of index.tables){
 const i=names.indexOf(t.entry);need(i>=0,'缺表');
 const start=at+offsets[i];need(start+4<=archive.length,'条目头越界');
 const n=archive.readUInt32BE(start);
 need(start+4+n<=archive.length&&!(flags[i]&1&&sizes[i]),'条目边界/压缩不符');
 const b=archive.subarray(start+4,start+4+n);need(hash(b)===t.sha256,'表哈希不符');
 const rows=b.toString('utf8').trimEnd().split(/\r?\n/).map(s=>s.split('\t'));
 need(rows.length===t.definitions&&rows.every(r=>r.length===t.columns),'表结构不符');
 need(t.records.length===rows.length&&rows.every((r,j)=>Number(r[0])===t.records[j].id),'索引与原表ID顺序不符');
 tables.set(t.kind,rows);
}
const kinds=['facility','weapon','armour','accessory','item','magic_recipe'];
const expected=[85,33,50,30,36,40];
const groups=kinds.map((kind,i)=>{const t=index.tables.find(t=>t.kind===kind);need(t.definitions===expected[i],'定义分母变化');return {kind,definitions:t.definitions,definition_index:'data/PROGRESSION_CONTENT_INDEX.json',records:t.records.map(r=>({id:r.id,producers:[]}))};});
const routeLimits={};
function push(kind,id,producer){const row=groups.find(g=>g.kind===kind)?.records.find(r=>r.id===id);need(row,'产生者指向未知定义');if(producer.limits){routeLimits[producer.route]=producer.limits;delete producer.limits;}row.producers.push(producer);}
const flagColumns={facility:35,weapon:18,armour:12,accessory:12,item:24,magic_recipe:9};
const difficultyColumns={weapon:5,armour:4,accessory:4,item:8};
// bit2相同不代表完整谓词相同：道具不检查p，装备必须p!=1；任务和战斗的池亦不同。
const poolContracts={
 item:{flag:2,status_filter:null,ordinary_task:'difficulty<=effective_difficulty',rare_task:'difficulty==effective_difficulty+1',battle:'difficulty<=rolled_difficulty',task_removal_after_selection:false},
 equipment:{flag:2,status_filter:'p!=1',ordinary_task:'difficulty<=effective_difficulty',rare_task:'difficulty==effective_difficulty+1',battle:'15%装备尝试成立且difficulty<=rolled_difficulty',task_removal_after_selection:true},
 source:{task:'work/decompiled/sources/c/n.java:585-623',battle:'work/decompiled/sources/c/e.java:60-90',maintenance:'example/src/world_task_creation.cpp:280-288'},
 difficulty_bounds:{task_effective:[1,9],battle_rolled:[1,9],rare_task_can_include:10},
 limits:'任务先生成候选记录，非最终目录授予；战斗生成掉落实体，非库存已领取。难度、随机、挑战替换及实际领取均须另外满足。'
};
for(const group of groups){const t=index.tables.find(t=>t.kind===group.kind);for(const r of t.records){
 const sourceRow=tables.get(group.kind).find(row=>Number(row[0])===r.id);
 need(Number(sourceRow[flagColumns[group.kind]])===r.flags,'索引flags与冻结原表不符');
 if(r.flags&1)push(group.kind,r.id,{route:'reset_flag1',level:'static_initial_state'});
 if(group.kind in difficultyColumns&&(r.flags&2)){
  const difficulty=Number(sourceRow[difficultyColumns[group.kind]]);
  need(Number.isInteger(difficulty)&&difficulty>=1&&difficulty<=10,'奖励难度范围变化');
  push(group.kind,r.id,{route:'task_or_battle_reward_pool_flag2',level:'conditional_pool_eligibility',contract:group.kind==='item'?'item':'equipment',difficulty,
   ordinary_task_difficulty_can_match:difficulty<=9,battle_difficulty_can_match:difficulty<=9,
   limits:'仅静态bit2资格；具体准入谓词见pool_contracts，不能合并道具与装备状态过滤；difficulty10仅可能进任务稀有池，不进战斗池'});
 }
 if(group.kind==='weapon'&&(r.flags&16))push(group.kind,r.id,{route:'first_task_final_reward',level:'conditional_selection',limits:'总成功数0的最后一份奖励；不等于创建任务即领取'});
 if(group.kind==='magic_recipe'&&(r.flags&2))push(group.kind,r.id,{route:'magic_recipe_discovery46',level:'conditional_page',required_experience:Number(tables.get('magic_recipe')[r.id][4]),limits:'处理m()筛选阈值后生成46，计数>6确认才发现'});
}}
for(const r of tables.get('facility'))if(Number(r[32])>=0)push('facility',Number(r[0]),{route:'commerce85_rank',level:'conditional_catalogue',rank:Number(r[32]),limits:'p!=2；付村子点数后93满40确认'});
for(const r of tables.get('item')){
 const id=Number(r[0]);
 if(Number(r[17])>0)push('item',id,{route:'reset_inventory',level:'static_initial_stock',quantity:Number(r[17])});
 if(Number(r[18])>0)push('item',id,{route:'reset_commerce_stock',level:'conditional_catalogue',quantity:Number(r[18])});
 if(Number(r[7])>=0&&Number(r[20])>0)push('item',id,{route:'commerce_restock_H',level:'conditional_supply',rank:Number(r[7]),limits:'缺货量与随机使本次增加可为0；p0时只开放，尚未得到z库存'});
}
const recipeKinds=['item','weapon','armour','accessory','facility'];
for(const r of tables.get('magic_recipe')){const recipe=Number(r[0]),kind=recipeKinds[Number(r[2])],id=Number(r[3]);need(kind,'配方结果类别变化');push(kind,id,{route:'magic_manufacture47',recipe,level:Number(r[9])&2?'conditional_recipe_result':'excluded_recipe_literal',limits:'无flags2不在43普通目录；有目录资格仍需发现/元素足额；47计数>6重核，设施还须93领取'});}
const opKind={20:'facility',24:'facility',25:'weapon',26:'armour',27:'accessory',28:'item',34:'facility'};
function program(value,source){for(const token of value.split('&').filter(Boolean)){const ins=token.split(',').map(Number);need(ins.every(Number.isInteger),'程序列非整数');const kind=opKind[ins[0]];if(kind)push(kind,ins[1],{route:ins[0]===20?'script_direct20':ins[0]===34?'script_page95':'script_page94',level:'literal_script_reference',opcode:ins[0],source,limits:'已定位程序列引用；未单凭引用认证上游事件/人物可自然到达'});}}
for(const r of tables.get('event'))program(r[4],{table:'event',id:Number(r[0]),column:4});
for(const r of tables.get('popularity_bonus'))program(r[5],{table:'popularity_bonus',id:Number(r[0]),column:5,threshold:Number(r[2])});
for(const r of tables.get('human'))for(const column of [9,10])program(r[column],{table:'human',id:Number(r[0]),column});
for(const group of groups){
 const hasEntry=r=>r.producers.some(p=>p.level!=='excluded_recipe_literal');
 group.definitions_with_indexed_entry=group.records.filter(hasEntry).length;
 group.no_indexed_entry=group.records.filter(r=>!hasEntry(r)).map(r=>r.id);
 group.pool_flag2_definitions=group.records.filter(r=>r.producers.some(p=>p.route==='task_or_battle_reward_pool_flag2')).length;
 group.pool_difficulty10_ids=group.records.filter(r=>r.producers.some(p=>p.route==='task_or_battle_reward_pool_flag2'&&p.difficulty===10)).map(r=>r.id);
}
need(groups.map(g=>g.definitions_with_indexed_entry).join(',')==='47,33,49,27,36,38','已核入口范围变化，需人工重新核对');
for(const p of ['work/decompiled/sources/a/o.java','work/decompiled/sources/a/p.java','work/decompiled/sources/a/b.java','work/decompiled/sources/a/a.java','work/decompiled/sources/a/g.java','work/decompiled/sources/a/i.java','work/decompiled/sources/a/e.java','work/decompiled/sources/a/h.java','work/decompiled/sources/b/g.java','work/decompiled/sources/c/n.java','work/decompiled/sources/c/e.java','work/decompiled/sources/d/a.java','example/src/world_task_creation.cpp','example/src/human_growth.cpp','example/src/human_management.cpp','prototype/src/startup_world_commerce.cpp','prototype/src/startup_world_magic_pot.cpp'])await read(p);
await read('tools/scripts/catalogue_unlocks.mjs');
const result={version:2,date:'2026-10-10',qualification:'static_entry_and_conditional_pool_audit_not_natural_reachability',route_limits:routeLimits,pool_contracts:poolContracts,groups,
 mastery:{catalogue_equipment_producer:false,source:'a/e.java:579-603; b/g.java raw70',effect:'等级10生成70；当前职业p0则立即开放职业并生成94/r4，不发装备/设施/普通道具；70确认另给人物属性加成'},
 exclusions:['未枚举任意存档外部状态','本索引不证明Steam消费者等价','无入口索引不证明绝对不可达','配方0为dummy，结果引用不能单独证明可制造'],sources:[...sources.values()]};
const output=JSON.stringify(result,null,2)+'\n';
await fs.mkdir(path.dirname(outputPath),{recursive:true});
await fs.writeFile(outputPath,output);
console.log(JSON.stringify({groups:groups.map(g=>({kind:g.kind,total:g.definitions,indexed:g.definitions_with_indexed_entry,unknown:g.no_indexed_entry})),sources:sources.size,bytes:Buffer.byteLength(output)}));
