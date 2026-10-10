#include "ark/simulation/application/startup_application.hpp"
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace ark::simulation {
namespace {
using Page = StartupApplicationPage;
using Request = StartupTitleMenuRequest;
using Kind = StartupTitleMenuRequestKind;
StartupTitleCatalogStamp stamp(const StartupApplicationStorageSnapshot &storage) {
    StartupTitleCatalogStamp result;
    result.revision = storage.records.revision;
    // 缺失系统同样有规范默认记录摘要，由storage读取器提供，不能用全零冒充。
    if (storage.digest.size() != 64) throw std::runtime_error("系统目录摘要缺失");
    const auto digit = [](char c) -> unsigned {
        if (c >= '0' && c <= '9') return static_cast<unsigned>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<unsigned>(c - 'a' + 10);
        throw std::runtime_error("系统目录摘要不是规范小写hex");
    };
    for (std::size_t i = 0; i < result.digest.size(); ++i)
        result.digest[i] = static_cast<std::uint8_t>(digit(storage.digest[2*i])*16 + digit(storage.digest[2*i+1]));
    return result;
}
Request keys(StartupTitleMenuKeys value) { Request r; r.keys = value; return r; }
}
StartupTitleMenuContext StartupApplication::title_menu_context() const {
    StartupTitleMenuContext context;
    context.slot = draft_.slot;
    context.catalog = stamp(storage_);
    for (std::size_t slot = 0; slot < 2; ++slot)
        for (std::size_t kind = 0; kind < 2; ++kind)
            context.present[slot][kind] = storage_.records.save_directory[slot][kind].packed_date != -1;
    return context;
}
void StartupApplication::sync_title_page() {
    if (page_ == Page::world) return;
    if (title_menu_.external && !title_menu_.external->returned)
        page_ = title_menu_.external->kind == StartupTitleExternalPage::records ? Page::records : Page::configure;
    else if (title_menu_.confirmation && !title_menu_.confirmation->returned)
        page_ = Page::overwrite;
    else page_ = Page::title;
}
std::string StartupApplication::apply_title_request(std::uint64_t id, const Request &request) {
    if (!error_.empty()) return error_;
    if (page_ == Page::world || clear_) return "活动世界不能消费标题输入";
    try {
        auto prepared = prepare_startup_title_menu(title_menu_, title_menu_context(), id, request);
        if (!prepared.candidate) return prepared.error.empty() ? "标题控制器缺候选" : prepared.error;
        auto candidate = *this;
        candidate.title_menu_ = std::move(prepared.candidate->state);
        candidate.draft_.slot = prepared.candidate->slot;
        std::string error;
        if (prepared.candidate->intent) {
            const auto &intent = *prepared.candidate->intent;
            switch (intent.kind) {
            case StartupTitleIntentKind::open_records:
                error = candidate.open_records_candidate(); break;
            case StartupTitleIntentKind::open_configuration:
                candidate.page_ = Page::configure; break;
            case StartupTitleIntentKind::load_record:
                candidate.page_ = Page::title;
                error = candidate.load_world(intent.slot, static_cast<StartupSaveKind>(intent.row)); break;
            case StartupTitleIntentKind::hide_record: {
                auto committed = hide_startup_application_slot(candidate.paths_.root, candidate.storage_,
                    candidate.storage_.records, intent.slot, static_cast<StartupSaveKind>(intent.row));
                if (!committed.snapshot) error = committed.error;
                else {
                    // 下游没有可能失败的业务；真正系统成功后仅移动已分配候选。
                    auto installed = std::move(*committed.snapshot);
                    candidate.storage_ = std::move(installed);
                    candidate.cleanup_pending_ = committed.cleanup_pending;
                }
                break;
            }
            case StartupTitleIntentKind::start_game:
                error = candidate.start_game_candidate(); break;
            }
        }
        if (!error.empty()) return error;
        if (candidate.page_ != Page::world) {
            candidate.sync_title_page();
            if (candidate.page_ != Page::records) candidate.decorations_.clear();
        }
        static_assert(std::is_nothrow_move_assignable_v<StartupApplication>);
        *this = std::move(candidate);
        return {};
    } catch (const std::exception &e) { return e.what(); }
}
std::string StartupApplication::request_new_game(int slot) {
    if (!error_.empty()) return error_;
    if (page_ != Page::title || slot < 0 || slot > 1 || title_menu_.save_menu ||
        title_menu_.confirmation || title_menu_.external) return "新局入口或栏位非法";
    try {
        auto candidate = *this;
        candidate.title_menu_.mode = StartupTitleRootMode::slots;
        candidate.title_menu_.row = 1;
        candidate.draft_.slot = slot;
        auto error = candidate.apply_title_request(candidate.title_page_id(), keys({false,false,false,false,true,false}));
        if (!error.empty()) return error;
        if (candidate.title_menu_.save_menu) {
            error = candidate.apply_title_request(candidate.title_page_id(), keys({false,false,false,true,false,false}));
            if (!error.empty()) return error;
            error = candidate.apply_title_request(candidate.title_page_id(), keys({false,false,false,false,true,false}));
            if (!error.empty()) return error;
        }
        *this = std::move(candidate); return {};
    } catch (const std::exception &e) { return e.what(); }
}
std::string StartupApplication::answer_overwrite(bool yes) {
    if (page_ != Page::overwrite || !title_menu_.confirmation ||
        title_menu_.confirmation->reason != StartupTitleConfirmationReason::restart) return "没有覆盖询问";
    try {
        auto candidate = *this;
        Request choose; choose.kind = Kind::touch_enter; choose.component = 3; choose.value = yes ? 0 : 1;
        auto error = candidate.apply_title_request(candidate.title_page_id(), choose);
        if (!error.empty()) return error;
        error = candidate.apply_title_request(candidate.title_page_id(), keys({false,false,false,false,true,false}));
        if (!error.empty()) return error;
        Request consume; consume.kind = Kind::consume_return;
        error = candidate.apply_title_request(candidate.title_page_id(), consume);
        if (!error.empty()) return error;
        if (yes) {
            error = candidate.apply_title_request(candidate.title_page_id(), consume);
            if (!error.empty()) return error;
        }
        *this = std::move(candidate); return {};
    } catch (const std::exception &e) { return e.what(); }
}
std::string StartupApplication::cancel_configuration() {
    if (page_ != Page::configure || !title_menu_.external) return "当前不是配置页";
    auto candidate = *this;
    Request done; done.kind = Kind::return_external;
    auto error = candidate.apply_title_request(candidate.title_page_id(), done);
    if (!error.empty()) return error;
    done.kind = Kind::consume_return;
    error = candidate.apply_title_request(candidate.title_page_id(), done);
    if (!error.empty()) return error;
    *this = std::move(candidate); return {};
}
std::string StartupApplication::start_game() {
    if (page_ != Page::configure || !title_menu_.external) return "新局配置页面非法";
    try {
        auto candidate = *this;
        Request done; done.kind = Kind::return_external; done.completed = true;
        auto error = candidate.apply_title_request(candidate.title_page_id(), done);
        if (!error.empty()) return error;
        done = {}; done.kind = Kind::consume_return;
        error = candidate.apply_title_request(candidate.title_page_id(), done);
        if (!error.empty()) return error;
        *this = std::move(candidate); return {};
    } catch (const std::exception &e) { return e.what(); }
}
std::string StartupApplication::open_records() {
    if (page_ != Page::title || title_menu_.save_menu || title_menu_.confirmation || title_menu_.external)
        return "纪录入口页面非法";
    auto candidate = *this;
    candidate.title_menu_.mode = StartupTitleRootMode::menu;
    candidate.title_menu_.selection = 1;
    auto error = candidate.apply_title_request(candidate.title_page_id(), keys({false,false,false,false,true,false}));
    if (!error.empty()) return error;
    *this = std::move(candidate); return {};
}
std::string StartupApplication::return_to_title() {
    if (!error_.empty()) return error_;
    if (clear_) return "计分页仍在运行";
    try {
        auto candidate = *this;
        if (page_ == Page::world) {
            if (!world_ || title_menu_.next_id > std::numeric_limits<std::uint64_t>::max()-1)
                return "标题重入身份耗尽";
            candidate.random_ = world_->state().scene.random;
            const int retained = title_.f132f;
            candidate.title_ = {};
            if (mode_ == StartupApplicationMode::title_presentation) candidate.title_.f132f = retained;
            candidate.title_menu_ = {};
            candidate.title_menu_.root_id = title_menu_.next_id;
            candidate.title_menu_.next_id = title_menu_.next_id+1;
            candidate.draft_.slot = storage_.records.last_slot;
            candidate.audio_requests_.push_back({StartupAudioOperation::replace_bgm,0});
            candidate.page_ = Page::title;
        } else if (title_menu_.external) {
            Request request; request.kind = Kind::return_external;
            auto error = candidate.apply_title_request(candidate.title_page_id(), request);
            if (!error.empty()) return error;
            request.kind = Kind::consume_return;
            error = candidate.apply_title_request(candidate.title_page_id(), request);
            if (!error.empty()) return error;
        } else if (title_menu_.save_menu || title_menu_.confirmation) return "请先沿实际菜单取消返回";
        candidate.decorations_.clear();
        *this = std::move(candidate); return {};
    } catch (const std::exception &e) { return e.what(); }
}
std::string StartupApplication::refresh_title_storage() {
    if (!error_.empty()) return error_;
    if (page_ == Page::world || clear_) return "只能在标题重新读取目录";
    if (title_menu_.next_id > std::numeric_limits<std::uint64_t>::max()-1) return "标题重开身份耗尽";
    try {
        auto loaded = load_startup_application_storage(paths_.root);
        if (!loaded.snapshot) return loaded.error;
        auto candidate = *this;
        candidate.storage_ = std::move(*loaded.snapshot);
        if (candidate.world_) {
            // 只重绑跨局纪录镜像，保留旧世界的资金、实体、随机与历史，不加载其它玩家世界。
            candidate.world_->state_.cash_peak = candidate.storage_.records.cash_peak;
            candidate.world_->state_.cash_peak_village = candidate.storage_.records.cash_village;
        }
        candidate.title_menu_ = {};
        candidate.title_menu_.root_id = title_menu_.next_id;
        candidate.title_menu_.next_id = title_menu_.next_id+1;
        candidate.title_menu_.mode = StartupTitleRootMode::slots;
        candidate.page_ = Page::title;
        candidate.decorations_.clear();
        candidate.cleanup_pending_ = cleanup_pending_ || loaded.cleanup_pending;
        *this = std::move(candidate); return {};
    } catch (const std::exception &e) { return e.what(); }
}
} // namespace ark::simulation
