import { readFileSync } from 'node:fs';
import assert from 'node:assert/strict';
import { compileStartup } from '../scripts/compile_startup.mjs';
const [root, tenant] = process.argv.slice(2);
const json = name => JSON.parse(readFileSync(`${root}/${name}.json`,'utf8'));
const source = [json('MAP'),json('STATE'),json('TABLES'),readFileSync(tenant,'utf8')];
assert.match(compileStartup(...source), /const StartupEvidence &startup_evidence/);
// 固定源表与较小消费者的交叉契约，不执行Java或宣称恢复完整脚本解释器。
const table = name => source[2].entries.find(entry => entry.entry === name).source_utf8
  .split('\n').map(line => line.split('\t'));
const events = table('events.txt');
const automatic = events.filter(row => row[3] !== '0,0');
assert.deepEqual(automatic.map(row => Number(row[0])), [7,35,200,204,225,226,227]);
const first = events.find(row => row[0] === '89');
assert.equal(first[3], '0,0');
assert.equal(first[4], '2,69&12,0');
assert.equal(automatic[0][3], '');
assert.equal(automatic[0][4], '6,3&2,4');
for (const subperiod of [0,1]) {
  const matches = automatic.filter(row => row[3] === '' || row[3].split('&').every(part => {
    const [invert,type,lower,upper] = part.split(',').map(Number);
    assert.equal(invert, 0);
    assert.ok([3,4,5].includes(type));
    const actual = [1,4,subperiod + 1][type - 3];
    return lower <= actual && (upper === undefined || actual <= upper);
  }));
  assert.deepEqual(matches.map(row => Number(row[0])), [7]);
}
const weapons = table('weapon.txt');
assert.deepEqual(weapons.filter(row => Number(row[18]) & 1).map(row => Number(row[0])), [0,1]);
assert.equal(weapons[0][1], '短剑');
assert.equal(Number(weapons[0][5]), 1);
assert.equal(Number(weapons[0][11]), 400);
const road = table('mapchip_main.txt').find(row => row[0] === '37');
assert.equal(road[1], 'road00.seb');
assert.equal(Number(road[4]), 0);
assert.equal(Number(road[6]) & 1, 1);
const reject = edit => { const input=structuredClone(source); edit(input); assert.throws(()=>compileStartup(...input)); };
reject(([m])=>m.cells.pop());
reject(([m])=>m.cells[0][0]=[999,0]);
reject(([m])=>m.cells[0][0]=[14,0]);
reject(([m])=>m.cells[0][0][1]=-1);
reject(([m])=>m.unread_tail_hex='');
reject(([,s])=>s.resources.money=10000);
reject(([,s])=>s.initial_catalog[0].id=29);
reject(([,s])=>s.initial_catalog[2].construction_counter_threshold=400);
reject(([,s])=>s.map_seed_instances[0].x=-1);
reject(([,s])=>s.first_arrival.definition_id=0);
reject(([,s])=>s.first_arrival.equipment_ids[0]=1);
reject(([,s])=>s.first_arrival.job_level=2);
reject(([,s])=>s.reset.unlocked_character_definitions=[1]);
reject(([,,t])=>t.entries[0].source_utf8+='x');
reject(([,,t])=>t.apk_sha256='other');
reject(input=>input[3]+='\n');
console.log('新局数据编译正例与16项拒绝、自动脚本/首访/武器/道路源表契约通过');
