// Strict reader for the two maintained reset-only TSVs. No APK parsing or reconstruction here.
import { createHash } from 'node:crypto';
const need = (ok, message) => { if (!ok) throw new Error(message); };
const rows = (text, header, count) => {
  need(typeof text === 'string' && text.endsWith('\n'), 'Loaded TSV missing final newline');
  const lines = text.slice(0, -1).split('\n');
  need(lines.shift() === header && lines.length === count, 'Loaded TSV header/row count mismatch');
  return lines.map(line => {
    const fields = line.split('\t');
    need(fields.length === header.split('\t').length, 'Loaded TSV column count mismatch');
    return fields.map(v => {
      need(/^-?\d+$/.test(v) && Number.isSafeInteger(+v) && +v >= -1 && +v <= 2147483647,
           'Invalid loaded TSV integer');
      return +v;
    });
  });
};
export function parseLoadedMap(cellText, instanceText) {
  const cells = rows(cellText, 'x\ty\tdefinition_id\tlegacy_state\troute_category\tdisplay_id\tvariant\troad_mask\tboundary_fragment\texternal_direction\troad_quad\tedge_road_pair\tlegacy_instance_id\tmaintained_instance_id', 576);
  const instances = rows(instanceText, 'vector_index\tlegacy_instance_id\tmaintained_instance_id\tdefinition_id\tx\ty', 8);
  const ids = new Set(), positions = new Map();
  instances.forEach((r, i) => {
    need(r[0] === i && r[1] >= 0 && r[1] < 8 && r[2] === r[1] + 1 && !ids.has(r[2]), 'Invalid loaded instance identity/order');
    need(r[3] >= 0 && r[3] < 85 && r[4] >= 0 && r[4] < 24 && r[5] >= 0 && r[5] < 24,
         'Invalid loaded instance definition/position');
    const index = r[5] * 24 + r[4];
    need(!positions.has(index), 'Overlapping loaded instances');
    ids.add(r[2]); positions.set(index, r);
  });
  cells.forEach((r, i) => {
    need(r[0] === i % 24 && r[1] === Math.floor(i / 24), 'Loaded cell order mismatch');
    need(r[2] >= 0 && r[2] < 85 && r[3] >= 0 && r[3] <= 12 && r[4] >= 0 && r[4] <= 4 &&
         r[5] >= 0 && r[5] < 85 && r[6] >= 0 && r[7] >= 0 && r[7] < 16 &&
         r[8] <= 5 && r[9] <= 3 && [0,1].includes(r[10]) && [0,1].includes(r[11]), 'Invalid loaded tile fields');
    const instance = positions.get(i);
    need(instance ? r[12] === instance[1] && r[13] === instance[2] && r[2] === instance[3]
                  : r[12] === -1 && r[13] === 0, 'Loaded cell/instance binding mismatch');
  });
  for (const [text, hash] of [[cellText,'1a3955e1a139931c2731c5598806f920d836b353fa5ba98914b89f2269eefea1'],
                             [instanceText,'835062acdce1f3c8befd641b65fc71e69f0aa7f08cf395a3ca1ee612445a3c8d']])
    need(createHash('sha256').update(text).digest('hex') === hash, 'Loaded snapshot hash mismatch');
  return {cells, instances};
}
