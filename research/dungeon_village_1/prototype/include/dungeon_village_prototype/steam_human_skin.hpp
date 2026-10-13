#pragma once

#include "dungeon_village_prototype/startup_skin.hpp"
#include "dungeon_village_prototype/startup_title_actor_skin.hpp"
#include "dungeon_village_prototype/steam_facility_skin.hpp"
#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace dungeon_village_prototype {
// Steam raw60 的局部表现；资源身份独立核验，不按 APK 同名推定相等。
enum class SteamHumanAsset {
    medal, pinch, bubble, number05, portrait_background, home_background,
    experience_bar, experience_label, experience_remaining, smile, combat_icon,
    equipment_background, weapon_icon, armour_icon, accessory_icon, magic_icon, catalogue_new, catalogue_get, gift_tab,
    ceremony_background, curtain, continue_cursor, result_icon,
    president, gift_box, gift_face, item_icon, gift_flash, recovery_effect, recovery_number,
    residence_background, shadow
};
struct SteamHumanResource {
    int image{}, sprite{-1};
    const char *published_image;
    const char *published_sprite;
};
std::optional<SteamHumanResource> steam_human_resource(SteamHumanAsset asset);
// UserData.frame_／APK V的已存Owner映射。只在获准世界入口推进，模态绘制保持。
std::optional<int> steam_human_shared_character_frame(const StartupWorldRuntimeState &state);
struct SteamHumanImage {
    SteamHumanAsset asset;
    std::array<int,2> position{};
    int frame{};
    // 有裁片直接取 PNG；否则读取资源对应 SEB，内部偏移只加一次。
    std::optional<std::array<int,4>> crop;
};
enum class SteamHumanTextRole { down };
struct SteamHumanText {
    SteamHumanTextRole role;
    std::array<int,2> position{};
    int anchor{};
    std::array<int,3> rgb{};
};
using SteamHumanDraw = std::variant<StartupSkinRect,SteamHumanImage,SteamHumanText>;
struct SteamHumanSkinPlan { std::vector<SteamHumanDraw> draws; };
struct SteamHumanHpInput {
    int now{}, after{}, maximum{};
};
// 锚点已包含 MOVE_DATA 插值及住宅偏移。仅普通肖像调用；state7由父页另走倒下分支。
// 无实例必须传 nullopt；maximum由当前共享定义 GetHpMax 对应值提供，不改 HP 六槽。
// 保留有符号 HP，长度夹限但危险判断仍读取 after；不推进世界／表现计数。
std::optional<SteamHumanSkinPlan> steam_human_power_skin(
    std::array<int,2> position, int direction, int frame,
    std::optional<SteamHumanHpInput> live);
// 默认调用锚是(83,70)；5枚起复用已核 number05 数字 helper，不能写字体乘号。
std::optional<SteamHumanSkinPlan> steam_human_medal_skin(int medals,
    std::array<int,2> position = {83,70});
// 默认倒下文字锚基点(75,79)，宽度由实际字体 StringWidth 提供。
// 4096为维护输出预算，不是原字体或原游戏字符串上限。
std::optional<SteamHumanSkinPlan> steam_human_down_bubble_skin(int measured_width,
    std::array<int,2> position = {75,79});
struct SteamHumanDetailSkin {
    SteamHumanSkinPlan medals; // raw60共用区中，先奖章再人物助手。
    std::array<int,2> portrait_position{};
    StartupTitleActorSkin portrait; // 武器(若有)→身体；本页不加标题阴影。
    SteamHumanSkinPlan after_portrait; // 普通为HP/危险图标，倒下为气泡/文字。
};
// 当前职业/性别/武器和实际实例来自只读Owner view；显式W优先，否则取同定义首实例。
// 不推进页面或共享scratch。
// down_text_width只在state7使用，必须来自实际字体测宽；其它情况不需要测宽。
// 这里只展开已核基础肖像，不认证原共享scratch任意历史残留的附加效果。
std::optional<SteamHumanDetailSkin> steam_human_detail_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    std::optional<int> down_text_width = {});
// raw60概览／六属性复用共用区及有序图元；接口分别严格匹配页0／1。
enum class SteamHumanOverviewTextRole { name, profession, satisfaction, effort, gift, attribute, profession_change,
    equipment_slot, equipment_name, equipment_empty, equipment_hint, magic_title, spell, magic_empty, magic_status,
    gift_change_prompt, gift_accept, profession_current, profession_insufficient,
    profession_cost_label, profession_population_label, profession_salary_label,
    profession_bonus_label, profession_bonus_description,
    profession_transition, result_message, gift_result_name, gift_result_suffix,
    gift_catalogue_header, gift_catalogue_empty, gift_current_equipment, item_explain };
