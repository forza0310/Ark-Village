#include "dungeon_village_prototype/startup_application_replay.hpp"
#include "startup_application_replay_wire.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
using namespace dungeon_village_prototype;
namespace fs=std::filesystem;
using Bytes=std::vector<std::uint8_t>;
using application_replay_fixture::u32;
using application_replay_fixture::u64;
using application_replay_fixture::number;
using application_replay_fixture::set_number;
using application_replay_fixture::text;
using application_replay_fixture::resign;
using application_replay_fixture::unpack;
using application_replay_fixture::pack;
using application_replay_fixture::section;
using application_replay_fixture::control_offsets;
constexpr const char *controller="application-state-contract-v1";
struct Checks {
    int count{};
    void operator()(bool value,const std::string &reason) {
        ++count;
        if(!value)throw std::runtime_error("完整应用状态回放："+reason);
    }
    void good(const std::string &error) { (*this)(error.empty(),error); }
};
void require(bool value,const char *reason) {
    if(!value)throw std::runtime_error(std::string("应用状态夹具：")+reason);
}
struct Work {
    fs::path root;
    explicit Work(const fs::path &parent):root(parent/"application-replay-state") {
        require(fs::create_directory(root),"独占项目内目录");
    }
    ~Work() { std::error_code ec; fs::remove_all(root,ec); }
    fs::path directory(const std::string &name) {
        const auto p=root/name;
        require(fs::create_directory(p),"独占夹具子目录");
        return p;
    }
    StartupApplicationPaths paths(const std::string &name) {
        const auto dir=directory(name);
        return {dir/"system.avr",{dir/"world0.avr",dir/"world1.avr"}};
    }
};
Bytes read(const fs::path &p) {
    std::ifstream input(p,std::ios::binary);
    require(static_cast<bool>(input),"读取已存在夹具");
    return {std::istreambuf_iterator<char>(input),{}};
}
void write(const fs::path &p,const Bytes &bytes) {
    require(!fs::exists(p),"错误容器夹具不得覆盖已有文件");
    std::ofstream output(p,std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    require(static_cast<bool>(output),"写夹具失败");
}
std::uint32_t phase(const StartupApplication &app) {
    const auto page=app.page();
    if(page==StartupApplicationPage::title)return app.world()?4U:0U;
    if(page==StartupApplicationPage::configure)return app.world()?6U:1U;
    if(page==StartupApplicationPage::records)return app.world()?5U:2U;
    if(page==StartupApplicationPage::world && app.world())return 3U;
    throw std::runtime_error("应用状态Driver不注册覆盖询问或非法页世界组合");
}
// 规范小端载荷：版本/phase/下一外层轮/下一命令/输出已消费。不是永远成功的空validator。
Bytes driver(const StartupApplication &app,std::uint64_t frame,std::uint64_t command) {
    Bytes b;u32(b,1);u32(b,phase(app));u64(b,frame);u64(b,command);u32(b,1);return b;
}
using Metadata=StartupApplicationReplayMetadata;
std::string validate_driver(const StartupApplication &app,const Metadata &meta) {
    try {
        if(meta.controller_id!=controller || meta.controller_state.size()!=28)
            return "应用状态Driver身份或长度错误";
        std::size_t at{};
        const auto &b=meta.controller_state;
        if(number(b,at,4)!=1 || number(b,at,4)!=phase(app) ||
           number(b,at,8)!=meta.next_frame || number(b,at,8)!=meta.next_command ||
           number(b,at,4)!=1 || at!=b.size())return "应用状态Driver阶段/序号/输出消费不一致";
        return {};
    } catch(const std::exception &e) { return e.what(); }
}
Metadata metadata(const StartupApplication &app,std::uint64_t frame,std::uint64_t command) {
    Metadata result;
    result.controller_id=controller;result.producer_revision="application-state-conditions-v1";
    result.next_frame=frame;result.next_command=command;
    result.controller_state=driver(app,frame,command);
    // 逆ID顺序及零字节均有意保留，不能排序或丢空段。
    result.extensions={{2048,3,{0,1,0,255}},{1024,1,{}},{1300,2,{9,8,7}}};
    return result;
}
bool same_metadata(const Metadata &a,const Metadata &b) {
    if(a.controller_id!=b.controller_id || a.producer_revision!=b.producer_revision ||
       a.next_frame!=b.next_frame || a.next_command!=b.next_command ||
       a.controller_state!=b.controller_state || a.extensions.size()!=b.extensions.size())return false;
    for(std::size_t i=0;i<a.extensions.size();++i)
        if(a.extensions[i].id!=b.extensions[i].id || a.extensions[i].version!=b.extensions[i].version ||
           a.extensions[i].bytes!=b.extensions[i].bytes)return false;
    return true;
}
void fresh_targets(Checks &check,const fs::path &dir) {
    check(!fs::exists(dir/"system.avr") && !fs::exists(dir/"world0.avr") && !fs::exists(dir/"world1.avr"),
          "拒绝/准备不提前创建隔离三目标");
}
std::string digest(const StartupApplication &app,const Metadata &meta) {
    return startup_application_replay_digest(app,meta,validate_driver);
}
void roundtrip(Checks &check,Work &work,const std::string &name,StartupApplication &app,
               const Metadata &meta,StartupApplicationMode mode) {
    const auto directory=work.directory(name+"-capture");
    const auto source=directory/"state.avrapp";
    const auto before=digest(app,meta);
    check(before.size()==64,"有效应用摘要64字符");
    check.good(save_startup_application_replay(directory,source,app,meta,validate_driver));
    check(digest(app,meta)==before,"捕获不推进原应用或Driver");
    const auto original=read(source);
    const auto target_paths=work.paths(name+"-old-destination");
    StartupApplication target(target_paths,ref::WorldRandomStream::from_java_seed(900),mode);
    check.good(target.error());
    auto target_meta=metadata(target,0,0);
    const auto isolated=work.directory(name+"-restore");
    check.good(restore_startup_application_replay(source,isolated,controller,target,target_meta,validate_driver));
    check(digest(target,target_meta)==before && same_metadata(meta,target_meta),
          "完整应用与规范Driver/未知可选段顺序及字节完全往返");
    check(fs::is_regular_file(isolated/"system.avr") && !fs::exists(isolated/"world0.avr") &&
          !fs::exists(isolated/"world1.avr") && read(source)==original,
          "恢复只发布新隔离系统，源容器和两栏不改写");
    const auto records=load_startup_system_file(isolated/"system.avr");
    check(records.records && records.records->high_score==target.records().high_score &&
          records.records->cash_peak==target.records().cash_peak && records.records->last_slot==target.records().last_slot,
          "隔离系统发布的是捕获记录");
    if(app.world()) {
        check(target.world() && startup_world_session_digest(*target.world())==startup_world_session_digest(*app.world()),
              "保留world的非世界页面不丢Session或审计历史");
        check(app.handoff_random().has_value()==target.handoff_random().has_value(),"历史random交接存在资格保留");
        if(app.handoff_random()) {
            const auto &a=*app.handoff_random(),&b=*target.handoff_random();
            check(a.engine_state==b.engine_state && a.cursor==b.cursor && a.tape==b.tape && a.tape_mode==b.tape_mode,
                  "历史交接完整引擎/游标/磁带保存");
        }
    }
    // 未移交前旧应用路径仍未被创建，恢复后写入只作用于新隔离系统。
    check(!fs::exists(target_paths.system) && !fs::exists(target_paths.worlds[0]) &&
          !fs::exists(target_paths.worlds[1]),"恢复没有写旧应用路径");
}
void corruption(Checks &check,Work &work) {
    StartupApplication source_app(work.paths("corrupt-source-app"),ref::WorldRandomStream::from_java_seed(44),
                                  StartupApplicationMode::title_presentation);
    check.good(source_app.error());
    const auto source_meta=metadata(source_app,0,0);
    const auto directory=work.directory("corrupt-files");
    const auto source=directory/"source.avrapp";
    check.good(save_startup_application_replay(directory,source,source_app,source_meta,validate_driver));
    const auto original=read(source);
    const auto decoded=unpack(original);
    check(pack(decoded)==original,"独立格式解析/重签复建逐字节相同，损坏夹具不是错位猜字段");
    const auto current_paths=work.paths("corrupt-current");
    StartupSystemRecords initial_records;
    initial_records.high_score=321;initial_records.score_village="已保留纪录";
    check.good(save_startup_system_file(current_paths.system,initial_records));
    StartupApplication target(current_paths,ref::WorldRandomStream::from_java_seed(55),
                               StartupApplicationMode::title_presentation);
    check.good(target.error());
    check.good(target.request_new_game(1));check.good(target.edit_main_name("原目标草稿"));
    auto target_meta=metadata(target,0,2);
    const auto current_bytes=read(current_paths.system);
    const auto before=digest(target,target_meta);
    const auto meta_before=target_meta;
    int index{};
    const auto reject=[&](const Bytes &bytes,const char *reason) {
        const auto file=directory/("bad-"+std::to_string(index)+".avrapp");
        const auto isolated=work.directory("bad-target-"+std::to_string(index++));
        write(file,bytes);
        const auto error=restore_startup_application_replay(file,isolated,controller,target,target_meta,validate_driver);
        check(!error.empty(),reason);
        check(digest(target,target_meta)==before && same_metadata(target_meta,meta_before) &&
              read(current_paths.system)==current_bytes && read(file)==bytes,
              "坏候选保留完整旧应用/Driver/系统及输入文件");
        fresh_targets(check,isolated);
    };
    auto broken=original;broken.pop_back();reject(broken,"截断整体文件明确拒绝");
    broken=original;broken[24]^=1;reject(broken,"整体摘要不符拒绝");
    broken=original;broken[decoded.prefix.size()+4+84]^=1;resign(broken);
    reject(broken,"整体已重签但内层分区摘要不符仍拒绝");
    broken=original;broken.push_back(0);reject(broken,"尾随字节拒绝");
    auto wire=decoded;section(wire,3).id=999;reject(pack(wire),"未知必需分区重签后拒绝");
    wire=decoded;wire.sections.push_back(section(wire,3));reject(pack(wire),"重复分区重签后拒绝");
    wire=decoded;section(wire,3).version=2;reject(pack(wire),"必需分区未知版本拒绝");
    wire=decoded;section(wire,3).required=0;reject(pack(wire),"必需分区不能伪装可选段");
    wire=decoded;
    std::size_t at=20;(void)text(wire.prefix,at);(void)text(wire.prefix,at);
    const auto schema_size=number(wire.prefix,at,4);
    require(schema_size>0 && schema_size<=wire.prefix.size()-at,"应用schema字段存在");
    wire.prefix[at]=wire.prefix[at]=='0'?'1':'0';reject(pack(wire),"错误应用schema重签后拒绝");
    wire=decoded;set_number(wire.prefix,12,4,2);reject(pack(wire),"未知应用语义版本拒绝");
    wire=decoded;set_number(wire.prefix,16,4,2);reject(pack(wire),"未知捕获边界版本拒绝");
    broken=original;set_number(broken,decoded.prefix.size(),4,66);resign(broken);
    reject(broken,"声明超过总分区数量预算拒绝");
    broken=original;set_number(broken,decoded.prefix.size()+4+12,8,128ULL*1024ULL*1024ULL+1ULL);resign(broken);
    reject(broken,"分区声明超过整体预算先拒绝，不按不可信长度分配");
    wire=decoded;section(wire,2048).bytes.resize(1024U*1024U+1U,0);
    reject(pack(wire),"实际未知可选段超过单段预算拒绝");
    wire=decoded;set_number(section(wire,3).bytes,4,4,99);reject(pack(wire),"重签后非法page枚举拒绝");
    wire=decoded;set_number(section(wire,3).bytes,4,4,4);reject(pack(wire),"world页却没有world的关系拒绝");
    wire=decoded;set_number(section(wire,3).bytes,0,4,99);reject(pack(wire),"未知应用mode拒绝");
    wire=decoded;auto offsets=control_offsets(section(wire,3).bytes);
    section(wire,3).bytes[offsets.name]=0;reject(pack(wire),"重签后姓名控制字节拒绝");
    wire=decoded;offsets=control_offsets(section(wire,3).bytes);
    set_number(section(wire,3).bytes,offsets.decorations_count,4,0xffffffffU);
    reject(pack(wire),"重签后非法名单count预算拒绝");
    wire=decoded;offsets=control_offsets(section(wire,3).bytes);
    set_number(section(wire,3).bytes,4,4,3);set_number(section(wire,3).bytes,offsets.requests,8,0);
    reject(pack(wire),"纪录页没有显式请求且名单不符资格不能靠恢复补抽");
    wire=decoded;offsets=control_offsets(section(wire,3).bytes);
    set_number(section(wire,3).bytes,offsets.world,4,1);reject(pack(wire),"world存在标记与分区4不一致拒绝");
    wire=decoded;set_number(section(wire,5).bytes,24,4,0);
    reject(pack(wire),"重签后Driver未消费输出拒绝");
    wire=decoded;set_number(section(wire,5).bytes,8,8,1);
    reject(pack(wire),"重签后Driver下一外层轮与metadata不一致拒绝");
    wire=decoded;set_number(section(wire,5).bytes,16,8,1);
    reject(pack(wire),"重签后Driver下一命令与metadata不一致拒绝");
    wire=decoded;set_number(section(wire,5).bytes,4,4,2);
    reject(pack(wire),"重签后Driver页面phase与应用不符拒绝");
    wire=decoded;section(wire,5).bytes.clear();reject(pack(wire),"空Driver载荷不能占位通过");

    const auto wrong_controller=work.directory("wrong-controller");
    check(!restore_startup_application_replay(source,wrong_controller,"not-this-controller",target,target_meta,validate_driver).empty(),
          "调用者期望controller不符拒绝");
    fresh_targets(check,wrong_controller);
    StartupApplication logic(work.paths("mode-logic"),ref::WorldRandomStream::from_java_seed(66));
    auto logic_meta=metadata(logic,0,0);const auto logic_before=digest(logic,logic_meta);
    const auto wrong_mode=work.directory("wrong-mode");
    check(!restore_startup_application_replay(source,wrong_mode,controller,logic,logic_meta,validate_driver).empty() &&
          digest(logic,logic_meta)==logic_before,"应用回放不隐式切换调用者mode");
    fresh_targets(check,wrong_mode);
    const auto validator_failure=work.directory("validator-failure");
    const auto refuse=[](const StartupApplication &app,const Metadata &meta)->std::string {
        const auto error=validate_driver(app,meta);
        return error.empty()?"有意拒绝已解码候选，不允许发布":error;
    };
    check(!restore_startup_application_replay(source,validator_failure,controller,target,target_meta,refuse).empty(),
          "外部Driver严格验证失败在发布之前拒绝");
    fresh_targets(check,validator_failure);
    for(const char *name:{"system.avr","world0.avr","world1.avr"}) {
        const auto occupied=work.directory(std::string("occupied-")+name);
        const Bytes retained{1,2,3,4};write(occupied/name,retained);
        check(!restore_startup_application_replay(source,occupied,controller,target,target_meta,validate_driver).empty() &&
              read(occupied/name)==retained && std::distance(fs::directory_iterator(occupied),fs::directory_iterator{})==1,
              "任一恢复目标已存在时原内容保留且不创建另外两个文件");
    }
    auto bad_meta=source_meta;bad_meta.controller_state[24]=0;
    const auto save_rejected=directory/"unconsumed.avrapp";
    check(!save_startup_application_replay(directory,save_rejected,source_app,bad_meta,validate_driver).empty() &&
          !fs::exists(save_rejected),"捕获未消费Driver输出不产生容器");
    check(!save_startup_application_replay(directory,source,source_app,source_meta,validate_driver).empty() &&
          read(source)==original,"捕获不能覆盖已有容器");
    check(digest(target,target_meta)==before && same_metadata(target_meta,meta_before) &&
          read(current_paths.system)==current_bytes && read(source)==original,
          "所有外围拒绝之后旧应用/Driver/系统/源完整不变");
}
} // namespace

