// Verify product snapshots against their import manifest without reading research at runtime.
import { readFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { resolve, sep } from 'node:path';
const root = resolve(process.argv[2]);
const manifest = JSON.parse(readFileSync(`${root}/SOURCES.json`, 'utf8'));
const seen = new Set();
for (const entry of manifest.files) {
  const path = resolve(root, entry.file);
  if (!path.startsWith(root + sep) || seen.has(entry.file)) throw new Error('Unsafe or duplicate resource path');
  seen.add(entry.file);
  const bytes = readFileSync(path);
  if (bytes.length !== entry.bytes || createHash('sha256').update(bytes).digest('hex') !== entry.sha256)
    throw new Error(`Product resource changed: ${entry.file}`);
  if (entry.width && (bytes.readUInt32BE(16) !== entry.width || bytes.readUInt32BE(20) !== entry.height))
    throw new Error(`PNG dimensions changed: ${entry.file}`);
}
console.log(`PASS ${seen.size} product source hashes`);
