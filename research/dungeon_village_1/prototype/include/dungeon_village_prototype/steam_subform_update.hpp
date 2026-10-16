#pragma once

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace dungeon_village_prototype {
// 一次真实获准SubForm.Update的共同前段，不是世界tick或完整平台Frame。
// 只作调用DTO；页面/共享SEB的权威状态仍归既有Owner，不在此保存第二份。
struct SteamSubformPrefixState {
    std::int32_t frame{}, frame2{}, loop_counter{}, wait{}, ud_arrow_counter{}, jingle{-1};
    bool until_active_hide{};
};
enum class SteamSubformUpdateAdmission { unverified, ready };
struct SteamSubformPrefixInput {
    SteamSubformPrefixState state;
    SteamSubformUpdateAdmission admission{SteamSubformUpdateAdmission::unverified};
};
struct SteamSubformRedraw {}; // DrawAllForms只请求管理器重画，不立即执行Draw。
struct SteamSubformJingle { int id{}; }; // 原PlayJingle，不改写为普通SE。
struct SteamSubformSebFrame { int common{}; }; // 每次真实调用仍读取当时的自动动画/共同门槛。
struct SteamSubformDispatch {}; // 继续本页Update/Update2，不代表已执行或返回true。
using SteamSubformPrefixOperation=std::variant<SteamSubformRedraw,SteamSubformJingle,
    SteamSubformSebFrame,SteamSubformDispatch>;
struct SteamSubformPrefixPlan {
    SteamSubformPrefixState candidate;
    std::vector<SteamSubformPrefixOperation> operations;
    // wait减后仍正时已完成共同前段并返回true；否则本页真实分派结果尚待取得。
    std::optional<bool> immediate_return;
};
// 三钟均先按int32回卷加1，再signed余INT_MAX；负wait/箭头计数沿原跳过，不夹零。
// 有序输出只消费一次；后继成功后才安装最终候选，jingle目录/真实SEB资格由后继验证。
// 原异常可能留下部分修改，维护联合回滚只能由唯一事务调用者保证，纯计划不代承诺。
// 不Frame共享SEB、不发声/抽签，不增加Owner/codec字段或用页钟猜手形帧。
std::optional<SteamSubformPrefixPlan> steam_subform_update_prefix(const SteamSubformPrefixInput &);
} // namespace dungeon_village_prototype
