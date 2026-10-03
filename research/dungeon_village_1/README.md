# 《冒险迷宫村》一代参考研究

固定输入是汉化重签 APK 1.0.8，身份、哈希、工具与局限见 [证据索引](EVIDENCE.md)。
维护代码是独立 C++17 规则与研究原型，不是反编译实现翻译，也不属于产品主构建。

## 阅读与功能入口

| 需要查什么 | 正文入口 |
| --- | --- |
| 真实新局、首名到访、初期目录、施工与暂停 | [新局与首个可玩切片证据](rules/STARTUP.md)、[可读取数据](data/startup/README.md) |
| 建设交互、实例持久化、全局聚合 | [状态与建设](rules/STATE_CONSTRUCTION.md) |
| 设施字段、占地、两朝向、经营、邻接 | [设施规格](rules/FACILITIES.md) |
| 地图绑定、加权路径、到达身份 | [地图访问](rules/MAP_ACCESS.md) |
| 人物自主调度与原型控制边界 | [人物与寻路](rules/CHARACTERS.md) |
| 类别计划、候选、重复计权、目标格 | [活动选择](rules/ACTIVITY.md) |
| 到达收费、退出效果、生命值显示协议 | [设施使用](rules/FACILITY_USE.md) |
| 道具共享改良、实例事件与延迟计划 | [事件与道具](rules/FACILITY_EFFECTS.md) |
| 即时金币、村子点数、费用与月报 | [周期账本](rules/ACCOUNTING.md) |
| 页面入口、地图/字体/投影与视觉缺口 | [UI 基线](ui/README.md)、[渲染映射](ui/PAGES.md) |
| 原始美术、七个逻辑键、换图方式 | [素材说明](assets/README.md) |
| 原始表与来源 | [数据说明](data/README.md) |
| 实况截图、版本隔离与逐图观察 | [视觉参考](references/README.md) |
| 当前优先级、已确认范围与历史方案 | [阶段入口](stages/README.md) |
| 最新检查、历史记录与复现环境 | [验证入口](VERIFICATION.md) |

原作事实、带警告方法的局部推断、独立设计策略和测试夹具必须分开。
规则子目录按功能合并报告，历史阶段按专题归档；长集合按实际增长继续拆分，不再为每个小函数新建阶段文档。
不保留重复跳转文档；旧路径入口已删除，引用直接指向对应正文。

## 三个独立 C++ 包

- [example](example/README.md)：标准 C++17 规则、接口契约和回归测试；保留明确标记的 R1/早期模拟夹具。
- [tools](tools/README.md)：归档、SEB、设施/道具表的严格解析与素材发布，不处理人物或 UI 业务。
- [prototype](prototype/README.md)：聚合已维护规则与 raylib 画面，供用户交互测试；领域层不依赖窗口。

先按 [原型说明](prototype/README.md)运行已有研究原型。真实新局的源地图/资源/人物/初期目录已有静态交付，
初始化后完整快照、首段行动、建设取消/道路分支、原版 UI 连续交互、完整人物优先级和退出成长仍有缺口。
默认原型已接入源布局、新局资源/目录、首访与施工/模态计数；旧自主访问演示改为显式夹具模式。
首名人物首段AI、最终地图刷新及跨月结算仍受保护，当前不能宣称原版还原完成。
用户截图与固定 APK 版本不同，画面、数值和动作证据分别登记。

## 复现静态分析

从仓库根目录执行，需本地 APK 和 JADX 1.5.6：

```sh
jadx --show-bad-code --comments-level warn \
  --output-dir research/dungeon_village_1/work/decompiled \
  research/dungeon_village_1/maoxianmigongcun.apk
```

生成 Java 和调用图只在忽略的工作目录中，不加入维护源码。更换反编译参数可能改变行号，
不得用带重构警告的输出认证完整分支顺序。各包的独立构建方式及当前结果见 [验证入口](VERIFICATION.md)。
