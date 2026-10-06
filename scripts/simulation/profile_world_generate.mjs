// Measurement-only copies under build. Frozen inputs and normal builds are untouched.
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
const scriptDir = dirname(fileURLToPath(import.meta.url));
const root = resolve(scriptDir, '../..');
const out = resolve(root, 'build/validation/world-performance/instrumented');
mkdirSync(out, { recursive: true });
for (const name of ['world_runtime', 'world_scene', 'world_schedule', 'world_actor_schedule', 'world_nonactor_schedule']) {
  const path = `ark/simulation/rules/${name}.hpp`;
  let count = 0;
  let source = readFileSync(resolve(root, 'include', path), 'utf8').replace(
    /Owner (next|scratch) = (state|current);/g, (_, local, original) => {
      ++count;
      return `Owner ${local} = profile_copy(${original});`;
    });
  if (name === 'world_runtime') {
    // The published runtime delegates its old calendar entry to the call-level consumer.
    const calendar = source.includes('prepare_owned_world_runtime_calendar_with_consumer(')
      ? 'prepare_owned_world_runtime_calendar_with_consumer'
      : 'prepare_owned_world_runtime_calendar';
    for (const [functionName, metric] of [['prepare_owned_world_runtime_domain', 11],
                                        [calendar, 13]]) {
      const start = source.indexOf(functionName + '(');
      const body = source.indexOf('{', start);
      if (start < 0 || body < 0) throw new Error(`Missing ${functionName}`);
      source = source.slice(0, body + 1) + `\n    ProfileScope profile_scope{${metric}};` + source.slice(body + 1);
    }
  }
  const target = resolve(out, path);
  mkdirSync(dirname(target), { recursive: true });
  writeFileSync(target, source);
  console.log(`${name}: ${count} explicit Owner copy sites instrumented`);
}
const source = readFileSync(resolve(root, 'src/simulation/startup_world_runtime.cpp'), 'utf8');
const start = source.indexOf('StartupWorldRuntimeResult prepare_startup_world_runtime(const State &s) {');
const end = source.indexOf('StartupWorldRuntimeResult StartupWorldRuntimeSession::update()', start);
if (start < 0 || end < 0) throw new Error('Runtime function boundaries changed');
let fn = source.slice(start, end).replace('prepare_startup_world_runtime(const State &s)', 'profile_prepare(const State &s)');
const cached = fn.includes('static const auto a = startup_world_runtime_adapter();');
const adapterAnchor = cached ? 'static const auto a = startup_world_runtime_adapter();'
  : 'auto a = startup_world_runtime_adapter();';
if (fn.split(adapterAnchor).length !== 2)
  throw new Error('Expected exactly one runtime adapter instrumentation anchor');
// Wrap only the diagnostic adapter during initialization; the production cached object stays
// const, and this preserves its one-construction lifetime in the instrumented entry as well.
fn = fn.replace(adapterAnchor, `${cached ? 'static const' : 'const'} auto a = [] {
    ProfileScope profile_scope{12};
    auto instrumented = startup_world_runtime_adapter();
    profile_wrap(instrumented.scene.read, 1); profile_wrap(instrumented.scene.write, 2);
    profile_wrap(instrumented.scripts.read, 3); profile_wrap(instrumented.scripts.write, 4);
    profile_wrap(instrumented.actors.read_routes, 5); profile_wrap(instrumented.actors.write_routes, 6);
    profile_wrap(instrumented.actors.decision, 7); profile_wrap(instrumented.scene_other, 8);
    profile_wrap(instrumented.before_common, 9); profile_wrap(instrumented.normal_conditions, 10);
    return instrumented;
  }();`);
const metrics = readFileSync(resolve(scriptDir, 'profile_world_metrics.hpp'), 'utf8');
writeFileSync(resolve(out, 'profile_prepare.hpp'), metrics + source.slice(0, source.indexOf('namespace ark::simulation {')) +
  '\nnamespace ark::simulation {\nusing State = StartupWorldRuntimeState;\n' + fn + '}\n');
console.log(out);
