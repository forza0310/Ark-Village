// 只读既有定义与消费者，输出定位/hash及本批新增交叉结论，不复制原表或反编译实现。
import {archiveFs as fs, archiveWork} from '../work_archive_paths.mjs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const here=archiveWork(import.meta.url);
const root=path.resolve(here,'../..');
await fs.mkdir(here,{recursive:true});
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(v,m)=>{if(!v)throw Error(m);};
const text=p=>fs.readFile(path.join(root,p),'utf8');
const rows=s=>s.trimEnd().split(/\r?\n/).map((line,i)=>({line:i+1,c:line.split('\t')}));
const index=JSON.parse(await text('data/PROGRESSION_CONTENT_INDEX.json'));
const quests=rows(await text('data/world/questData.txt'));
const activities=rows(await text('data/world/asEventData.txt'));
const monsters=rows(await text('data/world/monster.txt'));
const events=rows(await text('data/scripts/original/events.txt'));
const news=(await text('data/scripts/original/news.txt')).trimEnd().split('|');
need(index.definitions===739 && quests.length===81 && activities.length===31,'冻结内容分母变化');
need(index.tables.find(x=>x.kind==='quest').definitions===quests.length &&
     index.tables.find(x=>x.kind==='village_activity').definitions===activities.length,'索引/消费源分母不符');
const boss=quests.filter(r=>(Number(r.c[16])&2)!==0);
need(boss.length===6 && boss.map(r=>Number(r.c[0])).join(',')==='39,47,55,63,72,80','特殊任务目录身份变化');
const target=monsters.filter(r=>(Number(r.c[16])&4)!==0&&Number(r.c[4])===5);
need(target.length===1&&Number(target[0].c[0])===35,'最终BOSS新闻实际谓词不唯一');
const event217=events.find(r=>Number(r.c[0])===217),event152=events.find(r=>Number(r.c[0])===152);
need(event217?.c[4]==='6,300&4,17'&&news[17].includes('机械王野吉'),'事件217新闻消费者交叉失败');
need(event152?.c[4]==='6,100&2,123&38,0&39,0','BOSS延迟奖励脚本变化');
const character=await text('work/decompiled/sources/c/b.java');
const reader=await text('work/decompiled/sources/a/k.java');
need(character.includes('kVarO.f59f == 5 && !aVarA.f(217)')&&
     reader.includes('this.f59f = Integer.parseInt(strArrA[4]);'),'原怪物字段/致死分支定位变化');
const grouping={};
for(const r of quests){const key=`kind${r.c[3]}_flags${r.c[16]}`;grouping[key]=(grouping[key]??0)+1;}
const sources=[
 'data/PROGRESSION_CONTENT_INDEX.json','data/world/questData.txt','data/world/asEventData.txt','data/world/monster.txt',
 'data/scripts/original/events.txt','data/scripts/original/news.txt',
 'work/decompiled/sources/a/n.java','work/decompiled/sources/a/m.java','work/decompiled/sources/a/k.java',
 'work/decompiled/sources/b/c.java','work/decompiled/sources/c/b.java','work/decompiled/sources/c/f.java',
 'work/decompiled/sources/c/k.java','work/decompiled/sources/c/n.java','work/decompiled/sources/d/a.java',
 'example/src/world_calendar_tasks.cpp','example/src/world_task_creation.cpp','example/src/world_task_deadline.cpp',
 'example/src/world_dungeon.cpp','example/src/world_scripts.cpp','example/src/combat_execution.cpp',
 'example/src/battle_commit.cpp','example/src/encounter_lifecycle.cpp',
 'prototype/src/startup_world_runtime.cpp','prototype/src/startup_world_runtime_tasks.cpp',
 'prototype/src/startup_world_runtime_pages.cpp'];
const evidence=[];
for(const p of sources){const bytes=await fs.readFile(path.join(root,p));evidence.push({path:p,bytes:bytes.length,sha256:hash(bytes)});}
const result={version:1,date:'2026-10-09',qualification:'read_only_apk_static_and_maintained_consumer_crosscheck',
 denominator:{indexed_definitions:739,quests:81,village_activities:31,quest_groups:grouping},
 source_profiles:index.profiles,sources:evidence,
 closures:[{id:'boss-news-217',source:'c/b.java:1320 -> a/k.java:68 -> monster.txt -> events217 -> news17',
   predicate:{monster_flags_all:4,monster_column4:5,event217_absent:true},matching_monster_id:35,
   matching_monster_name:target[0].c[1],internal_event_label:event217.c[1],news_record_index:17,
   conclusion:'实际消费者和新闻正文均指向机械王野吉；内部事件标签不能把它绑定普通怪物29。',
   limitation:'未执行自然击败/原版动态，未认证EXE同一分支。'},
  {id:'boss-reward-152-resume-phase',source:'c/f.java:537-542 -> d/a.java:647-653,933-943 -> c/k.java:72-73 -> c/n.java:4730-4737',
   conclusion:'事件152先挂延迟续体；38/39在续体执行时读取当时探索阶段x，95领取时再交付预先捕获金额。',
   limitation:'局部静态顺序与维护消费者一致；不承诺任意抢先新增任务/脚本条件下x不会再次改变。'}],
 special_boss_quests:boss.map(r=>({definition:Number(r.c[0]),stage:Number(r.c[5]),monster:Number(r.c[11]),
  name:r.c[1],source_line:r.line})),
 unclosed:['自然第二至五星','六特殊BOSS逐个自然出现/接受/胜利/再次出现','81任务与31活动逐定义自然开放',
  '任务特殊选择模式全部设置者','Steam完整后期消费者等价','未发现定义等于不可达/废弃的证明'],
 scope:'不改原表、产品、Owner或schema；不构建/运行游戏；原任务选择普通Java有已知坏分支，不据此重写维护规则'};
const output=JSON.stringify(result,null,2)+'\n';
await fs.writeFile(path.join(here,'EVIDENCE.json'),output);
console.log(JSON.stringify({source_files:evidence.length,evidence_bytes:Buffer.byteLength(output),quest_count:quests.length,
 activity_count:activities.length,boss_quests:boss.length,news217_monster:target[0].c[0]}));
