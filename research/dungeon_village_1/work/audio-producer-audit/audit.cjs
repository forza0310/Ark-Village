// 只读声音生产者静态审计：逐入队点分类，原音频操作不由资源ID猜测。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(path.join(root,p));let checks=0;
const need=(v,m)=>{++checks;if(!v)throw Error(m);};
const windows=[
 ['bgm-wrapper','work/decompiled/sources/d/a.java',3436,3446],
 ['ordinary-wrapper','work/decompiled/sources/d/a.java',3787,3792],
 ['jingle-wrapper','work/decompiled/sources/d/a.java',3936,3940],
 ['restore-bgm-selector','work/decompiled/sources/d/a.java',4154,4161],
 ['rank-music-and-finish','work/decompiled/sources/b/g.java',4625,4641],
 ['activity-jingle','work/decompiled/sources/b/g.java',4719,4725],
 ['human-unlock-jingle','work/decompiled/sources/b/g.java',4878,4886],
 ['upgrade-facility-jingle','work/decompiled/sources/b/g.java',5699,5723],
 ['trade-sound','work/decompiled/sources/b/g.java',5834,5843],
 ['award-order','work/decompiled/sources/b/g.java',5909,5949],
 ['trade-reward-jingle','work/decompiled/sources/b/g.java',6086,6092],
 ['gift-jingle','work/decompiled/sources/b/g.java',6116,6122],
 ['tax-jingle','work/decompiled/sources/b/g.java',6152,6158],
 ['message-jingle','work/decompiled/sources/b/g.java',11366,11377],
 ['task-success-jingle','work/decompiled/sources/b/g.java',11921,11927],
 ['gift-render-ordinary','work/decompiled/sources/b/g.java',6508,6521],
 ['item-use-render-ordinary','work/decompiled/sources/b/g.java',7604,7622],
 ['clear-finish-order','work/decompiled/sources/b/g.java',6894,6904],
 ['build-road-edit-ordinary','work/decompiled/sources/b/c.java',1729,1891],
 ['global-delay-ordinary','work/decompiled/sources/d/a.java',3993,4010],
 ['notice-ordinary','work/decompiled/sources/d/a.java',4234,4249],
 ['notice-factory','work/decompiled/sources/d/a.java',1039,1046],
 ['notice-factory-default-delay','work/decompiled/sources/d/a.java',3452,3454],
 ['notice-text-and-delay','work/decompiled/sources/c/n.java',95,96],
 ['actor-sound-wrapper','work/decompiled/sources/c/n.java',806,810],
 ['actor-contact-ordinary','work/decompiled/sources/c/b.java',1228,1248],
 ['actor-effect-ordinary','work/decompiled/sources/c/b.java',4754,4779],
 ['actor-cast-ordinary','work/decompiled/sources/c/b.java',4988,4997],
 ['actor-death-ordinary','work/decompiled/sources/c/b.java',5095,5106],
 ['actor-rescue-ordinary','work/decompiled/sources/c/b.java',5282,5295],
 ['actor-delayed-ordinary','work/decompiled/sources/c/b.java',5450,5469],
 ['actor-pickup-ordinary','work/decompiled/sources/c/b.java',3989,3998],
 ['task-new-bgm','work/decompiled/sources/c/b.java',359,369],
 ['task-abort-bgm','work/decompiled/sources/c/n.java',4697,4711],
 ['title-bgm','work/decompiled/sources/b/h.java',291,299],
 ['world-initialize-bgm','work/decompiled/sources/b/c.java',1176,1185],
 ['pot-render-ordinary','work/decompiled/sources/c/n.java',3052,3064],
 ['task-low-candidate','example/src/world_encounters.cpp',81,89],
 ['task-path-candidate','example/src/world_departure.cpp',794,805],
 ['task-direct-daily-candidate','example/src/world_daily.cpp',126,159],
 ['task-path-state-case','example/src/world_daily.cpp',259,280],
 ['task-direct-state5-case','example/src/world_daily.cpp',280,303],
 ['task-direct-state11-case','example/src/world_daily.cpp',382,394],
 ['task-bind-instead-of-create','example/src/ai_perception.cpp',40,48],
 ['task-route-handoff','example/src/world_actor_routes.cpp',239,273],
 ['award-maintained-order','example/src/world_award_page.cpp',135,173],
 ['session-commit','prototype/src/startup_world_runtime.cpp',1157,1171],
 ['session-output-swap','prototype/src/startup_world_runtime.cpp',993,998],
 ['app-output-forward','prototype/src/startup_application.cpp',280,282],
 ['cli-output-consumer','prototype/src/startup_application_main.cpp',53,61],
 ['normal-output-empty-boundary','prototype/src/startup_world_persistence.cpp',122,126]
];
const sources=windows.map(([purpose,p,first,last])=>{const b=read(p),l=b.toString('utf8').split(/\r?\n/);
 need(last<=l.length,'source window '+p);return {purpose,path:p,first,last,bytes:b.length,sha256:sha(b),window_lf_sha256:sha(Buffer.from(l.slice(first-1,last).join('\n')))};});
