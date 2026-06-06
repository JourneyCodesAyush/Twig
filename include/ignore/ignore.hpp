/**
 * @file ignore.hpp
 * @brief Git ignore rule parsing and evaluation.
 *
 * This module implements .gitignore semantics, including:
 *
 * - Parsing ignore patterns from files.
 * - Supporting absolute (repository-wide) rules.
 * - Supporting scoped (directory-specific) rules.
 * - Evaluating whether a given path should be ignored.
 *
 * Rules follow Git-style pattern matching conventions and are evaluated
 * in order of precedence, with later rules overriding earlier ones.
 *
 * Note: Pattern matching is delegated to utils::fnmatch.
 */

#pragma once

#include <optional>
#include <unordered_map>
#include <string>
#include <vector>

#include "../repository/repo.hpp"

namespace twig::ignore
{
    /**
     * @brief A sequence of ignore patterns.
     *
     * Each rule is a pair:
     * - glob pattern (Git-style)
     * - negation flag (true = "!pattern", re-includes a path)
     */
    using RuleSet = std::vector<std::pair<std::string, bool>>;

    /**
     * @brief Parse a single ignore pattern line.
     *
     * Returns std::nullopt if the line is empty or a comment.
     */
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

    /**
     * @return
     * - std::nullopt : rule set does not match the path
     * - true         : path is ignored
     * - false        : path is explicitly included (negation rule)
     */
    std::optional<bool> check_ignore1(const RuleSet &rules, const std::string &path);

    /**
     * @brief Evaluate repository-wide ignore rules.
     */
    std::optional<bool> check_ignore_scoped(const std::unordered_map<std::string, RuleSet> &rules, const std::string &path);
    bool check_ignore_absolute(const std::vector<RuleSet> &rules, const std::string &path);

    /**
     * @brief Determine whether a path should be ignored.
     *
     * Combines absolute and scoped rules according to Git ignore precedence.
     */
    bool check_ignore(const GitIgnore &rules, const std::string &path);

} // namespace twig::ignore
