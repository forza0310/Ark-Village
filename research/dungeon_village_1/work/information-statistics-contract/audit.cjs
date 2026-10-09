// 只读固定APK静态分支与已发布素材；不运行游戏，不构建，不重写原表。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(path.join(root,p));
let checks=0;const need=(v,m)=>{++checks;if(!v)throw Error(m);};
const windows=[
 ['soft-labels','work/decompiled/sources/b/g.java',108,108],
 ['soft-label-init','work/decompiled/sources/b/g.java',10542,10552],
 ['soft-label-qualification','work/decompiled/sources/d/a.java',384,395],
 ['town-labels','work/decompiled/sources/b/g.java',166,167],
 ['icon-helper','work/decompiled/sources/b/g.java',1361,1412],
 ['people-draw-four-pages','work/decompiled/sources/b/g.java',1885,2029],
 ['town-statistics','work/decompiled/sources/b/g.java',2032,2190],
 ['item-list-and-equipment-list','work/decompiled/sources/b/g.java',2232,2417],
 ['facility-list','work/decompiled/sources/b/g.java',2418,2463],
 ['equipment-detail-init','work/decompiled/sources/b/g.java',3641,3726],
 ['human-detail-trace','work/decompiled/sources/b/g.java',4888,4947],
 ['equipment-detail-input','work/decompiled/sources/b/g.java',5466,5480],
 ['equipment-detail-draw','work/decompiled/sources/b/g.java',6751,6801],
 ['information-list-init','work/decompiled/sources/b/g.java',10809,10956],
 ['information-input','work/decompiled/sources/b/g.java',12010,12193],
 ['all-item-new-clear','work/decompiled/sources/c/n.java',2236,2240],
 ['human-follow','work/decompiled/sources/c/n.java',3086,3115],
 ['people-contribution-recompute','work/decompiled/sources/a/e.java',395,434],
 ['item-description-column','work/decompiled/sources/a/g.java',48,92],
 ['weapon-icon-column','work/decompiled/sources/a/p.java',59,69],
 ['facility-profit-period','work/decompiled/sources/c/m.java',903,909],
 ['focus-facility-camera','work/decompiled/sources/b/c.java',2087,2110],
 ['replace-with-base-scene','work/decompiled/sources/kairo/android/a/b.java',72,96],
 ['fallback-detail-branch','work/world-page-fallback/GamePage.java',14213,14239],
 ['fallback-detail-fallback-index-and-dead-branch','work/world-page-fallback/GamePage.java',14455,14504],
 ['owner-readonly-definition-fields','prototype/include/dungeon_village_prototype/startup_world_projection.hpp',42,77],
 ['owner-dynamic-inventory','prototype/include/dungeon_village_prototype/startup_world_runtime.hpp',54,80],
 ['maintained-detail-human-binding','prototype/src/startup_world_human.cpp',95,100],
 ['maintained-detail-selection-reject','prototype/src/startup_world_human.cpp',179,217],
 ['maintained-whole-item-clear','prototype/src/startup_world_human.cpp',250,261]
];
const sources=windows.map(([purpose,p,first,last])=>{
 const b=read(p),l=b.toString('utf8').split(/\r?\n/);need(last<=l.length,'source '+p);
 return {purpose,path:p,first,last,bytes:b.length,sha256:sha(b),window_lf_sha256:sha(Buffer.from(l.slice(first-1,last).join('\n')))};
});
const g=read('work/decompiled/sources/b/g.java').toString('utf8');
const labels=JSON.parse(g.match(/private static final int\[\]\[\] bH = ([^;]+);/)[1]
 .replace(/new int(?:\[\])+/g,'').replaceAll('{','[').replaceAll('}',']'));
need(JSON.stringify(labels[38])==='[0,2]','38 normal labels do not expose information action7');
const init=g.slice(g.indexOf('} else if (this.f121a == 38) {',g.indexOf('public final boolean a()')));
need(init.startsWith('} else if (this.f121a == 38) {'),'specific init found');
need(init.slice(0,init.indexOf('} else if (this.f121a == 39)')).includes('this.Z = new Vector[4];'),'only four equipment pages');
need(init.includes('if (pVar2.u < pVar.u)') && init.includes('if (aVar2.j < aVar.j)'),'strict original exchange ordering');
need(g.includes('gVar11.y = 1;'),'people details trace qualification');
const item=read('data/world/item.txt'),rows=item.toString('utf8').trimEnd().split(/\r?\n/).map(r=>r.split('\t'));
need(rows.every(r=>r.length===25),'item source column shape');
need(rows.every(r=>r[23].length>0),'item description column present');
const header=read('prototype/include/dungeon_village_prototype/startup_world_projection.hpp').toString('utf8');
const itemStruct=header.slice(header.indexOf('struct StartupWorldItem {'),header.indexOf('struct StartupWorldTask {'));
need(!itemStruct.includes('description'),'description not yet exported by maintained item definition');
const imageIndex=new Map(read('assets/original/common/img.inf').toString('utf8').trimEnd().split(/\r?\n/).map(l=>{const a=l.split('\t');return [+a[0],a[1].replace(/\.[^.]+$/,'.png')];}));
const images=[9,12,20,21,24,37,91,128,147,148].map(id=>{
 const p='assets/original/common/'+imageIndex.get(id),b=read(p);
 need(b.subarray(0,8).equals(Buffer.from([137,80,78,71,13,10,26,10])),'real PNG '+id);
 return {id,path:p,bytes:b.length,sha256:sha(b),width:b.readUInt32BE(16),height:b.readUInt32BE(20)};
});
const sebIndex=read('assets/original/common/seb.inf').toString('utf8').trimEnd().split(/\r?\n/);
const sprites=[21,88,98].map(id=>{const p='assets/original/common/'+sebIndex[id],b=read(p);return {id,path:p,bytes:b.length,sha256:sha(b)};});
const evidence={date:'2026-10-09',qualification:'APK1.0.8静态；Steam只有声明；未新增控制器或窗口认证',sources,
 soft_labels:Object.fromEntries([34,35,37,38,39,60,73].map(p=>[p,labels[p]])),
 item_source:{path:'data/world/item.txt',bytes:item.length,sha256:sha(item),rows:rows.length,description_column_zero_based:23},
 item:{page:37,filter:'z>0',order:'by source array',visible_rows:5,close_clears:'all item r',empty_event:15},
 equipment:{page:38,pages:4,filters:['all weapons','armour d==2','armour d!=2','all accessories'],availability_filter:false,
  sort:'u/j strict-less descending-inner exchange',visible_rows:4,conditional_detail_branch:73,
  normal_detail_input_reachable:'not certified; bH[38]=[0,2] conflicts with required action7 label',close_clears_new:false},
 assets:{images,sprites},
 missing_maintained_inputs:['StartupWorldItem description g.y','weapon list icon p.d distinct from body p.e'],
 consumers:['ANALYSIS.md','../../ui/INFORMATION_MENU.md'],cpp_changes:0,cpp_builds:0,window_operations:0};
for(const consumer of evidence.consumers)need(fs.existsSync(path.resolve(__dirname,consumer)),'analysis/formal contract consumes evidence');
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(evidence,null,2)+'\n');
console.log(JSON.stringify({checks,source_windows:sources.length,images:images.length,sprites:sprites.length,soft_labels:evidence.soft_labels,
 item_rows:rows.length,evidence_bytes:fs.statSync(path.join(__dirname,'EVIDENCE.json')).size}));
