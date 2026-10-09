"""Package an already validated local Release build with the CI dependency closure."""

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import uuid


ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work-parent", type=Path, required=True)
    parser.add_argument("--toolchain", type=Path, required=True)
    parser.add_argument("--font-dir", type=Path, required=True)
    parser.add_argument("--raylib-license", type=Path, required=True)
    args = parser.parse_args()
    parent = args.work_parent.resolve(strict=True)
    build_root = (ROOT / "build").resolve(strict=True)
    if parent == build_root or not parent.is_relative_to(build_root):
        parser.error("work parent must already exist strictly inside this checkout's build")
    toolchain = args.toolchain.resolve(strict=True)
    font_dir = args.font_dir.resolve(strict=True)
    license_file = args.raylib_license.resolve(strict=True)
    for file in (toolchain / "LICENSE.TXT", toolchain / "bin/llvm-strip.exe",
                 toolchain / "bin/llvm-readobj.exe", font_dir / "default.otf",
                 font_dir / "OFL.txt", font_dir / "SOURCES.json", license_file):
        if not file.is_file():
            parser.error(f"required input missing: {file}")
    # A local artifact identifies its exact commit; do not label dirty product code as that
    # commit. The independent research worktree is deliberately outside this check.
    git = ["git", "-c", f"safe.directory={ROOT.as_posix()}", "-C", str(ROOT)]
    dirty = subprocess.check_output(
        [*git, "status", "--porcelain", "--", ".", ":!research"], text=True)
    if dirty.strip():
        parser.error("commit the validated product checkpoint before packaging")
    revision = subprocess.check_output([*git, "rev-parse", "HEAD"], text=True).strip()
    # Python's Windows mkdtemp uses a private owner-only ACL. A reviewable local
    # artifact must inherit this workspace directory's normal access instead.
    # Exclusive mkdir still refuses a collision without overwriting any output.
    work = parent / ("release-package-" + uuid.uuid4().hex)
    work.mkdir()
    spec = importlib.util.spec_from_file_location("ark_release_package", ROOT / ".github/ci/build.py")
    package = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(package)
    # Reuse packaging only. No CI build entry point, downloads or environment impersonation.
    package.CI = work
    package.LOGS = work / "logs"
    package.LOGS.mkdir()
    for directory in ("fonts", "raylib"):
        (work / directory).mkdir()
    for name in ("default.otf", "OFL.txt", "SOURCES.json"):
        shutil.copy2(font_dir / name, work / "fonts" / name)
    shutil.copy2(license_file, work / "raylib/LICENSE")
    os.environ["ARK_LLVM_ROOT"] = str(toolchain)
    os.environ["GITHUB_SHA"] = revision
    os.environ["PATH"] = str(toolchain / "bin") + os.pathsep + os.environ.get("PATH", "")
    package.package("windows10-x64")
    archive = work / "dist/ark-village-windows10-x64.zip"
    stage = work / "package/ark-village-windows10-x64"
    files = [file for file in stage.rglob("*") if file.is_file()]
    result = {
        "revision": revision, "archive": str(archive), "archive_bytes": archive.stat().st_size,
        "sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
        "unpacked_bytes": sum(file.stat().st_size for file in files), "files": len(files),
        "dlls": sorted(file.name for file in stage.glob("*.dll")),
        "validation": "PE closure, assets and staged --check passed; extraction/window checks separate",
    }
    (work / "RESULT.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
