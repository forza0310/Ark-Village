// Publish deterministic loaded-map TSV without a JSON library or any APK runtime dependency.
// The published files are static reconstruction evidence; raw IDs and maintained IDs are separate.
#include "dungeon_village_prototype/startup.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace dv = dungeon_village_prototype;
int main(int argc, char **argv) {
    try {
        if (argc < 2 || argc > 3)
            throw std::invalid_argument("参数：cells|instances [输出TSV]");
        const std::string mode = argv[1];
        if (mode != "cells" && mode != "instances")
            throw std::invalid_argument("快照类型必须是cells或instances");
        std::ofstream file;
        if (argc == 3) {
            file.open(argv[2], std::ios::binary | std::ios::trunc);
            if (!file)
                throw std::runtime_error("快照输出无法打开");
        }
        std::ostream &output = argc == 3 ? file : std::cout;
        const auto map = dv::reconstruct_startup_map(dv::startup_evidence());
        if (mode == "cells") {
            output << "x\ty\tdefinition_id\tlegacy_state\troute_category\tdisplay_"
                      "id\tvariant\troad_mask"
                      "\tboundary_fragment\texternal_direction\troad_quad\tedge_road_pair"
                      "\tlegacy_instance_id\tmaintained_instance_id\n";
            for (int y = 0; y < map.height; ++y) {
                for (int x = 0; x < map.width; ++x) {
                    const auto &c = map.cells.at(static_cast<std::size_t>(y * map.width + x));
                    output << x << '\t' << y << '\t' << c.definition_id << '\t' << c.legacy_state
                           << '\t' << static_cast<int>(c.category) << '\t' << c.display_id << '\t'
                           << c.variant << '\t' << c.road_mask << '\t' << c.boundary_fragment
                           << '\t' << c.external_direction << '\t' << c.road_quad << '\t'
                           << c.edge_road_pair << '\t' << c.legacy_instance_id.value_or(-1) << '\t'
                           << (c.legacy_instance_id ? *c.legacy_instance_id + 1 : 0) << '\n';
                }
            }
        } else {
            output << "vector_index\tlegacy_instance_id\tmaintained_instance_id\tdefinition_"
                      "id\tx\ty\n";
            for (std::size_t i = 0; i < map.instances.size(); ++i) {
                const auto &v = map.instances[i];
                output << i << '\t' << v.legacy_id << '\t' << v.legacy_id + 1 << '\t'
                       << v.definition_id << '\t' << v.anchor.x << '\t' << v.anchor.y << '\n';
            }
        }
        output.flush();
        if (!output)
            throw std::runtime_error("快照输出失败");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
