#include "../include/utils/utils.hpp"

#include <fnmatch.h>

namespace twig::utils
{
    // Thin wrapper over POSIX fnmatch(3).
    // Argument order is (path, pattern) — note this is the reverse of the raw POSIX call.
    bool fnmatch(std::string path, std::string pattern)
    {
        return ::fnmatch(pattern.c_str(), path.c_str(), 0) == 0;
    }
}
