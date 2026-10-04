#include "dungeon_village_reference/world_gift_page.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
namespace {
struct Failure {
    WorldGiftPageError error;
};
[[noreturn]] void fail(WorldGiftPageError error) { throw Failure{error}; }
int checked(std::int64_t value) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        fail(WorldGiftPageError::overflow);
    return static_cast<int>(value);
}
} // namespace
WorldGiftPageResult prepare_world_gift_page(const WorldGiftPageState &state,
                                            const WorldGiftPageInput &input,
                                            const WorldScriptCatalog &catalog) {
    try {
        const auto &scripts = state.facility.scripts;
        const auto found = std::find_if(scripts.pages.begin(), scripts.pages.end(),
                                        [&](const auto &value) { return value.id == input.page; });
        if (input.counter < 0 || found == scripts.pages.end() || found->lifecycle != 2 ||
            state.facility.finish.event_calls != scripts.event_calls ||
            state.facility.finish.dungeon.world.ai.pending_completion !=
                scripts.pending_completion ||
            !prepare_world_script_continuations(catalog, scripts, false).candidate)
            fail(WorldGiftPageError::invalid_owner);
        if (found->legacy_page != 94 || found->kind != WorldScriptPageKind::raw_page ||
            (found->legacy_r != 3 && (found->legacy_r < 5 || found->legacy_r > 8)))
            fail(WorldGiftPageError::unsupported_page);
        const int mode = found->legacy_r;
        const int definition = found->legacy_s;
        if (definition < 0)
            fail(WorldGiftPageError::invalid_owner);
        WorldGiftPageCandidate candidate{state, input.counter, false, false, {}, {}, {}};
        auto &next = candidate.state;
        if (input.counter == 1)
            candidate.sound = 5;
        if (!input.confirm_pressed)
            return {WorldGiftPageError::none, std::move(candidate)};
        if (input.counter < 40) {
            candidate.counter = 40;
            return {WorldGiftPageError::none, std::move(candidate)};
        }
        auto invoke = [&](int event) {
            const auto result =
                prepare_world_script(catalog, next.facility.scripts, {event, {}, {}});
            if (!result.candidate)
                fail(WorldGiftPageError::script_failed);
            next.facility.scripts = result.candidate->state;
            next.facility.finish.event_calls = next.facility.scripts.event_calls;
            next.facility.finish.dungeon.world.ai.pending_completion =
                next.facility.scripts.pending_completion;
            candidate.scripts.insert(candidate.scripts.end(), result.candidate->executed.begin(),
                                     result.candidate->executed.end());
            candidate.pages.insert(candidate.pages.end(), result.candidate->inserted_pages.begin(),
                                   result.candidate->inserted_pages.end());
        };
        if (mode == 3) {
            const auto item = next.facility_unlocks.find(definition);
            if (item == next.facility_unlocks.end() || item->second.status < 0 ||
                item->second.free_builds < 0 || item->second.free_builds > 99)
                fail(WorldGiftPageError::invalid_owner);
            auto &value = item->second;
            value.free_builds =
                std::min(checked(static_cast<std::int64_t>(value.free_builds) + 1), 99);
            if (value.status == 0) {
                value.pending_notice = true;
                value.status = 2;
                value.unlock_counter = 0;
            }
        } else {
            const int kind = mode == 8 ? 0 : mode - 4;
            const auto item = next.facility.finish.dungeon.catalog.find({kind, definition});
            if (item == next.facility.finish.dungeon.catalog.end() || item->second.status < 0)
                fail(WorldGiftPageError::invalid_owner);
            auto &value = item->second;
            if (mode == 8) {
                if (value.inventory < 0 || value.inventory > 999)
                    fail(WorldGiftPageError::invalid_owner);
                value.inventory =
                    std::min(checked(static_cast<std::int64_t>(value.inventory) + 1), 999);
                // 原a.g字段r投影是ObjectCatalogRecord.newly_unlocked；不因误名另造一份状态。
                if (value.status == 0) {
                    value.newly_unlocked = true;
                    value.status = 1;
                    value.unlock_counter = 0;
                }
            } else {
                if (value.status == 0) {
                    value.newly_unlocked = true;
                    value.free_purchases = 1;
                    if (mode == 5 && (value.flags & 32U) != 0 &&
                        !world_script_seen(next.facility.scripts, 216))
                        invoke(216);
                    const int shop = mode == 5 ? 1 : mode == 6 ? 4 : 5;
                    for (auto identity : next.facility_order) {
                        const auto facility =
                            next.facility.finish.dungeon.world.facilities.find(identity);
                        const auto detail = next.facility.details.find(identity);
                        if (facility == next.facility.finish.dungeon.world.facilities.end() ||
                            detail == next.facility.details.end())
                            fail(WorldGiftPageError::invalid_owner);
                        if (facility->second.detail != shop)
                            continue;
                        auto &notices = detail->second.notices;
                        if (std::none_of(notices.begin(), notices.end(),
                                         [](const auto &entry) { return entry[0] == 7; }))
                            notices.push_back({7, 0});
                    }
                }
                value.status = 1; // 原p非0也写1，但不补免费库存、不重排shop7。
            }
        }
        const auto closed = prepare_world_script_close_page(next.facility.scripts, input.page);
        if (!closed.candidate)
            fail(WorldGiftPageError::close_failed);
        next.facility.scripts = closed.candidate->state;
        candidate.claimed = candidate.closed = true;
        return {WorldGiftPageError::none, std::move(candidate)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
} // namespace dungeon_village_reference
