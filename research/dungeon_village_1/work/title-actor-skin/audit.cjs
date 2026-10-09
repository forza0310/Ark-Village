// 固定APK标题人物基础图层审计；仅读冻结源/资源并写本目录派生证据，不执行原游戏。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
let checks=0;const need=(v,m)=>{++checks;if(!v)throw Error(m);};
const windows=[
 ['title-layer-order','b/h.java',129,140],['record-layer-order','b/e.java',56,69],
 ['definition-profession-weapon','a/e.java',717,735],['profession-sex-image','a/h.java',98,104],
 ['weapon-table-binding','a/p.java',70,95],['weapon-walk-offsets','a/p.java',34,45],
 ['scratch-setup','c/n.java',730,736],['scratch-draw-wrapper','c/n.java',1008,1014],
 ['scratch-initialize','c/n.java',1983,1991],['body-action-frame-tables','c/b.java',121,130],
 ['weapon-direct-draw','c/b.java',1079,1116],['weapon-offset-selector','c/b.java',1852,1876],
 ['action-to-weapon-phase','c/b.java',2106,2126],['body-walk-plan','c/b.java',4030,4051],
 ['body-scratch-write','c/b.java',4125,4132],['scratch-reset-fields','c/b.java',4176,4195],
 ['body-and-extra-overlays','c/b.java',4522,4562],['scratch-effect-storage','c/b.java',253,274],
 ['scratch-debug-reuse','b/c.java',1962,1984],['resource-package-load','b/a.java',479,507],
 ['resource-package-names','d/a.java',118,118],['single-image-seb-override','d/a.java',334,349]
];
const sources=windows.map(([purpose,name,first,last])=>{
 const relative='work/decompiled/sources/'+name,b=fs.readFileSync(path.join(root,relative));
 const lines=b.toString('utf8').split(/\r?\n/);need(last<=lines.length,'source window');
 return {purpose,path:relative,first,last,bytes:b.length,source_sha256:sha(b),
  window_lf_sha256:sha(Buffer.from(lines.slice(first-1,last).join('\n')))};
});
const tablesPath='data/startup/TABLES.json',tables=JSON.parse(fs.readFileSync(path.join(root,tablesPath),'utf8'));
need(tables.apk_sha256==='1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5','APK identity');
const tableEvidence=[];
const table=name=>{const t=tables.entries.find(e=>e.entry===name);need(!!t,'table exists');
 const b=Buffer.from(t.source_utf8);need(sha(b)===t.sha256,'table frozen hash');
 tableEvidence.push({path:tablesPath,entry:name,bytes:b.length,sha256:t.sha256});
 return t.source_utf8.split(/\r?\n/).filter(Boolean).map(row=>row.split('\t'));};
