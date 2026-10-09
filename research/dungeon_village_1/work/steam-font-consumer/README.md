# Steam 字体消费者证据包

2026-10-09。正式合同见[STEAM_FONT_CONSUMER.md](../../ui/STEAM_FONT_CONSUMER.md)。只读固定研究副本，不运行游戏／原档，不导出字体、图片或指令全文；不改产品、研究C++和现有共享索引，不构建、不提交。

## 范围与复算

- [inspect.cjs](inspect.cjs)：只接受12个具名固定入口，逐项核DLL／metadata哈希、方法表身份、下一方法起点、PE可执行节和raw范围；单方法≤2048字节。本批12入口合计6416字节，包含方法异常尾和填充，不作为6416字节全为业务指令的证明。使用已安装iced-x86 1.21.0，仅stdout输出，未保存解码全文。
- [data.cjs](data.cjs)：只读上述入口已实际引用的6个字符串使用槽及7个float32常量；核metadata字符串区界与固定PE映射。另完整读取已盘点ResourceManager首表1251行，并核Unity v22外部文件表及字体对象引用。
- [EVIDENCE.json](EVIDENCE.json)：固定来源、12方法窗口哈希与资格；[DATA.json](DATA.json)：有限常量／字符串／字体资源引用，不含字体字节。

复算示例：`node inspect.cjs select`；其它键为`update,widthF,width,scale,needScale,offset,updateScale,init,construct,staticInit,draw`。`node data.cjs`输出数据证据，脚本不自动覆盖已归档JSON。未知键、多余参数、越界、源身份变化均拒绝。没有扩大到历史被拒的建设非入口续窗；本批全部固定字体入口通过工具执行，没有审批阻断。

## 输出消费与审计边界

12入口的主要消费者分别是默认字体链、GUIStyle条件替换、浮点／整数测宽、本地化准入／比例／偏移、请求字号重算、GUIStyle初始化、构造、静态初值及DrawString转发；全部有限结果已被正式合同消费。

源码工具的call注释只列同地址前两条元数据名称，IL2CPP泛型／共享桩可能同地址多名，不能把第一条候选当作调用语义。例如测宽中的GUIContent文本写入地址同样被标成其它类setter，正式结论结合接收对象、字段及调用参数；旧／新font key的泛型名单更新仍保留未知，不擅自命名Add／Remove。

目录首次探针将资源路径当作唯一键，发现77个重复路径行后主动拒绝。进一步核明同路径分别引用Font／Material／Texture2D，调整为保留原序所有行、检查`路径+file_id+path_id`联合唯一，并核每个字体对象的classID。这是来源模型修正，未删除有效字节／引用校验或按名字覆盖数据。

ResourceManager首表结束于对象内63852，仍有2352字节尾部未解析；它不妨碍首表边界与本批12字体对象引用，但不能宣称完整ResourceManager格式已解完。四个字体路径及每路径三个对象原序保留；中文／默认路径缺失与运行时实际fallback分别登记。

本包没有后台任务或构建缓存。交付为两个只读脚本、两份小型JSON及本README，另有一份正式合同；最终文件规模、链接与父阶段集中验收由主会话记录。来源历史保留，不声称work整体永久有界。
