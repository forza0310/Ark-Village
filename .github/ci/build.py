"""Runner-only build/test/package driver; no product or test-source changes."""

import hashlib
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess
import tarfile
import zipfile


ROOT = Path(__file__).resolve().parents[2]
CI = ROOT / "build" / "ci"
LOGS = CI / "logs"
PRESETS = ("desktop-debug", "desktop-release")


def run(label, args, cwd=ROOT):
    """Stream output to Actions and retain the exact failing command's log."""
    print(f"\n::group::{label}", flush=True)
    print(subprocess.list2cmdline([str(arg) for arg in args]), flush=True)
    try:
        with (LOGS / f"{label}.log").open("w", encoding="utf-8") as log:
            with subprocess.Popen(
                [str(arg) for arg in args], cwd=cwd, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace",
            ) as process:
                for line in process.stdout:
                    print(line, end="", flush=True)
                    log.write(line)
                if process.wait():
                    raise subprocess.CalledProcessError(process.returncode, args)
    finally:
        print("::endgroup::", flush=True)


def check_architecture(executable, windows):
    data = executable.read_bytes()
    if windows:
        assert data[:2] == b"MZ", f"Not a PE binary: {executable}"
        offset = struct.unpack_from("<I", data, 0x3C)[0]
        assert data[offset:offset + 4] == b"PE\0\0", executable
        assert struct.unpack_from("<H", data, offset + 4)[0] == 0x14C, "Expected x86 PE"
        assert struct.unpack_from("<H", data, offset + 24)[0] == 0x10B, "Expected PE32"
    else:
        assert struct.unpack_from("<II", data) == (0xFEEDFACF, 0x100000C), "Expected ARM64 Mach-O"
    print(f"PASS architecture: {executable.name}", flush=True)


def package(target, windows):
    name = f"ark-village-{target}"
    stage = CI / "package" / name
    stage.mkdir(parents=True)
    suffix = ".exe" if windows else ""
    for name in ("ark_village", "ark_world_simulation"):
        executable = stage / f"{name}{suffix}"
        shutil.copy2(ROOT / "build" / "desktop-release" / "bin" / executable.name, executable)
        check_architecture(executable, windows)
        if windows:
            run(f"imports-{name}", ["llvm-readobj", "--coff-imports", executable])
            imports = (LOGS / f"imports-{name}.log").read_text(encoding="utf-8").lower()
            for dependency in ("libc++", "libunwind", "libwinpthread", "libgcc", "raylib.dll"):
                assert dependency not in imports, f"Unbundled runtime dependency: {dependency}"
        else:
            run(f"imports-{name}", ["otool", "-L", executable])
            imports = (LOGS / f"imports-{name}.log").read_text(encoding="utf-8")
            for line in imports.splitlines()[1:]:
                assert line.strip().startswith(("/usr/lib/", "/System/Library/")), line
    shutil.copytree(ROOT / "build" / "desktop-release" / "bin" / "assets", stage / "assets")
    shutil.copy2(CI / "raylib" / "LICENSE", stage / "raylib-LICENSE.txt")
    revision = os.environ["GITHUB_SHA"]
    instructions = (
        f"Ark-Village — {target}\nCommit: {revision}\n\n"
        "Keep assets/ next to the executable. Node, CMake and raylib installations are not needed.\n"
        "ark_village --check validates resources without opening a window.\n"
        "The game currently does not save progress.\n\n"
    )
    if windows:
        instructions += (
            "Target: Windows 10 x86 (32-bit, UCRT); CI executes on Windows Server 2022.\n"
            "Windows 10 hardware/graphics compatibility has not been verified by this CI.\n"
            "Start with Run-Ark-Village.cmd. It uses an installed Chinese system font.\n"
            "If none is installed, supply a Chinese TTF/TTC yourself:\n"
            '  ark_village.exe --font "C:\\path\\Chinese.ttf"\n'
        )
        launcher = (
            '@echo off\nsetlocal\ncd /d "%~dp0"\n'
            'for %%F in (msyh.ttc simhei.ttf simsun.ttc) do (\n'
            '  if exist "%WINDIR%\\Fonts\\%%F" (\n'
            '    "%~dp0ark_village.exe" --font "%WINDIR%\\Fonts\\%%F" %*\n'
            '    exit /b\n  )\n)\n'
            'echo No Chinese system font found. See README.txt for the --font option.\npause\nexit /b 1\n'
        )
        (stage / "Run-Ark-Village.cmd").write_bytes(launcher.replace("\n", "\r\n").encode("ascii"))
    else:
        instructions += (
            "Target: macOS 11+ on Apple Silicon. Run ./ark_village from Terminal.\n"
            "Uses the system Arial Unicode font; --font can select another Chinese TTF.\n"
            "This archive is not signed or notarized.\n"
        )
    (stage / "README.txt").write_text(instructions, encoding="utf-8")
    # Validate only the staged files, from outside the checkout/build working directory.
    smoke = CI / "smoke"
    smoke.mkdir()
    run("package-assets", ["node", ROOT / "scripts" / "verify_assets.mjs", stage / "assets"], smoke)
    run("package-check", [stage / f"ark_village{suffix}", "--check"], smoke)
    run("package-world-help", [stage / f"ark_world_simulation{suffix}", "--help"], smoke)
    dist = CI / "dist"
    dist.mkdir()
    if windows:
        archive = dist / f"{stage.name}.zip"
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as output:
            for path in sorted(stage.rglob("*")):
                if path.is_file():
                    output.write(path, path.relative_to(stage.parent))
    else:
        # tar preserves the executable bit inside the Actions artifact ZIP.
        archive = dist / f"{stage.name}.tar.gz"
        with tarfile.open(archive, "w:gz") as output:
            output.add(stage, arcname=stage.name)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (dist / f"{archive.name}.sha256").write_text(f"{digest}  {archive.name}\n", encoding="ascii")


