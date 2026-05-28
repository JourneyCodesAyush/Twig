#include <algorithm>
#include <string>
#include <optional>
#include <filesystem>
#include <fstream>
#include <vector>
#include <exception>

#include "../include/repository/objects.hpp"
#include "../include/utils/utils.hpp"
#include "../include/errors/error.hpp"

namespace twig::repository
{
    namespace fs = std::filesystem;

    std::string repo_path(const GitRepository &repo, const std::vector<std::string> &path)
    {
        fs::path join_path;
        for (const std::string &pth : path)
        {
            join_path = join_path / fs::path(pth);
        }

        fs::path final_path = fs::path(repo.gitdir) / join_path;

        return final_path.string();
    }

    std::optional<std::string> repo_dir(const GitRepository &repo, bool mkdir, const std::vector<std::string> &path)
    {
        std::string str_path = repo_path(repo, path);

        if (fs::exists(str_path))
        {
            if (fs::is_directory(str_path))
            {
                return str_path;
            }
            throw std::runtime_error("Not a directory " + str_path);
        }

        if (mkdir == true)
        {
            fs::create_directories(str_path);
            return str_path;
        }
        else
        {
            return std::nullopt;
        }
    }

    std::optional<std::string> repo_file(const GitRepository &repo, bool mkdir, const std::vector<std::string> &path)
    {
        if (!path.empty())
        {
            if (repo_dir(repo, mkdir, std::vector<std::string>(path.begin(), path.end() - 1)))
            {
                return repo_path(repo, path);
            }
        }
        return std::nullopt;
    }

    GitRepository repo_create(const std::string &path)
    {
        GitRepository repo;

        repo.worktree = path;
        repo.gitdir = (fs::path(path) / ".git").string();

        if (fs::exists(repo.worktree))
        {
            if (!fs::is_directory(repo.worktree))
                throw std::runtime_error(repo.worktree + " is not a directory!");
            else if (fs::is_directory(repo.worktree) && !fs::is_empty(repo.worktree))
                throw std::runtime_error(path + " is not empty!");
        }
        else
        {
            fs::create_directory(repo.worktree);
        }

        fs::create_directory(repo.gitdir);

        repo_dir(repo, true, {"branches"});
        repo_dir(repo, true, {"objects"});
        repo_dir(repo, true, {"refs", "tags"});
        repo_dir(repo, true, {"refs", "heads"});

        {
            // .git/description
            auto file = repo_file(repo, false, {"description"});
            std::string description = "Unnamed repository; edit this file 'description' to name the repository.\n";
            if (file)
                utils::write_file(*file, description);
        }
        {
            // .git/HEAD
            auto file = repo_file(repo, false, {"HEAD"});
            std::string description = "ref: refs/heads/master\n";
            if (file)
                utils::write_file(*file, description);
        }
        {
            // .git/config
            auto file = repo_file(repo, false, {"config"});
            std::string description = "[core]\n";
            description += "\trepositoryformatversion=0\n";
            description += "\tfilemode=true\n";
            description += "\tbare=false\n";

            if (file)
                utils::write_file(*file, description);
        }

        return repo;
    }

    std::optional<GitRepository> repo_find(const std::string &path, bool required)
    {
        fs::path absolute_path = fs::absolute(path);

        if (fs::is_directory(absolute_path / ".git"))
        {
            GitRepository repo;
            repo.worktree = absolute_path.string();
            repo.gitdir = (absolute_path / ".git").string();
            return repo;
        }

        fs::path parent = absolute_path.parent_path();

        if (absolute_path == parent)
        {
            if (required)
                throw std::runtime_error("No git directory.");
            else
                return std::nullopt;
        }
        return repo_find(parent.string(), required);
    }

    std::string object_write(objects::GitObject *obj, const GitRepository *repo)
    {
        const std::string data = obj->serialize();
        std::string result = obj->format + " ";
        result += std::to_string(data.length());
        result += std::string(1, '\0');
        result += data;

        std::string sha1 = utils::sha1(result);

        if (repo)
        {
            std::optional<std::string> path = repo_file(*repo, true, {"objects", sha1.substr(0, 2), sha1.substr(2)});

            if (path && !fs::exists(*path))
            {
                const std::string compressed = utils::compress(result);
                utils::write_file_binary(*path, compressed);
            }
        }
        return sha1;
    }

    std::string object_find(const GitRepository &repo, std::string name, std::string format, bool follow)
    {
        return name;
    }

    std::unique_ptr<objects::GitObject> object_read(const GitRepository &repo, const std::string &sha)
    {
        std::optional<std::string> path_optional = repo_file(repo, false, {"objects", sha.substr(0, 2), sha.substr(2)});

        if (!path_optional)
        {
            throw errors::GitException("Object not found.", errors::ExitCode::OBJECT_NOT_FOUND);
        }

        const std::string path = *path_optional;

        if (!fs::is_regular_file(path))
        {
            throw errors::GitException("Object not found.", errors::ExitCode::OBJECT_NOT_FOUND);
        }

        std::string raw = utils::decompress(utils::read_file_binary(path));

        size_t x = raw.find_first_of(' ');
        const std::string &format = raw.substr(0, x);

        size_t y = raw.find_first_of('\0', x);

        size_t size = std::stoi(raw.substr(x, y - x));

        if (size != (raw.length() - y - 1))
        {
            throw errors::GitException("Malformed object " + sha, errors::ExitCode::MALFORMED_OBJECT);
        }

        if (format == "blob")
        {
            return std::make_unique<objects::GitBlob>(raw.substr(y + 1));
        }
        if (format == "commit")
        {
            return std::make_unique<objects::GitCommit>(raw.substr(y + 1));
        }
        if (format == "tree")
        {
            return std::make_unique<objects::GitTree>(raw.substr(y + 1));
        }

        throw errors::GitException("Unknown object type", errors::ExitCode::UNKNOWN_OBJECT_TYPE);
    }

    std::optional<std::string> ref_resolve(const GitRepository &repo, const std::string &ref)
    {
        if (!fs::is_regular_file(ref))
            return std::nullopt;

        std::string data = utils::read_file(ref);
        // Drop the \n
        if (data.size() > 0 && data.back() == '\n')
            data.pop_back();

        if (data.rfind("ref: ", 0) == 0)
            return ref_resolve(repo, (fs::path(repo.gitdir) / data.substr(5)).string());
        else
            return data;
    }

    std::vector<std::pair<std::string, std::string>> ref_list(const GitRepository &repo, std::optional<std::string> path)
    {
        if (!path)
            path = repo_dir(repo, false, {"refs"});

        std::vector<std::pair<std::string, std::string>> list;

        // Sort the entries in the directory
        std::vector<fs::directory_entry> entries;
        for (const auto &entry : fs::directory_iterator(*path))
            entries.push_back(entry);
        std::sort(entries.begin(), entries.end());

        for (const auto &entry : entries)
        {
            fs::path can = entry.path();
            if (fs::is_directory(can))
            {
                auto sub = ref_list(repo, can.string());
                list.insert(list.end(), sub.begin(), sub.end());
            }
            else
            {
                std::optional<std::string> sha = ref_resolve(repo, can);
                if (!sha)
                    throw errors::GitException("Malformed object", errors::ExitCode::MALFORMED_OBJECT);
                list.push_back({can.string(), *sha});
            }
        }

        return list;
    }
} // namespace twig::repository
