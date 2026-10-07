// 同一continuous程序的进程隔离场景：新局或已声明资格的前缀继续，新保存后另起两进程恢复同一尾段。
// 临时目录由本脚本创建和回收，原输入/用户目录不递归删除；任何失败收齐子进程。
import fs from 'node:fs/promises';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';

const productRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const buildRoot = path.join(productRoot, 'build');
// Product diagnostics own only build descendants; resolve existing ancestors before mkdir
// so a junction cannot redirect creation or cleanup into research or user save directories.
async function checkedBuildPath(candidate, label) {
    const relative = path.relative(buildRoot, candidate);
    if (!relative || relative.startsWith('..') || path.isAbsolute(relative))
        throw new Error(label + '必须位于产品build内的独立路径');
    let ancestor = candidate;
    let resolved;
    while (!resolved) {
        try { resolved = await fs.realpath(ancestor); }
        catch (error) {
            if (error.code !== 'ENOENT') throw error;
            const parent = path.dirname(ancestor);
            if (parent === ancestor) throw error;
            ancestor = parent;
        }
    }
    const realBuild = await fs.realpath(buildRoot);
    const actualRelative = path.relative(realBuild, resolved);
    if (actualRelative.startsWith('..') || path.isAbsolute(actualRelative))
        throw new Error(label + '现存祖先位于产品build外');
    return candidate;
}
const args = process.argv.slice(2);
const options = new Map();
for (let i = 0; i < args.length; i += 2) {
    if (i + 1 === args.length || options.has(args[i]) ||
        !['--exe', '--work-dir', '--save-at', '--stop-at', '--producer-revision',
            '--snapshot-file', '--save-every', '--save-directory', '--scenario', '--load-prefix',
            '--prefix-status', '--prefix-next-frame'].includes(args[i]))
        throw new Error('需要 --exe <程序> --work-dir <产品build工作目录> [--save-at 420 --stop-at 840]');
    options.set(args[i], args[i + 1]);
}
if (!options.get('--exe') || !options.get('--work-dir'))
    throw new Error('缺少 --exe 或 --work-dir');
const executable = path.resolve(options.get('--exe'));
const workRoot = path.resolve(options.get('--work-dir'));
await checkedBuildPath(workRoot, '回放临时目录');
if (options.has('--snapshot-file'))
    await checkedBuildPath(path.resolve(options.get('--snapshot-file')), '保留快照');
// 场景身份同时决定入口、保护上限和完整尾段oracle；默认保留原晋级验收。
const scenario = options.get('--scenario') ?? 'natural_progression';
if (!['natural_progression', 'natural_expansion'].includes(scenario))
    throw new Error('未知回放场景');
