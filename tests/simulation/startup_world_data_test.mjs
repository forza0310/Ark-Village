// 固定字节哈希和输入拒绝回归；不修改原始表或用改表通过业务断言。
import { readFileSync } from 'node:fs';
import assert from 'node:assert/strict';
import { compileStartupWorld } from '../../scripts/simulation/compile_startup_world.mjs';
const [startup,world,tenant] = process.argv.slice(2);
if(!startup||!world||!tenant)throw new Error('参数：startup目录 world目录 tenant原表');
const tables=JSON.parse(readFileSync(`${startup}/TABLES.json`));
const map=JSON.parse(readFileSync(`${startup}/MAP.json`));
const state=JSON.parse(readFileSync(`${startup}/STATE.json`));
const sources={'tenantData.txt':readFileSync(tenant,'utf8')};
for(const name of ['monster.txt','questData.txt','armour.txt','accessory.txt','item.txt','asEventData.txt'])
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
console.log(`startup world data checks: ${checks}`);
