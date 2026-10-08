// 独立字段/资源审核；不执行原游戏、构建或读取用户账号/档案。
const fs=require('fs'), path=require('path'), crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
let checks=0;
const check=(yes,why)=>{++checks;if(!yes)throw Error(why);};
const manifestBytes=fs.readFileSync(path.join(root,'prototype/src/startup_world_codec_fields.json'));
const manifest=JSON.parse(manifestBytes), identity=hash(manifestBytes);
check(identity==='a1ca189f3b9290b18390812af91f601eaac9f295154ab1de3601590d0fbda2d7','批准的profile布局身份');
const owner=manifest.records.find(r=>r.name==='dungeon_village_prototype::StartupWorldRuntimeState');
const value=manifest.records.find(r=>r.name==='dungeon_village_prototype::StartupWorldHumanProfile');
check(!!owner&&!!value,'profile所属Owner字段闭包');
check(owner.fields.some(f=>f.name==='human_profiles'&&f.type==='std::map<int, StartupWorldHumanProfile>'),'稳定ID覆盖完整进入codec');
check(value.fields.map(f=>f.name+':'+f.type).join(',')==='name:std::string,sex:int,custom_name:bool','值类型字段完整覆盖');
const consumers=['startup_world_human.cpp','startup_world_visuals.cpp','startup_world_runtime.cpp',
  'startup_world_runtime_arrival.cpp','startup_world_building.cpp','startup_world_projection.cpp',
  'startup_world_restore_validation.cpp','startup_view.cpp'];
for(const file of consumers) {
  const source=fs.readFileSync(path.join(root,'prototype/src',file),'utf8');
  check(source.includes('startup_world_human_profile('),file+'统一profile消费者');
}
const files=['prototype/include/dungeon_village_prototype/startup_world_profile.hpp',
 'prototype/include/dungeon_village_prototype/startup_world_inheritance.hpp',
 'prototype/include/dungeon_village_prototype/startup_world_runtime.hpp',
 'prototype/include/dungeon_village_prototype/startup_world_human.hpp',
 'prototype/src/startup_world_profile.cpp','prototype/src/startup_world_inheritance.cpp',
 ...consumers.map(f=>'prototype/src/'+f),
 'prototype/src/startup_world_codec_fields.json','prototype/src/startup_world_codec_fields.inc',
 'prototype/tests/startup_world_runtime_test.cpp','prototype/tests/startup_world_building_test.cpp',
 'prototype/tests/startup_world_persistence_test.cpp','prototype/tests/startup_world_restore_checks.cpp',
 'prototype/STARTUP_PROFILE.md'];
const resources=files.map(file=>{const b=fs.readFileSync(path.join(root,file));return {file,bytes:b.length,sha256:hash(b)};});
const ast=path.join(__dirname,'owner-codec-ast.json');
let retiredAst;
if(fs.existsSync(ast)) {const b=fs.readFileSync(ast); retiredAst={bytes:b.length,sha256:hash(b),disposition:'生成字段表后单文件退休'};}
else if(fs.existsSync(path.join(__dirname,'CHECK.json')))retiredAst=JSON.parse(fs.readFileSync(path.join(__dirname,'CHECK.json'))).retired_ast;
check(!!retiredAst,'生成输入退休摘要可追溯');
const report={kind:'维护字段与资源审计，不代替功能测试',passed:true,checks,schema:identity,
  records:manifest.records.length,enums:manifest.enums.length,retired_ast:retiredAst,
  resources:{files:resources.length,bytes:resources.reduce((n,f)=>n+f.bytes,0),manifest:resources},
  boundary:{profile_definitions:1,max_name_bytes:4096,inheritance_facility_bytes:170,inheritance_profession_bytes:46},
  functional_validation:'根会话集中验收，不在本子任务重复构建/测试'};
fs.writeFileSync(path.join(__dirname,'CHECK.json'),JSON.stringify(report,null,2)+'\n');
console.log(checks+'项字段/接线检查通过，'+manifest.records.length+'记录/'+manifest.enums.length+'枚举');
