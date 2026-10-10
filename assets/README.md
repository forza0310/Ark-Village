# 运行资源

当前705项素材的来源路径、尺寸、用途和导入记录见[SOURCES.json](SOURCES.json)。程序读取EXE旁的assets副本，不依赖research、APK或当前工作目录；截图只用于对照，不作运行纹理。

- `common/`、`common2/`、`ui/`、`event/`、`title/`、`steam_common/`与`steam_title/`：窗框、菜单、数字、通知及已接静态皮肤。
- `human/`、`monster/`、`image/`、`weapon/`：人物/怪物图集、地图设施、装备举物及原SEB记录。素材存在不代表全部演出已经接线，剩余合同见[当前对照](../docs/reference/REFERENCE_CHECKLIST.md)。
- `audio/`：26份正式Ogg，由桌面音频后端按成功提交的一次性请求播放。
- `simulation/`：维护规则和新局数据，构建期生成标准C++只读目录；来源/产品适配清单独立保存。旧切片的data副本和生成器已退役。

PNG、SEB、INF及源数据按维护发布原样导入，不裁图或修改原值来满足测试。SEB结构校验与实际使用帧的PNG边界校验分别保留；不因未使用记录扩大或放宽图集边界。正常构建不自动导入研究工作区。

Windows玩家包另附Noto中文子集与许可；构建清单和运行图集共用字形需求，`--font`可覆盖。源版本证据见research的EVIDENCE与ASSETS，资源仅用于用户授权的本地学习。
