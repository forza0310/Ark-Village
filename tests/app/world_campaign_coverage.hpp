#pragma once
#include "ark/app/session/world_session.hpp"
#include <filesystem>
#include <iosfwd>
#include <map>
#include <set>

namespace ark::test {
// Per-process evidence inventory. It deliberately does not reconstruct a command history
// from a loaded save, or turn an inventory decrease into a verified item effect.
class CampaignCoverage {
  public:
    void observe(const app::WorldState &before, const app::WorldState &after);
    void write(std::ostream &out, const app::WorldState &state) const;
    void save(const std::filesystem::path &directory, const app::WorldState &state) const;

  private:
    using Key = std::pair<std::string, int>;
    void mark(const char *kind, int id, const char *event);
    std::map<Key, std::set<std::string>> observations_;
    std::string segment_;
    std::uint64_t first_step_{}, last_step_{};
};
void campaign_coverage_contract();
} // namespace ark::test
