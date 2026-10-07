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

显式离线性能入口见[性能采样](profile_world.md)；采样副本只写忽略的build目录，正常构建不使用插桩头。计算优化通过既有product_patch登记保留研究/首次导入身份，不改变原消费者、随机和失败回滚。

524415a维护存取另用`compile_persistence_identity.mjs`生成真实15文件数据身份；逻辑文件名保持研究协议，只映射到产品assets读取。`generate_owner_codec.mjs --check`用Clang AST核对92结构/11枚举、源规范JSON及机械namespace翻译inc。运行游戏不依赖这些工具。

精确文件回放在现有continuous套件，`replay_file_test.mjs`短三进程场景进入标准CTest。较长认证档先核对数据/布局/语义和前缀资格，按风险复用相关尾段；完整外层轮是唯一续跑边界，日历轮内审计不能重跑完整框架。命令及证据见[B1](../../docs/stages/B1-playable-prototype.md#research-524415a-design)。
