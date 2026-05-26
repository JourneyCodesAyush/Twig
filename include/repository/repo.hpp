#pragma once

#include <string>

namespace twig::repository
{
    struct GitRepository
    {
    public:
        std::string worktree;
        std::string gitdir;
    };
} // namespace twig::repository