struct SteamHumanOverviewText {
    SteamHumanOverviewTextRole role;
    std::string value;
    std::array<int,4> rectangle{};
    int anchor{}, font_size{};
    bool layout{};
    std::array<int,3> rgb{92,51,31}; // 固定Steam SC_WINDOW_BLACK；名称虽BLACK，实际为棕色。
    int argument{}; // attribute仅0..5，原体力／力／器用／丈夫／魔力／運顺序。
    bool rich{}; // 原DrawString2；布局与富文本资格分别保存，不因有value就改普通DrawString。
};
struct SteamHumanOverviewScroll {
    std::array<StartupSkinRect,2> rectangles;
    std::array<int,4> thumb_touch; // 12的基矩形，25仍使用(221,69,3,127)。
};
struct SteamHumanEquipmentRow {
    SteamStartupTouch touch; // 原Touch10，value=0x20000|slot。
    std::array<int,4> margin{0,-20,0,0}; // left/right/top/bottom，不抄赠礼目录的margin。
};
// raw61/64共用原组件12/25；保留真实first的滑块几何和原拖动附加参数。
struct SteamHumanCatalogueScroll {
    std::array<StartupSkinRect,2> rectangles;
    SteamStartupTouch touch;
    std::array<int,4> margin;
    std::array<int,3> arguments;
    SteamStartupTouch track_touch;
};
// DrawString2的学习说明：只有魔法名称为0065FF，后缀沿棕色，不能整句染蓝。
struct SteamHumanLearningText {
    int spell{};
    std::array<int,2> position{};
    int anchor{4};
    std::array<int,3> name_rgb{0,101,255}, suffix_rgb{92,51,31};
};
// 小肖像保留普通clip／ImageClip的源请求；完整身体不裁剪，不写共享scratch。
struct SteamHumanPortrait {
    StartupTitleActorSkin actor;
    std::array<int,2> position{};
    std::optional<std::array<int,4>> clip;
    bool image_clip{};
};
enum class SteamHumanClipKind { push_intersect, pop };
struct SteamHumanClip { SteamHumanClipKind kind; std::array<int,4> rectangle{}; };
using SteamHumanOverviewOperation=std::variant<SteamStartupFrameDraw,SteamHumanImage,
    SteamFacilityImage,SteamFacilityNumber,SteamHumanOverviewText,SteamHumanDetailSkin,
    SteamStartupTouch,SteamHumanOverviewScroll,SteamHumanEquipmentRow,SteamFacilityArrowTouch,SteamHumanPortrait,
    SteamHumanCatalogueScroll,SteamHumanLearningText,SteamHumanClip,SteamFacilityMapchip2>;
