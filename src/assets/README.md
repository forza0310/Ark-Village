# assets

ark_asset_metadata从research/tools维护代码迁入SEB与UTF-8/TSV子集，接口在`include/ark/assets`。
不提取APK、不解析游戏规则、不依赖raylib；desktop使用它解码素材描述。
SEB保留全部记录和legacy_tag，严格拒绝截断、压缩格式和尾随字节。
结构解析不要求每条记录都可绘制：草地有额外记录，海面有未用越界帧。
desktop单独校验实际请求帧的PNG矩形/翻转，不裁切或忽略正在使用的非法帧。
依据：[素材格式](../../research/dungeon_village_1/assets/README.md)、[维护解析器](../../research/dungeon_village_1/tools/src/sprite.cpp)。
