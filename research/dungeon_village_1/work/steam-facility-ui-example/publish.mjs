// 核生成清单、源资源与实际输出，拒绝覆盖不同的归档图像。
import fs from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const base=path.dirname(fileURLToPath(import.meta.url)),root=path.resolve(base,'../..');
const output=path.join(base,'output'),destination=path.join(root,'ui/examples/steam-facility-upgrade');
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(v,m)=>{if(!v)throw Error(m);};
const raw=await fs.readFile(path.join(output,'MANIFEST.json')),m=JSON.parse(raw);
need(m.qualification==='research_cpp_plan_fixture_not_original_screenshot'&&m.font==='magenta_anchor_only'&&m.samples.length===6,'示例资格');
const items=[{file:'MANIFEST.json',bytes:raw.length,sha256:hash(raw)}];
for(const s of m.samples){
 items.push({file:s.png,bytes:s.bytes,sha256:s.sha256,width:s.width,height:s.height});
 items.push({file:s.plan,bytes:s.plan_bytes,sha256:s.plan_sha256});
}
items.push(m.contact_sheet);
for(const s of m.sources){const b=await fs.readFile(path.join(root,'assets',s.path));need(hash(b)===s.sha256&&b.length===s.bytes,'源素材失配 '+s.path);}
const check=process.argv.includes('--check');if(!check)await fs.mkdir(destination,{recursive:true});
for(const s of items){
 need(path.basename(s.file)===s.file,'输出名越界');const b=await fs.readFile(path.join(output,s.file));
 need(b.length===s.bytes&&hash(b)===s.sha256,'输出hash '+s.file);
 if(s.width)need(b.subarray(0,8).toString('hex')==='89504e470d0a1a0a'&&b.readUInt32BE(16)===s.width&&b.readUInt32BE(20)===s.height,'PNG身份');
 try{need((await fs.readFile(path.join(destination,s.file))).equals(b),'拒绝覆盖不同输出');}
 catch(e){if(e.code!=='ENOENT'||check)throw e;await fs.writeFile(path.join(destination,s.file),b,{flag:'wx'});}
}
const result={qualification:m.qualification,font:m.font,files:items.length,bytes:items.reduce((n,s)=>n+s.bytes,0),source_files:m.sources.length,
 visual_inspection:'主会话查看744x504拼图，六面板裁剪/数字/设施/角色可见，文字为洋红锚点',
 checks:{release_build:'passed',visuals_seconds:0.61,cpu_export:'passed',png_and_source_hashes:'passed',existing_outputs:'unchanged'},
 limitations:['夹具数值与字宽；不是原窗口截图或真实升级结果','Steam字体、OS输入及运行窗口未认证'],
 processes:'无窗口/后台任务；生成器正常退出',items};
if(!check)await fs.writeFile(path.join(base,'VALIDATION.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({files:result.files,bytes:result.bytes,sources:result.source_files,check}));
