"""历史工具的只读路径迁移；新输出始终回到本地 work，不重签历史证据。"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
_outputs = set()


def archive_input(value):
    """读取旧work路径时优先找实际工具／归档，原始输入和日志留在work。"""
    path = Path(value).resolve()
    if str(path) in _outputs:
        return path
    if path.is_relative_to(ROOT / "work"):
        relative = path.relative_to(ROOT / "work")
        for directory in ("tools", "verification"):
            candidate = ROOT / directory / relative
            if candidate.is_file():
                return candidate
    # 少量旧工具读取同目录的ARRAYS等材料；新工具目录本身不保存证据。
    if path.is_relative_to(ROOT / "tools") and not path.exists():
        relative = path.relative_to(ROOT / "tools")
        for directory in ("verification", "work"):
            candidate = ROOT / directory / relative
            if candidate.is_file():
                return candidate
    return path


def archive_output(value):
    """本轮生成候选可再读；永不把旧路径写入转向verification。"""
    path = Path(value).resolve()
    if not path.is_relative_to(ROOT / "work"):
        raise ValueError("研究工具新输出必须位于work：" + str(path))
    _outputs.add(str(path))
    return path


def archive_work(script):
    """工具主题保持同深度，候选报告只写 work/<topic>。"""
    topic = Path(script).resolve().parent.name
    output = ROOT / "work" / topic
    output.mkdir(parents=True, exist_ok=True)
    return output
