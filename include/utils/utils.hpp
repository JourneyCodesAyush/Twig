#pragma once

#include <string>

namespace twig::utils
{
    std::string sha1(const std::string &content);

    std::string compress(const std::string &content);
    std::string decompress(const std::string &content);

    std::string read_file_binary(const std::string &filename);
} // namespace twig::utils
