// 只过滤应用及标题类型 AST；直接成员和新增标题嵌套字段必须逐项分类。
// 不生成/保留大型 world AST。
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {spawnSync} from 'node:child_process';
const args=process.argv.slice(2);
const option=k=>args[args.indexOf(k)+1];
for(const k of ['--root','--compiler'])
  if(!args.includes(k)||!option(k)||option(k).startsWith('--'))throw Error('缺参数'+k);
const root=path.resolve(option('--root'));
let astBytes=0;
function readTrees(filter){
const result=spawnSync(option('--compiler'),['-std=c++17','-I'+path.join(root,'include'),'-x','c++','-fsyntax-only','-Xclang','-ast-dump=json',
  '-Xclang','-ast-dump-filter=ark::simulation::'+filter,'-'],{
  input:'#include "ark/simulation/startup_application.hpp"\n',maxBuffer:8*1024*1024});
if(result.status!==0)throw Error(result.stderr.toString());
astBytes+=result.stdout.length;
const source=result.stdout.toString('utf8'), trees=[];
let depth=0,quoted=false,escaped=false,start=0;
for(let i=0;i<source.length;i++){
  const c=source[i];
  if(quoted){if(escaped)escaped=false;else if(c==='\\')escaped=true;else if(c==='"')quoted=false;continue;}
  if(c==='"'){quoted=true;continue;}
  if(c==='{'){if(depth===0)start=i;depth++;}
  if(c==='}'&&--depth===0)trees.push(JSON.parse(source.slice(start,i+1)));
}
return trees;
}
const trees=readTrees('StartupApplication');
const type=trees.find(t=>t.kind==='CXXRecordDecl'&&t.name==='StartupApplication'&&t.completeDefinition);
if(!type||type.bases?.length)throw Error('应用类型缺失或新增未分类基类');
const policies={
  paths_:'rebind: 显式research/work隔离路径；不读取快照旧路径',
  storage_:'save/rebind: records完整系统2及槽位引用闭包；missing/digest在新根发布后重绑',
  draft_:'save: village/main_character{name,sex,custom_name}/slot',
  random_:'save: WorldRandomSnapshot完整引擎/磁带/游标/模式',
  mode_:'save: logic或title_presentation',
  title_:'save: StartupTitlePresentation完整计数和20槽；logic模式必须为初始态',
  title_menu_:'save: 完整根菜单、raw20、询问、外部子页及未消费返回载荷',
  audio_requests_:'require-empty: 一次性音频请求必须已消费，恢复为空',
  cleanup_pending_:'rebind: 源环境文件清理债务不跨环境恢复，新根置false',
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
// 独立过滤 StartupTitle 防止只看到外层类型名，遗漏槽或计数器新增字段。
const titleTrees=[...readTrees('StartupTitle'),...readTrees('StartupApplicationStorageSnapshot')];
const nestedPolicies={
  StartupApplicationStorageSnapshot:{
    records:'save: 完整系统2、四条目录、引用及未知可选段',
    missing:'rebind: 新根已发布system，恒false；不参与行为摘要',
    digest:'rebind: 发布system原始字节的SHA256；不信任源路径观察'
  },
  StartupTitleCatalogStamp:{revision:'save: uint64目录修订',digest:'save: 32字节系统文件身份'},
  StartupTitleSaveMenu:{id:'save: uint64稳定页ID',parent:'save: uint64父页ID',
    slot:'save: int32栏位',selection:'save: int32当前行',frame:'save: int32帧计数',
    result:'save: int32返回值',returned:'save: bool已返回位',catalog:'save: 可选目录修订与32字节身份'},
  StartupTitleConfirmation:{id:'save: uint64稳定页ID',parent:'save: uint64父页ID',
    reason:'save: 重新开始或隐藏',selection:'save: int32当前行',result:'save: int32返回值',returned:'save: bool已返回位'},
  StartupTitleExternal:{id:'save: uint64稳定页ID',parent:'save: uint64父页ID',
    kind:'save: 纪录或配置',returned:'save: bool已返回位',completed:'save: bool配置完成位'},
  StartupTitleMenuState:{mode:'save: 根菜单或两栏',selection:'save: int32根菜单行',row:'save: int32两栏行',
    root_id:'save: uint64根页ID',next_id:'save: uint64下一页ID',save_menu:'save: 可选raw20完整载荷',
    confirmation:'save: 可选询问完整载荷',external:'save: 可选外部子页完整载荷'},
  StartupTitleSlot:{
    active:'save: int32活动位',definition:'save: int32人物定义ID',
    x:'save: int32横坐标',y:'save: int32纵坐标；inactive仍参与排序',
    direction:'save: int32方向',age:'save: int32步龄；退休和重用仍保留'
  },
  StartupTitlePresentation:{
    l:'save: int32入场计数，饱和递增',f132f:'save: int32标题计数，回卷递增',
    s:'save: int32出生间隔',t:'save: int32间隔内计数',
    slots:'save: 原序20槽StartupTitleSlot，含inactive全部字段'
  }
};
const nested=Object.entries(nestedPolicies).map(([name,policies])=>{
  const type=titleTrees.find(t=>t.kind==='CXXRecordDecl'&&t.name===name&&t.completeDefinition);
  if(!type||type.bases?.length)throw Error('标题嵌套类型缺失或新增未分类基类：'+name);
  const fields=(type.inner??[]).filter(x=>x.kind==='FieldDecl').map(x=>{
    if(!policies[x.name])throw Error('标题嵌套新增未分类成员：'+name+'.'+x.name);
    return {name:x.name,type:x.type.qualType,policy:policies[x.name]};
  });
  if(fields.length!==Object.keys(policies).length)throw Error('标题嵌套字段移除/重命名，需重审分类：'+name);
  return {name,fields};
});
const manifest={format:'AVRAPP01',semantics:7,boundary:'complete-outer-round-v1',fields,nested};
const canonical=JSON.stringify(manifest,null,2)+'\n';
const schema=crypto.createHash('sha256').update(canonical).digest('hex');
const inc='// 应用字段分类生成身份；不覆盖独立world schema。\nconstexpr const char application_schema[] = "'+schema+'";\n';
for(const [name,content] of [['startup_application_replay_fields.json',canonical],['startup_application_replay_fields.inc',inc]]){
  const output=path.join(root,'src/simulation',name);
  if(args.includes('--check')){if(fs.readFileSync(output,'utf8')!==content)throw Error('应用字段清单不匹配：'+name);}
  else fs.writeFileSync(output,content);
}
console.log(JSON.stringify({fields:fields.length,nested_fields:nested.reduce((n,t)=>n+t.fields.length,0),schema,
  ast_bytes:astBytes,qualification:'直接应用成员及标题嵌套字段分类；其余嵌套字段由具名codec与world/system身份约束'}));
