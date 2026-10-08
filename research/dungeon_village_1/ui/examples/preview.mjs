// 只读本机预览：仅允许本例三个页面文件、五张用到的原素材及五张明确截图。
// 不提供目录枚举，不暴露APK、生成代码、存档或任意路径。
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const files=[
 'ui/examples/index.html','ui/examples/examples.css','ui/examples/examples.js',
 'assets/original/common/wnd_back.png','assets/original/common/wnd_bar.png',
 'assets/original/common/icon_tenantInfo.png','assets/original/common/finger_r.png',
 'assets/original/image/t_cafe.png',
 ...['facilities/S043-details-before.jpg','facilities/S045-reinforcement-result.jpg',
 'menu/S049-system-menu.jpg','menu/S041-save-complete.jpg','items/S044-facility-item-list-before.jpg']
 .map(p=>'references/screenshots/2026-10-07-steam-restore/'+p)
];
const allowed=new Set(files),mime={'.html':'text/html; charset=utf-8','.css':'text/css; charset=utf-8',
 '.js':'text/javascript; charset=utf-8','.png':'image/png','.jpg':'image/jpeg'};
const server=http.createServer((req,res)=>{
 let p;try{p=decodeURIComponent(new URL(req.url,'http://127.0.0.1').pathname).slice(1);}catch{res.writeHead(400);res.end();return;}
 if(p==='')p='ui/examples/index.html';
 if(req.method!=='GET'||!allowed.has(p)){res.writeHead(404);res.end();return;}
 fs.readFile(path.join(root,p),(err,b)=>{if(err){res.writeHead(404);res.end();return;}
 res.writeHead(200,{'Content-Type':mime[path.extname(p)],'Cache-Control':'no-store','X-Content-Type-Options':'nosniff'});res.end(b);});
});
server.listen(8766,'127.0.0.1',()=>console.log('UI example: http://127.0.0.1:8766/ui/examples/index.html'));
for(const s of ['SIGINT','SIGTERM'])process.on(s,()=>server.close());
