#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <utility>

struct ProfileMetric {
    long long calls{};
    double ms{};
};
inline ProfileMetric profile_metrics[20];
struct ProfileScope {
    int id;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    ~ProfileScope() {
        ++profile_metrics[id].calls;
        profile_metrics[id].ms +=
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                .count();
    }
};
template <class T> T profile_copy(const T &source) {
    ProfileScope timer{0};
    return T(source);
}
template <class R, class... A> void profile_wrap(std::function<R(A...)> &function, int id) {
    auto original = function;
    function = [original, id](A... args) -> R {
        ProfileScope timer{id};
        return original(std::forward<A>(args)...);
    };
}
inline void profile_report(int frame = 0) {
    const char *names[] = {
        "owner_copy",        "scene_read",   "scene_write",       "scripts_read", "scripts_write",
        "routes_read",       "routes_write", "decision_input",    "scene_other",  "before_common",
        "normal_conditions", "domain",       "adapter_construct", "calendar",
        "facility_read", "facility_write", "nonactor_read", "nonactor_write",
        "actor_decision", "actor_control"};
    for (int i = 0; i < 20; ++i)
        std::cerr << "frame=" << frame << ' ' << names[i] << " calls=" << profile_metrics[i].calls
                  << " ms=" << profile_metrics[i].ms << '\n';
}
