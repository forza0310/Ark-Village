#pragma once

#include "dungeon_village_prototype/steam_startup_skin.hpp"
#include "dungeon_village_prototype/steam_touch_commands.hpp"
#include <functional>
#include <string>

namespace dungeon_village_prototype {
// Steam raw13局部皮肤：文字来源由Owner提供，布局不读取文件、不翻译或消费随机。
enum class SteamManualTextRole { body, about, game_name, copyright };
struct SteamManualWindow {
    SteamStartupWindow source;
    int page_number{}, total_pages{}; // 标题为“遊び方 空格 当前/总数”，调用方负责已核翻译。
};
struct SteamManualText {
    SteamManualTextRole role;
    std::string body; // 仅body使用；其它角色按固定原文/语言适配器取字。
    std::array<int,4> rectangle{};
    std::array<int,3> rgb{{30,30,30}};
    std::optional<int> anchor;
    int font_size{}, line_space{}; // 0保留当前字号；正文是独立TextLayout。
    bool localize_text{}, trial_version{};
};
struct SteamManualClip {
    bool push{};
    std::array<int,4> rectangle{}; // pop时不用；不把旧clip丢弃成全屏。
};
// 依次GetWeapon/GetJob/GetImgId，再SetDispPlayerData及DrawDispPlayer。
// 身体/武器从该定义当前共享值解析，不缓存副本，也不认证scratch历史附加效果。
struct SteamManualActorRequest {
    int definition{};
    std::array<int,2> position{};
    int seb{}, anime_index{}, direction{}, update_counter{};
};
// AddTouch的SEB帧只定义原bounds，不是最终箭头贴图帧。
struct SteamManualArrowRequest {
    std::array<int,2> position{};
    int bounds_frame{}; // raw13左3／右0。
    bool selected{}; // 调用方提供CheckTouch(1,value)，不是鼠标悬停猜测。
};
struct SteamManualArrowPlan {
    int texture_index{}; // GameView TEX0..3，统一来自buttoneffect.png，而非common74。
    std::array<int,4> crop{};
    std::array<float,4> destination{};
    std::array<int,4> touch_rectangle{}; // 原_addTouch基矩形，尚未作surface margin/clip。
    std::array<int,2> paint_origin{};
    int paint_frame{};
};
// Steam AddTouch2399B0 / DrawImage23A460 / DrawScaledImage23A530。
// 密度image_width/128与逻辑缩放0.75分开；选用哪个实际密度纹理由调用方明确。
std::optional<SteamManualArrowPlan> steam_manual_arrow_skin(
    const SteamManualArrowRequest &,std::array<int,4> seb_bounds,int image_width);

// 共用箭头id0的临时表现状态，不属于世界Owner或文件快照；无效果池/平台对象所有权。
// clock在源调用位置读取：Init一次，存活Frame三次，退休Frame一次，Draw一次。
using SteamTouchEffectClock = std::function<std::optional<std::int64_t>()>;
struct SteamTouchArrowState {
    std::uint64_t surface{}, component_key{};
    int texture_index{};
    float x{}, y{}, velocity_x{};
    std::array<float,6> arguments{}; // 目的宽高、密度换算后源x/y/宽高；包装时不舍入。
    std::int64_t created{}, last_frame{};
};
struct SteamTouchArrowFrame {
    SteamTouchArrowState state;
    bool retire{}; // 仅请求从active移除；退休不清arg/绑定，也不推入第二份buffer。
};
struct SteamTouchArrowSurface { int x{}, y{}, scale_ratio{100}; };
struct SteamTouchArrowBlend { int mode{}, source{}, destination{}; };
struct SteamTouchArrowImage {
    std::array<int,4> crop{};
    std::array<float,4> destination{};
};
struct SteamTouchArrowDraw {
    std::uint64_t surface{};
    std::vector<std::variant<SteamTouchArrowBlend,SteamTouchArrowImage>> operations;
};
// 只消费1/18正常DOWN的既有请求；绑定非0是维护资格，实际纹理宽由宿主供给。
std::optional<SteamTouchArrowState> prepare_steam_touch_arrow_effect(
    const SteamTouchArrowEffect &,std::uint64_t surface,int image_width,const SteamTouchEffectClock &);
// 保留低32位年龄比较、完整long积分差及float32存储。时钟失败/异常无部分候选。
std::optional<SteamTouchArrowFrame> advance_steam_touch_arrow_effect(
    const SteamTouchArrowState &,const SteamTouchEffectClock &);
// 读取同次真实surface观测，返回原RenderMode→DrawScaledImage→固定恢复模式，空计划是不绘制。
// 不代调Frame/退休，不把BlendMode请求简化成已核等价tint或推进世界/随机。
std::optional<SteamTouchArrowDraw> steam_touch_arrow_effect_skin(
    const SteamTouchArrowState &,const SteamTouchArrowSurface &,const SteamTouchEffectClock &);

// 仅id0的实际临时表现池：buffer保留全部槽，active保存原加入顺序；不持平台对象。
// Add复用首个闲槽，Frame逆序、Draw正序；Clear/Remove只撤active，状态仍留buffer。
// Frame读钟失败时全池不提交是维护原子策略，不认证原异常路径具备相同保证。
class SteamTouchArrowEffects {
  public:
    using SurfaceQuery=std::function<std::optional<SteamTouchArrowSurface>(std::uint64_t)>;
    std::optional<std::size_t> add(const SteamTouchArrowEffect &,std::uint64_t surface,
                                  int image_width,const SteamTouchEffectClock &);
    bool frame(const SteamTouchEffectClock &);
    std::optional<std::vector<SteamTouchArrowDraw>> draw(const SurfaceQuery &,
                                                        const SteamTouchEffectClock &) const;
    bool remove(std::size_t slot) noexcept;
    void clear() noexcept { active_.clear(); }
    std::size_t active_count() const noexcept { return active_.size(); }
    std::size_t retained_count() const noexcept { return buffer_.size(); }
    std::optional<SteamTouchArrowState> state(std::size_t slot) const;
  private:
    std::vector<SteamTouchArrowState> buffer_;
    std::vector<std::size_t> active_;
};
// 触摸在原绘制位置登记；image_draw索引引用本draws中的箭头，不能自行改成OS矩形。
// 此处StartupSkinRect只借几何载体保留Steam DrawRect调用实参，不沿用其APK像素+1语义；
// raylib描边/DPI实际像素覆盖仍由后端单独验，不把153×55实参称为已核半开覆盖区域。
using SteamManualDraw = std::variant<SteamManualWindow,SteamManualArrowRequest,SteamStartupBox,
    SteamManualText,SteamManualClip,StartupSkinDraw,SteamManualActorRequest,StartupSkinRect,
    SteamStartupTouch>;
struct SteamManualSkinPlan {
    std::array<int,2> origin{};
    std::vector<SteamManualDraw> draws;
};
struct SteamManualSkinInput {
    int width{240}, height{240};
    std::array<int,2> origin{}; // 原SubForm x/y，外层共同居中只由本计划加一次。
    int body_pages{}, page{}, frame{}, wait{};
    bool japanese{}, on_top{true}, until_active_hide{}, trial_version{};
    std::array<bool,2> arrow_selected{};
    std::optional<std::string> body_text; // 可见正文页必须有缓存，允许合法空串。
    bool localize_text{}; // Owner区分初次LT与后续直接SetText，本函数不执行翻译。
    std::vector<int> frozen_definitions;
    int definition_count{}; // 当前来源目录上界，只有末页实际消费名单时核验。
};
// 不产生Owner更新/文件/声音/随机；隐藏页不消费正文或人物，末页不消费旧正文缓存。
std::optional<SteamManualSkinPlan> steam_manual_skin(const SteamManualSkinInput &input);
} // namespace dungeon_village_prototype
