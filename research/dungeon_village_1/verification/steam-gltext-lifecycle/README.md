# Steam GLText消费与池保留证据

2026-10-09。正式合同见[GLText生命周期](../../ui/STEAM_GLTEXT_LIFECYCLE.md)。从前批文字后端的GL队列落点接续，核最终GUI.Label、meshOnly保留、正常状态恢复和消费后引用保留。未扩展Image任务、原窗口或系统字体访问。

- [EVIDENCE.json](EVIDENCE.json)：7个具名固定方法／4368字节、复用DrawString生产者的身份与4096字节原窗口、9项GLText字段和Add／get_Item真实泛型绑定。
- `inspect.cjs`仅白名单具名入口、PE可执行节和来源hash检查，单项≤1568字节；不从未经确认的中途地址解码，不落盘反汇编全文。
- `bindings.cjs`独立解析两个原MethodInfo槽；`audit.cjs`核原DLL、metadata、旧证据与生产者字节并生成有限摘要。

复算：

```powershell
node research/dungeon_village_1/tools/steam-gltext-lifecycle/audit.cjs
```

实质闭合点是“先mesh后逐条GUI.Label，正常清active count并恢复GUIStyle／clip”；不是单纯增加方法覆盖数。资源检查明确：GLText池和文字／Font引用消费后仍保留、后继覆盖复用；Dispose(false)只清mesh。池容量与全局生命周期仍不宣称永久有界或无泄漏。

本包不新增二进制缓存或字体／图片副本。EVIDENCE由正式合同直接消费，无一次性大输出未清。脚本同步退出、无后台进程，不构建、不运行游戏测试、不提交；只做来源、语法、链接与幂等复算。
