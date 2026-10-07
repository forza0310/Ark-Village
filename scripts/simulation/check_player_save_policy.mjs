// Player ARKSAVE1 policy audit. Reuse the maintained AST walk; this tool never encodes a save.
import { readFileSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const productName = value => value.replace(/\bdungeon_village_reference\b/g, 'ark::simulation::rules')
    .replace(/\bdungeon_village_prototype\b/g, 'ark::simulation');
const codecName = value => value.replace(/^r::/, 'ark::simulation::rules::')
    .replace(/^s::/, 'ark::simulation::');

// Read only the explicit per-declaration visit lists. Macro argument order remains a separate
// byte-protocol contract: this audit checks inclusion, not serialization order or algorithms.
export function codecFields(source) {
    const text = source.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
    const records = new Map();
    let ledgerBody;
    const register = (type, body) => {
        if (type === 'Type') return; // Macro definition, not an instantiated declaration.
        const name = codecName(type);
        if (records.has(name)) throw Error('duplicate player codec declaration: ' + name);
        records.set(name, [...new Set([...body.matchAll(/\bx\.([A-Za-z_]\w*)/g)].map(m => m[1]))]);
    };
    for (const match of text.matchAll(/ARK_SAVE_FIELDS\s*\(\s*([\w:]+)\s*,([^)]*)\)/g))
        register(match[1], match[2]);
    for (const match of text.matchAll(/void\s+fields\s*\(Archive\s*&io,\s*([\w:]+)\s*&x\)\s*\{/g)) {
        let depth = 1, end = match.index + match[0].length;
        const begin = end;
        while (depth && end < text.length) {
            if (text[end] === '{') ++depth;
            else if (text[end] === '}') --depth;
            ++end;
        }
        if (depth) throw Error('unterminated player codec declaration: ' + match[1]);
        const body = text.slice(begin, end - 1);
        register(match[1], body);
        if (codecName(match[1]) === 'ark::simulation::rules::PeriodAccounting') ledgerBody = body;
    }
    // The private ledger wrapper is encoded through public accessors, without its history.
    if (!records.has('ark::simulation::rules::PeriodAccounting'))
        throw Error('player ledger adapter is missing');
    const ledgerContract = 'std::int64_t funds = x.funds(); std::uint16_t points = x.village_points(); '
        + 'io(funds, points); if constexpr (Archive::reading) x = r::PeriodAccounting(funds, points);';
    if (ledgerBody?.replace(/\s+/g, '') !== ledgerContract.replace(/\s+/g, ''))
        throw Error('player ledger adapter changed; review its explicit classification');
    records.set('ark::simulation::rules::PeriodAccounting', ['funds_', 'village_points_']);
    return records;
}

export function checkPolicy(inventory, policy, codec) {
    if (policy.format !== 'ARKSAVE1' || policy.schema !== 2 || policy.policy_version !== 1)
        throw Error('unsupported player classification policy');
    const actual = new Map([...inventory.records, ...inventory.wrappers].map(record =>
        [productName(record.name), record.fields.map(field => ({name: field.name, type: productName(field.type)}))]));
    const declared = new Map();
    const counts = {save: 0, inherit: 0, rebuild: 0, discard: 0};
    for (const record of policy.records) {
        if (declared.has(record.name)) throw Error('duplicate policy record: ' + record.name);
        if (!record.reason || !record.groups) throw Error('policy record lacks scope/reason: ' + record.name);
        const fields = new Map();
        for (const group of record.groups) {
            if (!Object.hasOwn(counts, group.action) || !group.reason)
                throw Error('unknown classification or missing reason: ' + record.name);
            for (const [name, type] of group.fields) {
                if (fields.has(name)) throw Error('duplicate policy field: ' + record.name + '.' + name);
                if (typeof name !== 'string' || typeof type !== 'string' || !type)
                    throw Error('invalid policy field: ' + record.name);
                fields.set(name, {type, action: group.action});
                ++counts[group.action];
            }
        }
        if (!Array.isArray(record.wire_fields) || new Set(record.wire_fields).size !== record.wire_fields.length)
            throw Error('duplicate or missing wire declaration: ' + record.name);
        for (const name of record.wire_fields)
            if (!fields.has(name)) throw Error('wire field has no policy: ' + record.name + '.' + name);
        for (const [name, field] of fields)
            if (field.action === 'save' && !record.wire_fields.includes(name))
                throw Error('saved field is absent from wire declaration: ' + record.name + '.' + name);
        for (const [name, field] of fields)
            if (field.action === 'discard' && record.wire_fields.includes(name))
                throw Error('discarded field entered player wire declaration: ' + record.name + '.' + name);
        declared.set(record.name, {fields, wire: record.wire_fields});
    }
    for (const [name, fields] of actual) {
        const record = declared.get(name);
        if (!record) throw Error('unclassified owner record: ' + name);
        for (const field of fields) {
            const classified = record.fields.get(field.name);
            if (!classified) throw Error('unclassified owner field: ' + name + '.' + field.name);
            if (classified.type !== field.type) throw Error('player policy field type changed: ' + name + '.' + field.name);
        }
        for (const field of record.fields.keys())
            if (!fields.some(actualField => actualField.name === field))
                throw Error('stale player policy field: ' + name + '.' + field);
        const expected = codec.get(name) ?? [];
        if (expected.length !== record.wire.length || expected.some(field => !record.wire.includes(field)))
            throw Error('player codec/policy inclusion differs: ' + name);
    }
    for (const name of declared.keys()) if (!actual.has(name)) throw Error('stale player policy record: ' + name);
    for (const name of codec.keys()) if (!actual.has(name)) throw Error('player codec type is outside inventory: ' + name);
    return {records: actual.size, fields: Object.values(counts).reduce((a,b) => a+b, 0), counts};
}

function refusalChecks(inventory, policy, codec) {
    const cases = [
        ['top-level field', (i) => i.records.find(r => r.name.endsWith('::StartupWorldRuntimeState')).fields.push({name: 'future_owner_field', type: 'int'}), 'unclassified owner field'],
        ['nested field', (i) => i.records.find(r => r.name.endsWith('::CharacterHpState')).fields.push({name: 'future_hp_field', type: 'int'}), 'unclassified owner field'],
        ['private wrapper field', (i) => i.wrappers[0].fields.push({name: 'future_random_field', type: 'int'}), 'unclassified owner field'],
        ['changed field type', (i) => {i.records.find(r => r.name.endsWith('::CharacterHpState')).fields[0].type = 'double';}, 'field type changed'],
        ['duplicate classification', (_i,p) => p.records[0].groups[0].fields.push(p.records[0].groups[0].fields[0]), 'duplicate policy field'],
        ['unknown classification', (_i,p) => {p.records[0].groups[0].action = 'guess';}, 'unknown classification'],
        ['codec drift', (_i,_p,c) => {const key = 'ark::simulation::rules::CharacterHpState'; c.set(key, c.get(key).slice(1));}, 'codec/policy inclusion differs'],
    ];
    for (const [label, mutate, expected] of cases) {
        const i = structuredClone(inventory), p = structuredClone(policy);
        const c = new Map([...codec].map(([key, value]) => [key, [...value]]));
        mutate(i,p,c);
        let rejected = false;
        try { checkPolicy(i,p,c); }
        catch (error) { if (!error.message.includes(expected)) throw error; rejected = true; }
        if (!rejected) throw Error('policy guard accepted ' + label);
    }
    return cases.length;
}

function main() {
    const args = process.argv.slice(2);
    const get = key => args[args.indexOf(key) + 1];
    for (const key of ['--root', '--ast'])
        if (!args.includes(key) || !get(key) || get(key).startsWith('--')) throw Error('missing ' + key);
    const root = path.resolve(get('--root')), ast = path.resolve(get('--ast'));
    const inventoryFile = ast + '.player-inventory.json';
    const parameters = [path.join(root, 'scripts/simulation/generate_owner_codec.mjs'), '--root', root,
        '--ast', ast, '--inventory-only', inventoryFile];
    if (args.includes('--compiler')) parameters.push('--compiler', get('--compiler'));
    const result = spawnSync(process.execPath, parameters, {encoding: 'utf8', maxBuffer: 512 * 1024});
    if (result.error || result.status !== 0) throw Error(result.error?.message ?? result.stderr);
    const inventory = JSON.parse(readFileSync(inventoryFile, 'utf8'));
    const policy = JSON.parse(readFileSync(path.join(root, 'src/app/world_save_policy.json'), 'utf8'));
    const codec = codecFields(readFileSync(path.join(root, 'src/app/world_save_fields.hpp'), 'utf8'));
    const checked = checkPolicy(inventory, policy, codec);
    const refusals = args.includes('--self-test') ? refusalChecks(inventory, policy, codec) : 0;
    console.log('Player save policy: ' + JSON.stringify({...checked, refusal_checks: refusals}));
}
if (process.argv[1] === fileURLToPath(import.meta.url)) main();
