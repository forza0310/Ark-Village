// Cold processes join real player files at a business milestone, not a date target.
// Synchronous product-command acceptance is not OS input or full-route FIFO coverage.
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn, execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { createHash, randomUUID } from 'node:crypto';
import { verify as verifyLibraries } from '../../scripts/shared_library_contract.mjs';
const [exe, supplied, option, prefixArgument] = process.argv.slice(2);
if (!exe || !supplied || !(process.argv.length === 4 ||
    (process.argv.length === 6 && ['--resume-prefix', '--first-star-prefix', '--late-resume-prefix', '--second-star-prefix', '--third-star-prefix'].includes(option) && prefixArgument)))
  throw Error('Expected executable and build work parent [--resume-prefix completed-new-directory | --first-star-prefix passed-first-star-directory | --late-resume-prefix completed-late-new-directory | --second-star-prefix passed-second-star-directory | --third-star-prefix passed-second-star-directory]');
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const build = await fs.realpath(path.join(root, 'build'));
const inside = candidate => {
  const relative = path.relative(build, candidate);
  return !path.isAbsolute(relative) && relative !== '..' && !relative.startsWith(`..${path.sep}`);
};
const sha256 = content => createHash('sha256').update(content).digest('hex');
const runFile = promisify(execFile);
// The exact Git blob from formal 932fa45 also works in a shallow clone whose tree
// contains this blob. Never read the mutable research checkout or add it to save identity.
const coverageSource = {
  revision: '932fa45', blob: '26a0f1c56a605d65620d3ccab65c48be0a84046c',
  file: 'research/dungeon_village_1/data/PROGRESSION_CONTENT_INDEX.json',
  bytes: 212601, sha256: '8bd283f5834d17dc29c25f1773c7f086b0f2285721dfc1fa72b5e51e7da08a6c'
};
const aliases = {activity: 'village_activity', task: 'quest', recipe: 'magic_recipe'};
const sourceKey = (kind, id) => `${aliases[kind] ?? kind}:${kind === 'rank' ? id - 1 : id}`;
const need = (ok, message) => { if (!ok) throw Error(`Campaign coverage: ${message}`); };
const sharedContracts = {
  village_activity: ['persistence_contract'], quest: ['persistence_contract'],
  popularity_bonus: ['persistence_contract'], event: ['caller_contract'],
  facility: ['instance_contract', 'use_effect_contract'], human: ['persistence_contract', 'appearance_contract'],
  profession: ['persistence_contract', 'effect_contract'], weapon: ['effect_contract'],
  armour: ['effect_contract'], accessory: ['effect_contract'], item: ['effect_contract'],
  magic_recipe: ['persistence_contract'], monster: ['persistence_contract']
};
function indexRecords(index) {
  need(index.version === 8 && index.definitions === 775 && Array.isArray(index.tables), 'index version/denominator');
  const records = new Map(), kinds = new Set();
  for (const table of index.tables) {
    need(!kinds.has(table.kind) && Array.isArray(table.records) &&
      table.records.length === table.definitions,
      'table identity/count/contract');
    kinds.add(table.kind);
    // Schema6 keeps each chain contract at its table; the duplicated top-level map was removed.
    const requiredContracts = ['chain_contract', ...(sharedContracts[table.kind] ?? [])];
    for (const field of requiredContracts)
      need(typeof table[field] === 'string' && table[field].startsWith('../rules/'), 'missing shared contract ' + table.kind + '.' + field);
    for (const record of table.records) {
      const key = `${table.kind}:${record.id}`;
      need(Number.isSafeInteger(record.id) && record.id >= 0 && !records.has(key) &&
        record.source?.entry === table.entry && Number.isSafeInteger(record.source.line) && record.source.line > 0,
        'duplicate/invalid source definition ' + key);
      if (table.kind === 'event')
        need(record.event_type === 0 && !('flags' in record), 'event type is not an unlock flag ' + key);
      for (const entry of [...(record.unlock_sources ?? []), ...(record.non_directory_sources ?? [])])
        need(typeof index.route_contracts[entry.route] === 'string', 'unknown source route ' + entry.route);
      records.set(key, {table, record});
    }
  }
  need(records.size === 775 && kinds.size === 14, 'source catalogue incomplete');
  const differences = index.version_differences, arrival = differences?.[0];
  need(Array.isArray(differences) && differences.length === 1 &&
    arrival?.feature === 'simultaneous_arrival_limit' && arrival.apk?.soft === 20 && arrival.apk?.hard === 20 &&
    arrival.steam?.value === 25 && arrival.steam?.basis === 'character_definition_directory_length' &&
    arrival.contract === index.tables.find(table => table.kind === 'human').appearance_contract,
    'APK/Steam arrival metadata');
  return records;
}
const coverageBytes = (await runFile('git', ['-c', `safe.directory=${root.replaceAll('\\', '/')}`,
  'show', coverageSource.blob], {cwd: root, windowsHide: true, encoding: 'buffer', timeout: 30000,
  maxBuffer: 1024 * 1024})).stdout;
need(coverageBytes.length === coverageSource.bytes && sha256(coverageBytes) === coverageSource.sha256,
  'formal index blob identity');