const frameLimit = scenario === 'natural_expansion' ? 240000 : 180000;
const number = (name, fallback) => {
    const text = options.get(name) ?? String(fallback);
    if (!/^\d+$/.test(text)) throw new Error(`非法帧参数 ${name}`);
    const value = Number(text);
    if (!Number.isSafeInteger(value) || value >= frameLimit) throw new Error(`帧超范围 ${name}`);
    return value;
};
const saveAt = number('--save-at', 420);
const stopAt = options.get('--stop-at') === 'complete' ? undefined : number('--stop-at', 840);
if (stopAt !== undefined && stopAt <= saveAt) throw new Error('尾段终点必须晚于完整前缀保存轮');
const stopArgs = stopAt === undefined ? [] : ['--stop-at', String(stopAt)];
const producer = options.get('--producer-revision') ?? 'unspecified';
const sha256 = bytes => createHash('sha256').update(bytes).digest('hex');
// 仅预检AVRSAVE1分区摘要、metadata及AWDRV1七个身份字段；完整世界仍由C++加载器校验。
function replayIdentity(bytes) {
    if (bytes.length < 64 || bytes.subarray(-64).toString('ascii') !== sha256(bytes.subarray(0, -64)))
        throw new Error('源前缀整体摘要不符');
    const reader = buffer => ({ buffer, at: 0,
        raw(length) {
            if (!Number.isSafeInteger(length) || length < 0 || length > this.buffer.length - this.at)
                throw new Error('源前缀分区边界不符');
            const result = this.buffer.subarray(this.at, this.at + length); this.at += length; return result;
        },
        u32() { return this.raw(4).readUInt32LE(); },
        u64() { const value = this.raw(8).readBigUInt64LE();
            if (value > BigInt(Number.MAX_SAFE_INTEGER)) throw new Error('源身份数值不可表示');
            return Number(value); },
        text() { const length = this.u32();
            if (length > 4096) throw new Error('源前缀文本超预算'); return this.raw(length).toString('utf8'); }
    });
    const input = reader(bytes.subarray(0, -64));
    if (input.raw(8).toString('ascii') !== 'AVRSAVE1' || input.u32() !== 1 ||
        input.u32() !== 1 || input.u32() !== 2) throw new Error('源不是本版replay文件');
    const dataset = input.text(), schema = input.text(), count = input.u32(), sections = new Map();
    if (count < 2 || count > 64) throw new Error('源分区数非法');
    for (let n = 0; n < count; ++n) {
        const id = input.u32(), version = input.u32(), required = input.u32(), length = input.u64();
        const digest = input.text(), body = input.raw(length);
        if (!version || required > 1 || sections.has(id) || sha256(body) !== digest)
            throw new Error('源分区身份／摘要不符');
        sections.set(id, { version, required, body });
    }
    if (input.at !== input.buffer.length) throw new Error('源分区有尾随字节');
    for (const id of [1, 4])
        if (sections.get(id)?.version !== 1 || sections.get(id)?.required !== 1)
            throw new Error('源缺少必需metadata或controller');
    const meta = reader(sections.get(1).body), revision = meta.text(), controller = meta.text(), nextFrame = meta.u64();
    if (meta.at !== meta.buffer.length || controller !== 'natural-progression-expansion-v1')
        throw new Error('源controller身份不符');
    const driver = reader(sections.get(4).body);
    if (driver.buffer.length > 1024 * 1024 || driver.u64() !== 0x315652445741)
        throw new Error('源driver标识或预算不符');
    const seed = driver.u64(), speed = driver.u64(), expansion = driver.u64();
    const actualNext = driver.u64(), observed = driver.u64(), checks = driver.u64(), terminal = driver.u64();
    if (expansion > 1 || terminal !== 0 || actualNext !== nextFrame || observed !== nextFrame - 1 ||
        checks < nextFrame || seed !== 1 || speed !== 0 ||
        expansion !== Number(scenario === 'natural_expansion') || nextFrame < 1 || nextFrame >= frameLimit)
        throw new Error('源实际next_frame／场景／seed／speed不符');
    return { next_frame: nextFrame, scenario, seed, speed, producer_revision: revision, dataset, schema };
}
let sourcePrefix;
let sourcePrefixBytes;
if (!options.has('--load-prefix') && (options.has('--prefix-status') || options.has('--prefix-next-frame')))
    throw new Error('源资格和帧声明必须与--load-prefix同时使用');
