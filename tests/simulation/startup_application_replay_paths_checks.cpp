#include "../../src/simulation/application/startup_application_replay_paths.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
namespace fs=std::filesystem;
using namespace ark::simulation;
using namespace ark::simulation::persistence_detail;
struct Checks {
    int count{};
    void operator()(bool okay,const char *message) {
        ++count;
        if(!okay)throw std::runtime_error(std::string("应用快照路径：")+message);
    }
};
std::string bytes(const fs::path &p) {
    std::ifstream input(p,std::ios::binary);
    if(!input)throw std::runtime_error("路径测试不能读取保留夹具");
    return {std::istreambuf_iterator<char>(input),{}};
}
void file(const fs::path &p,const std::string &content) {
    if(fs::exists(p))throw std::runtime_error("路径夹具拒绝覆盖已有文件");
    std::ofstream output(p,std::ios::binary);
    output<<content;
    if(!output)throw std::runtime_error("路径测试不能建立夹具");
}
struct Work {
    fs::path root;
    explicit Work(const fs::path &parent):root(parent/"application-replay-path-boundaries") {
        if(!fs::create_directory(root))throw std::runtime_error("路径夹具目录必须独占新建");
    }
    ~Work() { std::error_code ec; fs::remove_all(root,ec); }
    fs::path directory(const std::string &name) {
        const auto result=root/name;
        if(!fs::create_directory(result))throw std::runtime_error("路径夹具子目录新建失败");
        return result;
    }
};
template<class F> void rejects(Checks &check,F &&operation,const char *message) {
    bool refused=false;
    try { operation(); } catch(const std::runtime_error &) { refused=true; }
    check(refused,message);
}
bool link_unavailable(const std::error_code &ec) {
    return ec==std::errc::operation_not_permitted || ec==std::errc::permission_denied ||
        ec==std::errc::operation_not_supported
#ifdef _WIN32
        || ec.value()==ERROR_PRIVILEGE_NOT_HELD
#endif
        ;
}
}

