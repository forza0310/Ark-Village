// Canonical world owns simulation and page effects; this adapter owns only window/input/raster.
#include "world_view.hpp"
#include "ark/app/world_report.hpp"
#include "ark/app/world_session.hpp"
#include "desktop_session.hpp"
#include "ui/layout.hpp"
#include "ui/script_text.hpp"
#include "ui/skin.hpp"
#include "ui/world_award.hpp"
#include "ui/world_crew_summary.hpp"
#include "ui/world_menu.hpp"
#include "ui/world_panels.hpp"
#include "ui/world_reports.hpp"
#include "ui/world_tasks.hpp"
#include "world_inspection.hpp"
#include "world_management.hpp"
#include "world_management_inspection.hpp"
#include "world_rank.hpp"
#include "world_render_statistics.hpp"
#include "world_save_menu.hpp"
#include "world_scene.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>

namespace ark::desktop {
namespace {
namespace rules = simulation::rules;
using State = simulation::StartupWorldRuntimeState;
struct WorldWindow {
    explicit WorldWindow(const app::LaunchOptions &options) {
        require_display();
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
        InitWindow(options.width, options.height, "Ark-Village - World");
        if (!IsWindowReady())
            throw std::runtime_error("Cannot initialize world window");
        SetWindowMinSize(240, 256);
        SetExitKey(KEY_NULL);
        SetTargetFPS(0);
    }
    ~WorldWindow() { CloseWindow(); }
};
struct WorldCanvas {
    RenderTexture2D texture{};
    Extent size{};
    ~WorldCanvas() {
        if (texture.id)
            UnloadRenderTexture(texture);
    }
    void resize(Extent next) {
        if (texture.id && next.width == size.width && next.height == size.height)
            return;
        auto created = LoadRenderTexture(next.width, next.height);
        if (!created.id)
            throw std::runtime_error("Cannot allocate world framebuffer");
        SetTextureFilter(created.texture, TEXTURE_FILTER_POINT);
        if (texture.id)
            UnloadRenderTexture(texture);
        texture = created;
        size = next;
    }
};
const rules::WorldScriptPage *active_page(const State &s) {
    const auto found = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                    [](const auto &page) { return page.lifecycle != 4; });
    if (found == s.scripts.pages.rend() || found->kind == rules::WorldScriptPageKind::scene)
        return nullptr;
    return &*found;
}
std::string glyphs(const State &s) {
    std::string result =
        "本月结算打倒怪物获得村子点数收入支出收支成果入手当前活动尚未接入确定姓名打倒数下降倍完成"
        "村庄升级条件人气最高月收入设施数量住宅任务成功次数活动举办建造满足未"
        "大家的冒险通信下一页下一屏关闭月报待确认年度授勋持有勋章贡献结束本次吗？是否成果统计倒地"
        "参加任务征集队员冒险队伍追加准备出发期限延长费用评价加速取消中止再加把劲需要补充战力"
        "重新来过比较好应该撤退资金不足队伍已满暂无可追加人员页面已变化请重试当前操作不可用"
        "菜单建设村办情报系统募集入住维护费品质魅力加成价格经营设施居民返回授予勋章转职"
        "点击选择位置旋转超出地图请建在村庄范围内位置已被占用当前不可建设页面或选择已变化"
        "村庄晋级能力上升自宅完成现在下级恢复攻击防御魔法条件说明满足尚未达成庆典勇气"
        "胜利！讨伐真遗憾经费合计因为街道的人气上升了显示全文"
        // The bundled font subset and this runtime atlas are separate inventories. Include
        // every human/tax label, even when its page is first opened long after startup.
        "人物情报转职确认职业变更赠送装备赠送确认礼物评价装备能力职业大师装备情报"
        "概况属性装备魔法体力力量灵活结实魔力运气最大HP攻击防御武器衣服盾帽饰品"
        "库存无暂无候选点经验住宅已入住未入住满足努力勋章可使用不可使用"
        "转职准备中职业已变更赠送给？评价大师保持现状返回关闭确定赠送情报住宅税收合计"
        "谢谢我要加油!转机好开心合适吗？太好了!感动了!支付维护费Ｇ"
        "系统保存读取手动存档栏位空覆盖此已有原将被替换当前未进度"
        "取消确定返回处理中了请稍候无法操作后重试没有不可失败成功损坏格式版本数据不匹配"
        "稳定主场景才能打开文件目录创建写入载入校验完成忙碌权限路径错误"
        "周大小限制完整检查临时保留世界有效完毕先并等告容受与游戏和事其离";
    result += "距离下个等级还有 人周围设施暂无设施来源入住希望者商品种类";
    result += s.rules->script_sources.talks + s.rules->script_sources.news +
              s.rules->script_sources.event_messages;
    for (const auto &f : s.rules->facilities)
        result += f.name;
    for (const auto &h : s.rules->humans)
        result += h.name;
    for (const auto &job : s.rules->jobs)
        result += job.name;
    for (const auto &activity : s.rules->activities)
        result += activity.name + activity.detail + activity.description;
    result += "村办活动开展活动进行中结果季度剩余此活动尚未接入完成获得奖励配置更替领取";
    result += "南瓜商会购买道具出售多谢惠顾持有剩余获得设施使用道具设施强化赠送礼物能力提升商品种类"
              "出售中";
    result += "道路撤除配置更替从哪里开始铺呢铺到哪里呢撤到哪里移动哪个移动去哪里路铺好了"
              "选起点选设施中止";
    for (const auto &t : s.rules->tasks)
        result += t.name + t.title;
    for (const auto &i : s.rules->items)
        result += i.name;
    for (const auto &e : s.rules->equipment)
        result += e.name;
    for (const auto &monster : s.rules->monsters)
        result += monster.name;
    return result;
}
std::string page_body(const State &s, const rules::WorldScriptPage &page, int paragraph) {
    std::string body;
    if (!page.paragraphs.empty())
        body =
            page.paragraphs.at(std::min(paragraph, static_cast<int>(page.paragraphs.size()) - 1));
    if (page.task_definition &&
        (page.legacy_page == 30 || page.legacy_page == 31 || page.legacy_page == 32))
        body += s.rules->tasks.at(*page.task_definition).name + "完成!";
    if (page.legacy_page == 49)
        body = s.page_counters.count(page.id) ? world_rank_conditions(s) : "";
    if (page.legacy_page == 89 && page.monster_definition && body.empty()) {
        // This page owns a monster definition identity, not an actor or source-record index.
        const auto found = std::find_if(
            s.rules->monsters.begin(), s.rules->monsters.end(),
            [&](const auto &monster) { return monster.identity == *page.monster_definition; });
        if (found != s.rules->monsters.end())
            body = found->name;
    }
    return body;
}
void hud(const State &s, const ui::Layout &layout, const ui::Skin &skin, bool failed,
         bool menu_open, bool menu_pending) {
    const float w = layout.extent.width, h = layout.extent.height;
    skin.tile("top_bar.png", {22, 0, 90, 24}, {22, 0, w - 150, 24});
    skin.sprites.image("top_bar.png", {0, 0, 22, 24}, {0, 0, 22, 24});
    skin.sprites.image("top_bar.png", {112, 0, 128, 24}, {w - 128, 0, 128, 24});
    const auto cash = std::to_string(s.scene.world.world.ai.accounting.funds()) + "G";
    ui::draw_world_date(ui::world_date_view(s.scene.calendar), skin,
                        w - 15 - skin.text.width(cash));
    skin.right(cash, w - 7, 6);
    skin.sprites.image("townPointbar.png", {0, 0, 55, 15}, {w - 55, 24, 55, 15});
    skin.number(s.village_points, {w - 3, 27});
    skin.tile("btmbar.png", {116, 1, 4, 20}, {0, h - 21, w, 20});
    ui::draw_world_popularity(s.popularity, layout, skin);
    skin.button(layout.left_button, s.scene.framework_paused ? "继续" : "暂停", !failed);
    skin.button(ui::world_menu_button(layout.extent), "菜单",
                !failed && !menu_pending &&
                    (menu_open || (s.scene.scene_state == 0 && !active_page(s))));
    if (failed)
        skin.centered("当前活动尚未接入", {8, 46, w - 16, 20}, MAROON);
    if (s.report_state && !active_page(s)) {
        ui::draw_world_month(ui::world_month_view(s), skin);
    }
}
} // namespace

