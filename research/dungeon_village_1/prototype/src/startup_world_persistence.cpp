#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_tools/archive.hpp"
#include "startup_world_codec.hpp"
#include "startup_world_file_io.hpp"
#include "startup_world_restore_validation.hpp"
#include "startup_persistence_bytes.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace dungeon_village_prototype {
// 唯一私有安装点；只有本文件完成边界、目录、预算和跨域校验后调用。
struct StartupWorldPersistenceAccess {
    static StartupWorldRuntimeSession
    make(StartupWorldRuntimeState state,
         std::vector<std::shared_ptr<const StartupWorldRuntimeState>> history) {
        StartupWorldRuntimeSession session;
        session.state_ = std::move(state);
        session.checkpoints_ = std::move(history);
        return session;
    }
};
namespace {
using Bytes = std::vector<std::uint8_t>;
namespace detail = persistence_detail;
constexpr std::size_t file_budget = 128U * 1024U * 1024U;
constexpr std::size_t controller_budget = 1024U * 1024U;
// 语义4同步邻接变化后的到达价格；旧现金/随机历史不能靠加载时重算缓存迁移。
constexpr std::uint32_t format_version = 1, state_semantics = 4;
constexpr char magic[] = "AVRSAVE1";
void need(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
struct Writer {
    Bytes bytes;
    void raw(const void *data, std::size_t size) {
        need(size <= file_budget - bytes.size(), "存档编码超出总预算");
        if (size == 0)
            return;
        const auto *p = static_cast<const std::uint8_t *>(data);
        bytes.insert(bytes.end(), p, p + size);
    }
    void u32(std::uint32_t n) {
        for (int i = 0; i < 4; ++i) {
            const auto b = static_cast<std::uint8_t>(n >> (8 * i));
            raw(&b, 1);
        }
    }
    void u64(std::uint64_t n) {
        for (int i = 0; i < 8; ++i) {
            const auto b = static_cast<std::uint8_t>(n >> (8 * i));
            raw(&b, 1);
        }
    }
    void text(const std::string &s) {
        need(s.size() <= 4096, "存档文本过长");
        u32(static_cast<std::uint32_t>(s.size()));
        raw(s.data(), s.size());
    }
    void blob(const Bytes &b) {
        u64(b.size());
        raw(b.data(), b.size());
    }
};
struct Reader {
    const Bytes &bytes;
    std::size_t at{};
    void require(std::size_t n) const { need(n <= bytes.size() - at, "存档截断"); }
    std::uint32_t u32() {
        require(4);
        std::uint32_t n{};
        for (int i = 0; i < 4; ++i)
            n |= static_cast<std::uint32_t>(bytes[at++]) << (8 * i);
        return n;
    }
    std::uint64_t u64() {
        require(8);
        std::uint64_t n{};
        for (int i = 0; i < 8; ++i)
            n |= static_cast<std::uint64_t>(bytes[at++]) << (8 * i);
        return n;
    }
    Bytes raw(std::uint64_t n) {
        need(n <= file_budget, "分区长度超预算");
        require(static_cast<std::size_t>(n));
        const auto first = bytes.begin() + static_cast<std::ptrdiff_t>(at);
        at += static_cast<std::size_t>(n);
        return {first, bytes.begin() + static_cast<std::ptrdiff_t>(at)};
    }
    std::string text() {
        const auto n = u32();
        need(n <= 4096, "存档文本过长");
        const auto b = raw(n);
        return {b.begin(), b.end()};
    }
    Bytes blob() { return raw(u64()); }
    void end() const { need(at == bytes.size(), "存档分区有尾字节"); }
};
struct Section {
    std::uint32_t id{}, version{1}, required{1};
    Bytes bytes;
};
void valid_metadata(const StartupWorldSaveMetadata &m) {
    need(m.purpose == StartupWorldSavePurpose::normal ||
             m.purpose == StartupWorldSavePurpose::replay,
         "未知存档用途");
    need(m.producer_revision.size() <= 256 && m.controller_id.size() <= 256,
         "来源或控制器身份过长");
    need(m.controller_state.size() <= controller_budget, "控制器载荷超预算");
    if (m.purpose == StartupWorldSavePurpose::normal)
        need(m.controller_id.empty() && m.controller_state.empty() && m.next_frame == 0,
             "正常档不得夹带回放控制器");
    else
        need(!m.controller_id.empty() && !m.controller_state.empty(), "回放档缺少控制器身份或状态");
    need(m.extensions.size() <= 60, "可选分区过多");
    std::set<std::uint32_t> ids;
    for (const auto &e : m.extensions)
        need(e.id >= 1024 && e.version > 0 && ids.insert(e.id).second, "可选分区身份重复或非法");
}
void valid_capture(const StartupWorldRuntimeState &s, StartupWorldSavePurpose purpose) {
    need(s.rules == &startup_world_rules(), "存档只支持已登记的固定规则目录");
    need(!s.scripts.executing_page, "不能保存正在执行的页面事务");
    need(s.sound_requests.empty(), "保存边界要求先消费声音输出");
    if (purpose == StartupWorldSavePurpose::normal) {
        need(s.scene.scene_state == 0 && s.build_mode == 0 && !s.build_definition &&
                 !s.build_anchor && !s.build_moving_facility,
             "正常存档只支持稳定主场景");
        need(s.scripts.pages.size() == 1 &&
                 s.scripts.pages.front().kind == ref::WorldScriptPageKind::scene &&
                 s.scripts.pages.front().lifecycle == 2,
             "正常存档暂不支持模态页面");
    }
}
void valid_state(const StartupWorldRuntimeState &s, bool history = false) {
    std::string reason;
    if (!detail::validate_restored_state(s, reason, history))
        throw std::runtime_error(std::string("存档状态拒绝（") + (history ? "审计历史" : "当前世界") +
                                 "，step=" + std::to_string(s.simulation_steps) + "）：" + reason);
}
Bytes encode_file(const StartupWorldRuntimeSession &session,
                  const StartupWorldSaveMetadata &metadata) {
    valid_metadata(metadata);
    valid_capture(session.state(), metadata.purpose);
    valid_state(session.state());
    Writer meta;
    meta.text(metadata.producer_revision);
    meta.text(metadata.controller_id);
    meta.u64(metadata.next_frame);
    auto current = session.state();
    if (metadata.purpose == StartupWorldSavePurpose::normal) {
        current.confirm_input = false;
        current.cancel_input = false;
        current.menu_input = false;
        current.page_confirm_held = false;
        current.focus_held_input = 0;
    }
    std::vector<Section> sections;
    sections.push_back({1, 1, 1, std::move(meta.bytes)});
    sections.push_back({2, 1, 1, detail::encode_state(current)});
    if (metadata.purpose == StartupWorldSavePurpose::replay) {
        need(session.checkpoints().size() <= 4096, "审计历史超过首版预算");
        Writer audit;
        audit.u32(static_cast<std::uint32_t>(session.checkpoints().size()));
        for (const auto &checkpoint : session.checkpoints()) {
            need(static_cast<bool>(checkpoint), "空审计检查点");
            valid_state(*checkpoint, true);
            audit.blob(detail::encode_state(*checkpoint));
        }
        sections.push_back({3, 1, 1, std::move(audit.bytes)});
        sections.push_back({4, 1, 1, metadata.controller_state});
    }
    for (const auto &e : metadata.extensions)
        sections.push_back({e.id, e.version, 0, e.bytes});
    Writer file;
    file.raw(magic, 8);
    file.u32(format_version);
    file.u32(state_semantics);
    file.u32(static_cast<std::uint32_t>(metadata.purpose));
    file.text(startup_world_persistence_dataset());
    file.text(detail::codec_schema_identity());
    file.u32(static_cast<std::uint32_t>(sections.size()));
    for (const auto &s : sections) {
        file.u32(s.id);
        file.u32(s.version);
        file.u32(s.required);
        file.u64(s.bytes.size());
        file.text(dungeon_village_tools::sha256_hex(s.bytes));
        file.raw(s.bytes.data(), s.bytes.size());
    }
    const auto digest = dungeon_village_tools::sha256_hex(file.bytes);
    file.raw(digest.data(), digest.size());
    return std::move(file.bytes);
}
StartupWorldSavedSession decode_file(Bytes file, const StartupWorldRules &rules,
                                     StartupWorldSavePurpose expected_purpose,
                                     const std::string &expected_controller,
                                     detail::CodecDecodeBudget &budget) {
    need(&rules == &startup_world_rules(), "恢复只支持已登记的固定规则目录");
    need(file.size() >= 64 && file.size() <= file_budget, "存档长度非法");
    const std::string checksum(file.end() - 64, file.end());
    file.resize(file.size() - 64);
    need(checksum == dungeon_village_tools::sha256_hex(file), "存档整体摘要不符");
    Reader input{file};
    const auto signature = input.raw(8);
    need(std::equal(signature.begin(), signature.end(), magic), "存档标识不符");
    need(input.u32() == format_version && input.u32() == state_semantics,
         "不支持的存档或状态语义版本");
    const auto purpose = static_cast<StartupWorldSavePurpose>(input.u32());
    need(purpose == expected_purpose, "存档用途不符");
    need(input.text() == startup_world_persistence_dataset(), "固定数据来源不匹配");
    need(input.text() == detail::codec_schema_identity(), "状态字段布局不兼容");
    const auto count = input.u32();
    need(count >= 2 && count <= 64, "分区数非法");
    std::map<std::uint32_t, Section> sections;
    for (std::uint32_t n = 0; n < count; ++n) {
        Section s;
        s.id = input.u32();
        s.version = input.u32();
        s.required = input.u32();
        need(s.version > 0 && s.required <= 1, "分区版本或必需标志非法");
        const auto length = input.u64();
        const auto digest = input.text();
        s.bytes = input.raw(length);
        need(digest == dungeon_village_tools::sha256_hex(s.bytes), "分区摘要不符");
        need(sections.emplace(s.id, std::move(s)).second, "重复分区");
    }
    input.end();
    const auto require_section = [&](std::uint32_t id) -> const Bytes & {
        const auto it = sections.find(id);
        need(it != sections.end() && it->second.version == 1 && it->second.required == 1,
             "必需分区缺失或版本不符");
        return it->second.bytes;
    };
    StartupWorldSaveMetadata metadata;
    metadata.purpose = purpose;
    Reader meta{require_section(1)};
    metadata.producer_revision = meta.text();
    metadata.controller_id = meta.text();
    metadata.next_frame = meta.u64();
    meta.end();
    need(metadata.controller_id == expected_controller, "回放控制器语义版本不匹配");
    if (purpose == StartupWorldSavePurpose::replay)
        metadata.controller_state = require_section(4);
    for (auto &[id, s] : sections) {
        if (id <= 4) {
            need(id >= 1 && (purpose == StartupWorldSavePurpose::replay || id <= 2),
                 "该用途不允许此分区");
            continue;
        }
        need(id >= 1024 && s.required == 0, "未知必需分区或保留ID");
        metadata.extensions.push_back({id, s.version, std::move(s.bytes)});
    }
    valid_metadata(metadata);
    auto state = detail::decode_state(require_section(2), rules, budget);
    valid_capture(state, purpose);
    valid_state(state);
    std::vector<std::shared_ptr<const StartupWorldRuntimeState>> history;
    if (purpose == StartupWorldSavePurpose::replay) {
        Reader audit{require_section(3)};
        const auto size = audit.u32();
        need(size <= 4096, "审计历史超过预算");
        // 不预先reserve攻击者声明的规模；全部Owner共用节点／分配预算。
        for (std::uint32_t i = 0; i < size; ++i) {
            auto checkpoint = detail::decode_state(audit.blob(), rules, budget);
            valid_state(checkpoint, true);
            need(checkpoint.simulation_steps <= state.simulation_steps, "历史检查点晚于当前世界");
            if (!history.empty())
                need(history.back()->simulation_steps <= checkpoint.simulation_steps,
                     "审计历史次序错误");
            history.push_back(
                std::make_shared<const StartupWorldRuntimeState>(std::move(checkpoint)));
        }
        audit.end();
    } else {
        // 正常读档不把旧OS按住状态带到新进程；其余世界和随机保持保存值。
        state.confirm_input = false;
        state.cancel_input = false;
        state.menu_input = false;
        state.page_confirm_held = false;
        state.focus_held_input = 0;
    }
    return {StartupWorldPersistenceAccess::make(std::move(state), std::move(history)),
            std::move(metadata)};
}
} // namespace

namespace persistence_detail {
Bytes encode_world_session_bytes(const StartupWorldRuntimeSession &session,
                                 const StartupWorldSaveMetadata &metadata) {
    auto bytes = encode_file(session, metadata);
    // 保留原写前恢复自检，包含当前Owner与全部历史共用的解码预算。
    CodecDecodeBudget budget;
    (void)decode_file(bytes, *session.state().rules, metadata.purpose, metadata.controller_id, budget);
    return bytes;
}
StartupWorldSavedSession decode_world_session_bytes(Bytes bytes, const StartupWorldRules &rules,
                                                     StartupWorldSavePurpose purpose,
                                                     const std::string &controller) {
    CodecDecodeBudget budget;
    return decode_file(std::move(bytes), rules, purpose, controller, budget);
}
StartupWorldSavedSession decode_world_session_bytes(Bytes bytes, const StartupWorldRules &rules,
                                                     StartupWorldSavePurpose purpose,
                                                     const std::string &controller,
                                                     CodecDecodeBudget &budget) {
    return decode_file(std::move(bytes), rules, purpose, controller, budget);
}
} // namespace persistence_detail

StartupWorldSaveResult save_startup_world_file(const std::filesystem::path &path,
                                               const StartupWorldRuntimeSession &session,
                                               const StartupWorldSaveMetadata &metadata) {
    try {
        auto bytes = detail::encode_world_session_bytes(session, metadata);
        detail::replace_save_file(path, bytes);
        return {true, {}};
    } catch (const std::exception &e) {
        return {false, e.what()};
    }
}
StartupWorldLoadResult load_startup_world_file(const std::filesystem::path &path,
                                               const StartupWorldRules &rules,
                                               StartupWorldSavePurpose expected,
                                               const std::string &controller) {
    try {
        return {detail::decode_world_session_bytes(detail::read_save_file(path, file_budget), rules, expected, controller),
                {}};
    } catch (const std::exception &e) {
        return {{}, e.what()};
    }
}
std::string startup_world_state_digest(const StartupWorldRuntimeState &state) {
    return dungeon_village_tools::sha256_hex(detail::encode_state(state));
}
std::string startup_world_session_digest(const StartupWorldRuntimeSession &session) {
    // 不可变历史只算一次；弱引用缓存不延长退休世界寿命，失效项每次清理。
    using Weak = std::weak_ptr<const StartupWorldRuntimeState>;
    thread_local std::map<Weak, std::string, std::owner_less<Weak>> cache;
    for (auto it = cache.begin(); it != cache.end();) {
        if (it->first.expired())
            it = cache.erase(it);
        else
            ++it;
    }
    Writer summary;
    summary.text(startup_world_state_digest(session.state()));
    summary.u64(session.checkpoints().size());
    for (const auto &checkpoint : session.checkpoints()) {
        need(static_cast<bool>(checkpoint), "空审计检查点");
        const Weak key(checkpoint);
        auto found = cache.find(key);
        if (found == cache.end()) {
            if (cache.size() >= 4096)
                cache.clear();
            found = cache.emplace(key, startup_world_state_digest(*checkpoint)).first;
        }
        summary.text(found->second);
    }
    return dungeon_village_tools::sha256_hex(summary.bytes);
}
} // namespace dungeon_village_prototype
