const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 本批只读来源、文档和规模审计；不启动原游戏，不读取实时玩家存档。
const fs = archivePaths.require('fs'), path = archivePaths.require('path'), crypto = archivePaths.require('crypto');
const cp = archivePaths.require('child_process');
const root = path.resolve(archivePaths.workDir, '../..');
const repo = path.resolve(root, '../..');
const hash = b => crypto.createHash('sha256').update(b).digest('hex');
let checks = 0;
const need = (value, message) => { ++checks; if (!value) throw Error(message); };
const source = JSON.parse(fs.readFileSync(path.join(root, 'work/startup-skin-contract/EVIDENCE.json')));
for (const row of source.sources) {
  const bytes = fs.readFileSync(path.join(root, row.path));
  need(hash(bytes) === row.source_sha256, '来源文件身份变化：' + row.path);
  const lines = bytes.toString('utf8').split(/\r?\n/);
  need(hash(Buffer.from(lines.slice(row.first - 1, row.last).join('\n'))) === row.window_lf_sha256,
       '局部窗口身份变化：' + row.path);
}
for (const row of source.assets) {
  const bytes = fs.readFileSync(path.join(root, row.path));
  need(bytes.length === row.bytes && hash(bytes) === row.sha256, '原素材变化：' + row.path);
}
const git = args => cp.execFileSync('git', ['-c', 'safe.directory=D:/code/Ark-Village', ...args],
                                    { cwd: repo, encoding: 'utf8' }).trim();
const files = [...new Set([...git(['diff', 'HEAD', '--name-only']).split('\n'),
  ...git(['ls-files', '--others', '--exclude-standard']).split('\n'),
  'research/dungeon_village_1/work/startup-skin-delivery/README.md'])].filter(Boolean);
let links = 0;
const missing = [];
const outputPath = path.join(archivePaths.workDir, 'VALIDATION.json');
for (const file of files.filter(p => p.startsWith('research/') && p.endsWith('.md'))) {
  const absolute = path.join(repo, file);
  const content = fs.readFileSync(absolute, 'utf8');
  for (const m of content.matchAll(/\[[^\]\r\n]*\]\(([^)\r\n]+)\)/g)) {
    let target = m[1].replace(/^<|>$/g, '').split('#')[0];
    if (!target || /^(https?:|mailto:)/.test(target)) continue;
    target = target.replace(/:\d+(?:-\d+)?$/, '');
    ++links;
    const resolved = path.resolve(path.dirname(absolute), target);
    // 审计自己的输出在末尾生成后再核，不要求事先放置伪摘要。
    if (resolved !== outputPath && !fs.existsSync(resolved)) missing.push({file, target});
  }
}
need(missing.length === 0, '新增/修改文档断链：' + JSON.stringify(missing));
const protectedPaths = ['prototype/include/dungeon_village_prototype/startup_application.hpp',
 'prototype/src/startup_application.cpp', 'prototype/src/startup_world_codec_fields.inc', 'data', 'assets/original'];
need(!git(['diff', 'HEAD', '--', ...protectedPaths.map(p => 'research/dungeon_village_1/' + p)]),
     '本批不得改变Owner、schema、原表或素材');
const codec = fs.readFileSync(path.join(root, 'prototype/src/startup_world_codec_fields.inc'), 'utf8');
const schema = codec.match(/schema_identity\[\] = "([a-f0-9]+)"/)[1];
need(schema === 'a1ca189f3b9290b18390812af91f601eaac9f295154ab1de3601590d0fbda2d7', 'schema变化');
function scan(dir) {
  let files = 0, bytes = 0, pending = [];
  for (const entry of fs.readdirSync(dir, {withFileTypes:true})) {
    const p = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      const child = scan(p); files += child.files; bytes += child.bytes; pending.push(...child.pending);
    } else { ++files; bytes += fs.statSync(p).size; if (/\.pending$/.test(entry.name)) pending.push(p); }
  }
  return {files, bytes, pending};
}
const outputNames = ['build.log', 'build-final.log', 'ctest.log', 'visuals.log', 'skin-pieces.png'];
const outputs = outputNames.map(name => {
  const b = fs.readFileSync(path.join(archivePaths.workDir, name));
  return {path:name, bytes:b.length, sha256:hash(b)};
});
const png = fs.readFileSync(path.join(archivePaths.workDir, 'skin-pieces.png'));
need(png.readUInt32BE(16) === 600 && png.readUInt32BE(20) === 660, 'CPU拼图尺寸');
const result = {date:'2026-10-09', baseline:git(['rev-parse', 'HEAD']), checks, links,
  sources:source.sources.length, assets:source.assets.length, schema,
  release:scan(path.join(root, 'work/release')), outputs,
  qualification:'APK只读图块计划及CPU裁片；未认证完整Steam皮肤、OS输入或自然结局'};
fs.writeFileSync(outputPath, JSON.stringify(result,null,2)+'\n');
if (!fs.existsSync(outputPath)) throw Error('审计摘要未生成');
console.log(JSON.stringify(result,null,2));
