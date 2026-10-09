# Steam手动档菜单静态研究

2026-10-09。正式结论见[raw20菜单与确认](../../ui/STEAM_SAVE_MENU.md)。本批只读原DLL／metadata与既有证据，不操作窗口、原档或账号目录，不构建、不提交。

- [EVIDENCE.json](EVIDENCE.json)：新具名窗口范围与SHA、复用旧8方法证据、原图／SEB引用、宽度／颜色／浮点立即数及分发锚点。
- [LITERALS.json](LITERALS.json)：菜单三文字、覆盖／删除询问、按钮及有限翻译链固定槽。
- [WIDTH.json](WIDTH.json)：Steam cctor的日文宽数组独立字段句柄解析。
- `inspect.cjs`：从固定具名入口顺序解码，只按白名单过滤stdout，不写指令全文；`audit.cjs`只汇集三份摘要；`literals.cjs`和`width-data.cjs`解析固定metadata槽。

复算：`node research/dungeon_village_1/work/steam-save-menu-contract/audit.cjs`。Init／Update／FrameMenu／触摸／cctor复用旧证据，先核旧JSON与原方法字节SHA，不复制约2.7MB反汇编文件。新读取最大前缀17,312字节，单次可打印范围不超过4,112字节；所有终点截断指令均不当事实。

边界发现：SubForm._draw索引到下一具名入口相差197,616字节，但菜单／确认路径在0x103571BF前已结束，0x103571C0出现新函数prologue。本批把消费前缀截止此处；历史“登记范围”保持资格，不宣称该巨型间距都属于同一个函数或全部页面已闭合。

关键结果：raw20按25/26/27原序；继续直接返回0，新局／删除先预存1/2再推默认“否”的raw1；只有答“是”后父菜单关闭，取消明确返回-1。行组件9消费ENTER/UP、仅UP确认；方向与横向询问分开。皮肤使用common menu.seb帧2/3，不能换成普通木框；menu两项同字节原发布素材已有真实路径，未改上一包38项清单。

语言支线已核Graphics实际翻译入口、缓存、当前表／内置表回退、Tab／换行路径及LoadTranslateTable默认与回调分流；当前中文语言包安装与选择、实际译文、字体仍未补猜值。无新Language.cctor全量扫描。

三份JSON均由正式合同消费，目录为有限研究摘要；脚本同步结束，无后台进程。源文件、工作说明、相对链接及幂等复算验收后由主会话统一归档。
