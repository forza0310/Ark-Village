"""Runner-only build/test/package driver; no product or test-source changes."""

import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import struct
import subprocess
import sys
import zipfile


ROOT = Path(__file__).resolve().parents[2]
CI = ROOT / "build" / "ci"
LOGS = CI / "logs"
PRESETS = ("desktop-release",)


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


def check_architecture(executable):
    data = executable.read_bytes()
    assert data[:2] == b"MZ", f"Not a PE binary: {executable}"
    offset = struct.unpack_from("<I", data, 0x3C)[0]
    assert data[offset:offset + 4] == b"PE\0\0", executable
    assert struct.unpack_from("<H", data, offset + 4)[0] == 0x8664, "Expected x86-64 PE"
    assert struct.unpack_from("<H", data, offset + 24)[0] == 0x20B, "Expected PE32+"
    print(f"PASS architecture: {executable.name}", flush=True)


def package(target):
    assert target == "windows10-x64", target
    name = f"ark-village-{target}"
    stage = CI / "package" / name
    # Repeated local packaging removes only this named staging directory, never
    # the build tree or a caller-provided path outside the checkout's build area.
    assert CI.resolve().is_relative_to((ROOT / "build").resolve()), CI
    assert not stage.is_symlink() and stage.resolve().parent == (CI / "package").resolve(), stage
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    binary_dir = ROOT / "build" / "desktop-release" / "bin"
    executable = stage / "ark_village.exe"
    shutil.copy2(binary_dir / executable.name, executable)
    # Only the shipped copy is stripped; tests and their diagnostic binaries stay intact.
    run("package-strip", ["llvm-strip", "--strip-unneeded", executable])
    check_architecture(executable)
    run("imports-ark_village", ["llvm-readobj", "--coff-imports", executable])
    imports = (LOGS / "imports-ark_village.log").read_text(encoding="utf-8").lower()
    dependencies = re.findall(r"^\s+name: (\S+)$", imports, re.MULTILINE)
    assert dependencies, "No PE imports found"
    system_libraries = {"kernel32.dll", "user32.dll", "winmm.dll", "shell32.dll", "gdi32.dll",
                        "opengl32.dll", "advapi32.dll", "ole32.dll", "ntdll.dll", "ucrtbase.dll"}
    for dependency in dependencies:
        assert dependency in system_libraries or (
            dependency.startswith("api-ms-win-crt-") and dependency.endswith(".dll")
        ), f"Unbundled non-system runtime dependency: {dependency}"
    # The immutable simulation catalog is compiled into the executable. Ship the
    # complete audited sprite/data manifest once, without source/build-only data.
    assets = binary_dir / "assets"
    manifest = json.loads((assets / "SOURCES.json").read_text(encoding="utf-8"))
    for entry in manifest["files"]:
        relative = Path(entry["file"])
        assert not relative.is_absolute() and ".." not in relative.parts, relative
        destination = stage / "assets" / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(assets / relative, destination)
    shutil.copy2(assets / "SOURCES.json", stage / "assets" / "SOURCES.json")
    (stage / "fonts").mkdir()
    for generated, packaged in (("default.otf", "default.otf"), ("OFL.txt", "LICENSE.txt")):
        shutil.copy2(binary_dir / "fonts" / packaged, stage / "fonts" / packaged)
        assert (stage / "fonts" / packaged).read_bytes() == (CI / "fonts" / generated).read_bytes(), (
            f"Bundled font/license differs from the verified input: {packaged}"
        )
    shutil.copy2(CI / "fonts" / "SOURCES.json", stage / "fonts" / "SOURCES.json")
    shutil.copy2(CI / "raylib" / "LICENSE", stage / "raylib-LICENSE.txt")
    shutil.copy2(Path(os.environ["ARK_LLVM_ROOT"]) / "LICENSE.TXT", stage / "LLVM-LICENSE.txt")
    revision = os.environ["GITHUB_SHA"]
    instructions = (
        f"Ark-Village — {target}\nCommit: {revision}\n\n"
        "Keep assets/ and fonts/ next to the executable. No build tools or separate raylib installation are needed.\n"
        "ark_village.exe --check validates resources without opening a window.\n"
        "System > Save/Load offers two manual slots from a stable village scene.\n"
        "Saves use %LOCALAPPDATA%/Ark-Village/saves; --save-dir PATH overrides the directory.\n"
        "Closing the game does not save automatically. Original APK and older Ark formats are unsupported.\n\n"
    )
    instructions += (
        "Target: Windows 10+ x64 (UCRT); CI executes on Windows Server 2022.\n"
        "Windows 10 hardware/graphics compatibility has not been verified by this CI.\n"
        "Start ark_village.exe or Run-Ark-Village.cmd. The CJK font is included.\n"
        "raylib and the C++ runtime are statically linked into the executable.\n"
        'Optional font override: ark_village.exe --font "C:\\path\\Chinese.ttf"\n'
    )
    launcher = (
        '@echo off\nsetlocal\ncd /d "%~dp0"\n'
        '"%~dp0ark_village.exe" %*\n'
        'if errorlevel 1 (\n  pause\n  exit /b 1\n)\n'
    )
    (stage / "Run-Ark-Village.cmd").write_bytes(launcher.replace("\n", "\r\n").encode("ascii"))
    (stage / "README.txt").write_text(instructions, encoding="utf-8")
    # Validate only the staged files, from outside the checkout/build working directory.
    smoke = CI / "smoke"
    smoke.mkdir(exist_ok=True)
    run("package-assets", ["node", ROOT / "scripts" / "verify_assets.mjs", stage / "assets"], smoke)
    run("package-check", [executable, "--check"], smoke)
    dist = CI / "dist"
    dist.mkdir(exist_ok=True)
    archive = dist / f"{stage.name}.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as output:
        for path in sorted(stage.rglob("*")):
            if path.is_file():
                output.write(path, path.relative_to(stage.parent))
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (dist / f"{archive.name}.sha256").write_text(f"{digest}  {archive.name}\n", encoding="ascii")


