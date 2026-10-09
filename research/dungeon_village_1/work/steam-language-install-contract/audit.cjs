// 只收窄方法身份、固定槽、字段和资源摘要，不保存原始代码全文。
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(__dirname,'../..'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const run=(p,args=[])=>cp.execFileSync(process.execPath,[path.join(__dirname,p),...args],{encoding:'utf8',maxBuffer:1024*1024});
const windows=run('inspect.cjs',['manifest']).trim().split('\n').map(JSON.parse);
const unique=[...new Set(windows.map(w=>w.rva))].map(r=>windows.filter(w=>w.rva===r).sort((a,b)=>b.bytes-a.bytes)[0]);
const xrefs=JSON.parse(run('xrefs.cjs')),literals=JSON.parse(run('literals.cjs')),bindings=JSON.parse(run('generic-bindings.cjs'));
if(bindings.entries.map(e=>e.method).join(',')!=='.ctor,Add,ToArray')throw Error('folder list bindings');
const resourceText=cp.execFileSync('python',[path.join(__dirname,'resources.py')],{encoding:'utf8',env:{...process.env,PYTHONIOENCODING:'utf-8'},maxBuffer:1024*1024}),resources=JSON.parse(resourceText);
const language=resources.objects.find(o=>o.name==='language');if(!language||language.entries.length!==12)throw Error('package count');
const required={'DungeonVillage_SC.csv':['@language,zh-CN','@scale,auto'],'DungeonVillage_TC.csv':['@language,zh','@scale,auto']};
for(const [n,headers] of Object.entries(required)){const e=language.entries.find(e=>e.name===n);if(!e||headers.some(h=>!e.headers.includes(h)))throw Error('Chinese source headers');}
const dumpPath=path.join(root,'work/persistence-replay-analysis/exe/dumper/dump.cs'),dump=fs.readFileSync(dumpPath),text=dump.toString('utf8');
const a=text.indexOf('public class Language // TypeDefIndex: 600'),z=text.indexOf('// Methods',a),fields=text.slice(a,z).split(/\r?\n/).filter(s=>/language_;|priorityLanguage_;|translateTableMethod_;|langPack(Code|Title|Author|Scale|Folders|File)_;|PREF_LANG_PACK;|initialized_;/.test(s)).map(s=>s.trim());
if(fields.length!==11||!fields.includes('private static ThreadStart translateTableMethod_; // 0x3C'))throw Error('field identity');
const scripts=['inspect.cjs','literals.cjs','generic-bindings.cjs','xrefs.cjs','resources.py','audit.cjs'];
const evidence={scope:'Steam language installation/selection bounded static study, no live game, user preferences or saves',windows,unique_methods:unique.length,unique_prefix_bytes:unique.reduce((s,w)=>s+w.bytes,0),method_index:{path:'../persistence-replay-analysis/exe/methods.json',sha256:hash(fs.readFileSync(path.join(root,'work/persistence-replay-analysis/exe/methods.json')))},field_source:{path:'../persistence-replay-analysis/exe/dumper/dump.cs',sha256:hash(dump),fields},direct_register_xrefs:xrefs,generic_bindings:bindings,limits:['No direct E8/E9 candidates does not exclude inlining, reflection or indirect delegate registration','Init configuration and locale branches are not reproduced as a complete controller','Pack parsing text replacement and automatic font-scale sampling remain partial','No live selected pack or actual font object certified'],scripts:scripts.map(p=>{const b=fs.readFileSync(path.join(__dirname,p));return {path:p,bytes:b.length,sha256:hash(b)};})};
for(const [p,j] of [['EVIDENCE.json',evidence],['LITERALS.json',literals],['RESOURCES.json',resources]])fs.writeFileSync(path.join(__dirname,p),JSON.stringify(j,null,2)+'\n');
console.log(JSON.stringify({unique_methods:evidence.unique_methods,prefix_bytes:evidence.unique_prefix_bytes,windows:windows.length,string_slots:literals.strings.length,language_packages:language.entries.length,register_direct_candidates:xrefs.candidates.length}));