struct SteamHumanOverviewPlan {
    std::vector<SteamHumanOverviewOperation> operations;
    std::array<int,2> soft_labels{0,2};
    bool recovery_gift{};
    std::string title; // 空沿raw60标题；管理子窗提供原角色的中文适配标题。
};
struct SteamHumanOverviewOptions {
    int view_y{}, finger_frame{};
    bool japanese{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> profession_width, profession_repeat_width; // 两次均在原改字号之前。
    std::array<std::optional<int>,2> bottom_widths; // 普通礼物／恢复提示的实际StringWidth。
    std::optional<int> down_text_width;
    std::optional<int> profession_change_width; // 页1底部TextLayout的实际StringWidth。
    bool english{};
    std::optional<int> equipment_hint_width, magic_hint_width;
    std::array<std::optional<int>,2> comparison_name_widths; // raw65字号10后实际测宽，现装／候选。
    std::optional<int> gift_accept_width;
    std::optional<int> insufficient_points_width, message_width;
    // 原mentalUp使用UserData共享表现计数，不拿结果页counter替代；平台需显式提供。
    std::optional<int> character_frame;
    bool catalogue_scroll_first_touch{};
    int catalogue_arrow_count{}; // 原udArrowCnt，0用上下矩形，非0用Touch23箭头。
    // 原当前Font实际测宽：属性名／属性后缀／学习完整句。只消费当前分支需要的槽。
    std::array<std::optional<int>,3> item_explain_widths;
    std::optional<int> result_name_width, result_suffix_width; // 68非JP在11号字测，69沿当前字号；68JP后缀固定36。
};
// 共用mentalUp的只读调用输入，三行仍分别为旧值／新值／原请求，不补封顶差。
struct SteamHumanRewardInput {
    int profession{},sex{},weapon{-1},phase{},counter{};
    std::string name;
    std::array<std::array<int,2>,3> rewards{};
};
std::optional<SteamHumanOverviewPlan> steam_human_reward_skin(
    const SteamHumanRewardInput &,const SteamHumanOverviewOptions &);
struct SteamHumanResidenceResultInput {
    int phase{},counter{},secondary{},mapchip{};
    SteamHumanRewardInput human;
};
// raw96上方身体不用武器；副钟跳跃，phase1底部mentalUp另读共享character_frame。
std::optional<SteamHumanOverviewPlan> steam_human_residence_result_skin(
    const SteamHumanResidenceResultInput &,const SteamHumanOverviewOptions &);
// 只接受已初始化真实栈顶96；旧绑定、双钟及住宅来源由Owner共享校验器检查。
std::optional<SteamHumanOverviewPlan> steam_human_residence_result_skin(
    const StartupWorldRuntimeState &,std::uint64_t page,const SteamHumanOverviewOptions &);
// ItemData.Draw_itemExplain，默认raw64底部锚(120,199)；不把description/数值拼成效果说明。
std::optional<SteamHumanOverviewPlan> steam_human_item_explain_skin(
    const StartupWorldItem &item,const SteamHumanOverviewOptions &options);
// raw64四行五页；Owner冻结目录与独立scroll，不改变NEW、库存、收费或页面。
std::optional<SteamHumanOverviewPlan> steam_human_gift_catalogue_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
// 只读已初始化raw60页0；无W仍绘定义，坏装备/库存依赖显式拒绝。
// 不重算成长/HP、初始化、推进计数、抽随机、发声或提交追踪。
std::optional<SteamHumanOverviewPlan> steam_human_overview_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
// 六属性读取当前共享baseParam对应缓存，不叠加装备，不查页0专有回复库存。
// 与概览使用同一计划类型／共用区执行器；仅接受已初始化页1，输入仍由Owner提交。
std::optional<SteamHumanOverviewPlan> steam_human_attributes_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
// 四页统一入口；页2选中槽来自Owner。具体提示的文本/控制图标由平台后端消费。
std::optional<SteamHumanOverviewPlan> steam_human_information_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
// raw73只读实际目录选择，价格为装备基础价；与赠礼收费/当前人物缓存分开。
std::optional<SteamHumanOverviewPlan> steam_human_equipment_detail_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
// raw65只读当前装备／父目录候选的定义param；不报价/收费/装配或消费父答案。
std::optional<SteamHumanOverviewPlan> steam_human_gift_confirmation_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
// raw62只读冻结属性预览、真实旧／目标等级及勋章／点数，不重新预览或扣点。
std::optional<SteamHumanOverviewPlan> steam_human_profession_preview_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
// 共用mentalUp：page仅0/1，frame为其局部计数；读reward_display的请求量而非封顶差。
std::optional<SteamHumanOverviewPlan> steam_human_reward_skin(
    const StartupWorldRuntimeState &state,int human,int page,int frame,const SteamHumanOverviewOptions &options);
// raw63纯表现；恰55的职业提交及满197确认的最终重算仍由Owner执行。
std::optional<SteamHumanOverviewPlan> steam_human_profession_result_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
// raw66只读父目录兑现后冻结载荷；礼盒/评价/回复展示均不再次付费或改变HP。
std::optional<SteamHumanOverviewPlan> steam_human_gift_result_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
// raw68/69只消费已兑现后冻结的choice(slot/newID或4/item)与三张参数表。
// 缺头部来源拒绝，不从当前装备反推；69仅绘真实effect行的非零差额。
// 测宽由当前字体提供，计时/确认/奖励由Owner执行，70不在此入口范围。
std::optional<SteamHumanOverviewPlan> steam_human_attribute_result_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
// raw61五行两页；读取Owner目录与真实scroll，工资展示不执行账本十整。
std::optional<SteamHumanOverviewPlan> steam_human_profession_catalogue_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
} // namespace dungeon_village_prototype
