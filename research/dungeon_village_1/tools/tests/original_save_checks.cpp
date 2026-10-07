// 原档外壳／容器的独立手工夹具，属于既有严格二进制工具套件，不冒充真实存档。
#include "dungeon_village_tools/original_save.hpp"
#include <stdexcept>
#include <string>

using namespace dungeon_village_tools;
namespace {
std::vector<std::uint8_t> hex(const std::string &text) {
    std::vector<std::uint8_t> result;
    for (std::size_t i = 0; i < text.size(); i += 2)
        result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(i, 2), nullptr, 16)));
    return result;
}
void require(bool passed, const std::string &scenario, int &checks) {
    ++checks;
    if (!passed)
        throw std::runtime_error("原档检查失败: " + scenario);
}
template <typename F> void rejects(F function, const std::string &scenario, int &checks) {
    bool rejected = false;
    try {
        function();
    } catch (const std::exception &) {
        rejected = true;
    }
    require(rejected, scenario, checks);
}
} // namespace

int original_save_checks() {
    int checks = 0;
    const auto fixture = hex("00000005000000000000000000000001000000020000006c00001388"
                             "00000002000000010102030405060708000000030000000100000004e4b8adff"
                             "00000004000000020000000700010203fffefd0000000300807f");
    const auto original = fixture;
    const auto parsed = inspect_original_save(fixture, OriginalSaveFormat::container);
    require(parsed.container_bytes == original && fixture == original, "保留全部字节、不修改输入",
            checks);
    require(parsed.fields.size() == 11, "标签与六字段均列出", checks);
    const auto find = [&](const std::string &path) -> const OriginalSaveField & {
        for (const auto &field : parsed.fields)
            if (field.path == path)
                return field;
        throw std::runtime_error("缺夹具字段: " + path);
    };
    require(find("$/1/1").value == "5000", "大端int32", checks);
    require(find("$/2/0").value == "72623859790382856", "大端int64", checks);
    require(find("$/3/0").value == "e4b8adff", "非法UTF8仍保留原字节", checks);
    require(find("$/4/0").size == 7 && find("$/4/1").size == 3, "不透明段边界", checks);
    require(diff_original_saves(parsed, parsed).empty(), "同档无差异", checks);
    auto changed = fixture;
    changed[26] = 0x13;
    changed[27] = 0xec; // 手工5000 -> 5100；其余完全不变。
    const auto delta =
        diff_original_saves(parsed, inspect_original_save(changed, OriginalSaveFormat::container));
    require(delta.size() == 1 && delta[0] == "$/1/1\tchanged\t5000\t5100", "只定位一个已知字段变化",
            checks);
    for (std::size_t n = 0; n < fixture.size(); ++n) {
        rejects(
            [&] {
                inspect_original_save(
                    {fixture.begin(), fixture.begin() + static_cast<std::ptrdiff_t>(n)},
                    OriginalSaveFormat::container);
            },
            "每个字节截断点", checks);
    }
    for (const auto &bad :
         {hex("000000010000000500000000"), hex("0000000200000001000000000000000100000000"),
          hex("0000000100000001ffffffff"), hex("0000000100000004000000017fffffff"),
          hex("0000000000"), hex("0000000600000000")})
        rejects([&] { inspect_original_save(bad, OriginalSaveFormat::container); },
                "未知/重复标签、负/超长/尾字节", checks);
    const auto signed_values = inspect_original_save(
        hex("0000000200000001000000018000000000000002000000018000000000000000"),
        OriginalSaveFormat::container);
    require(signed_values.fields[1].value == "-2147483648" &&
                signed_values.fields[3].value == "-9223372036854775808",
            "有符号最小值", checks);
    const auto nested = inspect_original_save(hex("0000000100000000000000010000000400000000"),
                                              OriginalSaveFormat::container);
    require(nested.fields.size() == 2 && nested.fields[1].type == "container", "长度限定子容器",
            checks);
    auto deep = hex("00000000");
    for (unsigned i = 0; i < 34; ++i) {
        auto parent = hex("00000001000000000000000100000000");
        const auto n = deep.size();
        for (unsigned byte = 0; byte < 4; ++byte)
            parent[12 + byte] = static_cast<std::uint8_t>(n >> ((3 - byte) * 8U));
        parent.insert(parent.end(), deep.begin(), deep.end());
        deep = std::move(parent);
    }
    rejects([&] { inspect_original_save(deep, OriginalSaveFormat::container); }, "递归深度预算",
            checks);
    rejects(
        [&] {
            inspect_original_save(std::vector<std::uint8_t>(16U * 1024U * 1024U + 1),
                                  OriginalSaveFormat::container);
        },
        "总字节预算", checks);
    auto many = hex("000000010000000100020000"); // 131072个int，连同组头超过总行预算。
    many.resize(many.size() + 131072U * 4U);
    rejects([&] { inspect_original_save(many, OriginalSaveFormat::container); }, "全局行预算",
            checks);
    const auto apk = hex("65b648ad85f7d0ecfdaa4be9");
    const auto steam = hex("08070605fbfcfdfe08070605");
    constexpr std::uint64_t synthetic_id = 0x0102030405060708ULL;
    // 以下固定字节由独立标量递推计算：payload=四个零、gameCRC=FFFFFFFF。
    require(inspect_original_save(apk, OriginalSaveFormat::apk_record).container_bytes ==
                hex("00000000"),
            "APK44字节key解壳", checks);
    require(inspect_original_save(steam, OriginalSaveFormat::steam_record, synthetic_id)
                    .container_bytes == hex("00000000"),
            "Steam8字节小端key解壳", checks);
    const auto null_blob =
        inspect_original_save(hex("08070605581145c1080706040403020508070604fbfcfdfe"),
                              OriginalSaveFormat::steam_record, synthetic_id);
    require(null_blob.fields.size() == 2 && null_blob.fields[1].type == "null-bytes",
            "Steam null与空bytes区分", checks);
    rejects(
        [&] {
            inspect_original_save(hex("000000010000000400000001ffffffff"),
                                  OriginalSaveFormat::container);
        },
        "APK不接受Steam null载荷", checks);
    const std::string base64 = "ZbZIrYX30Oz9qkvp\r\n";
    require(inspect_original_save({base64.begin(), base64.end()}, OriginalSaveFormat::apk_base64)
                    .container_bytes == hex("00000000"),
            "APK Base64先于XOR", checks);
    rejects([&] { inspect_original_save(steam, OriginalSaveFormat::apk_record); },
            "平台key不能混用", checks);
    rejects([&] { inspect_original_save(steam, OriginalSaveFormat::steam_record); },
            "Steam缺身份明确拒绝", checks);
    rejects(
        [&] { inspect_original_save(steam, OriginalSaveFormat::steam_record, synthetic_id + 1); },
        "错误身份校验失败", checks);
    rejects([&] { inspect_original_save(apk, OriginalSaveFormat::apk_record, synthetic_id); },
            "APK拒绝Steam参数", checks);
    auto corrupt = apk;
    corrupt.back() ^= 1;
    rejects([&] { inspect_original_save(corrupt, OriginalSaveFormat::apk_record); }, "损坏校验失败",
            checks);
    for (const std::string bad : {"A", "AA=A", "AA==AAAA", "AB==", "AAB=", "!!!!", "H4s="})
        rejects(
            [&] {
                inspect_original_save({bad.begin(), bad.end()}, OriginalSaveFormat::apk_base64);
            },
            "Base64非法/未接GZIP分支", checks);
    require(inspect_original_save({}, OriginalSaveFormat::steam_record, synthetic_id).empty_record,
            "空记录不伪造新局", checks);
    rejects([&] { inspect_original_save({}, OriginalSaveFormat::container); }, "空容器拒绝",
            checks);
    return checks;
}
