const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 冻结反馈技术审核：固定输入，只写本目录摘要，不访问游戏/实时档/进程。
// 不复制原图、原日志、purpose、params、user_message、真实村名或账号字段。
const fs = archivePaths.require('fs');
const path = archivePaths.require('path');
const crypto = archivePaths.require('crypto');
const input = path.resolve(archivePaths.workDir, '../window-restore-observation/20261009-110014-startup-ui');
const sha256 = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const issues = [];
let checks = 0;
const check = (yes, message, context) => {
    ++checks;
    if (!yes) issues.push({message, ...context});
    return yes;
};
function inventory(dir, prefix = '') {
    const files = [];
    for (const name of fs.readdirSync(dir).sort()) {
        const absolute = path.join(dir, name), relative = prefix + name;
        const stat = fs.lstatSync(absolute);
        if (stat.isSymbolicLink()) { issues.push({message: '输入有符号链接，拒绝跟随', file: relative}); continue; }
        if (stat.isDirectory()) files.push(...inventory(absolute, relative + '/'));
        else if (stat.isFile()) {
            const bytes = fs.readFileSync(absolute);
            files.push({file: relative, bytes: bytes.length, sha256: sha256(bytes)});
        } else issues.push({message: '输入含非普通文件', file: relative});
    }
    return files;
}
function jpeg(bytes) {
    if (bytes.length < 4 || bytes[0] !== 0xff || bytes[1] !== 0xd8) throw Error('不是JPEG SOI');
    const sof = new Set([0xc0, 0xc1, 0xc2, 0xc3, 0xc5, 0xc6, 0xc7, 0xc9, 0xca, 0xcb, 0xcd, 0xce, 0xcf]);
    let at = 2;
    while (at < bytes.length) {
        if (bytes[at++] !== 0xff) throw Error('JPEG段起点非法');
        while (at < bytes.length && bytes[at] === 0xff) ++at;
        if (at === bytes.length) throw Error('JPEG marker截断');
        const marker = bytes[at++];
        if (marker === 0xda || marker === 0xd9) break;
        if (marker === 0x01 || marker >= 0xd0 && marker <= 0xd7) continue;
        if (at + 2 > bytes.length) throw Error('JPEG段长度截断');
        const size = bytes.readUInt16BE(at);
        if (size < 2 || at + size > bytes.length) throw Error('JPEG段越界');
        if (sof.has(marker)) {
            if (size < 8) throw Error('JPEG SOF截断');
            return {format: 'Jpeg', width: bytes.readUInt16BE(at + 5), height: bytes.readUInt16BE(at + 3)};
        }
        at += size;
    }
    throw Error('JPEG没有SOF尺寸');
}
const text = name => fs.readFileSync(path.join(input, name), 'utf8');
const jpegFormat = value => typeof value === 'string' &&
    ['jpeg', 'image/jpeg'].includes(value.toLowerCase()); // 容器名和MIME是同一格式的两种标记。
const sameInstant = (a, b) => typeof a === 'string' && typeof b === 'string' &&
    a.endsWith('Z') && b.endsWith('Z') && Number.isFinite(Date.parse(a)) && Date.parse(a) === Date.parse(b);
