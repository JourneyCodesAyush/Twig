#pragma once

#include <string>

namespace twig::utils
{
    std::string sha1(const std::string &content);

    std::string compress(const std::string &content);
    std::string decompress(const std::string &content);

    std::string bytes_to_hex(const std::string &bytes);
    std::string hex_to_bytes(const std::string &hex);

    void write_file(const std::string &path, const std::string &message);
    std::string read_file(const std::string &filename);
    std::string read_file_binary(const std::string &filename);
    void write_file_binary(const std::string &path, const std::string &message);
} // namespace twig::utils
