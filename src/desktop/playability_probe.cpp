#include "playability_probe.hpp"
#include "character_visibility.hpp"
#include "ui/controller.hpp"
#include <algorithm>
#include <ostream>
#include <stdexcept>

namespace ark::desktop {
namespace {
Vector2 center(Rectangle rectangle) {
    return {rectangle.x + rectangle.width / 2, rectangle.y + rectangle.height / 2};
}
bool same_position(world::WorldPosition a, world::WorldPosition b) {
    return a.x == b.x && a.z == b.z;
}
} // namespace

void PlayabilityProbe::observe(const app::Game &game) {
    const auto &state = game.state();
    if (const auto *life = game.life_state()) {
        if (last_position_ && !same_position(*last_position_, life->position))
            moved_ = true;
        last_position_ = life->position;
        if (life->completions > last_completions_)
            for (const auto id : last_occupied_) {
                const auto &occupants = state.facility_life.at(id).occupants;
                if (std::find(occupants.begin(), occupants.end(), life->actor) == occupants.end()) {
                    released_ = true;
                    shown_after_exit_ = shown_after_exit_ || character_visible(game);
                }
            }
        last_completions_ = life->completions;
    }
    for (const auto &[id, entry] : state.accounting.entries()) {
        (void)id;
        if (entry.direction == economy::CashDirection::income && entry.amount > 0)
            income_ = true;
    }
    last_occupied_.clear();
    for (const auto &[id, service] : state.facility_life) {
        if (game.life_state() && std::find(service.occupants.begin(), service.occupants.end(),
                                           game.life_state()->actor) != service.occupants.end()) {
            occupied_ = true;
            if (!character_visible(game))
                hidden_using_ = true;
            last_occupied_.push_back(id);
        }
    }
    if (state.money != state.accounting.funds())
        throw std::runtime_error("Playability probe: cash projection diverged from ledger");
}

void PlayabilityProbe::drive(app::Game &game, ui::State &view, const ui::Layout &layout) {
    observe(game);
    const auto click = [&](Rectangle rectangle) {
        ui::click(game, view, layout, center(rectangle));
    };
    const auto &state = game.state();
    switch (stage_) {
    case Stage::arrival:
        if (state.mode == app::Mode::tutorial) {
            click(layout.dialogue); // Both original dialogue lines take their normal command path.
        } else if (state.adventurer && state.mode == app::Mode::normal) {
            click(layout.right_button);
            stage_ = Stage::menu;
        }
        break;
    case Stage::menu:
        if (view.page != ui::Page::menu)
            throw std::runtime_error("Playability probe: menu did not open");
        click(layout.menu_rows[0]);
        stage_ = Stage::catalog;
        break;
    case Stage::catalog:
        if (state.mode != app::Mode::catalog)
            throw std::runtime_error("Playability probe: catalog did not open");
        click(layout.tabs[2]);
        stage_ = Stage::tab;
        break;
    case Stage::tab: {
        const auto items = ui::catalog_items(view.tab);
        if (items.empty() || items.front()->id != 33)
            throw std::runtime_error("Playability probe: published bun shop row changed");
        click(layout.rows[0]);
        stage_ = state.mode == app::Mode::placement ? Stage::placement : Stage::row;
        break;
    }
    case Stage::row:
        click(layout.rows[0]);
        stage_ = Stage::placement;
        break;
    case Stage::placement: {
        if (state.mode != app::Mode::placement)
            throw std::runtime_error("Playability probe: build selection failed");
        const auto bounds = app::startup_data().build_bounds;
        for (int y = bounds.min_y; y <= bounds.max_y && !build_cell_; ++y)
            for (int x = bounds.min_x; x <= bounds.max_x && !build_cell_; ++x) {
                const world::Cell cell{x, y};
                const auto point = tile_center(cell, view.camera, layout.extent, view.zoom);
                if (game.preview(cell) == app::Error::none &&
                    CheckCollisionPointRec(point, layout.scene) &&
                    !CheckCollisionPointRec(point, layout.pause_button))
                    build_cell_ = cell;
            }
        if (!build_cell_)
            throw std::runtime_error("Playability probe: no visible legal build cell");
        if (view.selection != build_cell_)
            ui::click(game, view, layout,
                      tile_center(*build_cell_, view.camera, layout.extent, view.zoom));
        stage_ = Stage::place;
        break;
    }
    case Stage::place: {
        const auto old_funds = state.accounting.funds();
        const auto old_entries = state.accounting.entries().size();
        const auto old_facilities = state.facilities.size();
        const auto old_revision = state.layout_revision;
        ui::click(game, view, layout,
                  tile_center(*build_cell_, view.camera, layout.extent, view.zoom));
        built_ = game.facility_at(*build_cell_);
        if (!built_ || game.state().facilities.at(*built_).definition_id != 33)
            throw std::runtime_error("Playability probe: controller did not commit construction");
        if (state.accounting.funds() != old_funds - game.definition(33).price ||
            state.accounting.entries().size() != old_entries + 1 ||
            state.facilities.size() != old_facilities + 1 || state.layout_revision <= old_revision)
            throw std::runtime_error("Playability probe: build/map/cash transaction diverged");
        const auto &payment = state.accounting.entries().rbegin()->second;
        if (payment.direction != economy::CashDirection::expense ||
            payment.amount != game.definition(33).price)
            throw std::runtime_error("Playability probe: construction cash entry missing");
        stage_ = Stage::close;
        break;
    }
    case Stage::close:
        click(layout.right_button); // Placement -> catalog -> menu -> village.
        if (game.state().mode == app::Mode::normal && view.page == ui::Page::village)
            stage_ = Stage::pause;
        break;
    case Stage::pause:
        click(layout.pause_button);
        if (!state.paused || !state.life || state.facilities.at(*built_).remaining_ticks <= 0)
            throw std::runtime_error("Playability probe: construction pause did not start");
        frozen_ = state;
        stage_ = Stage::resume;
        break;
    case Stage::resume:
        if (!state.paused || state.simulation_steps != frozen_->simulation_steps ||
            state.calendar != frozen_->calendar || state.money != frozen_->money || !state.life ||
            state.life->rounds != frozen_->life->rounds ||
            !same_position(state.life->position, frozen_->life->position))
            throw std::runtime_error("Playability probe: paused world advanced");
        for (const auto &[id, instance] : state.facilities)
            if (instance.remaining_ticks != frozen_->facilities.at(id).remaining_ticks)
                throw std::runtime_error("Playability probe: paused construction advanced");
        if (++pause_frames_ >= 30) {
            pause_verified_ = true;
            click(layout.pause_button);
            stage_ = Stage::run;
        }
        break;
    case Stage::run:
        if (!state.paused && state.simulation_steps > frozen_->simulation_steps && state.life &&
            state.life->rounds > frozen_->life->rounds)
            resumed_ = true;
        break;
    }
}

bool PlayabilityProbe::passed(const app::Game &game) const {
    const auto &state = game.state();
    const auto *life = game.life_state();
    return state.event89_count == 1 && life && moved_ && income_ && occupied_ && released_ &&
           life->completions > 0 && built_ && state.facilities.at(*built_).remaining_ticks == 0 &&
           pause_verified_ && resumed_ && hidden_using_ && shown_after_exit_ &&
           state.money == state.accounting.funds() &&
           (life->error == app::InitialAiError::none ||
            life->error == app::InitialAiError::unsupported_branch);
}

void PlayabilityProbe::report(std::ostream &output, const app::Game &game) const {
    output << "Playability controller probe: " << (passed(game) ? "PASS" : "FAIL")
           << " input=logical-controller arrival=" << game.state().event89_count
           << " moved=" << moved_ << " income=" << income_ << " occupied=" << occupied_
           << " released=" << released_
           << " completions=" << (game.life_state() ? game.life_state()->completions : 0)
           << " construction="
           << (built_ && game.state().facilities.at(*built_).remaining_ticks == 0)
           << " remaining_ticks="
           << (built_ ? game.state().facilities.at(*built_).remaining_ticks : -1)
           << " pause_frozen=" << pause_verified_ << " resumed=" << resumed_
           << " hidden_using=" << hidden_using_ << " shown_after_exit=" << shown_after_exit_
           << " handoff="
           << (game.life_state() &&
               game.life_state()->error == app::InitialAiError::unsupported_branch)
           << " pending_category="
           << (game.life_state() && game.life_state()->pending_category
                   ? *game.life_state()->pending_category
                   : -1)
           << " pending_definition="
           << (game.life_state() && game.life_state()->pending_definition
                   ? *game.life_state()->pending_definition
                   : -1)
           << '\n';
}
} // namespace ark::desktop
