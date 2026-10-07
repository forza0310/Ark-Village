// Unicode authority contract: source discovery, decoding and stable generated output.
// Asset provenance tests verify shipped bytes; they cannot cover scanner rejection
// or a newly authored label. This Node suite owns those build-time contracts.
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { mkdtempSync, mkdirSync, readFileSync, writeFileSync, statSync, utimesSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { collectDesktopGlyphs, desktopGlyphsHeader } from '../../scripts/compile_desktop_glyphs.mjs';

const product = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const directory = path.join(product, 'build/validation/desktop-glyph-contract');
mkdirSync(directory, { recursive: true });
const root = mkdtempSync(path.join(directory, 'fixture-'));
for (const folder of ['src', 'include', 'assets', 'research', 'build'])
  mkdirSync(path.join(root, folder));
const put = (file, bytes) => writeFileSync(path.join(root, file), bytes);
const source = '中\\u6587\\U0001F680\u3000\n\t\u007f';
const data = '{"\\u706b":"\\ud83d\\ude80"}';
put('src/a.cpp', source);
put('include/a.hpp', '中');
put('assets/a.json', data);
for (const file of ['src/ignored.md', 'research/a.cpp', 'build/a.hpp', 'assets/a.png'])
  put(file, '禁');
const expected = [...Array.from({ length: 95 }, (_, index) => index + 32),
  0x3000, 0x4e2d, 0x6587, 0x706b, 0x1f680].sort((a, b) => a - b);
const inventory = collectDesktopGlyphs(root);
assert.deepEqual(inventory.codepoints, expected, 'raw/escaped Unicode demand and control filter');
assert.deepEqual(inventory.inputs, ['src/a.cpp', 'include/a.hpp', 'assets/a.json'].map(file => ({
  file, sha256: createHash('sha256').update(readFileSync(path.join(root, file))).digest('hex'),
})), 'only authored supported inputs, with byte hashes');
const headerBytes = Buffer.from([...desktopGlyphsHeader(inventory).matchAll(/\\x([a-f0-9]{2})/g)]
  .map(match => parseInt(match[1], 16)));
assert.equal(headerBytes.toString('utf8'), expected.map(point => String.fromCodePoint(point)).join(''),
  'generated C++ string and font demand must agree');

const jsonOutput = path.join(root, 'build/glyphs.json');
const headerOutput = path.join(root, 'build/glyphs.hpp');
const invoke = extra => spawnSync(process.execPath, [path.join(product, 'scripts/compile_desktop_glyphs.mjs'),
  '--root', root, '--json', jsonOutput, '--header', headerOutput, ...extra], { encoding: 'utf8', windowsHide: true });
assert.equal(invoke([]).status, 0, 'CLI creates shared outputs');
assert.deepEqual(JSON.parse(readFileSync(jsonOutput, 'utf8')), inventory);
const sentinel = new Date('2001-01-01T00:00:00Z');
for (const file of [jsonOutput, headerOutput]) utimesSync(file, sentinel, sentinel);
assert.equal(invoke([]).status, 0);
for (const file of [jsonOutput, headerOutput])
  assert.equal(statSync(file).mtimeMs, sentinel.getTime(), 'unchanged input must not rebuild all consumers');
put('src/new.cpp', '新');
assert.equal(invoke([]).status, 0);
assert.ok(JSON.parse(readFileSync(jsonOutput, 'utf8')).codepoints.includes(0x65b0),
  'a new source file must be discovered without reconfiguration');
assert.notEqual(statSync(headerOutput).mtimeMs, sentinel.getTime());

const invalid = [
  ['src/a.cpp', Buffer.from([0xc3, 0x28]), 'invalid UTF-8'],
  ['src/a.cpp', '\\U00110000', 'out of range scalar'],
  ['assets/a.json', '{', 'invalid JSON'],
  ['assets/a.json', '"\\ud800"', 'unpaired surrogate'],
];
for (const [file, bytes, scenario] of invalid) {
  const original = readFileSync(path.join(root, file));
  const before = [jsonOutput, headerOutput].map(output => readFileSync(output));
  put(file, bytes);
  assert.throws(() => collectDesktopGlyphs(root), undefined, scenario);
  assert.notEqual(invoke([]).status, 0, scenario);
  for (const [index, output] of [jsonOutput, headerOutput].entries())
    assert.deepEqual(readFileSync(output), before[index], scenario + ': preserve last valid outputs');
  put(file, original);
}
const forbidden = spawnSync(process.execPath, [path.join(product, 'scripts/compile_desktop_glyphs.mjs'),
  '--root', root, '--header', path.join(root, 'src/generated.hpp')], { encoding: 'utf8', windowsHide: true });
assert.notEqual(forbidden.status, 0, 'output cannot feed itself into the next inventory');
console.log('PASS desktop Unicode discovery, rejection, header and incremental build contracts');
