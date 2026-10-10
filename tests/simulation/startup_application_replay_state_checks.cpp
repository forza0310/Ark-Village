#include "ark/simulation/application/startup_application_replay.hpp"
#include "startup_application_replay_wire.hpp"
#include "../../src/simulation/application/startup_application_replay_paths.hpp"
#include "../../src/simulation/persistence/startup_world_file_io.hpp"
#include "../../src/simulation/application/startup_application_storage_replay.hpp"
#include "../../src/simulation/application/startup_application_storage_paths.hpp"
#include "../../src/simulation/persistence/startup_persistence_bytes.hpp"
#include "../../src/simulation/persistence/startup_world_codec.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <iostream>
#include <map>
#include <sstream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
using namespace ark::simulation;
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
        return {dir};
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
    if(page==StartupApplicationPage::overwrite)return 7U;
    if(page==StartupApplicationPage::world && app.world())return 3U;
    throw std::runtime_error("应用状态Driver不注册该页与世界组合");
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
    check(!fs::exists(dir),"拒绝/准备不提前创建恢复根或部分文件");
}
std::string digest(const StartupApplication &app,const Metadata &meta) {
    try {return startup_application_replay_digest(app,meta,validate_driver);}
    catch(const std::exception &e) {
        throw std::runtime_error("状态摘要（phase="+std::to_string(phase(app))+",frame="+
            std::to_string(meta.next_frame)+",command="+std::to_string(meta.next_command)+"）："+e.what());
    }
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
    exhausted.take_audio_requests();
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
    bad=*exhausted.capture_title_replay();bad.controller="startup-title-v2";
    check(!exhausted.restore_title_replay(bad).empty() && digest(exhausted,meta)==before,
          "不静默迁移没有菜单栈和目录身份的标题v2");
    StartupApplication logic(work.paths("title-logic-refuse"),ref::WorldRandomStream::from_java_seed(4));
    logic.take_audio_requests();
    const auto lm=metadata(logic,0,0);const auto lb=digest(logic,lm);
    check(!logic.advance_title_background(title_update(true)).error.empty() && digest(logic,lm)==lb,
          "logic模式显式拒绝标题背景更新，保持旧世界黄金随机资格");

    const auto paths=work.paths("title-owner-source");
    check.good(save_startup_system_file((paths.root/"system.avr"),{}));
    StartupApplication app(paths,ref::WorldRandomStream::from_java_seed(42),StartupApplicationMode::title_presentation);
    app.take_audio_requests();
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
    check.good(app.return_to_title());app.take_audio_requests();
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
    const auto retained_file=read((paths.root/"system.avr"));
    const auto lock=CreateFileW((paths.root/"system.avr").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,
                               OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    check(lock!=INVALID_HANDLE_VALUE,"标题交接系统文件替换锁夹具");
    const auto error=app.start_game();
    const bool released=CloseHandle(lock)!=0;
    check(released,"释放标题交接替换锁");
    check(!error.empty() && digest(app,start_meta)==locked_before && read((paths.root/"system.avr"))==retained_file,
          "系统写失败保留标题q、random、草稿、页面、旧纪录和空world");
#endif
    check.good(app.start_game());app.take_sound_requests();
    check(app.handoff_random() && app.handoff_random()->cursor==handoff.cursor &&
          app.handoff_random()->engine_state==handoff.engine_state &&
          app.world()->state().scene.random.snapshot().engine_state==handoff.engine_state &&
          app.world()->state().scene.random.draws()==handoff.cursor,
          "标题出生及纪录抽数实际交给新局引擎，不另设标题seed");
    const auto world_before=startup_world_session_digest(*app.world());
    check.good(app.return_to_title());app.take_audio_requests();
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
    source.take_audio_requests();
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
    a.take_audio_requests();
    StartupApplication b(work.paths("title-natural-b-old"),ref::WorldRandomStream::from_java_seed(2),
                          StartupApplicationMode::title_presentation);
    b.take_audio_requests();
    auto am=metadata(a,0,0),bm=metadata(b,0,0);
    const auto old_a=work.root/"title-natural-a-old",old_b=work.root/"title-natural-b-old";
    const auto old_a_count=std::distance(fs::directory_iterator(old_a),fs::directory_iterator{});
    const auto old_b_count=std::distance(fs::directory_iterator(old_b),fs::directory_iterator{});
    const auto restored_a=work.root/"title-natural-a",restored_b=work.root/"title-natural-b";
    check.good(restore_startup_application_replay(file,restored_a,controller,a,am,validate_driver));
    check(fs::is_regular_file(restored_a/"system.avr") && fs::is_directory(restored_a/"worlds") &&
          fs::is_empty(restored_a/"worlds") && !fs::exists(old_a/"system.avr") &&
          std::distance(fs::directory_iterator(old_a),fs::directory_iterator{})==old_a_count,
          "标题自然恢复A须发布精确新根，旧应用目录不变");
    check.good(restore_startup_application_replay(file,restored_b,controller,b,bm,validate_driver));
    check(fs::is_regular_file(restored_b/"system.avr") && fs::is_directory(restored_b/"worlds") &&
          fs::is_empty(restored_b/"worlds") && !fs::exists(old_b/"system.avr") &&
          std::distance(fs::directory_iterator(old_b),fs::directory_iterator{})==old_b_count,
          "标题自然恢复B须发布精确新根，旧应用目录不变");
    check(digest(a,am)==before && digest(b,bm)==before && same_title(a.title_presentation(),captured_title),
          "两恢复完整保存活跃和退休槽，不重初始化或补跑一次更新");
    bool reused{};
    for(int request=0;request<96;++request) {
        bool advanced=false;
        if(request==3) {check.good(a.open_records());check.good(b.open_records());}
        else if(request==4) {check.good(a.turn_record_page(1));check.good(b.turn_record_page(1));}
        else if(request==5) {check.good(a.return_to_title());a.take_audio_requests();check.good(b.return_to_title());b.take_audio_requests();}
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
    source.take_audio_requests();
    check.good(source.advance_title_background(title_update(true)).error);
    const auto source_meta=title_metadata(1);
    const auto directory=work.directory("title-cli-wrong-boundary");const auto file=directory/"frame1.avrapp";
    check.good(save_startup_application_replay(directory,file,source,source_meta,validate_title_driver));
    StartupApplication target(work.paths("title-cli-boundary-old"),ref::WorldRandomStream::from_java_seed(9),
                               StartupApplicationMode::title_presentation);
    target.take_audio_requests();
    auto target_meta=title_metadata(0);
    const auto before=startup_application_replay_digest(target,target_meta,validate_title_driver);
    const auto original=read(file);const auto isolated=work.root/"title-cli-boundary-rejected";
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
    target.take_audio_requests();
    check.good(target.error());
    auto target_meta=metadata(target,0,0);
    const auto isolated=work.root/(name+"-restore");
    check.good(restore_startup_application_replay(source,isolated,controller,target,target_meta,validate_driver));
    check(digest(target,target_meta)==before && same_metadata(meta,target_meta),
          "完整应用与规范Driver/未知可选段顺序及字节完全往返");
    std::set<std::array<std::uint8_t,32>> hashes;
    for(const auto &slot:app.records().save_directory)for(const auto &entry:slot)
        if(entry.reference)hashes.insert(entry.reference->sha256);
    if(!fs::is_regular_file(isolated/"system.avr")) {
        std::error_code ec;
        const auto status=fs::symlink_status(isolated,ec);
        std::ostringstream listing;
        listing<<"expected="<<isolated.u8string()<<",type="<<static_cast<int>(status.type())
               <<",error="<<ec.value()<<":"<<ec.message();
        check(false,name+"：已发布system须为普通文件；"+listing.str());
    }
    check(fs::is_regular_file(isolated/"system.avr"),name+"：已发布system为普通文件");
    check(fs::is_directory(isolated/"worlds"),name+"：已发布worlds须为目录："+isolated.u8string());
    std::ostringstream actual_files;
    std::size_t actual_count{};
    for(const auto &entry:fs::directory_iterator(isolated/"worlds")) {
        ++actual_count;actual_files<<entry.path().filename().u8string()<<":"
            <<static_cast<int>(entry.symlink_status().type())<<";";
    }
    check(actual_count==hashes.size(),name+"：恢复blob闭包数量不符，expected="+
          std::to_string(hashes.size())+",actual="+std::to_string(actual_count)+",entries="+actual_files.str());
    const auto preserved_source=read(source);
    check(preserved_source==original,name+"：源容器原字节改变，expected_sha="+
          ark::assets::sha256_hex(original)+",actual_sha="+ark::assets::sha256_hex(preserved_source));
    for(const auto &hash:hashes) {
        const auto hex=persistence_detail::storage_hash_hex(hash);
        check(ark::assets::sha256_hex(read(isolated/"worlds"/("world-"+hex+".avrs")))==hex,
              "恢复各blob逐字节身份相同，不解码重编码损失正常档字段");
    }
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
        if(name=="world-after-handoff") {
            auto bad=unpack(original);
            auto &nested=section(bad,4).bytes;
            set_number(nested,12,4,3);resign(nested);
            const auto bad_bytes=pack(bad);
            const auto old_meta=target_meta;
            const auto bad_path=directory/"nested-world3.avrapp";
            write(bad_path,bad_bytes);
            const auto rejected=work.root/"nested-world3-rejected";
            const auto error=restore_startup_application_replay(
                bad_path,rejected,controller,target,target_meta,validate_driver);
            check(error.find("不支持的存档或状态语义版本")!=std::string::npos,
                  "当前应用中section4世界3全重签后由世界语义守卫拒绝："+error);
            check(digest(target,target_meta)==before && same_metadata(target_meta,old_meta) &&
                  read(source)==original && read(bad_path)==bad_bytes,
                  "旧嵌套世界拒绝不改变应用Driver与新旧来源");
            fresh_targets(check,rejected);
        }
    }
    // 未移交前旧应用路径仍未被创建，恢复后写入只作用于新隔离系统。
    check(!fs::exists((target_paths.root/"system.avr")) && !fs::exists((target_paths.root/"worlds")) &&
          !fs::exists((target_paths.root/"world0.avr")),"恢复没有写旧应用路径");
}
// 四个正常存档使用各自合法新世界；隐藏项仍属引用闭包，不注入游戏进度。
void directory_view_roundtrip(Checks &check,Work &work) {
    namespace detail=persistence_detail;
    const auto source_paths=work.paths("four-slot-source");
    auto loaded=load_startup_application_storage(source_paths.root);
    check(loaded.snapshot.has_value(),loaded.error);
    auto snapshot=std::move(*loaded.snapshot);
    for(int slot=0;slot<2;++slot)for(int kind=0;kind<2;++kind) {
        StartupSession reset;
        StartupWorldRuntimeSession world(reset.state(),ref::WorldRandomStream::from_java_seed(100+slot*2+kind));
        world.take_audio_requests();
        auto saved=save_startup_application_slot(source_paths.root,snapshot,snapshot.records,
                                                slot,static_cast<StartupSaveKind>(kind),world);
        check(saved.snapshot.has_value(),saved.error);snapshot=std::move(*saved.snapshot);
    }
    auto hidden=hide_startup_application_slot(source_paths.root,snapshot,snapshot.records,1,StartupSaveKind::manual);
    check(hidden.snapshot.has_value(),hidden.error);snapshot=std::move(*hidden.snapshot);
    auto captured=capture_startup_application_storage(source_paths.root,snapshot);
    check(captured.view && captured.view->blobs.size()==4,"四独立正常档保留四唯一blob，包括日期-1隐藏手动档");
    StartupApplication app(source_paths,ref::WorldRandomStream::from_java_seed(5),StartupApplicationMode::title_presentation);
    app.take_audio_requests();check.good(app.error());
    roundtrip(check,work,"four-slots-hidden",app,metadata(app,0,0),StartupApplicationMode::title_presentation);
    check.good(app.request_new_game(0));
    check(app.page()==StartupApplicationPage::overwrite && app.title_menu().save_menu && app.title_menu().confirmation,
          "实际重新开始请求建立raw20父页和默认否询问");
    roundtrip(check,work,"four-slots-confirmation",app,metadata(app,0,1),StartupApplicationMode::title_presentation);
    StartupTitleMenuRequest returned;returned.kind=StartupTitleMenuRequestKind::keys;returned.keys.confirm=true;
    check.good(app.apply_title_request(app.title_page_id(),returned));
    check(app.title_menu().confirmation && app.title_menu().confirmation->returned,
          "真实确认默认否后保留等待父消费的询问载荷");
    roundtrip(check,work,"four-slots-returned",app,metadata(app,0,2),StartupApplicationMode::title_presentation);
    returned={};returned.kind=StartupTitleMenuRequestKind::consume_return;
    check.good(app.apply_title_request(app.title_page_id(),returned));
    roundtrip(check,work,"four-slots-raw20",app,metadata(app,0,3),StartupApplicationMode::title_presentation);

    // 成功解一个blob后已消耗的节点不得给后面的blob重新发放。
    detail::CodecDecodeBudget single;
    const auto original_nodes=single.nodes_remaining;
    (void)detail::decode_world_session_bytes(captured.view->blobs.front().bytes,startup_world_rules(),
                                            StartupWorldSavePurpose::normal,"",single);
    detail::CodecDecodeBudget joint;
    joint.nodes_remaining=2*(original_nodes-single.nodes_remaining);
    bool refused=false;
    try {detail::validate_application_replay_storage_view(*captured.view,joint);}
    catch(const std::runtime_error &) {refused=true;}
    check(refused,"共享两份世界的节点预算不能解过四唯一blob，不得逐档重置预算");

    const auto before_files=std::distance(fs::directory_iterator(work.root),fs::directory_iterator{});
    const auto target=work.root/"publish-cancelled";
    refused=false;bool callback_reached=false;
    try {detail::publish_application_replay_storage(target,*captured.view,[&]{
        callback_reached=true;
        throw std::runtime_error("测试发布前拒绝");
    });} catch(const std::runtime_error &) {refused=true;}
    check(refused && callback_reached && !fs::exists(target) &&
          std::distance(fs::directory_iterator(work.root),fs::directory_iterator{})==before_files,
          "四blob暂存完成后拒绝仍回收本轮临时根，未发布任何部分目标");

    const auto capture_dir=work.directory("blob-corruption");
    const auto source=capture_dir/"source.avrapp";const auto meta=metadata(app,0,3);
    check.good(save_startup_application_replay(capture_dir,source,app,meta,validate_driver));
    auto wire=unpack(read(source));
    const auto before=digest(app,meta);
    auto target_meta=meta;
    const auto reject=[&](application_replay_fixture::Wire bad,const char *name,
                          const char *expected_error=nullptr) {
        const auto path=capture_dir/(std::string(name)+".avrapp");write(path,pack(bad));
        const auto root=work.root/(std::string("blob-bad-")+name);
        const auto error=restore_startup_application_replay(path,root,controller,app,target_meta,validate_driver);
        check(!error.empty() && (!expected_error || error.find(expected_error)!=std::string::npos),
              std::string(name)+"："+error);
        check(digest(app,target_meta)==before && !fs::exists(root),"槽位坏载荷拒绝保留旧应用和完整文件视图");
    };
    auto bad=wire;section(bad,6).bytes.back()^=1;reject(std::move(bad),"blob-hash");
    bad=wire;set_number(section(bad,6).bytes,0,4,0);section(bad,6).bytes.resize(4);reject(std::move(bad),"missing-hidden-and-visible");
    bad=wire;set_number(section(bad,6).bytes,4+32+8,4,2);reject(std::move(bad),"wrong-purpose");
    bad=wire;
    auto &view_bytes=section(bad,6).bytes;
    const auto &hidden_hash=snapshot.records.save_directory[1][1].reference->sha256;
    std::size_t at=4;bool removed=false;
    for(int i=0;i<4;++i) {
        const auto start=at;
        require(at+44<=view_bytes.size(),"独立blob夹具头完整");
        const bool is_hidden=std::equal(hidden_hash.begin(),hidden_hash.end(),view_bytes.begin()+static_cast<std::ptrdiff_t>(at));
        at+=32;const auto size=number(view_bytes,at,8);(void)number(view_bytes,at,4);
        require(size<=view_bytes.size()-at,"独立blob夹具体完整");at+=static_cast<std::size_t>(size);
        if(is_hidden) {
            view_bytes.erase(view_bytes.begin()+static_cast<std::ptrdiff_t>(start),view_bytes.begin()+static_cast<std::ptrdiff_t>(at));
            set_number(view_bytes,0,4,3);removed=true;break;
        }
    }
    require(removed,"找到真实隐藏目录引用对应blob");reject(std::move(bad),"missing-hidden-only");

    const auto &control=section(wire,3).bytes;
    at=control_offsets(control).world;
    for(int i=0;i<4;++i)require(number(control,at,4)==0,"四槽标题夹具没有活动world/计分");
    at+=124U*4U; // 标题背景字段之后逐项确认菜单布局，避免靠容器尾长猜偏移。
    (void)number(control,at,4);(void)number(control,at,4);(void)number(control,at,4);
    (void)number(control,at,8);(void)number(control,at,8);
    require(number(control,at,4)==1,"该夹具确有raw20");
    const auto raw20_id=at;(void)number(control,at,8);const auto raw20_parent=at;
    (void)number(control,at,8);const auto raw20_slot=at;
    for(int i=0;i<5;++i)(void)number(control,at,4);
    require(number(control,at,4)==1,"该raw20有目录stamp");
    const auto revision=at;(void)number(control,at,8);const auto hash_at=at;
    bad=wire;set_number(section(bad,3).bytes,raw20_id,8,0);reject(std::move(bad),"raw20-zero-id");
    bad=wire;set_number(section(bad,3).bytes,raw20_parent,8,999);reject(std::move(bad),"raw20-parent");
    bad=wire;set_number(section(bad,3).bytes,raw20_slot,4,1);reject(std::move(bad),"raw20-draft-slot");
    bad=wire;set_number(section(bad,3).bytes,revision,8,0);reject(std::move(bad),"raw20-stale-revision");
    bad=wire;section(bad,3).bytes[hash_at]^=1;reject(std::move(bad),"raw20-stale-hash");

    // 隐藏normal槽仍须核世界语义；同步重签blob、系统引用、目录stamp和外容器，
    // 避免只触发摘要不符或raw20陈旧守卫，误称旧世界已经拒绝。
    bad=wire;
    const auto binary_hash=[](const Bytes &bytes) {
        const auto hex=ark::assets::sha256_hex(bytes);
        const auto digit=[](char c){return c<='9'?c-'0':c-'a'+10;};
        std::array<std::uint8_t,32> result{};
        for(std::size_t i=0;i<result.size();++i)
            result[i]=static_cast<std::uint8_t>(digit(hex[2*i])*16+digit(hex[2*i+1]));
        return result;
    };
    auto blobs=captured.view->blobs;
    const auto old_blob=std::find_if(blobs.begin(),blobs.end(),[&](const auto &blob) {
        return blob.reference.sha256==hidden_hash;
    });
    require(old_blob!=blobs.end(),"隐藏槽旧语义夹具找到唯一normal源");
    set_number(old_blob->bytes,12,4,3);resign(old_blob->bytes);
    old_blob->reference.sha256=binary_hash(old_blob->bytes);
    const auto replacement=old_blob->reference.sha256;
    std::sort(blobs.begin(),blobs.end(),[](const auto &a,const auto &b) {
        return a.reference.sha256<b.reference.sha256;
    });
    auto &storage=section(bad,6).bytes;storage.clear();u32(storage,static_cast<std::uint32_t>(blobs.size()));
    for(const auto &blob:blobs) {
        storage.insert(storage.end(),blob.reference.sha256.begin(),blob.reference.sha256.end());
        u64(storage,blob.bytes.size());u32(storage,static_cast<std::uint32_t>(blob.reference.purpose));
        storage.insert(storage.end(),blob.bytes.begin(),blob.bytes.end());
    }
    auto &old_system_bytes=section(bad,2).bytes;
    std::size_t pos=8;
    require(number(old_system_bytes,pos,4)==2,"旧世界normal夹具保持系统2");
    (void)text(old_system_bytes,pos);
    const auto parts=number(old_system_bytes,pos,4);
    int replaced{};
    for(std::uint64_t i=0;i<parts;++i) {
        const auto id=number(old_system_bytes,pos,4);
        (void)number(old_system_bytes,pos,4);(void)number(old_system_bytes,pos,4);
        const auto length=number(old_system_bytes,pos,8);
        const auto hash_offset=pos;pos+=64;const auto payload=pos;
        require(payload<=old_system_bytes.size()-64 && length<=old_system_bytes.size()-64-payload,
                "旧世界normal系统分区范围完整");
        if(id==3) {
            auto entry=payload;(void)number(old_system_bytes,entry,8);
            for(int index=0;index<4;++index) {
                const auto date=number(old_system_bytes,entry,4);
                (void)text(old_system_bytes,entry);(void)number(old_system_bytes,entry,8);
                require(number(old_system_bytes,entry,4)==1 && entry+44<=payload+length,
                        "四个真实normal引用均存在");
                if(std::equal(hidden_hash.begin(),hidden_hash.end(),
                              old_system_bytes.begin()+static_cast<std::ptrdiff_t>(entry))) {
                    require(index==3 && date==UINT32_MAX,"只替换slot1隐藏手动目录引用");
                    std::copy(replacement.begin(),replacement.end(),
                              old_system_bytes.begin()+static_cast<std::ptrdiff_t>(entry));
                    ++replaced;
                }
                entry+=44;
            }
            const Bytes payload_bytes(old_system_bytes.begin()+static_cast<std::ptrdiff_t>(payload),
                                      old_system_bytes.begin()+static_cast<std::ptrdiff_t>(payload+length));
            const auto hash=ark::assets::sha256_hex(payload_bytes);
            std::copy(hash.begin(),hash.end(),old_system_bytes.begin()+static_cast<std::ptrdiff_t>(hash_offset));
        }
        pos=payload+static_cast<std::size_t>(length);
    }
    require(replaced==1 && pos==old_system_bytes.size()-64,"只替换一个隐藏引用，保留所有系统尾段");
    resign(old_system_bytes);
    const auto replacement_records=detail::decode_system_records_bytes(old_system_bytes);
    require(replacement_records.save_directory[1][1].packed_date==-1 &&
            replacement_records.save_directory[1][1].reference->sha256==replacement,
            "重签系统引用可独立解码，隐藏资格不变");
    const auto stamp=binary_hash(old_system_bytes);
    std::copy(stamp.begin(),stamp.end(),section(bad,3).bytes.begin()+static_cast<std::ptrdiff_t>(hash_at));
    reject(std::move(bad),"hidden-normal-world3","不支持的存档或状态语义版本");

    // 独立改系统目录可见性，再重签内外摘要及raw20 stamp；不能让旧hash拒绝掩盖新关系守卫。
    bad=wire;
    auto &system=section(bad,2).bytes;
    std::size_t system_at=8;
    require(number(system,system_at,4)==2,"隐藏目录夹具使用系统版本2");
    (void)text(system,system_at);
    const auto section_count=number(system,system_at,4);
    bool changed_visibility=false;
    for(std::uint64_t i=0;i<section_count;++i) {
        const auto id=number(system,system_at,4);
        (void)number(system,system_at,4);(void)number(system,system_at,4);
        const auto length=number(system,system_at,8);
        const auto hash_offset=system_at;system_at+=64;
        const auto payload=system_at;
        require(length<=system.size()-payload-64,"系统目录分区独立定位边界");
        if(id==3) {
            auto directory_at=payload;
            const auto directory_revision=number(system,directory_at,8);
            require(directory_revision==snapshot.records.revision,"修改日期不修改系统修订");
            for(int index=0;index<2;++index) {
                const auto date_at=directory_at;
                const auto date=number(system,directory_at,4);
                (void)text(system,directory_at);(void)number(system,directory_at,8);
                require(number(system,directory_at,4)==1,"选中slot0两行均有真实blob引用");
                require(directory_at+44<=payload+length,"引用保持完整");directory_at+=44;
                if(index==1) {
                    require(date!=UINT32_MAX,"源raw20手动行确实可见");
                    set_number(system,date_at,4,UINT32_MAX);changed_visibility=true;
                }
            }
            const Bytes payload_bytes(system.begin()+static_cast<std::ptrdiff_t>(payload),
                                      system.begin()+static_cast<std::ptrdiff_t>(payload+length));
            const auto hash=ark::assets::sha256_hex(payload_bytes);
            std::copy(hash.begin(),hash.end(),system.begin()+static_cast<std::ptrdiff_t>(hash_offset));
        }
        system_at=payload+static_cast<std::size_t>(length);
    }
    require(changed_visibility && system_at==system.size()-64,"只修改选中手动目录日期，保留全部分区");
    resign(system);
    const auto hidden_records=detail::decode_system_records_bytes(system);
    check(hidden_records.save_directory[0][1].packed_date==-1 &&
              hidden_records.save_directory[0][1].reference==snapshot.records.save_directory[0][1].reference,
          "重签系统自身有效且隐藏仍保留原blob，不由坏格式触发拒绝");
    const auto system_hash=ark::assets::sha256_hex(system);
    auto &hidden_control=section(bad,3).bytes;
    const auto nibble=[](char c){return c<='9'?c-'0':c-'a'+10;};
    for(std::size_t i=0;i<32;++i)
        hidden_control[hash_at+i]=static_cast<std::uint8_t>(nibble(system_hash[2*i])*16+nibble(system_hash[2*i+1]));
    set_number(hidden_control,revision,8,hidden_records.revision);
    const auto hidden_path=capture_dir/"raw20-hidden-correct-stamp.avrapp";
    const auto hidden_bytes=pack(bad);write(hidden_path,hidden_bytes);
    const auto hidden_target=work.root/"raw20-hidden-correct-stamp-target";
    std::set<fs::path> prior_entries;
    for(const auto &item:fs::directory_iterator(work.root))prior_entries.insert(item.path().filename());
    const auto old_system=read(source_paths.root/"system.avr");
    const auto old_source=read(source);
    const auto error=restore_startup_application_replay(hidden_path,hidden_target,controller,app,target_meta,validate_driver);
    check(error.find("raw20目录")!=std::string::npos,
          "系统与stamp都有效时仍在raw20目录可见性关系守卫拒绝："+error);
    std::set<fs::path> after_entries;
    for(const auto &item:fs::directory_iterator(work.root))after_entries.insert(item.path().filename());
    check(!fs::exists(hidden_target) && after_entries==prior_entries &&
              digest(app,target_meta)==before && same_metadata(target_meta,meta) &&
              read(source_paths.root/"system.avr")==old_system && read(source)==old_source &&
              read(hidden_path)==hidden_bytes,
          "隐藏raw20坏恢复不发布根/暂存文件，不改旧应用/Driver/系统/来源");
}
void corruption(Checks &check,Work &work) {
    StartupApplication source_app(work.paths("corrupt-source-app"),ref::WorldRandomStream::from_java_seed(44),
                                  StartupApplicationMode::title_presentation);
    source_app.take_audio_requests();
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
    check.good(save_startup_system_file((current_paths.root/"system.avr"),initial_records));
    StartupApplication target(current_paths,ref::WorldRandomStream::from_java_seed(55),
                               StartupApplicationMode::title_presentation);
    target.take_audio_requests();
    check.good(target.error());
    check.good(target.request_new_game(1));check.good(target.edit_main_name("原目标草稿"));
    auto target_meta=metadata(target,0,2);
    const auto current_bytes=read((current_paths.root/"system.avr"));
    const auto before=digest(target,target_meta);
    const auto meta_before=target_meta;
    int index{};
    const auto reject=[&](const Bytes &bytes,const char *reason) {
        const auto file=directory/("bad-"+std::to_string(index)+".avrapp");
        const auto isolated=work.root/("bad-target-"+std::to_string(index++));
        write(file,bytes);
        const auto error=restore_startup_application_replay(file,isolated,controller,target,target_meta,validate_driver);
        check(!error.empty(),reason);
        check(digest(target,target_meta)==before && same_metadata(target_meta,meta_before) &&
              read((current_paths.root/"system.avr"))==current_bytes && read(file)==bytes,
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
    wire=decoded;set_number(wire.prefix,12,4,8);reject(pack(wire),"未知应用语义版本拒绝");
    wire=decoded;set_number(wire.prefix,12,4,1);reject(pack(wire),"旧应用语义1不静默补零标题q");
    wire=decoded;set_number(wire.prefix,12,4,2);reject(pack(wire),"旧应用语义2缺世界缓存收尾，不静默接续");
    wire=decoded;set_number(wire.prefix,12,4,3);reject(pack(wire),"旧应用语义3缺初始任务池，不静默接续");
    wire=decoded;set_number(wire.prefix,12,4,4);reject(pack(wire),"旧应用语义4缺声音操作和遭遇输出，不静默接续");
    wire=decoded;set_number(wire.prefix,12,4,5);reject(pack(wire),"旧应用语义5缺四目录和菜单栈，不静默接续");
    wire=decoded;set_number(wire.prefix,12,4,6);reject(pack(wire),"旧应用语义6可能已按过期邻接价格收费，不暗补历史");
    wire=decoded;set_number(wire.prefix,16,4,2);reject(pack(wire),"未知捕获边界版本拒绝");
    broken=original;set_number(broken,decoded.prefix.size(),4,67);resign(broken);
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
    require(source_control.size()-title_at==124U*4U+44U,"语义7保持完整标题字段及空菜单/空音频尾部布局");
    wire=decoded;set_number(section(wire,3).bytes,title_at+16U+19U*24U,4,2);
    reject(pack(wire),"重签末槽active非法仍由标题语义校验拒绝");
    wire=decoded;section(wire,3).bytes.resize(section(wire,3).bytes.size()-4);
    reject(pack(wire),"重签控制分区截断音频边界仍拒绝，不补零");
    wire=decoded;section(wire,3).bytes.resize(title_at+124U*4U-4U);
    reject(pack(wire),"末槽age与整个菜单被截断，不能默认填初始值");
    wire=decoded;set_number(section(wire,3).bytes,title_at+124U*4U,4,99);
    reject(pack(wire),"未知根菜单模式即使重签也拒绝");
    wire=decoded;set_number(section(wire,3).bytes,title_at+124U*4U+12U,8,0);
    reject(pack(wire),"根菜单稳定ID零值拒绝");
    wire=decoded;set_number(section(wire,3).bytes,section(wire,3).bytes.size()-4,4,1);
    reject(pack(wire),"重签应用音频未消费标志拒绝");
    wire=decoded;set_number(section(wire,6).bytes,0,4,5);
    reject(pack(wire),"槽位文件视图超过最多四个唯一blob拒绝");
    wire=decoded;wire.sections.erase(std::remove_if(wire.sections.begin(),wire.sections.end(),
        [](const auto &part){return part.id==6;}),wire.sections.end());
    reject(pack(wire),"即使无槽位引用也必须提供空文件视图段");
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

    const auto wrong_controller=work.root/"wrong-controller";
    check(!restore_startup_application_replay(source,wrong_controller,"not-this-controller",target,target_meta,validate_driver).empty(),
          "调用者期望controller不符拒绝");
    fresh_targets(check,wrong_controller);
    StartupApplication logic(work.paths("mode-logic"),ref::WorldRandomStream::from_java_seed(66));
    logic.take_audio_requests();
    auto logic_meta=metadata(logic,0,0);const auto logic_before=digest(logic,logic_meta);
    const auto wrong_mode=work.root/"wrong-mode";
    check(!restore_startup_application_replay(source,wrong_mode,controller,logic,logic_meta,validate_driver).empty() &&
          digest(logic,logic_meta)==logic_before,"应用回放不隐式切换调用者mode");
    fresh_targets(check,wrong_mode);
    wire=decoded;set_number(section(wire,3).bytes,0,4,0);
    set_number(section(wire,3).bytes,title_at,4,1);set_number(section(wire,3).bytes,title_at+4,4,1);
    const auto logic_file=directory/"logic-with-title.avrapp";const auto logic_bytes=pack(wire);write(logic_file,logic_bytes);
    const auto logic_isolated=work.root/"logic-with-title-rejected";
    check(!restore_startup_application_replay(logic_file,logic_isolated,controller,logic,logic_meta,validate_driver).empty() &&
          digest(logic,logic_meta)==logic_before && read(logic_file)==logic_bytes,
          "匹配logic调用者的重签合法非零标题状态仍拒绝，不借模式错误提前通过测试");
    fresh_targets(check,logic_isolated);
    const auto validator_failure=work.root/"validator-failure";
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
          read((current_paths.root/"system.avr"))==current_bytes && read(source)==original,
          "所有外围拒绝之后旧应用/Driver/系统/源完整不变");
}
} // namespace

int run_startup_application_replay_state_checks(const std::filesystem::path &parent) {
    Checks check;Work work(parent);
    const auto scenario=[&](const char *name,auto run) {
        try {run();}catch(const std::exception &e) {throw std::runtime_error(std::string(name)+"："+e.what());}
    };
    scenario("标题Owner事务",[&]{title_owner_transactions(check,work);});
    scenario("标题自然双恢复",[&]{title_natural_dual_restore(check,work);});
    scenario("标题CLI边界",[&]{title_cli_boundary(check,work);});
    scenario("四槽文件视图",[&]{directory_view_roundtrip(check,work);});
    StartupApplication app(work.paths("source-app"),ref::WorldRandomStream::from_java_seed(42),
                            StartupApplicationMode::title_presentation);
    app.take_audio_requests();
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
    check.good(app.return_to_title());app.take_audio_requests();++command;
    check.good(app.request_new_game(1));++command;
    check.good(app.start_game());++command;
    app.take_sound_requests();
    check(app.world() && app.handoff_random() && app.handoff_random()->cursor==title_random->random.cursor &&
          app.world()->state().scene.random.draws()==title_random->random.cursor,
          "合法开始操作交接真实标题随机；不是测试补写world或handoff");
    roundtrip(check,work,"world-after-handoff",app,metadata(app,0,command),StartupApplicationMode::title_presentation);
    check.good(app.return_to_title());app.take_audio_requests();++command;
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
    return {directory};
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
        (void)persistence_detail::prepare_replay_restore_paths(live,snapshot,current,{trace});
    else
        (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(),snapshot,current,{trace});
    require(fs::create_directory(loading?current.root:live),"标题CLI独占初始应用目录");
    StartupApplication app(loading?current:title_paths(live),ref::WorldRandomStream::from_java_seed(17),
                           StartupApplicationMode::title_presentation);
    app.take_audio_requests();
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
