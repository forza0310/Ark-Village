const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 原皮肤局部来源审计：不运行原游戏、不解码Steam机器码、不保存原实现摘录。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const root=path.resolve(archivePaths.workDir,'../..'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
let checks=0;const need=(v,m)=>{++checks;if(!v)throw Error(m);};
const windows=[
 ['原标题资源与菜单','b/h.java',96,204],['原标题门槛','b/h.java',42,43],
 ['原标题背景人物随机更新','b/h.java',404,466],['Logo短整数插值','c/d.java',115,132],
 ['Logo短弹跳参数','c/d.java',151,156],
 ['纪录两页','b/e.java',32,104],['91四项原绘制','b/g.java',8709,8778],
 ['17分阶段原绘制','b/g.java',9506,9590],['通用颜色','b/g.java',120,128],
 ['开始按钮','b/g.java',437,468],['特殊角色','b/g.java',332,430],
 ['共享标题框','d/a.java',2763,2792],['共享内框','d/a.java',474,496],
 ['字体','kairo/android/ui/n.java',16,116],['页面偏移','b/g.java',9105,9126],
 ['91低层交叉','../world-page-fallback/GamePage.java',29749,29777],
 ['17低层交叉','../world-page-fallback/GamePage.java',34186,34212]
];
const sourceRoot=path.join(root,'work/decompiled/sources');
const sources=windows.map(([summary,file,first,last])=>{
 const p=file.startsWith('../')?path.join(root,'work',file.slice(3)):path.join(sourceRoot,file);
 const b=fs.readFileSync(p),lines=b.toString('utf8').split(/\r?\n/);need(last<=lines.length,'源码窗口越界');
 return {summary,path:path.relative(root,p).replaceAll('\\','/'),first,last,source_sha256:hash(b),window_lf_sha256:hash(Buffer.from(lines.slice(first-1,last).join('\n')))};
});
const index=JSON.parse(fs.readFileSync(path.join(root,'assets/RESOURCE_CODE_INDEX.json'),'utf8'));
need(index.apk_sha256==='1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5','固定APK');
const wanted=['title/title00.png','title/title_window.png','title/title_logo.png','title/title_cursor.png','title/title_grass.png',
 'event/event_backGlad.png','event/event_medelCelemony_back2.png','common/chara_hishoko.png','common/chara_president.png',
 'common/wnd_back.png','common/wnd_bar.png','common/arrow01.png','common/arrow02.seb','common/finger_r.seb',
 'common/chara_hisho.seb','common/chara_president.seb','common/wnd_conner.seb'];
const assets=wanted.map(id=>{const row=index.records.find(r=>r.id===id);need(!!row,'资源绑定缺失');
 const p='assets/original/'+id,b=fs.readFileSync(path.join(root,p));need(hash(b)===row.sha256,'资源哈希变化');
 let dimensions;if(id.endsWith('.png')){need(b.subarray(0,8).equals(Buffer.from([137,80,78,71,13,10,26,10])),'PNG标识');dimensions={width:b.readUInt32BE(16),height:b.readUInt32BE(20)};}
 return {id,path:p,bytes:b.length,sha256:hash(b),...dimensions,slots:row.slots};
});
const shot='references/screenshots/2026-10-07-steam-restore/menu/S038-title.jpg',b=fs.readFileSync(path.join(root,shot));
const evidence={date:'2026-10-08',qualification:'固定APK局部静态/低层交叉；Steam仅S038既有归档与既有小方法合同，没有新机器码探针或原窗口',sources,assets,
 steam_screenshot:{path:shot,bytes:b.length,sha256:hash(b),observation:'根只查看归档：英文logo、开始游戏/纪录、设置/键盘/语言组件；含标题栏，DPI及客户区比例未測'},
 missing:['Steam91/17/RankForm精确Draw绑定','Steam字体字形/度量与OS输入','标题装饰实际运动与主框架计数资格','原APK/Steam通关连续窗口'],
 consumer:'../../ui/STARTUP_SKIN.md'};
fs.writeFileSync(path.join(archivePaths.workDir,'EVIDENCE.json'),JSON.stringify(evidence,null,2)+'\n');
const files=['../../ui/STARTUP_SKIN.md','../../ui/examples/startup-sources.html'];
for(const f of files)need(fs.existsSync(path.resolve(archivePaths.workDir,f)),'正式消费者缺失');
console.log(JSON.stringify({checks,windows:sources.length,assets:assets.length,steam_new_decoded_bytes:0,bytes:fs.statSync(path.join(archivePaths.workDir,'EVIDENCE.json')).size}));
