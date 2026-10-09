// 只过滤应用类AST；直接私有成员必须逐项分类，不生成/保留大型world AST。
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {spawnSync} from 'node:child_process';
const args=process.argv.slice(2);
const option=k=>args[args.indexOf(k)+1];
for(const k of ['--root','--compiler'])
  if(!args.includes(k)||!option(k)||option(k).startsWith('--'))throw Error('缺参数'+k);
const root=path.resolve(option('--root'));
const result=spawnSync(option('--compiler'),['-std=c++17','-I'+path.join(root,'prototype/include'),
  '-I'+path.join(root,'example/include'),'-x','c++','-fsyntax-only','-Xclang','-ast-dump=json',
  '-Xclang','-ast-dump-filter=dungeon_village_prototype::StartupApplication','-'],{
  input:'#include "dungeon_village_prototype/startup_application.hpp"\n',maxBuffer:8*1024*1024});
if(result.status!==0)throw Error(result.stderr.toString());
const source=result.stdout.toString('utf8'), trees=[];
let depth=0,quoted=false,escaped=false,start=0;
for(let i=0;i<source.length;i++){
  const c=source[i];
  if(quoted){if(escaped)escaped=false;else if(c==='\\')escaped=true;else if(c==='"')quoted=false;continue;}
  if(c==='"'){quoted=true;continue;}
  if(c==='{'){if(depth===0)start=i;depth++;}
  if(c==='}'&&--depth===0)trees.push(JSON.parse(source.slice(start,i+1)));
}
const type=trees.find(t=>t.kind==='CXXRecordDecl'&&t.name==='StartupApplication'&&t.completeDefinition);
if(!type||type.bases?.length)throw Error('应用类型缺失或新增未分类基类');
const policies={
  paths_:'rebind: 显式research/work隔离路径；不读取快照旧路径',
  records_:'save: 完整AVRSYS01字节及未知可选段',
  draft_:'save: village/main_character{name,sex,custom_name}/slot',
  random_:'save: WorldRandomSnapshot完整引擎/磁带/游标/模式',
  mode_:'save: logic或title_presentation',
  page_:'save: 应用页面枚举', record_page_:'save: 纪录页0或1',
  decorations_:'save: 原序定义ID名单', requests_:'save: 显式纪录请求序号',
  handoff_:'save: 可选历史交接完整随机快照',
  world_:'save: 可选AVRSAVE1完整Session与原序审计历史',
  clear_rows_:'save: 可选六行count/score',
  clear_:'save: 可选stage/counter/row/sum/captured_high_score/finished/new_record/trophy',
  clear_id_:'save: 可选稳定raw17页ID',
  error_:'require-empty: 仅健康应用可捕获；恢复不重演旧错误'
};
const fields=(type.inner??[]).filter(x=>x.kind==='FieldDecl').map(x=>{
  if(!policies[x.name])throw Error('应用新增未分类成员：'+x.name);
  return {name:x.name,type:x.type.qualType,policy:policies[x.name]};
});
if(fields.length!==Object.keys(policies).length)throw Error('应用字段移除/重命名，需重审分类');
const manifest={format:'AVRAPP01',semantics:1,boundary:'complete-outer-round-v1',fields};
const canonical=JSON.stringify(manifest,null,2)+'\n';
const schema=crypto.createHash('sha256').update(canonical).digest('hex');
const inc='// 应用字段分类生成身份；不覆盖独立world schema。\nconstexpr const char application_schema[] = "'+schema+'";\n';
for(const [name,content] of [['startup_application_replay_fields.json',canonical],['startup_application_replay_fields.inc',inc]]){
  const output=path.join(root,'prototype/src',name);
  if(args.includes('--check')){if(fs.readFileSync(output,'utf8')!==content)throw Error('应用字段清单不匹配：'+name);}
  else fs.writeFileSync(output,content);
}
console.log(JSON.stringify({fields:fields.length,schema,ast_bytes:result.stdout.length,qualification:'直接应用成员分类；嵌套字段由具名codec与world/system身份约束'}));
