// Source pins cover mechanical translations and byte-exact data, without reading research.
import { readFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { resolve, sep } from 'node:path';
const root = resolve(process.argv[2]);
const manifest = JSON.parse(readFileSync(`${root}/assets/simulation/SOURCES.json`, 'utf8'));
const seen = new Set();
for (const entry of manifest.files) {
  const path = resolve(root, entry.file);
  if (!path.startsWith(root + sep) || seen.has(entry.file)) throw new Error('Invalid source path');
  seen.add(entry.file);
  const bytes = readFileSync(path);
  if (bytes.length !== entry.bytes || createHash('sha256').update(bytes).digest('hex') !== entry.sha256)
    throw new Error(`Pinned simulation source changed: ${entry.file}`);
}
console.log(`PASS ${seen.size} frozen simulation source/data hashes`);
