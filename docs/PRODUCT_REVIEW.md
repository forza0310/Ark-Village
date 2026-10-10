# 产品可维护性进度

[文档索引](README.md) · [当前架构](ARCHITECTURE.md)

原审阅基于e7f441f。本页集中保留发现、实施状态和仍有效的维护要求；已完成批次的验收见[B1](stages/B1-playable-prototype.md)，原始审阅和旧任务台账可从Git历史查阅。

| 发现 | 当前状态 | 入口 |
| --- | --- | --- |
| P2-01 独立simulation配置失效 | 已撤下旧承诺，统一根headless预设 | tests/simulation与CONTRIBUTING |
| P2-02 无限绘制统计 | 已改为仅有界诊断采样 | world_render_statistics |
| P2-03 维护存取混入runtime | 已分离ark_world_persistence；玩家runtime不依赖维护hash | cmake/WorldSimulation |
| P2-04 协调器职责集中 | 已分离inspection/统计；本批移出HUD和页面文本，输入/回执进一步分责待办 | world_view、world_inspection、ui/world_panels |
| P2-05 玩家新增字段漏分类 | 已有Clang AST字段分类防漏，独立于维护完整codec | world_save_policy.json |
| P2-06 历史与现状冲突 | 已有分类索引及历史归档；2026-10-08进一步统一schema3、商品/口碑/魔法壶、举物/建筑提示现状，历史版本原身份保留 | docs/README、ARCHITECTURE |
| P3-01 公开未实现archive API | 已窄化为SHA接口，来源头私有 | include/ark/assets/sha256.hpp |
| P3-02 payload/页族分散 | 仍需随实际功能渐进明确身份；不强行合并不同资格 | world_session、world_commands |
| P3-03 两份字体需求 | 已统一扫描器/清单/运行图集 | compile_desktop_glyphs.mjs |
| P3-04 共享DLL旧库防错 | 内容/产物指纹、消费者构建检查与启动早期C守卫已接；本批本地验收通过，四消费者已首次重建；旧守卫前EXE无法追溯保护 | [本批范围](stages/B1-playable-prototype.md#business-validation-dll-docs) |

当前依赖方向是desktop→session/queries/visuals→runtime→rules；玩家save→runtime，维护persistence→runtime/hash。旧ark_game及其专属领域、桌面入口和测试已经退役，产品只维护持续世界。冻结源不为目录美观重排，私有desktop头不扩为核心公开接口，不按单页新增DLL。

<a id="independent-simulation-entry"></a>

旧独立入口只包含simulation模块，缺少根工程的公共库与Windows初始化，曾因未知CMake命令配置失败；现已明确转向根headless预设，不恢复第二套初始化。相关验收记在[B1](stages/B1-playable-prototype.md#research-save-organization)。

## 剩余维护风险

- **桌面协调器职责集中（P2-04）**：自然预运行、截图与绘制统计已经拆出；窗口生命周期、平台输入、FIFO回执和页面调度仍需随实际功能渐进分责。重点保持generation清理、pending打开屏障、held释放、只读绘制及普通行走插值资格，不为缩短文件预建事件总线。验收归既有会话、菜单、任务和建设套件，补输入竞争与焦点丢失场景及相关窗口即可，不复制领域组合或重跑无关长链。
- **命令与页族身份分散（P3-02）**：通用命令仍共享selection、definition、actor、facility和page等字段。新增命令须明确selection究竟是列表索引、定义ID还是实例ID，并保留观察页与目标身份绑定；有真实调用需求时再引入小范围类型化构造。UI可见性、回执资格和模态更新是不同策略，不能因都使用raw页号就强行合并。回归关注输入路由、绘制接线及迟到命令拒绝。

## 持续维护边界

- 玩家存档字段分类独立于维护完整codec；新Owner字段必须说明保存、沿用、重建或丢弃及其原因，维护格式覆盖不能替代玩家政策检查。
- 线程、事务、随机和父页恢复注释解释约束原因；公共参数标明身份，模块说明保留诊断与正常运行边界。不为简单函数机械补注释。
- 冻结规则/codec按正式研究交付演进，不因文件大小或目录美观重排；保留当前runtime的有效断言和独立oracle。旧Game专属回归随其实现退役，不恢复第二套规则。DLL按依赖职责划分，不给单页新增库。
- 当前状态归架构、reference和TODO，带版本的验收归B1；模块README只保留职责与入口。历史失败不写成当前缺陷，旧验证结果不替代本批检查。
