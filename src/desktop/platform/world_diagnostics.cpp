#include "ark/app/session/world_session.hpp"
#include "crash_report.hpp"
#include <chrono>
#include <sstream>

namespace ark::desktop {
void WorldDiagnostics::observe(const app::WorldFrame &frame) noexcept {
    auto *reporter = CrashReporter::current();
    if (!reporter || !frame.state)
        return;
    try {
        using Clock = std::chrono::steady_clock;
        if (generation_ != frame.generation) {
            generation_ = frame.generation;
            next_ = {};
            reported_failure_ = false;
        }
        const bool failed = frame.failed || !frame.system_error.empty();
        if (!failed)
            reported_failure_ = false;
        const bool failure = failed && !reported_failure_;
        if (!failure && Clock::now() < next_)
            return;
        next_ = Clock::now() + std::chrono::seconds(1);
        const auto &s = *frame.state;
        std::ostringstream out;
        out << "generation=" << frame.generation << " revision=" << frame.revision
            << " world_steps=" << s.simulation_steps << " random_draws=" << s.scene.random.draws()
            << " date=" << s.scene.calendar.year << '/' << s.scene.calendar.month << '/'
            << s.scene.calendar.subperiod << " rank=" << s.rank
            << " paused=" << s.scene.framework_paused
            << " active_task=" << s.active_task.value_or(0) << '\n';
        for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
            if (p->lifecycle != 4) {
                out << "page=" << p->id << " raw=" << p->legacy_page << '\n';
                break;
            }
        const auto begin =
            frame.command_results.size() > 64 ? frame.command_results.size() - 64 : 0;
        for (std::size_t i = begin; i < frame.command_results.size(); ++i) {
            const auto &c = frame.command_results[i];
            out << "command serial=" << c.serial << " kind=" << int(c.kind) << " page=" << c.page
                << " outcome=" << int(c.outcome) << " error=" << int(c.runtime_error)
                << " task_denial=" << int(c.denial) << " build_denial=" << int(c.build_denial)
                << '\n';
        }
        if (frame.failed)
            out << "runtime_failure=" << frame.error << '\n';
        if (!frame.system_error.empty())
            out << "system_failure=" << frame.system_error << '\n';
        reporter->context(out.str());
        if (failure) {
            reported_failure_ = true;
            reporter->report(frame.failed ? "runtime-rejection" : "system-rejection",
                             frame.error + frame.system_error +
                                 "; stack is desktop observation site, use stage/id and context "
                                 "for the worker failure");
        }
    } catch (...) {
    } // Diagnostic allocation/I/O must not stop rendering or alter the Owner.
}
} // namespace ark::desktop