const coverageIndex = JSON.parse(coverageBytes.toString('utf8'));
const catalogue = indexRecords(coverageIndex);
function eventCallers(index, records) {
  const table = index.tables.find(value => value.kind === 'event'), callers = new Map();
  const surface = table.script_surface;
  need(surface?.programs === 552 && surface.nonempty_programs === 465 && surface.instructions === 835 &&
    surface.literal_nested_calls === 81 && Array.isArray(surface.nested_targets) &&
    typeof surface.nested_evidence === 'string' && surface.nested_evidence.includes('不是运行时调用数') &&
    surface.nested_targets.join(',') === '77' && surface.parameterized_nested_targets === 0 &&
    Array.isArray(surface.used_opcodes) && surface.used_opcodes.join(',') ===
      '0,1,2,3,4,5,6,7,12,13,14,15,16,17,18,21,22,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40',
    'compact script surface');
  const add = (category, id, reference) => {
    need(Number.isSafeInteger(id) && records.has(`event:${id}`), 'invalid event caller reference');
    const previous = callers.get(id);
    need(!previous || previous.category === category, 'overlapping event source groups ' + id);
    const entry = previous ?? {category, refs: []};
    if (!entry.refs.includes(reference)) entry.refs.push(reference);
    callers.set(id, entry);
  };
  for (const [method, ids] of Object.entries(table.literal_caller_groups)) {
    need(typeof method === 'string' && Array.isArray(ids), 'literal caller group');
    for (const id of ids) add('literal', id, method);
  }
  for (const [group, entry] of table.formula_caller_groups.entries()) {
    need(typeof entry.method === 'string' && typeof entry.expression === 'string' && Array.isArray(entry.event_ids),
      'formula caller group');
    for (const id of entry.event_ids) add('formula', id, group);
  }
  for (const id of table.automatic_candidates) add('automatic', id, 'automatic_candidates');
  const tableReferences = {};
  for (const record of table.records) if (record.script_callers?.length) {
    for (const caller of record.script_callers) {
      need(caller.table === 'quest' && caller.evidence === 'table_reference_only' &&
        caller.column === 13 && caller.instruction === 0 && caller.definition_encoding === 'inclusive_range' &&
        Array.isArray(caller.definitions) && caller.definitions.length === 2 &&
        caller.definitions.every(Number.isSafeInteger) && caller.definitions[0] >= 0 &&
        caller.definitions[1] >= caller.definitions[0], 'compact script caller range');
      for (let id = caller.definitions[0]; id <= caller.definitions[1]; ++id)
        need(records.has(`quest:${id}`), 'unknown quest in script caller range');
    }
    // Keep the one inclusive 0..80 range, rather than expanding 81 identical edges.
    // A stored opcode reference has no certified runtime caller in either audited version.
    tableReferences[record.id] = record.script_callers;
    add('table_reference_only', record.id, record.id);
  }
  for (const id of table.unresolved_callers) add('unresolved', id, 'unresolved_callers');
  const counts = Object.fromEntries(['literal', 'formula', 'automatic', 'table_reference_only', 'unresolved']
    .map(category => [category, [...callers.values()].filter(entry => entry.category === category).length]));
  need(callers.size === 200 && Object.values(counts).join(',') === '121,37,7,1,34', 'event source partition');
  const runtimeUnresolved = [...callers].filter(([, entry]) =>
    ['table_reference_only', 'unresolved'].includes(entry.category)).map(([id]) => id).sort((a, b) => a - b);
  need(counts.literal + counts.formula + counts.automatic === 165 && runtimeUnresolved.length === 35,
    'actual entry and caller gap counts');
  return {callers, shared: {contract: table.caller_contract, counts, known_entry_count: 165,
    runtime_unresolved_count: runtimeUnresolved.length, runtime_unresolved: runtimeUnresolved,
    literal: table.literal_caller_groups, formula: table.formula_caller_groups,
    automatic_candidates: table.automatic_candidates, table_references: tableReferences,
    script_surface: table.script_surface, unresolved_callers: table.unresolved_callers}};
}
const eventSources = eventCallers(coverageIndex, catalogue);
function inventory(text) {
  const lines = text.trim().split(/\r?\n/).filter(line => line && !line.startsWith('#'));
  const expected = ['category', 'id', 'name', 'source_kind', 'rank_hint', 'observed_in_segment',
    'verified_receipts', 'required_evidence'];
  need(lines.shift() === expected.join('\t'), 'inventory header');
  const rows = new Map();
  for (const line of lines) {
    const values = line.split('\t');
    need(values.length === expected.length && /^\d+$/.test(values[1]), 'inventory row boundary');
    const row = Object.fromEntries(expected.map((key, i) => [key, values[i]]));
    row.id = Number(row.id);
    const key = sourceKey(row.category, row.id);
    need(!rows.has(key) && (catalogue.has(key) || ['spell', 'campaign', 'function'].includes(row.category)),
      'duplicate/unknown product definition ' + key);
    rows.set(key, row);
  }
  for (const [key, {table}] of catalogue)
    need(rows.has(key) || ['event', 'popularity_bonus'].includes(table.kind), 'missing product definition ' + key);
  return rows;
}
function scope(table, record) {
  if (['event', 'popularity_bonus'].includes(table.kind)) return 'source_dependency';
  if ((table.kind === 'village_activity' && record.id === 27) ||
      (table.kind === 'magic_recipe' && record.normal_catalogue === false)) return 'normal_catalogue_excluded';
  if (record.non_directory_sources?.some(entry => entry.level === 'static_visual_consumer')) return 'visual_contract';
  if (table.kind === 'facility' && [1, 4, 5, 7, 10, 11].includes(record.kind)) return 'instance_contract';
  if (table.kind === 'facility' && record.kind === 12) return 'residence_contract';
  if (record.source_limit && !record.unlock_sources?.length) return 'source_unresolved';
  return 'player_content';
}
function dependencies(record) {
  const result = new Set();
  const add = (kind, id) => {
    const key = `${kind}:${id}`;
    need(catalogue.has(key), 'unresolved source reference ' + key);
    result.add(key);
  };
  for (const entry of record.unlock_sources ?? []) {
    if (entry.source) add(entry.source.table, entry.source.id);
    if (entry.table) add(entry.table, entry.definition);
    for (const [field, kind] of [['bonus', 'popularity_bonus'], ['event', 'event'],
      ['facility', 'facility'], ['recipe', 'magic_recipe'], ['quest', 'quest'], ['previous', 'monster']])
      if (entry[field] !== undefined) add(kind, entry[field]);
    if (entry.rank !== undefined && ['activity_rank', 'commerce85_rank'].includes(entry.route) && entry.rank > 0)
      add('rank', entry.rank - 1);
    for (const id of entry.human_candidates ?? []) add('human', id);
  }
  return [...result];
}
function report(rows) {
  return {
    source: coverageSource, index_schema: coverageIndex.version, definitions: 775,
    scope: 'Source definitions and segment evidence only; no natural reachability or full-campaign certificate',
    contract_groups: Object.fromEntries(coverageIndex.tables.map(table => [table.kind,
      Object.fromEntries(['chain_contract', 'effect_contract', 'persistence_contract', 'instance_contract',
        'caller_contract', 'use_effect_contract', 'appearance_contract']
        .filter(field => table[field] !== undefined).map(field => [field, table[field]]))])),
    version_differences: coverageIndex.version_differences,
    event_source_groups: eventSources.shared,
    records: [...catalogue].map(([key, {table, record}]) => ({
      key, source_id: record.id, runtime_id: rows.get(key)?.id, name: record.name_zh,
      scope: scope(table, record), source: record.source, contract: table.chain_contract, contract_group: table.kind,
      unlock_sources: record.unlock_sources ?? [], dependencies: dependencies(record),
      non_directory_sources: record.non_directory_sources,
      event_type: record.event_type, caller_refs: table.kind === 'event' ? eventSources.callers.get(record.id) : undefined,
      source_limit: record.source_limit ?? table.callers_status,
      observed: [], verified_receipts: {}, required_evidence: rows.get(key)?.required_evidence ??
        (table.kind === 'event' && ['table_reference_only', 'unresolved'].includes(eventSources.callers.get(record.id).category)
          ? 'resolve_caller_gap;actual_trigger_and_effect_if_reachable;successor;applicable_persistence'
          : ['event', 'popularity_bonus'].includes(table.kind)
            ? 'source_contract;actual_trigger_and_effect_if_reachable;successor;applicable_persistence' : undefined)
    })),
    product_checklist: [...rows].filter(([key]) => !catalogue.has(key)).map(([, row]) => row)
  };
}
// Receipt values describe the already-asserted observation; no business algorithm is run here.
const receiptStages = {
  points_paid: 'village_activity', effect_queued: 'village_activity', pot_opened: 'village_activity', bound_use: 'item',
  attribute_applied: 'item', obtained: 'item', hp_restored: 'item', discovered: 'magic_recipe',
  elements_paid_and_crafted: 'magic_recipe', blueprint_points_paid: 'facility',
  blueprint_claimed: 'facility', construction_paid: 'facility',
  reward_equipped: ['weapon', 'armour', 'accessory'],
  combat_attributes_applied: ['weapon', 'armour', 'accessory'], medal_awarded: 'human', home_admitted: 'human',
  home_tax_collected: 'human', retirement_cash_committed: 'monster', retirement_cash_month_reset: 'monster',
  victory_committed: 'quest', commerce_bought: 'item', facility_bound_use: 'item',
  item_improvement_applied: 'facility', task_loot_claimed: ['item', 'weapon', 'armour', 'accessory'],
  conditions_promoted: 'rank'
};
function* receipts(text, phase) {
  for (const [line, content] of text.split(/\r?\n/).entries()) {
    if (!content.startsWith('COVERAGE_RECEIPT\t')) continue;
    const fields = content.split('\t');
    need(fields.length === 16 && fields.slice(4).every(value => /^-?\d+$/.test(value)) && /^\d+$/.test(fields[2]),
      'receipt boundary/integers');
    const [category, id, stage] = [aliases[fields[1]] ?? fields[1], Number(fields[2]), fields[3]];
    const key = sourceKey(fields[1], id); // Display ranks1..5 map to source terms0..4.
    const numbers = fields.slice(4).map(Number);
    need(numbers.every(Number.isSafeInteger) && numbers[0] >= 0 && numbers[1] > 0 && numbers[3] >= 0 &&
      [].concat(receiptStages[stage] ?? []).includes(category) && catalogue.has(key), 'receipt identity/binding');
    if (['bound_use', 'attribute_applied', 'hp_restored'].includes(stage)) need(numbers[2] >= 0, 'receipt human binding');
    if (['reward_equipped', 'combat_attributes_applied', 'medal_awarded', 'home_admitted', 'home_tax_collected'].includes(stage))
      need(numbers[2] >= 0 && numbers[2] < 25, 'receipt source human binding');
    if (['hp_restored', 'construction_paid', 'reward_equipped', 'combat_attributes_applied', 'home_admitted', 'victory_committed', 'facility_bound_use', 'item_improvement_applied', 'task_loot_claimed', 'retirement_cash_committed', 'retirement_cash_month_reset'].includes(stage))
      need(numbers[3] > 0, 'receipt actual subject binding');
    if (['medal_awarded', 'home_admitted', 'home_tax_collected'].includes(stage)) need(id === numbers[2], 'receipt human definition binding');
    if (stage === 'home_tax_collected')
      need(numbers[7] >= 0 && numbers[7] < 12 && numbers[7] === numbers[11] && numbers[9] === 0, 'receipt tax month/reset binding');
    if (['facility_bound_use', 'item_improvement_applied', 'task_loot_claimed'].includes(stage))
      need(numbers[2] === -1 && numbers[7] === numbers[11] &&
        catalogue.has(`${stage === 'task_loot_claimed' ? 'quest' : stage === 'facility_bound_use' ? 'facility' : 'item'}:${numbers[7]}`),
      'receipt related definition binding');
    if (stage === 'task_loot_claimed') need(numbers[6] > 0 && numbers[6] === numbers[10], 'receipt direct grant count');
    if (stage === 'retirement_cash_committed' || stage === 'retirement_cash_month_reset')
      need(numbers[2] === -1 && [4, 5, 7, 8, 9].every(n => numbers[n] >= 0 && numbers[n] <= 2147483647) &&
        numbers[6] === 0 && numbers[10] >= 0 && numbers[7] > 0 && numbers[7] === numbers[11] &&
        numbers[8] === (stage === 'retirement_cash_month_reset' ? 0 : numbers[4] + numbers[7]) &&
        numbers[9] === numbers[5] + numbers[7],
      'receipt retirement cash/counters/batch');
    if (stage === 'conditions_promoted')
      need(id >= 1 && id <= 5 && numbers[2] === -1 && numbers[3] === 0 &&
        numbers[4] === id - 1 && numbers[8] === id && numbers[5] === 15 && numbers[9] === 15 &&
        [numbers[6], numbers[7], numbers[10], numbers[11]].every(value => value >= 0),
        'receipt rank transition/four cached conditions');
    yield {key, stage, phase, line: line + 1,
      step: numbers[0], page: numbers[1], recipient: numbers[2], subject: numbers[3],
      before: numbers.slice(4, 8), after: numbers.slice(8, 12)};
  }
}
function coverageContract(rows) {
  const actual = report(rows), get = key => actual.records.find(row => row.key === key);
  need(actual.records.length === 775 && get('human:24')?.runtime_id === 24 &&
    get('rank:0')?.runtime_id === 1 && get('rank:4')?.runtime_id === 5, 'human/rank mapping');
  need(get('village_activity:27').scope === 'normal_catalogue_excluded' &&
    get('magic_recipe:38').scope === 'normal_catalogue_excluded' &&
    get('facility:58').scope === 'source_unresolved' && get('facility:0').scope === 'instance_contract',
    'finite exclusions incorrectly erased or labelled unreachable');
  need(get('village_activity:30').dependencies.includes('event:43') &&
    get('human:0').dependencies.includes('popularity_bonus:0') &&
    get('magic_recipe:7').verified_receipts.elements_paid_and_crafted === undefined &&
    get('item:29').observed.length === 0, 'source links inferred historical consumption');
  need(actual.index_schema === 8 && actual.contract_groups.item.effect_contract.includes('#逐id效果') &&
    actual.contract_groups.human.persistence_contract.includes('#人物职业与活动') &&
    actual.contract_groups.profession.persistence_contract === actual.contract_groups.human.persistence_contract &&
    actual.contract_groups.facility.instance_contract.includes('#85定义'),
    'shared effects/persistence/instance contracts');
  need(actual.contract_groups.quest.persistence_contract === '../rules/PERSISTENCE.md#任务与怪物共享定义当前任务和已抽结果' &&
    actual.contract_groups.quest.persistence_contract === actual.contract_groups.monster.persistence_contract &&
    actual.contract_groups.magic_recipe.persistence_contract === '../rules/MAGIC_POT.md' &&
    actual.contract_groups.popularity_bonus.chain_contract === '../rules/ACCOUNTING.md#popularity-rewards' &&
    actual.contract_groups.popularity_bonus.persistence_contract === actual.contract_groups.popularity_bonus.chain_contract &&
    actual.contract_groups.facility.use_effect_contract === '../rules/FACILITY_USE.md#definition-exit-effects' &&
    actual.contract_groups.human.appearance_contract === '../rules/STARTUP.md#25人物共用的自然到访重访与住宅资格' &&
    actual.contract_groups.profession.effect_contract === '../rules/CHARACTERS.md#23职业的转入精通及魔法后继',
    'task/reward/recipe persistence and arrival/career/facility effect contracts');
  const surface = actual.event_source_groups.script_surface;
  need(surface.programs === 552 && surface.nonempty_programs === 465 && surface.instructions === 835 &&
    surface.literal_nested_calls === 81 && surface.nested_targets.join(',') === '77' &&
    surface.nested_evidence.includes('不是运行时调用数') &&
    surface.parameterized_nested_targets === 0 && surface.used_opcodes.join(',') ===
      '0,1,2,3,4,5,6,7,12,13,14,15,16,17,18,21,22,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40',
    'compact script surface');
  const arrival = actual.version_differences[0];
  need(actual.version_differences.length === 1 && arrival.feature === 'simultaneous_arrival_limit' &&
    arrival.apk.soft === 20 && arrival.apk.hard === 20 && arrival.steam.value === 25 &&
    arrival.steam.basis === 'character_definition_directory_length' &&
    arrival.contract === actual.contract_groups.human.appearance_contract, 'APK/Steam arrival distinction');
  need(get('facility:23').scope === 'visual_contract' &&
    get('facility:23').non_directory_sources[0].route === 'sky_mapchip_flag2' &&
    get('facility:25').scope === 'residence_contract' &&
    get('facility:25').unlock_sources[0].route === 'residence_demolition_return', 'non-directory and residence sources');
  need(get('event:7').caller_refs.category === 'automatic' &&
    get('event:77').caller_refs.category === 'table_reference_only' &&
    get('event:77').required_evidence.startsWith('resolve_caller_gap') &&
    typeof get('event:77').source_limit === 'string' &&
    get('event:217').caller_refs.category === 'literal' &&
    get('event:96').caller_refs.category === 'unresolved' &&
    get('event:96').required_evidence.startsWith('resolve_caller_gap') &&
    get('event:96').event_type === 0 && !('flags' in get('event:96')) &&
    actual.event_source_groups.table_references[77].length === 1 &&
    actual.event_source_groups.table_references[77][0].definitions.join(',') === '0,80' &&
    actual.event_source_groups.table_references[77][0].evidence === 'table_reference_only' &&
    actual.event_source_groups.counts.table_reference_only === 1 && actual.event_source_groups.counts.unresolved === 34 &&
    actual.event_source_groups.known_entry_count === 165 && actual.event_source_groups.runtime_unresolved_count === 35 &&
    actual.event_source_groups.runtime_unresolved.includes(77) &&
    actual.records.filter(row => row.key.startsWith('event:') && row.required_evidence.startsWith('resolve_caller_gap')).length === 35,
    'event type/source groups/range/caller gaps');
  const expectReject = action => { let rejected = false; try { action(); } catch { rejected = true; }
    need(rejected, 'invalid coverage input was accepted'); };
  const duplicate = {...coverageIndex, tables: coverageIndex.tables.map(table => ({...table, records: [...table.records]}))};
  duplicate.tables[0].records[1] = duplicate.tables[0].records[0];
  expectReject(() => indexRecords(duplicate));
  expectReject(() => indexRecords({...coverageIndex, version: 2}));
  for (const table of coverageIndex.tables) {
    for (const field of ['chain_contract', ...(sharedContracts[table.kind] ?? [])]) {
      const missing = {...coverageIndex, tables: coverageIndex.tables.map(value => ({...value}))};
      delete missing.tables.find(value => value.kind === table.kind)[field];
      expectReject(() => indexRecords(missing));
    }
  }
  expectReject(() => indexRecords({...coverageIndex, version_differences: []}));
  const mixedArrival = {...coverageIndex, version_differences: coverageIndex.version_differences.map(value =>
    ({...value, apk: {...value.apk, hard: 25}}))};
  expectReject(() => indexRecords(mixedArrival));
  for (const field of ['nonempty_programs', 'used_opcodes', 'nested_evidence']) {
    const missing = {...coverageIndex, tables: coverageIndex.tables.map(table => ({...table}))};
    const event = missing.tables.find(table => table.kind === 'event');
    event.script_surface = {...event.script_surface};
    delete event.script_surface[field];
    expectReject(() => eventCallers(missing, indexRecords(missing)));
  }
  for (const change of [caller => { delete caller.evidence; },
    caller => { caller.evidence = 'runtime_call'; }, caller => { caller.definitions = [0, 81]; }]) {
    const invalid = {...coverageIndex, tables: coverageIndex.tables.map(table => ({...table, records: [...table.records]}))};
    const event = invalid.tables.find(table => table.kind === 'event');
    const position = event.records.findIndex(record => record.id === 77);
    event.records[position] = {...event.records[position],
      script_callers: event.records[position].script_callers.map(caller => ({...caller}))};
    change(event.records[position].script_callers[0]);
    expectReject(() => eventCallers(invalid, indexRecords(invalid)));
  }
  const badGroups = {...coverageIndex, tables: coverageIndex.tables.map(table => ({...table}))};
  badGroups.tables.find(table => table.kind === 'event').unresolved_callers = [7,
    ...badGroups.tables.find(table => table.kind === 'event').unresolved_callers];
  expectReject(() => eventCallers(badGroups, indexRecords(badGroups)));
  const missingGroup = {...coverageIndex, tables: coverageIndex.tables.map(table => ({...table}))};
  missingGroup.tables.find(table => table.kind === 'event').unresolved_callers =
    missingGroup.tables.find(table => table.kind === 'event').unresolved_callers.slice(1);
  expectReject(() => eventCallers(missingGroup, indexRecords(missingGroup)));
  const header = 'category\tid\tname\tsource_kind\trank_hint\tobserved_in_segment\tverified_receipts\trequired_evidence';
  expectReject(() => inventory(header + '\nitem\t29\tmedicine\t0\t0\tUNOBSERVED\tUNVERIFIED\tuse'));
  const queued = [...receipts('COVERAGE_RECEIPT\tactivity\t16\teffect_queued\t0\t1\t-1\t0\t0\t0\t0\t0\t10\t20\t1\t1', 'contract')];
  need(queued.length === 1 && queued[0].stage === 'effect_queued', 'queued effect promoted to actual effect');
  for (const [category, stage, id, limit, subject, recipient] of [
    ['facility', 'blueprint_points_paid', 40, 85, 0, -1], ['facility', 'blueprint_claimed', 40, 85, 0, -1],
    ['facility', 'construction_paid', 40, 85, 1, -1], ['activity', 'pot_opened', 30, 31, 0, -1],
    ['weapon', 'reward_equipped', 32, 33, 1, 24], ['armour', 'reward_equipped', 49, 50, 1, 24],
    ['accessory', 'reward_equipped', 29, 30, 1, 24], ['weapon', 'combat_attributes_applied', 32, 33, 1, 24],
    ['armour', 'combat_attributes_applied', 49, 50, 1, 24], ['accessory', 'combat_attributes_applied', 29, 30, 1, 24],
    ['human', 'medal_awarded', 24, 25, 0, 24], ['human', 'home_admitted', 24, 25, 1, 24],
    ['human', 'home_tax_collected', 24, 25, 0, 24],
    ['task', 'victory_committed', 80, 81, 1, -1], ['item', 'commerce_bought', 35, 36, 0, -1],
    ['item', 'facility_bound_use', 35, 36, 1, -1],
    ['facility', 'item_improvement_applied', 84, 85, 1, -1],
    ['item', 'task_loot_claimed', 35, 36, 1, -1], ['weapon', 'task_loot_claimed', 32, 33, 1, -1],
    ['armour', 'task_loot_claimed', 49, 50, 1, -1], ['accessory', 'task_loot_claimed', 29, 30, 1, -1]]) {
    const fields = ['COVERAGE_RECEIPT', category, String(id), stage, '2', '1', String(recipient), String(subject),
      '120', '0', '2', '0', '0', '1', '2', '0'];
    if (stage === 'home_tax_collected') fields[13] = '0';
    const parsed = [...receipts(fields.join('\t'), 'contract')];
    need(parsed.length === 1 && parsed[0].key === sourceKey(category, id) && parsed[0].stage === stage &&
      parsed[0].subject === subject && parsed[0].recipient === recipient && parsed[0].before.join(',') === '120,0,2,0' &&
      parsed[0].after.join(',') === (stage === 'home_tax_collected' ? '0,0,2,0' : '0,1,2,0'), 'campaign numeric receipt mapping');
    const last = [...fields]; last[2] = String(limit - 1);
    need([...receipts(last.join('\t'), 'contract')].length === 1, 'last source definition receipt rejected');
    for (const [field, value] of [[2, '-1'], [2, String(limit)], [5, '0'],
      [1, category === 'facility' ? 'activity' : 'facility'], ...(subject ? [[7, '0']] : []),
      ...(recipient >= 0 ? [[6, '-1'], [6, '25']] : []), ...(category === 'human' ? [[6, '23']] : [])]) {
      const invalid = [...fields]; invalid[field] = value;
      expectReject(() => [...receipts(invalid.join('\t'), 'contract')]);
    }
    if (recipient >= 0) {
      const first = [...fields]; first[6] = '0';
      if (category === 'human') first[2] = '0';
      need([...receipts(first.join('\t'), 'contract')].length === 1, 'first source person receipt rejected');
    }
    if (['facility_bound_use', 'item_improvement_applied', 'task_loot_claimed'].includes(stage)) {
      const relatedLimit = stage === 'task_loot_claimed' ? 81 : stage === 'facility_bound_use' ? 85 : 36;
      for (const related of [-1, relatedLimit]) {
        const invalid = [...fields]; invalid[11] = invalid[15] = String(related);
        expectReject(() => [...receipts(invalid.join('\t'), 'contract')]);
      }
      const lastRelated = [...fields]; lastRelated[11] = lastRelated[15] = String(relatedLimit - 1);
      need([...receipts(lastRelated.join('\t'), 'contract')].length === 1, 'last related definition rejected');
      for (const [field, value] of [[15, '1'], [6, '0']]) {
        const invalid = [...fields]; invalid[field] = value;
        expectReject(() => [...receipts(invalid.join('\t'), 'contract')]);
      }
    }
    if (stage === 'task_loot_claimed')
      for (const [field, value] of [[10, '0'], [14, '1']]) {
        const invalid = [...fields]; invalid[field] = value;
        expectReject(() => [...receipts(invalid.join('\t'), 'contract')]);
      }
    if (stage === 'home_tax_collected') {
      for (const month of [-1, 12]) {
        const invalid = [...fields]; invalid[11] = invalid[15] = String(month);
        expectReject(() => [...receipts(invalid.join('\t'), 'contract')]);
      }
      for (const [field, value] of [[15, '1'], [13, '1']]) {
        const invalid = [...fields]; invalid[field] = value;
        expectReject(() => [...receipts(invalid.join('\t'), 'contract')]);
      }
    }
  }
  expectReject(() => [...receipts('COVERAGE_RECEIPT\titem\t29\thp_restored\t0\t0\t1\t1\t1\t0\t0\t0\t51\t0\t0\t0', 'contract')]);
  const retired = 'COVERAGE_RECEIPT\tmonster\t35\tretirement_cash_committed\t2\t1\t-1\t7\t0\t99\t0\t1\t1\t100\t120\t1';
  for (const rank of [1, 3, 5]) {
    const fields = ['COVERAGE_RECEIPT', 'rank', String(rank), 'conditions_promoted', '2', '1', '-1', '0',
      String(rank - 1), '15', '35000', '1500', String(rank), '15', '35000', '1490'];
    const parsed = [...receipts(fields.join('\t'), 'contract')];
    need(parsed.length === 1 && parsed[0].key === `rank:${rank - 1}`,
      'promoted display rank must map to the correct source term');
    for (const [field, value] of [[2, '0'], [2, '6'], [5, '0'], [6, '0'], [7, '1'],
      [8, '9'], [9, '7'], [10, '-1'], [11, '-1'], [12, '9'], [13, '7'], [14, '-1'], [15, '-1']]) {
      const invalid = [...fields]; invalid[field] = value;
      expectReject(() => [...receipts(invalid.join('\t'), 'contract')]);
    }
  }
  need([...receipts(retired, 'contract')].length === 1, 'bound retirement receipt rejected');
  const reset = retired.split('\t'); reset[3] = 'retirement_cash_month_reset'; reset[12] = '0';
  need([...receipts(reset.join('\t'), 'contract')].length === 1, 'monthly reset lost actual cash/growth');
  reset[12] = '1';
  expectReject(() => [...receipts(reset.join('\t'), 'contract')]);
  for (const [field, value] of [[2, '36'], [5, '0'], [6, '0'], [7, '0'], [8, '-1'],
    [9, '2147483648'], [10, '1'], [11, '0'], [12, '0'], [13, '99'], [14, '-1'], [15, '2']]) {
    const invalid = retired.split('\t'); invalid[field] = value;
    expectReject(() => [...receipts(invalid.join('\t'), 'contract')]);
  }
  const validReceipt = 'COVERAGE_RECEIPT\titem\t29\thp_restored\t0\t1\t1\t1\t1\t0\t0\t0\t51\t0\t0\t0';
  need([...receipts(validReceipt, 'contract')].length === 1, 'actual receipt rejected');
  for (const [field, value] of [[1, 'activity'], [2, '9999'], [3, 'effect_queued'], [4, '-1'],
    [5, '0'], [6, '-1'], [7, '0'], [8, '9007199254740992']]) {
    const invalid = validReceipt.split('\t'); invalid[field] = value;
    expectReject(() => [...receipts(invalid.join('\t'), 'contract')]);
  }
}
async function captureCoverage(directory, phases, initialRows) {
  const result = report(initialRows), byKey = new Map(result.records.map(row => [row.key, row]));
  for (const name of (await fs.readdir(directory)).filter(name => /^coverage-segment-[0-9]+\.tsv$/.test(name))) {
    const rows = inventory(await fs.readFile(path.join(directory, name), 'utf8'));
    for (const [key, row] of rows) if (byKey.has(key) && row.observed_in_segment !== 'UNOBSERVED')
      byKey.get(key).observed = [...new Set([...byKey.get(key).observed, ...row.observed_in_segment.split(';').filter(Boolean)])];
  }
  for (const phase of phases.filter(value => !value.reused)) {
    for (const receipt of receipts(await fs.readFile(path.join(directory, `${phase.phase}.log`), 'utf8'), phase.phase)) {
      const row = byKey.get(receipt.key), previous = row.verified_receipts[receipt.stage];
      // Keep a bounded per-definition/stage summary; every original numeric receipt stays in its phase log.
      row.verified_receipts[receipt.stage] = {count: (previous?.count ?? 0) + 1,
        first: previous?.first ?? {phase: receipt.phase, line: receipt.line}, last: receipt,
        phase_status: phase.code === 0 && !phase.timedOut ? 'passed' : 'failed'};
    }
  }
  result.cold_endpoint_checks = phases.filter(phase => ['verify', 'verify-pot', 'verify-late', 'verify-third'].includes(phase.phase))
    .map(phase => ({phase: phase.phase, passed: phase.code === 0 && !phase.timedOut}));
  await fs.writeFile(path.join(directory, 'coverage.json'), JSON.stringify(result, null, 2) + '\n', {flag: 'wx'});
  return result;
}
const contractOnly = supplied === '--contract';
if (contractOnly) {
  need(process.argv.length === 4, 'contract arguments');
  await runFile(exe, ['--contract'], {windowsHide: true, timeout: 30000, maxBuffer: 8 * 1024 * 1024});
  const output = await runFile(exe, ['--coverage-inventory'], {windowsHide: true, timeout: 30000, maxBuffer: 1024 * 1024});
  coverageContract(inventory(output.stdout));
  console.log('PASS campaign source/receipt contract: 932fa45 schema8, 775 definitions, 165 known event entries and 35 caller gaps');
  process.exit(0);
}
// These are captured before launching any phase, separately from the executable's own
// loaded-DLL guard. A different binary/source manifest cannot continue this certificate.
const executablePath = await fs.realpath(path.resolve(exe));
const sourceManifestPath = path.join(root, 'assets/simulation/SOURCES.json');
const libraryContractPath = path.join(build, 'shared-libraries/ArkLibraryContract.json');
const libraryInputsPath = path.join(build, 'shared-libraries/ArkLibraryInputs.json');
const executableBytes = await fs.readFile(executablePath);
const sourceManifestBytes = await fs.readFile(sourceManifestPath);
const libraryContractBytes = await fs.readFile(libraryContractPath);
const libraryInputsBytes = await fs.readFile(libraryInputsPath);
const sourceManifest = JSON.parse(sourceManifestBytes.toString());
need(typeof sourceManifest.research_commit === 'string' && /^[0-9a-f]{7,40}$/.test(sourceManifest.research_commit),
  'runtime manifest formal research commit');
