#pragma once

#include <optional>

namespace dungeon_village_prototype {
// Steam Font._setSize的纯数值部分；保留请求模式，used_size不等于字形像素高。
struct SteamFontSize {
    int request{}, scale{}, used_size{};
    bool use_preset{};
};
// preset 0/1/2为12/24/30，其余为原请求，之后整数乘scale再/100向零截断。
// 维护拒绝负请求/比例及32位乘法溢出；原实现没有这些保护。合法0不夹成1。
std::optional<SteamFontSize> steam_font_size(int request,int scale,bool use_preset);

struct SteamFontLocalizeGate {
    int used_size{};
    std::optional<bool> stack_top; // 非空对应原设置栈count>0；不能用false回退全局开关。
    bool config_enabled{}, locked{}, japanese{};
};
std::optional<bool> steam_font_needs_localize(const SteamFontLocalizeGate &input);

// ko_or_zh是原StartsWith查询结果，适配方须按原语言/比较资格供给，不在此猜locale。
// 返回新的整数请求字号；真正SetSize仍须再次消费实例scale，不能直接当最终used_size。
// 本函数不代替上面的准入判定，也不接触字体、字形、平台线程或全局锁。
std::optional<int> steam_font_localized_request(int used_size,bool asia_font_scale,bool ko_or_zh);
} // namespace dungeon_village_prototype
