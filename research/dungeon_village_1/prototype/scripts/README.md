# 新局数据编译

[compile_startup.mjs](compile_startup.mjs)只在构建期使用 Node 内置 JSON 解析与 SHA-256，
从[发布包](../../data/startup/README.md)生成构建目录中的 C++ 常量，不手工维护第二套576格数据。
校验地图尺寸、未解释尾部、显示绑定、八个种子、单格目录、首访与职业派生契约；错误阻止构建。
运行时仅标准 C++17，不需要 Node、APK 或反编译目录。生成文件不能手改。
