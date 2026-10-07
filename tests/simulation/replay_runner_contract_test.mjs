// Product runner interface tests, distinct from the frozen default progression oracle.
// Use the real C++ writer/loader for prefixes; do not emulate AVRSAVE1 or its identity parser.
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';

const runFile = promisify(execFile);
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const build = await fs.realpath(path.join(root, 'build'));
const [executableArgument, directoryArgument] = process.argv.slice(2);
if (!executableArgument || !directoryArgument || process.argv.length !== 4)
    throw Error('Expected continuous executable and product build test directory');
const executable = path.resolve(executableArgument);
const directory = await fs.realpath(directoryArgument);
const inside = file => {
    const relative = path.relative(build, file);
    return relative && !relative.startsWith('..') && !path.isAbsolute(relative);
};
assert(inside(directory), 'Test working directory must be an existing product build child');
const owned = await fs.mkdtemp(path.join(directory, 'runner-contract-'));
const work = path.join(owned, 'work');
const periodic = path.join(owned, 'periodic');
const snapshot = path.join(owned, 'expansion.awr');
const script = path.join(root, 'tests/simulation/replay_file_test.mjs');
const hash = bytes => createHash('sha256').update(bytes).digest('hex');
let checks = 0, passed = false;
async function run(args, expectedError) {
    let output;
    try {
        output = await runFile(process.execPath, [script, '--exe', executable, '--work-dir', work, ...args], {
            cwd: root, windowsHide: true, timeout: 180000, maxBuffer: 8 * 1024 * 1024,
        });
    } catch (error) {
        if (!expectedError) throw error;
        assert.notEqual(error.code, 0, 'Invalid invocation must fail');
        assert.match(error.stderr, expectedError, 'Reject the specific violated contract');
        ++checks;
        return;
    }
    assert.equal(expectedError, undefined, 'Invalid invocation unexpectedly succeeded');
    assert.equal(output.stderr, '', 'Valid runner has no error output');
    ++checks;
}
try {
    await run(['--scenario', 'natural_expansion', '--save-at', '420', '--stop-at', '440',
        '--save-every', '420', '--save-directory', periodic, '--snapshot-file', snapshot]);
    const certificate = JSON.parse(await fs.readFile(snapshot + '.json', 'utf8'));
    assert.equal(certificate.scenario, 'natural_expansion');
    assert.equal(certificate.process_count, 3);
    assert.equal(certificate.tail_frames, 20);
    assert.equal(certificate.certification_level, 'new_game_reference_tail');
    const original = await fs.readFile(snapshot);
    const originalCertificate = await fs.readFile(snapshot + '.json');
    const candidate = path.join(periodic, 'prefix-420.awr');
    const originalCandidate = await fs.readFile(candidate);
    assert.equal(certificate.snapshot_sha256, hash(original));

    const resumed = path.join(owned, 'resumed.awr');
    await run(['--scenario', 'natural_expansion', '--load-prefix', snapshot,
        '--save-at', '440', '--stop-at', '444', '--snapshot-file', resumed]);
    const resumedCertificate = JSON.parse(await fs.readFile(resumed + '.json', 'utf8'));
    assert.equal(resumedCertificate.certification_level, 'resumed_reference_tail');
    assert.equal(resumedCertificate.source_prefix.snapshot_sha256, hash(original));
    assert.equal(resumedCertificate.source_prefix.next_frame, 421);
    assert.equal(resumedCertificate.tail_frames, 4);

    const fromCandidate = path.join(owned, 'candidate-tail.awr');
    await run(['--scenario', 'natural_expansion', '--load-prefix', candidate,
        '--prefix-status', 'candidate', '--prefix-next-frame', '421',
        '--save-at', '440', '--stop-at', '444', '--snapshot-file', fromCandidate]);
    const candidateCertificate = JSON.parse(await fs.readFile(fromCandidate + '.json', 'utf8'));
    assert.equal(candidateCertificate.certification_level, 'candidate_reference_tail');
    assert.equal(candidateCertificate.source_prefix.source_status, 'candidate');
    assert.equal(candidateCertificate.source_prefix.snapshot_sha256, hash(originalCandidate));

    await run(['--scenario', 'unknown'], /未知回放场景/);
    await run(['--load-prefix', snapshot], /场景.*seed.*speed/);
    await run(['--scenario', 'natural_expansion', '--load-prefix', candidate,
        '--prefix-status', 'candidate'], /裸候选需要显式/);
    await run(['--scenario', 'natural_expansion', '--load-prefix', candidate,
        '--prefix-status', 'candidate', '--prefix-next-frame', '422', '--save-at', '440'], /声明与文件实际身份不符/);
    await run(['--scenario', 'natural_expansion', '--load-prefix', snapshot, '--save-at', '420'], /新capture必须位于源实际next_frame/);
    const invalid = path.join(owned, 'wrong-certificate.awr');
    await fs.writeFile(invalid, original);
    await fs.writeFile(invalid + '.json', JSON.stringify({...certificate, snapshot_sha256: '0'.repeat(64)}));
    await run(['--scenario', 'natural_expansion', '--load-prefix', invalid, '--save-at', '440'], /原证书资格不符/);
    const corrupted = Buffer.from(original);
    corrupted[corrupted.length - 1] ^= 1;
    await fs.writeFile(invalid, corrupted);
    await run(['--scenario', 'natural_expansion', '--load-prefix', invalid, '--save-at', '440'], /整体摘要不符/);

    // Preflight must reject product-root/research paths before writing or launching the game.
    await run(['--snapshot-file', path.join(root, 'forbidden-prefix.awr')], /必须位于产品build/);
    await run(['--save-every', '420', '--save-directory', path.join(root, 'research')], /必须位于产品build/);
    await run(['--load-prefix', path.join(root, 'README.md')], /源前缀必须是产品build/);
    const escape = path.join(owned, 'outside');
    await fs.symlink(root, escape, 'junction');
    await run(['--snapshot-file', path.join(escape, 'forbidden-prefix.awr')], /现存祖先位于产品build外/);
    await run(['--load-prefix', path.join(escape, 'README.md')], /源前缀必须是产品build/);
    await fs.unlink(escape);

    assert.deepEqual(await fs.readFile(snapshot), original, 'Certified source is read-only');
    assert.deepEqual(await fs.readFile(snapshot + '.json'), originalCertificate, 'Source certificate is read-only');
    assert.deepEqual(await fs.readFile(candidate), originalCandidate, 'Bare candidate is read-only');
    assert.deepEqual(await fs.readdir(work), [], 'Successful runner removes only its owned scratch directories');
    passed = true;
    console.log(`PASS replay runner: ${checks} real three-process/qualification/rejection scenarios`);
} finally {
    // Remove only our mkdtemp directory after success; leave failed evidence intact.
    const actual = await fs.realpath(owned);
    assert(inside(actual) && path.dirname(actual) === directory, 'Owned cleanup stayed in its original parent');
    if (passed) await fs.rm(actual, {recursive: true});
    else console.error(`Runner contract failure retained at ${owned}`);
}