if (options.has('--load-prefix')) {
    const prefixStatus = options.get('--prefix-status') ?? 'certified';
    if (!['certified', 'candidate'].includes(prefixStatus)) throw new Error('未知源前缀资格');
    if (prefixStatus === 'candidate' && !options.has('--prefix-next-frame'))
        throw new Error('裸候选需要显式--prefix-next-frame声明');
    const source = await fs.realpath(path.resolve(options.get('--load-prefix')));
    const realProductBuild = await fs.realpath(buildRoot);
    const relative = path.relative(realProductBuild, source);
    if (!relative || relative.startsWith('..') || path.isAbsolute(relative))
        throw new Error('源前缀必须是产品build内的冻结文件');
    const sourceSize = (await fs.stat(source)).size;
    if (!sourceSize || sourceSize > 128 * 1024 * 1024) throw new Error('源前缀超出既有预算');
    sourcePrefixBytes = await fs.readFile(source);
    if (sourcePrefixBytes.length !== sourceSize || sourcePrefixBytes.length > 128 * 1024 * 1024)
        throw new Error('源前缀读取期间长度改变或超预算');
    const identity = replayIdentity(sourcePrefixBytes);
    if (saveAt < identity.next_frame) throw new Error('新capture必须位于源实际next_frame或之后');
    if (options.has('--prefix-next-frame') && number('--prefix-next-frame', 0) !== identity.next_frame)
        throw new Error('源next_frame声明与文件实际身份不符');
    sourcePrefix = { file: source, snapshot_sha256: sha256(sourcePrefixBytes), snapshot_bytes: sourcePrefixBytes.length,
        ...identity, source_status: prefixStatus,
        qualification: '未认证裸候选恢复reference；仅本轮新捕获之后尾段可获三路比较，不认证此前新局前缀' };
    if (prefixStatus === 'certified') {
        const certificateFile = await fs.realpath(source + '.json');
        const certificateRelative = path.relative(realProductBuild, certificateFile);
        if (!certificateRelative || certificateRelative.startsWith('..') || path.isAbsolute(certificateRelative))
            throw new Error('源证书不能逃逸产品build');
        const certificateSize = (await fs.stat(certificateFile)).size;
        if (!certificateSize || certificateSize > 1024 * 1024) throw new Error('源证书超出既有预算');
        const certificateBytes = await fs.readFile(certificateFile);
        if (certificateBytes.length !== certificateSize || certificateBytes.length > 1024 * 1024)
            throw new Error('源证书读取期间长度改变或超预算');
        const certificate = JSON.parse(certificateBytes.toString('utf8'));
        const nextFrame = certificate.save_at + 1;
        const legacyProtocol = certificate.certification === undefined &&
            certificate.comparison === '逐帧全Session和Driver及尾段stdout三路相同';
        const certifiedProtocol = typeof certificate.certification === 'string' && certificate.certification.length > 0;
        // 只消费与具体源hash绑定的既有三路证书；裸周期候选不提升为已认证前缀。
        if (certificate.scenario !== scenario || certificate.seed !== 1 || certificate.speed !== 0 ||
            !Number.isSafeInteger(certificate.save_at) || certificate.save_at < 1 ||
            !Number.isSafeInteger(nextFrame) || nextFrame !== identity.next_frame ||
            !Number.isSafeInteger(certificate.stop_at) || certificate.stop_at < nextFrame ||
            certificate.tail_frames !== certificate.stop_at - certificate.save_at ||
            certificate.process_count !== 3 || !(certifiedProtocol || legacyProtocol) ||
            certificate.snapshot_bytes !== sourcePrefixBytes.length ||
            certificate.snapshot_sha256 !== sha256(sourcePrefixBytes) ||
            !/^[0-9a-f]{64}$/.test(certificate.trace_sha256 ?? ''))
            throw new Error('源前缀hash、场景、seed/speed或原证书资格不符');
        if (saveAt < nextFrame)
            throw new Error('新capture必须位于源前缀next_frame或之后');
        sourcePrefix = { ...sourcePrefix, certificate_file: certificateFile,
            snapshot_sha256: certificate.snapshot_sha256, snapshot_bytes: sourcePrefixBytes.length,
            certificate_sha256: sha256(certificateBytes), next_frame: nextFrame,
            scenario, seed: 1, speed: 0, producer_revision: certificate.producer_revision,
            source_certification: legacyProtocol ? '既有晋级三路比较证书协议' : certificate.certification,
            source_certification_level: certificate.certification_level ?? '既有三路尾段证书（未声明本批等级字段）',
            source_certification_boundary: certificate.certification_boundary ?? '仅沿源证书指定前缀和尾段，不额外认证历史',
            source_save_at: certificate.save_at, source_stop_at: certificate.stop_at,
            qualification: '已有三路证书绑定的指定前缀；本轮不重跑此前新局路径' };
    }
}
if (options.has('--save-every') !== options.has('--save-directory'))
    throw new Error('--save-every与--save-directory必须同时指定');
const saveEvery = options.has('--save-every') ? number('--save-every', 0) : undefined;
if (saveEvery === 0) throw new Error('周期保存间隔必须为正数');
let periodicDirectory;
if (saveEvery !== undefined) {
    periodicDirectory = path.resolve(options.get('--save-directory'));
    await checkedBuildPath(periodicDirectory, '周期档目录');
    await fs.mkdir(periodicDirectory, { recursive: true });
    for (let frame = saveEvery; frame <= (stopAt ?? frameLimit - 1); frame += saveEvery) {
        try {
            await fs.lstat(path.join(periodicDirectory, `prefix-${frame}.awr`));
            throw new Error(`周期快照已存在，拒绝覆盖：prefix-${frame}.awr`);
        } catch (error) { if (error.code !== 'ENOENT') throw error; }
    }
}
const periodicArgs = periodicDirectory
    ? ['--save-every', String(saveEvery), '--save-directory', periodicDirectory] : [];
