#pragma once

// Strict metadata subset adapted from research/tools: retain empty TSV fields and validate UTF-8.
#include <cstdint>
#include <string>
#include <vector>
namespace ark::assets {
using TableRows = std::vector<std::vector<std::string>>;
TableRows parse_tsv(const std::vector<std::uint8_t> &bytes);
std::int32_t parse_table_integer(const std::string &field);
} // namespace ark::assets
