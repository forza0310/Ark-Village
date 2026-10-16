// 固定Steam Font._setSize 77FD50、GetLocalizeScale 77D810、IsNeedLocalizeScale 77DAA0。
#include "dungeon_village_prototype/steam_font_metrics.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace dungeon_village_prototype {
namespace {
static_assert(sizeof(float)==sizeof(std::uint32_t)&&std::numeric_limits<float>::is_iec559);
float source_float(std::uint32_t bits) {
    float value{};std::memcpy(&value,&bits,sizeof(value));return value;
}
// 强制每个mulss/addss阶段落回float32，不能融合或借double推迟舍入。
float rounded(float value) {volatile float result=value;return result;}
}
std::optional<SteamFontSize> steam_font_size(int request,int scale,bool use_preset) {
    if(request<0||scale<0)return {};
    constexpr std::array<int,3> presets{12,24,30};
    const int base=use_preset&&request<3?presets[request]:request;
    const auto product=std::int64_t(base)*scale;
    if(product>std::numeric_limits<std::int32_t>::max())return {};
    return SteamFontSize{request,scale,static_cast<int>(product/100),use_preset};
}
std::optional<bool> steam_font_needs_localize(const SteamFontLocalizeGate &in) {
    if(in.used_size<0)return {};
    return in.stack_top.value_or(in.config_enabled)&&!in.locked&&!in.japanese&&in.used_size>=6;
}
std::optional<int> steam_font_localized_request(int used_size,bool asia_font_scale,bool ko_or_zh) {
    if(used_size<0)return {};
    const float ratio=source_float(asia_font_scale&&ko_or_zh?0x3F7AE148U:0x3F547AE1U);
    const float adjusted=rounded(rounded(rounded(static_cast<float>(used_size))*ratio)+source_float(0x3DCCCCCDU));
    if(static_cast<double>(adjusted)>std::numeric_limits<int>::max())return {};
    const int request=static_cast<int>(adjusted);
    return used_size<=9?std::max(request,used_size-1):request;
}
} // namespace dungeon_village_prototype
