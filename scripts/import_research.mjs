// Explicit product snapshot importer, not an APK extractor. Copies maintained data/assets byte
// for byte, follows published display/SEB bindings, and records source hashes for review.
import { readFileSync, copyFileSync, mkdirSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { dirname, join, basename } from 'node:path';
const [research, destination, mode] = process.argv.slice(2);
if (!research || !destination) throw new Error('Expected research root and product assets root');
if (mode && mode !== '--ui-only') throw new Error('Unknown import mode');
const uiOnly = mode === '--ui-only';
const records = uiOnly ? JSON.parse(readFileSync(join(destination, 'SOURCES.json'), 'utf8')).files : [];
const copy = (source, target) => {
  const bytes = readFileSync(join(research, source));
  mkdirSync(dirname(join(destination, target)), { recursive: true });
  copyFileSync(join(research, source), join(destination, target));
  const png = target.endsWith('.png');
  const previous = records.findIndex(v => v.file === target);
  if (previous >= 0) records.splice(previous, 1);
  records.push({ file: target, source: `research/dungeon_village_1/${source}`,
    sha256: createHash('sha256').update(bytes).digest('hex'), bytes: bytes.length,
    ...(png ? { width: bytes.readUInt32BE(16), height: bytes.readUInt32BE(20) } : {}) });
};
if (!uiOnly) {
for (const file of ['MAP.json', 'STATE.json', 'TABLES.json']) copy(`data/startup/${file}`, `data/${file}`);
for (const file of ['LOADED_MAP.tsv', 'LOADED_INSTANCES.tsv']) copy(`data/startup/${file}`, `data/${file}`);
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
}
// Published UI bindings only. A UI refresh must not silently upgrade the gameplay snapshot.
for (const file of ['img.inf', 'top_bar.png', 'btmbar.png', 'btmbar_popular00.png',
  'btmbar_popular01.png', 'townPointbar.png', 'menu.png', 'menu.seb', 'wnd_menuIcon.png',
  'wnd_menuIcon.seb', 'finger_r.png', 'finger_r.seb', 'number01.png', 'number01.seb',
  'number05.png', 'number05.seb', 'number08.png', 'number08.seb', 'number12.png',
  'number12.seb', 'icon_season.png', 'icon_season.seb', 'wnd_conner.png', 'wnd_conner.seb',
  'wnd_lv.png', 'wnd_max.png', 'icon_tenantInfo.png', 'arrow00.png', 'hisho_talk.png',
  'arrow01.png', 'arrow02.png', 'arrow02.seb', 'icon_param00.png', 'road4block00.png', 'road4block01.png'])
  copy(`assets/original/common/${file}`, `common/${file}`);
for (const file of ['img.inf', 'touch_arrow.png', 'touch_arrow.seb', 'buildCategoryBack.png',
  'buildCategoryBack.seb', 'frame.png', 'frame.seb'])
  copy(`assets/original/common2/${file}`, `common2/${file}`);
writeFileSync(join(destination, 'SOURCES.json'), JSON.stringify({ scope: 'loaded_startup_first_arrival_ui', files: records }, null, 2) + '\n');
console.log(`Imported ${records.length} maintained files; all hashes recorded`);
