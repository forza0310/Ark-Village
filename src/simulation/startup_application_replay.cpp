#include "ark/simulation/startup_application_replay.hpp"
#include "ark/assets/sha256.hpp"
#include "startup_application_replay_paths.hpp"
#include "startup_persistence_bytes.hpp"
#include "startup_world_codec.hpp"
#include "startup_world_file_io.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ark::simulation {
namespace {
using Bytes = std::vector<std::uint8_t>;
using Metadata = StartupApplicationReplayMetadata;
using Validator = StartupApplicationReplayValidator;
using Page = StartupApplicationPage;
using Mode = StartupApplicationMode;
namespace detail = persistence_detail;
#include "startup_application_replay_fields.inc"
constexpr std::size_t file_budget = 128U * 1024U * 1024U;
constexpr std::size_t small_budget = 1024U * 1024U;
constexpr std::size_t system_budget = 4U * 1024U * 1024U;
constexpr char nested_controller[] = "application-world-session-v1";
const Bytes nested_state{1};

void need(bool ok, const char *reason) {
    if (!ok) throw std::runtime_error(std::string("应用回放拒绝：") + reason);
}
struct Writer {
    Bytes bytes;
    std::size_t limit;
    explicit Writer(std::size_t budget) : limit(budget) {}
    void raw(const void *data, std::size_t n) {
        need(bytes.size() <= limit && n <= limit - bytes.size(), "编码超预算");
        if (n) {
            const auto *p = static_cast<const std::uint8_t *>(data);
            bytes.insert(bytes.end(), p, p + n);
        }
    }
    void u32(std::uint32_t n) {
        for (int i = 0; i < 4; ++i) { const auto v = static_cast<std::uint8_t>(n >> (i*8)); raw(&v, 1); }
    }
    void u64(std::uint64_t n) {
        for (int i = 0; i < 8; ++i) { const auto v = static_cast<std::uint8_t>(n >> (i*8)); raw(&v, 1); }
    }
    void i32(std::int32_t n) { u32(static_cast<std::uint32_t>(n)); }
    void i64(std::int64_t n) { u64(static_cast<std::uint64_t>(n)); }
    void boolean(bool value) { u32(value ? 1U : 0U); }
    void text(const std::string &s) {
        need(s.size() <= 4096, "文本超预算");
        u32(static_cast<std::uint32_t>(s.size())); raw(s.data(), s.size());
    }
};
struct Reader {
    const Bytes &bytes;
    std::size_t at{};
    void room(std::size_t n) const { need(at <= bytes.size() && n <= bytes.size()-at, "载荷截断"); }
    std::uint32_t u32() {
        room(4); std::uint32_t n{};
        for (int i=0; i<4; ++i) n |= static_cast<std::uint32_t>(bytes[at++]) << (i*8);
        return n;
    }
    std::uint64_t u64() {
        room(8); std::uint64_t n{};
        for (int i=0; i<8; ++i) n |= static_cast<std::uint64_t>(bytes[at++]) << (i*8);
        return n;
    }
    std::int32_t i32() {
        const auto n=u32();
        return n <= INT32_MAX ? static_cast<std::int32_t>(n) :
            static_cast<std::int32_t>(static_cast<std::int64_t>(n)-(INT64_C(1)<<32));
    }
    std::int64_t i64() {
        const auto n=u64();
        return n <= INT64_MAX ? static_cast<std::int64_t>(n) :
            -1-static_cast<std::int64_t>(UINT64_MAX-n);
    }
    bool boolean() { const auto n=u32(); need(n<=1,"布尔值非法"); return n!=0; }
    Bytes raw(std::uint64_t n) {
        need(n<=file_budget,"分区超预算"); room(static_cast<std::size_t>(n));
        const auto begin=bytes.begin()+static_cast<std::ptrdiff_t>(at);
        at+=static_cast<std::size_t>(n);
        return {begin,bytes.begin()+static_cast<std::ptrdiff_t>(at)};
    }
    std::string text() {
        const auto n=u32(); need(n<=4096,"文本超预算"); const auto b=raw(n);
        return {b.begin(),b.end()};
    }
    void end() const { need(at==bytes.size(),"分区存在尾字节"); }
};
void write_random(Writer &out, const ref::WorldRandomSnapshot &s) {
    need(s.tape.size() <= small_budget/4, "随机磁带超预算");
    need(ref::WorldRandomStream::from_snapshot(s).has_value(), "随机快照非法");
    out.u64(s.engine_state); out.u64(s.cursor); out.boolean(s.tape_mode);
    out.u32(static_cast<std::uint32_t>(s.tape.size()));
    for (const auto v:s.tape) out.i32(v);
}
ref::WorldRandomSnapshot read_random(Reader &in) {
    ref::WorldRandomSnapshot s;
    s.engine_state=in.u64(); s.cursor=in.u64(); s.tape_mode=in.boolean();
    const auto count=in.u32(); need(count<=small_budget/4,"随机磁带声明超预算");
    in.room(static_cast<std::size_t>(count)*4);
    s.tape.reserve(count);
    for(std::uint32_t i=0;i<count;++i)s.tape.push_back(in.i32());
    need(ref::WorldRandomStream::from_snapshot(s).has_value(),"随机快照非法");
    return s;
}
void valid_metadata(const Metadata &m, const Validator &validator) {
    need(static_cast<bool>(validator),"缺少实际Driver校验器");
    need(!m.controller_id.empty() && m.controller_id.size()<=256 && m.producer_revision.size()<=256,
         "Driver身份或来源长度非法");
    need(m.controller_id.find('\0')==std::string::npos && m.producer_revision.find('\0')==std::string::npos,
         "身份含NUL");
    need(!m.controller_state.empty() && m.controller_state.size()<=small_budget,"Driver载荷为空或超预算");
    need(m.extensions.size()<=60,"可选分区过多");
    std::set<std::uint32_t> seen;
    for(const auto &e:m.extensions)
        need(e.id>=1024 && e.version>0 && e.bytes.size()<=small_budget && seen.insert(e.id).second,
             "可选分区身份重复、保留或超预算");
}
void validate_driver(const StartupApplication &app, const Metadata &m, const Validator &validator) {
    const auto reason=validator(app,m);
    if(!reason.empty())throw std::runtime_error("应用回放Driver拒绝："+reason);
}
const ref::WorldScriptPage *top(const StartupWorldRuntimeState &s) {
    for(auto p=s.scripts.pages.rbegin();p!=s.scripts.pages.rend();++p)
        if(p->lifecycle!=4)return &*p;
    return nullptr;
}
bool is_clear(const ref::WorldScriptPage *p) {
    return p && p->kind==ref::WorldScriptPageKind::raw_page && p->legacy_page==17;
}
} // namespace