def main():
    if os.environ.get("GITHUB_ACTIONS") != "true":
        raise SystemExit("Builds and tests are restricted to GitHub Actions runners.")
    os.chdir(ROOT)
    LOGS.mkdir(parents=True, exist_ok=True)
    target = os.environ["ARK_CI_TARGET"]
    windows = target == "windows10-x86"
    assert target in ("windows10-x86", "macos-arm64"), target
    prefix = CI / "raylib-install"
    flags = []
    if windows:
        assert platform.system() == "Windows"
        toolchain = Path(os.environ["ARK_LLVM_ROOT"]) / "bin"
        os.environ["PATH"] = str(toolchain) + os.pathsep + os.environ["PATH"]
        flags += [
            f"-DCMAKE_C_COMPILER={(toolchain / 'i686-w64-mingw32-clang.exe').as_posix()}",
            f"-DCMAKE_CXX_COMPILER={(toolchain / 'i686-w64-mingw32-clang++.exe').as_posix()}",
            f"-DCMAKE_RC_COMPILER={(toolchain / 'i686-w64-mingw32-windres.exe').as_posix()}",
            "-DCMAKE_EXE_LINKER_FLAGS=-static",
        ]
        # raylib 6.0's .pc omits the embedded GLFW/system dependencies for static linking.
        raylib_libraries = "-lwinmm -lgdi32 -lopengl32"
    else:
        assert platform.system() == "Darwin" and platform.machine() == "arm64"
        flags += ["-DCMAKE_OSX_ARCHITECTURES=arm64", "-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0"]
        raylib_libraries = "-framework Cocoa -framework IOKit -framework CoreFoundation -framework OpenGL"
    os.environ["PKG_CONFIG_PATH"] = (prefix / "lib" / "pkgconfig").as_posix()
    run("raylib-configure", [
        "cmake", "-S", CI / "raylib", "-B", CI / "raylib-build", "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_EXAMPLES=OFF", "-DBUILD_SHARED_LIBS=OFF",
        "-DCMAKE_INSTALL_LIBDIR=lib", f"-DCMAKE_INSTALL_PREFIX={prefix.as_posix()}",
        f"-DPKG_CONFIG_LIBS_EXTRA={raylib_libraries}", *flags,
    ])
    run("raylib-build", ["cmake", "--build", CI / "raylib-build", "--parallel", "3"])
    run("raylib-install", ["cmake", "--install", CI / "raylib-build"])
    for preset in PRESETS:
        run(f"{preset}-configure", [
            "cmake", "--preset", preset, "-G", "Ninja", "-DARK_LONG_WORLD_TESTS=OFF", *flags,
        ])
        run(f"{preset}-build", ["cmake", "--build", "--preset", preset, "--parallel", "3"])
        run(f"{preset}-test", [
            "ctest", "--preset", preset, "--parallel", "2", "--no-tests=error",
            "--output-junit", LOGS / f"{preset}.xml",
        ])
    package(target, windows)


if __name__ == "__main__":
    main()
