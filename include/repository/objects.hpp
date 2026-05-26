#pragma once

#include <string>
#include <optional>
#include <vector>

#include "./repo.hpp"

namespace twig::repository
{
    std::string repo_path(const GitRepository &repo, const std::vector<std::string> &path = {});

    std::optional<std::string> repo_file(const GitRepository &repo, bool mkdir = false, const std::vector<std::string> &path = {});

    std::optional<std::string> repo_dir(const GitRepository &repo, bool mkdir = false, const std::vector<std::string> &path = {});

    GitRepository repo_create(const std::string &path);

} // namespace twig::repository
