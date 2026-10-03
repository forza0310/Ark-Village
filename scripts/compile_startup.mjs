// Adapted from research/prototype/scripts/compile_startup.mjs. Structured publishing inputs are
// validated at build time; the C++ game needs neither Node nor a JSON parser at runtime.
import { readFileSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { parseLoadedMap } from './compile_loaded_map.mjs';

const apk = '1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5';
const need = (condition, message) => { if (!condition) throw new Error(message); };
const integer = value => {
  need(Number.isSafeInteger(value) && value >= -32768 && value <= 2147483647, 'Invalid published integer');
  return value;
};
const decimal = value => { need(/^-?\d+$/.test(value), 'Invalid table integer'); return integer(Number(value)); };
const integers = value => value === '' ? [] : value.split('&').map(decimal);
const text = value => {
  need(typeof value === 'string' && !value.includes('\0'), 'Invalid published string');
  // JSON Unicode/control escapes are not all legal C++ escapes; use UTF-8 octal bytes instead.
  return '"' + [...Buffer.from(value)].map(byte => '\\' + byte.toString(8).padStart(3, '0')).join('') + '"';
};
const list = values => `{${values.map(integer).join(',')}}`;
const array = (values, count) => { need(values.length === count, 'Invalid array length'); return list(values); };

export function compileStartup(map, state, tables, tenantText, loadedCells, loadedInstances) {
  const loaded = parseLoadedMap(loadedCells, loadedInstances);
  need(createHash('sha256').update(tenantText).digest('hex') ===
       '5ae35310fbd178f98b273fc2bbe98b1bbf5b72950fca56dbd93c089ca345834a', 'Facility source hash mismatch');
  need([map, state, tables].every(v => v.apk_sha256 === apk), 'APK provenance mismatch');
  need(map.width === 24 && map.height === 24 && map.cells.length === 24 &&
       map.cells.every(row => row.length === 24), 'Invalid source map dimensions');
  need(map.bytes === 2356 && map.parsed_bytes === 2354 && map.unread_tail_hex === '0000', 'Source tail changed');
  const entries = new Map();
  for (const entry of tables.entries) {
    need(!entries.has(entry.entry), 'Duplicate source table');
    const bytes = Buffer.from(entry.source_utf8);
    need(bytes.length === entry.bytes && createHash('sha256').update(bytes).digest('hex') === entry.sha256,
         'Source table hash mismatch');
    entries.set(entry.entry, entry.source_utf8.split('\n').map(line => line.split('\t')));
  }
  const rows = new Map();
  for (const line of tenantText.trimEnd().split('\n')) {
    const row = line.replace(/\r$/, '').split('\t'), id = decimal(row[0]);
    need(row.length === 36 && !rows.has(id), 'Invalid or duplicate facility row');
    rows.set(id, row);
  }
  need(rows.size === 85, 'Incomplete facility table');
  const displayRows = entries.get('mapchip_main.txt');
  need(displayRows?.length === 85, 'Missing display table');
  const displays = new Map();
  for (const row of displayRows) {
    const id = decimal(row[0]);
    need(row.length === 7 && /^[a-zA-Z0-9_]+\.seb$/.test(row[1]) && !displays.has(id), 'Invalid display record');
    displays.set(id, row);
  }
  const cells = map.cells.flat();
  for (const cell of cells)
    need(cell.length === 2 && displays.has(integer(cell[0])) && integer(cell[1]) >= 0, 'Invalid source cell');
  const keys = new Set();
  need(state.map_seed_instances.length === 8 && state.reset.scene_characters === 0, 'Source seeds changed');
  for (const seed of state.map_seed_instances) {
    const key = `${integer(seed.x)},${integer(seed.y)}`;
    need(seed.x >= 0 && seed.x < 24 && seed.y >= 0 && seed.y < 24 && !keys.has(key), 'Invalid seed cell');
    keys.add(key);
    need(map.cells[seed.y][seed.x][0] === seed.mapchip_id && seed.orientation === 0 && seed.construction_state === 1 &&
         decimal(displays.get(seed.mapchip_id)[5]) === seed.definition_id &&
         decimal(rows.get(seed.definition_id)[3]) === seed.kind, 'Seed/table mismatch');
  }
  const catalog = new Map(state.initial_catalog.map(item => [item.id, item]));
  need(catalog.size === 9 && [...catalog.keys()].sort((a,b) => a-b).join(',') === '18,24,28,30,31,33,35,45,66',
       'Initial catalog changed');
  for (const item of catalog.values()) {
    const row = rows.get(item.id);
    need(row && decimal(row[3]) === item.kind && decimal(row[13]) === item.priceBase &&
         decimal(row[14]) === item.constructionBase && decimal(row[35]) === item.flags &&
         decimal(row[10]) === item.shape && item.shape === 0, 'Catalog/table mismatch');
    need(item.effective_price === (item.id === 66 ? 200 : item.priceBase) &&
         item.construction_counter_threshold === ((item.flags & 64) ? (item.id === 24 ? 1 : 280) : null),
         'Initial job-derived quote changed');
  }
  const first = state.first_arrival;
  // STARTUP documents only this finite initial cohort: two job1 farmers and one job2 carpenter.
  // Do not generalize the unknown ten-category mapping to arbitrary later professions.
  need(state.reset.unlocked_character_definitions.join(',') === '1,2,3', 'Initial profession cohort changed');
  const characters = new Map(entries.get('character.txt').map(row => [decimal(row[0]), row]));
  need([1,2,3].map(id => decimal(characters.get(id)[3])).join(',') === '1,2,1', 'Initial professions changed');
  const jobCounts = [0,0,0,0,0,0,0,0,2,1];
  const firstRow = entries.get('character.txt').find(row => decimal(row[0]) === 1);
  need(first.definition_id === 1 && first.instance_uid === 0 && first.name === firstRow[1] &&
       first.job_id === decimal(firstRow[3]) && first.flags === decimal(firstRow[13]) &&
       first.derived_attributes.join(',') === '22,2,2,2,2,2' && first.combat.join(',') === '22,7,2,2',
       'First visitor mismatch');
  const talk = tables.talk.selected.find(record => record.id === 69)?.record.split('\t').slice(4);
  need(talk?.length === 2, 'Missing first tutorial');
  const b = state.boundary.build_interior;
  need([b.min_x,b.max_x,b.min_y,b.max_y].join(',') === '7,16,3,9' &&
       state.boundary.spawn_points.map(v => v.join(',')).join(';') === '11,0;12,0', 'Boundary changed');
  need(state.resources.money === 5000 && state.resources.village_points === 10 && state.resources.popularity === 50 &&
       state.reset.arrival_counter === 420, 'Opening resources changed');
  const definitions = [...rows].map(([id, row]) => {
    const item = catalog.get(id);
    const display = [...displays.values()].find(v => decimal(v[5]) === id);
    need(display, 'Missing facility display binding');
    const slots = integers(row[26]), deltas = integers(row[27]);
    need(slots.length === deltas.length && slots.every(v => v >= 0 && v < 3), 'Invalid neighbour modifiers');
    const effects = integers(row[28]), plusCounts = integers(row[29]);
    need(effects.every(v => v >= 0 && v < 7) &&
         plusCounts.every(v => v >= 0), 'Invalid effect indicator');
    const endpoints = Array.from({length:4}, (_,i) => array(row.slice(15+i*2,17+i*2).map(decimal),2));
    need(row.slice(15,25).every(v => decimal(v) >= 0) && decimal(row[35]) >= 0 &&
         decimal(row[2]) >= 0 && decimal(row[2]) < 7, 'Invalid facility economy or icon');
    return `{${id},${text(row[1])},${decimal(row[3])},${item ? integer(item.tab) : -1},` +
      `${item ? integer(item.effective_price) : 0},${item ? integer(item.construction_counter_threshold ?? 0) : 0},` +
      `${decimal(display[0])},${decimal(row[10])},${decimal(row[2])},${decimal(row[4])},${decimal(row[5])},` +
      `{ {{${endpoints.join(',')}}},${array(row.slice(23,25).map(decimal),2)},${decimal(row[13])},` +
      `${decimal(row[14])},${decimal(row[35])},${decimal(row[3]) === 2 ? 'true' : 'false'}},` +
      `{${slots.map((slot,i)=>`{${slot},${deltas[i]}}`).join(',')}},` +
      `${list(effects)},${list(plusCounts)},${decimal(row[11])}}`;
  });
  const date = state.calendar;
  return `// Generated from pinned research data; do not edit.\n#include "ark/app/startup_data.hpp"\n` +
    `namespace ark::app { const StartupData &startup_data() { static const StartupData value{\n` +
    `{24,24,{${loaded.cells.map(v => array(v.slice(5,7),2)).join(',')}}},\n` +
    `{${[...displays.values()].map(row => `{${decimal(row[0])},${decimal(row[5])},${text(row[1])}}`).join(',')}},\n` +
    `{${definitions.join(',')}},\n` +
    `{${loaded.instances.map(v => `{${v[2]},${v[3]},{${v[4]},${v[5]}},0,0,true,${v[1]}}`).join(',')}},\n` +
    `{${state.boundary.spawn_points.map(v => array(v,2)).join(',')}},${list([b.min_x,b.max_x,b.min_y,b.max_y])},\n` +
    `${state.resources.money},${state.resources.village_points},${state.resources.popularity},` +
    `${list([date.year_index,date.month_index,date.subperiod_index,date.counter])},\n` +
    `${list(state.reset.unlocked_character_definitions)},${state.reset.arrival_counter},` +
    `{${integer(state.render.initial_camera.x)},${integer(state.render.initial_camera.y)}},\n` +
    `{${integer(first.instance_uid)},${integer(first.definition_id)},${text(first.name)},${integer(first.job_id)},${integer(first.sex)},` +
    `${integer(first.job_level)},${integer(first.effort)},${integer(first.satisfaction)},${array(first.derived_attributes,6)},` +
    `${array(first.equipment_ids,4)},${array(first.combat,4)},${array(first.initial_hp_slots,3)},{0,0},2,{0,0},0},\n` +
    `{${talk.map(text).join(',')}},${array(jobCounts,10)},\n` +
    `{${loaded.cells.map(v => `{${v[2]},${v[3]},world::RouteCategory(${v[4]}),${v.slice(5,10).join(',')},${!!v[10]},${!!v[11]},${v[12]},${v[13]}}`).join(',')}}}; return value; } }\n`;
}
if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const [root, output] = process.argv.slice(2);
  const json = name => JSON.parse(readFileSync(`${root}/${name}.json`, 'utf8'));
  writeFileSync(output, compileStartup(json('MAP'), json('STATE'), json('TABLES'), readFileSync(`${root}/tenantData.txt`, 'utf8'),
    readFileSync(`${root}/LOADED_MAP.tsv`, 'utf8'), readFileSync(`${root}/LOADED_INSTANCES.tsv`, 'utf8')));
}
