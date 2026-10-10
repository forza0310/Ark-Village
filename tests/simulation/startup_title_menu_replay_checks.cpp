#include "ark/simulation/startup_application_replay.hpp"
#include "ark/assets/sha256.hpp"
#include "startup_application_replay_wire.hpp"
#include "startup_application_replay_paths.hpp"
#include "startup_world_file_io.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

using namespace ark::simulation;
namespace {
namespace fs=std::filesystem;
namespace wire=application_replay_fixture;
using Bytes=std::vector<std::uint8_t>;
using Metadata=StartupApplicationReplayMetadata;
constexpr const char *controller="title-menu-files-v1";
constexpr std::uint64_t terminal=17;
constexpr std::size_t budget=128U*1024U*1024U;
void need(bool ok,const std::string &message) {
    if(!ok) throw std::runtime_error("title menu file replay: "+message);
}
void good(const std::string &error) { need(error.empty(),error); }
std::string hash(const Bytes &bytes) { return ark::assets::sha256_hex(bytes); }
// Driver只表达此具名17步尾段；不引入通用菜单脚本或任意命令解释器。
struct Driver {
    std::uint64_t step{}, audio_count{3}, audio_peak{2};
    StartupSaveReference original;
};
Bytes encode(const Driver &d) {
    Bytes bytes;
    wire::u64(bytes,1); wire::u64(bytes,d.step);
    wire::u64(bytes,d.audio_count); wire::u64(bytes,d.audio_peak);
    wire::u64(bytes,1); wire::u64(bytes,0); wire::u64(bytes,terminal); // seed/speed/策略终点。
    bytes.insert(bytes.end(),d.original.sha256.begin(),d.original.sha256.end());
    wire::u64(bytes,d.original.bytes);
    wire::u64(bytes,static_cast<std::uint64_t>(d.original.purpose));
    return bytes;
}
Driver decode(const Bytes &bytes) {
    need(bytes.size()==104,"fixed driver size");
    std::size_t at{};
    const auto n=[&]{return wire::number(bytes,at,8);};
    need(n()==1,"driver version");
    Driver d; d.step=n(); d.audio_count=n(); d.audio_peak=n();
    need(n()==1 && n()==0 && n()==terminal,"driver seed/speed/terminal");
    std::copy_n(bytes.begin()+static_cast<std::ptrdiff_t>(at),32,d.original.sha256.begin()); at+=32;
    d.original.bytes=n(); const auto purpose=n();
    need(d.step<=terminal && d.audio_peak==2 && d.original.bytes>0 && d.original.bytes<=budget &&
         purpose==static_cast<std::uint64_t>(StartupWorldSavePurpose::normal),"driver range/reference purpose");
    d.original.purpose=StartupWorldSavePurpose::normal;
    const std::uint64_t sounds=3+(d.step>=11?1:0)+(d.step>=13?1:0)+(d.step>=17?1:0);
    need(d.audio_count==sounds && at==bytes.size() && encode(d)==bytes,"canonical consumed typed output count");
    return d;
}
Metadata metadata(const Driver &d) {
    Metadata m;
    m.controller_id=controller; m.producer_revision="title-menu-four-directory-application-v6";
    m.next_frame=d.step+1; m.next_command=d.step+1; m.controller_state=encode(d);
    return m;
}
bool menu_shape(const StartupTitleMenuState &s,bool returned,int selection,int result) {
    return s.save_menu && s.save_menu->returned==returned && s.save_menu->selection==selection &&
        s.save_menu->result==result && s.save_menu->slot==0 && s.save_menu->frame==0;
}
bool dialog_shape(const StartupTitleMenuState &s,bool returned,int selection,int result) {
    return s.confirmation && s.confirmation->returned==returned &&
        s.confirmation->selection==selection && s.confirmation->result==result &&
        s.confirmation->reason==StartupTitleConfirmationReason::hide;
}
std::string validate(const StartupApplication &app,const Metadata &m) {
    try {
        need(m.controller_id==controller && m.producer_revision=="title-menu-four-directory-application-v6" &&
             m.extensions.empty(),"metadata identity/extensions");
        const auto d=decode(m.controller_state);
        need(m.next_frame==d.step+1 && m.next_command==d.step+1,"driver position");
        need(app.error().empty() && app.mode()==StartupApplicationMode::logic && app.world() &&
             !app.has_pending_audio_requests() && app.world()->state().sound_requests.empty(),"healthy consumed boundary");
        const auto &world=app.world()->state();
        need(world.scene.random.draws()==0 && world.simulation_steps==0 &&
             world.scene.calendar.year==0 && world.scene.calendar.month==3 && world.scene.calendar.subperiod==0 &&
             world.scene.world.world.ai.accounting.funds()==5000 && app.draft().slot==0 &&
             app.records().cash_peak==0 && app.records().high_score==0 && app.records().last_slot==0,
             "real untouched new-game business and random");
        const auto &directory=app.records().save_directory;
        need(directory[0][0]==StartupSaveDirectoryEntry{} && directory[1][0]==StartupSaveDirectoryEntry{} &&
             directory[1][1]==StartupSaveDirectoryEntry{},"other three directories untouched");
        const auto &entry=directory[0][1];
        need(entry.reference && *entry.reference==d.original && entry.cash==5000 &&
             entry.village==app.draft().village && entry.packed_date==(d.step>=9 && d.step<12?-1:300),
             "manual visible/hidden identity and original immutable bytes");
        const std::uint64_t revision=2+(d.step>=9?1:0)+(d.step>=11?1:0)+(d.step>=12?1:0)+(d.step>=17?1:0);
        need(app.records().revision==revision,"exact successful file commit sequence");
        const auto &s=app.title_menu(); good(validate_startup_title_menu(s));
        bool shape{};
        switch(d.step) {
        case 0: shape=menu_shape(s,false,2,2)&&dialog_shape(s,false,1,-1); break;
        case 1: shape=menu_shape(s,false,2,2)&&dialog_shape(s,true,1,1); break;
        case 2: shape=menu_shape(s,false,2,2)&&!s.confirmation; break;
        case 3: shape=menu_shape(s,true,2,-1)&&!s.confirmation; break;
        case 4: case 9: case 14:
            shape=s.mode==StartupTitleRootMode::slots&&!s.save_menu&&!s.confirmation&&!s.external; break;
        case 5: case 15: shape=menu_shape(s,false,0,-1)&&!s.confirmation; break;
        case 6: shape=menu_shape(s,false,2,2)&&dialog_shape(s,false,1,-1); break;
        case 7: shape=menu_shape(s,false,2,2)&&dialog_shape(s,true,0,0); break;
        case 8: shape=menu_shape(s,true,2,2)&&!s.confirmation; break;
        case 10: shape=s.external&&s.external->kind==StartupTitleExternalPage::configure&&
                           !s.external->returned&&!s.save_menu&&!s.confirmation; break;
        case 11: case 12: case 17: shape=app.page()==StartupApplicationPage::world &&
                                              !s.save_menu&&!s.confirmation&&!s.external; break;
        case 13: shape=s.mode==StartupTitleRootMode::menu&&!s.save_menu&&!s.confirmation&&!s.external; break;
        case 16: shape=menu_shape(s,true,0,0)&&!s.confirmation; break;
        default: break;
        }
        need(shape,"exact menu phase/returned payload");
        if(d.step==0 || d.step==6) need(app.page()==StartupApplicationPage::overwrite,"dialog visible page");
        else if(d.step==10) need(app.page()==StartupApplicationPage::configure,"configuration visible page");
        else if(d.step!=11 && d.step!=12 && d.step!=17)
            need(app.page()==StartupApplicationPage::title,"title effective parent page");
        return {};
    } catch(const std::exception &e) { return e.what(); }
}
std::string capture_boundary(const StartupApplication &app,const Metadata &m) {
    const auto error=validate(app,m);
    if(!error.empty()) return error;
    return decode(m.controller_state).step==0?std::string{}:"title menu capture is not the default-no boundary";
}
StartupTitleMenuRequest confirm() { StartupTitleMenuRequest r; r.keys.confirm=true; return r; }
StartupTitleMenuRequest cancel() { StartupTitleMenuRequest r; r.keys.back=true; return r; }
StartupTitleMenuRequest consume() {
    StartupTitleMenuRequest r; r.kind=StartupTitleMenuRequestKind::consume_return; return r;
}
StartupTitleMenuRequest touch(int component,int value) {
    StartupTitleMenuRequest r; r.kind=StartupTitleMenuRequestKind::touch_up;
    r.component=component; r.value=value; return r;
}
void apply(StartupApplication &app,const StartupTitleMenuRequest &request) {
    good(app.apply_title_request(app.title_page_id(),request));
}
std::vector<StartupAudioRequest> take(StartupApplication &app,const std::vector<int> &ids) {
    const auto actual=app.take_audio_requests();
    std::vector<StartupAudioRequest> expected;
    for(int id:ids) expected.push_back({StartupAudioOperation::replace_bgm,id});
    need(actual==expected && app.take_sound_requests().empty() && !app.has_pending_audio_requests() &&
         (!app.world() || app.world()->state().sound_requests.empty()),"typed original output sequence consumed once");
    return actual;
}
Driver initialize(StartupApplication &app) {
    good(app.error()); good(app.request_new_game(0)); good(app.start_game());
    take(app,{0,1}); good(app.save_world()); good(app.return_to_title()); take(app,{0});
    apply(app,confirm()); apply(app,confirm()); apply(app,touch(9,2));
    need(app.title_menu().confirmation && app.title_menu().confirmation->selection==1,
         "capture actual manual deletion prompt default no");
    Driver d;
    need(app.records().save_directory[0][1].reference.has_value(),"actual manual save reference");
    d.original=*app.records().save_directory[0][1].reference;
    good(validate(app,metadata(d)));
    return d;
}
std::vector<StartupAudioRequest> step(StartupApplication &app,Driver &d) {
    need(d.step<terminal,"tail already finished");
    switch(d.step) {
    case 0: apply(app,confirm()); break;
    case 1: apply(app,consume()); break;
    case 2: apply(app,cancel()); break;
    case 3: apply(app,consume()); break;
    case 4: apply(app,confirm()); break;
    case 5: apply(app,touch(9,2)); break;
    case 6: apply(app,touch(3,0)); break;
    case 7: apply(app,consume()); break;
    case 8: apply(app,consume()); break;
    case 9: apply(app,confirm()); break;
    case 10: good(app.start_game()); break;
    case 11: good(app.save_world()); break;
    case 12: good(app.return_to_title()); break;
    case 13: apply(app,confirm()); break;
    case 14: apply(app,confirm()); break;
    case 15: apply(app,touch(9,0)); break;
    case 16: apply(app,consume()); break;
    default: throw std::runtime_error("unreachable title command");
    }
    const std::vector<int> expected=d.step==10 || d.step==16?std::vector<int>{1}:
        d.step==12?std::vector<int>{0}:std::vector<int>{};
    auto audio=take(app,expected);
    ++d.step; d.audio_count+=audio.size(); d.audio_peak=std::max(d.audio_peak,static_cast<std::uint64_t>(audio.size()));
    good(validate(app,metadata(d)));
    return audio;
}
struct Observation {
    std::string digest,system_hash,view_hash;
    std::uint64_t blobs{},bytes{};
};
Observation observe(const StartupApplication &app,const Driver &d,const fs::path &root) {
    const auto disk=load_startup_application_storage(root);
    need(disk.snapshot.has_value(),disk.error);
    const auto captured=capture_startup_application_storage(root,*disk.snapshot);
    need(captured.view.has_value(),captured.error);
    const auto &view=*captured.view;
    need(view.snapshot.records.revision==app.records().revision &&
         view.snapshot.records.save_directory==app.records().save_directory && view.blobs.size()==1,
         "disk file view and app exact directory identity");
    Bytes canonical=view.system_bytes;
    std::uint64_t total=view.system_bytes.size();
    for(const auto &blob:view.blobs) {
        need(blob.reference==d.original && blob.bytes.size()==d.original.bytes,
             "hidden and re-saved manual keep original blob identity");
        wire::u64(canonical,blob.bytes.size()); canonical.insert(canonical.end(),blob.bytes.begin(),blob.bytes.end());
        total+=blob.bytes.size();
    }
    need(total<=budget,"128MiB unchanged file-view budget");
    return {startup_application_replay_digest(app,metadata(d),validate),hash(view.system_bytes),hash(canonical),
        static_cast<std::uint64_t>(view.blobs.size()),total-view.system_bytes.size()};
}
std::string row(const StartupApplication &app,const Driver &d,const fs::path &root,
                const std::vector<StartupAudioRequest> &audio) {
    const auto o=observe(app,d,root);
    std::ostringstream out;
    out<<"{\"step\":"<<d.step<<",\"app_driver_digest\":\""<<o.digest
       <<"\",\"system_sha256\":\""<<o.system_hash<<"\",\"file_view_digest\":\""<<o.view_hash
       <<"\",\"blob_count\":"<<o.blobs<<",\"blob_bytes\":"<<o.bytes<<",\"typed_audio\":[";
    for(std::size_t i=0;i<audio.size();++i) {
        if(i) out<<',';
        out<<'['<<static_cast<int>(audio[i].operation)<<','<<audio[i].id<<']';
    }
    out<<"],\"random\":"<<app.world()->state().scene.random.draws()<<"}\n";
    return out.str();
}
}