const libraryContract = JSON.parse(libraryContractBytes.toString());
const libraryInputs = JSON.parse(libraryInputsBytes.toString());
verifyLibraries(root, libraryInputs, libraryContract);
const initialInventory = inventory((await runFile(executablePath, ['--coverage-inventory'],
  {windowsHide: true, timeout: 30000, maxBuffer: 1024 * 1024})).stdout);
const execution = {
  executable: executablePath, executable_bytes: executableBytes.length,
  executable_sha256: sha256(executableBytes),
  research_commit: sourceManifest.research_commit,
  source_manifest_sha256: sha256(sourceManifestBytes),
  library_identity: libraryContract.identity,
  library_contract_sha256: sha256(libraryContractBytes),
  library_inputs_sha256: sha256(libraryInputsBytes)
};
async function verifyExecution() {
  for (const [file, expected] of [[executablePath, executableBytes],
    [sourceManifestPath, sourceManifestBytes], [libraryContractPath, libraryContractBytes],
    [libraryInputsPath, libraryInputsBytes]])
    if (!(await fs.readFile(file)).equals(expected)) throw Error(`Execution identity changed: ${file}`);
  verifyLibraries(root, libraryInputs, libraryContract);
}
const third = option === '--third-star-prefix';
const lateResume = option === '--late-resume-prefix';
const late = option === '--first-star-prefix' || lateResume;
const pot = option === '--second-star-prefix';
const firstStarPrefix = late && !lateResume;
const endpointPrefix = firstStarPrefix || pot || third;
const firstQualification = 'active_player_first_star_business_checkpoint_restart';
const secondQualification = 'active_player_second_star_from_verified_first_star_restart';
const qualification = third ? 'active_player_third_star_from_verified_second_star_restart' : pot ? 'active_player_magic_pot_from_verified_second_star_restart'
  : late ? secondQualification : firstQualification;
