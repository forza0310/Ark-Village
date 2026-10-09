#include "startup_application_replay_paths.hpp"
#include <algorithm>
#include <stdexcept>
#include <system_error>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#ifndef DUNGEON_VILLAGE_RESEARCH_WORK_ROOT
#error "应用回放路径检查必须绑定本研究包work目录"
#endif

namespace dungeon_village_prototype::persistence_detail {
namespace {
namespace fs = std::filesystem;
[[noreturn]] void fail(const char *reason) {
    throw std::runtime_error(std::string("应用回放路径拒绝：") + reason);
}
bool same_component(const fs::path &a, const fs::path &b) {
#ifdef _WIN32
    const int comparison=CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE);
    if(comparison==0) fail("Windows路径大小写比较失败");
    return comparison==CSTR_EQUAL;
#else
    return a==b;
#endif
}
bool same_path(const fs::path &a,const fs::path &b) {
    auto x=a.begin(),y=b.begin();
    for(;x!=a.end() && y!=b.end();++x,++y)
        if(!same_component(*x,*y))return false;
    return x==a.end() && y==b.end();
}
bool inside(const fs::path &child,const fs::path &parent) {
    auto c=child.begin();
    for(auto p=parent.begin();p!=parent.end();++p,++c)
        if(c==child.end() || !same_component(*c,*p))return false;
    return c!=child.end(); // 指定隔离根本身不是一个可写文件，也不能等于整个work根。
}
fs::file_status checked_status(const fs::path &path) {
    std::error_code ec;
    const auto status=fs::symlink_status(path,ec);
    if(ec && ec!=std::errc::no_such_file_or_directory)fail("读取文件系统状态失败");
    if(!ec && status.type()==fs::file_type::none)fail("文件系统返回未知状态");
    return status;
}
bool present(const fs::path &path) {
    return checked_status(path).type()!=fs::file_type::not_found;
}
void spelling(const fs::path &path) {
    if(path.empty() || path.filename().empty() || path.filename()=="." || path.filename()=="..")
        fail("空路径或目录分隔符不能作为目标名");
#ifdef _WIN32
    const auto native=path.native();
    if(native.rfind(L"\\\\?\\",0)==0 || native.rfind(L"\\\\.\\",0)==0)
        fail("Windows设备命名空间不接受");
#endif
    for(const auto &component:path.relative_path()) {
        if(component=="..")fail("路径不得包含上溯组件");
#ifdef _WIN32
        const auto text=component.native();
        if(text.empty() || text==L".")continue;
        if(text.back()==L'.' || text.back()==L' ' || text.find(L':')!=std::wstring::npos)
            fail("Windows尾点/空格或备用数据流路径不接受");
        auto stem=text.substr(0,text.find(L'.'));
        std::transform(stem.begin(),stem.end(),stem.begin(),[](wchar_t c) {
            return c>=L'a' && c<=L'z'?static_cast<wchar_t>(c-L'a'+L'A'):c;
        });
        if(stem==L"CON" || stem==L"PRN" || stem==L"AUX" || stem==L"NUL" ||
           (stem.size()==4 && (stem.substr(0,3)==L"COM" || stem.substr(0,3)==L"LPT") &&
            stem[3]>=L'1' && stem[3]<=L'9'))fail("Windows设备名称不能作为研究文件");
#endif
    }
}
// 先检查原输入路径的每个实际父组件，不能先lexically_normal消掉link/..再检查。
fs::path normalized(const fs::path &path,bool reject_links) {
    spelling(path);
    std::error_code ec;
    const auto absolute=fs::absolute(path,ec);
    if(ec)fail("不能转换绝对路径");
    fs::path cursor=absolute.root_path();
    const auto relative=absolute.relative_path();
    for(auto it=relative.begin();it!=relative.end();++it) {
        cursor/=*it;
        const auto status=checked_status(cursor);
        if(status.type()==fs::file_type::not_found)continue;
        if(reject_links) {
            if(fs::is_symlink(status))fail("可访问目标及父目录不得是符号链接");
#ifdef _WIN32
            const auto attributes=GetFileAttributesW(cursor.c_str());
            if(attributes==INVALID_FILE_ATTRIBUTES)fail("读取Windows目录属性失败");
            if((attributes & FILE_ATTRIBUTE_REPARSE_POINT)!=0)fail("目标或父目录包含重解析点");
#endif
        }
        auto next=it; ++next;
        if(next!=relative.end()) {
            const auto followed=fs::status(cursor,ec);
            if(ec || !fs::is_directory(followed))fail("路径父组件不是可访问目录");
        }
    }
    const auto result=fs::weakly_canonical(absolute,ec);
    if(ec || !result.is_absolute())fail("不能规范化路径");
    return result;
}
fs::path trusted_root() {
    std::error_code ec;
    const auto root=fs::canonical(fs::path(DUNGEON_VILLAGE_RESEARCH_WORK_ROOT),ec);
    if(ec || !fs::is_directory(root,ec) || ec)fail("编译绑定的研究work目录不可访问");
    return root;
}
fs::path isolation_root(const fs::path &input,const fs::path &trusted) {
    const auto root=normalized(input,true);
    if(!inside(root,trusted))fail("隔离目录不在编译绑定的研究work目录内");
    std::error_code ec;
    if(!fs::is_directory(root,ec) || ec)fail("须显式提供已存在的隔离目录");
    return root;
}
bool aliases(const fs::path &a,const fs::path &b) {
    if(same_path(a,b))return true;
    // 对实际存在的路径核文件身份，覆盖不同名字的硬链接；不存在不能拿equivalent的错误当通过。
    if(!present(a) || !present(b))return false;
    std::error_code ec;
    const bool same=fs::equivalent(a,b,ec);
    if(ec)fail("不能判定已有文件别名");
    return same;
}
std::vector<fs::path> protected_files(const StartupApplicationPaths &app,
                                      const std::vector<fs::path> &extra) {
    std::vector<fs::path> result;
    result.reserve(3+extra.size());
    for(const auto &p:std::array<fs::path,3>{app.system,app.worlds[0],app.worlds[1]})
        result.push_back(normalized(p,false));
    for(const auto &p:extra)result.push_back(normalized(p,false));
    return result;
}
void distinct_from(const fs::path &p,const std::vector<fs::path> &blocked) {
    for(const auto &b:blocked)
        if(aliases(p,b) || inside(p,b) || inside(b,p))fail("路径与现有应用/受保护文件重叠");
}
void new_file(const fs::path &p,const fs::path &root) {
    if(!inside(p,root))fail("目标文件逃离指定隔离目录");
    if(present(p))fail("新目标已存在，不能覆盖文件/目录/链接");
    std::error_code ec;
    if(!fs::is_directory(p.parent_path(),ec) || ec)fail("新目标的父目录尚不存在或不可访问");
}
}
ApplicationReplayPaths prepare_replay_capture_paths(const fs::path &research_directory,
    const fs::path &output,const StartupApplicationPaths &current,const std::vector<fs::path> &extra) {
    const auto trusted=trusted_root();
    const auto root=isolation_root(research_directory,trusted);
    const auto target=normalized(output,true);
    const auto blocked=protected_files(current,extra);
    distinct_from(target,blocked);
    new_file(target,root);
    return {root,target,{}};
}
ApplicationReplayPaths prepare_replay_restore_paths(const fs::path &research_directory,
    const fs::path &source,const StartupApplicationPaths &current,const std::vector<fs::path> &extra) {
    const auto trusted=trusted_root();
    const auto root=isolation_root(research_directory,trusted);
    const auto input=normalized(source,true);
    if(!inside(input,trusted) || !fs::is_regular_file(checked_status(input)))
        fail("源快照须是研究work内的普通文件");
    auto blocked=protected_files(current,extra);
    // source可以只读，但不能兼作trace/证书等调用者声明的另一个文件。
    for(const auto &p:extra)if(aliases(input,normalized(p,false)))fail("源快照与受保护输出是别名");
    blocked.push_back(input);
    StartupApplicationPaths files{root/"system.avr",{root/"world0.avr",root/"world1.avr"}};
    for(const auto &p:std::array<fs::path,3>{files.system,files.worlds[0],files.worlds[1]}) {
        const auto target=normalized(p,true);
        distinct_from(target,blocked);
        new_file(target,root);
        blocked.push_back(target);
    }
    return {root,input,std::move(files)};
}
} // namespace dungeon_village_prototype::persistence_detail
