#pragma once

#include <string>

/**
 * @brief Repository discovery and path information.
 */
namespace twig::repository
{
    /**
     * @brief Represents a Git repository on disk.
     *
     * A GitRepository stores the filesystem locations of both the working
     * tree and the corresponding .git directory. Repository discovery
     * functions populate this structure and pass it throughout the codebase
     * so object, reference, and index operations can locate repository data.
     *
     * Example:
     * @code
     * worktree = "/home/user/project"
     * gitdir   = "/home/user/project/.git"
     * @endcode
     *
     * @note Both paths are stored as absolute filesystem paths.
     */
    struct GitRepository
    {
    public:
        /**
         * @brief Path to the repository working tree.
         */
        std::string worktree;

        /**
         * @brief Path to the repository's .git directory.
         */
        std::string gitdir;
    };
} // namespace twig::repository
