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
} // namespace twig::ignore
