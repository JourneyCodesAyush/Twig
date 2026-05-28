#pragma once

#include <memory>
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

    std::vector<std::string> object_resolve(const GitRepository &repo, const std::string &name);
    std::optional<std::string> object_find(const GitRepository &repo, std::string name, std::string format = "", bool follow = true);

    std::string object_write(objects::GitObject *obj, const GitRepository *repo = nullptr);
    std::unique_ptr<objects::GitObject> object_read(const GitRepository &repo, const std::string &sha);

    std::optional<std::string> ref_resolve(const GitRepository &repo, const std::string &ref);
    std::vector<std::pair<std::string, std::string>> ref_list(const GitRepository &repo, std::optional<std::string> path = std::nullopt);
} // namespace twig::repository
