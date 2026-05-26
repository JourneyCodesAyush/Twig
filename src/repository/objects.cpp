#include <string>
#include <optional>
#include <filesystem>
#include <fstream>
#include <vector>
#include <exception>

#include "../include/repository/objects.hpp"

namespace twig::repository
{
    namespace fs = std::filesystem;

    // Anonymous and hidden implementation
    namespace
    {
        void write_file(const std::string &path, const std::string &message)
        {
            std::ofstream file(path);
            if (file.is_open())
            {
                file << message;
                file.close();
            }
            else
            {
                throw std::runtime_error("Failed to create " + path);
            }
        }
    } // namespace

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
                write_file(*file, description);
        }
        {
            // .git/HEAD
            auto file = repo_file(repo, false, {"HEAD"});
            std::string description = "ref: refs/heads/master\n";
            if (file)
                write_file(*file, description);
        }
        {
            // .git/config
            auto file = repo_file(repo, false, {"config"});
            std::string description = "[core]\n";
            description += "\trepositoryformatversion=0\n";
            description += "\tfilemode=true\n";
            description += "\tbare=false\n";

            if (file)
                write_file(*file, description);
        }

        return repo;
    }

} // namespace twig::repository
