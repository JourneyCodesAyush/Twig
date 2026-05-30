#pragma once

#include <optional>
#include <unordered_map>
#include <string>
#include <vector>

#include "../repository/repo.hpp"

namespace twig::ignore
{
    using RuleSet = std::vector<std::pair<std::string, bool>>;

    std::optional<std::pair<std::string, bool>> gitignore_parse1(std::string &raw);
    RuleSet gitignore_parse(std::vector<std::string> &lines);

    class GitIgnore
    {
    public:
        std::vector<RuleSet> absolute;
        std::unordered_map<std::string, RuleSet> scoped;
        GitIgnore(std::vector<RuleSet> absolute = {},
                  std::unordered_map<std::string, RuleSet> scoped = {}) : absolute(std::move(absolute)), scoped(std::move(scoped)) {}
    };

    GitIgnore gitignore_read(const repository::GitRepository &repo);

    std::optional<bool> check_ignore1(const RuleSet &rules, const std::string &path);
    std::optional<bool> check_ignore_scoped(const std::unordered_map<std::string, RuleSet> &rules, const std::string &path);
    bool check_ignore_absolute(const std::vector<RuleSet> &rules, const std::string &path);
    bool check_ignore(const GitIgnore &rules, const std::string &path);

} // namespace twig::ignore
