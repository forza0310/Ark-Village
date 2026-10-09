# Steam人物60外层输入补证

2026-10-10。[正式合同](../../ui/STEAM_HUMAN_INPUT.md)，[EVIDENCE](EVIDENCE.json)。只读固定Steam DLL SHA-256 `9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a`，不执行原程序、读取存档或修改产品。

本批解决人物页初始标签能否被外层Update按页改写的问题，并补Get/Set触摸值、SubForm触摸放行、GameView列表分派与确认条件。结论限定已核路径；label11存在输入监听，不等于实际可见标签。

## 固定证据预算

- 新8个具名入口／前缀共5,033字节，最大1,664字节：GetTouchValue、SetTouchValue、OnTouchEvent、Draw wrapper、`_draw`前1,097字节、GameView两个Set及OnTouchEvent到跳表前的1,664字节。原巨型`_draw`不整段解码；其前缀仅确认委托，不假装下游链全闭合。
- 复用已冻结Update、Init、OnTouchDlgSel三方法的指令与source hash；展示前核源方法字节，逐行再核DLL对应字节。没有保存全文反汇编副本或改写历史证据。
- 26个调用／固定指令锚，3个显式跳表项（component3/10/11）；跳表只按uint32数据读取，不把线性解码后的数据当指令。记录5个局部分支哈希。
- 0个新PNG／SEB、0个原表副本、0个游戏构建缓存。引用前批EVIDENCE及字段映射的真实哈希，不自动枚举资源包，输出由独立合同消费。

```powershell
node --check research/dungeon_village_1/work/steam-human-input/inspect.cjs
node --check research/dungeon_village_1/work/steam-human-input/audit.cjs
node research/dungeon_village_1/work/steam-human-input/inspect.cjs update
node research/dungeon_village_1/work/steam-human-input/inspect.cjs viewTouch
node research/dungeon_village_1/work/steam-human-input/audit.cjs
```

其他固定inspect键为manifest、init、get、set、touch、draw、drawPrefix、viewSet、accessSet、touchDialog；仅stdout。audit仅生成本目录EVIDENCE。UTF-8无BOM、LF；验收语法、源与字节锚、具名alias、跳表、重复生成幂等、Markdown链接及Git差异。纯静态文档批，无游戏回归。

父会话集中索引、提交；本分支无后台／游戏／预览进程，无需清理他人缓存。没有引用运行时实体，亦未验证全部触摸组件退休／整个应用永久有界。
