#pragma once

#include <string>

namespace twig::utils
{
    std::string sha1(const std::string &content);

    std::string compress(const std::string &content);
    std::string decompress(const std::string &content);

    std::string read_file_binary(const std::string &filename);
    void write_file_binary(const std::string &path, const std::string &message);
} // namespace twig::utils
