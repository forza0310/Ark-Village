// 有界Steam字体后端证据汇总；不执行原DLL，不导出字体或原图。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(__dirname,'../..'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const run=(p,args=[])=>cp.execFileSync(process.execPath,[path.join(__dirname,p),...args],{encoding:'utf8',maxBuffer:1024*1024});
const windows=run('inspect.cjs',['manifest']).trim().split('\n').map(JSON.parse),unique=[...new Set(windows.map(w=>w.rva))].map(r=>windows.filter(w=>w.rva===r).sort((a,b)=>b.bytes-a.bytes)[0]);
const data=JSON.parse(run('data.cjs'));
const evidence={scope:'Steam text backend state and default skin/font static references; no live font certification',windows,unique_registered_methods:unique.length-1,direct_cleanup_helpers:1,unique_bytes:unique.reduce((s,w)=>s+w.bytes,0),method_index_sha256:hash(fs.readFileSync(path.join(root,'work/persistence-replay-analysis/exe/methods.json'))),limits:['DefaultUseFontTexture setter ends after 80 bytes; indexed next method gap contains unrelated helpers','OnGUI cleanup is identified by a direct call from a named method, not invented metadata','GUIStyle null font and Arial resource object do not prove active CJK glyph face','No complete matrix/clip/TextLayout/font rasterizer or every branch certification'],scripts:['inspect.cjs','data.cjs','audit.cjs'].map(p=>{const b=fs.readFileSync(path.join(__dirname,p));return {path:p,bytes:b.length,sha256:hash(b)};})};
for(const [p,j] of [['EVIDENCE.json',evidence],['DATA.json',data]])fs.writeFileSync(path.join(__dirname,p),JSON.stringify(j,null,2)+'\n');
console.log(JSON.stringify({registered_methods:evidence.unique_registered_methods,cleanup_helpers:1,windows:windows.length,bytes:evidence.unique_bytes,builtin_objects:data.objects.length}));
