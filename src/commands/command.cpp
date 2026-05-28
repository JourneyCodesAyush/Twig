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
            std::optional<std::string> new_sha = repository::object_find(repo, sha, format);

            if (!new_sha)
                throw errors::GitException("Object not found: " + sha, errors::ExitCode::OBJECT_NOT_FOUND);

            std::unique_ptr<objects::GitObject> object = repository::object_read(repo, *new_sha);
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

        void tree_checkout(const repository::GitRepository &repo, const objects::GitTree &object, const std::string &path)
        {
            for (const auto &leaf : object.leaves)
            {
                std::unique_ptr<twig::objects::GitObject> obj = repository::object_read(repo, leaf.sha);
                std::string destination = (fs::path(path) / leaf.path).string();

                if (obj->format == "tree")
                {
                    fs::create_directory(destination);

                    objects::GitTree *tree = dynamic_cast<objects::GitTree *>(obj.get());
                    if (!tree)
                        throw errors::GitException("Not a tree object", errors::ExitCode::MALFORMED_OBJECT);

                    tree_checkout(repo, *tree, destination);
                }
                else if (obj->format == "blob")
                {
                    objects::GitBlob *blob = dynamic_cast<objects::GitBlob *>(obj.get());
                    if (!blob)
                        throw errors::GitException("Not a blob object", errors::ExitCode::MALFORMED_OBJECT);

                    utils::write_file_binary(destination, blob->blobdata);
                }
            }
        }

        void show_ref(const repository::GitRepository &repo, const std::vector<std::pair<std::string, std::string>> &refs, bool with_hash = true)
        {
            for (const auto &[path, sha] : refs)
                std::cout
                    << (with_hash ? sha + " " : "")
                    << path.substr(repo.gitdir.length() + 1)
                    << "\n";
        }

        void ref_create(const repository::GitRepository &repo, const std::string &ref_name, const std::string &sha)
        {
            std::string path = (fs::path(repo.gitdir) / "refs" / ref_name).string();
            utils::write_file(path, sha + "\n");
        }

        void tag_create(const repository::GitRepository &repo, const std::string &name, const std::string &ref, bool create_tag = false)
        {
            std::optional<std::string> sha_optional = repository::object_find(repo, ref);
            if (!sha_optional)
                throw errors::GitException("Reference not found " + ref, errors::ExitCode::OBJECT_NOT_FOUND);

            std::string sha = *sha_optional;
            if (create_tag)
            {
                std::unique_ptr<objects::GitTag> tag = std::make_unique<objects::GitTag>("");
                std::vector<std::pair<std::string, std::string>> kvlm;
                kvlm.emplace_back("object", sha);
                kvlm.emplace_back("type", "commit");
                kvlm.emplace_back("tag", name);
                kvlm.emplace_back("tagger", "Tagger <tagger@tagme.com>");
                kvlm.emplace_back("None", "A generated tag which won't let you customize the message");

                tag->kvlm = kvlm;

                std::string tag_sha = repository::object_write(tag.get(), &repo);
                ref_create(repo, "tags/" + name, tag_sha);
            }
            else
            {
                ref_create(repo, "tags/" + name, sha);
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

        std::optional<std::string> sha = repository::object_find(*repo, args.get<std::string>("commit"));
        if (!sha)
            throw errors::GitException("Object not found: " + args.get<std::string>("commit"), errors::ExitCode::OBJECT_NOT_FOUND);

        std::cout << "digraph wyaglog{\n"
                  << "  node[shape=rect]\n";

        std::unordered_set<std::string> seen;

        log_graphviz((*repo), *sha, seen);
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

        std::optional<std::string> resolved = repository::object_find(*repo, tree, "tree");

        if (!resolved)
            throw errors::GitException("Not a tree object", errors::ExitCode::OBJECT_NOT_FOUND);

        ls_tree(*repo, *resolved, recursive);
        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_checkout(const ParseResult &args)
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);

        std::string commit = args.get<std::string>("commit");
        std::string path = args.get<std::string>("path");

        std::optional<std::string> obj = repository::object_find(*repo, commit, "", true);
        if (!obj)
            throw errors::GitException("Object not found " + commit, errors::ExitCode::OBJECT_NOT_FOUND);

        std::unique_ptr<objects::GitObject> object = repository::object_read(*repo, *obj);

        if (object->format == "commit")
        {
            objects::GitCommit *comm = dynamic_cast<objects::GitCommit *>(object.get());
            for (const auto &[key, val] : comm->kvlm)
            {
                if (key == "tree")
                {
                    object = repository::object_read(*repo, val);
                    break;
                }
            }
        }

        if (fs::exists(path))
        {
            if (!fs::is_directory(path))
                throw errors::GitException("Not a directory", errors::ExitCode::IO_ERROR);
            if (!fs::is_empty(path))
                throw errors::GitException("Not empty '" + path + "'", errors::ExitCode::IO_ERROR);
        }
        else
        {
            fs::create_directories(path);
        }

        objects::GitTree *tree = dynamic_cast<objects::GitTree *>(object.get());
        if (!tree)
            throw errors::GitException("Not a tree object", errors::ExitCode::MALFORMED_OBJECT);
        tree_checkout(*repo, *tree, fs::absolute(path).string());

        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_show_ref(const ParseResult &args)
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);

        auto refs = repository::ref_list(*repo);

        show_ref(*repo, refs);
        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_tag(const ParseResult &args)
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);

        std::string name = args.get<std::string>("name");
        std::string ref = args.get<std::string>("object");
        bool create_tag = args.get<bool>("a");

        if (name.empty())
        {
            const auto refs = repository::ref_list(*repo);
            show_ref(*repo, refs, false);
            return errors::ExitCode::SUCCESS;
        }
        tag_create(*repo, name, ref, create_tag);
        return errors::ExitCode::SUCCESS;
    }
} // namespace twig::commands
