// Explicit product snapshot importer, not an APK extractor. Copies maintained data/assets byte
// for byte, follows published display/SEB bindings, and records source hashes for review.
import { readFileSync, copyFileSync, mkdirSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { dirname, join, basename } from 'node:path';
const [research, destination] = process.argv.slice(2);
if (!research || !destination) throw new Error('Expected research root and product assets root');
const records = [];
const copy = (source, target) => {
  const bytes = readFileSync(join(research, source));
  mkdirSync(dirname(join(destination, target)), { recursive: true });
  copyFileSync(join(research, source), join(destination, target));
  const png = target.endsWith('.png');
  records.push({ file: target, source: `research/dungeon_village_1/${source}`,
    sha256: createHash('sha256').update(bytes).digest('hex'), bytes: bytes.length,
    ...(png ? { width: bytes.readUInt32BE(16), height: bytes.readUInt32BE(20) } : {}) });
};
for (const file of ['MAP.json', 'STATE.json', 'TABLES.json']) copy(`data/startup/${file}`, `data/${file}`);
copy('data/original/tenantData.txt', 'data/tenantData.txt');
const table = JSON.parse(readFileSync(join(research, 'data/startup/TABLES.json'), 'utf8'));
const map = JSON.parse(readFileSync(join(research, 'data/startup/MAP.json'), 'utf8'));
const state = JSON.parse(readFileSync(join(research, 'data/startup/STATE.json'), 'utf8'));
const rows = table.entries.find(v => v.entry === 'mapchip_main.txt').source_utf8.split('\n').map(v => v.split('\t'));
const displayIds = new Set(map.cells.flat().map(v => v[0]));
for (const item of state.initial_catalog) displayIds.add(Number(rows.find(v => Number(v[5]) === item.id)[0]));
displayIds.add(27); // Published grass binding used beneath source-seed projection.
const sprites = new Set([...displayIds].map(id => rows.find(v => Number(v[0]) === id)[1]));
const indices = new Set();
// Walk the documented legacy layout solely to collect referenced images for packaging.
// Runtime parsing/validation is provided by the adapted, maintained C++ SEB reader.
for (const sprite of sprites) {
  if (basename(sprite) !== sprite) throw new Error('Unsafe sprite path');
  copy(`assets/original/image/${sprite}`, `image/${sprite}`);
  const bytes = readFileSync(join(research, `assets/original/image/${sprite}`));
  let at = 0;
  const short = () => { const v = bytes.readInt16BE(at); at += 2; return v; };
  const layers = short(); short();
  if (layers < 0) throw new Error('Unsupported compressed sprite');
  for (let i = 0; i < layers; ++i) {
    const count = short(); short();
    if (count < 0) throw new Error('Invalid sprite record count');
    for (let j = 0; j < count; ++j) {
      short(); const index = short();
      if (index >= 0) indices.add(index);
      for (let k = 0; k < 8; ++k) short();
    }
  }
  if (at !== bytes.length) throw new Error('Unexpected sprite tail');
}
copy('assets/original/image/img.inf', 'image/img.inf');
const images = new Map(readFileSync(join(research, 'assets/original/image/img.inf'), 'utf8')
  .trimEnd().split(/\r?\n/).map(v => { const [id, name] = v.split('\t'); return [Number(id), name.replace(/\.gif$/, '.png')]; }));
for (const index of indices) {
  const name = images.get(index);
  if (!name || basename(name) !== name) throw new Error('Missing or unsafe image binding');
  copy(`assets/original/image/${name}`, `image/${name}`);
}
copy('assets/original/human/chara_flower00.png', 'human/chara_flower00.png');
copy('assets/original/human/walk00.seb', 'human/walk00.seb');
copy('assets/original/common/chara_hishoko01.png', 'common/chara_hishoko01.png');
copy('assets/original/common/chara_hishoko01.seb', 'common/chara_hishoko01.seb');
copy('assets/original/common/wnd_back.png', 'ui/wnd_back.png');
copy('assets/original/common/wnd_bar.png', 'ui/wnd_bar.png');
writeFileSync(join(destination, 'SOURCES.json'), JSON.stringify({ scope: 'static_source_projection_first_arrival', files: records }, null, 2) + '\n');
console.log(`Imported ${records.length} maintained files; all hashes recorded`);
