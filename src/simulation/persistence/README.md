# 维护世界存储

完整世界 codec、字段清单、恢复验证和文件 I/O 位于此。公开接口在 `include/ark/simulation/persistence`；私有字节工具不作为游戏业务接口。

候选解码、身份/引用/预算校验全部成功后才安装；文件采用原子替换。保持 `ark_world_persistence` 与 runtime 分离，玩家两栏存档在 `src/app/save`，不因整理目录改变两者政策或字段顺序。