const firstMarker = 'ark-active-first-star-v1\n';
const secondMarker = 'ark-active-second-star-v1\n';
const marker = third ? 'ark-active-third-star-v1\n' : pot ? 'ark-active-magic-pot-v1\n' : late ? secondMarker : firstMarker;
const sourceMarker = pot || third || lateResume ? secondMarker : firstMarker;
let prefix;
let prefixFiles;
// Validate the finished first process before creating the new run. Never mutate or resume
// in the old evidence directory, and never transplant its failed resume world's state.
if (prefixArgument) {
  const source = await fs.realpath(path.resolve(prefixArgument));
  if (!inside(source) || source === build) throw Error('Prefix must stay strictly below real build');
  const read = async name => {
    const file = path.join(source, name);
    if (path.dirname(await fs.realpath(file)) !== source)
      throw Error(`Prefix file escapes its evidence directory: ${name}`);
    return fs.readFile(file);
  };
  if ((await read('ACTIVE_CAMPAIGN')).toString() !== sourceMarker)
    throw Error('Wrong prefix marker');
  const resultBytes = await read('RESULT.json');
  const result = JSON.parse(resultBytes.toString());
  if (result.qualification !== (pot || third || lateResume ? secondQualification : firstQualification) ||
      !(endpointPrefix ? result.status === 'passed' : ['passed', 'failed'].includes(result.status)))
    throw Error('Prefix is not a completed active-business run');
  if (!result.execution || result.execution.research_commit !== execution.research_commit ||
      result.execution.source_manifest_sha256 !== execution.source_manifest_sha256 ||
      result.execution.library_identity !== execution.library_identity)
    throw Error('Prefix execution source/library identity differs from current certified runtime');
  // Pot acceptance requires the full successful second-star chain, including a separate
  // cold-file verifier. A passed label or a saved date alone cannot qualify an input.
  if (pot || third) {
    for (const name of ['late-new', 'late-resume', 'verify-late']) {
      const matches = result.phases?.filter(phase => phase.phase === name) ?? [];
      if (matches.length !== 1 || matches[0].code !== 0 || matches[0].timedOut || matches[0].reused)
        throw Error(`Second-star prefix lacks its original successful ${name} process`);
    }
  }
  const newPhases = result.phases?.filter(phase => phase.phase ===
    (pot || third ? 'late-resume' : lateResume ? 'late-new' : firstStarPrefix ? 'resume' : 'new')) ?? [];
  if (newPhases.length !== 1 || newPhases[0].code !== 0 || newPhases[0].timedOut || newPhases[0].reused)
    throw Error('Prefix requires its original successful new process, not a reused or failed phase');
  prefixFiles = new Map();
  const inputs = [];
  const slotName = endpointPrefix ? 'manual-2.ark' : 'manual-1.ark';
  const strategyName = endpointPrefix ? 'strategy1.txt' : 'strategy0.txt';
  const logName = pot || third ? 'late-resume.log' : lateResume ? 'late-new.log' : firstStarPrefix ? 'resume.log' : 'new.log';
  for (const name of [slotName, strategyName, 'system.arksys', logName,
    ...(endpointPrefix ? ['manual-1.ark', 'strategy0.txt'] : []),
    ...(pot || third ? ['late-new.log', 'verify-late.log'] : [])]) {
    const expected = result.files?.filter(file => file.name === name) ?? [];
    const content = await read(name);
    const digest = sha256(content);
    if (expected.length !== 1 || expected[0].bytes !== content.length || expected[0].sha256 !== digest)
      throw Error(`Prefix evidence hash differs: ${name}`);
    inputs.push({ name, bytes: content.length, sha256: digest });
    if (!name.endsWith('.log')) prefixFiles.set(name, content);
    else if (name === logName && !(endpointPrefix ? /^SAVE slot=1 bytes=\d+ /m : /^SAVE slot=0 bytes=\d+ /m).test(content.toString()))
      throw Error('Successful new phase has no recorded business checkpoint save');
  }
  const [sidecarMagic, boundDigest] = prefixFiles.get(strategyName).toString().split(/\r?\n/);
  if (sidecarMagic !== (pot || third || lateResume ? 'ARK_ACTIVE_LATE_CHECKPOINT_1' : 'ARK_ACTIVE_CHECKPOINT_1') ||
      boundDigest !== sha256(prefixFiles.get(slotName)))
    throw Error('Prefix strategy evidence is not bound to the exact player checkpoint');
  prefix = {
    directory: source, resultSha256: sha256(resultBytes), sourceStatus: result.status,
    qualification: result.qualification, newPhase: newPhases[0], inputs,
    scope: third ? 'Completed current-identity second-star endpoint revalidated by cold-loading both player slots before the real restaurant, third-star and school route' : pot ? 'Completed second-star endpoint with three successful original processes; the new process revalidates both cold player slots before moving the sole Owner into the pot route'
      : lateResume ? 'Reuse only the original successful late-new player checkpoint; no failed late-resume world progress is inherited or certified. The source RESULT links its verified historical first-star input.'
      : firstStarPrefix ? 'Historical first-star endpoint only; this does not certify the prefix as a new-game rerun under the current source revision'
      : 'Reuse only the successful new-process player checkpoint; failed resume progress is not inherited or certified',
    systemPolicy: endpointPrefix ? 'Copy the hash-verified completed business system record; source files stay untouched'
      : 'Use the verified current system file; it may retain cash records written by a failed resume, not the original new-stage system bytes'
  };
}
const parent = path.resolve(supplied);
if (!inside(parent) || parent === build) throw Error('Campaign must stay strictly below product build');
let ancestor = parent;
for (;;) {
  try { ancestor = await fs.realpath(ancestor); break; }
  catch (error) { if (error.code !== 'ENOENT') throw error; ancestor = path.dirname(ancestor); }
}
if (!inside(ancestor)) throw Error('Campaign ancestor escapes build');
await fs.mkdir(parent, { recursive: true });
if (!inside(await fs.realpath(parent))) throw Error('Campaign parent escapes build');
const directory = path.join(parent, `${third ? 'third-star-player-' : pot ? 'magic-pot-player-' : late ? 'second-star-player-' : 'active-player-'}${randomUUID()}`);
await fs.mkdir(directory); // Exclusive creation with inherited workspace ACLs.
await fs.writeFile(path.join(directory, 'ACTIVE_CAMPAIGN'), marker, { flag: 'wx' });
if (endpointPrefix) await fs.mkdir(path.join(directory, 'input'));
if (prefixFiles)
  for (const [name, content] of prefixFiles)
    await fs.writeFile(path.join(directory, ...(endpointPrefix ? ['input'] : []), name), content, { flag: 'wx' });
