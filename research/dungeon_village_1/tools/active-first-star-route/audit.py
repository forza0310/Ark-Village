"""显式主动首星证书及其有限来源链审计；不扫描快照目录，不执行游戏。"""
# 读取按现存工具／归档位置解析历史路径；本轮候选输出只留在本地work。
import sys as _archive_sys
from pathlib import Path as _ArchivePath
_archive_sys.dont_write_bytecode = True
_archive_sys.path.insert(0, str(_ArchivePath(__file__).resolve().parents[1]))
from work_archive_paths import archive_input, archive_work, archive_output
from pathlib import Path
import argparse
import hashlib
import json
import re
import struct

ROOT = Path(__file__).resolve().parents[2]
BASE = archive_work(__file__)
WORK = (ROOT / "work").resolve()
CONTROLLER = "application-active-progression-v1"
QUALIFICATION = "active_application_management_tail"
LIMIT = 128 * 1024 * 1024


def need(value, reason):
    if not value:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def local(value):
    path = Path(value)
    path = (path if path.is_absolute() else ROOT / path).resolve(strict=True)
    need(path.is_relative_to(WORK) and archive_input(path).is_file(), "来源不在research/work内：" + str(path))
    return path


class Reader:
    def __init__(self, data):
        self.data, self.at = memoryview(data), 0

    def raw(self, size):
        need(0 <= size <= len(self.data) - self.at, "分区越界")
        result = self.data[self.at:self.at + size]
        self.at += size
        return result

    def u32(self):
        return struct.unpack("<I", self.raw(4))[0]

    def u64(self):
        return struct.unpack("<Q", self.raw(8))[0]

    def text(self):
        size = self.u32()
        need(size <= 4096, "身份文字超限")
        return bytes(self.raw(size)).decode("utf-8")


