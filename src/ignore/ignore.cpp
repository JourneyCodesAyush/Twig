#include <fstream>
#include <sstream>
#include <filesystem>

#include "../include/ignore/ignore.hpp"
#include "../include/repository/objects.hpp"
#include "../include/index/index.hpp"
#include "../include/utils/utils.hpp"
#include "../include/errors/error.hpp"

namespace twig::ignore
{
    namespace fs = std::filesystem;

    std::optional<std::pair<std::string, bool>> gitignore_parse1(std::string &raw)
    {
        raw = utils::strip(raw);
        if (raw.empty() || raw[0] == '#')
            return std::nullopt;
        else if (raw[0] == '!')
            return std::make_pair<std::string, bool>(raw.substr(1), false); // Do NOT ignore
        else if (raw[0] == '\\')
            return std::make_pair<std::string, bool>(raw.substr(1), true); // Ignore
        else
            return std::make_pair<std::string, bool>(raw.substr(0), true); // Ignore
    }

    RuleSet gitignore_parse(std::vector<std::string> &lines)
    {
        RuleSet list;

        for (std::string &line : lines)
        {
            std::optional<std::pair<std::string, bool>> rule = gitignore_parse1(line);
            if (rule)
                list.push_back(*rule);
        }
        return list;
    }

    GitIgnore gitignore_read(const repository::GitRepository &repo)
    {
        GitIgnore ret;

        std::string repo_file = (fs::path(repo.gitdir) / "info" / "exclude").string();

        // Local configuration in .git/info/exclude
        if (fs::exists(repo_file))
        {
            std::ifstream file(repo_file);
            if (!file.is_open())
                throw errors::GitException("Could not read: " + repo_file, errors::ExitCode::IO_ERROR);

            std::vector<std::string> lines;
            std::string line;
            // std::getline returns the stream, which evaluates to false on EOF or error
            while (std::getline(file, line))
                lines.push_back(line);

            RuleSet rule = gitignore_parse(lines);
            ret.absolute.push_back(std::move(rule));

            file.close();
        }

        // Global configuration
        const char *xdg = std::getenv("XDG_CONFIG_HOME");
        std::string config_home = xdg ? xdg : (fs::path(std::getenv("HOME")) / ".config").string();

        std::string global_file = (fs::path(config_home) / "git" / "ignore").string();
        if (fs::exists(global_file))
        {
            std::ifstream file(global_file);
            if (!file.is_open())
                throw errors::GitException("Could not read: " + global_file, errors::ExitCode::IO_ERROR);

            std::vector<std::string> lines;
            std::string line;
            // std::getline returns the stream, which evaluates to false on EOF or error
            while (std::getline(file, line))
                lines.push_back(line);

            RuleSet rule = gitignore_parse(lines);
            ret.absolute.push_back(std::move(rule));

            file.close();
        }

        index::GitIndex index = index::index_read(repo);
        for (const auto &entry : index.entries)
        {
            if (entry.name == ".gitignore" ||
                (entry.name.size() >= 10 && entry.name.rfind("/.gitignore") == entry.name.size() - 11))
            {
                std::string dir_name = fs::path(entry.name).parent_path().string();
                std::unique_ptr<twig::objects::GitObject> obj = repository::object_read(repo, entry.sha);

                objects::GitBlob *blob = dynamic_cast<objects::GitBlob *>(obj.get());

                if (!blob)
                    throw errors::GitException("Malformed object found reading index", errors::ExitCode::MALFORMED_OBJECT);

                std::string content = blob->blobdata;

                std::vector<std::string> lines;
                std::istringstream stream(content);
                std::string line;
                while (std::getline(stream, line))
                    lines.push_back(line);

                RuleSet rule = gitignore_parse(lines);
                ret.scoped[dir_name] = std::move(rule);
            }
        }
        return ret;
    }

    std::optional<bool> check_ignore1(const RuleSet &rules, const std::string &path)
    {
        std::optional<bool> result = std::nullopt;
        for (const auto &[pattern, value] : rules)
        {
            if (utils::fnmatch(path, pattern))
                result = value;
        }
        return result;
    }

    std::optional<bool> check_ignore_scoped(const std::unordered_map<std::string, RuleSet> &rules, const std::string &path)
    {
        std::optional<bool> result = std::nullopt;
        std::string parent = fs::path(path).parent_path().string();
        while (1)
        {
            if (rules.count(parent) != 0)
            {
                result = check_ignore1(rules.at(parent), path);
                if (result)
                    return result;
            }
            std::string next = fs::path(parent).parent_path().string();
            if (next == parent || next.empty())
                break;
            parent = next;
        }
        return std::nullopt;
    }

    bool check_ignore_absolute(const std::vector<RuleSet> &rules, const std::string &path)
    {
        for (const auto &ruleset : rules)
        {
            std::optional<bool> result = check_ignore1(ruleset, path);
            if (result)
                return *result;
        }
        return false;
    }

    bool check_ignore(const GitIgnore &rules, const std::string &path)
    {
        if (fs::path(path).is_absolute())
            throw errors::GitException("This function requires path to be relative to the repository's root", errors::ExitCode::FAILURE);

        std::optional<bool> result = check_ignore_scoped(rules.scoped, path);
        if (result)
            return *result;

        return check_ignore_absolute(rules.absolute, path);
    }

} // namespace twig::ignore
