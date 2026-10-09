#include "dungeon_village_prototype/startup_application_replay.hpp"
#include "startup_application_replay_wire.hpp"
#include "startup_application_replay_paths.hpp"
#include "startup_world_file_io.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

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
Metadata title_metadata(std::uint64_t next_frame);
std::string validate_title_driver(const StartupApplication &,const Metadata &);
std::string validate_title_restore_driver(const StartupApplication &,const Metadata &);
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
bool same_title(const StartupTitlePresentation &a,const StartupTitlePresentation &b) {
    if(a.l!=b.l || a.f132f!=b.f132f || a.s!=b.s || a.t!=b.t)return false;
    for(std::size_t i=0;i<a.slots.size();++i) {
        const auto &x=a.slots[i],&y=b.slots[i];
        if(x.active!=y.active || x.definition!=y.definition || x.x!=y.x ||
           x.y!=y.y || x.direction!=y.direction || x.age!=y.age)return false;
    }
    return true;
}
bool same_plan(const StartupTitleProjection &a,const StartupTitleProjection &b) {
    if(!a.error.empty() || !b.error.empty() || a.people.size()!=b.people.size())return false;
    for(std::size_t i=0;i<a.people.size();++i) {
        const auto &x=a.people[i],&y=b.people[i];
        if(x.slot!=y.slot || x.definition!=y.definition || x.age!=y.age ||
           x.step!=y.step || x.facing!=y.facing || x.anchor!=y.anchor || x.layers!=y.layers)return false;
    }
    return true;
}
StartupTitleUpdateRequest title_update(bool confirm=false) {
    return {StartupTitleAdmission::top_lifecycle_ready,confirm};
}

