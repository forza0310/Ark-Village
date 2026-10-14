#pragma once

// One static record plan serves title and in-game records; no Init list or animation Owner.
#include "../common/layout.hpp"
#include "ark/simulation/application/startup_system_records.hpp"
#include "ark/simulation/presentation/steam_record_skin.hpp"

namespace ark::desktop::ui {
class Skin;
struct WorldRecordsMeasurements {
    int topic_width{}, value_width{};
    float village_width{};
};
struct WorldRecordsLayout {
    Extent extent;
    int page{};
    Vector2 origin{};
    std::array<Rectangle, 2> arrows{};
    Rectangle back{};
};
struct WorldRecordsPaint {
    simulation::SteamRecordSkinPlan source;
    WorldRecordsLayout layout;
    std::array<simulation::SteamStartupFramePlan, 2> windows;
    std::array<simulation::SteamManualArrowPlan, 2> arrows;
    // Source body text order: topic, village, bottom label, value. PC fitting changes size only.
    std::array<float, 4> text_sizes{{12, 12, 12, 12}};
};
enum class WorldRecordsAction { none, turn, back };
std::optional<simulation::SteamRecordSkinPlan>
world_records_plan(const simulation::StartupSystemRecords &, int page, Extent,
                   const WorldRecordsMeasurements &);
WorldRecordsLayout world_records_layout(const simulation::SteamRecordSkinPlan &, int page, Extent,
                                        const std::array<simulation::SteamManualArrowPlan, 2> &);
// Mouse controls require this page's painted bounds; keys retain Left -> Right -> Back priority.
WorldRecordsAction world_records_input(const WorldRecordsLayout *, std::optional<Vector2> click,
                                       bool left, bool right, bool back);
WorldRecordsPaint prepare_world_records(const simulation::StartupSystemRecords &, int page, Extent,
                                        const Skin &);
void draw_world_records(const WorldRecordsPaint &, const Skin &);
} // namespace ark::desktop::ui