const files = inventory(input), byFile = new Map(files.map(file => [file.file, file]));
const evidence = JSON.parse(text('EVIDENCE.json'));
const metadata = JSON.parse(text('screenshot-metadata.json'));
const actions = text('actions.jsonl').trim().split(/\r?\n/).map(JSON.parse);
const result = text('RESULT.md'), actionsMarkdown = text('ACTIONS.md');
check(Array.isArray(evidence.files), 'EVIDENCE files不是数组');
check(Array.isArray(metadata), '截图元数据不是数组');
const listed = new Set(), missing = [], mismatches = [], imageRows = [];
for (const declared of evidence.files) {
    check(typeof declared.file === 'string' && !declared.file.includes('..') &&
        !path.isAbsolute(declared.file), '清单路径越界');
    check(!listed.has(declared.file), '清单路径重复', {file: declared.file});
    listed.add(declared.file);
    const actual = byFile.get(declared.file);
    if (!check(!!actual, '清单文件缺失', {file: declared.file})) { missing.push(declared.file); continue; }
    if (!check(actual.bytes === declared.bytes, '清单字节数不符', {file: declared.file}))
        mismatches.push({file: declared.file, field: 'bytes', declared: declared.bytes, actual: actual.bytes});
    if (!check(actual.sha256 === declared.sha256, '清单哈希不符', {file: declared.file}))
        mismatches.push({file: declared.file, field: 'sha256'});
    if (!declared.file.endsWith('.jpg')) continue;
    let dimensions;
    try { dimensions = jpeg(fs.readFileSync(path.join(input, declared.file))); }
    catch (error) { issues.push({message: error.message, file: declared.file}); continue; }
    check(dimensions.width === declared.actual_width && dimensions.height === declared.actual_height &&
        dimensions.format === declared.actual_format, '清单JPEG真实尺寸/格式不符', {file: declared.file});
    const meta = metadata.filter(row => row.file === declared.file);
    check(meta.length === 1, '截图元数据缺项/重复', {file: declared.file});
    if (meta.length === 1)
        check(meta[0].bytes === actual.bytes && meta[0].width === dimensions.width &&
            meta[0].height === dimensions.height && jpegFormat(meta[0].format) &&
            sameInstant(meta[0].utc, declared.captured_utc), '截图元数据与文件/清单不符', {file: declared.file});
    imageRows.push({file: declared.file, ...dimensions, captured_utc: declared.captured_utc});
}
const unlisted = files.filter(file => !listed.has(file.file)).map(file => file.file);
check(evidence.report === 'RESULT.md' && !!evidence.report_excluded_from_hash_manifest,
    'RESULT排除哈希清单的说明缺失');
check(unlisted.length === 2 && unlisted.includes('EVIDENCE.json') && unlisted.includes('RESULT.md'),
    '出现未登记且未明确排除的文件', {files: unlisted});
check(metadata.length === imageRows.length, '截图元数据和图片数量不符');

// 更名记录只取匿名文件名；原日志保持原时点的旧名，不改写冻结输入。
const renames = new Map();
for (const row of actions)
    if (row.event === 'label_corrected' && row.old && row.new)
        renames.set('screenshots/' + row.old, 'screenshots/' + row.new);
const resolved = file => renames.get(file) || file;
const screenshots = actions.filter(row => row.event === 'screenshot');
const rawUtcProblems = [];
let last = -Infinity;
for (let index = 0; index < actions.length; ++index) {
    const value = actions[index].utc, instant = Date.parse(value);
    if (!check(typeof value === 'string' && value.endsWith('Z') && Number.isFinite(instant),
        '动作UTC格式非法', {line: index + 1})) rawUtcProblems.push(index + 1);
    check(instant >= last, '原始动作UTC时序倒退', {line: index + 1});
    last = instant;
}
const screenshotTrace = [];
for (const row of screenshots) {
    const file = resolved(row.file);
    const image = imageRows.filter(image => image.file === file);
    check(image.length === 1, '动作截图缺少现存对应图', {file});
    if (image.length === 1) {
        check(sameInstant(row.utc, image[0].captured_utc), '动作截图UTC与清单不符', {file});
        check(row.width === image[0].width && row.height === image[0].height &&
            row.bytes === byFile.get(file).bytes && jpegFormat(row.format),
            '动作截图字节数/尺寸/格式不符', {file});
    }
    screenshotTrace.push({file, utc: row.utc,
        local_shanghai: new Date(Date.parse(row.utc) + 8 * 3600000).toISOString().replace('T', ' ').replace('Z', '+08:00')});
}
check(screenshots.length === imageRows.length, '动作截图与JPEG数量不符');
const staleBefore = [];
for (let index = 0; index < actions.length; ++index) {
    const row = actions[index];
    if (row.before && !byFile.has(resolved(row.before))) staleBefore.push({line: index + 1, file: row.before});
}
check(staleBefore.length === 0, '动作前图存在未解析引用', {references: staleBefore});

// 只审时间列；不摘录动作正文、私人文本或账户/进程JSON。
const tableTimes = [...actionsMarkdown.matchAll(/^\|\s*(\d+)\s*\|\s*(\d{2}:\d{2}:\d{2}\.\d{3})\s*\|/gm)]
    .map(match => ({row: Number(match[1]), stated_clock: match[2]}));
