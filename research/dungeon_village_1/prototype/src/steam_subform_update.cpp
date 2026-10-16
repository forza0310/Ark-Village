// Steam SubForm.Update RVA321B00的共同前段10321B63–10321CF1；局部分派另归各页面。
#include "dungeon_village_prototype/steam_subform_update.hpp"

namespace dungeon_village_prototype {
std::optional<SteamSubformPrefixPlan> steam_subform_update_prefix(const SteamSubformPrefixInput &in) {
    if(in.admission!=SteamSubformUpdateAdmission::ready)return {};
    SteamSubformPrefixPlan plan{in.state,{},std::nullopt};
    auto &s=plan.candidate;
    const auto advance=[](std::int32_t n) {
        const auto bits=static_cast<std::uint32_t>(n)+1U;
        const std::int64_t signed_next=bits<=0x7fffffffU?static_cast<std::int64_t>(bits):
            static_cast<std::int64_t>(bits)-0x100000000LL;
        return static_cast<std::int32_t>(signed_next%2147483647LL);
    };
    s.frame=advance(s.frame);s.frame2=advance(s.frame2);s.until_active_hide=false;
    if(s.wait>0) {
        plan.operations.emplace_back(SteamSubformRedraw{});
        if(--s.wait>0) {plan.immediate_return=true;return plan;}
    }
    if(s.ud_arrow_counter>0 && --s.ud_arrow_counter==0)
        plan.operations.emplace_back(SteamSubformRedraw{});
    s.loop_counter=advance(s.loop_counter);
    if(s.jingle!=-1) {
        plan.operations.emplace_back(SteamSubformJingle{s.jingle});
        s.jingle=-1;
    }
    for(const int common:{22,21,23})plan.operations.emplace_back(SteamSubformSebFrame{common});
    plan.operations.emplace_back(SteamSubformDispatch{});
    return plan;
}
} // namespace dungeon_village_prototype
