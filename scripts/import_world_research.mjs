// Explicit maintained-source import. Freeze and validate a complete snapshot before translation.
import { readFileSync, writeFileSync, readdirSync, existsSync, mkdirSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { dirname, join, resolve, basename } from 'node:path';

const [mode, sourceRoot, destinationRoot] = process.argv.slice(2);
if (!['--snapshot', '--import', '--record-patch'].includes(mode) || !sourceRoot || !destinationRoot)
  throw new Error('Expected --snapshot research snapshot, --import snapshot product, or --record-patch product file reason');
const source = resolve(sourceRoot), destination = resolve(destinationRoot);
const digest = bytes => createHash('sha256').update(bytes).digest('hex');
const put = (path, bytes) => {
  if (existsSync(path) && readFileSync(path).equals(Buffer.from(bytes))) return false;
  mkdirSync(dirname(path), { recursive: true });
  writeFileSync(path, bytes);
  return true;
};
const prototypeModules = ['startup', 'startup_map', 'startup_ai', 'facility_projection', 'loop_pacing',
  'startup_world_projection', 'startup_world_routes', 'startup_world_runtime',
  'startup_world_runtime_arrival', 'startup_world_runtime_calendar', 'startup_world_runtime_scene',
  'startup_world_runtime_focus', 'startup_world_runtime_pages', 'startup_world_runtime_tasks',
  'startup_world_runtime_task_pages', 'startup_world_runtime_deadline',
  'startup_world_runtime_nonactors', 'startup_world_building', 'startup_world_visuals',
  'startup_world_persistence'];
const prototypeTests = ['startup_world_projection', 'startup_world_routes', 'startup_world_scene',
  'startup_world_arrival', 'startup_world_runtime_tasks', 'startup_world_runtime',
  'startup_world_continuous', 'startup_world_pages', 'startup_world_runtime_nonactors',
  'startup_world_task_flow', 'startup_world_deadline', 'startup_world_building',
  'startup_world_visuals', 'startup_world_persistence'];
const translate = text => text.replaceAll('dungeon_village_tools/', 'ark/assets/')
  .replaceAll('dungeon_village_tools', 'ark::assets')
  .replaceAll('dungeon_village_reference/', 'ark/simulation/rules/')
  .replaceAll('dungeon_village_prototype/', 'ark/simulation/')
  .replaceAll('dungeon_village_reference', 'ark::simulation::rules')
  .replaceAll('dungeon_village_prototype', 'ark::simulation');

if (mode === '--record-patch') {
  const reason = process.argv[5];
  const kind = process.argv[6] ?? 'product_patch';
  if (!['product_patch', 'test_fixture_patch'].includes(kind)) throw new Error('Invalid patch kind');
  if (!reason) throw new Error('An evidence-backed patch reason is required');
  const manifestPath = join(source, 'assets/simulation/SOURCES.json');
  const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'));
  const entry = manifest.files.find(record => record.file === destinationRoot);
  if (!entry) throw new Error('Patch must refer to an already imported source');
  const bytes = readFileSync(join(source, entry.file));
  const sha256 = digest(bytes);
  if (sha256 === entry.sha256) throw new Error('Source has no unrecorded patch');
  entry[kind] = {
    imported_sha256: (entry.product_patch ?? entry.test_fixture_patch)?.imported_sha256 ?? entry.sha256,
    reason
  };
  entry.sha256 = sha256;
  entry.bytes = bytes.length;
  put(manifestPath, JSON.stringify(manifest, null, 2) + '\n');
  console.log(`Recorded product patch: ${entry.file}`);
} else if (mode === '--snapshot') {
  if (existsSync(join(destination, 'SNAPSHOT.json')))
    throw new Error('Refuse to replace an existing frozen snapshot');
  const files = new Set(), pending = prototypeModules.map(name => `prototype/src/${name}.cpp`);
  for (const name of prototypeTests) pending.push(`prototype/tests/${name}_test.cpp`);
  for (const name of readdirSync(join(source, 'example/tests')).sort()) {
    if (/^(world_|actor_|ai_|battle_|character_|combat_|encounter_|object_|rescue_|human_|weapon_|accounting|activity_|snapshot_facility_choice|geometry|map_access|navigation|neighbourhood|domain|facility_(arrival|departure|economy|exit|items|service|use))/.test(name) && name.endsWith('_test.cpp'))
      pending.push(`example/tests/${name}`);
  }
  while (pending.length) {
    const relative = pending.pop();
    if (files.has(relative)) continue;
    files.add(relative);
    const text = readFileSync(join(source, relative), 'utf8');
    for (const match of text.matchAll(/^#include "([^"]+)"/gm)) {
      const reference = match[1].startsWith('dungeon_village_reference/');
      const prototype = match[1].startsWith('dungeon_village_prototype/');
      const tools = match[1].startsWith('dungeon_village_tools/');
      if (tools && match[1] === 'dungeon_village_tools/archive.hpp') {
        // Persistence only needs portable SHA-256, not archive extraction or graphics.
        pending.push('tools/include/' + match[1], 'tools/src/sha256.cpp');
        continue;
      }
      if (!reference && !prototype) {
        // CPU portrait regression reuses the product's existing identical SEB/TSV API.
        // Keep the full test body; only its header/namespace and target dependency change.
        if (relative === 'prototype/tests/startup_world_visuals_test.cpp' &&
            ['dungeon_village_tools/sprite.hpp', 'dungeon_village_tools/table.hpp'].includes(match[1]))
          continue;
        // Maintained prototype regressions share a local fixture. Keep its relative path
        // and source identity; never resolve an arbitrary include outside the frozen tree.
        if (relative.startsWith('prototype/tests/') && /^support\/[A-Za-z0-9_]+\.hpp$/.test(match[1])) {
          pending.push(`prototype/tests/${match[1]}`);
          continue;
        }
        // Private maintained codec and test helpers stay beside their consumer.
        // Only a single safe filename can resolve here; no traversal outside the freeze.
        if (/^prototype\/(?:src|tests)\//.test(relative) &&
            /^[A-Za-z0-9_]+\.(?:hpp|inc)$/.test(match[1])) {
          let sibling = dirname(relative).replaceAll('\\', '/') + '/' + match[1];
          if (!existsSync(join(source, sibling)) && relative.startsWith('prototype/tests/') &&
              existsSync(join(source, 'prototype/src', match[1])))
            sibling = 'prototype/src/' + match[1];
          if (!existsSync(join(source, sibling)))
            throw new Error(`Missing local include: ${relative}: ${match[1]}`);
          pending.push(sibling);
          const implementation = sibling.replace(/\.hpp$/, '.cpp');
          if (implementation !== sibling && existsSync(join(source, implementation)))
            pending.push(implementation);
          if (sibling.endsWith('startup_world_codec_fields.inc'))
            pending.push('prototype/src/startup_world_codec_fields.json');
          continue;
        }
        throw new Error(`Unresolved local include: ${relative}: ${match[1]}`);
      }
      const packageName = reference ? 'example' : 'prototype';
      pending.push(`${packageName}/include/${match[1]}`);
      const implementation = `${packageName}/src/${basename(match[1], '.hpp')}.cpp`;
      if (existsSync(join(source, implementation))) pending.push(implementation);
    }
  }
  for (const file of ['MAP.json', 'STATE.json', 'TABLES.json', 'LOADED_MAP.tsv', 'LOADED_INSTANCES.tsv'])
    files.add(`data/startup/${file}`);
  files.add('data/original/tenantData.txt');
  for (const file of readdirSync(join(source, 'data/world')))
    if (file.endsWith('.txt') || file.endsWith('.tsv')) files.add(`data/world/${file}`);
  files.add('data/scripts/SOURCE.tsv');
  for (const file of readdirSync(join(source, 'data/scripts/original')))
    files.add(`data/scripts/original/${file}`);
  for (const file of ['compile_startup.mjs', 'compile_startup_world.mjs',
      'compile_persistence_identity.mjs', 'generate_owner_codec.mjs'])
    files.add(`prototype/scripts/${file}`);
  files.add('prototype/tests/startup_world_data_test.mjs');
  files.add('prototype/tests/replay_file_test.mjs');
  files.add('prototype/tests/support/README.md');
  const records = [...files].sort().map(file => {
    const bytes = readFileSync(join(source, file));
    put(join(destination, file), bytes);
    return { file, sha256: digest(bytes), bytes: bytes.length };
  });
  // Re-read every original after copying. No product import is permitted from a mixed snapshot.
  for (const entry of records)
    if (digest(readFileSync(join(source, entry.file))) !== entry.sha256)
      throw new Error(`Research changed while snapshotting: ${entry.file}; use a new snapshot directory`);
  put(join(destination, 'SNAPSHOT.json'), JSON.stringify({ schema: 1, files: records }, null, 2) + '\n');
  console.log(`Frozen ${records.length} maintained files at ${destination}`);
} else {
  const snapshotBytes = readFileSync(join(source, 'SNAPSHOT.json'));
  const snapshot = JSON.parse(snapshotBytes);
  const records = [];
  const changed = [];
  const supersedeFixtures = process.argv[5] === '--supersede-fixture-patches';
  if (process.argv[5] && !supersedeFixtures) throw new Error('Unknown import option');
  const previousPath = join(destination, 'assets/simulation/SOURCES.json');
  const previous = existsSync(previousPath) ? JSON.parse(readFileSync(previousPath, 'utf8')) : { files: [] };
  for (const entry of snapshot.files) {
    const original = readFileSync(join(source, entry.file));
    if (original.length !== entry.bytes || digest(original) !== entry.sha256)
      throw new Error(`Frozen source changed: ${entry.file}`);
    let target, content = original;
    if (entry.file.startsWith('example/include/dungeon_village_reference/'))
      target = 'include/ark/simulation/rules/' + basename(entry.file);
    else if (entry.file.startsWith('example/src/')) target = 'src/simulation/rules/' + basename(entry.file);
    else if (entry.file.startsWith('example/tests/')) target = 'tests/simulation/rules/' + basename(entry.file);
    else if (entry.file.startsWith('prototype/include/dungeon_village_prototype/'))
      target = 'include/ark/simulation/' + basename(entry.file);
    else if (entry.file.startsWith('prototype/src/')) target = 'src/simulation/' + basename(entry.file);
    else if (entry.file === 'tools/include/dungeon_village_tools/archive.hpp')
      target = 'src/assets/archive_source.hpp';
    else if (entry.file === 'tools/src/sha256.cpp') target = 'src/assets/sha256.cpp';
    else if (entry.file.startsWith('prototype/scripts/')) target = 'scripts/simulation/' + basename(entry.file);
    else if (entry.file.startsWith('prototype/tests/'))
      target = 'tests/simulation/' + entry.file.slice('prototype/tests/'.length);
    else if (entry.file === 'data/original/tenantData.txt') target = 'assets/simulation/tenantData.txt';
    else if (entry.file.startsWith('data/')) target = 'assets/simulation/' + entry.file.slice(5);
    else throw new Error(`Unsupported snapshot path: ${entry.file}`);
    // The canonical protocol manifest retains source logical names and exact bytes.
    // Namespace renaming changes C++ access spelling, not the persisted wire schema.
    if (!entry.file.startsWith('data/') && !entry.file.endsWith('startup_world_codec_fields.json')) {
      // Product source files are checked out as LF by .gitattributes. Keep their recorded
      // bytes stable across Windows checkouts; source_bytes/source_sha256 stay byte-exact.
      let text = translate(original.toString('utf8').replaceAll('\r\n', '\n'));
      // Keep the complete archive declaration as private frozen evidence; consumers
      // expose only the three portable digest overloads actually implemented here.
      text = text.replaceAll('#include "ark/assets/archive.hpp"',
        '#include "ark/assets/sha256.hpp"');
      if (target.endsWith('.mjs') && target.startsWith('tests/'))
        text = text.replaceAll("from '../scripts/", "from '../../scripts/simulation/");
      if (target.startsWith('tests/simulation/rules/'))
        text = text.replace(/std::filesystem::path\(__FILE__\)(?:\.parent_path\(\)){3}\s*\/\s*"data\/scripts\/original"/g,
          'std::filesystem::path(ARK_WORLD_TEST_DATA) / "scripts/original"')
          .replace('const auto root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();\n    std::istringstream table(read(root / "data/original/tenantData.txt"));',
            'std::istringstream table(read(std::filesystem::path(ARK_WORLD_TEST_DATA) / "tenantData.txt"));')
          .replace(/const std::string source = __FILE__;\s*const auto root =\s*source\.substr\([^;\n]+?\)\s*\+\s*"\/\.\.\/\.\.\/data\/scripts\/original\/";/g,
            'const auto root = std::string(ARK_WORLD_TEST_DATA) + "/scripts/original/";');
      content = Buffer.from(text);
    }
    const patched = previous.files.find(record => record.file === target && (record.product_patch || record.test_fixture_patch));
    if (patched && !(supersedeFixtures && patched.test_fixture_patch && patched.source_sha256 !== entry.sha256)) {
      const current = readFileSync(join(destination, target));
      const patch = patched.product_patch ?? patched.test_fixture_patch;
      if (entry.sha256 !== patched.source_sha256 || digest(content) !== patch.imported_sha256 || digest(current) !== patched.sha256)
        throw new Error(`Patch requires explicit rebasing before re-import: ${target}`);
      records.push(patched);
      continue;
    }
    if (put(join(destination, target), content)) changed.push(target);
    records.push({ file: target, source: 'research/dungeon_village_1/' + entry.file,
      source_sha256: entry.sha256, source_bytes: entry.bytes,
      sha256: digest(content), bytes: content.length,
      ...(original.toString('utf8').includes('#include "dungeon_village_tools/archive.hpp"')
        ? { include_adaptation: 'Use the implemented-only ark/assets/sha256.hpp interface; the complete source archive header remains frozen privately at src/assets/archive_source.hpp.' } : {}) });
  }
  put(join(destination, 'assets/simulation/SOURCES.json'), JSON.stringify({
    scope: 'maintained_complete_world_rules_and_single_runtime_owner',
    snapshot_sha256: digest(snapshotBytes), files: records
  }, null, 2) + '\n');
  const rules = records.filter(v => v.file.startsWith('src/simulation/rules/') && v.file.endsWith('.cpp'));
  const persistenceModules = new Set(['startup_world_codec.cpp', 'startup_world_file_io.cpp',
    'startup_world_persistence.cpp', 'startup_world_restore_validation.cpp']);
  const worldSources = records.filter(v => v.file.startsWith('src/simulation/') &&
    !v.file.startsWith('src/simulation/rules/') && v.file.endsWith('.cpp'));
  const runtime = worldSources.filter(v => !persistenceModules.has(basename(v.file)));
  const persistence = worldSources.filter(v => persistenceModules.has(basename(v.file)));
  const tests = records.filter(v => v.file.startsWith('tests/simulation/') && v.file.endsWith('_test.cpp'));
  const hashes = records.filter(v => v.file === 'src/assets/sha256.cpp');
  const continuousSupport = records.filter(v => v.file === 'tests/simulation/startup_world_replay_driver.cpp');
  const persistenceSupport = records.filter(v => /^tests\/simulation\/startup_world_(?:codec|restore)_checks\.cpp$/.test(v.file));
  let cmake = '# Explicit frozen-source inventory, generated by scripts/import_world_research.mjs.\n';
  cmake += 'set(ARK_WORLD_RULE_SOURCES\n' + rules.map(v => '    "${ARK_WORLD_ROOT}/' + v.file + '"').join('\n') + '\n)\n';
  cmake += 'set(ARK_WORLD_RUNTIME_SOURCES\n' + runtime.map(v => '    "${ARK_WORLD_ROOT}/' + v.file + '"').join('\n') + '\n)\n';
  cmake += 'set(ARK_WORLD_TEST_SOURCES\n' + tests.map(v => '    "${ARK_WORLD_ROOT}/' + v.file + '"').join('\n') + '\n)\n';
  for (const [name, entries] of [['ARK_WORLD_PERSISTENCE_SOURCES', persistence],
      ['ARK_WORLD_HASH_SOURCES', hashes],
      ['ARK_WORLD_CONTINUOUS_SUPPORT_SOURCES', continuousSupport],
      ['ARK_WORLD_PERSISTENCE_SUPPORT_SOURCES', persistenceSupport]])
    cmake += 'set(' + name + '\n' + entries.map(v => '    "${ARK_WORLD_ROOT}/' + v.file + '"').join('\n') + '\n)\n';
  put(join(destination, 'cmake/WorldSimulationSources.cmake'), cmake);
  console.log(`Imported ${records.length} files: ${rules.length} rule sources, ${runtime.length} runtime sources, ${persistence.length} persistence sources, ${tests.length} C++ regressions`);
  console.log(`Changed ${changed.length} product files:\n${changed.join('\n')}`);
}
