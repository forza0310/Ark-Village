# 完整世界构建数据

两份维护编译器读取产品`assets/simulation`，验证固定原表、APK身份、表尺寸和交叉引用，生成标准C++只读目录与脚本。
Node只用于构建。脚本不会提取APK，也不会在构建时更新research或产品输入。

显式升级使用上一级`import_world_research.mjs`：先`--snapshot`冻结维护源码/测试/数据，再`--import`导入产品；
源与产品哈希记录于`assets/simulation/SOURCES.json`。`verify_sources.mjs`无需research即可校验全部副本。
命名空间、include路径与测试数据路径是机械适配，规则算法与回归断言保持维护版本。