await fs.mkdir(workRoot, { recursive: true });
const owned = await fs.mkdtemp(path.join(workRoot, 'replay-process-'));
const snapshot = path.join(owned, 'prefix.awr');
let child;
let cancelled = false;
let passed = false;
const processSeconds = [];
let invocation = 0;
const cancel = () => { cancelled = true; child?.kill(); };
process.on('SIGINT', cancel);
process.on('SIGTERM', cancel);

async function run(parameters) {
    if (cancelled) throw new Error('回放验证被取消');
    const started = performance.now();
    const index = invocation++;
    return await new Promise((resolve, reject) => {
        const current = spawn(executable, parameters, {
            cwd: path.dirname(executable), windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'],
        });
        child = current;
        const out = [], err = [];
        let timedOut = false;
        let launchError;
        const timer = setTimeout(() => { timedOut = true; current.kill(); }, 180 * 60 * 1000);
        current.stdout.on('data', b => { out.push(b); process.stdout.write(b); });
        current.stderr.on('data', b => err.push(b));
        current.on('error', e => { launchError = e; });
        current.on('close', async code => {
            clearTimeout(timer);
            child = undefined;
            const stdout = Buffer.concat(out).toString('utf8');
            const stderr = Buffer.concat(err).toString('utf8');
            try {
                await fs.writeFile(path.join(owned, `process-${index}.log`), stdout + '\n' + stderr);
            } catch (error) { reject(error); return; }
            if (launchError || timedOut || cancelled || code !== 0)
                reject(new Error(`回放进程失败 code=${code} timeout=${timedOut}\n${launchError ?? ''}\n${stdout}\n${stderr}`));
            else if (stderr.trim()) reject(new Error(`回放进程有错误输出\n${stderr}`));
            else { processSeconds.push((performance.now() - started) / 1000); resolve(stdout); }
        });
    });
}