// 唯一封闭恢复桥：只在完整字节、关系和外部Driver校验后交给最终联合安装。
// 不借普通构造读隔离默认系统，也不调用install_loaded覆盖捕获镜像。
struct StartupApplicationReplayAccess {
    static const StartupApplicationPaths &paths(const StartupApplication &a) { return a.paths_; }
    static Mode mode(const StartupApplication &a) { return a.mode_; }
    static void validate(const StartupApplication &a) {
        need(a.error_.empty(),"不健康应用不可捕获");
        need(a.mode_==Mode::logic || a.mode_==Mode::title_presentation,"应用模式未知");
        need(validate_startup_title_presentation(a.title_).empty(),"标题表现状态非法");
        need(a.mode_!=Mode::logic || pristine_startup_title_presentation(a.title_),
             "逻辑模式含已推进标题表现状态");
        need(a.page_>=Page::title && a.page_<=Page::world,"应用页面未知");
        need(a.record_page_>=0 && a.record_page_<=1 && a.draft_.slot>=0 && a.draft_.slot<=1,
             "草稿栏位或纪录页非法");
        need(valid_startup_world_human_profile({a.draft_.village,0,false}) &&
             valid_startup_world_human_profile(a.draft_.main_character),"草稿文字或人物非法");
        need(validate_startup_system_records(a.records_).empty(),"系统记录非法");
        need(ref::WorldRandomStream::from_snapshot(a.random_.snapshot()).has_value(),"标题随机非法");
        if(a.handoff_)need(ref::WorldRandomStream::from_snapshot(*a.handoff_).has_value(),"交接随机非法");
        need(a.page_!=Page::world || a.world_.has_value(),"世界页缺世界");
        need(a.page_==Page::records || a.decorations_.empty(),"非纪录页仍有装饰名单");
        need(a.mode_!=Mode::logic || a.decorations_.empty(),"逻辑模式含表现名单");
        need(a.page_!=Page::records || a.requests_>0,"纪录页缺显式请求");
        std::set<int> seen;
        for(int id:a.decorations_)
            need(startup_world_human_profile(startup_world_rules(),id).has_value() && seen.insert(id).second,
                 "装饰定义引用缺失或重复");
        if(a.page_==Page::records && a.mode_==Mode::title_presentation) {
            std::vector<int> pool;
            for(const auto &h:startup_world_rules().humans) {
                int status=h.status;
                if(a.world_) {
                    const auto it=a.world_->state().human_presence.find(h.identity);
                    need(it!=a.world_->state().human_presence.end(),"保留世界的人物资格缺失");
                    status=it->second;
                }
                if(status==1)pool.push_back(h.identity);
            }
            if(pool.empty()) {
                for(const auto &h:startup_world_rules().humans)if(h.flags&1U)pool.push_back(h.identity);
                need(a.decorations_==pool,"后备名单不是原定义序");
            } else {
                need(a.decorations_.size()==std::min<std::size_t>(5,pool.size()),"装饰名单数量不符");
                for(int id:a.decorations_)need(std::find(pool.begin(),pool.end(),id)!=pool.end(),"装饰定义未开放");
            }
        }
        const bool has_clear=a.clear_.has_value();
        need(a.clear_rows_.has_value()==has_clear && a.clear_id_.has_value()==has_clear,
             "计分三元组不完整");
        if(a.world_) {
            const auto &s=a.world_->state();
            need(!s.scripts.executing_page && s.sound_requests.empty(),"世界未到已消费输出的完整轮末");
            need(s.cash_peak==a.records_.cash_peak && s.cash_peak_village==a.records_.cash_village,
                 "系统资金纪录与世界运行镜像不同");
            const auto *p=top(s);
            if(has_clear) {
                need(a.page_==Page::world && is_clear(p) && p->id==*a.clear_id_ && !s.scene.framework_paused,
                     "计分控制器页面引用或暂停资格非法");
                const auto counter=s.page_counters.find(p->id), phase=s.page_phases.find(p->id);
                need(counter!=s.page_counters.end() && phase!=s.page_phases.end() &&
                     counter->second==a.clear_->counter && phase->second==a.clear_->stage,
                     "世界计分页与应用计数阶段不同");
                need(validate_startup_clear_score_page(*a.clear_rows_,*a.clear_)==StartupClearScoreError::none,
                     "计分阶段累计关系非法");
                need(a.clear_->captured_high_score==a.records_.high_score,"捕获最高分与系统纪录不同");
                const auto rows=startup_world_clear_score(s);
                need(rows.candidate.has_value(),"世界六类计分投影拒绝");
                for(std::size_t i=0;i<6;++i)
                    need((*rows.candidate)[i].count==(*a.clear_rows_)[i].count &&
                         (*rows.candidate)[i].score==(*a.clear_rows_)[i].score,"捕获六行不是当前世界投影");
            } else if(is_clear(p)) {
                const auto counter=s.page_counters.find(p->id), phase=s.page_phases.find(p->id);
                need((counter==s.page_counters.end() || counter->second==0) &&
                     (phase==s.page_phases.end() || phase->second==0),"已推进raw17缺应用计分控制器");
            }
        } else need(!has_clear,"无世界却存在计分控制器");
    }
    static Bytes control(const StartupApplication &a) {
        Writer out(small_budget);
        out.u32(static_cast<std::uint32_t>(a.mode_)); out.u32(static_cast<std::uint32_t>(a.page_));
        out.i32(a.record_page_); out.text(a.draft_.village); out.text(a.draft_.main_character.name);
        out.i32(a.draft_.main_character.sex); out.boolean(a.draft_.main_character.custom_name); out.i32(a.draft_.slot);
        write_random(out,a.random_.snapshot()); out.u64(a.requests_);
        need(a.decorations_.size()<=startup_world_rules().humans.size(),"装饰列表超定义规模");
        out.u32(static_cast<std::uint32_t>(a.decorations_.size()));
        for(int id:a.decorations_)out.i32(id);
        out.boolean(a.handoff_.has_value()); if(a.handoff_)write_random(out,*a.handoff_);
        out.boolean(a.world_.has_value()); out.boolean(a.clear_rows_.has_value());
        if(a.clear_rows_)for(const auto &row:*a.clear_rows_){out.i64(row.count);out.i64(row.score);}
        out.boolean(a.clear_.has_value());
        if(a.clear_) {
            const auto &c=*a.clear_;
            out.i32(c.stage);out.i32(c.counter);out.i32(c.row);out.i64(c.sum);out.i64(c.captured_high_score);
            out.boolean(c.finished);out.boolean(c.new_record);out.i32(c.trophy);
        }
        out.boolean(a.clear_id_.has_value());if(a.clear_id_)out.u64(*a.clear_id_);
        // 语义 v2 在控制分区尾部追加原标题背景；inactive 槽也完整保存。
        out.i32(a.title_.l);out.i32(a.title_.f132f);out.i32(a.title_.s);out.i32(a.title_.t);
        for(const auto &slot:a.title_.slots) {
            out.i32(slot.active);out.i32(slot.definition);out.i32(slot.x);out.i32(slot.y);
            out.i32(slot.direction);out.i32(slot.age);
        }
        return std::move(out.bytes);
    }
    static StartupApplication restore(const Bytes &bytes, StartupApplicationPaths paths,
                                       StartupSystemRecords records,
                                       std::optional<StartupWorldRuntimeSession> world, Mode expected) {
        Reader in{bytes};
        const auto mode_value=in.u32(),page_value=in.u32();
        need(mode_value<=static_cast<std::uint32_t>(Mode::title_presentation) &&
             mode_value==static_cast<std::uint32_t>(expected),"应用回放模式与调用者不同");
        need(page_value<=static_cast<std::uint32_t>(Page::world),"应用页面未知");
        const int record_page=in.i32();
        StartupTitleDraft draft; draft.village=in.text();draft.main_character.name=in.text();
        draft.main_character.sex=in.i32();draft.main_character.custom_name=in.boolean();draft.slot=in.i32();
        const auto random=ref::WorldRandomStream::from_snapshot(read_random(in));
        need(random.has_value(),"标题随机无法恢复");
        StartupApplication a(StartupApplication::RestoreTag{},std::move(paths),*random,expected);
        a.records_=std::move(records);a.draft_=std::move(draft);a.world_=std::move(world);
        a.page_=static_cast<Page>(page_value);a.record_page_=record_page;a.requests_=in.u64();
        const auto count=in.u32();need(count<=startup_world_rules().humans.size(),"装饰名单声明超定义规模");
        in.room(static_cast<std::size_t>(count)*4);
        for(std::uint32_t i=0;i<count;++i)a.decorations_.push_back(in.i32());
        if(in.boolean())a.handoff_=read_random(in);
        need(in.boolean()==a.world_.has_value(),"world存在标志与分区不同");
        if(in.boolean()) {
            a.clear_rows_.emplace();
            for(auto &row:*a.clear_rows_){row.count=in.i64();row.score=in.i64();}
        }
        if(in.boolean()) {
            a.clear_.emplace();auto &c=*a.clear_;
            c.stage=in.i32();c.counter=in.i32();c.row=in.i32();c.sum=in.i64();c.captured_high_score=in.i64();
            c.finished=in.boolean();c.new_record=in.boolean();c.trophy=in.i32();
        }
        if(in.boolean())a.clear_id_=in.u64();
        a.title_.l=in.i32();a.title_.f132f=in.i32();a.title_.s=in.i32();a.title_.t=in.i32();
        for(auto &slot:a.title_.slots) {
            slot.active=in.i32();slot.definition=in.i32();slot.x=in.i32();slot.y=in.i32();
            slot.direction=in.i32();slot.age=in.i32();
        }
        in.end(); validate(a); return a;
    }
};

