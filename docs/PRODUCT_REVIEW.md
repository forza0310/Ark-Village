# 产品可维护性进度

[文档索引](README.md) · [完整审阅基线](stages/history/PRODUCT_REVIEW-20261007.md) · [当前架构](ARCHITECTURE.md)

原审阅基于e7f441f，发现与当时复现证据完整保留在归档；本页跟踪实施状态，避免把历史问题当成当前实现。

| 发现 | 当前状态 | 入口 |
| --- | --- | --- |
| P2-01 独立simulation配置失效 | 已撤下旧承诺，统一根headless预设 | tests/simulation与CONTRIBUTING |
| P2-02 无限绘制统计 | 已改为仅有界诊断采样 | world_render_statistics |
| P2-03 维护存取混入runtime | 已分离ark_world_persistence；玩家runtime不依赖维护hash | cmake/WorldSimulation |
| P2-04 协调器职责集中 | 已分离inspection/统计；本批移出HUD和页面文本，输入/回执进一步分责待办 | world_view、world_inspection、ui/world_panels |
| P2-05 玩家新增字段漏分类 | 已有Clang AST字段分类防漏，独立于维护完整codec | world_save_policy.json |
| P2-06 历史与现状冲突 | 本批分类索引、B1/TODO/审阅归档并修正无存档旧结论 | docs/README、ARCHITECTURE |
| P3-01 公开未实现archive API | 已窄化为SHA接口，来源头私有 | include/ark/assets/sha256.hpp |
| P3-02 payload/页族分散 | 仍需随实际功能渐进明确身份；不强行合并不同资格 | world_session、world_commands |
| P3-03 两份字体需求 | 已统一扫描器/清单/运行图集 | compile_desktop_glyphs.mjs |
| P3-04 共享DLL旧库防错 | 已有先公共库后消费者入口；内容指纹仍待办 | build_product.mjs |

当前依赖方向是desktop→session/queries/visuals→runtime→rules；玩家save→runtime，维护persistence→runtime/hash。旧Game仍是显式诊断，冻结源不为目录美观重排。私有desktop头不扩为核心公开接口；不按单页新增DLL。

<a id="independent-simulation-entry"></a>

旧独立入口问题与复现日志见[原审阅P2-01](stages/history/PRODUCT_REVIEW-20261007.md#independent-simulation-entry)。本批验证统一记在[B1](stages/B1-playable-prototype.md#research-save-organization)。
