// Rebuild the single Release library set before its configured consumer.
// Configure both presets once with the selected toolchain; this entry does not
// silently select a new compiler, run tests, package a game or touch research.
import { existsSync, readFileSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const [preset, option, parallelText] = process.argv.slice(2);
const consumers = new Set(['desktop-debug', 'desktop-release', 'headless-debug', 'headless-release']);
const parallel = parallelText === undefined ? 4 : Number(parallelText);
if (!consumers.has(preset) || (option !== undefined && option !== '--parallel') ||
    (option === '--parallel' && parallelText === undefined) ||
    process.argv.length > 5 || !Number.isSafeInteger(parallel) || parallel <= 0)
    throw new Error('Usage: node scripts/build_product.mjs <consumer-preset> [--parallel N]');
for (const directory of ['shared-libraries', preset + '-shared'])
    if (!existsSync(path.join(root, 'build', directory, 'CMakeCache.txt')))
        throw new Error('Configure ' + directory + ' from the root with the selected toolchain first.');

// CMake caches the chosen C++ compiler absolutely, but can cache sibling helpers such as
// windres by name. Resolve those from the same configured toolchain, never a different MinGW.
const cache = readFileSync(path.join(root, 'build/shared-libraries/CMakeCache.txt'), 'utf8');
const compiler = cache.match(/^CMAKE_CXX_COMPILER:[^=]+=(.+)$/m)?.[1].trim();
const env = compiler && path.isAbsolute(compiler) && existsSync(compiler)
    ? {...process.env, PATH: path.dirname(compiler) + path.delimiter + (process.env.PATH ?? '')}
    : process.env;

for (const current of ['shared-libraries', preset]) {
    process.stdout.write('Rebuilding ' + current + '\n');
    const result = spawnSync('cmake', ['--build', '--preset', current, '--parallel', String(parallel)], {
        cwd: root, env, stdio: 'inherit', windowsHide: true,
    });
    if (result.error) throw result.error;
    if (result.status !== 0) process.exit(result.status ?? 1);
}