// A suite retains one immutable natural checkpoint. Each capture owns an independent branch;
// purchases and point payments cannot leak into the next capture or into normal gameplay.
static void run_world_game_capture(const app::LaunchOptions &options,
                                   const std::filesystem::path &assets,
                                   std::optional<State> *checkpoint = nullptr,
                                   WorldManagementInspection *inspection_checkpoint = nullptr) {
    WorldWindow window(options);
    float zoom = options.zoom_percent / 100.F;
    Extent extent = canvas_extent(GetScreenWidth(), GetScreenHeight());
    // Initialization is temporary; this value is the sole persistent canonical world.
    State state = [&] {
        if (checkpoint && *checkpoint) {
            std::cout << "World inspection checkpoint: target=" << options.inspect_page
                      << " rounds=" << (*checkpoint)->simulation_steps
                      << " random=" << (*checkpoint)->scene.random.draws() << '\n';
            return **checkpoint;
        }
        std::uint64_t seed = 1;
        if (options.inspect_page.empty() || options.inspect_page == "world-load") {
            // Original files omit randomness. Every normal/cold-load process starts a new stream;
            // bounded source inspections retain their explicit reproducible seed.
            std::random_device entropy;
            seed = static_cast<std::uint64_t>(
                       std::chrono::system_clock::now().time_since_epoch().count()) ^
                   (static_cast<std::uint64_t>(entropy()) << 32) ^ entropy();
        }
        simulation::StartupSession initial;
        simulation::StartupWorldRuntimeSession session(
            initial.state(), rules::WorldRandomStream::from_java_seed(seed));
        return session.state();
    }();
    // Visibility affects source decisions, so inspection uses the actual window before any round.
    state.reference_viewport = world_viewport(extent, zoom);
    const bool inspecting = options.inspect_page.rfind("world-", 0) == 0;
    const bool menu_inspection =
        options.inspect_page == "world-menu" || options.inspect_page == "world-village-menu";
    const bool save_inspection = options.inspect_page == "world-save" ||
                                 options.inspect_page == "world-load" ||
                                 options.inspect_page == "world-load-error";
    const bool transient =
        inspecting && options.inspect_page != "world-active" && !menu_inspection &&
        options.inspect_page != "world-month" && options.inspect_page != "world-month-income" &&
        options.inspect_page != "world-rank" && options.inspect_page != "world-award" &&
        options.inspect_page != "world-building" && options.inspect_page != "world-details" &&
        !save_inspection;
    WorldManagementInspection management_inspection;
    prepare_world_inspection(state, options, transient, management_inspection, checkpoint,
                             inspection_checkpoint);
    WorldCanvas canvas;
    Sprites sprites(assets);
    Text text(desktop_font_path(assets, options.font), glyphs(state));
    ui::Skin skin(sprites, text);
    state.scene.framework_paused = options.paused || transient;
    state.scene.speed_setting = 0; // Player windows always use the normal source update count.
    WorldCameraView view{state.camera, state.reference_viewport};
    WorldSaveInspection save_inspection_driver(options);
    app::WorldSession session(std::move(state), save_inspection_driver.directory());
    auto publication = session.frame();
    int frames{}, paragraph{}, scroll{};
    std::uint64_t viewed_page{}, pending_ack{}, pending_view{}, pending_pause{};
    std::uint64_t pending_task{}, held_task_page{}, pending_menu{};
    int menu_selection = 1;
    bool village_menu = options.inspect_page == "world-village-menu";
    int village_selection{};
    std::string menu_feedback;
    // Menu inspection exercises the real asynchronous desktop command after natural startup.
    // This explicit diagnostic player pause permits checking the exact loaded date before
    // another source round. Loading must preserve it and the current fresh-session random stream.
    if (options.inspect_page == "world-load")
        session.set_paused(true);
    if (menu_inspection || save_inspection)
        pending_menu = session.open_main_menu();
    WorldManagement management;
    WorldSaveMenu save_menu;
    auto generation = publication->generation;
    std::uint64_t discard_interpolation_revision{};
    if (management_inspection.preview_anchor) {
        if (world_edit_view(*publication->state, {}).active)
            management.inspect_edit(*publication->state,
                                    management_inspection.edit_endpoint.value_or(
                                        *management_inspection.preview_anchor));
        else
            management.inspect_placement(*management_inspection.selection,
                                         *management_inspection.preview_anchor,
                                         management_inspection.preview_orientation);
    }
    ui::WorldTaskSelection task_selection;
    std::string task_feedback;
    bool desired_pause = publication->state->scene.framework_paused;
    const auto started = GetTime();
    auto next_render = started;
    double last_render{};
    WorldRenderStatistics render_statistics(options.frames);
    while (!WindowShouldClose() && (options.frames == 0 || frames < options.frames)) {
        const auto now = GetTime();
        if (now < next_render) {
            WaitTime(next_render - now);
            continue;
        }
        next_render = now + 1.0 / 60;
        render_statistics.interval(frames, (now - last_render) * 1000);
        last_render = now;
        publication = session.frame(); // Only a shared_ptr exchange; never waits for world work.
        const auto &current = *publication->state;
        const bool failed = publication->failed;
        if (publication->generation != generation) {
            // Loaded IDs may match the discarded world. Drop every local binding before
            // reading input; neither held buttons nor prior interpolation crosses a load.
            generation = publication->generation;
            discard_interpolation_revision = publication->revision;
            viewed_page = pending_ack = pending_view = pending_pause = 0;
            pending_task = held_task_page = pending_menu = 0;
            paragraph = scroll = 0;
            management = {};
            save_menu = {};
            task_selection = {};
            menu_feedback.clear();
            task_feedback.clear();
            view = {current.camera, current.reference_viewport};
            desired_pause = current.scene.framework_paused;
        }
        management.observe(*publication);
        save_menu.observe(*publication);
        if (publication->last_command_serial >= pending_ack)
            pending_ack = 0;
        if (pending_task) {
            const auto result = std::find_if(
                publication->command_results.begin(), publication->command_results.end(),
                [&](const auto &r) { return r.serial == pending_task; });
            if (result != publication->command_results.end()) {
                task_feedback.clear();
                if (result->outcome == app::WorldCommandOutcome::rejected) {
                    using Denial = rules::TaskCommandDenial;
                    switch (result->denial) {
                    case Denial::insufficient_funds:
                        task_feedback = "资金不足";
                        break;
                    case Denial::team_full:
                        task_feedback = "队伍已满";
                        break;
                    case Denial::no_extra_candidates:
                        task_feedback = "暂无可追加人员";
                        break;
                    default:
                        task_feedback = "页面已变化，请重试";
                        break;
                    }
                }
                pending_task = 0;
            }
        }
        if (pending_menu) {
            const auto result = std::find_if(
                publication->command_results.begin(), publication->command_results.end(),
                [&](const auto &r) { return r.serial == pending_menu; });
            if (result != publication->command_results.end()) {
                menu_feedback = result->outcome == app::WorldCommandOutcome::rejected
                                    ? (result->kind == app::WorldCommandKind::open_save_menu &&
                                               !publication->save_message.empty()
                                           ? publication->save_message
                                           : "当前操作不可用")
                                    : "";
                pending_menu = 0;
            }
        }
        save_inspection_driver.observe(*publication, session, pending_menu, now - started);
        if (publication->last_command_serial >= pending_view)
            view = {current.camera, current.reference_viewport};
        if (publication->last_command_serial >= pending_pause)
            desired_pause = current.scene.framework_paused;
        extent = canvas_extent(GetScreenWidth(), GetScreenHeight());
        bool view_changed{};
        const auto next_viewport = world_viewport(extent, zoom);
        if (next_viewport != view.viewport) {
            view.viewport = next_viewport;
            view_changed = true;
        }
        canvas.resize({GetRenderWidth(), GetRenderHeight()});
        const auto destination = viewport(GetScreenWidth(), GetScreenHeight(), extent);
        const auto raster =
            canvas_camera(viewport(canvas.size.width, canvas.size.height, extent), extent);
        text.prepare(raster.zoom);
        const ui::Layout layout(extent);
        const auto mouse = logical_mouse(GetMousePosition(), destination, extent);
        const bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        const auto hit = [&](Rectangle rectangle) {
            return mouse && click && CheckCollisionPointRec(*mouse, rectangle);
        };
        if (!failed && !publication->save_menu_open && !save_menu.pending() &&
            (hit(layout.left_button) || IsKeyPressed(KEY_SPACE))) {
            desired_pause = !desired_pause;
            pending_pause = session.set_paused(desired_pause);
        }
        ui::WorldMenuInput menu_input;
        menu_input.click = click ? mouse : std::nullopt;
        menu_input.toggle = IsKeyPressed(KEY_M);
        menu_input.escape = IsKeyPressed(KEY_ESCAPE);
        menu_input.up = IsKeyPressed(KEY_UP);
        menu_input.down = IsKeyPressed(KEY_DOWN);
        menu_input.enter = IsKeyPressed(KEY_ENTER);
        if (!publication->main_menu_open && !pending_menu)
            village_menu = false;
        const bool menu_pending = failed || pending_menu || pending_task || management.pending() ||
                                  publication->save_menu_open || save_menu.pending();
        const bool was_village_menu = village_menu;
        if (village_menu) {
            const auto intent = ui::world_village_menu_input(
                layout, !desired_pause, (current.scripts.user_flags & 16U) != 0, menu_pending,
                village_selection, menu_input);
            if (intent) {
                menu_feedback.clear();
                switch (*intent) {
                case ui::WorldVillageMenuIntent::back:
                    village_menu = false;
                    break;
                case ui::WorldVillageMenuIntent::close:
                    pending_menu = session.close_main_menu();
                    break;
                case ui::WorldVillageMenuIntent::activities:
                    pending_menu = session.open_menu_village_activities();
                    break;
                case ui::WorldVillageMenuIntent::commerce:
                    pending_menu = session.open_menu_commerce();
                    break;
                }
            }
        }
        const auto menu_intent =
            was_village_menu
                ? std::nullopt
                : ui::world_menu_input(
                      layout, publication->main_menu_open,
                      !active_page(current) && current.scene.scene_state == 0, !desired_pause,
                      failed || pending_menu || pending_task || management.pending() ||
                          publication->save_menu_open || save_menu.pending(),
                      menu_selection, menu_input);
        if (menu_intent) {
            menu_feedback.clear();
            switch (*menu_intent) {
            case ui::WorldMenuIntent::open:
                pending_menu = session.open_main_menu();
                break;
            case ui::WorldMenuIntent::close:
                pending_menu = session.close_main_menu();
                break;
            case ui::WorldMenuIntent::build:
                pending_menu = session.open_menu_build();
                break;
            case ui::WorldMenuIntent::tasks:
                pending_menu = session.open_menu_tasks();
                break;
            case ui::WorldMenuIntent::village:
                village_menu = true;
                village_selection = 0;
                break;
            case ui::WorldMenuIntent::system:
                pending_menu = session.open_save_menu();
                break;
            }
        }
        if (publication->save_menu_open) {
            WorldSaveMenuInput save_input;
            save_input.click = click ? mouse : std::nullopt;
            save_input.up = IsKeyPressed(KEY_UP);
            save_input.down = IsKeyPressed(KEY_DOWN);
            save_input.left = IsKeyPressed(KEY_LEFT);
            save_input.right = IsKeyPressed(KEY_RIGHT);
            save_input.enter = IsKeyPressed(KEY_ENTER);
            save_input.escape = IsKeyPressed(KEY_ESCAPE);
            save_menu.input(*publication, extent, save_input, session);
        }
        // A pending open is already a local input barrier; it cannot leak T/drag/Enter to the
        // old scene while the simulation worker completes its previous atomic update.
        const bool menu_blocked = publication->main_menu_open || publication->save_menu_open ||
                                  pending_menu || save_menu.pending() || management.pending();
        if (!menu_blocked && mouse && CheckCollisionPointRec(*mouse, layout.scene) &&
            !active_page(current)) {
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                const float factor = destination.width / extent.width * zoom;
                const auto delta = GetMouseDelta();
                view.camera[0] -= delta.x / factor;
                view.camera[1] += delta.y / factor;
                view_changed = view_changed || delta.x != 0 || delta.y != 0;
            }
            if (const auto wheel = GetMouseWheelMove(); wheel) {
                world_zoom_camera(view, extent, *mouse, wheel, zoom);
                view_changed = true;
            }
        }
        if (view_changed && !failed && !publication->save_menu_open && !save_menu.pending())
            pending_view = session.set_view(view.camera, view.viewport);
        if (const auto *page = menu_blocked ? nullptr : active_page(current)) {
            if (viewed_page != page->id) {
                viewed_page = page->id;
                paragraph = scroll = 0;
                task_selection = {};
                task_feedback.clear();
            }
            if (management.input_page(current, *page, extent, mouse, click,
                                      desired_pause || failed || pending_task || pending_ack,
                                      session)) {
                // Management controller owns only selection and forwards explicit FIFO intents.
            } else if (ui::world_task_page(current, *page)) {
                const auto task = ui::world_task_view(current, *page);
                const auto task_layout = ui::world_task_layout(extent);
                ui::WorldTaskInput input;
                input.click = click ? mouse : std::nullopt;
                input.enter = IsKeyPressed(KEY_ENTER);
                input.escape = IsKeyPressed(KEY_ESCAPE);
                input.up = IsKeyPressed(KEY_UP);
                input.down = IsKeyPressed(KEY_DOWN);
                input.left = IsKeyPressed(KEY_LEFT);
                input.right = IsKeyPressed(KEY_RIGHT);
                if (mouse && CheckCollisionPointRec(*mouse, task_layout.rows))
                    input.wheel_rows = -static_cast<int>(GetMouseWheelMove() * 2);
                const auto intent = ui::world_task_input(task, task_layout, task_selection, input,
                                                         desired_pause || failed || pending_task);
                if (intent) {
                    task_feedback.clear();
                    pending_task =
                        session.act_task_page(page->id, intent->action, intent->selection);
                }
            } else if (page->kind == rules::WorldScriptPageKind::raw_page &&
                       page->legacy_page == 83) {
                const auto box = ui::world_page_layout(*page, extent);
                const Rectangle cancel{box.panel.x + 10, box.confirm.y, 58, 20};
                if (!desired_pause && !failed && !pending_task &&
                    (hit(cancel) || IsKeyPressed(KEY_ESCAPE)))
                    pending_task = session.cancel_page(page->id);
            } else if (page->kind == rules::WorldScriptPageKind::raw_page &&
                       page->legacy_page == 31) {
                const auto crew = ui::world_crew_summary_view(current, page->id);
                const auto crew_layout = ui::world_crew_summary_layout(extent);
                if (crew.initialized && !desired_pause && !failed && !pending_ack &&
                    (hit(crew_layout.confirm) || IsKeyPressed(KEY_ENTER)))
                    pending_ack = session.ack_page(page->id);
                if (mouse && CheckCollisionPointRec(*mouse, crew_layout.rows))
                    scroll = std::max(0, scroll - static_cast<int>(GetMouseWheelMove() * 2));
                const int visible = ui::world_crew_summary_visible_rows(crew_layout);
                scroll = std::clamp(scroll, 0,
                                    std::max(0, static_cast<int>(crew.rows.size()) - visible));
            } else if (page->legacy_page == 30) {
                const auto victory = ui::world_victory_view(current, *page);
                if (victory.initialized && !desired_pause && !failed && !pending_ack &&
                    (hit(ui::world_victory_layout(extent).confirm) || IsKeyPressed(KEY_ENTER)))
                    pending_ack = session.ack_page(page->id);
            } else if (ui::world_page_regular_confirmation(*page) &&
                       ui::world_task_related_confirmation(current, *page)) {
                const auto page_layout = ui::world_page_layout(*page, extent);
                if (!desired_pause && !failed && !pending_ack &&
                    (hit(page_layout.confirm) || IsKeyPressed(KEY_ENTER))) {
                    const auto decoded =
                        ui::decode_script_text(page_body(current, *page, paragraph));
                    const auto wrapped =
                        ui::wrap_plain_text(decoded.text, page_layout.body.width,
                                            [&](const auto &value) { return text.width(value); });
                    const int visible = std::max(1, static_cast<int>(page_layout.body.height / 17));
                    // Finishing a paragraph is separate from reaching the end of its visible slice.
                    if (scroll + visible < static_cast<int>(wrapped.size()))
                        scroll =
                            std::min(scroll + visible, static_cast<int>(wrapped.size()) - visible);
                    else if (paragraph + 1 < static_cast<int>(page->paragraphs.size())) {
                        ++paragraph;
                        scroll = 0;
                    } else
                        pending_ack = session.ack_page(page->id);
                }
                if (mouse && CheckCollisionPointRec(*mouse, page_layout.panel))
                    scroll = std::max(0, scroll - static_cast<int>(GetMouseWheelMove() * 2));
            }
        }
        // Held acceleration is page-bound and edge-triggered; 60 FPS cannot add source ticks.
        const auto *held_page = active_page(current);
        const auto held_layout = ui::world_task_layout(extent);
        const bool held =
            !menu_blocked && held_page && held_page->legacy_page == 24 && !desired_pause &&
            !failed && IsWindowFocused() &&
            (IsKeyDown(KEY_ENTER) || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && mouse &&
                                      CheckCollisionPointRec(*mouse, held_layout.confirm)));
        const auto next_held_page = held ? held_page->id : 0;
        if (held_task_page != next_held_page) {
            if (held_task_page)
                session.set_page_confirm_held(held_task_page, false);
            if (next_held_page)
                session.set_page_confirm_held(next_held_page, true);
            held_task_page = next_held_page;
        }
        const bool main_scene = !menu_blocked && !active_page(current);
        const bool scene_handled =
            main_scene && management.input_scene(current, view, extent, mouse, click, zoom,
                                                 desired_pause || failed || pending_task, session);
        if (main_scene && !scene_handled && current.scene.scene_state == 0 && !desired_pause &&
            !failed && !pending_task && IsKeyPressed(KEY_T)) {
            task_feedback.clear();
            pending_task = session.open_task_menu();
        }
        if (main_scene && !scene_handled && current.scene.scene_state == 0 && !desired_pause &&
            !failed && !pending_task && !pending_menu && IsKeyPressed(KEY_V))
            pending_menu = session.open_village_activities();
        BeginTextureMode(canvas.texture);
        ClearBackground(Color{145, 211, 247, 255});
        BeginMode2D(raster);
        const auto clip = layout.scene_clip;
        BeginScissorMode(static_cast<int>(std::floor(raster.offset.x + clip.x * raster.zoom)),
                         static_cast<int>(std::floor(raster.offset.y + clip.y * raster.zoom)),
                         static_cast<int>(std::ceil(clip.width * raster.zoom)),
                         static_cast<int>(std::ceil(clip.height * raster.zoom)));
        const double age =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - publication->published)
                .count();
        const float alpha = static_cast<float>(
            std::clamp(age / std::max(.001, publication->interval_seconds), 0.0, 1.0));
        draw_world_scene(current, sprites, zoom,
                         publication->revision <= discard_interpolation_revision
                             ? nullptr
                             : publication->previous.get(),
                         alpha, &view);
        if (!active_page(current) && !publication->main_menu_open && !publication->save_menu_open)
            management.draw_footprint(current, view, extent, mouse, zoom, sprites);
        EndScissorMode();
        if (!active_page(current) && !publication->main_menu_open && !publication->save_menu_open)
            management.draw_placement(current, view, extent, mouse, zoom, skin,
                                      !desired_pause && !failed);
        ui::draw_world_notices(ui::world_notice_view(current, extent), skin);
        // Flush these labels before opaque modal artwork, preserving both clip and paint order.
        EndMode2D();
        text.flush(raster.zoom, raster.offset);
        BeginMode2D(raster);
        hud(current, layout, skin, failed, publication->main_menu_open, pending_menu != 0);
        if (const auto *page = active_page(current)) {
            if (management.draw_page(current, *page, extent, skin,
                                     !desired_pause && !failed && !pending_ack && !pending_task)) {
                // Source-bound management pages are drawn by their own small UI modules.
            } else if (ui::world_task_page(current, *page)) {
                ui::draw_world_task(ui::world_task_view(current, *page),
                                    ui::world_task_layout(extent), skin, task_selection,
                                    !desired_pause && !failed && !pending_task, task_feedback);
            } else if (page->kind == rules::WorldScriptPageKind::raw_page &&
                       page->legacy_page == 31) {
                const auto crew = ui::world_crew_summary_view(current, page->id);
                const auto crew_layout = ui::world_crew_summary_layout(extent);
                ui::draw_world_crew_summary(crew, crew_layout, skin, scroll,
                                            !desired_pause && !failed && !pending_ack);
            } else if (page->legacy_page == 30) {
                ui::draw_world_victory(ui::world_victory_view(current, *page),
                                       ui::world_victory_layout(extent), skin,
                                       !desired_pause && !failed && !pending_ack);
            } else if (!ui::world_page_automatic(*page)) {
                // Timed waits and camera pages draw the world only; they do not expose a fake modal
                // or a confirmation capable of skipping their source-owned counter/focus consumer.
                const auto page_layout = ui::world_page_layout(*page, extent);
                ui::draw_world_page_chrome(*page, page_layout, skin, paragraph);
                ui::draw_world_task_monster(current, *page, page_layout.body, skin);
                const auto decoded = ui::decode_script_text(page_body(current, *page, paragraph));
                const auto wrapped =
                    ui::wrap_plain_text(decoded.text, page_layout.body.width,
                                        [&](const auto &value) { return text.width(value); });
                const int visible = std::max(1, static_cast<int>(page_layout.body.height / 17));
                scroll =
                    std::clamp(scroll, 0, std::max(0, static_cast<int>(wrapped.size()) - visible));
                for (int row = 0; row < visible && scroll + row < static_cast<int>(wrapped.size());
                     ++row) {
                    const float y = page_layout.body.y + row * 17;
                    if (decoded.centered)
                        skin.centered(wrapped[scroll + row],
                                      {page_layout.body.x, y, page_layout.body.width, 17});
                    else
                        text.draw(wrapped[scroll + row], page_layout.body.x, y);
                }
                const bool more_text = scroll + visible < static_cast<int>(wrapped.size());
                if (page->legacy_page == 83)
                    skin.button({page_layout.panel.x + 10, page_layout.confirm.y, 58, 20}, "取消",
                                !desired_pause && !failed && !pending_task);
                else if (ui::world_task_related_confirmation(current, *page))
                    skin.button(page_layout.confirm,
                                more_text ? "下一屏"
                                : paragraph + 1 < static_cast<int>(page->paragraphs.size())
                                    ? "下一页"
                                    : "确定",
                                !desired_pause && !failed && !pending_ack);
            }
        }
        if (publication->main_menu_open) {
            if (village_menu)
                ui::draw_world_village_menu(layout, skin, village_selection,
                                            !desired_pause && !failed && !pending_menu,
                                            (current.scripts.user_flags & 16U) != 0, menu_feedback);
            else
                ui::draw_world_menu(layout, skin, menu_selection,
                                    !desired_pause && !failed && !pending_menu, menu_feedback,
                                    !failed && !pending_menu);
        } else if (!publication->save_menu_open && !menu_feedback.empty())
            skin.text.draw(menu_feedback, 8, extent.height - 58.F, MAROON);
        save_menu.draw(*publication, extent, skin);
        EndMode2D();
        text.flush(raster.zoom, raster.offset);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            canvas.texture.texture,
            {0, 0, static_cast<float>(canvas.size.width), -static_cast<float>(canvas.size.height)},
            {0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
            {0, 0}, 0, WHITE);
        EndDrawing();
        render_statistics.cost(frames, (GetTime() - now) * 1000);
        if (save_inspection_driver.ready())
            ++frames;
    }
    session.stop();
    publication = session.frame();
    const auto &final_state = *publication->state;
    const bool failed = publication->failed;
    if (menu_inspection) {
        std::cout << "World menu inspection: open=" << publication->main_menu_open
                  << " explicit_pause=" << final_state.scene.framework_paused << '\n';
        if (!publication->main_menu_open || failed)
            throw std::runtime_error("World menu inspection did not receive successful FIFO open");
    }
    capture_world_screenshot(options, frames);
    std::cout << "World render: window=" << GetScreenWidth() << 'x' << GetScreenHeight()
              << " framebuffer=" << GetRenderWidth() << 'x' << GetRenderHeight()
              << " canvas=" << canvas.size.width << 'x' << canvas.size.height << '\n';
    std::cout << "World window: frames=" << frames << " rounds=" << final_state.simulation_steps
              << " cash=" << final_state.scene.world.world.ai.accounting.funds()
              << " humans=" << final_state.scene.world.world.ai.human_order.size()
              << " monsters=" << final_state.scene.world.world.ai.monster_order.size()
              << " report=" << final_state.report_state << '/' << final_state.report_counter
              << " date=" << final_state.scene.calendar.year << '/'
              << final_state.scene.calendar.month << '/' << final_state.scene.calendar.subperiod
              << '/' << final_state.scene.calendar.units
              << " random=" << final_state.scene.random.draws() << " failed=" << failed
              << " elapsed=" << GetTime() - started << '\n';
    const auto render_summary = render_statistics.finish();
    std::cout << "World pacing: heartbeat_ms=47 speed=" << final_state.scene.speed_setting + 1
              << " outer_updates=" << publication->outer_updates
              << " render_interval_p50_ms=" << render_summary.interval_p50_ms
              << " render_interval_p95_ms=" << render_summary.interval_p95_ms
              << " render_cost_p95_ms=" << render_summary.cost_p95_ms
              << " render_samples=" << render_summary.samples
              << " render_sampling=" << (options.frames > 0 ? "bounded" : "off")
              << " simulation_max_ms=" << publication->max_update_ms << '\n';
    if (failed)
        throw std::runtime_error(publication->error);
}
void run_world_game(const app::LaunchOptions &options, const std::filesystem::path &assets) {
    const bool home = options.inspect_page == "world-home-suite";
    if (options.inspect_page != "world-commerce-suite" && !home) {
        run_world_game_capture(options, assets);
        return;
    }
    std::optional<State> checkpoint;
    WorldManagementInspection inspection_checkpoint;
    const std::filesystem::path prefix(options.screenshot);
    const auto started = std::chrono::steady_clock::now();
    const std::vector<const char *> modes =
        home ? std::vector<const char *>{"world-home-credit", "world-home-rebuilt"}
             : std::vector<const char *>{"world-commerce",
                                         "world-commerce-buy",
                                         "world-commerce-receipt",
                                         "world-commerce-facilities",
                                         "world-commerce-facility-info",
                                         "world-commerce-facility-reward"};
    for (const auto *mode : modes) {
        const auto capture_started = std::chrono::steady_clock::now();
        auto capture = options;
        capture.inspect_page = mode;
        capture.screenshot =
            (prefix.parent_path() / (prefix.stem().string() + "-" + mode + ".png")).string();
        run_world_game_capture(capture, assets, &checkpoint,
                               home ? &inspection_checkpoint : nullptr);
        std::cout << "World suite capture: target=" << mode << " elapsed="
                  << std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                                   capture_started)
                         .count()
                  << '\n';
    }
    std::cout << "World inspection suite: captures=" << modes.size()
              << " natural_preparations=1 elapsed="
              << std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count()
              << '\n';
}
} // namespace ark::desktop
