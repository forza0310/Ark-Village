#include "dungeon_village_prototype/startup_application_storage.hpp"
#include "dungeon_village_tools/archive.hpp"
#include "startup_application_storage_paths.hpp"
#include "startup_persistence_bytes.hpp"
#include "startup_world_file_io.hpp"
#include <algorithm>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#ifndef DUNGEON_VILLAGE_RESEARCH_WORK_ROOT
#error "应用目录存储必须绑定research/work"
#endif

namespace dungeon_village_prototype {
namespace {
namespace fs = std::filesystem;
namespace detail = persistence_detail;
using Bytes = std::vector<std::uint8_t>;
using Snapshot = StartupApplicationStorageSnapshot;
constexpr std::size_t file_budget=128U*1024U*1024U, system_budget=4U*1024U*1024U;
void need(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
void supported() {
#ifndef _WIN32
    throw std::runtime_error("应用目录存储只支持已实现的Windows平台");
#endif
}
fs::file_status storage_status(const fs::path &p) {
    std::error_code ec;
    const auto result=fs::symlink_status(p,ec);
    need(!ec || ec==std::errc::no_such_file_or_directory,"应用目录路径状态不可读取");
    need(result.type()!=fs::file_type::none,"应用目录路径状态未知");
    return result;
}
bool is_present(const fs::path &p) { return storage_status(p).type()!=fs::file_type::not_found; }
void plain(const fs::path &p,bool directory) {
    const auto s=storage_status(p);
    need(!fs::is_symlink(s),"应用目录不允许符号链接");
#ifdef _WIN32
    const auto a=GetFileAttributesW(p.c_str());
    if(a==INVALID_FILE_ATTRIBUTES) {
        const auto code=GetLastError();
        throw std::runtime_error("应用目录属性不可读："+p.u8string()+" (Windows "+std::to_string(code)+")");
    }
    if(a&FILE_ATTRIBUTE_REPARSE_POINT)
        throw std::runtime_error("应用目录不允许重解析点："+p.u8string()+" (attributes "+std::to_string(a)+")");
#endif
    need(directory?fs::is_directory(s):fs::is_regular_file(s),"应用目录对象种类不符");
    if(!directory) need(fs::hard_link_count(p)==1,"应用存储文件不允许硬链接别名");
}
fs::path normalized(const fs::path &input) {
    need(!input.empty() && !input.filename().empty(),"应用存储路径为空或缺文件名");
#ifdef _WIN32
    const auto native=input.native();
    need(native.find(L'\0')==std::wstring::npos &&
         native.rfind(L"\\\\?\\",0)!=0 && native.rfind(L"\\\\.\\",0)!=0,
         "应用存储不接受设备命名空间");
#endif
    const auto absolute=fs::absolute(input);
    auto cursor=absolute.root_path();
    for(const auto &part:absolute.relative_path()) {
        need(part!="..","应用存储路径不得上溯");
        if(part==".")continue;
#ifdef _WIN32
        const auto text=part.native();
        need(!text.empty() && text.back()!=L'.' && text.back()!=L' ' && text.find(L':')==std::wstring::npos,
             "应用存储路径含尾点/空格或备用数据流");
        auto stem=text.substr(0,text.find(L'.'));
        std::transform(stem.begin(),stem.end(),stem.begin(),[](wchar_t c){return c>=L'a'&&c<=L'z'?c-L'a'+L'A':c;});
        need(stem!=L"CON" && stem!=L"PRN" && stem!=L"AUX" && stem!=L"NUL" &&
             !(stem.size()==4 && (stem.substr(0,3)==L"COM" || stem.substr(0,3)==L"LPT") &&
               stem[3]>=L'1' && stem[3]<=L'9'),"应用存储路径包含设备名");
#endif
        cursor/=part;
        if(is_present(cursor)) plain(cursor,cursor!=absolute || fs::is_directory(storage_status(cursor)));
    }
    return fs::weakly_canonical(absolute);
}
bool inside(const fs::path &child,const fs::path &parent) {
    auto c=child.begin();
    for(const auto &p:parent) {
        if(c==child.end())return false;
#ifdef _WIN32
        if(CompareStringOrdinal(c->c_str(),-1,p.c_str(),-1,TRUE)!=CSTR_EQUAL)return false;
#else
        if(*c!=p)return false;
#endif
        ++c;
    }
    return c!=child.end();
}
class Lease {
#ifdef _WIN32
    HANDLE handle_{INVALID_HANDLE_VALUE};
#endif
  public:
    explicit Lease(const fs::path &root) {
        supported();
        const auto lock=root/"write.lock";
        detail::validate_storage_file(lock);
#ifdef _WIN32
        handle_=CreateFileW(lock.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,nullptr);
        need(handle_!=INVALID_HANDLE_VALUE,"应用存储写租约正忙或无法取得");
        BY_HANDLE_FILE_INFORMATION info{};
        if(!GetFileInformationByHandle(handle_,&info) || info.nNumberOfLinks!=1 ||
           info.nFileSizeHigh!=0 || info.nFileSizeLow!=0 ||
           (info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)) {
            CloseHandle(handle_); handle_=INVALID_HANDLE_VALUE;
            throw std::runtime_error("应用存储租约文件身份非法");
        }
#endif
    }
    ~Lease() {
#ifdef _WIN32
        if(handle_!=INVALID_HANDLE_VALUE)CloseHandle(handle_);
#endif
    }
    Lease(const Lease&)=delete;
    Lease &operator=(const Lease&)=delete;
};
Snapshot read_system(const fs::path &root) {
    const auto file=root/"system.avr";
    detail::validate_storage_file(file);
    if(!is_present(file)) {
        StartupSystemRecords records;
        const auto hash=dungeon_village_tools::sha256_hex(detail::encode_system_records_bytes(records));
        return {std::move(records),true,hash}; // 缺失仍有规范目录身份，但missing不能与已有文件混同。
    }
    auto bytes=detail::read_save_file(file,system_budget);
    const auto hash=dungeon_village_tools::sha256_hex(bytes);
    return {detail::decode_system_records_bytes(std::move(bytes)),false,hash};
}
void matches(const Snapshot &disk,const Snapshot &expected) {
    need(disk.missing==expected.missing && disk.digest==expected.digest &&
         disk.records.revision==expected.records.revision &&
         detail::encode_system_records_bytes(disk.records)==detail::encode_system_records_bytes(expected.records),
         "应用存储修订或文件摘要已变化，拒绝旧观察值提交");
}
std::size_t kind_index(StartupSaveKind kind) {
    need(kind==StartupSaveKind::interrupt || kind==StartupSaveKind::manual,"存档目录类型非法");
    return static_cast<std::size_t>(kind);
}
const StartupSaveDirectoryEntry &entry(const Snapshot &s,int slot,StartupSaveKind kind) {
    need(slot>=0 && slot<2,"存档槽号非法");
    return s.records.save_directory[static_cast<std::size_t>(slot)][kind_index(kind)];
}
fs::path blob_path(const fs::path &root,const StartupSaveReference &r) {
    return root/"worlds"/("world-"+detail::storage_hash_hex(r.sha256)+".avrs");
}
StartupSaveReference reference(const Bytes &bytes) {
    StartupSaveReference r;
    const auto hash=dungeon_village_tools::sha256_hex(bytes);
    const auto nibble=[](char c){return c<='9'?c-'0':c-'a'+10;};
    for(std::size_t i=0;i<32;++i)r.sha256[i]=static_cast<std::uint8_t>(nibble(hash[2*i])*16+nibble(hash[2*i+1]));
    r.bytes=bytes.size();
    return r;
}
Bytes read_blob(const fs::path &root,const StartupSaveReference &r) {
    need(r.purpose==StartupWorldSavePurpose::normal && r.bytes && r.bytes<=file_budget,
         "存档载荷用途或大小非法");
    plain(root/"worlds",true);
    const auto file=blob_path(root,r);
    detail::validate_storage_file(file,false);
    const auto bytes=detail::read_save_file(file,static_cast<std::size_t>(r.bytes));
    need(bytes.size()==r.bytes && reference(bytes)==r,"存档载荷字节数或摘要不符");
    return bytes;
}
std::set<fs::path> references(const fs::path &root,const StartupSystemRecords &r) {
    std::set<fs::path> paths;
    for(const auto &slot:r.save_directory)for(const auto &e:slot)
        if(e.reference)paths.insert(blob_path(root,*e.reference));
    return paths;
}
enum class Mutation { records, save, hide };
StartupApplicationStorageResult commit(const fs::path &input,const Snapshot &expected,
    const StartupSystemRecords &candidate,Mutation mutation,int slot,StartupSaveKind kind,
    const StartupWorldRuntimeSession *world) {
    fs::path unpublished;
    fs::path world_directory;
    bool owns_blob=false, owns_directory=false;
    StartupApplicationStorageResult out;
    // 错误清理也必须持租约，否则另一写者可能复用刚发布的同hash文件后被本次删除。
    std::unique_ptr<Lease> lease;
    try {
        supported();
        const auto root=detail::validate_storage_root(input,true);
        world_directory=root/"worlds";
        lease=std::make_unique<Lease>(root);
        const auto disk=read_system(root);
        matches(disk,expected);
        need(candidate.revision==expected.records.revision &&
             candidate.save_directory==expected.records.save_directory,
             "调用者不得直接改系统修订或存档目录");
        need(candidate.revision<std::numeric_limits<std::uint64_t>::max(),"系统修订号耗尽");
        Snapshot next{candidate,false,{}};
        ++next.records.revision;
        Bytes world_bytes;
        fs::path target;
        if(mutation!=Mutation::records) {
            (void)entry(expected,slot,kind);
            auto &e=next.records.save_directory[static_cast<std::size_t>(slot)][kind_index(kind)];
            if(mutation==Mutation::hide) {
                need(e.packed_date!=-1 && e.reference.has_value(),"所选目录已经隐藏或为空");
                e.packed_date=-1;
            } else {
                need(world!=nullptr,"保存缺少世界候选");
                world_bytes=detail::encode_world_session_bytes(*world,{});
                const auto &s=world->state();
                const auto &date=s.scene.calendar;
                // 原P.b[3]对应维护subperiod（0..3）；目录显示时才按周标签加一。
                const auto packed=static_cast<std::int64_t>(date.year)*10000+date.month*100+date.subperiod;
                need(packed>=0 && packed<=INT32_MAX,"保存日期目录溢出");
                e={static_cast<int>(packed),s.scripts.village_name,s.scene.world.world.ai.accounting.funds(),reference(world_bytes)};
                target=blob_path(root,*e.reference);
            }
        }
        const auto bytes=detail::encode_system_records_bytes(next.records);
        next.digest=dungeon_village_tools::sha256_hex(bytes);
        const auto old_paths=references(root,disk.records), new_paths=references(root,next.records);
        std::vector<fs::path> obsolete;
        for(const auto &p:old_paths)if(!new_paths.count(p))obsolete.push_back(p);
        // 结果与候选全部在唯一提交点之前分配。成功后只移动，不再验证或推进业务。
        out.snapshot=std::move(next);
        static_assert(std::is_nothrow_move_constructible_v<StartupApplicationStorageResult>);
        if(!target.empty()) {
            unpublished=target; // 在发布前完成路径分配，成功后只能置不抛异常的所有权标记。
            if(!is_present(world_directory)) {
                need(fs::create_directory(world_directory),"无法创建应用世界目录");
                owns_directory=true;
            }
            plain(world_directory,true);
            detail::validate_storage_file(target);
            if(is_present(target))need(detail::read_save_file(target,file_budget)==world_bytes,"同摘要文件内容不同，拒绝覆盖");
            else { detail::create_save_file(target,world_bytes); owns_blob=true; }
        }
        // 任意既有system硬链接/重解析点在最终发布前仍须拒绝；租约协调本模块写者。
        detail::validate_storage_root(root,true);
        detail::validate_storage_file(root/"system.avr");
        if(disk.missing)detail::create_save_file(root/"system.avr",bytes);
        else detail::replace_save_file(root/"system.avr",bytes);
        owns_blob=false;
        owns_directory=false;
        // 只回收上次已持有且现在无四目录引用的文件，不扫描用户任意文件；隐藏引用算活跃。
        for(const auto &p:obsolete) {
            try {
                detail::validate_storage_file(p);
                if(is_present(p)) {
                    const auto old_bytes=detail::read_save_file(p,file_budget);
                    need(p.filename()==fs::path("world-"+dungeon_village_tools::sha256_hex(old_bytes)+".avrs"),
                         "待清理旧载荷身份已变化，保留文件");
                    std::error_code ec; fs::remove(p,ec); if(ec)out.cleanup_pending=true;
                }
            }
            catch(...) { out.cleanup_pending=true; }
        }
        return out;
    } catch(const std::exception &e) {
        out.snapshot.reset();
        if(owns_blob) {
            std::error_code ec; fs::remove(unpublished,ec); out.cleanup_pending=static_cast<bool>(ec);
        }
        if(owns_directory) {
            std::error_code ec; fs::remove(world_directory,ec);
            out.cleanup_pending=out.cleanup_pending || static_cast<bool>(ec);
        }
        out.error=e.what(); // 错误文案分配也排在本次文件回收之后。
        return out;
    }
}
} // namespace
namespace persistence_detail {
std::string storage_hash_hex(const std::array<std::uint8_t,32> &hash) {
    static constexpr char digits[]="0123456789abcdef";
    std::string text; text.reserve(64);
    for(auto b:hash) {text.push_back(digits[b>>4]);text.push_back(digits[b&15]);}
    return text;
}
fs::path validate_storage_root(const fs::path &input,bool must_exist) {
    supported();
    const auto root=normalized(input);
    const auto trusted=fs::canonical(fs::path(DUNGEON_VILLAGE_RESEARCH_WORK_ROOT));
    need(inside(root,trusted),"应用存储根必须位于本研究work内且不能是整个work");
    if(must_exist)plain(root,true);
    else {need(!is_present(root),"应用恢复目标根已存在");plain(root.parent_path(),true);}
    return root;
}
void validate_storage_file(const fs::path &file,bool allow_missing) {
    (void)normalized(file);
    if(is_present(file))plain(file,false);
    else need(allow_missing,"应用存储文件缺失");
}
} // namespace persistence_detail
StartupApplicationStorageResult load_startup_application_storage(const fs::path &input) {
    try { supported();const auto root=detail::validate_storage_root(input,true);return {read_system(root),{},false}; }
    catch(const std::exception &e) {return {{},e.what(),false};}
}
StartupApplicationStorageResult commit_startup_application_records(const fs::path &root,
    const Snapshot &expected,const StartupSystemRecords &candidate) {
    return commit(root,expected,candidate,Mutation::records,0,StartupSaveKind::manual,nullptr);
}
StartupApplicationStorageResult save_startup_application_slot(const fs::path &root,
    const Snapshot &expected,const StartupSystemRecords &candidate,int slot,StartupSaveKind kind,
    const StartupWorldRuntimeSession &world) {
    return commit(root,expected,candidate,Mutation::save,slot,kind,&world);
}
StartupApplicationStorageResult hide_startup_application_slot(const fs::path &root,
    const Snapshot &expected,const StartupSystemRecords &candidate,int slot,StartupSaveKind kind) {
    return commit(root,expected,candidate,Mutation::hide,slot,kind,nullptr);
}
StartupWorldLoadResult load_startup_application_slot(const fs::path &input,const Snapshot &expected,
    int slot,StartupSaveKind kind) {
    try {
        supported();const auto root=detail::validate_storage_root(input,true);
        Lease lease(root); matches(read_system(root),expected);
        const auto &e=entry(expected,slot,kind);
        need(e.packed_date!=-1 && e.reference.has_value(),"所选存档目录为空或已隐藏");
        auto bytes=read_blob(root,*e.reference);
        return {detail::decode_world_session_bytes(std::move(bytes),startup_world_rules(),
                 StartupWorldSavePurpose::normal,{}),{}};
    } catch(const std::exception &e) {return {{},e.what()};}
}
StartupApplicationStorageCapture capture_startup_application_storage(const fs::path &input,
    const Snapshot &expected) {
    try {
        supported();const auto root=detail::validate_storage_root(input,true);
        Lease lease(root);const auto disk=read_system(root);matches(disk,expected);
        auto system_bytes=disk.missing?detail::encode_system_records_bytes(disk.records):
            detail::read_save_file(root/"system.avr",system_budget);
        need(dungeon_village_tools::sha256_hex(system_bytes)==disk.digest,"捕获时系统字节已变化");
        StartupApplicationStorageView view{disk,std::move(system_bytes),{}};
        std::size_t size=view.system_bytes.size();
        std::set<std::array<std::uint8_t,32>> seen;
        for(const auto &slot:disk.records.save_directory)for(const auto &e:slot)if(e.reference) {
            if(!seen.insert(e.reference->sha256).second) {
                const auto found=std::find_if(view.blobs.begin(),view.blobs.end(),[&](const auto &b){return b.reference.sha256==e.reference->sha256;});
                need(found!=view.blobs.end() && found->reference==*e.reference,"相同摘要的目录载荷声明不一致");
                continue;
            }
            need(e.reference->bytes<=file_budget-size,"应用槽位文件视图总预算超限");
            auto bytes=read_blob(root,*e.reference);size+=bytes.size();
            // 这里只固定文件视图及完整性。回放调用者以共享解码预算验证所有世界，不能逐档重置预算。
            view.blobs.push_back({*e.reference,std::move(bytes)});
        }
        std::sort(view.blobs.begin(),view.blobs.end(),[](const auto &a,const auto &b){return a.reference.sha256<b.reference.sha256;});
        return {std::move(view),{}};
    } catch(const std::exception &e) {return {{},e.what()};}
}
} // namespace dungeon_village_prototype