try {
    const traces = [0, 1, 2].map(n => path.join(owned, `tail-${n}.trace`));
    const original = await run([scenario, '1', '0',
        ...(sourcePrefix ? ['--load-file', sourcePrefix.file] : []), '--save-at', String(saveAt),
        '--save-file', snapshot, ...stopArgs, '--trace-from', String(saveAt + 1),
        '--trace-file', traces[0], '--producer-revision', producer, ...periodicArgs]);
    const snapshotBytes = await fs.readFile(snapshot);
    if (!snapshotBytes.length) throw new Error('前缀没有生成完整快照文件');
    const outputs = [original];
    for (let n = 1; n !== 3; ++n)
        outputs.push(await run([scenario, '--load-file', snapshot,
            ...stopArgs, '--trace-file', traces[n]]));
    const bytes = await Promise.all(traces.map(p => fs.readFile(p)));
    const lines = bytes[0].toString('utf8').trimEnd().split('\n');
    const lastFrame = Number(lines.at(-1).split(' ')[0]);
    if (!Number.isSafeInteger(lastFrame) || lastFrame <= saveAt ||
        lines.length !== lastFrame - saveAt || (stopAt !== undefined && lastFrame !== stopAt))
        throw new Error(`逐帧摘要数量不符：${lines.length}`);
    for (let n = 0; n < lines.length; ++n)
        if (!lines[n].startsWith(`${saveAt + n + 1} `))
            throw new Error(`摘要帧顺序不符：${n}`);
    for (let n = 1; n !== 3; ++n)
        if (!bytes[0].equals(bytes[n])) throw new Error(`第 ${n} 次跨进程全状态尾段与本轮reference不同`);
    const tailOutput = output => output.split(/\r?\n/).filter(line => {
        if (line.startsWith('natural replay periodic saved ')) return false;
        const frame = line.match(/\bframe=(\d+)\b/);
        return frame ? Number(frame[1]) > saveAt : /^\d+ checks passed$/.test(line);
    }).join('\n');
    const expectedOutput = tailOutput(outputs[0]);
    if (!expectedOutput.includes(`frame=${lastFrame} `)) throw new Error('缺少真实尾段终点日志');
    if (stopAt === undefined && !expectedOutput.includes(scenario === 'natural_expansion'
        ? 'expansion summary expanded_month=' : 'progression summary completed='))
        throw new Error('完整模式没有达到对应场景原自然终点');
    const golden = scenario === 'natural_expansion'
        ? /expansion summary[^\n]*frame=111808\b[^\n]*cash=138463\b[^\n]*random=1047804\b/
        : /progression summary[^\n]*frame=38282\b[^\n]*cash=23388\b[^\n]*random=322697\b/;
    if (stopAt === undefined && !golden.test(expectedOutput))
        throw new Error('完整自然链偏离对应场景既有 seed1/speed0 黄金终点');
    for (let n = 1; n !== 3; ++n)
        if (tailOutput(outputs[n]) !== expectedOutput) throw new Error(`第 ${n} 次尾段输出或检查计数不同`);
    if (!(await fs.readFile(snapshot)).equals(snapshotBytes))
        throw new Error('只读回放修改了源快照');
    if (sourcePrefix && !(await fs.readFile(sourcePrefix.file)).equals(sourcePrefixBytes))
        throw new Error('本轮续跑修改了冻结源前缀');
    if (options.has('--snapshot-file')) {
        const destination = path.resolve(options.get('--snapshot-file'));
        await checkedBuildPath(destination, '保留快照');
        // 只有三路验证完成才发布复用快照，独占创建避免覆盖既有有效证据。
        await fs.mkdir(path.dirname(destination), { recursive: true });
        await fs.writeFile(destination, snapshotBytes, { flag: 'wx' });
    }
    const certificate = { scenario, seed: 1, speed: 0,
        reference_origin: sourcePrefix ? '恢复既有前缀后继续' : '真实新局不中断继续',
        certification_level: sourcePrefix?.source_status === 'candidate' ? 'candidate_reference_tail'
            : sourcePrefix ? 'resumed_reference_tail' : 'new_game_reference_tail',
        source_prefix: sourcePrefix ?? null,
        producer_revision: producer, save_at: saveAt, stop_at: lastFrame,
        periodic_interval: saveEvery ?? null, periodic_directory: periodicDirectory ?? null,
        periodic_status: periodicDirectory ? '仅生成并读器自检通过的候选，未独立尾段认证' : null,
        preserved_snapshot: options.get('--snapshot-file') ?? null,
        completion: stopAt === undefined ? '原自然终点' : '有界尾段',
        certification: '本证书仅覆盖指定场景主快照与所列尾段',
        tail_frames: lines.length, process_count: 3,
        snapshot_bytes: snapshotBytes.length, snapshot_sha256: sha256(snapshotBytes),
        trace_bytes: bytes[0].length, trace_sha256: sha256(bytes[0]),
        tail_output: expectedOutput, process_wall_seconds: processSeconds,
        comparison: '逐帧全Session和Driver及尾段stdout三路相同',
        certification_boundary: sourcePrefix
            ? '本轮仅认证源恢复reference→新快照→双恢复尾段；不是本轮完整从新局无中断认证'
            : '本轮reference从真实新局不中断至终点，双恢复仅认证主快照之后尾段' };
    if (options.has('--snapshot-file'))
        await fs.writeFile(path.resolve(options.get('--snapshot-file')) + '.json',
            JSON.stringify(certificate, null, 2) + '\n', { flag: 'wx' });
    console.log(JSON.stringify(certificate, null, 2));
    passed = true;
} finally {
    process.removeListener('SIGINT', cancel);
    process.removeListener('SIGTERM', cancel);
    if (sourcePrefix && !(await fs.readFile(sourcePrefix.file)).equals(sourcePrefixBytes))
        throw new Error('收口源前缀hash改变，保留现场');
    if (sourcePrefix?.certificate_file &&
        sha256(await fs.readFile(sourcePrefix.certificate_file)) !== sourcePrefix.certificate_sha256)
        throw new Error('收口源证书hash改变，保留现场');
    if (child) throw new Error('子进程尚未关闭，保留临时目录以免误删使用中文件');
    // owned来自mkdtemp，且必须仍在已核对的工作根下，不能回收传入根目录本身。
    if (path.dirname(owned) !== workRoot) throw new Error('临时目录范围校验失败');
    if (passed) {
        await checkedBuildPath(owned, '待回收临时目录');
        const actualOwned = await fs.realpath(owned);
        const actualWork = await fs.realpath(workRoot);
        if (path.dirname(actualOwned) !== actualWork)
            throw new Error('临时目录实际范围校验失败');
        await fs.rm(owned, { recursive: true, force: true });
    }
    else console.error(`失败现场保留：${owned}`);
    if (periodicDirectory) console.log(`周期前缀独立保留：${periodicDirectory}`);
}
