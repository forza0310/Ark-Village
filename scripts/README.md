# 构建与素材脚本

需要Node 18+，只消费维护数据，不执行逆向。

首次按[Windows构建说明](../docs/CONTRIBUTING.md#windows本地构建)配置公共库和消费者后，日常用`node scripts/build_product.mjs desktop-debug --parallel 4`重建。入口先完成唯一Release公共库，再构建指定消费者，任一步失败立即停止；不自动配置工具链、运行测试或发布。其它三个消费者使用对应预设名。

`package_windows.py`将已验证且已提交的本地Release构建打成便携ZIP，复用`.github/ci/build.py`的PE递归依赖、剥离、素材检查和资源启动检查；不执行CI构建入口、不下载或发布。`--work-parent`必须为当前`build`下已存在目录，工具每次新建独立暂存目录，不覆盖旧制品；另提供`--toolchain`、`--font-dir`、`--raylib-license`指向现有固定输入。只包含玩家EXE/DLL/资源/字体和许可，输出`RESULT.json`记录提交、ZIP/解压字节数及SHA；解压后的实际窗口与玩家存读档由阶段验收另记。

- import_world_assets.mjs：完整世界的人物/怪物/地图/共用图像目录，已有文件须与维护源相同才复用；不覆盖不同版本。导入前二次核对读取集，统一更新assets/SOURCES.json。
- import_world_research.mjs：冻结完整世界维护源码/测试/数据，迁入simulation模块，记录源与产品SHA256；仅显式更新时运行。
- simulation/compile_startup*.mjs：从assets/simulation的固定目录构建完整C++定义/脚本；来源与适配信息保留在SOURCES清单。
- shared_library_contract.mjs：由CMake调用prepare/publish/check/check-executable，区分当前输入身份与成功DLL产物哈希，拒绝部分构建、旧库或替换库；内容指纹不使用Git HEAD、文档或测试正文。生成头把身份编进DLL和消费者，Windows启动守卫在业务对象前检查实际已加载Ark依赖；运行不需要此脚本或manifest。开发者沿统一build_product入口重建，协议与限制见[构建检查](../docs/CONTRIBUTING.md#构建检查)。
- compile_desktop_glyphs.mjs：唯一Unicode需求扫描，只读产品src的cpp/hpp、include的hpp和assets的txt/tsv/json，保留ASCII32–126、Unicode escape及JSON解码字形，严格拒绝坏UTF8/无效Unicode。`node scripts/compile_desktop_glyphs.mjs --root . --json build/shared-libraries/desktop-generated/desktop_glyphs.json --header build/shared-libraries/desktop-generated/desktop_glyphs.hpp`生成按源SHA追踪的整数码点清单和私有UTF8串；无`--json`时将清单输出到stdout。不扫描research/build，不把生成头写入源目录；相同输出不改时间戳。
- prepare_windows_font.py：固定Noto Sans CJK SC2.004源/许可哈希，用fonttools==4.59.0消费上述Node需求清单，生成改名后的OFL字体子集，逐字形验证，输出default.otf/OFL.txt/SOURCES.json。字体准备先刷新唯一公共树desktop-generated的清单/头，与后续CMake和运行图集共用；自定义公共树可用`--glyph-inventory PATH`指定同一清单位置。运行`python scripts/prepare_windows_font.py --output-dir build/local-tools/fonts`；可用`--source-font`/`--source-license`复用已下载的同哈希文件。仅构建需要Python，游戏运行不需要。

栅栏/外部门柱按BOUNDARY规格导入common的fence01 PNG、三套fence01x SEB及door00 PNG/SEB，帧偏移由renderer与原SEB分层处理，不重裁PNG。
不修改原始PNG/SEB满足校验；未用记录与实际绘制帧分开检查。
