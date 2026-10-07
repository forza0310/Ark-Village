// research5aa5c37 TASK_REPORT_RENDER supplies the fixed-APK artwork/data contract.
// Victory confirmation is source-gated; month overlays advance automatically in the runtime.
#include "world_reports.hpp"
#include "ark/simulation/startup_world_visuals.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
ReportPortrait human(const State &s, int id) {
    const auto person = std::find_if(s.rules->humans.begin(), s.rules->humans.end(),
                                     [id](const auto &value) { return value.identity == id; });
    const auto growth = s.scene.world.world.ai.growth.find(id);
    if (person == s.rules->humans.end() || growth == s.scene.world.world.ai.growth.end())
        throw std::invalid_argument("Report references an unknown current human portrait");
    return {id, 1,
            s.rules->jobs.at(growth->second.definition.current_profession).sprites.at(person->sex)};
}
void money(const Skin &skin, int value, Vector2 right, const char *sprite) {
    skin.number(value, {right.x - 8, right.y}, sprite);
    skin.sprites.draw(sprite, 20, {right.x - 8, right.y}, WHITE, Sprites::Binding::common);
}
} // namespace
WorldVictoryView world_victory_view(const State &s, const simulation::rules::WorldScriptPage &p) {
    if (p.legacy_page != 30 || p.kind != simulation::rules::WorldScriptPageKind::raw_page)
        throw std::invalid_argument("Victory view requires raw30");
    WorldVictoryView v;
    const auto bound = s.exploration_summaries.find(p.id);
    if (bound == s.exploration_summaries.end())
        return v;
    if (p.task_identity != bound->second.task || p.task_definition != bound->second.definition)
        throw std::invalid_argument("Victory page and retained summary identities disagree");
    const auto task =
        std::find_if(s.rules->tasks.begin(), s.rules->tasks.end(),
                     [&](const auto &t) { return t.factory.identity == bound->second.definition; });
    if (task == s.rules->tasks.end())
        throw std::invalid_argument("Victory task definition missing");
    v.initialized = true;
    v.task = bound->second.task;
    v.definition = bound->second.definition;
    v.name = task->name;
    v.experience = p.legacy_f; // A display parameter, never per-person final credited experience.
    v.popularity = task->factory.pending_completion_value;
    if (const auto i = s.page_phases.find(p.id); i != s.page_phases.end())
        v.phase = i->second;
    if (const auto i = s.page_counters.find(p.id); i != s.page_counters.end())
        v.counter = i->second;
    // Source m survives task retirement; modal input cannot start a replacement task here.
    for (const int id : s.participants)
        v.members.push_back(human(s, id));
    return v;
}
WorldPageLayout world_victory_layout(Extent e) {
    const float x = (e.width - 222.F) / 2, y = (e.height - 196.F) / 2;
    return {{x, y, 222, 170}, {x + 12, y + 110, 198, 53}, {x + 154, y + 176, 68, 20}};
}
void draw_world_victory(const WorldVictoryView &v, const WorldPageLayout &l, const Skin &skin,
                        bool interactive) {
    skin.window(l.panel, "胜利！");
    const float x = l.panel.x, y = l.panel.y;
    // Image39 contains the original sky/grass strip, extended using source pixels, not the map.
    skin.tile("mp_back.png", {1, 1, 91, 79}, {x + 21, y + 27, 179, 79});
    DrawRectangleLinesEx({x + 20, y + 26, 181, 81}, 1, ink);
    skin.content(l.body);
    if (v.initialized) {
        skin.sprites.draw("wnd_exp.seb", 0, {x + 67, y + 34}, WHITE, Sprites::Binding::common);
        skin.number(v.experience, {x + 145, y + 33}, "number05.seb");
        const float spacing = 166.F / std::max<std::size_t>(1, v.members.size());
        for (std::size_t i = 0; i < v.members.size(); ++i) {
            const Vector2 foot{x + 28 + spacing * (i + .5F), y + 99};
            skin.sprites.draw("shadow00.seb", 0, foot, WHITE, Sprites::Binding::common);
            // The published body binding is current profession/sex. Precise jump trajectory
            // and held-weapon composition are not supplied by this static contract.
            skin.sprites.actor(false, v.members[i].sprite, v.members[i].image, 0, foot);
        }
        const float revealed = std::clamp((v.counter - 5) / 35.F, 0.F, 1.F) * l.body.height;
        const Rectangle clip{l.body.x, l.body.y, l.body.width, revealed};
        const auto line = [&](const std::string &text, float yy, Color color = ink) {
            skin.text.clipped(text, x + 111 - skin.text.width(text, 11) / 2, yy, clip, color, 11);
        };
        if (v.phase == 0) {
            line(v.name, y + 117);
            line("完成!!", y + 140);
        } else {
            line("因为完成任务", y + 112);
            line("街道的人气 " + std::to_string(v.popularity), y + 129, blue);
            line("上升了!", y + 146);
        }
    }
    // Early presses fast-forward only. Each phase retains its own source >=40 close gate.
    skin.button(l.confirm, v.counter < 40 ? "显示全文" : "确定", interactive && v.initialized);
}
WorldMonthView world_month_view(const State &s) {
    WorldMonthView v;
    v.phase = s.report_state;
    // Published entry/exit endpoints are six source counters over 104 pixels. Linear
    // positioning between those endpoints is a desktop rendering adaptation, never a clock.
    if (v.phase == 1)
        v.offset_x = -104.F * (1 - std::clamp(s.report_counter / 6.F, 0.F, 1.F));
    else if (v.phase == 2)
        v.offset_x = -104.F * std::clamp((s.report_counter - 64) / 6.F, 0.F, 1.F);
    v.defeats = s.report_snapshot[0];
    v.points = s.report_snapshot[1];
    v.income = s.report_snapshot[2];
    v.expenses = s.report_snapshot[3];
    v.balance = s.report_snapshot[4];
    v.record = s.report_new_records[4];
    if (v.phase != 1)
        return v;
    if (v.defeats == 0) {
        v.human_image = human(s, s.report_snapshot[5]).image;
        return v;
    }
    v.ellipsis = v.defeats > 4;
    const int limit = v.ellipsis ? 3 : std::min(4, v.defeats);
    for (int i = 0; i < limit; ++i) {
        const int id = s.report_portraits[i];
        if (id == -1)
            break;
        const auto &m = s.scene.world.world.ai.monster_growth.at(id);
        v.monsters.push_back({id, m.body * 4 + 1, m.body * 30 + m.sprite_variant});
    }
    return v;
}
void draw_world_month(const WorldMonthView &v, const Skin &skin) {
    const float dx = v.offset_x;
    const Rectangle box{dx, 24, 111, 61};
    skin.tile("wnd_back.png", {0, 0, 4, 240}, box, Sprites::Binding::window);
    DrawRectangleLinesEx(box, 1, ink);
    skin.content({dx + 3, 27, 105, 55});
    if (v.phase == 1) {
        skin.centered(v.defeats ? "讨伐怪物" : "本月", {dx + 3, 27, 105, 12}, ink, 10);
        if (v.defeats) {
            for (std::size_t i = 0; i < v.monsters.size(); ++i) {
                const auto &p = v.monsters[i];
                const float x = dx + 7 + 16.F * i;
                skin.sprites.image("icon_back00.png", {0, 0, 18, 18}, {x, 42, 18, 18});
                skin.sprites.actor_thumbnail(true, p.sprite, p.image, {x + 1, 43, 16, 16});
            }
            if (v.ellipsis)
                skin.text.draw("...", dx + 59, 47, ink, 10);
            skin.number(v.defeats, {dx + 106, 46}, "number13.seb");
            skin.sprites.draw("icon_result00.seb", 4, {dx + 9, 65}, WHITE,
                              Sprites::Binding::common);
            skin.number(v.points, {dx + 65, 67}, "number13.seb");
            skin.text.draw("获得!", dx + 70, 67, ink, 10);
        } else {
            skin.sprites.image("icon_back00.png", {0, 0, 18, 18}, {dx + 7, 43, 18, 18});
            if (v.human_image)
                skin.sprites.human_image(*v.human_image, {1, 27, 15, 14}, {dx + 8, 44, 15, 14});
            skin.text.draw("击倒 0", dx + 31, 46, ink, 10);
            skin.text.draw("真遗憾", dx + 35, 67, ink, 10);
        }
    } else {
        // r3 remains its separate source path; no rewards or ledger are changed here.
        const char *labels[]{"收入", "经费", "合计"};
        const int values[]{v.income, v.expenses, v.balance};
        for (int i = 0; i < 3; ++i) {
            skin.text.draw(labels[i], dx + 5, 32 + 17.F * i, ink, 10);
            money(skin, values[i], {dx + 107, 33 + 17.F * i},
                  i == 1 || values[i] < 0 ? "number12.seb" : "number05.seb");
        }
        if (v.record)
            skin.sprites.draw("icon_result00.seb", 2, {dx + 113, 66}, WHITE,
                              Sprites::Binding::common);
    }
}
} // namespace ark::desktop::ui
