// 全字段对账发布快照与当前C++重建结果；不把两者一致当成APK动态认证。
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';

const [executable, root] = process.argv.slice(2);
for (const [mode, name, count, columns] of [
  ['cells', 'LOADED_MAP.tsv', 576, 14],
  ['instances', 'LOADED_INSTANCES.tsv', 8, 6],
]) {
  const actual = execFileSync(executable, [mode], { encoding: 'utf8', timeout: 10000 });
  assert.equal(actual, readFileSync(`${root}/${name}`, 'utf8'), '发布快照与重建结果不同');
  const rows = actual.trimEnd().split('\n').slice(1).map(line => line.split('\t').map(Number));
  assert.equal(rows.length, count);
  assert.ok(rows.every(row => row.length === columns && row.every(Number.isSafeInteger)));
  if (mode === 'cells') {
    rows.forEach((row, i) => {
      assert.equal(row[0], i % 24);
      assert.equal(row[1], Math.trunc(i / 24));
      assert.equal(row[13], row[12] === -1 ? 0 : row[12] + 1);
    });
  } else {
    assert.deepEqual(rows.map(row => row[1]), [2, 3, 4, 5, 6, 7, 0, 1]);
  }
}
console.log('加载后576格/8实例发布快照全字段对账通过');
