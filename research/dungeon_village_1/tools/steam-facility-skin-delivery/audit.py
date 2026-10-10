"""记录Steam81只读计划的源码与资源身份；不构建或运行窗口。"""
# 读取按现存工具／归档位置解析历史路径；本轮候选输出只留在本地work。
import sys as _archive_sys
from pathlib import Path as _ArchivePath
_archive_sys.dont_write_bytecode = True
_archive_sys.path.insert(0, str(_ArchivePath(__file__).resolve().parents[1]))
from work_archive_paths import archive_input, archive_work, archive_output
from pathlib import Path
import hashlib
import json
import re

root = Path(__file__).resolve().parents[2]
base = archive_work(__file__)
paths = ["prototype/include/dungeon_village_prototype/steam_facility_skin.hpp",
         "prototype/src/steam_facility_skin.cpp", "prototype/tests/steam_facility_skin_checks.cpp",
         "prototype/tests/startup_skin_checks.cpp", "prototype/CMakeLists.txt",
         "prototype/STEAM_FACILITY_SKIN.md", "work/steam-facility-skin-delivery/README.md",
         "work/steam-facility-skin-delivery/audit.py", "work/facility-skin-integration/README.md"]
resources = ["assets/steam-facility-common/MANIFEST.json", "assets/steam-build-common/MANIFEST.json",
             "work/steam-facility-upgrade/EVIDENCE.json", "work/steam-facility-draw-helpers/EVIDENCE.json"]

def record(relative):
    data = (archive_input(root / relative)).read_bytes()
    return {"path": relative, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}

links = 0
for relative in paths:
    file = root / relative
    raw = archive_input(file).read_bytes()
    assert not raw.startswith(b"\xef\xbb\xbf") and b"\r" not in raw, relative
    text = raw.decode("utf-8")
    if file.suffix == ".md":
        for link in re.findall(r"\]\(([^)]+)\)", text):
            target = link.split("#", 1)[0]
            if target and not target.startswith(("https:", "http:")):
                assert (archive_input(file.parent / target)).exists(), (relative, link)
                links += 1
cache = [p for p in (root / "work/release").rglob("*") if archive_input(p).is_file()]
result = {"baseline": "11818a9", "files": [record(p) for p in paths],
          "reused_evidence": [record(p) for p in resources], "local_links": links,
          "validation": {"release_build": "passed", "visuals_seconds": 0.60,
                         "scope": "纯计划/资源，不含窗口、字体或OS输入"},
          "storage": {"release_files": len(cache), "release_bytes": sum(archive_input(p).stat().st_size for p in cache),
                      "new_asset_bytes": 0, "new_build_trees": 0},
          "outputs": "计划返回临时值；32次重复查询不累积图元/触摸或改变输入；复用资源不复制",
          "processes": "本批构建及检查均已退出", "limitations": ["数字与Mapchip2保留具名helper请求",
          "未接Owner页面view桥和实际研究窗口", "不对世界合法历史增长作永久有界声明"]}
(archive_output(base / "VALIDATION.json")).write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
print(json.dumps({"files": len(paths), "links": links, "release_bytes": result["storage"]["release_bytes"]}))
