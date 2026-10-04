#include "ark/simulation/rules/world_overlap.hpp"

#include <array>
#include <cmath>
#include <set>
#include <utility>

namespace ark::simulation::rules {
namespace {
bool eligible(const WorldOverlapActor &a) {
    return a.decision_area && a.state != 2 && a.state != 3 && a.state != 8 && a.state != 9 &&
           a.state != 14 && a.state != 15 && a.state != 16;
}
bool valid_position(WorldPosition p) {
    return std::isfinite(p.x) && std::isfinite(p.z) && std::abs(p.x) <= 1000000.0f &&
           std::abs(p.z) <= 1000000.0f;
}
bool adjacent(Position a, Position b) {
    const auto dx = static_cast<std::int64_t>(a.x) - b.x;
    const auto dy = static_cast<std::int64_t>(a.y) - b.y;
    return dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1;
}
bool overlaps(WorldPosition a, WorldPosition b, WorldOverlapRect r) {
    const float ax = a.x + static_cast<float>(r.x_offset);
    const float az = a.z + static_cast<float>(r.z_offset);
    const float bx = b.x + static_cast<float>(r.x_offset);
    const float bz = b.z + static_cast<float>(r.z_offset);
    return ax + r.width >= bx && ax <= bx + r.width && az >= bz - r.height && az - r.height <= bz;
}
void separate(WorldOverlapActor &a, const WorldOverlapActor &b, WorldOverlapRect r,
              WorldOverlapAttempt &attempt) {
    const float dx = std::abs(b.position.x - a.position.x);
    const float dz = std::abs(b.position.z - a.position.z);
    if (dx < 0.001f || dz < 0.001f) {
        a.position = a.previous_position;
        attempt.rolled_back = true;
        return;
    }
    // Source integer division happens before conversion to float, not width / 2.0f.
    const float half_width = static_cast<float>(r.width / 2);
    const float half_height = static_cast<float>(r.height / 2);
    if (dz <= dx) {
        const float shift = (dz * (2.0f * half_width)) / dx;
        a.position.z = b.position.z + (a.position.z < b.position.z ? -shift : shift);
        a.position.x =
            b.position.x + (a.position.x < b.position.x ? -(half_width * 2.0f) : half_width * 2.0f);
    } else {
        const float shift = (dx * (2.0f * half_height)) / dz;
        a.position.x = b.position.x + (a.position.x < b.position.x ? -shift : shift);
        a.position.z = b.position.z +
                       (a.position.z < b.position.z ? -(half_height * 2.0f) : half_height * 2.0f);
    }
}
} // namespace

std::optional<WorldOverlapRect> reference_world_overlap_rectangle(ActorKind kind, int shape) {
    // K(): ai[index][0] = (ah.x - width/2, ah.z + height/2).
    static constexpr std::array<WorldOverlapRect, 7> monsters{{{-30, 30, 30, 30},
                                                               {-30, 30, 30, 30},
                                                               {-30, 30, 30, 30},
                                                               {-40, 40, 40, 40},
                                                               {-60, 60, 60, 60},
                                                               {-30, 20, 30, 20},
                                                               {-240, 240, 240, 240}}};
    if (kind == ActorKind::human)
        return WorldOverlapRect{-30, 30, 30, 30};
    if (kind != ActorKind::monster || shape < 0 || shape >= static_cast<int>(monsters.size()))
        return std::nullopt;
    return monsters[static_cast<std::size_t>(shape)];
}

WorldOverlapResult prepare_world_overlap(const WorldOverlapInput &i) {
    if (i.attempt_limit == 0 || i.attempt_limit > 1000000 || i.humans.size() > 1000000 ||
        i.monsters.size() > 1000000)
        return {WorldOverlapError::invalid_input, std::nullopt};
    for (const auto *roster : {&i.humans, &i.monsters}) {
        std::set<std::uint64_t> ids;
        for (const auto &actor : *roster) {
            if (!ids.insert(actor.id.value).second)
                return {WorldOverlapError::duplicate_id, std::nullopt};
            if (actor.state < 0 || actor.state > 20 || !valid_position(actor.position) ||
                !valid_position(actor.previous_position))
                return {WorldOverlapError::invalid_input, std::nullopt};
        }
    }
    WorldOverlapCandidate c{i.humans, i.monsters, {}, 0};
    WorldOverlapError error{WorldOverlapError::none};
    const auto attempt = [&](WorldOverlapActor &first, ActorKind first_kind,
                             WorldOverlapActor &second, ActorKind second_kind) {
        if (c.attempts.size() >= i.attempt_limit) {
            error = WorldOverlapError::attempt_limit;
            return false;
        }
        std::optional<int> supplied;
        if (c.consumed_tickets < i.direction_tickets.size())
            supplied = i.direction_tickets[c.consumed_tickets];
        else if (i.draw) {
            try {
                supplied = i.draw(2);
            } catch (...) {
                error = WorldOverlapError::invalid_input;
                return false;
            }
        }
        if (!supplied) {
            error = WorldOverlapError::missing_ticket;
            return false;
        }
        const int ticket = *supplied;
        ++c.consumed_tickets;
        if (ticket < 0 || ticket >= 2) {
            error = WorldOverlapError::invalid_input;
            return false;
        }
        WorldOverlapActor &a = ticket == 0 ? first : second;
        WorldOverlapActor &b = ticket == 0 ? second : first;
        const auto ak = ticket == 0 ? first_kind : second_kind;
        const auto bk = ticket == 0 ? second_kind : first_kind;
        WorldOverlapAttempt trace{{ak, a.id}, {bk, b.id}, ticket, {}, adjacent(a.cell, b.cell),
                                  false,      false};
        if (trace.adjacent) {
            // The first lookup still must be valid even though the second overwrites br[0].
            const auto first_rect = reference_world_overlap_rectangle(ak, a.monster_shape);
            const auto second_rect = reference_world_overlap_rectangle(bk, b.monster_shape);
            if (!first_rect || !second_rect) {
                error = WorldOverlapError::invalid_input;
                return false;
            }
            trace.shared_rectangle = *second_rect;
            trace.overlapping = overlaps(a.position, b.position, *second_rect);
            if (trace.overlapping)
                separate(a, b, *second_rect, trace);
        }
        c.attempts.push_back(trace);
        return true;
    };
    for (std::size_t a = 0; a < c.humans.size(); ++a) {
        auto &human = c.humans[a];
        if (human.state != 1 || human.cell.y <= i.boundary_y)
            continue;
        if (!human.inside_town)
            for (std::size_t b = a + 1; b < c.humans.size(); ++b) {
                auto &other = c.humans[b];
                if (other.state == 1 && !other.inside_town && other.cell.y > i.boundary_y &&
                    !attempt(human, ActorKind::human, other, ActorKind::human))
                    return {error, std::nullopt};
            }
        for (auto &monster : c.monsters)
            if (eligible(monster) && !attempt(human, ActorKind::human, monster, ActorKind::monster))
                return {error, std::nullopt};
    }
    for (std::size_t a = 0; a < c.monsters.size(); ++a) {
        auto &monster = c.monsters[a];
        if (!eligible(monster))
            continue;
        for (std::size_t b = 0; b < c.monsters.size(); ++b)
            if (a != b && eligible(c.monsters[b]) &&
                !attempt(monster, ActorKind::monster, c.monsters[b], ActorKind::monster))
                return {error, std::nullopt};
    }
    return {WorldOverlapError::none, std::move(c)};
}
} // namespace ark::simulation::rules
