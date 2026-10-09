#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace dungeon_village_prototype {
enum class StartupTitleRootMode { menu, slots };
enum class StartupTitleExternalPage { records, configure };
enum class StartupTitleConfirmationReason { restart, hide };
// 目录由应用提供；纯控制器不读文件、不由文件存在性猜可见性。
struct StartupTitleCatalogStamp {
    std::uint64_t revision{};
    std::array<std::uint8_t,32> digest{};
};
struct StartupTitleMenuContext {
    int slot{}; // 唯一权威来自应用draft.slot，State不重复保存。
    std::array<std::array<bool,2>,2> present{}; // [slot][中断0/手动1]，目录日期!=-1。
    std::optional<StartupTitleCatalogStamp> catalog;
};
struct StartupTitleSaveMenu {
    std::uint64_t id{}, parent{};
    int slot{}, selection{}, frame{}, result{-1};
    bool returned{};
    std::optional<StartupTitleCatalogStamp> catalog;
};
struct StartupTitleConfirmation {
    std::uint64_t id{}, parent{};
    StartupTitleConfirmationReason reason{StartupTitleConfirmationReason::restart};
    int selection{1}, result{-1}; // 原默认否；返回结果-1取消、0是、1否。
    bool returned{};
};
struct StartupTitleExternal {
    std::uint64_t id{}, parent{};
    StartupTitleExternalPage kind{StartupTitleExternalPage::records};
    bool returned{}, completed{}; // configure完成才请求实际start；records不接受completed。
};
// 所有字段都影响恢复/输入资格，均须持久；没有任意深度栈、容器地址或第二个随机流。
struct StartupTitleMenuState {
    StartupTitleRootMode mode{StartupTitleRootMode::menu};
    int selection{}, row{1};
    std::uint64_t root_id{1}, next_id{2};
    std::optional<StartupTitleSaveMenu> save_menu;
    std::optional<StartupTitleConfirmation> confirmation;
    std::optional<StartupTitleExternal> external;
};
enum class StartupTitleIntentKind { open_records, open_configuration, load_record, hide_record, start_game };
struct StartupTitleMenuIntent {
    StartupTitleIntentKind kind;
    int slot{}, row{1};
    std::uint64_t page_id{};
    std::optional<StartupTitleCatalogStamp> catalog;
};
struct StartupTitleMenuKeys {
    bool left{}, right{}, up{}, down{}, confirm{}, back{};
};
enum class StartupTitleMenuRequestKind {
    keys, touch_enter, touch_up, consume_return, return_external, advance_frame_menu
};
struct StartupTitleMenuRequest {
    StartupTitleMenuRequestKind kind{StartupTitleMenuRequestKind::keys};
    StartupTitleMenuKeys keys;
    int component{}, value{}; // 具名touch请求：原组件0/3/9及实际行号；不是packed高位。
    bool completed{}; // return_external专用；配置实际开始，非覆盖询问。
};
struct StartupTitleMenuCandidate {
    StartupTitleMenuState state;
    int slot{}; // Owner成功后才与State联合安装到唯一draft.slot。
    std::optional<StartupTitleMenuIntent> intent; // 一次性输出，不作为待重复消费队列存档。
};
struct StartupTitleMenuResult {
    std::string error;
    std::optional<StartupTitleMenuCandidate> candidate;
};
std::string validate_startup_title_menu(const StartupTitleMenuState &state);
// 返回0表示坏State；returned子页不再是栈顶，但仍保留给父页显式消费的结果。
std::uint64_t startup_title_menu_top_id(const StartupTitleMenuState &state);
// 输入已经过框架准入。keys不自动推进计数/背景；advance_frame_menu只表达一次已核
// FrameMenu内部+1并夹3，不声称等于完整SubForm.Update或一个逻辑tick。
// 结果消费优先级用独立请求表达；有待返回载荷时，父页keys必须拒绝而不能绕过消费。
StartupTitleMenuResult prepare_startup_title_menu(const StartupTitleMenuState &state,
    const StartupTitleMenuContext &context, std::uint64_t expected_top_id,
    const StartupTitleMenuRequest &request);
} // namespace dungeon_village_prototype
