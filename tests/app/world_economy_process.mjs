// Five isolated, bounded source-consumer comparisons; at most two processes run together.
// These worlds never write player/system saves. Preserve all evidence, including failures.
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';
const [executable, supplied] = process.argv.slice(2);
if (!executable || !supplied || process.argv.length !== 4)
  throw Error('Expected executable and evidence parent below product build');
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const build = await fs.realpath(path.join(root, 'build'));
const parent = path.resolve(supplied);
const inside = candidate => {
  const relative = path.relative(build, candidate);
  return relative !== '' && !relative.startsWith('..') && !path.isAbsolute(relative);
};
if (!inside(parent)) throw Error('Economy evidence must stay below product build');
let ancestor = parent;
for (;;) {
  try { ancestor = await fs.realpath(ancestor); break; }
  catch (error) { if (error.code !== 'ENOENT') throw error; ancestor = path.dirname(ancestor); }
}
if (ancestor !== build && !inside(ancestor)) throw Error('Evidence ancestor escapes build');
await fs.mkdir(parent, { recursive: true });
if (!inside(await fs.realpath(parent))) throw Error('Evidence path escapes build');
const directory = await fs.mkdtemp(path.join(parent, 'compare-'));
console.log(`Evidence directory: ${directory}`);
const live = new Set();
let cancelled = false, next = 0;
const results = [], failures = [];
const cancel = () => { cancelled = true; for (const child of live) child.kill(); };
process.on('SIGINT', cancel); process.on('SIGTERM', cancel);
async function run(plan) {
  const started = Date.now(), output = [];
  let timedOut = false;
  try {
    await new Promise((resolve, reject) => {
      const child = spawn(path.resolve(executable), [plan], { windowsHide: true });
      live.add(child);
      const timer = setTimeout(() => { timedOut = true; child.kill(); }, 600000);
      child.stdout.on('data', bytes => { output.push(bytes); process.stdout.write(`[${plan}] ${bytes}`); });
      child.stderr.on('data', bytes => { output.push(bytes); process.stderr.write(`[${plan}] ${bytes}`); });
      child.on('error', error => { clearTimeout(timer); live.delete(child); reject(error); });
      child.on('close', code => {
        clearTimeout(timer); live.delete(child);
        if (code === 0 && !timedOut && !cancelled) resolve();
        else reject(Error(`${plan}: exit=${code} timeout=${timedOut} cancelled=${cancelled}`));
      });
    });
    const text = Buffer.concat(output).toString('utf8');
    const line = text.split(/\r?\n/).find(value => value.startsWith('RESULT '));
    if (!line) throw Error(`${plan}: missing structured result`);
    const result = JSON.parse(line.slice(7));
    if (result.plan !== plan || result.start_month !== 3 || result.end_month !== 6)
      throw Error(`${plan}: unexpected experiment identity/horizon`);
    results.push({ ...result, milliseconds: Date.now() - started });
  } catch (error) { failures.push({ plan, message: error.message, milliseconds: Date.now() - started }); }
  finally { await fs.writeFile(path.join(directory, `${plan}.log`), Buffer.concat(output)); }
}
try {
  const worker = async () => {
    while (!cancelled && next < 5) { const plan = `P${next++}`; await run(plan); }
  };
  await Promise.all([worker(), worker()]);
  results.sort((a, b) => a.plan.localeCompare(b.plan));
  const ranking = [...results].sort((a, b) => b.net_cash - a.net_cash).map(r => ({ plan: r.plan, net_cash: r.net_cash }));
  await fs.writeFile(path.join(directory, 'RESULT.json'), JSON.stringify({
    qualification: 'seed1_three_month_source_consumer_construction_economy_comparison',
    policy: 'real construction and ordinary page input; normal simulation speed; no file saves',
    exclusions: ['active campaign', 'star progression', 'global optimum', 'OS input'],
    results, ranking, failures, cancelled
  }, null, 2) + '\n');
  if (failures.length || cancelled || results.length !== 5) process.exitCode = 1;
  console.log(`${process.exitCode ? 'FAIL' : 'PASS'} five-plan comparison: ${directory}`);
} finally {
  process.off('SIGINT', cancel); process.off('SIGTERM', cancel);
}