int run_startup_application_replay_state_checks(const std::filesystem::path &parent) {
    Checks check;Work work(parent);
    StartupApplication app(work.paths("source-app"),ref::WorldRandomStream::from_java_seed(42),
                            StartupApplicationMode::title_presentation);
    check.good(app.error());
    std::uint64_t command{};
    roundtrip(check,work,"title-no-world",app,metadata(app,0,command),StartupApplicationMode::title_presentation);
    check.good(app.request_new_game(1));++command;
    check.good(app.edit_village("配置草稿村"));++command;
    check.good(app.edit_main_name("完整回放主角"));++command;
    check.good(app.edit_sex(1));++command;
    roundtrip(check,work,"configure-no-world",app,metadata(app,0,command),StartupApplicationMode::title_presentation);
    check.good(app.cancel_configuration());++command;
    check.good(app.open_records());++command;
    check.good(app.turn_record_page(1));++command;
    const auto title_random=app.capture_title_replay();
    check(title_random && title_random->random.cursor>0 && !app.decorations().empty(),
          "真实open_records已推进独立标题随机并产生名单");
    roundtrip(check,work,"records-no-world",app,metadata(app,0,command),StartupApplicationMode::title_presentation);
    check.good(app.return_to_title());++command;
    check.good(app.request_new_game(1));++command;
    check.good(app.start_game());++command;
    app.take_sound_requests();
    check(app.world() && app.handoff_random() && app.handoff_random()->cursor==title_random->random.cursor &&
          app.world()->state().scene.random.draws()==title_random->random.cursor,
          "合法开始操作交接真实标题随机；不是测试补写world或handoff");
    roundtrip(check,work,"world-after-handoff",app,metadata(app,0,command),StartupApplicationMode::title_presentation);
    check.good(app.return_to_title());++command;
    const auto world_digest=startup_world_session_digest(*app.world());
    roundtrip(check,work,"title-with-world",app,metadata(app,0,command),StartupApplicationMode::title_presentation);
    check.good(app.open_records());++command;
    check(startup_world_session_digest(*app.world())==world_digest,"保留世界的纪录请求只推进标题流");
    roundtrip(check,work,"records-with-world",app,metadata(app,0,command),StartupApplicationMode::title_presentation);
    corruption(check,work);
    return check.count;
}
