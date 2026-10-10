const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 信息菜单只读静态审计；不执行游戏、不构建、不复制反编译实现。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const root=path.resolve(archivePaths.workDir,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
let checks=0;const need=(v,m)=>{++checks;if(!v)throw Error(m);};
const windows=[
 ['menu-labels-and-soft-label-map','work/decompiled/sources/b/g.java',108,109],
 ['page-constructor-defaults','work/decompiled/sources/b/g.java',202,250],
 ['footer-is-draw-only','work/decompiled/sources/b/g.java',437,468],
 ['menu-append','work/decompiled/sources/b/g.java',513,524],
 ['menu-row-hit-and-label','work/decompiled/sources/b/g.java',548,631],
 ['menu-repeat-input','work/decompiled/sources/b/g.java',6805,6836],
 ['main-info-order','work/decompiled/sources/b/g.java',10542,10587],
 ['main-opens-info','work/decompiled/sources/b/g.java',11130,11174],
 ['info-opens-income-and-retires-menus','work/decompiled/sources/b/g.java',11260,11278],
 ['income-draw','work/decompiled/sources/b/g.java',1794,1885],
 ['income-input','work/decompiled/sources/b/g.java',12064,12082],
 ['arrow-components','work/decompiled/sources/b/g.java',11070,11073],
 ['menu-classification','work/decompiled/sources/b/g.java',12437,12439],
 ['page-outer-transform','work/decompiled/sources/b/g.java',9120,9127],
 ['retire-menu-family','work/decompiled/sources/d/a.java',545,558],
 ['soft-label-tokens','work/decompiled/sources/d/a.java',123,123],
 ['soft-label-consumption','work/decompiled/sources/d/a.java',384,395],
 ['main-menu-object','work/decompiled/sources/b/c.java',1164,1170],
 ['main-menu-request','work/decompiled/sources/b/c.java',1553,1575],
 ['year-turnover-clears-buckets','work/decompiled/sources/b/c.java',184,194],
 ['categories','work/decompiled/sources/c/n.java',103,104],
 ['buckets-allocation','work/decompiled/sources/c/n.java',143,143],
 ['income-write','work/decompiled/sources/c/n.java',3162,3165],
 ['expense-write','work/decompiled/sources/c/n.java',3459,3462],
 ['year-reset','work/decompiled/sources/c/n.java',4560,4567],
 ['amount-format','work/decompiled/sources/b/d.java',31,61],
 ['form-open-close','work/decompiled/sources/kairo/android/a/a.java',23,29],
 ['form-close','work/decompiled/sources/kairo/android/a/a.java',51,57],
 ['form-push-retire-parent','work/decompiled/sources/kairo/android/a/b.java',97,119],
 ['form-close-resume','work/decompiled/sources/kairo/android/a/b.java',144,164],
 ['frame-only-top-update','work/decompiled/sources/kairo/android/a/b.java',311,338],
 ['fallback-income-long-format','work/world-page-fallback/GamePage.java',8633,8671],
 ['fallback-profit-long-format','work/world-page-fallback/GamePage.java',8773,8812],
 ['fallback-income-input','work/world-page-fallback/GamePage.java',39653,39696],
 ['owner-buckets','prototype/include/dungeon_village_prototype/startup_world_runtime.hpp',88,95],
 ['owner-categories','example/include/dungeon_village_reference/accounting.hpp',13,16],
 ['owner-new-cash-write','prototype/src/startup_world_runtime.cpp',269,291],
 ['owner-finance-read','prototype/src/startup_world_runtime.cpp',341,354],
 ['owner-year-clear','prototype/src/startup_world_runtime_calendar.cpp',279,291]
];
const sources=windows.map(([purpose,file,first,last])=>{
 const b=fs.readFileSync(path.join(root,file)),lines=b.toString('utf8').split(/\r?\n/);
 need(last<=lines.length,'source window '+file);
 return {purpose,path:file,first,last,bytes:b.length,source_sha256:sha(b),
  window_lf_sha256:sha(Buffer.from(lines.slice(first-1,last).join('\n')))};
});
const g=fs.readFileSync(path.join(root,'work/decompiled/sources/b/g.java'),'utf8');
const menu=g.slice(g.indexOf('} else if (this.f121a == 9) {',g.indexOf('public final boolean a()')));
const order=[...menu.slice(0,menu.indexOf('} else',1)).matchAll(/b\((\d+)\);/g)].map(m=>+m[1]);
need(JSON.stringify(order)==='[15,14,16,17,18]','actual info menu order');
const row=g.match(/private static final int\[\]\[\] bH = ([^;]+);/)[1];
const labels=JSON.parse(row.replace(/new int(?:\[\])+/g,'').replaceAll('{','[').replaceAll('}',']'));
need(JSON.stringify(labels[36])==='[0,2]','income soft labels');
const owner=fs.readFileSync(path.join(root,'prototype/include/dungeon_village_prototype/startup_world_runtime.hpp'),'utf8');
need(owner.includes('std::array<std::array<std::array<int, 2>, 5>, 12> monthly_cash'),'full source bucket shape');
const codec=fs.readFileSync(path.join(root,'prototype/src/startup_world_codec_fields.inc'),'utf8');
need(codec.includes('StartupWorldRuntimeState.monthly_cash'),'existing codec covers buckets');
const evidence={date:'2026-10-09',scope:'原静态链闭合；尚无维护raw9/raw36消费者；Steam仅常量声明',sources,
 info_menu:order.map((label,index)=>({index,label,page:({14:34,15:35,16:36,17:37,18:38})[label]})),
 income:{page:36,initial_period:0,periods:['current_month_bucket','all_twelve_current_year_buckets'],
  categories:['设施','怪物','冒险者','商店','其它'],dimensions:[12,5,2],soft_labels:labels[36],
  menu_pages_retired_on_entry:[3,9],normal_return:'underlying_scene',footer_click_target:false,
  original_accumulation:'Java int, then explicit int-to-long currency formatter'},
 implementation_boundary:{owner_field:'StartupWorldRuntimeState.monthly_cash',new_owner_fields:0,
  query_inputs:['const monthly_cash&','current_month 0..11','period 0/1'],
  cpp_query:'prototype/src/startup_information.cpp',
  output_consumer:'prototype/tests/startup_skin_checks.cpp',
  cpp_page_implemented:false,steam_dynamic_observed:false,new_window_operations:0},
 consumers:['../../ui/INFORMATION_MENU.md','README.md']};
for(const file of evidence.consumers)need(fs.existsSync(path.resolve(archivePaths.workDir,file)),'consumer exists');
fs.writeFileSync(path.join(archivePaths.workDir,'EVIDENCE.json'),JSON.stringify(evidence,null,2)+'\n');
console.log(JSON.stringify({checks,source_windows:sources.length,bytes:fs.statSync(path.join(archivePaths.workDir,'EVIDENCE.json')).size,
 cpp_builds:0,window_operations:0}));