// Only the new run's system record is writable. The input copy is hash-checked again at exit.
if (endpointPrefix)
  await fs.writeFile(path.join(directory, 'system.arksys'), prefixFiles.get('system.arksys'), { flag: 'wx' });
let child;
let cancelled = false;
const cancel = () => { cancelled = true; child?.kill(); };
process.on('SIGINT', cancel); process.on('SIGTERM', cancel);
const phases = prefix && !endpointPrefix ? [{ ...prefix.newPhase, reused: true, source: prefix.directory }] : [];
let failure;
try {
  for (const phase of third ? ['third-new', 'third-resume', 'verify-third'] : pot ? ['pot-new', 'pot-resume', 'verify-pot'] : lateResume ? ['late-resume', 'verify-late'] : late ? ['late-new', 'late-resume', 'verify-late'] : prefix ? ['resume', 'verify'] : ['new', 'resume', 'verify']) {
    if (cancelled) throw Error('Campaign cancelled');
    await verifyExecution();
    const started = Date.now();
    const output = [];
    console.log(`PHASE ${phase} directory=${directory}`);
    await new Promise((resolve, reject) => {
      child = spawn(executablePath, [phase, directory], { windowsHide: true });
      let timedOut = false;
      const timer = setTimeout(() => { timedOut = true; child.kill(); }, 100 * 60 * 1000);
      child.stdout.on('data', bytes => { output.push(bytes); process.stdout.write(bytes); });
      child.stderr.on('data', bytes => { output.push(bytes); process.stderr.write(bytes); });
      child.on('error', error => { clearTimeout(timer); reject(error); });
      // close, unlike exit, guarantees all captured output pipes have drained.
      child.on('close', code => {
        clearTimeout(timer); child = undefined;
        phases.push({ phase, milliseconds: Date.now() - started, code, timedOut });
        code === 0 && !timedOut && !cancelled ? resolve()
          : reject(Error(`${phase} failed: exit=${code} timeout=${timedOut} cancelled=${cancelled}`));
      });
    }).finally(async () => { await fs.writeFile(path.join(directory, `${phase}.log`), Buffer.concat(output)); });
  }
} catch (error) {
  failure = error;
} finally {
  process.off('SIGINT', cancel); process.off('SIGTERM', cancel);
  try { await verifyExecution(); } catch (error) { failure ??= error; }
  if (late || pot || third) {
    for (const [name, expected] of prefixFiles) {
      try {
        // A resumed run legitimately updates its own system cash record. Its player
        // checkpoint remains immutable; all original prefix files remain immutable.
        if (endpointPrefix || name !== 'system.arksys') {
          const actual = await fs.readFile(path.join(directory, ...(endpointPrefix ? ['input'] : []), name));
          if (!actual.equals(expected)) throw Error(`Read-only prefix input changed: ${name}`);
        }
        const original = await fs.readFile(path.join(prefix.directory, name));
        if (!original.equals(expected)) throw Error(`Original prefix changed: ${name}`);
      } catch (error) { failure ??= error; }
    }
    // Keep the source audit chain stable too, including the successful phase log and
    // the RESULT that links late-new to its originally verified first-star inputs.
    try {
      for (const expected of prefix.inputs) {
        const content = await fs.readFile(path.join(prefix.directory, expected.name));
        if (content.length !== expected.bytes || sha256(content) !== expected.sha256)
          throw Error(`Original prefix evidence changed: ${expected.name}`);
      }
      if (sha256(await fs.readFile(path.join(prefix.directory, 'RESULT.json'))) !== prefix.resultSha256)
        throw Error('Original prefix RESULT changed');
      if ((await fs.readFile(path.join(prefix.directory, 'ACTIVE_CAMPAIGN'))).toString() !==
          sourceMarker)
        throw Error('Original prefix marker changed');
    } catch (error) { failure ??= error; }
  }
  let coverage;
  try { coverage = await captureCoverage(directory, phases, initialInventory); }
  catch (error) { failure ??= error; }
  const files = [];
  for (const name of (await fs.readdir(directory)).filter(name => /\.(ark|arksys|txt|log|tsv)$/.test(name) || name === 'coverage.json')) {
    const content = await fs.readFile(path.join(directory, name));
    files.push({ name, bytes: content.length, sha256: sha256(content) });
  }
  await fs.writeFile(path.join(directory, 'RESULT.json'), JSON.stringify({
    status: failure ? 'failed' : 'passed',
    qualification,
    policy: third ? 'Current-identity second-star prefix; real restaurant trade, task victory, stable save/restart, genuine third-star conditions, paid school construction and school activity 7, full subsequent calendar month trade; normal speed; cold-load fresh random' : pot ? 'Verified second-star prefix; real item deposit, stable player save/restart, natural date processing, discovery, paid production, resulting item consumption and continued trade; normal speed; cold-load fresh random'
      : late ? 'Verified historical first-star prefix; real construction, residence admission, new task victory, stable save/restart, four second-star conditions, paid activity 30 and continued trade; normal speed; cold-load fresh random'
      : 'P1 real construction, cultivation, activities, task victory, promotion, exhibition, continued trade; normal speed; cold-load fresh random',
    phases, files, prefix, execution, coverage_source: coverageSource,
    coverage_definitions: coverage?.definitions, error: failure?.message,
    exclusions: ['five-star/final-boss/date-clear/inheritance route', 'OS input', 'whole-route threaded FIFO', 'maintenance exact replay']
  }, null, 2) + '\n');
  console.log(`Campaign evidence: ${directory}`);
}
if (failure) throw failure;
console.log(`PASS active ${third ? 'third-star' : pot ? 'magic-pot' : late ? 'second-star' : 'first-star'} player campaign ${directory}`);