namespace {
struct Section { std::uint32_t id,version,required;Bytes bytes; };
Bytes encode(const StartupApplication &a,const Metadata &m,const Validator &validator) {
    valid_metadata(m,validator);StartupApplicationReplayAccess::validate(a);validate_driver(a,m,validator);
    Writer meta(small_budget);meta.text(m.controller_id);meta.text(m.producer_revision);
    meta.u64(m.next_frame);meta.u64(m.next_command);
    std::vector<Section> sections;
    sections.push_back({1,1,1,std::move(meta.bytes)});
    sections.push_back({2,1,1,detail::encode_system_records_bytes(a.records())});
    sections.push_back({3,1,1,StartupApplicationReplayAccess::control(a)});
    if(a.world()) {
        StartupWorldSaveMetadata nested;
        nested.purpose=StartupWorldSavePurpose::replay;nested.producer_revision=m.producer_revision;
        nested.controller_id=nested_controller;nested.controller_state=nested_state;nested.next_frame=m.next_frame;
        sections.push_back({4,1,1,detail::encode_world_session_bytes(*a.world(),nested)});
    }
    sections.push_back({5,1,1,m.controller_state});
    for(const auto &e:m.extensions)sections.push_back({e.id,e.version,0,e.bytes});
    Writer file(file_budget);file.raw("AVRAPP01",8);file.u32(1);file.u32(3);file.u32(1);
    file.text(startup_world_persistence_dataset());file.text(detail::codec_schema_identity());
    file.text(application_schema);file.u32(static_cast<std::uint32_t>(sections.size()));
    for(const auto &section:sections) {
        file.u32(section.id);file.u32(section.version);file.u32(section.required);file.u64(section.bytes.size());
        const auto digest=ark::assets::sha256_hex(section.bytes);file.raw(digest.data(),digest.size());
        file.raw(section.bytes.data(),section.bytes.size());
    }
    const auto digest=ark::assets::sha256_hex(file.bytes);file.raw(digest.data(),digest.size());
    return std::move(file.bytes);
}
struct Candidate { StartupApplication app; Metadata metadata; Bytes system_bytes; };
Candidate decode(Bytes file,StartupApplicationPaths paths,Mode expected,const std::string &controller,
                 const Validator &validator) {
    need(file.size()>=64 && file.size()<=file_budget,"容器整体长度非法");
    const std::string digest(file.end()-64,file.end());file.resize(file.size()-64);
    need(digest==ark::assets::sha256_hex(file),"容器整体摘要不符");
    Reader in{file};const auto signature=in.raw(8);
    need(std::string(signature.begin(),signature.end())=="AVRAPP01","容器标识不符");
    need(in.u32()==1 && in.u32()==3 && in.u32()==1,"格式、应用语义或捕获边界版本未知");
    need(in.text()==startup_world_persistence_dataset(),"数据来源不匹配");
    need(in.text()==detail::codec_schema_identity(),"世界字段身份不匹配");
    need(in.text()==application_schema,"应用字段身份不匹配");
    const auto count=in.u32();need(count>=4 && count<=65,"分区数非法");
    std::vector<Section> sections;std::set<std::uint32_t> ids;
    for(std::uint32_t i=0;i<count;++i) {
        const auto id=in.u32(),version=in.u32(),required=in.u32();const auto length=in.u64();
        need(version>0 && required<=1 && ids.insert(id).second,"分区身份、版本、必需位重复或非法");
        need(id>=1 && ((id<=5 && version==1 && required==1) || (id>=1024 && required==0)),
             "未知必需段或保留分区");
        const auto limit=id==4?file_budget:id==2?system_budget:small_budget;
        need(length<=limit,"分区载荷超预算");
        const auto hash=in.raw(64);auto bytes=in.raw(length);
        need(std::string(hash.begin(),hash.end())==ark::assets::sha256_hex(bytes),"分区摘要不符");
        sections.push_back({id,version,required,std::move(bytes)});
    }
    in.end();
    const auto section=[&](std::uint32_t id)->Section & {
        const auto it=std::find_if(sections.begin(),sections.end(),[id](const Section &s){return s.id==id;});
        need(it!=sections.end(),"必需段缺失");return *it;
    };
    Metadata metadata;Reader meta{section(1).bytes};metadata.controller_id=meta.text();
    metadata.producer_revision=meta.text();metadata.next_frame=meta.u64();metadata.next_command=meta.u64();meta.end();
    need(!controller.empty() && metadata.controller_id==controller,"外部Driver身份不匹配");
    metadata.controller_state=std::move(section(5).bytes);
    for(auto &s:sections)if(s.id>=1024)metadata.extensions.push_back({s.id,s.version,std::move(s.bytes)});
    valid_metadata(metadata,validator);
    auto system_bytes=std::move(section(2).bytes);
    auto records=detail::decode_system_records_bytes(system_bytes);
    std::optional<StartupWorldRuntimeSession> world;
    if(ids.count(4)) {
        auto nested=detail::decode_world_session_bytes(std::move(section(4).bytes),startup_world_rules(),
                                                       StartupWorldSavePurpose::replay,nested_controller);
        need(nested.metadata.next_frame==metadata.next_frame && nested.metadata.controller_state==nested_state &&
             nested.metadata.extensions.empty() && nested.metadata.producer_revision==metadata.producer_revision,
             "嵌套世界与容器边界不同");
        world=std::move(nested.session);
    }
    auto app=StartupApplicationReplayAccess::restore(section(3).bytes,std::move(paths),std::move(records),
                                                    std::move(world),expected);
    validate_driver(app,metadata,validator);
    return {std::move(app),std::move(metadata),std::move(system_bytes)};
}
} // namespace

