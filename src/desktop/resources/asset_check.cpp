// CPU image/SEB validation for --check; does not require a GPU texture or running world.
#include "ark/simulation/world/startup.hpp"
#include "resources.hpp"
#include "resource_metadata.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tuple>

namespace ark::desktop {
using namespace resource_detail;
void check_assets(const std::filesystem::path &root) {
    for (const auto &[name, width, height] :
         {std::tuple{"number05.png", 100, 21}, std::tuple{"number08.png", 100, 21},
          std::tuple{"icon_param00.png", 112, 32}, std::tuple{"wnd_ato.png", 10, 7}}) {
        auto image = LoadImage((root / "steam_common" / name).string().c_str());
        const bool valid = image.data && image.width == width && image.height == height;
        if (image.data)
            UnloadImage(image);
        if (!valid)
            throw std::runtime_error("Missing or invalid Steam common image: " + std::string(name));
    }
    for (const auto &[name, width, height] :
         {std::tuple{"title00.png", 600, 380}, std::tuple{"upper.png", 240, 9},
          std::tuple{"title_grass.png", 240, 18}}) {
        auto image = LoadImage((root / "steam_title" / name).string().c_str());
        const bool valid = image.data && image.width == width && image.height == height;
        if (image.data)
            UnloadImage(image);
        if (!valid)
            throw std::runtime_error("Missing or invalid Steam title image: " + std::string(name));
    }
    for (const auto &[name, width, height] :
         {std::tuple{"title00.png", 240, 330}, std::tuple{"title_logo.png", 236, 115},
          std::tuple{"title_window.png", 98, 68}}) {
        auto image = LoadImage((root / "title" / name).string().c_str());
        const bool valid = image.data && image.width == width && image.height == height;
        if (image.data)
            UnloadImage(image);
        if (!valid)
            throw std::runtime_error("Missing or invalid title image: " + std::string(name));
    }
    const auto images = image_index(root);
    const auto common_images = image_index(root, "common");
    const auto common2_images = image_index(root, "common2");
    for (const auto &[name, width, height] :
         {std::tuple{"road4block00.png", 30, 20}, std::tuple{"road4block01.png", 27, 15}}) {
        auto image = LoadImage((root / "common" / name).string().c_str());
        const bool valid_size = image.data && image.width == width && image.height == height;
        if (image.data)
            UnloadImage(image);
        if (!valid_size)
            throw std::runtime_error("Road patch missing or wrong dimensions");
    }
    // Structural validation retains unused source records, including out-of-atlas legacy records.
    // Pixel bounds apply to the frames this finite adapter actually requests (research/assets).
    for (const auto &entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() == ".png") {
            const auto image = LoadImage(entry.path().string().c_str());
            if (!image.data || image.width <= 0 || image.height <= 0)
                throw std::runtime_error("Packaged PNG failed decode");
            UnloadImage(image);
        } else if (entry.path().extension() == ".seb") {
            assets::parse_legacy_seb(read_bytes(entry.path()));
        }
    }
    const auto validate_frame = [&](const std::filesystem::path &sprite_path, int frame,
                                    Sprites::Binding binding) {
        const auto sprite = assets::parse_legacy_seb(read_bytes(sprite_path));
        if (binding == Sprites::Binding::map) {
            frame = map_frame_index(sprite_path.filename().string(), sprite, frame);
            if (frame >= sprite.frame_count)
                return;
        }
        if (frame < 0 || frame >= sprite.frame_count)
            throw std::runtime_error("Requested sprite frame invalid: " + sprite_path.string());
        for (const auto &layer : sprite.layers)
            for (const auto &part : layer.parts) {
                if (part.frame != frame)
                    continue;
                if (binding == Sprites::Binding::secretary && part.image_index != 126)
                    throw std::runtime_error("Secretary image binding changed");
                const auto path =
                    binding == Sprites::Binding::farmer      ? root / "human/chara_flower00.png"
                    : binding == Sprites::Binding::secretary ? root / "common/chara_hishoko01.png"
                    : binding == Sprites::Binding::steam_common
                        ? root / "steam_common" / common_images.at(part.image_index)
                    : binding == Sprites::Binding::common
                        ? root / "common" / common_images.at(part.image_index)
                    : binding == Sprites::Binding::common2
                        ? root / "common2" / common2_images.at(part.image_index)
                        : root / "image" / images.at(part.image_index);
                auto image = LoadImage(path.string().c_str());
                if (!image.data)
                    throw std::runtime_error("Requested sprite image missing: " + path.string());
                try {
                    validate(part, image.width, image.height);
                } catch (...) {
                    UnloadImage(image);
                    throw;
                }
                UnloadImage(image);
            }
    };
    std::map<std::string, std::set<int>> requested;
    // Construction can remove an existing road: every adjacency frame can become reachable.
    for (int frame = 0; frame < 16; ++frame)
        requested["road00.seb"].insert(frame);
    const auto &evidence = simulation::startup_evidence();
    for (const auto &cell : evidence.cells) {
        const auto it = std::find_if(evidence.displays.begin(), evidence.displays.end(),
                                     [&](const auto &d) { return d.id == cell.display_id; });
        if (it == evidence.displays.end())
            throw std::runtime_error("Map references an unknown display");
        requested[it->sprite].insert(cell.variant);
    }
    // Validate the maintained complete-world catalogue in both authored orientations.
    for (const auto &item : evidence.definitions) {
        if ((!(item.flags & 4) && item.kind != 12) || item.kind == 6)
            continue;
        const auto display = std::find_if(evidence.displays.begin(), evidence.displays.end(),
                                          [&](const auto &d) { return d.id == item.display_id; });
        if (display == evidence.displays.end())
            throw std::runtime_error("Building references an unknown map display");
        for (const auto orientation : {simulation::rules::FacilityOrientation::first,
                                       simulation::rules::FacilityOrientation::second}) {
            const auto footprint = simulation::rules::facility_footprint(
                static_cast<simulation::rules::FacilityShape>(item.shape), orientation, {1, 0}, 3,
                3);
            if (footprint.error != simulation::rules::GeometryError::none)
                throw std::runtime_error("Building has an unsupported footprint");
            for (const auto &part : footprint.cells)
                requested[display->sprite].insert(part.fragment_index);
        }
    }
    for (const auto &entry : requested)
        for (int frame : entry.second)
            validate_frame(root / "image" / entry.first, frame, Sprites::Binding::map);
    for (const auto *sprite : {"walk00.seb", "walk01.seb", "walk02.seb", "walk03.seb"})
        for (int frame = 0; frame < 4; ++frame)
            validate_frame(root / "human" / sprite, frame, Sprites::Binding::farmer);
    validate_frame(root / "common/chara_hishoko01.seb", 0, Sprites::Binding::secretary);
    // Every expansion skin and corner can be consumed; retain the published common IDs.
    for (const auto &[name, frames, image_id] :
         {std::tuple{"fence010.seb", 6, 64}, std::tuple{"fence011.seb", 6, 64},
          std::tuple{"fence012.seb", 6, 64}, std::tuple{"door00.seb", 2, 5}}) {
        const auto path = root / "common" / name;
        const auto definition = assets::parse_legacy_seb(read_bytes(path));
        if (definition.frame_count != frames)
            throw std::runtime_error("Boundary sprite frame count changed");
        for (const auto &layer : definition.layers)
            for (const auto &part : layer.parts)
                if (part.image_index != image_id)
                    throw std::runtime_error("Boundary common image binding changed");
        for (int frame = 0; frame < frames; ++frame)
            validate_frame(path, frame, Sprites::Binding::common);
    }
    for (const auto &name :
         {"menu.seb", "wnd_menuIcon.seb", "finger_r.seb", "number01.seb", "number05.seb",
          "number08.seb", "number12.seb", "icon_season.seb", "wnd_conner.seb", "arrow02.seb"}) {
        const auto path = root / "common" / name;
        const auto definition = assets::parse_legacy_seb(read_bytes(path));
        for (int frame = 0; frame < definition.frame_count; ++frame)
            validate_frame(path, frame, Sprites::Binding::common);
    }
    for (const auto &name : {"touch_arrow.seb", "buildCategoryBack.seb"}) {
        const auto path = root / "common2" / name;
        const auto definition = assets::parse_legacy_seb(read_bytes(path));
        for (int frame = 0; frame < definition.frame_count; ++frame)
            validate_frame(path, frame, Sprites::Binding::common2);
    }
    for (const auto &name : {"number05.seb", "number08.seb"}) {
        const auto path = root / "common" / name;
        const auto definition = assets::parse_legacy_seb(read_bytes(path));
        for (int frame = 0; frame < definition.frame_count; ++frame)
            validate_frame(path, frame, Sprites::Binding::steam_common);
    }
}
} // namespace ark::desktop
