// A process-boundary build contract needs isolated real DLLs. It cannot be covered
// by a world-rule fixture, and never overwrites the active build/bin library set.
import assert from 'node:assert/strict';
import { mkdtempSync, mkdirSync, writeFileSync, readFileSync, rmSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { contractHeader, fingerprint, run } from '../../scripts/shared_library_contract.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const scratch = path.join(root, 'build/validation/shared-library-contract');
mkdirSync(scratch, {recursive: true});
const temporary = mkdtempSync(path.join(scratch, 'case-'));
try {
    const library = path.join(temporary, 'library');
    mkdirSync(library);
    mkdirSync(path.join(temporary, 'include'));
    mkdirSync(path.join(temporary, 'src'));
    mkdirSync(path.join(temporary, 'assets'));
    writeFileSync(path.join(temporary, 'include/world.hpp'), 'struct Owner { int value; };\n');
    writeFileSync(path.join(temporary, 'source.cpp'), 'int rule() { return 1; }\n');
    const dll = path.join(library, 'fixture.dll');
    const importLibrary = path.join(library, 'fixture.dll.a');
    writeFileSync(dll, 'first compiled fixture');
    writeFileSync(importLibrary, 'matching import library');
    const spec = {sources: ['source.cpp'], artifacts: [dll, importLibrary], modules: ['fixture.dll'],
        desktopGlyphs: true, settings: 'compiler-1|x64|Release'};
    writeFileSync(path.join(library, 'ArkLibraryInputs.json'), JSON.stringify(spec));
    const initial = fingerprint(temporary, spec);
    run(['prepare', temporary, library]);
    assert.throws(() => run(['check', temporary, library]), /ENOENT/);
    run(['publish', temporary, library]);
    run(['check', temporary, library]);
    writeFileSync(path.join(temporary, 'README.md'), 'Documentation only.\n');
    assert.equal(fingerprint(temporary, spec), initial, 'documentation must not invalidate a consumer');
    writeFileSync(path.join(temporary, 'src/consumer.cpp'), '// consumer-only text input: 魔法壶\n');
    assert.notEqual(fingerprint(temporary, spec), initial, 'generated resource header inputs must invalidate consumers');
    assert.throws(() => run(['check', temporary, library]), /sources\/settings changed/);
    rmSync(path.join(temporary, 'src/consumer.cpp'));
    assert.equal(fingerprint(temporary, spec), initial);
    for (const extension of ['inc', 'ipp']) {
        const fragment = path.join(temporary, 'src/private_fields.' + extension);
        writeFileSync(fragment, 'FIELD(original)\n');
        const added = fingerprint(temporary, spec);
        assert.notEqual(added, initial, 'new private include fragments must invalidate consumers');
        assert.throws(() => run(['check', temporary, library]), /sources\/settings changed/);
        writeFileSync(fragment, 'FIELD(changed)\n');
        assert.notEqual(fingerprint(temporary, spec), added, 'private fragment content must enter the identity');
        rmSync(fragment);
        assert.equal(fingerprint(temporary, spec), initial);
    }
    writeFileSync(importLibrary, 'mismatched import library');
    assert.throws(() => run(['check', temporary, library]), /replaced or from an incomplete build/);
    writeFileSync(importLibrary, 'matching import library');
    run(['check', temporary, library]);
    writeFileSync(path.join(temporary, 'include/world.hpp'), 'struct Owner { int value; int added; };\n');
    assert.notEqual(fingerprint(temporary, spec), initial, 'public layout changes must invalidate consumers');
    assert.throws(() => run(['check', temporary, library]), /sources\/settings changed/);
    assert.throws(() => run(['publish', temporary, library]), /changed during compilation/);
    const incomplete = readFileSync(path.join(library, 'ArkLibraryContract.json'), 'utf8');
    run(['prepare', temporary, library]);
    writeFileSync(dll, 'partially rebuilt fixture');
    assert.throws(() => run(['check', temporary, library]), /sources\/settings changed/);
    assert.equal(readFileSync(path.join(library, 'ArkLibraryContract.json'), 'utf8'), incomplete);
    run(['publish', temporary, library]);
    run(['check', temporary, library]);
    writeFileSync(dll, 'a replaced same-version DLL');
    assert.throws(() => run(['check', temporary, library]), /replaced or from an incomplete build/);
    writeFileSync(path.join(temporary, 'stale.exe'), 'unsafe post-link output');
    assert.throws(() => run(['check-executable', temporary, library, path.join(temporary, 'stale.exe')]), /replaced/);
    assert.throws(() => readFileSync(path.join(temporary, 'stale.exe')), /ENOENT/);

    if (process.platform === 'win32') {
        const compiler = process.argv[2];
        assert.ok(compiler, 'Windows native contract checks need the configured C++ compiler');
        const env = {...process.env, PATH: path.dirname(compiler) + path.delimiter + path.join(root, 'build/bin') + path.delimiter + (process.env.PATH ?? '')};
        function compile(args) {
            const result = spawnSync(compiler, ['-std=c++17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', ...args],
                {cwd: temporary, env, encoding: 'utf8', windowsHide: true});
            assert.equal(result.status, 0, result.error?.message ?? result.stdout + result.stderr);
        }
        const module = 'guard_fixture.dll';
        writeFileSync(path.join(temporary, 'ark_library_contract.hpp'), contractHeader('a'.repeat(64), [module, 'unshipped_maintenance.dll']));
        // Existing product APIs have no dllexport annotation. MinGW must retain
        // those exports even with the explicitly exported C contract beside them.
        writeFileSync(path.join(temporary, 'library.cpp'), 'extern "C" int fixture_value() { return 42; }\n');
        writeFileSync(path.join(temporary, 'main.cpp'), '#include <cstdio>\nextern "C" __declspec(dllimport) int fixture_value();\nstruct BusinessGlobal { BusinessGlobal() { std::printf("GLOBAL %d\\n", fixture_value()); } };\nBusinessGlobal world;\nint main() { std::printf("MAIN %d\\n", fixture_value()); }\n');
        const identitySource = path.join(root, 'src/app/bootstrap/shared_library_identity.cpp');
        const guardSource = path.join(root, 'src/app/bootstrap/shared_library_guard.cpp');
        function buildLibrary(withIdentity = true) {
            compile(['-shared', '-Wl,--export-all-symbols', '-I.', 'library.cpp', ...(withIdentity ? [identitySource] : []),
                '-Wl,--out-implib,fixture.dll.a', '-o', module]);
        }
        buildLibrary();
        compile(['-I.', 'main.cpp', guardSource, 'fixture.dll.a', '-o', 'fixture.exe']);
        const execute = () => spawnSync(path.join(temporary, 'fixture.exe'), [], {cwd: temporary, env, encoding: 'utf8', windowsHide: true});
        let result = execute();
        assert.equal(result.status, 0, result.error?.message ?? result.stderr);
        assert.match(result.stdout, /MAIN 42/, 'matching libraries reach main without unrelated DLLs');
        // Keep the EXE unchanged; replace only the isolated DLL, like another
        // consumer rebuilding the single common library set with a new layout.
        writeFileSync(path.join(temporary, 'ark_library_contract.hpp'), contractHeader('b'.repeat(64), [module]));
        buildLibrary();
        result = execute();
        assert.equal(result.status, 78, result.stderr);
        assert.doesNotMatch(result.stdout, /MAIN|GLOBAL/);
        assert.match(result.stderr, /executable and DLL do not match: guard_fixture.dll/);
        assert.match(result.stderr, /build_product.mjs/);
        buildLibrary(false);
        result = execute();
        assert.equal(result.status, 78, result.stderr);
        assert.match(result.stderr, /no contract/);
        assert.doesNotMatch(result.stdout, /MAIN|GLOBAL/);
    }
    console.log('PASS shared library source/artifact identity and isolated startup rejection');
} finally {
    rmSync(temporary, {recursive: true, force: true});
}
