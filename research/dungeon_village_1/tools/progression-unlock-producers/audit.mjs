// 从既有31活动及已确认程序列派生“产生者”索引；不复制定义表、不改原始输入。
import {archiveFs as fs, archiveWork} from '../work_archive_paths.mjs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const here=archiveWork(import.meta.url),root=path.resolve(here,'../..');
await fs.mkdir(here,{recursive:true});
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(v,m)=>{if(!v)throw Error(m);};
const text=p=>fs.readFile(path.join(root,p),'utf8');
const rows=s=>s.trimEnd().split(/\r?\n/).map((line,i)=>({line:i+1,c:line.split('\t')}));
const activities=rows(await text('data/world/asEventData.txt'));
const facilities=rows(await text('data/original/tenantData.txt'));
const events=rows(await text('data/scripts/original/events.txt'));
const popular=rows(await text('data/scripts/original/popularBonus.txt'));
const index=JSON.parse(await text('data/PROGRESSION_CONTENT_INDEX.json'));
need(activities.length===31&&index.tables.find(t=>t.kind==='village_activity').definitions===31,'活动分母变化');
need(activities.every((r,i)=>Number(r.c[0])===i),'活动数组索引与定义ID映射变化');
const records=activities.map(r=>({activity_id:Number(r.c[0]),definition_reference:{path:'data/PROGRESSION_CONTENT_INDEX.json',
    table:'village_activity',id:Number(r.c[0])},producers:[]}));
const push=(id,producer)=>{const r=records.find(r=>r.activity_id===id);need(r,'产生者目标缺定义');r.producers.push(producer);};
for(const r of activities){
    const id=Number(r.c[0]);
    if(Number(r.c[11])&1)push(id,{kind:'reset_flag1',consumer:'c/n.java:3336-3345',effect:'p1,r最终false,m0',
        maintenance:'prototype/src/startup_world_runtime.cpp'});
    if(Number(r.c[8])>0)push(id,{kind:'rank_commit',rank:Number(r.c[8]),consumer:'b/g.java:4593-4600',
        effect:'a/c.a()直接p1，旧p0才r=true；notice6，不创建95',maintenance:'prototype/src/startup_world_runtime_pages.cpp'});
}
const parse=s=>s.split('&').filter(Boolean).map(x=>x.split(',').map(Number));
for(const r of popular)for(const [opcode,id] of parse(r.c[5]))if(opcode===33)
    push(id,{kind:'popularity_reward_page95',reward_definition:Number(r.c[0]),threshold:Number(r.c[2]),
        source:{path:'data/scripts/original/popularBonus.txt',line:r.line},
        consumer:'c/n.java:738-782 -> d/a.java:904 -> b/g.java:6147',
        effect:'首次未领人气奖励调度；33仅生成95/r11；满40确认才a/c.a()'});
const triggers={73:{kind:'construction_flag',flag:256,facility_id:63},74:{kind:'construction_flag',flag:512,facility_id:62},
    75:{kind:'construction_flag',flag:1024,facility_id:64},76:{kind:'construction_flag',flag:2048,facility_id:55},
    102:{kind:'magic_pot_process',minimum_experience:30,priority:'仅未进入>=100分支且未见事件102'},
    103:{kind:'magic_pot_process',minimum_experience:100,priority:'优先于30分支，未见事件103'}};
