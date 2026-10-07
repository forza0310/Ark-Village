#pragma once

// Portable SHA-256 for maintained file integrity and evidence identities.
// This target provides no archive extraction, game decryption or image APIs.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ark::assets {
std::string sha256_hex(const std::uint8_t *data, std::size_t size);
std::string sha256_hex(const std::vector<std::uint8_t> &data);
std::string sha256_hex(const std::string &text);
} // namespace ark::assets
