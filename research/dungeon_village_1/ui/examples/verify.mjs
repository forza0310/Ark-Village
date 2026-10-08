// 只验证本例来源/文件引用/预览白名单；不把静态结果当浏览器视觉或真实游戏输入认证。
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url)),root=path.resolve(here,'../..');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const assets=['common/wnd_back.png','common/wnd_bar.png','common/icon_tenantInfo.png','common/finger_r.png','image/t_cafe.png'];
const references=['facilities/S043-details-before.jpg','facilities/S045-reinforcement-result.jpg','menu/S049-system-menu.jpg','menu/S041-save-complete.jpg','items/S044-facility-item-list-before.jpg'];
const imageManifest=fs.readFileSync(path.join(root,'assets/MANIFEST.tsv'),'utf8');
const referenceBase='references/screenshots/2026-10-07-steam-restore/';
const referenceManifest=fs.readFileSync(path.join(root,referenceBase,'MANIFEST.tsv'),'utf8');
const sources=[];
for(const [prefix,entries,manifest,version]of [['assets/original/',assets,imageManifest,'APK1.0.8'],[referenceBase,references,referenceManifest,'Steam2.56']]) {
 for(const entry of entries){const p=prefix+entry,b=fs.readFileSync(path.join(root,p)),hash=sha(b);
 assert(manifest.includes(hash),'冻结来源清单未记录 '+p);
 const geometry=prefix==='assets/original/'?{width:b.readUInt32BE(16),height:b.readUInt32BE(20),
     crop:entry==='common/icon_tenantInfo.png'?[32,0,16,16]:null,
     transform:entry==='common/wnd_back.png'?'CSS二倍平铺':entry==='common/wnd_bar.png'?'CSS标题带适配':entry==='common/icon_tenantInfo.png'?'类别2裁片二倍':entry==='common/finger_r.png'?'二倍':'整图二倍'}:{};
 sources.push({path:p,sha256:hash,bytes:b.length,version,...geometry,usage:prefix==='assets/original/'?'示例纹理/图块；非Steam调用绑定认证':'完整原图对照；不是产品运行素材'});}
}
const result={schema:1,qualification:'UI参考草稿；APK素材与Steam截图分开，人物仅APK合同示意',sources};
const text=JSON.stringify(result,null,2)+'\n',out=path.join(here,'SOURCES.json');
assert(process.argv.slice(2).every(x=>['--write','--http'].includes(x)));
if(process.argv.includes('--write'))fs.writeFileSync(out,text);else assert.equal(fs.readFileSync(out,'utf8'),text);
for(const name of ['index.html','examples.css','examples.js','preview.mjs','README.md'])assert(!fs.readFileSync(path.join(here,name),'utf8').includes('\uFFFD'),name);
const css=fs.readFileSync(path.join(here,'examples.css'),'utf8');
for(const [,url]of css.matchAll(/url\(['"]?([^'")]+)['"]?\)/g))assert(fs.existsSync(path.resolve(here,url)),url);
const html=fs.readFileSync(path.join(here,'index.html'),'utf8');
for(const [,url]of html.matchAll(/(?:href|src)="([^"]+)"/g))assert(fs.existsSync(path.resolve(here,url)),url);
let httpChecks=0;
if(process.argv.includes('--http')) {
 for(const p of ['ui/examples/index.html','ui/examples/examples.css','ui/examples/examples.js',...sources.map(s=>s.path)]) {
  const res=await fetch('http://127.0.0.1:8766/'+p);assert.equal(res.status,200,p);
  assert.equal(sha(Buffer.from(await res.arrayBuffer())),sha(fs.readFileSync(path.join(root,p))),p);httpChecks++;
 }
 for(const p of ['maoxianmigongcun.apk','work/decompiled/sources/b/g.java','rules/PERSISTENCE.md','ui/examples/preview.mjs','assets/original/']) {
  assert.equal((await fetch('http://127.0.0.1:8766/'+p)).status,404,p);httpChecks++;
 }
}
console.log(JSON.stringify({source_files:sources.length,manifest_bytes:Buffer.byteLength(text),manifest_sha256:sha(Buffer.from(text)),http_checks:httpChecks,browser_visual_verified:false,game_logic_executed:false}));
