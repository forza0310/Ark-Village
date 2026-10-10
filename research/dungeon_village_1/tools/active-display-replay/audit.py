"""登记主动候选接续的实际源码、短证书与存储规模；不读取原游戏档。"""
# 读取按现存工具／归档位置解析历史路径；本轮候选输出只留在本地work。
import sys as _archive_sys
from pathlib import Path as _ArchivePath
_archive_sys.dont_write_bytecode = True
_archive_sys.path.insert(0, str(_ArchivePath(__file__).resolve().parents[1]))
from work_archive_paths import archive_input, archive_work, archive_output
from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parents[2]
out = archive_work(__file__)

def item(relative):
    data = (archive_input(root / relative)).read_bytes()
    return {"path": relative, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}

sources = [
    "prototype/tests/startup_application_active_replay.cpp",
    "prototype/tests/application_active_process.mjs",
    "prototype/tests/replay_file_test.mjs",
    "prototype/tests/APPLICATION_PROCESS.md",
    "work/active-progression-plan/ACTIVE_DRIVER.md",
    "work/active-display-replay/README.md",
    "work/active-display-replay/audit.py",
]
certificate_path = "work/snapshots/active-application-v1/frame20002.avra.json"
certificate = json.loads((archive_input(root / certificate_path)).read_text(encoding="utf-8"))
snapshot = item("work/snapshots/active-application-v1/frame20002.avra")
assert snapshot["sha256"] == certificate["snapshot_sha256"]
assert snapshot["bytes"] == certificate["snapshot_bytes"]
assert certificate["process_count"] == 3 and certificate["tail_frames"] == 100
assert certificate["capture_frame"] == 20002 and certificate["stop_at"] == 20102
assert certificate["uncertified_history"]["snapshot_sha256"] == "e6452b6d5b7ab9d9124ffde656144f1f4780c0a19538eb5f1c07a74b32978218"
candidate = item("work/active-application-later/application-active-5D8HYk/process-0/prefix.avra")
assert candidate["sha256"] == certificate["uncertified_history"]["snapshot_sha256"]
release_files = [p for p in (root / "work/release").rglob("*") if archive_input(p).is_file()]
record = {
    "baseline": "131d8ff", "files": [item(p) for p in sources],
    "source_candidate": candidate, "snapshot": snapshot, "certificate": item(certificate_path),
    "result": certificate,
    "checks": {"release_build": "passed", "pages_task_display_persistence_seconds": 31.12,
               "persistence_seconds": 31.11, "replay_process_before_candidate_tests_seconds": 53.96,
               "replay_process_final_seconds": 55.30},
    "storage": {"release_file_count": len(release_files),
                "release_bytes": sum(archive_input(p).stat().st_size for p in release_files),
                "new_source_text_bytes": sum((archive_input(root / p)).stat().st_size for p in sources),
                "new_asset_bytes": 0},
    "boundary": "仅新捕获后100轮认证；候选此前历史不追认；无原窗口或首星完成结论",
    "retention": "成功临时三进程根/trace已消费回收；失败现场及被引用快照保留；合法历史不删",
    "processes": "本批构建与测试均退出；后继30000采样不纳入本批结果",
}
(archive_output(out / "VALIDATION.json")).write_text(json.dumps(record, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
print(json.dumps({"snapshot_bytes": snapshot["bytes"], "release_bytes": record["storage"]["release_bytes"]}))