def main():
    if os.environ.get("GITHUB_ACTIONS") != "true":
        raise SystemExit("Builds and tests are restricted to GitHub Actions runners.")
    os.chdir(ROOT)
    LOGS.mkdir(parents=True, exist_ok=True)
    target = os.environ["ARK_CI_TARGET"]
    assert target == "windows10-x64", target
    assert platform.system() == "Windows"
    prefix = CI / "raylib-install"
    toolchain = Path(os.environ["ARK_LLVM_ROOT"]) / "bin"
    os.environ["PATH"] = str(toolchain) + os.pathsep + os.environ["PATH"]
    flags = [
        f"-DCMAKE_C_COMPILER={(toolchain / 'x86_64-w64-mingw32-clang.exe').as_posix()}",
        f"-DCMAKE_CXX_COMPILER={(toolchain / 'x86_64-w64-mingw32-clang++.exe').as_posix()}",
        f"-DCMAKE_RC_COMPILER={(toolchain / 'x86_64-w64-mingw32-windres.exe').as_posix()}",
        "-DCMAKE_EXE_LINKER_FLAGS=-static",
    ]
    # raylib 6.0's .pc omits the embedded GLFW/system dependencies for static linking.
    raylib_libraries = "-lwinmm -lgdi32 -lopengl32"
    run("prepare-font", [
        sys.executable, ROOT / "scripts" / "prepare_windows_font.py",
        "--output-dir", CI / "fonts",
    ])
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
            "cmake", "--preset", preset, "-G", "Ninja", "-DARK_LONG_WORLD_TESTS=OFF",
            f"-DARK_DESKTOP_FONT={(CI / 'fonts' / 'default.otf').as_posix()}",
            f"-DARK_DESKTOP_FONT_LICENSE={(CI / 'fonts' / 'OFL.txt').as_posix()}", *flags,
        ])
        run(f"{preset}-build", ["cmake", "--build", "--preset", preset, "--parallel", "3"])
        run(f"{preset}-test", [
            "ctest", "--preset", preset, "--parallel", "2", "--no-tests=error",
            "--output-junit", LOGS / f"{preset}.xml",
        ])
    package(target)


if __name__ == "__main__":
    main()