function classify(file,line){
 if(file==='startup_world_runtime_pages.cpp'){
  if(line.includes('push_back(3)')||line.includes('push_back(effect.value)')||line.includes('active_task'))return ['b','page50/87; restore via d/a.g'];
  if(line.includes('push_back(5)')||line.includes('push_back(4)')||line.includes('*command == 0 ? 4 : 6')||line.includes('push_back(*result.candidate->sound)'))
   return ['d','page59/96/11/30/94/95 jingle; exact page in enclosing source'];
  return null;
 }
 if(['startup_application.cpp','startup_world_runtime_deadline.cpp','startup_world_runtime_tasks.cpp'].includes(file))return ['b','d/a.g restore selector'];
 if(file==='startup_world_building.cpp')return line.includes('push_back(20)')?['d','page81']:['c','build'];
 if(file==='startup_world_commerce.cpp')return line.includes('push_back(25)')?['c','trade A[B]']:['d','page93'];
 if(file==='startup_world_facility_catalog.cpp')return ['d','page82'];
 if(file==='startup_world_village_activity.cpp')return ['d','page53'];
 if(['startup_world_runtime_scene.cpp','startup_world_runtime_nonactors.cpp','startup_world_editing.cpp','startup_world_presentation.cpp'].includes(file))return ['c','ordinary direct or c/n.a screen-qualified'];
 return null;
}
const emitters=[];
for(const f of fs.readdirSync(path.join(root,'prototype/src')).filter(f=>f.endsWith('.cpp')).sort()){
 const p='prototype/src/'+f,b=read(p),lines=b.toString('utf8').split(/\r?\n/);
 lines.forEach((l,i)=>{
  if(!/\.sound_requests\.(push_back|insert)\(/.test(l))return;
  const operation=classify(f,l);need(operation,'new unclassified enqueue '+p+':'+(i+1));
  emitters.push({path:p,line:i+1,sha256:sha(b),operation:operation[0],source_group:operation[1]});
 });
}
need(emitters.length>20,'production enqueue inventory');
const production=[];
for(const dir of ['prototype/src','example/src'])for(const f of fs.readdirSync(path.join(root,dir)).filter(f=>f.endsWith('.cpp'))){
 const p=dir+'/'+f,b=read(p),l=b.toString('utf8').split(/\r?\n/);
 l.forEach((s,i)=>{if(/\bmusic2\b/.test(s))production.push({path:p,line:i+1});});
}
need(production.every(r=>r.path.startsWith('example/src/')),'music2 not consumed in prototype');
need(production.every(r=>/world_(encounters|departure)\.cpp$/.test(r.path)),'music2 only lower candidate production/validation/copy');
const sounds=read('prototype/src/startup_world_runtime.cpp').toString('utf8');
need(sounds.includes('result.swap(state_.sound_requests);'),'once-only output swap');
const n=read('work/decompiled/sources/c/n.java').toString('utf8');
const noticeTexts=JSON.parse(n.match(/String\[\] ay = (\{[^\n]+\});/)[1].replace('{','[').replace('}',']'));
const noticeDelays=JSON.parse(n.match(/int\[\] az = (\{[^\n]+\});/)[1].replace('{','[').replace('}',']'));
need(noticeTexts[24]==='战斗任务开始' && noticeDelays[24]===1,'notice24 is non-script notice with delay1');
const evidence={date:'2026-10-09',qualification:'固定APK原局部静态与维护C++来源；未播放、未构建、未改格式',sources,emitters,
 operation_counts:Object.fromEntries(['b','c','d'].map(op=>[op,emitters.filter(e=>e.operation===op).length])),
 missing_music2:{current_references:production,original:'c/b.java:365 b(2)',candidate_routes:['daily.task_entry','daily.path'],
  qualification:'两路候选已产出，Owner/队列尚无消费；不在胜利/取消恢复处补发'},
 missing_notice24:{text:noticeTexts[24],initial_counter:-noticeDelays[24],duration:80,script_event:false,
  order:'successful creation and quota -> B2 -> append notice24; later eligible notice counter1 -> C11',
  duplicate_boundary:'state0 path vs state5/11 direct mutually exclusive per daily call; later actors bind installed task.encounter'},
 order_notes:{rank50:['restore b(1/2)','close'],award87_user_termination:['event22','restore b(1/2)','close'],
 award87_medals_exhausted:['restore b(1/2)','event22','close'],clear_end:['close','restore b(1/2)','event6','event4_or_5']},
 known_missing_source_groups:['title b(0)','world init g()->b(1/2)','new task encounter b(2)','raw76 render c(8)','magic-pot render c(8)','platform/UI lifecycle operations'],
 consumers:['README.md','../../ui/AUDIO_REQUESTS.md'],code_changes:0,new_audio_files:0,played:0,builds:0};
for(const p of evidence.consumers)need(fs.existsSync(path.resolve(__dirname,p)),'consumer '+p);
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(evidence,null,2)+'\n');
console.log(JSON.stringify({checks,source_windows:sources.length,queue_emitters:emitters.length,operations:evidence.operation_counts,
 bytes:fs.statSync(path.join(__dirname,'EVIDENCE.json')).size,music2_references:production.length}));
