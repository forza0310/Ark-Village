// Two cold processes join real player files at a business milestone, not a date target.
// Synchronous product-command acceptance is not OS input or full-route FIFO coverage.
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';
import { createHash } from 'node:crypto';
const [exe, supplied, option, prefixArgument] = process.argv.slice(2);
if (!exe || !supplied || !(process.argv.length === 4 ||
    (process.argv.length === 6 && option === '--resume-prefix' && prefixArgument)))
  throw Error('Expected executable and build work parent [--resume-prefix completed-new-directory]');
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const build = await fs.realpath(path.join(root, 'build'));
const inside = candidate => {
  const relative = path.relative(build, candidate);
  return !path.isAbsolute(relative) && relative !== '..' && !relative.startsWith(`..${path.sep}`);
};
const sha256 = content => createHash('sha256').update(content).digest('hex');
const qualification = 'active_player_first_star_business_checkpoint_restart';
const marker = 'ark-active-first-star-v1\n';
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
  if ((await read('ACTIVE_CAMPAIGN')).toString() !== marker) throw Error('Wrong prefix marker');
  const resultBytes = await read('RESULT.json');
  const result = JSON.parse(resultBytes.toString());
  if (result.qualification !== qualification || !['passed', 'failed'].includes(result.status))
    throw Error('Prefix is not a completed active-business run');
  const newPhases = result.phases?.filter(phase => phase.phase === 'new') ?? [];
  if (newPhases.length !== 1 || newPhases[0].code !== 0 || newPhases[0].timedOut || newPhases[0].reused)
    throw Error('Prefix requires its original successful new process, not a reused or failed phase');
  prefixFiles = new Map();
  const inputs = [];
  for (const name of ['manual-1.ark', 'strategy0.txt', 'system.arksys', 'new.log']) {
    const expected = result.files?.filter(file => file.name === name) ?? [];
    const content = await read(name);
    const digest = sha256(content);
    if (expected.length !== 1 || expected[0].bytes !== content.length || expected[0].sha256 !== digest)
      throw Error(`Prefix evidence hash differs: ${name}`);
    inputs.push({ name, bytes: content.length, sha256: digest });
    if (name !== 'new.log') prefixFiles.set(name, content);
    else if (!/^SAVE slot=0 bytes=\d+ /m.test(content.toString()))
      throw Error('Successful new phase has no recorded business checkpoint save');
  }
  const [sidecarMagic, boundDigest] = prefixFiles.get('strategy0.txt').toString().split(/\r?\n/);
  if (sidecarMagic !== 'ARK_ACTIVE_CHECKPOINT_1' || boundDigest !== sha256(prefixFiles.get('manual-1.ark')))
    throw Error('Prefix strategy evidence is not bound to the exact player checkpoint');
  prefix = {
    directory: source, resultSha256: sha256(resultBytes), sourceStatus: result.status,
    qualification: result.qualification, newPhase: newPhases[0], inputs,
    scope: 'Reuse only the successful new-process player checkpoint; failed resume progress is not inherited or certified',
    systemPolicy: 'Use the verified current system file; it may retain cash records written by a failed resume, not the original new-stage system bytes'
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
const directory = await fs.mkdtemp(path.join(parent, 'active-player-'));
await fs.writeFile(path.join(directory, 'ACTIVE_CAMPAIGN'), marker, { flag: 'wx' });
if (prefixFiles)
  for (const [name, content] of prefixFiles)
    await fs.writeFile(path.join(directory, name), content, { flag: 'wx' });
let child;
let cancelled = false;
const cancel = () => { cancelled = true; child?.kill(); };
process.on('SIGINT', cancel); process.on('SIGTERM', cancel);
const phases = prefix ? [{ ...prefix.newPhase, reused: true, source: prefix.directory }] : [];
let failure;
try {
  for (const phase of prefix ? ['resume'] : ['new', 'resume']) {
    if (cancelled) throw Error('Campaign cancelled');
    const started = Date.now();
    const output = [];
    console.log(`PHASE ${phase} directory=${directory}`);
    await new Promise((resolve, reject) => {
      child = spawn(path.resolve(exe), [phase, directory], { windowsHide: true });
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
  const files = [];
  for (const name of (await fs.readdir(directory)).filter(name => /\.(ark|arksys|txt|log)$/.test(name))) {
    const content = await fs.readFile(path.join(directory, name));
    files.push({ name, bytes: content.length, sha256: sha256(content) });
  }
  await fs.writeFile(path.join(directory, 'RESULT.json'), JSON.stringify({
    status: failure ? 'failed' : 'passed',
    qualification,
    policy: 'P1 real construction, cultivation, activities, task victory, promotion, exhibition, continued trade; normal speed; cold-load fresh random',
    phases, files, prefix, error: failure?.message,
    exclusions: ['five-star/final-boss/date-clear/inheritance route', 'OS input', 'whole-route threaded FIFO', 'maintenance exact replay']
  }, null, 2) + '\n');
  console.log(`Campaign evidence: ${directory}`);
}
if (failure) throw failure;
console.log(`PASS active first-star player campaign ${directory}`);