void title_owner_transactions(Checks &check,Work &work) {
    // 低层四个耗尽位置由内核套件负责；Owner只覆盖最后一抽失败的跨状态事务。
    StartupApplication exhausted(work.paths("title-exhausted"),ref::WorldRandomStream::from_raw({99,10,1}),
                                 StartupApplicationMode::title_presentation);
    check.good(exhausted.error());
    for(int i=0;i<98;++i)check.good(exhausted.advance_title_background(title_update()).error);
    const auto meta=metadata(exhausted,98,98);
    const auto before=digest(exhausted,meta);
    const auto failed=exhausted.advance_title_background(title_update(true));
    check(!failed.error.empty() && !failed.confirm_consumed && !failed.menu_confirm_ready &&
          failed.random_draws==0 && digest(exhausted,meta)==before,
          "Owner出生末抽耗尽不提交q/标题计数/随机/页面，也不吞确认脉冲");
    auto bad=*exhausted.capture_title_replay();
    bad.title.slots[19].active=2;
    check(!exhausted.restore_title_replay(bad).empty() && digest(exhausted,meta)==before,
          "内存标题快照坏槽不部分安装");
    bad=*exhausted.capture_title_replay();bad.controller="startup-title-v1";
    check(!exhausted.restore_title_replay(bad).empty() && digest(exhausted,meta)==before,
          "不静默迁移没有q载荷的旧标题控制器");
    StartupApplication logic(work.paths("title-logic-refuse"),ref::WorldRandomStream::from_java_seed(4));
    const auto lm=metadata(logic,0,0);const auto lb=digest(logic,lm);
    check(!logic.advance_title_background(title_update(true)).error.empty() && digest(logic,lm)==lb,
          "logic模式显式拒绝标题背景更新，保持旧世界黄金随机资格");

    const auto paths=work.paths("title-owner-source");
    check.good(save_startup_system_file(paths.system,{}));
    StartupApplication app(paths,ref::WorldRandomStream::from_java_seed(42),StartupApplicationMode::title_presentation);
    for(int i=0;i<100;++i)check.good(app.advance_title_background(title_update()).error);
    const auto title_before=app.title_presentation();
    const auto random_before=app.capture_title_replay()->random.cursor;
    std::uint64_t pool{};
    for(const auto &human:startup_world_rules().humans)if(human.status==1)++pool;
    check(pool>0,"本固定新局定义存在非空纪录装饰池");
    check.good(app.open_records());
    check(app.capture_title_replay()->random.cursor==random_before+pool &&
          same_title(app.title_presentation(),title_before),
          "完成标题出生后开纪录另外抽N次，开页不隐式推进q");
    auto child_meta=metadata(app,100,101);const auto child_before=digest(app,child_meta);
    check(!app.advance_title_background(title_update()).error.empty() && digest(app,child_meta)==child_before,
          "纪录栈顶不更新标题，拒绝时完整摘要不变");
    (void)app.record_view();(void)app.record_view();
    check.good(app.turn_record_page(1));
    check(app.capture_title_replay()->random.cursor==random_before+pool,"翻页和重画零抽");
    check.good(app.return_to_title());
    check(same_title(app.title_presentation(),title_before),"纪录返回保留完整标题q和计数");
    check.good(app.request_new_game(1));
    child_meta=metadata(app,100,104);const auto configured=digest(app,child_meta);
    check(!app.advance_title_background(title_update()).error.empty() && digest(app,child_meta)==configured,
          "配置页不准入标题背景");
    check.good(app.cancel_configuration());
    check(same_title(app.title_presentation(),title_before),"配置取消保留标题人物和出生间隔");
    check.good(app.request_new_game(1));
    const auto handoff=app.capture_title_replay()->random;
#ifdef _WIN32
    const auto start_meta=metadata(app,100,106);const auto locked_before=digest(app,start_meta);
    const auto retained_file=read(paths.system);
    const auto lock=CreateFileW(paths.system.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,
                               OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    check(lock!=INVALID_HANDLE_VALUE,"标题交接系统文件替换锁夹具");
    const auto error=app.start_game();
    const bool released=CloseHandle(lock)!=0;
    check(released,"释放标题交接替换锁");
    check(!error.empty() && digest(app,start_meta)==locked_before && read(paths.system)==retained_file,
          "系统写失败保留标题q、random、草稿、页面、旧纪录和空world");
#endif
    check.good(app.start_game());app.take_sound_requests();
    check(app.handoff_random() && app.handoff_random()->cursor==handoff.cursor &&
          app.handoff_random()->engine_state==handoff.engine_state &&
          app.world()->state().scene.random.snapshot().engine_state==handoff.engine_state &&
          app.world()->state().scene.random.draws()==handoff.cursor,
          "标题出生及纪录抽数实际交给新局引擎，不另设标题seed");
    const auto world_before=startup_world_session_digest(*app.world());
    check.good(app.return_to_title());
    const auto &reset=app.title_presentation();
    check(reset.l==0 && reset.s==0 && reset.t==0 && reset.f132f==title_before.f132f &&
          std::all_of(reset.slots.begin(),reset.slots.end(),[](const auto &s) {
              return s.active==0 && s.definition==0 && s.x==0 && s.y==0 && s.direction==0 && s.age==0;
          }),"从世界重入标题清q/l/s/t，保留同对象f132f");
    for(int i=0;i<100;++i)check.good(app.advance_title_background(title_update()).error);
    check(startup_world_session_digest(*app.world())==world_before && app.handoff_random()->cursor==handoff.cursor,
          "保留world时标题新抽4次不推进旧world或改历史handoff");
}

void title_natural_dual_restore(Checks &check,Work &work) {
    StartupApplication source(work.paths("title-natural-source"),ref::WorldRandomStream::from_java_seed(42),
                               StartupApplicationMode::title_presentation);
    std::uint64_t frame{},command{};int retired=-1;
    // 从全零真实初始化逐轮运行到首次非空退休槽，不注入q/age/随机或标题计数。
    for(;frame<700 && retired<0;) {
        check.good(source.advance_title_background(title_update()).error);++frame;++command;
        const auto &slots=source.title_presentation().slots;
        for(std::size_t i=0;i<slots.size();++i)
            if(!slots[i].active && slots[i].age>0 && slots[i].y>=210) { retired=static_cast<int>(i);break; }
    }
    check(retired>=0 && source.title_presentation().l>100 && source.title_presentation().f132f>100,
          "自然标题推进取得含inactive age/y的非零q捕获点");
    auto meta=metadata(source,frame,command);
    const auto before=digest(source,meta);
    const auto captured_title=source.title_presentation();
    const auto capture=work.directory("title-natural-capture");const auto file=capture/"title.avrapp";
    check.good(save_startup_application_replay(capture,file,source,meta,validate_driver));
    StartupApplication a(work.paths("title-natural-a-old"),ref::WorldRandomStream::from_java_seed(1),
                          StartupApplicationMode::title_presentation);
    StartupApplication b(work.paths("title-natural-b-old"),ref::WorldRandomStream::from_java_seed(2),
                          StartupApplicationMode::title_presentation);
    auto am=metadata(a,0,0),bm=metadata(b,0,0);
    check.good(restore_startup_application_replay(file,work.directory("title-natural-a"),controller,a,am,validate_driver));
    check.good(restore_startup_application_replay(file,work.directory("title-natural-b"),controller,b,bm,validate_driver));
    check(digest(a,am)==before && digest(b,bm)==before && same_title(a.title_presentation(),captured_title),
          "两恢复完整保存活跃和退休槽，不重初始化或补跑一次更新");
    bool reused{};
    for(int request=0;request<96;++request) {
        bool advanced=false;
        if(request==3) {check.good(a.open_records());check.good(b.open_records());}
        else if(request==4) {check.good(a.turn_record_page(1));check.good(b.turn_record_page(1));}
        else if(request==5) {check.good(a.return_to_title());check.good(b.return_to_title());}
        else if(request==6) {check.good(a.request_new_game(0));check.good(b.request_new_game(0));}
        else if(request==7) {check.good(a.cancel_configuration());check.good(b.cancel_configuration());}
        else {
            const auto x=a.advance_title_background(title_update(request==12));
            const auto y=b.advance_title_background(title_update(request==12));
            check.good(x.error);check.good(y.error);
            check(x.confirm_consumed==y.confirm_consumed && x.menu_confirm_ready==y.menu_confirm_ready &&
                  x.random_draws==y.random_draws,"双恢复逐请求确认资格及实际抽数相同");
            advanced=true;
        }
        if(advanced)++frame;
        ++command;
        am=metadata(a,frame,command);bm=metadata(b,frame,command);
        check(digest(a,am)==digest(b,bm),"双恢复逐步全应用与严格Driver摘要一致");
        check(same_plan(project_startup_title_presentation(a.title_presentation(),330),
                        project_startup_title_presentation(b.title_presentation(),330)),
              "双恢复逐步原序绘制请求含shadow/body完全一致");
        const auto &slot=a.title_presentation().slots[retired];
        if(slot.active && slot.age>=captured_title.slots[retired].age)reused=true;
    }
    check(reused && digest(source,meta)==before && same_title(source.title_presentation(),captured_title),
          "短回放自然复用退休槽并保留age，原捕获应用全程不变");
}
void title_cli_boundary(Checks &check,Work &work) {
    StartupApplication source(work.paths("title-cli-boundary-source"),ref::WorldRandomStream::from_java_seed(17),
                               StartupApplicationMode::title_presentation);
    check.good(source.advance_title_background(title_update(true)).error);
    const auto source_meta=title_metadata(1);
    const auto directory=work.directory("title-cli-wrong-boundary");const auto file=directory/"frame1.avrapp";
    check.good(save_startup_application_replay(directory,file,source,source_meta,validate_title_driver));
    StartupApplication target(work.paths("title-cli-boundary-old"),ref::WorldRandomStream::from_java_seed(9),
                               StartupApplicationMode::title_presentation);
    auto target_meta=title_metadata(0);
    const auto before=startup_application_replay_digest(target,target_meta,validate_title_driver);
    const auto original=read(file);const auto isolated=work.directory("title-cli-boundary-rejected");
    const auto error=restore_startup_application_replay(file,isolated,"title-background-requests-v2",target,
                                                        target_meta,validate_title_restore_driver);
    check(error=="应用回放Driver拒绝：标题CLI只重放认证600轮边界" &&
          startup_application_replay_digest(target,target_meta,validate_title_driver)==before && read(file)==original,
          "同controller合法但非600轮候选在联合安装前拒绝，旧应用/Driver/输入保持");
    fresh_targets(check,isolated);
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
    wire=decoded;set_number(wire.prefix,12,4,5);reject(pack(wire),"未知应用语义版本拒绝");
    wire=decoded;set_number(wire.prefix,12,4,1);reject(pack(wire),"旧应用语义1不静默补零标题q");
    wire=decoded;set_number(wire.prefix,12,4,2);reject(pack(wire),"旧应用语义2缺世界缓存收尾，不静默接续");
    wire=decoded;set_number(wire.prefix,12,4,3);reject(pack(wire),"旧应用语义3缺初始任务池，不静默接续");
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
    // 独立解析到标题尾部，验证固定124个i32布局后再损坏，避免靠尾长猜错偏移。
    wire=decoded;
    const auto &source_control=section(wire,3).bytes;
    std::size_t title_at=control_offsets(source_control).world;
    for(int i=0;i<4;++i)require(number(source_control,title_at,4)==0,"源无world/rows/clear/id载荷");
    require(source_control.size()-title_at==124U*4U,"语义2完整标题字段尾部");
    wire=decoded;set_number(section(wire,3).bytes,title_at+16U+19U*24U,4,2);
    reject(pack(wire),"重签末槽active非法仍由标题语义校验拒绝");
    wire=decoded;section(wire,3).bytes.resize(section(wire,3).bytes.size()-4);
    reject(pack(wire),"重签控制分区截断最后age字段仍拒绝，不补零");
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
    wire=decoded;set_number(section(wire,3).bytes,0,4,0);
    set_number(section(wire,3).bytes,title_at,4,1);set_number(section(wire,3).bytes,title_at+4,4,1);
    const auto logic_file=directory/"logic-with-title.avrapp";const auto logic_bytes=pack(wire);write(logic_file,logic_bytes);
    const auto logic_isolated=work.directory("logic-with-title-rejected");
    check(!restore_startup_application_replay(logic_file,logic_isolated,controller,logic,logic_meta,validate_driver).empty() &&
          digest(logic,logic_meta)==logic_before && read(logic_file)==logic_bytes,
          "匹配logic调用者的重签合法非零标题状态仍拒绝，不借模式错误提前通过测试");
    fresh_targets(check,logic_isolated);
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
    title_owner_transactions(check,work);
    title_natural_dual_restore(check,work);
    title_cli_boundary(check,work);
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
    for(int i=0;i<100;++i) {check.good(app.advance_title_background(title_update()).error);++command;}
    check(app.title_presentation().slots[0].active==1 && startup_world_session_digest(*app.world())==world_digest,
          "保留world的标题经过真实100请求生人并分叉随机，旧world不变");
    roundtrip(check,work,"title-active-with-world",app,metadata(app,100,command),StartupApplicationMode::title_presentation);
    check.good(app.open_records());++command;
    check(startup_world_session_digest(*app.world())==world_digest,"保留世界的纪录请求只推进标题流");
    roundtrip(check,work,"records-with-world",app,metadata(app,100,command),StartupApplicationMode::title_presentation);
    corruption(check,work);
    return check.count;
}

namespace {
constexpr const char *title_controller="title-background-requests-v2";
constexpr const char *title_producer="title-background-cli-v2";
// 首轮确认、600轮捕获、680轮结束都是Driver协议，不能由恢复调用者另换策略。
Metadata title_metadata(std::uint64_t next_frame) {
    Metadata m;m.controller_id=title_controller;m.producer_revision=title_producer;
    m.next_frame=next_frame;m.next_command=next_frame;
    auto &b=m.controller_state;
    u32(b,2);u64(b,next_frame);u64(b,next_frame);u32(b,1);u32(b,1);u32(b,600);u32(b,680);
    return m;
}
std::string validate_title_driver(const StartupApplication &app,const Metadata &m) {
    if(m.controller_id!=title_controller || m.producer_revision!=title_producer ||
       m.next_frame>680 || m.next_command!=m.next_frame || !m.extensions.empty() ||
       m.controller_state!=title_metadata(m.next_frame).controller_state)
        return "标题Driver身份/序号/首轮输入/捕获边界/输出消费非法";
    const auto replay=app.capture_title_replay();
    if(!replay || replay->mode!=StartupApplicationMode::title_presentation ||
       app.page()!=StartupApplicationPage::title || app.world() || replay->requests!=0 ||
       !replay->decorations.empty() || replay->record_page!=0)
        return "标题Driver禁止世界、子页或纪录请求";
    const auto &s=app.title_presentation();
    if(s.f132f!=static_cast<int>(m.next_frame) ||
       s.l!=(m.next_frame?static_cast<int>(m.next_frame)+99:0))
        return "标题Driver请求轮数与首确认计数关系不符";
    return validate_startup_title_presentation(s);
}
std::string validate_title_restore_driver(const StartupApplication &app,const Metadata &m) {
    const auto error=validate_title_driver(app,m);
    if(!error.empty())return error;
    return m.next_frame==600?std::string{}:"标题CLI只重放认证600轮边界";
}
std::string title_trace(const StartupApplication &app,const Metadata &m,const StartupTitleApplyResult &r) {
    const auto plan=project_startup_title_presentation(app.title_presentation(),330);
    require(plan.error.empty(),"标题trace只消费合法投影");
    std::ostringstream out;
    out<<"{\"next_frame\":"<<m.next_frame<<",\"digest\":\""
       <<startup_application_replay_digest(app,m,validate_title_driver)<<"\",\"confirm_consumed\":"
       <<(r.confirm_consumed?"true":"false")<<",\"menu_confirm_ready\":"
       <<(r.menu_confirm_ready?"true":"false")<<",\"random_draws\":"<<r.random_draws<<",\"people\":[";
    bool first=true;
    for(const auto &p:plan.people) {
        if(!first)out<<',';
        first=false;
        out<<"{\"slot\":"<<p.slot<<",\"definition\":"<<p.definition<<",\"age\":"<<p.age
           <<",\"step\":"<<p.step<<",\"facing\":"<<p.facing<<",\"anchor\":["<<p.anchor[0]
           <<','<<p.anchor[1]<<"],\"layers\":["<<static_cast<int>(p.layers[0])<<','
           <<static_cast<int>(p.layers[1])<<"]}";
    }
    out<<"]}\n";return out.str();
}
StartupApplicationPaths title_paths(const fs::path &directory) {
    return {directory/"system.avr",{directory/"world0.avr",directory/"world1.avr"}};
}
} // namespace

int run_startup_title_replay_cli(int argc,const char **argv) {
    std::map<std::string,std::string> options;
    require(argc>1 && std::string(argv[1])=="title-background-replay-v2","标题CLI入口不符");
    for(int i=2;i<argc;i+=2) {
        require(i+1<argc,"标题CLI选项缺值");const std::string key=argv[i];
        require(key=="--work-dir" || key=="--trace-file" || key=="--save-file" || key=="--load-file",
                "标题CLI未知选项");
        require(options.emplace(key,argv[i+1]).second,"标题CLI重复选项");
        require(!options.at(key).empty(),"标题CLI空路径");
    }
    require(options.count("--work-dir") && options.count("--trace-file") &&
            (options.count("--save-file")!=options.count("--load-file")),"标题CLI模式/路径不完整");
    const auto root=fs::absolute(options.at("--work-dir"));
    const auto trace=fs::absolute(options.at("--trace-file"));
    const bool loading=options.count("--load-file")!=0;
    const auto snapshot=fs::absolute(options.at(loading?"--load-file":"--save-file"));
    const auto current=title_paths(root/"unused-current");
    const auto live=root/"application";
    // 复用既有路径预检，先核全部输入/输出再创建任何本轮子目录。
    (void)persistence_detail::prepare_replay_capture_paths(root,root/".title-cli-probe",current,{trace,snapshot});
    const auto trace_target=persistence_detail::prepare_replay_capture_paths(root.parent_path(),trace,current,{snapshot});
    if(loading)
        (void)persistence_detail::prepare_replay_restore_paths(root,snapshot,current,{trace});
    else
        (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(),snapshot,current,{trace});
    require(fs::create_directory(live),"标题CLI独占空应用目录");
    StartupApplication app(loading?current:title_paths(live),ref::WorldRandomStream::from_java_seed(17),
                           StartupApplicationMode::title_presentation);
    require(app.error().empty(),"标题CLI初始应用拒绝");
    auto meta=title_metadata(0);
    if(loading) {
        const auto error=restore_startup_application_replay(snapshot,live,title_controller,app,meta,
                                                            validate_title_restore_driver,{trace});
        if(!error.empty())throw std::runtime_error(error);
    }
    if(const auto error=validate_title_driver(app,meta);!error.empty())throw std::runtime_error(error);
    std::ostringstream output;
    while(meta.next_frame<680) {
        const auto applied=app.advance_title_background(title_update(meta.next_frame==0));
        if(!applied.error.empty())throw std::runtime_error(applied.error);
        meta=title_metadata(meta.next_frame+1);
        output<<title_trace(app,meta,applied);
        require(static_cast<bool>(output),"标题CLI输出必须已消费");
        if(!loading && meta.next_frame==600) {
            const auto error=save_startup_application_replay(root.parent_path(),snapshot,app,meta,
                                                             validate_title_driver,{trace});
            if(!error.empty())throw std::runtime_error(error);
        }
    }
    (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(),trace,title_paths(live),{snapshot});
    const auto bytes=output.str();
    persistence_detail::create_save_file(trace_target.container,Bytes(bytes.begin(),bytes.end()));
    std::cout<<"title-background-replay next_frame="<<meta.next_frame<<" digest="
             <<startup_application_replay_digest(app,meta,validate_title_driver)<<" people="
             <<project_startup_title_presentation(app.title_presentation(),330).people.size()<<" draws="
             <<app.capture_title_replay()->random.cursor<<'\n';
    return 0;
}
