#include "startup_application_replay_paths.hpp"
#include "startup_application_storage_paths.hpp"
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
// 普通文件及其父组件沿存储层统一校验，不维护第二套Windows拼写规则。
fs::path file_path(const fs::path &input, bool allow_missing) {
    validate_storage_file(input, allow_missing);
    return fs::weakly_canonical(fs::absolute(input));
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
    result.reserve(1+extra.size());
    result.push_back(validate_storage_root(app.root,present(app.root)));
    for(const auto &p:extra)result.push_back(file_path(p,true));
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
    const auto root=validate_storage_root(research_directory,true);
    const auto target=file_path(output,true);
    const auto blocked=protected_files(current,extra);
    distinct_from(target,blocked);
    new_file(target,root);
    return {root,target,{}};
}
ApplicationReplayPaths prepare_replay_restore_paths(const fs::path &research_directory,
    const fs::path &source,const StartupApplicationPaths &current,const std::vector<fs::path> &extra) {
    const auto root=validate_storage_root(research_directory,false);
    const auto input=file_path(source,false);
    // 源文件可直接位于work；可写隔离根才必须是work的严格子目录。
    const auto trusted=fs::canonical(fs::path(DUNGEON_VILLAGE_RESEARCH_WORK_ROOT));
    if(!inside(input,trusted))fail("源快照须位于本研究work内");
    auto blocked=protected_files(current,extra);
    for(const auto &p:extra)if(aliases(input,file_path(p,true)))fail("源快照与受保护输出是别名");
    blocked.push_back(input);
    distinct_from(root,blocked);
    return {root,input,StartupApplicationPaths{root}};
}
} // namespace dungeon_village_prototype::persistence_detail
