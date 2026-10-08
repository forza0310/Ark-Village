// 中文研究CLI：只通过应用Owner操作，不是玩家皮肤；不读取原APK／Steam档。
#include "dungeon_village_prototype/startup_application.hpp"

#include <charconv>
#include <iostream>
#include <string_view>

namespace app = dungeon_village_prototype;
namespace {
std::string_view trimmed(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}
template <class Integer> bool integer(std::string_view text, Integer &value) {
    text = trimmed(text);
    if (text.empty()) return false;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
void usage() {
    std::cout << "用法：startup_application_cli --system 路径 --slot0 路径 --slot1 路径"
                 " [--seed N] [--title-presentation]\n"
                 "必须显式指定三个维护文件路径；可选seed为无符号整数，默认0仅为研究种子。\n"
                 "本入口不操作原版档，退出和输入结束均不自动保存。\n";
}
void help() {
    std::cout << "命令：\n"
                 "  help / view                 帮助／只读当前状态\n"
                 "  title                       返回标题\n"
                 "  new 0|1                     选择新局栏位\n"
                 "  overwrite yes|no            回答覆盖询问\n"
                 "  village 后续全串 / name 后续全串  编辑村名／主角名\n"
                 "  sex 0|1 / cancel / start    性别／取消配置／开始\n"
                 "  records / next / previous   纪录入口／翻页\n"
                 "  step [N]                    推进N次页面或世界更新，默认1，上限10000\n"
                 "  confirm                     确认栈顶页，计分页合并为一次更新脉冲\n"
                 "  save / load 0|1             显式保存当前世界／标题读入指定栏\n"
                 "  quit                        退出，不自动保存\n"
                 "声音请求在本CLI中领取一次并打印编号，不播放音频。\n";
}
const char *page_name(app::StartupApplicationPage page) {
    switch (page) {
    case app::StartupApplicationPage::title: return "标题";
    case app::StartupApplicationPage::overwrite: return "覆盖询问";
    case app::StartupApplicationPage::configure: return "新局配置";
    case app::StartupApplicationPage::records: return "纪录";
    case app::StartupApplicationPage::world: return "世界";
    }
    return "非法页";
}
void sounds(app::StartupApplication &application) {
    const auto requests = application.take_sound_requests();
    if (!requests.empty()) {
        std::cout << "声音请求（已领取，仅打印）：";
        for (const int id : requests) std::cout << ' ' << id;
        std::cout << '\n';
    }
}
void view(const app::StartupApplication &application) {
    const auto &draft = application.draft();
    std::cout << "页面：" << page_name(application.page()) << "；栏位：" << draft.slot
              << "\n配置：村名=" << draft.village << "；定义0姓名=" << draft.main_character.name
              << "；性别=" << draft.main_character.sex
              << "；自定义姓名=" << (draft.main_character.custom_name ? "是" : "否") << '\n';
    const auto &records = application.records();
    std::cout << "系统纪录：最高通关=" << records.high_score << " P（" << records.score_village
              << "）；奖杯=" << records.trophy << "；最高资金=" << records.cash_peak << " G（"
              << records.cash_village << "）\n";
    if (application.page() == app::StartupApplicationPage::records) {
        const auto record = application.record_view();
        std::cout << "纪录页 " << record.page + 1 << "/2：" << record.label << '=' << record.value
                  << ' ' << record.unit << "；村名=" << record.village << '\n';
    }
    if (const auto *world = application.world()) {
        const auto &s = world->state();
        const auto &calendar = s.scene.calendar;
        std::cout << "世界：" << s.scripts.village_name << "；日期=" << static_cast<std::int64_t>(calendar.year) + 1
                  << "年" << calendar.month + 1 << "月；原分段=" << calendar.subperiod
                  << "；单位=" << calendar.units << "；资金=" << s.scene.world.world.ai.accounting.funds()
                  << " G；人气=" << s.popularity << "；随机抽数=" << s.scene.random.draws()
                  << "；设施=" << s.scene.world.facility_order.size()
                  << "；人物=" << s.scene.world.world.ai.human_order.size() << '\n';
        for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
            if (p->lifecycle != 4) {
                std::cout << "栈顶：id=" << p->id << "；原页=" << p->legacy_page
                          << "；生命周期=" << p->lifecycle << '\n';
                break;
            }
    } else {
        std::cout << "当前无世界。\n";
    }
    if (application.clear_page()) {
        const auto &p = *application.clear_page();
        std::cout << "计分页：阶段=" << p.stage << "；计数=" << p.counter << "；行=" << p.row
                  << "；累计=" << p.sum << "；捕获旧最高=" << p.captured_high_score
                  << "；完成=" << (p.finished ? "是" : "否") << '\n';
        if (application.clear_rows()) {
            constexpr std::array<const char *, 6> labels{{"人气", "完成任务", "开放设施种类",
                                                        "全职业等级", "在籍住宅", "人物努力"}};
            for (std::size_t i = 0; i < 6; ++i) {
                const auto &row = (*application.clear_rows())[i];
                std::cout << "  " << i << ' ' << labels[i] << "：数量=" << row.count
                          << "；分数=" << row.score << '\n';
            }
        }
    }
    if (!application.decorations().empty()) {
        std::cout << "标题装饰人物定义：";
        for (const int id : application.decorations()) std::cout << ' ' << id;
        std::cout << '\n';
    }
}
} // namespace

int main(int argc, char **argv) {
    app::StartupApplicationPaths paths;
    std::array<bool, 3> have_paths{};
    std::uint64_t seed{};
    bool seed_set{}, presentation{};
    if (argc == 2 && std::string_view(argv[1]) == "--help") { usage(); return 0; }
    for (int i = 1; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if (option == "--title-presentation") {
            if (presentation) { std::cerr << "拒绝：重复的表现模式参数。\n"; return 2; }
            presentation = true; continue;
        }
        const int slot = option == "--system" ? 0 : option == "--slot0" ? 1 : option == "--slot1" ? 2 : -1;
        if (slot >= 0) {
            if (have_paths[slot] || i + 1 >= argc || std::string_view(argv[i + 1]).empty() ||
                std::string_view(argv[i + 1]).substr(0, 2) == "--") {
                std::cerr << "拒绝：路径参数重复或缺少路径。\n"; return 2;
            }
            have_paths[slot] = true;
            const std::filesystem::path value(argv[++i]);
            if (slot == 0) paths.system = value; else paths.worlds[slot - 1] = value;
        } else if (option == "--seed") {
            if (seed_set || i + 1 >= argc || !integer(std::string_view(argv[++i]), seed)) {
                std::cerr << "拒绝：seed必须是唯一的无符号整数。\n"; return 2;
            }
            seed_set = true;
        } else {
            std::cerr << "拒绝：未知启动参数。\n"; usage(); return 2;
        }
    }
    if (!have_paths[0] || !have_paths[1] || !have_paths[2]) { usage(); return 2; }
    try {
        app::StartupApplication application(std::move(paths), app::ref::WorldRandomStream::from_java_seed(seed),
            presentation ? app::StartupApplicationMode::title_presentation : app::StartupApplicationMode::logic);
        if (!application.error().empty()) {
            std::cerr << "启动拒绝：" << application.error() << '\n'; return 1;
        }
        help(); view(application); sounds(application);
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto first = line.find_first_not_of(" \t");
            if (first == std::string::npos) continue;
            const auto end = line.find_first_of(" \t", first);
            const std::string command = line.substr(first, end == std::string::npos ? end : end - first);
            // 仅消耗命令后的一个分隔符；名称剩余全串原样交给Owner校验。
            const std::string text = end == std::string::npos ? std::string{} : line.substr(end + 1);
            std::string error;
            bool quit{}, shown{};
            int number{};
            if (command == "village") error = application.edit_village(text);
            else if (command == "name") error = application.edit_main_name(text);
            else if (command == "new" || command == "load" || command == "sex") {
                if (!integer(std::string_view(text), number) || number < 0 || number > 1)
                    error = "参数必须是0或1";
                else if (command == "new") error = application.request_new_game(number);
                else if (command == "load") error = application.load_world(number);
                else error = application.edit_sex(number);
            } else if (command == "overwrite") {
                if (trimmed(text) == "yes") error = application.answer_overwrite(true);
                else if (trimmed(text) == "no") error = application.answer_overwrite(false);
                else error = "覆盖答案必须是yes或no";
            } else if (command == "step") {
                number = 1;
                if ((!trimmed(text).empty() && !integer(std::string_view(text), number)) ||
                    number < 1 || number > 10000) error = "推进次数必须为1..10000";
                else {
                    int completed{};
                    for (; completed < number; ++completed) {
                        error = application.update();
                        sounds(application); // 每次实际更新后退休输出，避免长批积累。
                        if (!error.empty()) break;
                    }
                    std::cout << "已完成更新：" << completed << '/' << number << '\n';
                }
            } else if (!trimmed(text).empty()) error = "该命令不接受参数";
            else if (command == "help") { help(); shown = true; }
            else if (command == "view") { view(application); shown = true; }
            else if (command == "title") error = application.return_to_title();
            else if (command == "cancel") error = application.cancel_configuration();
            else if (command == "start") error = application.start_game();
            else if (command == "records") error = application.open_records();
            else if (command == "next") error = application.turn_record_page(1);
            else if (command == "previous") error = application.turn_record_page(-1);
            else if (command == "confirm") error = application.update(true);
            else if (command == "save") error = application.save_world();
            else if (command == "quit") quit = true;
            else error = "未知命令；输入help查看合法命令";
            sounds(application);
            if (!error.empty()) std::cout << "拒绝：" << error << '\n';
            else if (quit) { std::cout << "已退出；未执行自动保存。\n"; return 0; }
            else if (!shown) { std::cout << "成功：" << command << '\n'; view(application); }
        }
        std::cout << "输入结束；未执行自动保存。\n";
    } catch (const std::exception &error) {
        std::cerr << "研究入口失败：" << error.what() << '\n'; return 1;
    }
}
