# 构建与素材脚本

需要Node 18+，只消费维护数据，不执行逆向。

- compile_startup.mjs：JSON.parse读取发布包，交叉验证哈希/身份/目录/初值，结合加载后数据生成标准C++只读数据；保留reset边界区域/皮肤0，拒绝与初局加载快照不一致的区域。
- compile_loaded_map.mjs：固定两份TSV哈希，验证14/6列、576格行主序、8实例原向量顺序与全部交叉绑定；不在产品重建加载流程。
- compile_initial_ai.mjs：从已校验原表投影23职业/33武器、首访定义和28/30/33/35/45服务字段；校验数组/范围，不猜其他未接设施效果含义。生成initial_ai_rules供严格初局诊断和正常人物生活临时适配器复用，不生成第二套可变世界。
- import_research.mjs：按显示/SEB绑定复制素材，生成assets/SOURCES.json，不是常规构建步骤。
- verify_assets.mjs：核对产品副本哈希/尺寸，不依赖research运行目录。

导入：`node scripts/import_research.mjs research/dungeon_village_1 assets`。先检查范围/版本，导入后核对差异并重跑验收。
只刷新已发布UI素材：追加`--ui-only`，保留原数据/地图/人物快照和清单，避免research并行推进时静默升级业务输入。
栅栏/外部门柱按BOUNDARY规格导入common的fence01 PNG、三套fence01x SEB及door00 PNG/SEB，帧偏移由renderer与原SEB分层处理，不重裁PNG。
不修改原始PNG/SEB满足校验；未用记录与实际绘制帧分开检查。
