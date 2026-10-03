// 构建期转换：JSON.parse 读取发布包，原表交叉验证后生成只读 C++；运行程序不依赖 Node。
import { readFileSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';

const apk = '1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5';
const integer = value => {
  if (!Number.isSafeInteger(value) || value < -32768 || value > 2147483647)
    throw new Error('发布包整数越界');
  return value;
};
const requireValue = (condition, message) => { if (!condition) throw new Error(message); };
const decimal = value => {
  requireValue(/^-?\d+$/.test(value), '原表整数格式错误');
  return integer(Number(value));
};
const text = value => JSON.stringify(value);
const list = values => `{${values.map(integer).join(',')}}`;

export function compileStartup(map, state, tables, tenantText) {
  requireValue([map, state, tables].every(v => v.apk_sha256 === apk), 'APK 来源不一致');
  requireValue(createHash('sha256').update(tenantText,'utf8').digest('hex') ===
    '5ae35310fbd178f98b273fc2bbe98b1bbf5b72950fca56dbd93c089ca345834a', '固定设施源表哈希变化');
  requireValue(map.width === 24 && map.height === 24 && map.cells.length === 24 &&
    map.cells.every(row => row.length === 24), '地图尺寸或格数不符');
  requireValue(map.bytes === 2356 && map.parsed_bytes === 2354 && map.unread_tail_hex === '0000',
    '地图未解释尾部发生变化');
  // 按原序重编码并核对完整哈希，防止合法记录ID的静默替换。
  const fields = [map.width,map.height,...[...map.cells].reverse().flat(2)];
  for (const regions of [map.regions_l,map.regions_m]) {
    fields.push(regions.length);
    for (const region of regions) {
      requireValue(region.raw.length === 4 && region.logical.join(',') ===
        [region.raw[0],23-region.raw[1],region.raw[2],23-region.raw[3]].join(','), '区域坐标翻转不一致');
      fields.push(...region.raw);
    }
  }
  fields.push(map.spawn_points.length);
  for (const point of map.spawn_points) {
    requireValue(point.raw.length === 2 && point.logical.join(',') ===
      [point.raw[0],23-point.raw[1]].join(','), '出生点坐标翻转不一致');
    fields.push(...point.raw);
  }
  const binary = Buffer.alloc(fields.length*2);
  fields.forEach((v,i)=>binary.writeInt16BE(integer(v),i*2));
  const mapBytes = Buffer.concat([binary,Buffer.from(map.unread_tail_hex,'hex')]);
  requireValue(mapBytes.length === 2356 && createHash('sha256').update(mapBytes).digest('hex') ===
    '51032b93d9d5c3539ad0d2edd0e155e7a3e82a77ca1f0b53a51cf34487f7b5cb', '源地图完整字节哈希不一致');
  const entries = new Map();
  for (const entry of tables.entries) {
    requireValue(!entries.has(entry.entry), '重复原表');
    const bytes = Buffer.from(entry.source_utf8);
    requireValue(bytes.length === entry.bytes && createHash('sha256').update(bytes).digest('hex') === entry.sha256,
      '原表字节或哈希不匹配');
    entries.set(entry.entry, entry.source_utf8.split('\n').map(line => line.split('\t')));
  }
  const rows = new Map(tenantText.trimEnd().split('\n').map(line => {
    const row = line.replace(/\r$/, '').split('\t');
    requireValue(row.length === 36, '设施表列数错误');
    return [decimal(row[0]), row];
  }));
  requireValue(rows.size === tenantText.trimEnd().split('\n').length, '设施ID重复');
  const displayRows = entries.get('mapchip_main.txt');
  requireValue(displayRows?.length === 85, '显示表缺失');
  const displays = new Map(displayRows.map(row => {
    requireValue(row.length === 7 && /^[a-zA-Z0-9_]+\.seb$/.test(row[1]), '显示记录不合法');
    return [decimal(row[0]), row];
  }));
  const cells = map.cells.flat();
  for (const cell of cells) {
    requireValue(cell.length === 2 && displays.has(integer(cell[0])) && integer(cell[1]) >= 0,
      '地图显示记录或源变体无效');
  }
  const seedKeys = new Set();
  requireValue(state.map_seed_instances.length === 8 && state.reset.scene_characters === 0,
    '源种子或人物初始数量变化');
  for (const seed of state.map_seed_instances) {
    integer(seed.x); integer(seed.y);
    const key = `${seed.x},${seed.y}`;
    requireValue(seed.x >= 0 && seed.x < 24 && seed.y >= 0 && seed.y < 24 && !seedKeys.has(key),
      '源种子坐标错误或重叠');
    seedKeys.add(key);
    requireValue(map.cells[seed.y][seed.x][0] === seed.mapchip_id && seed.orientation === 0 &&
      seed.construction_state === 1 && decimal(displays.get(seed.mapchip_id)[5]) === seed.definition_id &&
      decimal(rows.get(seed.definition_id)[3]) === seed.kind, '源种子与地图/原表不一致');
  }
  const catalogs = new Map(state.initial_catalog.map(item => [item.id, item]));
  requireValue(catalogs.size === 9 && [...catalogs.keys()].sort((a,b) => a-b).join(',') ===
    '18,24,28,30,31,33,35,45,66', '首局目录变化');
  for (const item of catalogs.values()) {
    const row = rows.get(item.id);
    requireValue(row && decimal(row[3]) === item.kind && decimal(row[13]) === item.priceBase &&
      decimal(row[14]) === item.constructionBase && decimal(row[35]) === item.flags &&
      decimal(row[10]) === 0 && item.shape === 0, '初期目录不符合单格契约');
    requireValue(item.effective_price === (item.id === 66 ? 200 : item.priceBase) &&
      item.construction_counter_threshold === ((item.flags & 64) ? (item.id === 24 ? 1 : 280) : null),
      '报价或施工阈值与首局职业推导不一致');
  }
  const first = state.first_arrival;
  const firstRow = entries.get('character.txt').find(row => decimal(row[0]) === 1);
  const jobRow = entries.get('job.txt').find(row => decimal(row[0]) === first.job_id);
  const base = firstRow[12].split('&').map(decimal);
  const percentage = jobRow[10].split('&').map(decimal);
  const equipment = firstRow[11].split('&').map(decimal);
  const attributes = base.map((value,i)=>Math.trunc(value*percentage[i]/100));
  requireValue(first.definition_id === 1 && first.instance_uid === 0 && first.name === firstRow[1] &&
    first.job_id === decimal(firstRow[3]) && first.flags === decimal(firstRow[13]) &&
    first.sex === decimal(firstRow[2]) && first.job_level === 1 && first.effort === 0 && first.satisfaction === 0 &&
    first.equipment_ids.join(',') === equipment.join(',') && first.derived_attributes.join(',') === attributes.join(',') &&
    first.initial_hp_slots.join(',') === '22,22,22' && first.combat.join(',') === '22,7,2,2',
    '首名人物身份或属性不一致');
  const talk = tables.talk.selected.find(record => record.id === 69)?.record.split('\t').slice(4);
  requireValue(talk?.length === 2, '首访对话缺失');
  const b = state.boundary.build_interior;
  requireValue([b.min_x,b.max_x,b.min_y,b.max_y].join(',') === '7,16,3,9' &&
    state.boundary.spawn_points.map(v => v.join(',')).join(';') === '11,0;12,0', '边界/出生点契约变化');
  requireValue(state.resources.money === 5000 && state.resources.village_points === 10 &&
    state.resources.popularity === 50 && state.reset.arrival_counter === 420 &&
    state.reset.unlocked_character_definitions.join(',') === '1,2,3', '首局资源/解锁初值变化');
  const ids = [...new Set([...catalogs.keys(), ...state.map_seed_instances.map(v => v.definition_id),
    ...cells.map(v => decimal(displays.get(v[0])[5]))])];
  const definitions = ids.map(id => {
    const row = rows.get(id), item = catalogs.get(id);
    const display = displays.get(decimal(row[9]));
    requireValue(row && display && decimal(display[5]) === id, '设施缺少显示绑定');
    return `{${id},${text(row[1])},${decimal(row[3])},${item ? integer(item.tab) : -1},` +
      `${item ? integer(item.effective_price) : 0},${item ? integer(item.construction_counter_threshold ?? 0) : 0},${decimal(display[0])},` +
      `${decimal(row[4])},${decimal(row[11])},${decimal(row[35])},${decimal(row[10])},${decimal(row[19])}}`;
  });
  const date = state.calendar;
  requireValue([date.year_index,date.month_index,date.subperiod_index,date.counter].join(',') === '0,3,0,0' &&
    state.render.initial_camera.x === 426 && state.render.initial_camera.y === -72, '日期/镜头初值变化');
  const output = `// 自动生成；来源为 data/startup 三份发布文件。不要手工编辑。\n` +
    `#include "dungeon_village_prototype/startup.hpp"\nnamespace dungeon_village_prototype {\n` +
    `const StartupEvidence &startup_evidence() {\nstatic const StartupEvidence value{\n` +
    `24,24,{${cells.map(v => list(v)).join(',')}},\n` +
    `{${[...displays.values()].map(row => `{${decimal(row[0])},${decimal(row[5])},${text(row[1])},${decimal(row[4])}}`).join(',')}},\n` +
    `{${definitions.join(',')}},\n` +
    `{${state.map_seed_instances.map(seed => `{0,${seed.definition_id},{${seed.x},${seed.y}},0,true,{}}`).join(',')}},\n` +
    `{${state.boundary.spawn_points.map(list).join(',')}},${list([b.min_x,b.max_x,b.min_y,b.max_y])},\n` +
    `${state.resources.money},${state.resources.village_points},${state.resources.popularity},` +
    `${list([date.year_index,date.month_index,date.subperiod_index,date.counter])},` +
    `${list(state.reset.unlocked_character_definitions)},${state.reset.arrival_counter},` +
    `{${state.render.initial_camera.x},${state.render.initial_camera.y}},\n` +
    `{${first.instance_uid},${first.definition_id},${text(first.name)},${first.job_id},${first.sex},` +
    `${first.job_level},${first.effort},${first.satisfaction},${list(first.derived_attributes)},` +
    `${list(first.equipment_ids)},${list(first.combat)},${list(first.initial_hp_slots)},{0,0},0},\n` +
    `{${talk.map(text).join(',')}}};\nreturn value;\n}\n}\n`;
  return output;
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const [directory, tenant, output] = process.argv.slice(2);
  if (!directory || !tenant || !output) throw new Error('参数：发布数据目录 设施原表 输出C++');
  const json = name => JSON.parse(readFileSync(`${directory}/${name}.json`, 'utf8'));
  writeFileSync(output, compileStartup(json('MAP'), json('STATE'), json('TABLES'), readFileSync(tenant, 'utf8')));
}
