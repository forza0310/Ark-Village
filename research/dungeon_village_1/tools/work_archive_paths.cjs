// 研究工具迁移适配：历史证据中的work路径保持原字节，读取同路径的现存正式文件。
// 不修改全局fs/require。每个工具显式取得局部适配器；新输出只进入自身work专题。
const fs = require('fs');
const path = require('path');
const {createRequire} = require('module');
const childProcess = require('child_process');

const researchRoot = path.resolve(__dirname, '..');
const workRoot = path.join(researchRoot, 'work');
const toolsRoot = path.join(researchRoot, 'tools');
const verificationRoot = path.join(researchRoot, 'verification');
const normalized = p => path.resolve(p).toLowerCase();
const within = (p, directory) => normalized(p) === normalized(directory) ||
    normalized(p).startsWith(normalized(directory) + path.sep);
function workPath(p) {
    for (const root of [toolsRoot, verificationRoot])
        if (within(p, root)) return path.join(workRoot, path.relative(root, p));
    return p;
}
function readCandidates(p) {
    if (!within(p, workRoot)) return [p];
    const relative = path.relative(workRoot, p);
    return [path.join(toolsRoot, relative), path.join(verificationRoot, relative), p];
}

function forTool(toolFile) {
    toolFile = path.resolve(toolFile);
    if (!within(toolFile, toolsRoot) || normalized(toolFile) === normalized(toolsRoot))
        throw Error('研究工具不在tools目录: ' + toolFile);
    const sourceFile = workPath(toolFile);
    const workDir = path.dirname(sourceFile), written = new Set();
    const nativeRequire = createRequire(toolFile);
    const absolute = p => typeof p === 'string' ? path.resolve(p) : p;
    // 本轮新写的候选优先用于写后消费；其他历史引用优先读取冻结归档，
    // 避免旧work副本或后续临时候选无声替换正式历史证据。
    function readPath(p) {
        if (typeof p !== 'string') return p;
        const resolved = absolute(p), candidate = workPath(resolved);
        if (written.has(normalized(candidate))) return candidate;
        return readCandidates(resolved).find(p => fs.existsSync(p)) || resolved;
    }
    function writePath(p) {
        if (typeof p !== 'string') throw Error('研究工具输出必须使用明确路径');
        const result = workPath(absolute(p));
        if (!within(result, workDir)) throw Error('研究工具输出越出本专题work: ' + result);
        // 检查实际已存在的祖先，拒绝以目录链接绕过输出边界。
        let ancestor = path.dirname(result);
        while (!fs.existsSync(ancestor)) {
            const parent = path.dirname(ancestor);
            if (parent === ancestor) throw Error('输出目录没有可验证的祖先');
            ancestor = parent;
        }
        if (!within(fs.realpathSync(ancestor), workRoot))
            throw Error('研究输出祖先不在真实work目录: ' + ancestor);
        return result;
    }
    function ensureParent(p) { fs.mkdirSync(path.dirname(p), {recursive: true}); }
    // 历史工具常按专题readdir。合并work本地输入与归档中的直接子项，
    // 再由readPath逐文件解析；不把少掉已迁报告误算为资源退休。
    function readDirectory(directory, options) {
        const resolved = absolute(directory);
        if (typeof resolved !== 'string') return fs.readdirSync(resolved, options);
        const directories = new Set(readCandidates(resolved).filter(p => fs.existsSync(p)));
        if (!directories.size) return fs.readdirSync(readPath(resolved), options);
        const found = new Map();
        for (const dir of directories)
            for (const item of fs.readdirSync(dir, options)) {
                const name = typeof item === 'string' || Buffer.isBuffer(item) ? item.toString() : item.name;
                if (!found.has(name)) found.set(name, item);
            }
        return [...found.values()].sort((a, b) => String(a.name || a).localeCompare(String(b.name || b)));
    }
    const fileSystem = new Proxy(fs, {get(target, key) {
        if (key === 'readFileSync') return (p, ...args) => fs.readFileSync(readPath(p), ...args);
        if (key === 'existsSync') return p => fs.existsSync(readPath(p));
        if (['statSync', 'lstatSync', 'realpathSync', 'accessSync'].includes(key))
            return (p, ...args) => fs[key](readPath(p), ...args);
        if (key === 'readdirSync') return readDirectory;
        if (key === 'writeFileSync' || key === 'appendFileSync') return (p, ...args) => {
            const out = writePath(p); ensureParent(out); fs[key](out, ...args); written.add(normalized(out));
        };
        if (key === 'mkdirSync') return (p, ...args) => {
            const out = writePath(p); ensureParent(out); return fs.mkdirSync(out, ...args);
        };
        if (key === 'mkdtempSync') return (p, ...args) => {
            const out = writePath(p); ensureParent(out); return fs.mkdtempSync(out, ...args);
        };
        if (key === 'rmSync' || key === 'unlinkSync') return (p, ...args) => fs[key](writePath(p), ...args);
        return Reflect.get(target, key);
    }});
    const processes = new Proxy(childProcess, {get(target, key) {
        if (['execFileSync', 'spawnSync', 'execFile', 'spawn'].includes(key)) return (file, args, ...rest) => {
            const mapArgument = value => {
                if (typeof value !== 'string') return value;
                const full = path.resolve(value), mapped = readPath(full);
                return normalized(full) !== normalized(mapped) ? mapped : value;
            };
            return childProcess[key](mapArgument(file), Array.isArray(args) ? args.map(mapArgument) : args, ...rest);
        };
        return Reflect.get(target, key);
    }});
    function localRequire(specifier) {
        if (specifier === 'fs' || specifier === 'node:fs') return fileSystem;
        if (specifier === 'child_process' || specifier === 'node:child_process') return processes;
        if (specifier.startsWith('.') || path.isAbsolute(specifier))
            return nativeRequire(readPath(path.resolve(workDir, specifier)));
        return nativeRequire(specifier);
    }
    localRequire.resolve = specifier => nativeRequire.resolve(
        specifier.startsWith('.') ? readPath(path.resolve(workDir, specifier)) : specifier);
    return {workDir, sourceFile, require: localRequire, readPath, writePath};
}

module.exports = {forTool};
