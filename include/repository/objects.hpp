#pragma once

#include <string>
#include <optional>
#include <vector>

#include "./repo.hpp"
#include "../objects/object.hpp"

namespace twig::repository
{
    std::string repo_path(const GitRepository &repo, const std::vector<std::string> &path = {});

    std::optional<std::string> repo_file(const GitRepository &repo, bool mkdir = false, const std::vector<std::string> &path = {});

    std::optional<std::string> repo_dir(const GitRepository &repo, bool mkdir = false, const std::vector<std::string> &path = {});

    GitRepository repo_create(const std::string &path);

    std::optional<GitRepository> repo_find(const std::string &path = ".", bool required = true);

    std::string object_write(objects::GitObject *obj, const GitRepository *repo = nullptr);
} // namespace twig::repository
