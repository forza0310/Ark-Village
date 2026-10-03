# 构建与素材脚本

需要Node 18+，只消费维护数据，不执行逆向。

- compile_startup.mjs：JSON.parse读取发布包，交叉验证哈希/身份/目录/初值/种子，生成标准C++只读数据。
- import_research.mjs：按显示/SEB绑定复制素材，生成assets/SOURCES.json，不是常规构建步骤。
- verify_assets.mjs：核对产品副本哈希/尺寸，不依赖research运行目录。

导入：`node scripts/import_research.mjs research/dungeon_village_1 assets`。先检查范围/版本，导入后核对差异并重跑验收。
只刷新已发布UI素材：追加`--ui-only`，保留原数据/地图/人物快照和清单，避免research并行推进时静默升级业务输入。
不修改原始PNG/SEB满足校验；未用记录与实际绘制帧分开检查。