int run_startup_title_menu_file_replay_cli(int argc,const char **argv) {
    std::map<std::string,std::string> options;
    for(int i=2;i<argc;i+=2) {
        need(i+1<argc,"missing option value"); const std::string key=argv[i];
        need(key=="--work-dir" || key=="--trace-file" || key=="--save-file" || key=="--load-file","unknown option");
        need(options.emplace(key,argv[i+1]).second,"duplicate option");
    }
    need(options.count("--work-dir") && options.count("--trace-file") &&
         (options.count("--save-file")!=options.count("--load-file")),"reference/restore CLI options");
    const auto root=fs::absolute(options.at("--work-dir")),live=root/"application";
    const auto trace=fs::absolute(options.at("--trace-file"));
    const bool loading=options.count("--load-file")!=0;
    const auto snapshot=fs::absolute(options.at(loading?"--load-file":"--save-file"));
    const StartupApplicationPaths current{root/"unused-current"};
    (void)persistence_detail::prepare_replay_capture_paths(root,root/".title-menu-probe",current,{trace,snapshot});
    const auto trace_paths=persistence_detail::prepare_replay_capture_paths(root.parent_path(),trace,current,{snapshot});
    if(loading) (void)persistence_detail::prepare_replay_restore_paths(live,snapshot,current,{trace});
    else (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(),snapshot,{live},{trace});
    const auto initial=loading?current.root:live;
    need(fs::create_directory(initial),"fresh application root; do not overwrite an old test run");
    StartupApplication app({initial},ref::WorldRandomStream::from_java_seed(1));
    Driver d; Metadata m;
    Bytes source;
    if(loading) {
        take(app,{0});
        source=persistence_detail::read_save_file(snapshot,budget);
        good(restore_startup_application_replay(snapshot,live,controller,app,m,capture_boundary,{trace}));
        d=decode(m.controller_state);
        need(d.step==0,"this CLI only replays its certified default-no capture boundary");
        need(!app.has_pending_audio_requests(),"exact restore does not replay title/activation music");
    } else {
        d=initialize(app); m=metadata(d);
        good(save_startup_application_replay(root.parent_path(),snapshot,app,m,capture_boundary,{trace}));
        source=persistence_detail::read_save_file(snapshot,budget);
        // 独立线格式复核完整分区/段摘要，不重签或生成预期容器来迁就实现。
        const auto parsed=wire::unpack(source);
        need(!parsed.sections.empty(),"capture has verified wire sections");
    }
    std::ostringstream output;
    output<<row(app,d,live,{});
    while(d.step<terminal) { const auto audio=step(app,d); output<<row(app,d,live,audio); }
    need(source==persistence_detail::read_save_file(snapshot,budget),"capture source remains unchanged");
    const auto rendered=output.str();
    need(rendered.size()<1024U*1024U,"bounded 18-row trace");
    (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(),trace,{live},{snapshot});
    persistence_detail::create_save_file(trace_paths.container,Bytes(rendered.begin(),rendered.end()));
    const auto o=observe(app,d,live);
    std::cout<<"title menu replay summary {\"controller\":\""<<controller<<"\",\"steps\":"<<d.step
        <<",\"digest\":\""<<o.digest<<"\",\"file_view_digest\":\""<<o.view_hash
        <<"\",\"system_sha256\":\""<<o.system_hash<<"\",\"blob_count\":"<<o.blobs
        <<",\"blob_bytes\":"<<o.bytes<<",\"audio_count\":"<<d.audio_count
        <<",\"random\":"<<app.world()->state().scene.random.draws()<<"}\n";
    return 0;
}