// 返回具名检查数给既有应用套件；只使用父套件已独占建立的项目内目录。
int run_startup_application_replay_paths_checks(const std::filesystem::path &parent) {
    Checks check;
    Work work(parent);
    const auto old=work.directory("old-application");
    StartupApplicationPaths current{old};
    file((current.root/"system.avr"),"old-system-payload");
    file((current.root/"world0.avr"),"old-world-0-payload");
    file((current.root/"world1.avr"),"old-world-1-payload");
    const auto capture=work.directory("capture");
    const auto source=capture/"source.avrapp";
    file(source,"source-container-unchanged");
    const auto output=capture/"new.avrapp";
    const auto prepared=prepare_replay_capture_paths(capture,output,current);
    check(prepared.directory==fs::canonical(capture) && prepared.container==fs::weakly_canonical(output) &&
          !fs::exists(output),"捕获准备仅规范路径，不产生目标");
    rejects(check,[&]{prepare_replay_capture_paths(capture,source,current);},"已有容器拒绝覆盖");
    rejects(check,[&]{prepare_replay_capture_paths(old,(current.root/"system.avr"),current);},"捕获拒绝当前系统");
    rejects(check,[&]{prepare_replay_capture_paths(old,(current.root/"world0.avr"),current);},"捕获拒绝当前世界0");
    rejects(check,[&]{prepare_replay_capture_paths(old,(current.root/"world1.avr"),current);},"捕获拒绝当前世界1");
    rejects(check,[&]{prepare_replay_capture_paths(capture,output,current,{output});},"不存在trace路径也不能重叠");
    const auto sibling=work.directory("capture-other");
    rejects(check,[&]{prepare_replay_capture_paths(capture,sibling/"other.avrapp",current);},
            "包含关系按组件判定，拒绝同前缀兄弟目录");
    rejects(check,[&]{prepare_replay_capture_paths(capture,capture/"missing"/"new.avrapp",current);},
            "父目录缺失不自动创建");
    check(!fs::exists(capture/"missing"),"拒绝后缺失父目录仍不存在");
    const auto file_parent=capture/"file-parent";
    file(file_parent,"not-a-directory");
    rejects(check,[&]{prepare_replay_capture_paths(capture,file_parent/"new.avrapp",current);},
            "文件占父目录显式拒绝");
    rejects(check,[&]{prepare_replay_capture_paths(capture,capture/".."/"escape.avrapp",current);},
            "上溯组件不能在规范化之前隐藏父路径");
    const auto occupied=work.directory("directory-target");
    rejects(check,[&]{prepare_replay_capture_paths(work.root,occupied,current);},"目录占目标拒绝");

    const auto restore=work.root/"restore";
    const auto notes=work.directory("notes");
    file(notes/"README.txt","keep-unrelated-notes");
    const auto loaded=prepare_replay_restore_paths(restore,source,current);
    check(loaded.container==fs::canonical(source) && loaded.directory==fs::weakly_canonical(restore) &&
          loaded.application.root==fs::weakly_canonical(restore),
          "恢复源可在另一研究目录，恢复根固定为尚不存在的新目录");
    check(!fs::exists(loaded.application.root) && bytes(notes/"README.txt")=="keep-unrelated-notes",
          "恢复准备不创建根或清理邻居资料");
    auto alias=current;alias.root=restore;
    rejects(check,[&]{prepare_replay_restore_paths(restore,source,alias);},"恢复根不能与当前预定根重合");
    for(const auto &target:{restore,restore/"system.avr",restore/"worlds"})
        rejects(check,[&]{prepare_replay_restore_paths(restore,source,current,{target});},
                "恢复整根与trace或证书预定路径重叠时拒绝");
    rejects(check,[&]{prepare_replay_restore_paths(restore,source,current,{source});},"源不能兼作trace");
    rejects(check,[&]{prepare_replay_restore_paths(restore,capture,current);},"目录不能作为容器源");
    rejects(check,[&]{prepare_replay_restore_paths(restore,capture/"no-source.avrapp",current);},
            "缺失源明确拒绝");
    for(int slot=0;slot<3;++slot) {
        const auto root=work.directory("occupied-"+std::to_string(slot));
        const auto name=slot==0?"system.avr":slot==1?"world0.avr":"world1.avr";
        file(root/name,"existing-target");
        rejects(check,[&]{prepare_replay_restore_paths(root,source,current);},"任一目标已存在整次拒绝");
        check(bytes(root/name)=="existing-target" && std::distance(fs::directory_iterator(root),fs::directory_iterator{})==1,
              "拒绝没有删旧目标或补写其他两个目标");
    }
    // 从当前源文件所在项目的真实work根向上判定，不能让调用者把整个工作根或包目录冒充隔离根。
    auto trusted=fs::canonical(parent);
    while(trusted.filename()!="build" && trusted!=trusted.root_path())trusted=trusted.parent_path();
    check(trusted.filename()=="build","测试位于产品build隔离根下");
    rejects(check,[&]{prepare_replay_capture_paths(trusted,trusted/"forbidden.avrapp",current);},
            "整个编译work根不是显式隔离目录");
    rejects(check,[&]{prepare_replay_capture_paths(trusted.parent_path(),trusted.parent_path()/"forbidden.avrapp",current);},
            "调用者不能把work外目录自称研究隔离根");
#ifdef _WIN32
    rejects(check,[&]{prepare_replay_capture_paths(capture,output,current,{capture/"NEW.AVRAPP"});},
            "捕获输出Windows大小写别名拒绝");
    rejects(check,[&]{prepare_replay_restore_paths(restore,source,current,{work.root/"RESTORE"});},
            "恢复根Windows大小写别名拒绝");
    for(const char *name:{"new.avrapp.","new.avrapp ","file:stream","NUL.avrapp"})
        rejects(check,[&]{prepare_replay_capture_paths(capture,capture/name,current);},
                "Windows尾点/尾空格/ADS/设备名显式拒绝");
#endif
    std::error_code ec;
    const auto hard=capture/"source-hardlink.avrapp";
    fs::create_hard_link(source,hard,ec);
    if(ec) {
        check(link_unavailable(ec),"硬链接建立失败须是明确环境限制");
        std::cout<<"应用回放路径：硬链接场景未覆盖（环境不支持），"<<ec.message()<<'\n';
    } else {
        check(fs::equivalent(source,hard),"真实硬链接身份建立");
        rejects(check,[&]{prepare_replay_restore_paths(restore,source,current,{hard});},
                "不同名称的源/证书硬链接别名拒绝");
        rejects(check,[&]{prepare_replay_capture_paths(capture,hard,current);},"已有硬链接目标不覆盖");
        check(bytes(hard)=="source-container-unchanged","硬链接原件仍完整");
        check(fs::remove(hard),"回收本夹具硬链接，使后续独立覆盖符号链接拒绝");
    }
    const auto linked=work.root/"linked-directory";
    ec.clear(); fs::create_directory_symlink(capture,linked,ec);
    if(ec) {
        check(link_unavailable(ec),"目录符号链接建立失败须是明确权限/平台限制");
        std::cout<<"应用回放路径：symlink场景未覆盖（权限或平台限制），"<<ec.message()<<'\n';
    } else {
        rejects(check,[&]{prepare_replay_capture_paths(linked,linked/"new.avrapp",current);},
                "即使指回研究范围也拒绝符号链接隔离根");
        rejects(check,[&]{prepare_replay_capture_paths(work.root,linked/"new.avrapp",current);},
                "符号链接父组件拒绝");
        rejects(check,[&]{prepare_replay_restore_paths(restore,linked/"source.avrapp",current);},
                "源路径父组件符号链接拒绝");
        const auto dangling=capture/"dangling.avrapp";
        ec.clear(); fs::create_symlink(capture/"missing-target",dangling,ec);
        check(!ec,"同会话有符号链接资格时建立断链");
        rejects(check,[&]{prepare_replay_capture_paths(capture,dangling,current);},"断链也是已有目标，不能当作缺失");
    }
    check(!fs::exists(output) && !fs::exists(loaded.application.root) &&
          bytes(source)=="source-container-unchanged" && bytes((current.root/"system.avr"))=="old-system-payload" &&
          bytes((current.root/"world0.avr"))=="old-world-0-payload" && bytes((current.root/"world1.avr"))=="old-world-1-payload" &&
          bytes(file_parent)=="not-a-directory" && bytes(notes/"README.txt")=="keep-unrelated-notes",
          "全部准备/拒绝后源、旧应用、无关文件逐字节不变，新目标均未创建");
    return check.count;
}
