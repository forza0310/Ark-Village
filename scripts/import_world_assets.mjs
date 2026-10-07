// Explicit import of maintained original sprites for the complete actor/facility catalog.
// No APK extraction. Existing product bytes may only be reused if they match the source.
import { readFileSync, readdirSync, mkdirSync, writeFileSync, existsSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { join } from 'node:path';

const [research, destination] = process.argv.slice(2);
if (!research || !destination) throw new Error('Expected research root and product assets root');
const manifestPath = join(destination, 'SOURCES.json');
const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'));
const records = new Map(manifest.files.map(record => [record.file, record]));
const pending = [];
for (const group of ['image', 'human', 'monster', 'common', 'weapon']) {
  for (const entry of readdirSync(join(research, 'assets/original', group), { withFileTypes: true })) {
    if (!entry.isFile() || !/\.(png|seb|inf)$/.test(entry.name)) continue;
    const file = `${group}/${entry.name}`;
    const source = `assets/original/${file}`;
    const bytes = readFileSync(join(research, source));
    const hash = createHash('sha256').update(bytes).digest('hex');
    const target = join(destination, file);
    if (existsSync(target) && !readFileSync(target).equals(bytes))
      throw new Error(`Refusing to overwrite a different product resource: ${file}`);
    const record = { file, source: `research/dungeon_village_1/${source}`, sha256: hash, bytes: bytes.length };
    if (entry.name.endsWith('.png')) {
      if (bytes.length < 24 || bytes.toString('hex', 0, 8) !== '89504e470d0a1a0a')
        throw new Error(`Invalid PNG: ${file}`);
      record.width = bytes.readUInt32BE(16);
      record.height = bytes.readUInt32BE(20);
    }
    pending.push({ bytes, record, target });
  }
}
// Verify the complete read set before writing: research may be changing in another agent.
for (const item of pending) {
  const source = item.record.source.replace('research/dungeon_village_1/', '');
  if (!readFileSync(join(research, source)).equals(item.bytes))
    throw new Error(`Research changed during import: ${source}`);
}
for (const { bytes, record, target } of pending) {
  mkdirSync(join(destination, record.file.split('/')[0]), { recursive: true });
  writeFileSync(target, bytes);
  records.set(record.file, record);
}
manifest.scope = 'startup_ui_and_complete_world_sprite_catalog';
manifest.files = [...records.values()].sort((a, b) => a.file.localeCompare(b.file));
writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + '\n');
console.log(`Imported ${pending.length} world sprite resources; ${records.size} total source hashes`);
