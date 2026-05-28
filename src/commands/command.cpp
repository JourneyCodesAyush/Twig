#include <memory>
#include <unordered_set>
#include <filesystem>

#include "../include/commands/command.hpp"
#include "../include/repository/objects.hpp"
#include "../include/utils/utils.hpp"
#include "../include/errors/error.hpp"

namespace twig::commands
{
    namespace fs = std::filesystem;
    namespace
    {
        void cat_file(const repository::GitRepository &repo, const std::string &sha, const std::string &type, std::string format = "")
        {
            std::unique_ptr<objects::GitObject> object = repository::object_read(repo, repository::object_find(repo, sha, format));
            std::cout << object->serialize() << "\n";
        }

        void log_graphviz(repository::GitRepository &repo, const std::string &sha, std::unordered_set<std::string> &seen)
        {
            if (seen.find(sha) != seen.end())
                return;
            seen.insert(sha);

            std::unique_ptr<objects::GitObject> obj = repository::object_read(repo, sha);

            objects::GitCommit *commit = dynamic_cast<objects::GitCommit *>(obj.get());
            if (!commit)
                throw errors::GitException("Not a commit object", errors::ExitCode::MALFORMED_OBJECT);

            auto kvlm = commit->kvlm;
            std::string message;
            for (const auto &[key, value] : kvlm)
            {
                if (key == "None")
                    message = value;
            }

            {
                // Replacing '\\' with '\\\\'
                size_t i = 0;
                while ((i = message.find("\\", i)) != std::string::npos)
                {
                    message.replace(i, 1, "\\\\");
                    i += 2;
                }
            }
            {
                // Replacing '\"' with '\\\"'
                size_t i = 0;
                while ((i = message.find("\"", i)) != std::string::npos)
                {
                    message.replace(i, 1, "\\\"");
                    i += 1;
                }
            }

            if (message.find_first_of("\n") != std::string::npos)
            {
                message = message.substr(0, message.find_first_of("\n"));
            }

            // print(f "  c_{sha} [label=\"{sha[0:7]}: {message}\"]")
            std::cout
                << "  c_"
                << sha
                << " [label=\""
                << sha.substr(0, 7)
                << ": "
                << message
                << "\"]";

            if (commit->format != "commit")
                return;

            std::vector<std::string> parents;
            bool found_parent = false;
            for (size_t i = 0; i < kvlm.size(); i++)
            {
                if (kvlm[i].first == "parent")
                {
                    found_parent = true;
                    parents.push_back(kvlm[i].second);
                }
            }

            // Base case
            if (!found_parent)
                return;

            for (const auto &parent_sha : parents)
            {
                std::cout << "  c_" << sha << " -> c_" << parent_sha << ";\n";
                log_graphviz(repo, parent_sha, seen);
            }
        }

        void ls_tree(repository::GitRepository &repo, const std::string &sha, bool recursive, const std::string &prefix = "")
        {
            std::unique_ptr<objects::GitObject> obj = repository::object_read(repo, sha);

            objects::GitTree *tree = dynamic_cast<objects::GitTree *>(obj.get());
            if (!tree)
                throw errors::GitException("Not a tree object", errors::ExitCode::MALFORMED_OBJECT);

            for (const auto &leaf : tree->leaves)
            {
                std::string type = leaf.mode.substr(0, 2);

                if (type == "04")
                    type = "tree";
                // A regular file
                else if (type == "10")
                    type = "blob";
                // A symlink. Blob contents in link target
                else if (type == "12")
                    type = "blob";
                // A submodule
                else if (type == "16")
                    type = "commit";
                else
                    throw errors::GitException("Weird tree leaf mode: " + leaf.mode, errors::ExitCode::MALFORMED_OBJECT);

                if (!recursive || type != "tree")
                    std::cout
                        << leaf.mode
                        << " "
                        << type
                        << " "
                        << leaf.sha
                        << "\t"
                        << (fs::path(prefix) / leaf.path).string()
                        << "\n";
                else
                    ls_tree(repo, leaf.sha, recursive, (fs::path(prefix) / fs::path(leaf.path)).string());
            }
        }
    } // namespace

    errors::ExitCode cmd_init(const ParseResult &args)
    {
        auto dir = args.get<std::string>("directory");
        std::cout << "[init] directory: " << dir << "\n";

        repository::GitRepository repo = repository::repo_create(dir);

        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_hash_object(const ParseResult &args)
    {
        std::string path = args.get<std::string>("path");
        bool write = false;
        if (args.has("w"))
        {
            write = args.get<bool>("w");
        }

        std::optional<repository::GitRepository> repo;

        if (write)
            repo = repository::repo_find();
        else
            repo = std::nullopt;

        std::string content = utils::read_file_binary(path);

        std::unique_ptr<objects::GitObject> obj = std::make_unique<objects::GitBlob>(content);

        std::string sha = repository::object_write(obj.get(), repo ? &(*repo) : nullptr);
        std::cout << sha << "\n";

        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_cat_file(const ParseResult &args)
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
        {
            throw errors::GitException("Not a repository.", errors::ExitCode::NOT_A_REPO);
        }
        std::string type = args.get<std::string>("type");
        std::string object = args.get<std::string>("object");
        cat_file(*repo, object, type);

        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_log(const ParseResult &args)
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
        {
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);
        }

        std::string commit = args.get<std::string>("commit");

        std::string sha;
        if (commit == "HEAD")
        {
            std::string path = (fs::path((*repo).gitdir) / "HEAD").string();
            sha = utils::read_file(path);
        }
        else
        {
            sha = commit;
        }

        if (sha.substr(0, 5) == "ref: ")
        {
            sha = sha.substr(5);
            if (!sha.empty() && sha.back() == '\n')
                sha.pop_back();
            std::string ref_path = (fs::path((*repo).gitdir) / sha).string();
            sha = utils::read_file(ref_path);
        }

        // Strip the newline
        if (!sha.empty() && sha.back() == '\n')
            sha.pop_back();

        std::cout << "digraph wyaglog{\n"
                  << "  node[shape=rect]\n";

        std::unordered_set<std::string> seen;

        // Will use this later
        // std::string sha = repository::object_find(*repo, args.get<std::string>("commit"));

        log_graphviz((*repo), sha, seen);
        std::cout << "}\n";
        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_ls_tree(const ParseResult &args)
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);

        bool recursive = args.get<bool>("r");
        std::string tree = args.get<std::string>("tree");

        ls_tree(*repo, tree, recursive);

        return errors::ExitCode::SUCCESS;
    }

} // namespace twig::commands
