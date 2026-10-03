// Exercise the executable's real CPU asset validation against isolated mutated copies.
import { mkdtempSync, cpSync, readFileSync, writeFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, dirname } from 'node:path';
import { spawnSync } from 'node:child_process';
import assert from 'node:assert/strict';
const executable = process.argv[2];
const root = mkdtempSync(join(tmpdir(), 'ark-frames-'));
try {
  const target = join(root, 'ark_village');
  cpSync(executable, target);
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
  console.log('PASS requested-frame bounds, connected-road frames, unused records, truncation and foreign cwd');
} finally { rmSync(root, {recursive:true,force:true}); }
