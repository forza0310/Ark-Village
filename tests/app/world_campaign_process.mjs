// Two cold processes join real player files at a business milestone, not a date target.
// Synchronous product-command acceptance is not OS input or full-route FIFO coverage.
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';
import { createHash } from 'node:crypto';
const [exe, supplied, option, prefixArgument] = process.argv.slice(2);
if (!exe || !supplied || !(process.argv.length === 4 ||
    (process.argv.length === 6 && ['--resume-prefix', '--first-star-prefix', '--late-resume-prefix'].includes(option) && prefixArgument)))
  throw Error('Expected executable and build work parent [--resume-prefix completed-new-directory | --first-star-prefix passed-first-star-directory | --late-resume-prefix completed-late-new-directory]');
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const build = await fs.realpath(path.join(root, 'build'));
const inside = candidate => {
  const relative = path.relative(build, candidate);
  return !path.isAbsolute(relative) && relative !== '..' && !relative.startsWith(`..${path.sep}`);
};
const sha256 = content => createHash('sha256').update(content).digest('hex');
const lateResume = option === '--late-resume-prefix';
const late = option === '--first-star-prefix' || lateResume;
const firstStarPrefix = late && !lateResume;
const firstQualification = 'active_player_first_star_business_checkpoint_restart';
const qualification = late ? 'active_player_second_star_from_verified_first_star_restart' : firstQualification;
const firstMarker = 'ark-active-first-star-v1\n';
const marker = late ? 'ark-active-second-star-v1\n' : firstMarker;
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
  if ((await read('ACTIVE_CAMPAIGN')).toString() !== (lateResume ? marker : firstMarker))
    throw Error('Wrong prefix marker');
  const resultBytes = await read('RESULT.json');
  const result = JSON.parse(resultBytes.toString());
  if (result.qualification !== (lateResume ? qualification : firstQualification) ||
      !(firstStarPrefix ? result.status === 'passed' : ['passed', 'failed'].includes(result.status)))
    throw Error('Prefix is not a completed active-business run');
  const newPhases = result.phases?.filter(phase => phase.phase ===
    (lateResume ? 'late-new' : firstStarPrefix ? 'resume' : 'new')) ?? [];
  if (newPhases.length !== 1 || newPhases[0].code !== 0 || newPhases[0].timedOut || newPhases[0].reused)
    throw Error('Prefix requires its original successful new process, not a reused or failed phase');
  prefixFiles = new Map();
  const inputs = [];
  const slotName = firstStarPrefix ? 'manual-2.ark' : 'manual-1.ark';
  const strategyName = firstStarPrefix ? 'strategy1.txt' : 'strategy0.txt';
  const logName = lateResume ? 'late-new.log' : firstStarPrefix ? 'resume.log' : 'new.log';
  for (const name of [slotName, strategyName, 'system.arksys', logName,
    ...(firstStarPrefix ? ['manual-1.ark', 'strategy0.txt'] : [])]) {
    const expected = result.files?.filter(file => file.name === name) ?? [];
    const content = await read(name);
    const digest = sha256(content);
    if (expected.length !== 1 || expected[0].bytes !== content.length || expected[0].sha256 !== digest)
      throw Error(`Prefix evidence hash differs: ${name}`);
    inputs.push({ name, bytes: content.length, sha256: digest });
    if (name !== logName) prefixFiles.set(name, content);
    else if (!(firstStarPrefix ? /^SAVE slot=1 bytes=\d+ /m : /^SAVE slot=0 bytes=\d+ /m).test(content.toString()))
      throw Error('Successful new phase has no recorded business checkpoint save');
  }
  const [sidecarMagic, boundDigest] = prefixFiles.get(strategyName).toString().split(/\r?\n/);
  if (sidecarMagic !== (lateResume ? 'ARK_ACTIVE_LATE_CHECKPOINT_1' : 'ARK_ACTIVE_CHECKPOINT_1') ||
      boundDigest !== sha256(prefixFiles.get(slotName)))
    throw Error('Prefix strategy evidence is not bound to the exact player checkpoint');
  prefix = {
    directory: source, resultSha256: sha256(resultBytes), sourceStatus: result.status,
    qualification: result.qualification, newPhase: newPhases[0], inputs,
    scope: lateResume ? 'Reuse only the original successful late-new player checkpoint; no failed late-resume world progress is inherited or certified. The source RESULT links its verified historical first-star input.'
      : firstStarPrefix ? 'Historical first-star endpoint only; this does not certify the prefix as a new-game rerun under the current source revision'
      : 'Reuse only the successful new-process player checkpoint; failed resume progress is not inherited or certified',
    systemPolicy: firstStarPrefix ? 'Copy the hash-verified completed first-star system record; source files stay untouched'
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
const directory = await fs.mkdtemp(path.join(parent, late ? 'second-star-player-' : 'active-player-'));
await fs.writeFile(path.join(directory, 'ACTIVE_CAMPAIGN'), marker, { flag: 'wx' });
if (firstStarPrefix) await fs.mkdir(path.join(directory, 'input'));
if (prefixFiles)
  for (const [name, content] of prefixFiles)
    await fs.writeFile(path.join(directory, ...(firstStarPrefix ? ['input'] : []), name), content, { flag: 'wx' });
// Only the new run's system record is writable. The input copy is hash-checked again at exit.
if (firstStarPrefix)
  await fs.writeFile(path.join(directory, 'system.arksys'), prefixFiles.get('system.arksys'), { flag: 'wx' });
let child;
let cancelled = false;
const cancel = () => { cancelled = true; child?.kill(); };
process.on('SIGINT', cancel); process.on('SIGTERM', cancel);
const phases = prefix && !firstStarPrefix ? [{ ...prefix.newPhase, reused: true, source: prefix.directory }] : [];
let failure;
try {
  for (const phase of lateResume ? ['late-resume', 'verify-late'] : late ? ['late-new', 'late-resume', 'verify-late'] : prefix ? ['resume'] : ['new', 'resume']) {
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
  if (late) {
    for (const [name, expected] of prefixFiles) {
      try {
        // A resumed run legitimately updates its own system cash record. Its player
        // checkpoint remains immutable; all original prefix files remain immutable.
        if (firstStarPrefix || name !== 'system.arksys') {
          const actual = await fs.readFile(path.join(directory, ...(firstStarPrefix ? ['input'] : []), name));
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
          (lateResume ? marker : firstMarker))
        throw Error('Original prefix marker changed');
    } catch (error) { failure ??= error; }
  }
  const files = [];
  for (const name of (await fs.readdir(directory)).filter(name => /\.(ark|arksys|txt|log)$/.test(name))) {
    const content = await fs.readFile(path.join(directory, name));
    files.push({ name, bytes: content.length, sha256: sha256(content) });
  }
  await fs.writeFile(path.join(directory, 'RESULT.json'), JSON.stringify({
    status: failure ? 'failed' : 'passed',
    qualification,
    policy: late ? 'Verified historical first-star prefix; real construction, residence admission, new task victory, stable save/restart, four second-star conditions, paid activity 30 and continued trade; normal speed; cold-load fresh random'
      : 'P1 real construction, cultivation, activities, task victory, promotion, exhibition, continued trade; normal speed; cold-load fresh random',
    phases, files, prefix, error: failure?.message,
    exclusions: ['five-star/final-boss/date-clear/inheritance route', 'OS input', 'whole-route threaded FIFO', 'maintenance exact replay']
  }, null, 2) + '\n');
  console.log(`Campaign evidence: ${directory}`);
}
if (failure) throw failure;
console.log(`PASS active ${late ? 'second' : 'first'}-star player campaign ${directory}`);
