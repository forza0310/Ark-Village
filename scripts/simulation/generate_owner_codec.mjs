// 从Clang公开字段AST生成维护格式访问表；未知类型及私有状态必须显式处理。
import { readFileSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { spawnSync } from 'node:child_process';
import path from 'node:path';
const args = process.argv.slice(2);
const get = key => args[args.indexOf(key)+1];
for(const key of ['--root','--ast'])if(!args.includes(key)||!get(key)||get(key).startsWith('--'))throw Error('missing '+key);
const compare = (a,b) => a<b?-1:a>b?1:0;
const root = path.resolve(get('--root'));
const astPath = get('--ast');
if (args.includes('--compiler')) {
  const result = spawnSync(get('--compiler'), ['-std=c++17', '-I'+path.join(root,'include'),
    '-x','c++','-fsyntax-only','-Xclang','-ast-dump=json',
    '-Xclang','-ast-dump-filter=ark','-'], {
      input:'#include "ark/simulation/startup_world_runtime.hpp"\n',maxBuffer:256*1024*1024});
  if(result.status!==0) throw Error(result.stderr.toString());
  writeFileSync(astPath,result.stdout);
}
let buffer=readFileSync(astPath);
let source=buffer[0]===255&&buffer[1]===254?buffer.toString('utf16le'):buffer.toString('utf8');
source=source.replace(/^\uFEFF/,'');
const roots=[];
let depth=0, quoted=false, escaped=false, begin=0;
for(let i=0;i<source.length;i++) {
  const c=source[i];
  if(quoted) {if(escaped)escaped=false;else if(c==='\\')escaped=true;else if(c==='"')quoted=false;continue;}
  if(c==='"'){quoted=true;continue;}
  if(c==='{'){if(depth===0)begin=i;depth++;}
  if(c==='}'&&--depth===0)roots.push(JSON.parse(source.slice(begin,i+1)));
}
// Compute the published canonical manifest from the product AST, then translate only
// generated C++ names. A namespace rename must not manufacture a different field identity.
const canonicalName = value => value.replace(/\bark::simulation::rules\b/g, 'dungeon_village_reference')
  .replace(/\bark::simulation\b/g, 'dungeon_village_prototype');
const productName = value => value.replace(/\bdungeon_village_reference\b/g, 'ark::simulation::rules')
  .replace(/\bdungeon_village_prototype\b/g, 'ark::simulation');
const records=new Map(), enums=new Map(), aliases=new Map();
function walk(node, scopes=[]) {
  const named=['NamespaceDecl','CXXRecordDecl','EnumDecl'].includes(node.kind)&&node.name&&!node.isImplicit;
  const fq=canonicalName([...scopes,node.name].join('::'));
  if(node.kind==='CXXRecordDecl'&&node.completeDefinition&&!node.isImplicit) {
    const fields=(node.inner??[]).filter(x=>x.kind==='FieldDecl').map(x=>({name:x.name,type:canonicalName(x.type.qualType),resolved:canonicalName(x.type.desugaredQualType??x.type.qualType)}));
    records.set(fq,{name:fq,fields,bases:node.bases??[],tag:node.tagUsed});
  }
  if(node.kind==='EnumDecl'&&(node.inner??[]).some(x=>x.kind==='EnumConstantDecl')) {
    let next=0n;
    function constant(x){if(x.kind==='ConstantExpr'&&x.value!==undefined)return x.value;for(const c of x.inner??[]){const v=constant(c);if(v!==undefined)return v;}}
    const values=node.inner.filter(x=>x.kind==='EnumConstantDecl').map(x=>{const explicit=constant(x);const value=explicit===undefined?next:BigInt(explicit);next=value+1n;return {name:x.name,value:String(value)};});
    enums.set(fq,{name:fq,underlying:canonicalName(node.fixedUnderlyingType?.qualType??'implicit'),values});
  }
  if(['TypeAliasDecl','TypedefDecl'].includes(node.kind))aliases.set(fq,{type:canonicalName(node.type.qualType),resolved:canonicalName(node.type.desugaredQualType??node.type.qualType)});
  for(const child of node.inner??[])walk(child,named?[...scopes,node.name]:scopes);
}
roots.forEach(x=>walk(x));
const wrappers=new Map([
 ['dungeon_village_reference::WorldRandomStream','dungeon_village_reference::WorldRandomSnapshot'],
 ['dungeon_village_reference::PeriodAccounting','dungeon_village_reference::PeriodAccountingSnapshot']]);
const selected=new Map(),selectedEnums=new Map(),selectedAliases=new Map();
function visitType(type,scope) {
  type=type.replace(/\bref::/g,'dungeon_village_reference::');
  for(const token of type.match(/[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*/g)??[]) {
    let candidates=[token];
    const parts=scope.split('::');
    while(parts.length){candidates.push([...parts,token].join('::'));parts.pop();}
    const name=candidates.find(x=>records.has(x)||enums.has(x)||aliases.has(x));
    if(!name)continue;
    if(wrappers.has(name)){visitType(wrappers.get(name),'');continue;}
    if(enums.has(name)){selectedEnums.set(name,enums.get(name));continue;}
    if(aliases.has(name)){
      if(selectedAliases.has(name))continue;
      selectedAliases.set(name,aliases.get(name).type);
      visitType(aliases.get(name).type,name.split('::').slice(0,-1).join('::'));
      visitType(aliases.get(name).resolved,name.split('::').slice(0,-1).join('::'));continue;
    }
    if(selected.has(name))continue;
    const record=records.get(name);
    if(record.bases.length)throw Error('unsupported inherited record: '+name);
    if(record.tag==='union')throw Error('unsupported union state: '+name);
    selected.set(name,record);
    for(const field of record.fields) {
      if(name==='dungeon_village_prototype::StartupWorldRuntimeState'&&field.name==='rules')continue;
      if(/[&*]/.test(field.type))throw Error('unsupported reference/pointer '+name+'.'+field.name);
      visitType(field.type,name);
      visitType(field.resolved,name);
    }
  }
}
visitType('dungeon_village_prototype::StartupWorldRuntimeState','');
// 清单保存声明类型；std::size_t/uint64_t不能因编译目标的typedef展开而改变格式身份。
const fieldsFor = record => record.fields.map(({name,type})=>({name,type}));
const manifest={wire:'owner-le-v1: integer64,bool8,floatIEEE,container-count64; rules=rebind',
 wrappers:[...wrappers].map(([name,snapshot])=>({name,snapshot,fields:fieldsFor(records.get(name)),aliases:[...aliases].filter(([key])=>key.startsWith(name+'::')).map(([key,value])=>({name:key,type:value.type}))})),
 records:[...selected.values()].map(r=>({name:r.name,fields:fieldsFor(r)})).sort((a,b)=>compare(a.name,b.name)),
 aliases:[...selectedAliases].sort(([a],[b])=>compare(a,b)).map(([name,type])=>({name,type})),
 enums:[...selectedEnums.values()].sort((a,b)=>compare(a.name,b.name))};
const canonical=JSON.stringify(manifest,null,2)+'\n';
const identity=createHash('sha256').update(canonical).digest('hex');
let code='// 由 scripts/generate_owner_codec.mjs 生成；不要手改。\n';
code+='constexpr const char schema_identity[] = "'+identity+'";\n';
for(const e of manifest.enums)code+='template<> bool valid_enum< ::'+e.name+'>(::'+e.name+' value) { return '+e.values.map(v=>'value == ::'+e.name+'::'+v.name).join(' || ')+'; }\n';
for(const r of manifest.records){
 code+='template<> struct CodecFields< ::'+r.name+'> {\n template<class Archive, class Value> static void visit(Archive& a, Value& v) {\n';
 for(const f of r.fields){
  if(r.name==='dungeon_village_prototype::StartupWorldRuntimeState'&&f.name==='rules')code+='  // rules静态目录由decode参数重绑定，不编码地址。\n';
  else code+='  a.field("'+r.name+'.'+f.name+'", v.'+f.name+');\n';
 }
 code+=' }\n static std::size_t minimum() { return 0';
 for(const f of r.fields)if(!(r.name==='dungeon_village_prototype::StartupWorldRuntimeState'&&f.name==='rules'))
  code+=' + Codec<decltype(std::declval< ::'+r.name+'>().'+f.name+')>::minimum()';
 code+='; }\n};\n';
}
for(const [file,text] of [['startup_world_codec_fields.inc',code],['startup_world_codec_fields.json',canonical]]) {
 const target=path.join(root,'src/simulation',file);
 const generated = file.endsWith('.inc') ? productName(text) : text;
 if(args.includes('--check')) {if(readFileSync(target,'utf8')!==generated)throw Error('codec field coverage stale: '+target);}
 else writeFileSync(target,generated);
}
console.log('Owner codec: '+selected.size+' records, '+selectedEnums.size+' enums, SHA256 '+identity);