def identity(path, expected_hash, expected_bytes=None):
    size = archive_input(path).stat().st_size
    need(84 < size <= LIMIT and (expected_bytes is None or size == expected_bytes), "快照长度不符")
    data = archive_input(path).read_bytes()
    need(sha(data) == expected_hash and len(data) == size, "快照字节/摘要不符")
    view = memoryview(data)
    need(sha(view[:-64]) == bytes(view[-64:]).decode("ascii"), "应用总摘要不符")
    r = Reader(view[:-64])
    need(bytes(r.raw(8)) == b"AVRAPP01" and (r.u32(), r.u32(), r.u32()) == (1, 7, 1), "应用语义不符")
    result = dict(dataset=r.text(), world_schema=r.text(), application_schema=r.text())
    count = r.u32()
    need(6 <= count <= 66, "分区数量不符")
    sections = {}
    sizes = {}
    for _ in range(count):
        key, version, required, length = r.u32(), r.u32(), r.u32(), r.u64()
        digest = bytes(r.raw(64)).decode("ascii")
        body = r.raw(length)
        need(key not in sections and version > 0 and required in (0, 1) and sha(body) == digest, "分区身份/摘要不符")
        if 1 <= key <= 6:
            need(version == 1 and required == 1, "必需分区身份不符")
        else:
            need(key >= 1024 and required == 0, "未知必需分区")
        sections[key] = body
        sizes[str(key)] = length
    need(r.at == len(r.data) and all(key in sections for key in range(1, 7)), "应用缺段或尾部")
    meta = Reader(sections[1])
    result.update(controller=meta.text(), producer_revision=meta.text(), next_frame=meta.u64(), next_command=meta.u64())
    need(meta.at == len(meta.data) and result["controller"] == CONTROLLER, "metadata身份不符")
    world = sections[4]
    need(bytes(world[:8]) == b"AVRSAVE1" and struct.unpack("<III", world[8:20]) == (1, 4, 2), "内嵌世界语义/用途不符")
    need(sha(world[:-64]) == bytes(world[-64:]).decode("ascii"), "内嵌世界摘要不符")
    system = sections[2]
    need(bytes(system[:8]) == b"AVRSYS01" and struct.unpack("<I", system[8:12])[0] == 2 and
         sha(system[:-64]) == bytes(system[-64:]).decode("ascii"), "系统2身份/摘要不符")
    result.update(path=str(path.relative_to(ROOT)), bytes=size, sha256=expected_hash, section_bytes=sizes)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("certificates", nargs="+", help="明确列出的证书路径，可相对dungeon_village_1")
    args = parser.parse_args()
    requested = [local(name) for name in args.certificates]
    need(len(requested) <= 16 and len(set(requested)) == len(requested), "证书清单重复或超过本批限界")
    certificates, candidates, frames = {}, {}, {}

    def candidate(source):
        path = local(source["file"])
        actual = identity(path, source["snapshot_sha256"], source.get("snapshot_bytes"))
        for key in ("next_frame", "producer_revision", "dataset", "world_schema", "application_schema"):
            if key in source:
                need(actual[key] == source[key], "候选身份不符：" + key)
        candidates[str(path)] = {"identity": actual, "source_record": source,
                                 "qualification": "unverified_complete_round_candidate"}

    def visit(path, chain):
        need(path not in chain and len(chain) < 32, "来源循环或超过32层")
        if str(path) in certificates:
            return certificates[str(path)]["certificate"]
        raw = archive_input(path).read_bytes()
        need(0 < len(raw) <= 1024 * 1024, "证书大小不符")
        c = json.loads(raw.decode("utf-8-sig"))
        need(c["controller"] == CONTROLLER and c["qualification"] == QUALIFICATION and
             c["seed"] == 1 and c["speed"] == 0 and c["process_count"] == 3, "证书策略/三路身份不符")
        need(c["capture_frame"] >= 1 and 1 <= c["tail_frames"] <= 1000 and
             c["stop_at"] == c["capture_frame"] + c["tail_frames"], "证书轮边界不符")
        need(re.fullmatch(r"[0-9a-f]{64}", c["trace_sha256"]) and c["trace_bytes"] > 0, "证书trace摘要非法")
        snapshot = local(str(path)[:-5])
        actual = identity(snapshot, c["snapshot_sha256"], c["snapshot_bytes"])
        need(actual["next_frame"] == c["capture_frame"] + 1 and actual["section_bytes"] == c["section_bytes"], "捕获轮/分区不符")
        for key in ("producer_revision", "dataset", "world_schema", "application_schema"):
            need(actual[key] == c[key], "证书来源身份不符：" + key)
        end = c["terminal"]
        need(end["controller"] == CONTROLLER and end["completed_frame"] == c["stop_at"] and
             end["next_frame"] == c["stop_at"] + 1 and end["trace_rows"] == c["tail_frames"], "终点身份不符")
        monthly = end.get("progress", {}).get("current_month_facility_income")
        if monthly is not None:
            need(isinstance(monthly, int) and monthly >= 0 and (not end["terminal"] or monthly > 0),
                 "已声明新月收入的终点未满足当月实际收入；旧证书不补造该字段")
        source = c.get("source_prefix")
        if source:
            status = source.get("source_status", "certified")
            if status == "candidate":
                need(source["qualification"] == "unverified_complete_round_candidate" and c.get("uncertified_history"), "候选历史限制缺失")
                candidate(source)
            else:
                need(status == "certified", "未知来源资格")
                parent = local(source["certificate_file"])
                need(sha(archive_input(parent).read_bytes()) == source["certificate_sha256"], "来源证书摘要不符")
                prior = visit(parent, chain + [path])
                need(prior["snapshot_sha256"] == source["snapshot_sha256"] and prior["capture_frame"] == source["capture_frame"] and
                     source["next_frame"] == prior["capture_frame"] + 1 and source["qualification"] == QUALIFICATION and
                     local(source["file"]) == local(str(parent)[:-5]), "来源快照链失配")
                if "rank" in source:
                    need(source["rank"] == prior["capture_rank"], "来源星级身份失配")
                if prior.get("uncertified_history"):
                    need(c.get("uncertified_history") == prior["uncertified_history"], "后继丢失候选历史限制")
            need(source["capture_frame"] < c["capture_frame"], "来源轮未向前")
        if c.get("uncertified_history"):
            candidate(c["uncertified_history"])
        name = "FRAME" + str(c["capture_frame"]) + ".json"
        need(name not in frames or frames[name] == sha(raw), "同轮不同证书不能互相覆盖")
        frames[name] = sha(raw)
        target = BASE / name
        if archive_input(target).exists():
            need(archive_input(target).read_bytes() == raw, "已有归档证书不同，拒绝覆盖")
        else:
            archive_output(target).write_bytes(raw)
        certificates[str(path)] = {"path": str(path.relative_to(ROOT)), "bytes": len(raw), "sha256": sha(raw),
                                    "archived_as": name, "snapshot": actual, "certificate": c}
        return c

    for path in requested:
        visit(path, [])
    result = {"scope": "仅显式证书及其有限来源链，未重跑游戏或三进程",
              "requested_certificates": [str(p.relative_to(ROOT)) for p in requested],
              "application_semantics": 7, "world_semantics": 4, "system_format": 2,
              "certificates": list(certificates.values()), "candidate_history": list(candidates.values()),
              "limitations": ["实际三路比较沿原证书，不把摘要审计说成重跑", "候选之前历史未认证限制原样保留",
                              "未自动挑选后继快照，不认证尚未取得的首星终点", "合法历史增长，不放宽128MiB预算"]}
    source_files = ["prototype/tests/startup_application_active_replay.cpp",
                    "prototype/tests/application_active_process.mjs",
                    "work/active-progression-plan/ACTIVE_DRIVER.md",
                    "work/active-first-star-route/README.md", "work/active-first-star-route/audit.py"]
    result["implementation"] = [{"path": p, "bytes": (archive_input(ROOT / p)).stat().st_size,
                                 "sha256": sha((archive_input(ROOT / p)).read_bytes())} for p in source_files]
    result["recorded_checks"] = {"build": "Release passed", "persistence_seconds": 31.98,
                                 "replay_process_seconds": 54.57,
                                 "note": "本批实跑记录，执行此审计不会重新运行CTest"}
    (archive_output(BASE / "VALIDATION.json")).write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"certificates": len(certificates), "candidate_histories": len(candidates),
                      "archived_certificate_bytes": sum(v["bytes"] for v in certificates.values())}))


if __name__ == "__main__":
    main()