for(const r of events)for(const [opcode,id] of parse(r.c[4])){
    const event=Number(r.c[0]);
    if(opcode===17){need(triggers[event],'新opcode17产生者需人工追调用');
        push(id,{kind:'event_direct17',event,trigger:triggers[event],source:{path:'data/scripts/original/events.txt',line:r.line},
            effect:'opcode17即时p1/新r及条件事件61/notice6，不创建95'});}
    if(opcode===33){need(event===43||event===45,'新opcode33事件需人工追调用');
        push(id,{kind:'rank_event_reward95',rank:event===43?2:4,event,
            source:{path:'data/scripts/original/events.txt',line:r.line},
            effect:'晋级事件延迟后创建95/r11；满40确认才p1/新r'});}
}
for(const trigger of Object.values(triggers))if(trigger.kind==='construction_flag'){
    const match=facilities.filter(r=>(Number(r.c[35])&trigger.flag)!==0);
    need(match.length===1&&Number(match[0].c[0])===trigger.facility_id,'建筑完成标志到定义映射变化');
}
const museum=facilities.find(r=>Number(r.c[0])===64);
need(museum?.c.length===36&&Number(museum.c[12])===200&&Number(museum.c[13])===3000&&
     Number(museum.c[32])===4&&Number(museum.c[35])===1124,'博物馆源字段变化');
need(records.filter(r=>r.producers.length).length===30&&records.find(r=>r.activity_id===27).producers.length===0,
    '30正向产生者/祭典缺口范围变化');
const page=await text('work/decompiled/sources/b/g.java');
need(page.includes('if (i != 27 && cVar.p == 1')&&page.includes('oVar2.D <= this.bG.k')&&
     page.includes('this.bG.c(oVar2.s);'),'目录/兑换源谓词变化');
const sources=['data/PROGRESSION_CONTENT_INDEX.json','data/world/asEventData.txt','data/original/tenantData.txt',
    'data/scripts/original/events.txt','data/scripts/original/popularBonus.txt','data/world/magicPot.txt',
    'work/decompiled/sources/a/c.java','work/decompiled/sources/a/o.java','work/decompiled/sources/b/g.java',
    'work/decompiled/sources/c/n.java','work/decompiled/sources/c/m.java','work/decompiled/sources/d/a.java',
    'example/src/world_village_activity.cpp','example/src/world_scripts.cpp','example/src/world_gift_page.cpp',
    'example/src/world_facility_update.cpp','prototype/src/startup_world_commerce.cpp',
    'prototype/src/startup_world_runtime_pages.cpp','prototype/src/startup_world_village_activity.cpp',
    'prototype/src/startup_world_magic_pot.cpp','prototype/src/startup_world_codec_fields.json'];
const manifest=[];for(const p of sources){const b=await fs.readFile(path.join(root,p));manifest.push({path:p,bytes:b.length,sha256:hash(b)});}
const result={version:1,date:'2026-10-09',qualification:'apk_static_producer_to_consumer_mapping_not_natural_run',
    source_profiles:index.profiles,denominator:31,positive_producer_definitions:30,unclosed_positive_producers:[27],
    activity_records:records,
    museum64:{source:{path:'data/original/tenantData.txt',line:museum.line},rank:4,exchange_village_points:200,
        ordinary_build_gold:3000,flags:1124,chain:['rank4 -> catalogue85 while p!=2',
        'pay village points -> raw93 r3/s64','confirm at counter>=40 -> H+1(max99), oldp0:p2/q0/rtrue',
        'normal build -> actual construction completion -> flag1024 events75/215','event75 opcode17 -> activity21 p1'],
        limitation:'一条已核普通路径；不证明所有可能获取途径、自然五星或Steam消费者。'},
    activity27:{initial_flags:0,rank_producer:-1,ordinary_catalogue:'explicit index27 exclusion',
        positive_producer:'none in confirmed reset/rank/event/popularity routes',effect_type4:'no dedicated effect/render branch',
        persistence:'原活动分区仍逐定义保存o/p/r/m，不能因普通目录排除而丢字段',
        limitation:'不将未找到正向产生者证明为全程序/任意外部存档中绝对不可达'},
    sources:manifest};
const output=JSON.stringify(result,null,2)+'\n';await fs.writeFile(path.join(here,'EVIDENCE.json'),output);
console.log(JSON.stringify({activities:records.length,positive:30,unclosed:[27],sources:manifest.length,bytes:Buffer.byteLength(output)}));
