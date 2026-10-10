# 完整世界回归

从冻结的维护实现迁入领域与运行时回归，保留真实规则断言、错误拒绝和候选回滚检查。
`rules/`覆盖领域消费者，当前目录覆盖完整目录、初始化、到访、场景、页栈、任务和连续世界。
普通构建和标准C++排查使用根工程headless-*预设与CTest，不依赖raylib。本目录不提供独立CMake入口；公共库初始化与测试注册只维护一份。
首次配置公共库和所需消费者后，可用`node scripts/build_product.mjs headless-debug`按序重建；CTest仍由根预设运行。
维护文件与回放用例显式链接`ark_world_persistence`，其它Owner/规则用例只链接runtime；不引入第二份Owner或改变现有断言。

脚本夹具由`ARK_WORLD_TEST_DATA`显式指向产品`assets/simulation`；不以当前目录或research路径找数据。
导入脚本登记机械改写和原始SHA256，功能测试验证行为与边界；这不等于原APK动态等价。
连续测试从真实新局出发，以明确的自动页面确认策略跨三个月。该标准用例保留注册/断言，本地默认精确排除，CI Release完整执行。任务调用点夹具与自然轨迹分别证明，
不能把空任务的连续测试描述成自然任务成功。Debug连续测试会明显慢于Release，保留相同断言与独立超时。
