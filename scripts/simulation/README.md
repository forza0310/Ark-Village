# 完整世界构建数据

两份维护编译器读取产品`assets/simulation`，验证固定原表、APK身份、表尺寸和交叉引用，生成标准C++只读目录与脚本。
Node只用于构建。脚本不会提取APK，也不会在构建时更新research或产品输入。

显式升级使用上一级`import_world_research.mjs`：先`--snapshot`冻结维护源码/测试/数据，再`--import`导入产品；
源与产品哈希记录于`assets/simulation/SOURCES.json`。`verify_sources.mjs`无需research即可校验全部副本。
命名空间、include路径与测试数据路径是机械适配，规则算法与回归断言保持维护版本。

e8f66d9接入新增建设/通知/只读头像及原测试，原型共享夹具保留在`tests/simulation/support`。
头像CPU测试的SEB/TSV头与命名空间映射到现有`ark::assets`等价API，保留全部图片裁剪断言；
该测试只在桌面配置链接raylib，运行时及其他规则测试保持标准C++依赖。
上游已完整交付勋章共享Owner桥，移除此前runtime.cpp的产品补丁记录，以原维护实现为准。