const humans=table('character.txt'),jobs=table('job.txt'),weapons=table('weapon.txt');
const index=JSON.parse(fs.readFileSync(path.join(root,'assets/RESOURCE_CODE_INDEX.json'),'utf8'));
const assets=new Map();
function asset(id) {
 if(assets.has(id))return assets.get(id);
 const relative='assets/original/'+id,b=fs.readFileSync(path.join(root,relative));
 const indexed=index.records.find(r=>r.id===id);need(!!indexed&&indexed.sha256===sha(b),'indexed asset hash '+id);
 const row={id,path:relative,bytes:b.length,sha256:sha(b)};
 if(id.endsWith('.png')){need(b.subarray(0,8).equals(Buffer.from([137,80,78,71,13,10,26,10])),'PNG signature');
  row.width=b.readUInt32BE(16);row.height=b.readUInt32BE(20);}
 assets.set(id,row);return row;
}
const imageMaps={},spriteMaps={};
for(const group of ['human','weapon','common']) {
 asset(group+'/img.inf');asset(group+'/seb.inf');
 imageMaps[group]=new Map(fs.readFileSync(path.join(root,'assets/original',group,'img.inf'),'utf8')
  .trim().split(/\r?\n/).map(line=>{const [id,name]=line.split('\t');return [+id,group+'/'+name.replace(/\.gif$/,'.png')];}));
 spriteMaps[group]=fs.readFileSync(path.join(root,'assets/original',group,'seb.inf'),'utf8').trim().split(/\r?\n/);
}
need(!imageMaps.human.has(32),'human32 is absent');
function seb(group,slot,frames) {
 const id=group+'/'+spriteMaps[group][slot],entry=asset(id),b=fs.readFileSync(path.join(root,entry.path));
 let at=0;const number=()=>{need(at+2<=b.length,'SEB truncated');const n=b.readInt16BE(at);at+=2;return n;};
 const layers=number(),frameCount=number();need(layers>0&&layers<=16&&frameCount>0,'SEB header');
 const selected=[];
 for(let layer=0;layer<layers;++layer){const count=number(),tag=number();need(count>=0&&count<=10000,'SEB count');
  for(let p=0;p<count;++p){const v=Array.from({length:10},number);
   if(frames.includes(v[0]))selected.push({layer,frame:v[0],image:v[1],crop:v.slice(2,6),offset:v.slice(6,8),flip:v.slice(8),tag});}}
 need(at===b.length,'SEB trailing');
 for(const frame of frames)need(selected.some(p=>p.frame===frame),'SEB requested frame exists');
 return {group,sprite:slot,id,frame_count:frameCount,layers,parts:selected};
}
const body=Array.from({length:4},(_,f)=>seb('human',f,[0,1,2,3]));
const weapon=Array.from({length:16},(_,f)=>seb('weapon',f,[0]));
const shadow=seb('common',25,[0]);
function bounds(sprite,image) {
 const png=asset(image);
 for(const p of sprite.parts) {
  const [x,y,w,h]=p.crop;need(x>=0&&y>=0&&w>0&&h>0&&x+w<=png.width&&y+h<=png.height,'PNG bounds '+image);
  need(p.flip.every(v=>v===0||v===1),'SEB flip');
 }
}
const bodyImages=[...new Set(jobs.flatMap(r=>[+r[3],+r[4]]))];
for(const id of bodyImages){need(imageMaps.human.has(id),'profession image identity');for(const s of body)bounds(s,imageMaps.human.get(id));}
for(const row of weapons){const image=+row[3],style=+row[6];need(style>=0&&style<=3&&imageMaps.weapon.has(image),'weapon image/style');
 for(let face=0;face<4;++face)bounds(weapon[style*4+face],imageMaps.weapon.get(image));}
bounds(shadow,imageMaps.common.get(3));
const raw=fs.readFileSync(path.join(root,'work/decompiled/sources/a/p.java'),'utf8');
const expression=raw.match(/public static int\[\]\[\]\[\]\[\] x = ([^;]+);/)[1];
const offsets=JSON.parse(expression.replace(/new int(?:\[\])+/g,'').replaceAll('{','[').replaceAll('}',']'));
need(offsets.length===4&&offsets.every(a=>a.length===4&&a.every(b=>b.length===4)),'weapon offset shape');
const initialTitle=humans.slice(0,11).map(row=>{
 const definition=+row[0],sex=+row[2],profession=+row[3],equipment=+row[11].split('&')[0];
 const job=jobs.find(j=>+j[0]===profession),w=weapons.find(w=>+w[0]===equipment);
 need(!!job&&!!w,'title human refs');
 return {definition,sex,profession,body_image:+job[3+sex],weapon_definition:equipment,
  weapon_image:+w[3],weapon_style:+w[6]};});
const evidence={date:'2026-10-09',qualification:'固定APK1.0.8静态与原SEB/PNG/原表，未新增Steam或窗口认证',
 sources,tables:tableEvidence,assets:[...assets.values()],walk_body:body,walk_weapon:weapon,shadow,
 seb_sequence_sha256:sha(Buffer.concat([...body,...weapon,shadow].map(s=>fs.readFileSync(path.join(root,'assets/original',s.id))))),
 weapon_walk_offsets:offsets,initial_title_definitions:initialTitle,
 assertions:{body_images:bodyImages.length,weapon_definitions:weapons.length,source_windows:sources.length,
  shader_or_random_execution:false,original_game_execution:false},
 boundary:['基础图层，不是完整W.draw复刻','W共享scratch存在调试复用，不能保证任意路径无残留叠加','Steam同图不证明同调用'],
 consumers:['../../ui/TITLE_ACTOR_SKIN.md','../../prototype/include/dungeon_village_prototype/startup_title_actor_skin.hpp']};
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(evidence,null,2)+'\n');
console.log(JSON.stringify({checks,assets:assets.size,bytes:fs.statSync(path.join(__dirname,'EVIDENCE.json')).size,
 body_images:bodyImages.length,weapon_definitions:weapons.length}));
