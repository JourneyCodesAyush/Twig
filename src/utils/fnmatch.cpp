#include "../include/utils/utils.hpp"

#include <fnmatch.h>

namespace twig::utils
{
    bool fnmatch(std::string path, std::string pattern)
    {
        return ::fnmatch(pattern.c_str(), path.c_str(), 0) == 0;
    }
}
