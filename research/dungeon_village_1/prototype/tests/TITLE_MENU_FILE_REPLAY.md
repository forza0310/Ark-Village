# 标题菜单文件视图跨进程尾段

[startup_title_menu_replay_checks.cpp](startup_title_menu_replay_checks.cpp)提供`run_startup_title_menu_file_replay_cli`，由既有应用回放可执行接入，不新增target或CTest。controller为`title-menu-files-v1`，是固定的17条命令策略，不是通用UI脚本框架。[三进程runner](title_menu_process.mjs)已挂既有进程套件，完整18行逐字节一致；实际证书见MENU_REPLAY（本地核对材料，不随仓库交付）。

```powershell
<现有应用套件exe> title-menu-files-v1 --work-dir <已存在的独立目录> --save-file <新快照.avra> --trace-file <新trace.jsonl>
<现有应用套件exe> title-menu-files-v1 --work-dir <另一个已存在的独立目录> --load-file <同一快照.avra> --trace-file <另一新trace.jsonl>
```

所有目录／文件仍由已验路径工具限制到编译绑定的`research/work`。`work-dir/application`须不存在；恢复宿主先在独立`unused-current`构造并消费它自己的B0，恢复完整应用后不将这次宿主声音算入原Driver。不覆盖现有trace、capture或应用根。程序保留研究根与完整证据供上层runner审核及清理，不启动后台任务。

reference使用seed1真实新局，没有世界update、时间注入、任务夹具或资金调整。实际开始→消费B0/G→正常保存第0栏手动→重入标题并消费B0→进入选档／raw20→删除询问默认否，然后捕获完整应用与四目录文件视图。其它三行始终空；不能把此路径声称为自然中断档已产生。捕获点资金5000、原日期0年3月、世界random draws与simulation steps均0。

尾段逐命令执行：默认否→父消费→取消raw20→根消费→重新开raw20→删除询问→是→父消费→根隐藏→隐藏手动行进入新局配置→实际start→保存manual→返回标题→选档→raw20→继续→根读取。`step0`是捕获状态，`step1…17`是各命令完成且音频已消费后的状态。只用现有公开命令consumer；每一级returned载荷都保留真实父消费边界。

校验器同时检查菜单精确形态／稳定ID关系、世界不变初值、原blob身份／长度／用途、另外三目录、隐藏区间及成功提交修订序列。隐藏步骤保留blob，随后相同真实新局的normal字节允许复用它；不是重签或删除历史凑一致。合法捕获恢复仅接受step0，边界校验发生在新根发布前。

每行JSON记录`step`、`app_driver_digest`、`system_sha256`、`file_view_digest`、`blob_count`、`blob_bytes`、`typed_audio`和`random`。应用摘要含完整Driver／应用／世界历史及文件视图；磁盘另外重新读取并校验系统和实际blob，不能只比较内存缓存。`file_view_digest`使用system完整字节及摘要排序blob的长度前缀／完整字节，不包含路径。声音序列用`[operation,id]`，原B0/G均为`replace_bgm`。

标准输出末行以`title menu replay summary `开头，后接JSON：`controller,steps,digest,file_view_digest,system_sha256,blob_count,blob_bytes,audio_count,random`。两次恢复必须与reference的18行trace及summary全部一致，并核源capture SHA不变。固定终点17命令、6条累计已消费音频（前缀3、尾段3）、random0；不以只有同终点资金作为恢复正确证据。

Driver为104字节固定规范载荷，保留策略身份、step、消费计数／峰值和原引用；字节预算保持128MiB，trace上限1MiB。没有新增资源文件、数据库、格式或播放器。本测试源归既有应用／持久化测试职责；线格式摘要复核复用`startup_application_replay_wire.hpp`，没有复制完整codec。
