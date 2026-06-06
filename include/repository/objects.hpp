/**
 * @file objects.hpp
 * @brief Repository discovery, object storage, and reference utilities.
 *
 * This module provides the core repository operations used throughout Twig:
 *
 * - Repository creation and discovery.
 * - Path resolution within the .git directory.
 * - Reading and writing Git objects.
 * - Resolving object names and references.
 * - Enumerating repository references.
 *
 * Most higher-level commands interact with the repository through these
 * functions rather than accessing the filesystem directly.
 */

#pragma once

#include <memory>
#include <string>
#include <optional>
#include <vector>

#include "./repo.hpp"
#include "../objects/object.hpp"

namespace twig::repository
{
    /**
     * @brief Join path components relative to a repository.
     *
     * @param repo Repository root information.
     * @param path Additional path components.
     * @return Absolute path inside the repository.
     */
    std::string repo_path(const GitRepository &repo, const std::vector<std::string> &path = {});

    std::optional<std::string> repo_file(const GitRepository &repo, bool mkdir = false, const std::vector<std::string> &path = {});

    /**
     * @brief Ensure a directory exists within the repository.
     *
     * If mkdir is true, missing directories are created.
     *
     * @return Path to the directory on success, std::nullopt on failure.
     */
    std::optional<std::string> repo_dir(const GitRepository &repo, bool mkdir = false, const std::vector<std::string> &path = {});

    /**
     * @brief Create a new repository at the specified path.
     *
     * Initializes the .git directory structure and repository metadata.
     */
    GitRepository repo_create(const std::string &path);

    /**
     * @brief Locate the nearest repository by walking parent directories.
     *
     * Starting from path, searches upward until a .git directory is found.
     *
     * @param required If true, failure results in an exception; otherwise
     *                 std::nullopt is returned.
     */
    std::optional<GitRepository> repo_find(const std::string &path = ".", bool required = true);

    /**
     * @brief Resolve a user-supplied revision name to candidate object IDs.
     *
     * Supports full SHA-1s, abbreviated SHA-1s, tags, branches, and other
     * reference names.
     */
    std::vector<std::string> object_resolve(const GitRepository &repo, const std::string &name);

    /**
     * @brief Resolve a revision name to a single object.
     *
     * Optionally follows tags and commit indirections until an object of
     * the requested format is reached.
     */
    std::optional<std::string> object_find(const GitRepository &repo, std::string name, std::string format = "", bool follow = true);

    /**
     * @brief Serialize, hash, and optionally store an object.
     *
     * When repo is nullptr, the object ID is computed without writing the
     * object to disk.
     */
    std::string object_write(objects::GitObject *obj, const GitRepository *repo = nullptr);

    /**
     * @brief Read and deserialize an object from the object database.
     */
    std::unique_ptr<objects::GitObject> object_read(const GitRepository &repo, const std::string &sha);

    std::optional<std::string> ref_resolve(const GitRepository &repo, const std::string &ref);
    std::vector<std::pair<std::string, std::string>> ref_list(const GitRepository &repo, std::optional<std::string> path = std::nullopt);
} // namespace twig::repository
