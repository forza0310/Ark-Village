#pragma once

// 两个应用回放套件共用的独立线格式夹具；只重签测试副本，不调用被测编码器。
#include "ark/assets/sha256.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace application_replay_fixture {
using Bytes=std::vector<std::uint8_t>;
inline void require(bool value,const char *reason) {
    if(!value)throw std::runtime_error(std::string("应用状态夹具：")+reason);
}
inline void u32(Bytes &b,std::uint32_t value) {
    for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(value>>(8*i)));
}
inline void u64(Bytes &b,std::uint64_t value) {
    for(unsigned i=0;i<8;++i)b.push_back(static_cast<std::uint8_t>(value>>(8*i)));
}
inline std::uint64_t number(const Bytes &b,std::size_t &at,std::size_t size) {
    require(at<=b.size() && size<=b.size()-at && size<=8,"读整数越界");
    std::uint64_t value{};
    for(std::size_t i=0;i<size;++i)value|=static_cast<std::uint64_t>(b[at++])<<(8*i);
    return value;
}
inline void set_number(Bytes &b,std::size_t at,std::size_t size,std::uint64_t value) {
    require(at<=b.size() && size<=b.size()-at && size<=8,"改整数越界");
    for(std::size_t i=0;i<size;++i)b[at+i]=static_cast<std::uint8_t>(value>>(8*i));
}
inline std::string text(const Bytes &b,std::size_t &at) {
    const auto length=number(b,at,4);
    require(length<=b.size()-at,"文字夹具越界");
    const auto begin=b.begin()+static_cast<std::ptrdiff_t>(at);
    at+=static_cast<std::size_t>(length);
    return {begin,b.begin()+static_cast<std::ptrdiff_t>(at)};
}
inline void resign(Bytes &bytes) {
    require(bytes.size()>=64,"整体摘要占64个十六进制字符");
    bytes.resize(bytes.size()-64);
    const auto hash=ark::assets::sha256_hex(bytes);
    bytes.insert(bytes.end(),hash.begin(),hash.end());
}
struct Section {
    std::uint32_t id{},version{},required{};
    Bytes bytes;
};
struct Wire {
    Bytes prefix; // magic/版本/三项来源身份，尚不含分区数。
    std::vector<Section> sections;
};
// 按已登记的容器格式独立读写损坏夹具，不调用被测control encoder产生期望。
inline Wire unpack(const Bytes &bytes) {
    require(bytes.size()>84 && std::string(bytes.begin(),bytes.begin()+8)=="AVRAPP01","原容器magic");
    std::size_t at=20;
    (void)text(bytes,at);(void)text(bytes,at);(void)text(bytes,at);
    Wire result;result.prefix.assign(bytes.begin(),bytes.begin()+static_cast<std::ptrdiff_t>(at));
    const auto count=number(bytes,at,4);
    require(count<=66,"夹具原段数预算");
    for(std::size_t i=0;i<count;++i) {
        Section part;
        part.id=static_cast<std::uint32_t>(number(bytes,at,4));
        part.version=static_cast<std::uint32_t>(number(bytes,at,4));
        part.required=static_cast<std::uint32_t>(number(bytes,at,4));
        const auto size=number(bytes,at,8);
        require(at<=bytes.size()-64 && 64<=bytes.size()-64-at,"段摘要范围");
        const std::string digest(bytes.begin()+static_cast<std::ptrdiff_t>(at),
                                 bytes.begin()+static_cast<std::ptrdiff_t>(at+64));
        at+=64;
        require(size<=bytes.size()-64-at,"原段payload范围");
        part.bytes.assign(bytes.begin()+static_cast<std::ptrdiff_t>(at),
                           bytes.begin()+static_cast<std::ptrdiff_t>(at+size));
        require(digest==ark::assets::sha256_hex(part.bytes),"夹具源分区摘要有效");
        at+=static_cast<std::size_t>(size);
        result.sections.push_back(std::move(part));
    }
    require(at==bytes.size()-64,"原容器没有未解析尾段");
    return result;
}
inline Bytes pack(const Wire &wire) {
    auto result=wire.prefix;
    u32(result,static_cast<std::uint32_t>(wire.sections.size()));
    for(const auto &section:wire.sections) {
        u32(result,section.id);u32(result,section.version);u32(result,section.required);u64(result,section.bytes.size());
        const auto hash=ark::assets::sha256_hex(section.bytes);
        result.insert(result.end(),hash.begin(),hash.end());
        result.insert(result.end(),section.bytes.begin(),section.bytes.end());
    }
    const auto hash=ark::assets::sha256_hex(result);
    result.insert(result.end(),hash.begin(),hash.end());
    return result;
}
inline Section &section(Wire &wire,std::uint32_t id) {
    const auto found=std::find_if(wire.sections.begin(),wire.sections.end(),[&](const auto &s){return s.id==id;});
    require(found!=wire.sections.end(),"夹具缺少所需分区");
    return *found;
}
struct ControlOffsets {std::size_t name{},decorations_count{},requests{},world{};};
inline ControlOffsets control_offsets(const Bytes &b) {
    ControlOffsets result;
    std::size_t at=12;
    (void)text(b,at);
    const auto name_size=number(b,at,4);result.name=at;
    require(name_size>0 && name_size<=b.size()-at,"合法姓名供控制字节损坏");
    at+=static_cast<std::size_t>(name_size)+12;
    const auto random=[&]{
        (void)number(b,at,8);(void)number(b,at,8);(void)number(b,at,4);
        const auto count=number(b,at,4);
        require(count<=(b.size()-at)/4,"随机磁带夹具边界");
        at+=static_cast<std::size_t>(count)*4;
    };
    random();result.requests=at;(void)number(b,at,8);
    result.decorations_count=at;
    const auto count=number(b,at,4);
    require(count<=(b.size()-at)/4,"人物名单夹具边界");at+=static_cast<std::size_t>(count)*4;
    const auto handoff=number(b,at,4);require(handoff<=1,"原handoff bool");if(handoff)random();
    result.world=at;(void)number(b,at,4);
    return result;
}
} // namespace application_replay_fixture
