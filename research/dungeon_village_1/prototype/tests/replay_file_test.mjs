// 同一continuous程序的进程隔离场景：真实前缀保存后继续，另起两进程恢复同一尾段。
// 临时目录由本脚本创建和回收，原输入/用户目录不递归删除；任何失败收齐子进程。
import fs from 'node:fs/promises';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';

const researchRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const args = process.argv.slice(2);
const options = new Map();
for (let i = 0; i < args.length; i += 2) {
    if (i + 1 === args.length || options.has(args[i]) ||
        !['--exe', '--work-dir', '--save-at', '--stop-at', '--producer-revision',
            '--snapshot-file', '--save-every', '--save-directory'].includes(args[i]))
        throw new Error('需要 --exe <程序> --work-dir <研究工作目录> [--save-at 420 --stop-at 840]');
    options.set(args[i], args[i + 1]);
}
if (!options.get('--exe') || !options.get('--work-dir'))
    throw new Error('缺少 --exe 或 --work-dir');
const executable = path.resolve(options.get('--exe'));
const workRoot = path.resolve(options.get('--work-dir'));
const relativeWork = path.relative(researchRoot, workRoot);
if (!relativeWork || relativeWork.startsWith('..') || path.isAbsolute(relativeWork))
    throw new Error('回放临时目录必须位于本研究包内的独立工作目录');
const number = (name, fallback) => {
    const text = options.get(name) ?? String(fallback);
    if (!/^\d+$/.test(text)) throw new Error(`非法帧参数 ${name}`);
    const value = Number(text);
    if (!Number.isSafeInteger(value) || value >= 180000) throw new Error(`帧超范围 ${name}`);
    return value;
};
const saveAt = number('--save-at', 420);
const stopAt = options.get('--stop-at') === 'complete' ? undefined : number('--stop-at', 840);
if (stopAt !== undefined && stopAt <= saveAt) throw new Error('尾段终点必须晚于完整前缀保存轮');
const stopArgs = stopAt === undefined ? [] : ['--stop-at', String(stopAt)];
const producer = options.get('--producer-revision') ?? 'unspecified';
if (options.has('--save-every') !== options.has('--save-directory'))
    throw new Error('--save-every与--save-directory必须同时指定');
const saveEvery = options.has('--save-every') ? number('--save-every', 0) : undefined;
if (saveEvery === 0) throw new Error('周期保存间隔必须为正数');
let periodicDirectory;
if (saveEvery !== undefined) {
    periodicDirectory = path.resolve(options.get('--save-directory'));
    const researchWork = path.join(researchRoot, 'work');
    const relative = path.relative(researchWork, periodicDirectory);
    if (!relative || relative.startsWith('..') || path.isAbsolute(relative))
        throw new Error('周期档目录必须是research工作目录内的独立子目录');
    // 先核对最接近的现存祖先，拒绝经junction/symlink逃逸到研究目录外再创建。
    let ancestor = periodicDirectory;
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
    const realWork = await fs.realpath(researchWork);
    const actualRelative = path.relative(realWork, resolved);
    if (actualRelative.startsWith('..') || path.isAbsolute(actualRelative))
        throw new Error('周期档现存祖先位于research/work外');
    await fs.mkdir(periodicDirectory, { recursive: true });
    for (let frame = saveEvery; frame <= (stopAt ?? 179999); frame += saveEvery) {
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
const sha256 = bytes => createHash('sha256').update(bytes).digest('hex');
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
    const original = await run(['natural_progression', '1', '0', '--save-at', String(saveAt),
        '--save-file', snapshot, ...stopArgs, '--trace-from', String(saveAt + 1),
        '--trace-file', traces[0], '--producer-revision', producer, ...periodicArgs]);
    const snapshotBytes = await fs.readFile(snapshot);
    if (!snapshotBytes.length) throw new Error('前缀没有生成完整快照文件');
    const outputs = [original];
    for (let n = 1; n !== 3; ++n)
        outputs.push(await run(['natural_progression', '--load-file', snapshot,
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
        if (!bytes[0].equals(bytes[n])) throw new Error(`第 ${n} 次跨进程全状态尾段与未中断轨迹不同`);
    const tailOutput = output => output.split(/\r?\n/).filter(line => {
        if (line.startsWith('natural replay periodic saved ')) return false;
        const frame = line.match(/\bframe=(\d+)\b/);
        return frame ? Number(frame[1]) > saveAt : /^\d+ checks passed$/.test(line);
    }).join('\n');
    const expectedOutput = tailOutput(outputs[0]);
    if (!expectedOutput.includes(`frame=${lastFrame} `)) throw new Error('缺少真实尾段终点日志');
    if (stopAt === undefined && !expectedOutput.includes('progression summary completed='))
        throw new Error('完整模式没有达到原自然晋级终点');
    if (stopAt === undefined &&
        !/progression summary[^\n]*frame=38282\b[^\n]*cash=23388\b[^\n]*random=322697\b/.test(expectedOutput))
        throw new Error('完整自然晋级/编辑链偏离既有 seed1/speed0 黄金终点');
    for (let n = 1; n !== 3; ++n)
        if (tailOutput(outputs[n]) !== expectedOutput) throw new Error(`第 ${n} 次尾段输出或检查计数不同`);
    if (!(await fs.readFile(snapshot)).equals(snapshotBytes))
        throw new Error('只读回放修改了源快照');
    if (options.has('--snapshot-file')) {
        const destination = path.resolve(options.get('--snapshot-file'));
        const relative = path.relative(researchRoot, destination);
        if (!relative || relative.startsWith('..') || path.isAbsolute(relative))
            throw new Error('保留的快照必须位于本研究包内');
        // 只有三路验证完成才发布复用快照，独占创建避免覆盖既有有效证据。
        await fs.mkdir(path.dirname(destination), { recursive: true });
        await fs.writeFile(destination, snapshotBytes, { flag: 'wx' });
    }
    const certificate = { scenario: 'natural_progression', seed: 1, speed: 0,
        producer_revision: producer, save_at: saveAt, stop_at: lastFrame,
        periodic_interval: saveEvery ?? null, periodic_directory: periodicDirectory ?? null,
        preserved_snapshot: options.get('--snapshot-file') ?? null,
        completion: stopAt === undefined ? '原自然终点' : '有界尾段',
        tail_frames: lines.length, process_count: 3,
        snapshot_bytes: snapshotBytes.length, snapshot_sha256: sha256(snapshotBytes),
        trace_bytes: bytes[0].length, trace_sha256: sha256(bytes[0]),
        tail_output: expectedOutput, process_wall_seconds: processSeconds,
        comparison: '逐帧全Session和Driver及尾段stdout三路相同' };
    if (options.has('--snapshot-file'))
        await fs.writeFile(path.resolve(options.get('--snapshot-file')) + '.json',
            JSON.stringify(certificate, null, 2) + '\n', { flag: 'wx' });
    console.log(JSON.stringify(certificate, null, 2));
    passed = true;
} finally {
    process.removeListener('SIGINT', cancel);
    process.removeListener('SIGTERM', cancel);
    if (child) throw new Error('子进程尚未关闭，保留临时目录以免误删使用中文件');
    // owned来自mkdtemp，且必须仍在已核对的工作根下，不能回收传入根目录本身。
    if (path.dirname(owned) !== workRoot) throw new Error('临时目录范围校验失败');
    if (passed) await fs.rm(owned, { recursive: true, force: true });
    else console.error(`失败现场保留：${owned}`);
    if (periodicDirectory) console.log(`周期前缀独立保留：${periodicDirectory}`);
}
