// 固定字节哈希和输入拒绝回归；不修改原始表或用改表通过业务断言。
import { readFileSync } from 'node:fs';
import assert from 'node:assert/strict';
import { compileStartupWorld, validateMagicPotRows } from '../scripts/compile_startup_world.mjs';
import { createHash } from 'node:crypto';
const [startup,world,tenant] = process.argv.slice(2);
if(!startup||!world||!tenant)throw new Error('参数：startup目录 world目录 tenant原表');
const tables=JSON.parse(readFileSync(`${startup}/TABLES.json`));
const map=JSON.parse(readFileSync(`${startup}/MAP.json`));
const state=JSON.parse(readFileSync(`${startup}/STATE.json`));
const sources={'tenantData.txt':readFileSync(tenant,'utf8')};
for(const name of ['monster.txt','questData.txt','armour.txt','accessory.txt','item.txt','asEventData.txt','magicPot.txt'])
  sources[name]=readFileSync(`${world}/${name}`,'utf8');
for(const name of ['events.txt','talk.txt','news.txt','evtmsgs.txt','popularBonus.txt'])
  sources[name]=readFileSync(`${world}/../scripts/original/${name}`,'utf8');
const output=compileStartupWorld(tables,map,sources,state);
assert.match(output,/const StartupWorldRules &startup_world_rules\(\)/);
assert.match(output,/{0,130,5,3,15}/);
assert.ok(output.includes('{54,{3}}'));
assert.equal(output,compileStartupWorld(tables,map,sources,state));
let checks=4;
const clone=value=>JSON.parse(JSON.stringify(value));
for(const name of Object.keys(sources)) {
  const bad={...sources,[name]:sources[name]+'\n'};
  assert.throws(()=>compileStartupWorld(tables,map,bad,state),/哈希|hash/);++checks;
}
for(const name of ['character.txt','job.txt','weapon.txt']) {
  const bad=clone(tables);bad.entries.find(entry=>entry.entry===name).source_utf8+='\n';
  assert.throws(()=>compileStartupWorld(bad,map,sources,state));++checks;
}
let bad=clone(tables);bad.entries.push(clone(bad.entries[0]));
assert.throws(()=>compileStartupWorld(bad,map,sources,state));++checks;
bad=clone(tables);bad.apk_sha256='0'.repeat(64);
assert.throws(()=>compileStartupWorld(bad,map,sources,state));++checks;
bad=clone(map);bad.regions_l[0].logical[3]++;
assert.throws(()=>compileStartupWorld(tables,bad,sources,state));++checks;
bad=clone(map);bad.cells[0][0][1]++;
assert.throws(()=>compileStartupWorld(tables,bad,sources,state));++checks;
bad=clone(state);bad.first_arrival.legacy_D[2]=1;
assert.throws(()=>compileStartupWorld(tables,map,sources,bad));++checks;
const missing={...sources};delete missing['monster.txt'];
assert.throws(()=>compileStartupWorld(tables,map,missing,state));++checks;
// 原magicPot构造列是ID/name/type/reward/经验/四成本/flags，不借Steam名字生成APK表。
assert.equal(Buffer.byteLength(sources['magicPot.txt']),1379);++checks;
assert.equal(createHash('sha256').update(sources['magicPot.txt']).digest('hex'),
  '7a28381b01ae861cd99ce8ccc63bef5f8ce62f12473e3047052879f8f12a5b8c');++checks;
const recipes=sources['magicPot.txt'].replace(/\n$/,'').split('\n').map(row=>row.split('\t'));
const rewards=[36,33,50,30,85];
validateMagicPotRows(recipes,rewards);++checks;
assert.equal(recipes.length,40);++checks;
assert.deepEqual(recipes[1],['1','烤肉','0','5','60','100','20','30','0','2']);++checks;
assert.ok(output.includes('{1,"烤肉",0,5,60,{100,20,30,0},2}'));++checks;
assert.ok(output.includes('{39,"木灵大树",4,74,45,{50,50,50,10},2}'));++checks;
assert.ok(output.includes(',400,{0,2,0,0},5,"培育的很好的马铃薯"}'));++checks; // APK道具0：保留四元素/图标oracle，并验证第23列原说明。
assert.ok(output.includes(',84,"恢复魔法可以学会"}'));++checks; // APK道具35原文；不是名称或按效果重新生成的文案。
for(const change of [
  r=>r.pop(), r=>r[0].pop(), r=>r[1][0]='0', r=>r[1][1]='',
  r=>r[1][2]='5', r=>r[1][2]='-1', r=>r[1][3]='36', r=>r[1][3]='-1',
  r=>r[1][4]='-1', r=>r[1][5]='-1', r=>r[1][9]='-1',
  r=>r[1][5]='2147483648', r=>r[1][2]='0.5',
]){
  const altered=clone(recipes);change(altered);
  assert.throws(()=>validateMagicPotRows(altered,rewards));++checks;
}
for(let kind=0;kind<rewards.length;++kind){
  const altered=clone(recipes);altered[1][2]=String(kind);altered[1][3]=String(rewards[kind]);
  assert.throws(()=>validateMagicPotRows(altered,rewards),/跨目录引用/);++checks;
}
const noRecipes={...sources};delete noRecipes['magicPot.txt'];
assert.throws(()=>compileStartupWorld(tables,map,noRecipes,state),/哈希/);++checks;
console.log(`startup world data checks: ${checks}`);
