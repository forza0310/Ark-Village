// Exercise the executable's real CPU asset validation against isolated mutated copies.
import { mkdtempSync, readdirSync, cpSync, readFileSync, writeFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, dirname, basename } from 'node:path';
import { spawnSync } from 'node:child_process';
import assert from 'node:assert/strict';
const executable = process.argv[2];
const root = mkdtempSync(join(tmpdir(), 'ark-frames-'));
try {
  const target = join(root, basename(executable));
  cpSync(executable, target);
  // Relocation includes the shared runtime; the same resource mutations/assertions follow.
  for (const name of readdirSync(dirname(executable))) {
    if (/\.(dll|dylib|so(?:\.\d+)*)$/i.test(name))
      cpSync(join(dirname(executable), name), join(root, name));
  }
  cpSync(join(dirname(executable), 'assets'), join(root, 'assets'), {recursive:true});
  const file = join(root, 'assets/image/plain00.seb');
  const original = readFileSync(file);
  const run = () => spawnSync(target, ['--check'], {cwd:tmpdir(), encoding:'utf8', timeout:10000});
  assert.equal(run().status, 0);
  // Header 4 + layer header 4 + four 20-byte records. Extra frame3 is retained but never drawn.
  const unused = Buffer.from(original); unused.writeInt16BE(30000, 8 + 3*20 + 4);
  writeFileSync(file, unused); assert.equal(run().status, 0, 'unused record must remain structural metadata');
  const active = Buffer.from(original); active.writeInt16BE(30000, 8 + 4);
  writeFileSync(file, active);
  const failure = run(); assert.notEqual(failure.status, 0); assert.match(failure.stderr, /rectangle/);
  writeFileSync(file, original.subarray(0, original.length-1));
  assert.notEqual(run().status, 0, 'truncated structure must be rejected');
  writeFileSync(file, original);
  // All adjacency frames are reachable after construction, including ones absent at startup.
  const roadFile = join(root, 'assets/image/road00.seb');
  const road = readFileSync(roadFile);
  road.writeInt16BE(30000, 8 + 15*20 + 4);
  writeFileSync(roadFile, road);
  const roadFailure = run(); assert.notEqual(roadFailure.status, 0);
  assert.match(roadFailure.stderr, /rectangle/);
  writeFileSync(roadFile, readFileSync(join(dirname(executable), 'assets/image/road00.seb')));
  for (const name of ['walk00.seb', 'walk01.seb', 'walk02.seb', 'walk03.seb']) {
    const walkFile = join(root, 'assets/human', name);
    const originalWalk = readFileSync(walkFile);
    const walk = Buffer.from(originalWalk);
    // Every direction can request non-idle frames, not only the old walk00 first pose.
    walk.writeInt16BE(30000, 8 + 20 + 4);
    writeFileSync(walkFile, walk);
    const walkFailure = run(); assert.notEqual(walkFailure.status, 0, name);
    assert.match(walkFailure.stderr, /rectangle/);
    writeFileSync(walkFile, originalWalk);
  }
  // The published common bindings are explicit image IDs, independent of SEB row indices.
  const commonIndex = new Map(readFileSync(join(root, 'assets/common/img.inf'), 'utf8')
    .trimEnd().split(/\r?\n/).map(row => row.split('\t')));
  assert.equal(commonIndex.get('64'), 'fence01.gif');
  assert.equal(commonIndex.get('5'), 'door00.gif');
  for (const [name, frames, imageId] of [
    ['fence010.seb', 6, 64], ['fence011.seb', 6, 64], ['fence012.seb', 6, 64], ['door00.seb', 2, 5]]) {
    const bytes = readFileSync(join(root, 'assets/common', name));
    assert.equal(bytes.readInt16BE(0), 1, `${name}: single layer`);
    assert.equal(bytes.readInt16BE(2), frames, `${name}: frame count`);
    assert.equal(bytes.readInt16BE(4), frames, `${name}: record count`);
    assert.equal(bytes.length, 8 + frames*20, `${name}: complete records`);
    for (let frame = 0; frame < frames; ++frame) {
      assert.equal(bytes.readInt16BE(8 + frame*20), frame, `${name}: frame order`);
      assert.equal(bytes.readInt16BE(8 + frame*20 + 2), imageId, `${name}: common image ID`);
    }
  }
  // A later skin's tall corner and the second entrance post must both be bounds checked.
  for (const [name, frame] of [['fence012.seb', 3], ['door00.seb', 1]]) {
    const boundaryFile = join(root, 'assets/common', name);
    const originalBoundary = readFileSync(boundaryFile);
    const invalidBoundary = Buffer.from(originalBoundary);
    invalidBoundary.writeInt16BE(30000, 8 + frame*20 + 4);
    writeFileSync(boundaryFile, invalidBoundary);
    const boundaryFailure = run(); assert.notEqual(boundaryFailure.status, 0);
    assert.match(boundaryFailure.stderr, /rectangle/);
    writeFileSync(boundaryFile, originalBoundary);
  }
  console.log('PASS requested-frame bounds, walking/road/boundary frames, common bindings, unused records, truncation and foreign cwd');
} finally { rmSync(root, {recursive:true,force:true}); }
