// 自然通关前缀只读盘点：仅容器、摘要及Driver身份，不解码Owner／不改旧档。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),assert=require('assert/strict');
const root=path.resolve(__dirname,'../..'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(path.join(root,p));
const schema=read('prototype/src/startup_world_codec_fields.inc').toString('utf8').match(/schema_identity\[\] = "([0-9a-f]{64})"/)[1];
const files=['work/snapshots/progression38000.awr','work/snapshots/progression-prefixes/prefix-15000.awr',
 'work/expansion-replay-assessment/expansion420.awr','work/expansion-replay-assessment/periodic-candidates/prefix-420.awr',
 'work/expansion-replay-assessment/periodic-candidates/prefix-840.awr'];
function reader(b){return {at:0,raw(n){assert(Number.isSafeInteger(n)&&n>=0&&n<=b.length-this.at);const r=b.subarray(this.at,this.at+n);this.at+=n;return r;},u32(){return this.raw(4).readUInt32LE();},u64(){const n=this.raw(8).readBigUInt64LE();assert(n<=BigInt(Number.MAX_SAFE_INTEGER));return Number(n);},text(){const n=this.u32();assert(n<=4096);return this.raw(n).toString('utf8');},end(){assert.equal(this.at,b.length);}};}
const inventory=files.map(file=>{
 const bytes=read(file);assert(bytes.length<=128*1024*1024);assert.equal(bytes.subarray(-64).toString('ascii'),hash(bytes.subarray(0,-64)));
 const r=reader(bytes.subarray(0,-64));assert.equal(r.raw(8).toString('ascii'),'AVRSAVE1');
 const version=r.u32(),semantics=r.u32(),purpose=r.u32();assert.equal(version,1);assert.equal(semantics,1);assert.equal(purpose,2);
 const dataset=r.text(),fileSchema=r.text(),count=r.u32();assert(count>=4&&count<=64);const sections=new Map();
 for(let i=0;i<count;i++){const id=r.u32(),version=r.u32(),required=r.u32(),length=r.u64(),digest=r.text(),body=r.raw(length);assert(!sections.has(id)&&version>0&&required<=1);assert.equal(hash(body),digest);sections.set(id,{id,version,required,bytes:length,body});}r.end();
 const meta=reader(sections.get(1).body),producer_revision=meta.text(),controller=meta.text(),next_frame=meta.u64();meta.end();
 assert.equal(controller,'natural-progression-expansion-v1');
 const drv=reader(sections.get(4).body);assert.equal(drv.u64(),0x315652445741);const seed=drv.u64(),speed=drv.u64(),expansion=drv.u64(),driver_next=drv.u64(),observed=drv.u64(),checks=drv.u64(),terminal=drv.u64();
 assert.equal(driver_next,next_frame);assert.equal(observed,next_frame-1);assert(checks>=next_frame&&expansion<=1&&terminal<=1);
 const hist=reader(sections.get(3).body),history_count=hist.u32();assert(history_count<=4096);let history_owner_bytes=0,max_history_owner_bytes=0;
 for(let i=0;i<history_count;i++){const size=hist.u64();hist.raw(size);history_owner_bytes+=size;max_history_owner_bytes=Math.max(max_history_owner_bytes,size);}hist.end();
 const sidecar=file+'.json',has_certificate=fs.existsSync(path.join(root,sidecar));
 return {file,bytes:bytes.length,sha256:hash(bytes),version,semantics,purpose,dataset,schema:fileSchema,current_schema_matches:fileSchema===schema,
  compatibility:fileSchema===schema?'仅布局匹配，完整恢复资格尚需权威加载器':'当前加载器明确拒绝；不迁移／不补字段／不改头',
  producer_revision,controller,next_frame,seed,speed,expansion:!!expansion,terminal:!!terminal,
  owner_bytes:sections.get(2).bytes,history_count,history_section_bytes:sections.get(3).bytes,history_owner_bytes,max_history_owner_bytes,
  source_level:has_certificate?'有旁证书，证书所列尾段资格另核':'周期裸候选，不能称已认证',
  certificate:has_certificate?{file:sidecar,bytes:read(sidecar).length,sha256:hash(read(sidecar))}:null};
});
const sources=['prototype/tests/startup_world_continuous_test.cpp','prototype/tests/startup_world_replay_driver.cpp','prototype/tests/replay_file_test.mjs',
 'prototype/src/startup_world_persistence.cpp','prototype/src/startup_world_codec.hpp','prototype/src/startup_world_codec_fields.inc',
 'prototype/src/startup_application.cpp','example/src/world_calendar_tasks.cpp','work/validation/windows-optimized-expansion.log'];
const out={read_only:true,current_schema:schema,inventory,sources:sources.map(file=>({file,bytes:read(file).length,sha256:hash(read(file))})),
 boundaries:['不加载／迁移旧Owner，只检验容器及原序摘要','旧证书不代表当前schema自然前缀','未运行游戏／长测／构建／读取实时原档'],
 route:{opening_raw_date:[0,3],clear_raw_date:[15,3],natural_month_transitions:180,qualified_calendar_units_per_month:43200,
  standard_units_per_eligible_world_iteration:27,minimum_eligible_iterations:288000,
  lower_bound_scope:'仅日历合格迭代下界；模态等待／场景及首帧会增加实际外层帧，非耗时承诺'}};
fs.writeFileSync(path.join(__dirname,'INVENTORY.json'),JSON.stringify(out,null,2)+'\n');
console.log(JSON.stringify({files:inventory.length,total_bytes:inventory.reduce((n,r)=>n+r.bytes,0),current_schema_matches:inventory.filter(r=>r.current_schema_matches).length}));
