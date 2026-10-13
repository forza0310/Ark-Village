#include "world_magic_pot.hpp"
#include "../common/skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
const simulation::rules::WorldMagicPotRecipeDefinition &recipe(const State &s, int id) {
    const auto r =
        std::find_if(s.rules->magic_pot_recipes.begin(), s.rules->magic_pot_recipes.end(),
                     [=](const auto &v) { return v.identity == id; });
    if (r == s.rules->magic_pot_recipes.end())
        throw std::invalid_argument("Magic pot lost recipe definition");
    return *r;
}
std::string reward_name(const State &s, const simulation::rules::WorldMagicPotRecipeDefinition &r) {
    if (r.reward_kind == 0) {
        for (const auto &v : s.rules->items)
            if (v.identity == r.reward_definition)
                return v.name;
    } else if (r.reward_kind < 4) {
        for (const auto &v : s.rules->equipment)
            if (v.shop.kind == r.reward_kind && v.shop.id == r.reward_definition)
                return v.name;
    } else if (r.reward_kind == 4) {
        for (const auto &v : s.rules->facilities)
            if (v.id == r.reward_definition)
                return v.name;
    }
    throw std::invalid_argument("Magic pot lost actual reward definition");
}
} // namespace
bool world_magic_pot_page(const simulation::rules::WorldScriptPage &p) {
    return p.kind == simulation::rules::WorldScriptPageKind::raw_page && p.legacy_page >= 41 &&
           p.legacy_page <= 47;
}
WorldMagicPotView world_magic_pot_view(const State &s,
                                       const simulation::rules::WorldScriptPage &p) {
    if (!s.rules || !world_magic_pot_page(p))
        throw std::invalid_argument("Unsupported magic pot page");
    WorldMagicPotView v;
    v.page = p.id;
    v.raw = p.legacy_page;
    v.title = "魔法壶";
    const auto source = simulation::inspect_startup_world_magic_pot_page(s, p.id);
    if (!source)
        return v;
    const auto live = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [&](const auto &entry) { return entry.id == p.id; });
    // Catalogue plans accept Init-complete payloads. Effect plans require the first live frame.
    if (live == s.scripts.pages.end() || (v.raw >= 44 && live->lifecycle == 0))
        return v;
    v.phase = source->phase;
    v.selection = source->selection;
    v.first_visible = source->first_visible;
    v.counter = source->counter;
    v.binding = source->binding;
    v.can_cancel = v.raw != 45;
    v.can_confirm = v.raw <= 43 || (v.raw == 44 && v.counter >= 36) ||
                    (v.raw == 45 && (v.counter < 77 || v.counter >= 83)) ||
                    (v.raw >= 46 && v.counter > 6);
    if (v.raw == 41)
        v.rows = {{0, "放入道具"}, {1, "查看调和书"}};
    else if (v.raw == 42) {
        for (int id : source->entries) {
            const auto r = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                                        [=](const auto &i) { return i.identity == id; });
            if (r == s.rules->items.end())
                throw std::invalid_argument("Magic pot lost owned item");
            v.rows.push_back({id, r->name});
        }
        v.can_confirm = !v.rows.empty();
    } else if (v.raw == 43) {
        v.title = "选择调和书 " + std::to_string(v.phase + 1) + "/2";
        for (int id : source->entries) {
            const auto &r = recipe(s, id);
            const bool known = s.magic_pot_recipes.at(id).status != 0;
            v.rows.push_back({id, known ? reward_name(s, r) : "？？？？？", known});
        }
        v.can_confirm = !v.rows.empty() && v.rows.at(v.selection).known;
    } else if (v.raw == 44)
        v.comment = s.magic_pot_comment;
    else if (v.raw == 45)
        v.title = "研究报告";
    else {
        v.title = "调和";
        const auto &r = recipe(s, v.binding);
        v.reward_name = reward_name(s, r);
        v.affordable = true;
        for (int i = 0; i < 4; ++i)
            v.affordable &= s.legacy_n[3 + i] >= r.costs[i];
    }
    v.initialized = true;
    v.interactive = live->lifecycle != 4 && !s.scene.framework_paused;
    return v;
}
WorldMagicPotLayout world_magic_pot_layout(Extent e, int raw) {
    if (e.width < 240 || e.height < 256 || raw < 41 || raw > 47)
        throw std::invalid_argument("Unsupported magic pot viewport/page");
    WorldMagicPotLayout l;
    l.extent = e;
    const float h = raw >= 46 ? 135.F : 168.F;
    l.panel = {(e.width - 220.F) / 2, (e.height - h) / 2, 220, h};
    const auto origin = world_list_skin_origin(l.panel);
    l.rows = {origin.x + 23, origin.y + 95, 191, 95};
    l.cancel = {l.panel.x + 170, l.panel.y + h + 4, 50, 18};
    l.confirm = {l.panel.x, l.panel.y + h + 4, 50, 18};
    l.feedback = {l.panel.x, l.panel.y + h + 24, 220, 12};
    return l;
}
// Measured text feeds only the pure plan. It never refreshes inventory or processes dates.
void prepare_world_magic_pot(WorldMagicPotView &v, const State &s, const Skin &skin,
                             bool scrolling) {
    v.skin.reset();
    if (!v.initialized)
        return;
    simulation::SteamMagicPotOptions o;
    o.view_y = 23;
    o.scroll_first_touch = scrolling;
    o.title_widths =
        std::array<int, 2>{int(skin.text.width(v.title)), int(skin.text.width(v.title))};
    if (v.raw <= 43) {
        o.button_widths = {int(skin.text.width("放入道具")), int(skin.text.width("查看调和书"))};
        o.hint_width =
            int(skin.text.width(v.selection ? "使用魔法点数生成道具" : "放入道具获得魔法点数"));
        v.skin = simulation::steam_magic_pot_skin(s, v.page, o);
    } else {
        simulation::SteamMagicPotEffectOptions effect;
        effect.common = o;
        if (v.raw == 44 && v.counter >= 97)
            effect.comment_width = int(skin.text.width(v.comment));
        if (v.raw == 45)
            effect.report_header_widths = std::array<int, 4>{
                int(skin.text.width(std::to_string(s.legacy_n[2]))),
                int(skin.text.width("素材", 11)), int(skin.text.width("消费", 11)),
                int(skin.text.width("素材", 11))};
        if (v.raw >= 46) {
            effect.reward_name_width = int(skin.text.width(v.reward_name));
            effect.confirm_width =
                int(skin.text.width(v.raw == 47 && v.affordable ? "制作!" : "了解"));
        }
        v.skin = simulation::steam_magic_pot_effect_skin(s, v.page, effect);
    }
    if (!v.skin)
        throw std::invalid_argument("Invalid bound magic pot plan");
}
} // namespace ark::desktop::ui
