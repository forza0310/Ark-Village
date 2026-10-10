const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 有限静态窗口身份与局部演算；不运行游戏、不构建、不把夹具当原版动态。
const fs = archivePaths.require('fs'), path = archivePaths.require('path'), crypto = archivePaths.require('crypto');
const root = path.resolve(archivePaths.workDir, '../..');
const hash = b => crypto.createHash('sha256').update(b).digest('hex');
let checks = 0;
function need(v, why) { checks++; if (!v) throw Error(why); }
const windows = [
  ['标题字段与构造', 'work/decompiled/sources/b/h.java', 17, 54],
  ['排序与人物绘制', 'work/decompiled/sources/b/h.java', 96, 145],
  ['标题初始化', 'work/decompiled/sources/b/h.java', 278, 315],
  ['标题更新与页面请求', 'work/decompiled/sources/b/h.java', 407, 553],
  ['输入脉冲消费', 'work/decompiled/sources/kairo/android/ui/a.java', 306, 310],
  ['表单调度', 'work/decompiled/sources/kairo/android/a/b.java', 204, 427],
  ['随机helper', 'work/decompiled/sources/c/d.java', 25, 77],
  ['整数插值', 'work/decompiled/sources/c/d.java', 118, 133],
  ['纪录初始化和更新', 'work/decompiled/sources/b/e.java', 108, 174],
  ['当前应用随机交接', 'prototype/src/startup_application.cpp', 99, 222],
  ['当前应用回放载荷', 'prototype/include/dungeon_village_prototype/startup_application.hpp', 9, 35],
];
const sources = windows.map(([purpose, name, first, last]) => {
  const b = fs.readFileSync(path.join(root, name)), lines = b.toString('utf8').split(/\r?\n/);
  need(last <= lines.length, '窗口超界: ' + name);
  return {purpose, path: name, first, last, bytes: b.length, source_sha256: hash(b),
    window_lf_sha256: hash(Buffer.from(lines.slice(first - 1, last).join('\n')))};
});
const title = fs.readFileSync(path.join(root, 'work/decompiled/sources/b/h.java'), 'utf8');
const spawn = title.slice(title.indexOf('if (i3 != -1)'), title.indexOf('int i4 = this.m;'));
need(spawn.includes('this.q[i3][1] = r[c.d.a(r.length)]'), '定义抽取');
need(spawn.includes('this.q[i3][4] = c.d.a(2)'), '方向抽取');
need(!spawn.includes('[5]'), '出生不重置age');
need(title.includes('this.u[i4][1] > this.u[length][1]'), '严格大于交换');
const input = fs.readFileSync(path.join(root, 'work/decompiled/sources/kairo/android/ui/a.java'), 'utf8');
need(input.includes('this.q = (i ^ (-1)) & i2;'), '确认脉冲消费');
// 三元素是原交换关系的最小局部反例，不是当前20槽的原窗口轨迹。
const scratch = [[0,212],[1,212],[2,210]];
for (let i = 0; i < scratch.length - 1; ++i)
  for (let j = scratch.length - 1; j > i; --j)
    if (scratch[i][1] > scratch[j][1]) [scratch[i], scratch[j]] = [scratch[j], scratch[i]];
need(JSON.stringify(scratch.map(r => r[0])) === '[2,1,0]', '相等y非稳定顺序');
const files = ['../../ui/TITLE_PRESENTATION.md', 'README.md'];
let links = 0;
for (const name of files) {
  const file = path.resolve(archivePaths.workDir, name), text = fs.readFileSync(file, 'utf8');
  for (const m of text.matchAll(/\]\(([^)]+)\)/g)) {
    if (/^(https?:|#)/.test(m[1])) continue;
    const target = m[1].split('#')[0].replace(/:\d+$/, '');
    // EVIDENCE是本脚本的显式输出，首次生成时尚不存在。
    need(target.endsWith('EVIDENCE.json') || fs.existsSync(path.resolve(path.dirname(file), target)), '坏链接: ' + target);
    links++;
  }
}
const evidence = {date: '2026-10-09', qualification: 'APK局部静态／维护源码；未新增Steam解码或原窗口；演算非C++回归',
  sources, checks, links, fixture: {input_y: [212,212,210], output_indices: scratch.map(r => r[0]),
    qualification: '局部交换反例'}, resources: {actor_slots:20, slot_integer_fields:6,
    natural_active_upper_bound_inference:13, added_asset_copies:0, build_trees:0, background_processes:0},
  consumer:'../../ui/TITLE_PRESENTATION.md'};
fs.writeFileSync(path.join(archivePaths.workDir, 'EVIDENCE.json'), JSON.stringify(evidence, null, 2) + '\n');
console.log(JSON.stringify({checks, links, windows:sources.length, evidence_bytes:fs.statSync(path.join(archivePaths.workDir,'EVIDENCE.json')).size}));
