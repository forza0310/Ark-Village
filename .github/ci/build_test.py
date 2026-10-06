"""Packaging contracts without a compiler or graphics context."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location("ark_ci_build", Path(__file__).with_name("build.py"))
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


class RuntimePackageTests(unittest.TestCase):
    def test_transitive_closure_cycle_and_unused_libraries(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            binaries, stage = root / "bin", root / "stage"
            binaries.mkdir()
            stage.mkdir()
            for name in ("Ark.dll", "leaf.dll", "unused_test.dll"):
                (binaries / name).write_bytes(name.encode("ascii"))
            executable = stage / "player.exe"
            graph = {
                "player.exe": ["ark.dll", "kernel32.dll"],
                "ark.dll": ["leaf.dll", "api-ms-win-crt-runtime-l1-1-0.dll"],
                "leaf.dll": ["ark.dll", "ucrtbase.dll"],
            }
            with patch.object(build, "imports", side_effect=lambda image: graph[image.name.lower()]), \
                    patch.object(build, "check_architecture") as architecture, \
                    patch.object(build, "run") as strip:
                copied = build.copy_runtime_dependencies(executable, binaries, stage)
            self.assertEqual(copied, {"ark.dll", "leaf.dll"})
            self.assertEqual({path.name for path in stage.iterdir()}, {"Ark.dll", "leaf.dll"})
            for path in stage.iterdir():
                self.assertEqual(path.read_bytes(), (binaries / path.name).read_bytes())
            self.assertEqual(architecture.call_count, 2)
            self.assertEqual(strip.call_count, 2)

    def test_missing_transitive_or_unknown_system_dependency_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "ark.dll").write_bytes(b"ark")
            stage = root / "stage"
            stage.mkdir()
            for missing in ("missing.dll", "api-ms-unrecognized.dll"):
                with self.subTest(dependency=missing):
                    graph = {"player.exe": ["ark.dll"], "ark.dll": [missing]}
                    with patch.object(build, "imports", side_effect=lambda image: graph[image.name]), \
                            patch.object(build, "check_architecture"), patch.object(build, "run"):
                        with self.assertRaisesRegex(AssertionError, "Unbundled runtime dependency"):
                            build.copy_runtime_dependencies(stage / "player.exe", root, stage)

    def test_non_x64_binary_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            executable = Path(temporary) / "test.dll"
            for machine, magic, accepted in ((0x8664, 0x20B, True), (0x14C, 0x10B, False),
                                              (0xAA64, 0x20B, False), (0x8664, 0x10B, False)):
                with self.subTest(machine=machine, magic=magic):
                    data = bytearray(128)
                    data[:2] = b"MZ"
                    struct.pack_into("<I", data, 0x3C, 64)
                    data[64:68] = b"PE\0\0"
                    struct.pack_into("<H", data, 68, machine)
                    struct.pack_into("<H", data, 88, magic)
                    executable.write_bytes(data)
                    if accepted:
                        build.check_architecture(executable)
                    else:
                        with self.assertRaises(AssertionError):
                            build.check_architecture(executable)


if __name__ == "__main__":
    unittest.main()