const char *startup_application_replay_schema() { return application_schema; }
std::string startup_application_replay_digest(const StartupApplication &app,const Metadata &metadata,
                                               const Validator &validator) {
    // 每轮完整状态比较不必反复构造/解码整个磁盘容器；真实保存仍走encode+decode自检。
    // Session摘要覆盖当前Owner和原序全部历史，复用弱引用历史摘要缓存，不删字段或历史。
    valid_metadata(metadata,validator);
    StartupApplicationReplayAccess::validate(app);
    validate_driver(app,metadata,validator);
    Writer summary(file_budget);
    summary.text("application-state-digest-v1");
    summary.text(startup_world_persistence_dataset());
    summary.text(detail::codec_schema_identity());summary.text(application_schema);
    const auto system=detail::encode_system_records_bytes(app.records());
    const auto control=StartupApplicationReplayAccess::control(app);
    summary.u64(system.size());summary.raw(system.data(),system.size());
    summary.u64(control.size());summary.raw(control.data(),control.size());
    summary.boolean(app.world()!=nullptr);
    if(app.world())summary.text(startup_world_session_digest(*app.world()));
    summary.text(metadata.controller_id);summary.text(metadata.producer_revision);
    summary.u64(metadata.next_frame);summary.u64(metadata.next_command);
    summary.u64(metadata.controller_state.size());
    summary.raw(metadata.controller_state.data(),metadata.controller_state.size());
    summary.u32(static_cast<std::uint32_t>(metadata.extensions.size()));
    for(const auto &extension:metadata.extensions) {
        summary.u32(extension.id);summary.u32(extension.version);summary.u64(extension.bytes.size());
        summary.raw(extension.bytes.data(),extension.bytes.size());
    }
    return ark::assets::sha256_hex(summary.bytes);
}
std::string save_startup_application_replay(const std::filesystem::path &directory,
                                           const std::filesystem::path &output,
                                           const StartupApplication &app,const Metadata &metadata,
                                           const Validator &validator,
                                           const std::vector<std::filesystem::path> &protected_paths) {
    try {
        const auto paths=detail::prepare_replay_capture_paths(directory,output,StartupApplicationReplayAccess::paths(app),protected_paths);
        auto bytes=encode(app,metadata,validator);
        (void)decode(bytes,StartupApplicationReplayAccess::paths(app),StartupApplicationReplayAccess::mode(app),
                     metadata.controller_id,validator);
        // 重新核预检后的路径状态；最终create-only仍保证同名目标竞态不覆盖。
        (void)detail::prepare_replay_capture_paths(directory,output,StartupApplicationReplayAccess::paths(app),protected_paths);
        detail::create_save_file(paths.container,bytes);
        return {};
    } catch(const std::exception &e) { return e.what(); }
}
std::string restore_startup_application_replay(const std::filesystem::path &source,
                                              const std::filesystem::path &isolated,
                                              const std::string &controller,
                                              StartupApplication &app,Metadata &metadata,
                                              const Validator &validator,
                                              const std::vector<std::filesystem::path> &protected_paths) {
    static_assert(std::is_nothrow_move_assignable_v<StartupApplication>);
    static_assert(std::is_nothrow_move_assignable_v<Metadata>);
    try {
        need(app.error().empty(),"调用者应用不健康");
        const auto paths=detail::prepare_replay_restore_paths(isolated,source,StartupApplicationReplayAccess::paths(app),protected_paths);
        auto candidate=decode(detail::read_save_file(paths.container,file_budget),paths.application,
                              StartupApplicationReplayAccess::mode(app),controller,validator);
        (void)detail::prepare_replay_restore_paths(isolated,source,StartupApplicationReplayAccess::paths(app),protected_paths);
        // 最后一次可失败动作只发布一份新系统文件；此后应用与Driver规范字节共同noexcept安装。
        detail::create_save_file(paths.application.system,candidate.system_bytes);
        app=std::move(candidate.app);metadata=std::move(candidate.metadata);
        return {};
    } catch(const std::exception &e) { return e.what(); }
}
} // namespace ark::simulation