let utcClockMatches = 0, localClockMatches = 0;
for (const item of tableTimes) {
    const seconds = item.stated_clock.slice(0, 8);
    const utcMatch = actions.some(row => row.utc.slice(11, 19) === seconds);
    const localMatch = actions.some(row => new Date(Date.parse(row.utc) + 8 * 3600000).toISOString().slice(11, 19) === seconds);
    if (utcMatch) ++utcClockMatches;
    if (localMatch) ++localClockMatches;
    check(utcMatch || localMatch, '动作整理表时间找不到对应原UTC/本地时点', {row: item.row});
}
const headerSaysLocal = /本地时间/.test(actionsMarkdown) && /Asia\/Shanghai/.test(actionsMarkdown);
const tableTimezoneCorrect = !(headerSaysLocal && utcClockMatches === tableTimes.length && localClockMatches === 0);
check(tableTimezoneCorrect, 'ACTIONS.md标本地时间但整列实际为UTC，需+08:00换算；原反馈冻结不改');
const firstResult = result.match(/首图(\d{2}:\d{2}:\d{2}\.\d{3})/);
const firstLocal = screenshotTrace[0].local_shanghai.slice(11, 23);
check(!!firstResult && firstResult[1] === firstLocal, 'RESULT首图本地时间与原UTC+08:00不符');
const claimed = result.match(/本轮文件数[／/]总字节数：\s*(\d+)文件[／/](\d+)字节/);
check(!!claimed, 'RESULT文件/总字节声明缺失');
const actualBytes = files.reduce((sum, file) => sum + file.bytes, 0);
if (claimed) {
    check(Number(claimed[1]) === files.length, 'RESULT文件数量与实际不符',
        {declared: Number(claimed[1]), actual: files.length});
    check(Number(claimed[2]) === actualBytes, 'RESULT总字节数与实际不符',
        {declared: Number(claimed[2]), actual: actualBytes, difference: actualBytes - Number(claimed[2])});
}
// 审计只写新目录；完结再核冻结源未变化。
const after = inventory(input);
check(JSON.stringify(after) === JSON.stringify(files), '审核期间冻结输入变化');
const integrityIssues = issues.filter(issue => !issue.message.startsWith('ACTIONS.md标') &&
    !issue.message.startsWith('RESULT总字节') && !issue.message.startsWith('RESULT文件数量'));
const report = {
    input: 'work/window-restore-observation/20261009-110014-startup-ui',
    scope: '冻结输入的技术核验；不含图像内容判断、私有字段或原游戏/存档/进程访问',
    checks, integrity_passed: integrityIssues.length === 0, report_consistency_passed: issues.length === 0,
    issues, inventory: {files: files.length, bytes: actualBytes, listed: evidence.files.length,
        missing, unlisted, manifest_mismatches: mismatches, declared_report: claimed ?
            {files: Number(claimed[1]), bytes: Number(claimed[2])} : null,
        all_files: files},
    jpeg: {files: imageRows.length, dimension_sets: [...new Set(imageRows.map(row => `${row.width}x${row.height}`))],
        metadata_records: metadata.length, action_screenshots: screenshots.length, verified: imageRows},
    chronology: {raw_actions: actions.length, raw_utc_problems: rawUtcProblems,
        first_utc: actions[0].utc, last_utc: actions.at(-1).utc,
        elapsed_ms: Date.parse(actions.at(-1).utc) - Date.parse(actions[0].utc),
        actions_table_rows: tableTimes.length, declared_timezone: 'Asia/Shanghai',
        utc_clock_matches: utcClockMatches, local_clock_matches: localClockMatches,
        table_timezone_correct: tableTimezoneCorrect, result_first_local: firstResult?.[1],
        expected_first_local: firstLocal, screenshots: screenshotTrace},
    references: {renames: [...renames].map(([old, current]) => ({old, current})), stale_before: staleBefore},
    representation_notes: ['EVIDENCE实际格式Jpeg与动作/元数据MIME image/jpeg语义一致，且JPEG SOF独立验证',
        'B05/C09/C15清单UTC分别省略毫秒尾零；按同一UTC instant核验，不误报真实时差'],
    limits: ['捕获UTC来自冻结元数据/动作日志，不由JPEG时间戳独立认证',
        '文件哈希一致不证明游戏操作无自动保存副作用',
        '缺号截图不自动判为丢图；被拦截操作的after_label不是成功截图',
        '未读取或复制账户/村名/原日志私有载荷；未触碰报告交还用户的进程']
};
fs.writeFileSync(path.join(archivePaths.workDir, 'AUDIT.json'), JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify({checks, integrity_passed: report.integrity_passed,
    report_consistency_passed: report.report_consistency_passed, files: files.length, bytes: actualBytes,
    listed: evidence.files.length, images: imageRows.length, utc_clock_matches: utcClockMatches,
    local_clock_matches: localClockMatches, issues}));
