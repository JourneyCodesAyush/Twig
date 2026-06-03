#include <algorithm>
#include <fstream>
#include <memory>
#include <unordered_set>
#include <filesystem>

#include <sys/stat.h>

#include "../include/commands/command.hpp"
#include "../include/index/index.hpp"
#include "../include/repository/objects.hpp"
#include "../include/ignore/ignore.hpp"
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

        std::optional<std::string> branch_get_active(const repository::GitRepository &repo)
        {
            std::string head = utils::read_file((fs::path(repo.gitdir) / "HEAD").string());
            if (head.starts_with("ref: refs/heads/"))
                return utils::strip(head.substr(16));
            else
                return std::nullopt;
        }

        void cmd_status_branch(const repository::GitRepository &repo)
        {
            std::optional<std::string> branch = branch_get_active(repo);
            if (branch)
                std::cout << "On branch: " << *branch << "\n";
            else
            {
                std::optional<std::string> sha = repository::object_find(repo, "HEAD");
                std::cout << "HEAD detached at: " << *sha << "\n";
            }
        }

        std::unordered_map<std::string, std::string> tree_to_map(
            const repository::GitRepository &repo,
            const std::string &ref,
            const std::string &prefix = "")
        {
            std::unordered_map<std::string, std::string> map;

            std::optional<std::string> tree_sha = repository::object_find(repo, ref, "tree");

            if (!tree_sha)
                throw errors::GitException("Could not resolve tree: " + ref, errors::ExitCode::OBJECT_NOT_FOUND);

            std::unique_ptr<twig::objects::GitObject> object = repository::object_read(repo, *tree_sha);

            objects::GitTree *tree = dynamic_cast<objects::GitTree *>(object.get());

            if (!tree)
                throw errors::GitException("Not a tree object", errors::ExitCode::MALFORMED_OBJECT);

            for (const auto &leaf : tree->leaves)
            {
                std::string full_path = (fs::path(prefix) / leaf.path).string();

                bool is_subtree = leaf.mode.starts_with("04");

                if (is_subtree)
                {
                    auto sub = tree_to_map(repo, leaf.sha, full_path);
                    map.merge(sub);
                }
                else
                {
                    map[full_path] = leaf.sha;
                }
            }

            return map;
        }

        void cmd_status_head_index(const repository::GitRepository &repo, const index::GitIndex &index)
        {
            std::cout << "Changes to be committed:\n";

            std::unordered_map<std::string, std::string> head = tree_to_map(repo, "HEAD");

            for (const auto &entry : index.entries)
            {
                if (head.count(entry.name) != 0)
                {
                    if (head[entry.name] != entry.sha)
                        std::cout << "  modified: " << entry.name << "\n";
                    head.erase(entry.name);
                }
                else
                {
                    std::cout << "  added: " << entry.name << "\n";
                }
            }

            for (const auto &[path, sha] : head)
                std::cout << "  deleted: " << path << "\n";
        }

        void cmd_status_index_worktree(const repository::GitRepository &repo, const index::GitIndex &index)
        {
            std::cout << "Changes not staged for commit:\n";

            std::unordered_set<std::string> all_files;

            ignore::GitIgnore ignore = ignore::gitignore_read(repo);

            std::string gitdir_prefix = repo.gitdir + fs::path::preferred_separator;

            for (const auto &entry : fs::recursive_directory_iterator(repo.worktree))
            {
                if (!fs::is_regular_file(entry))
                    continue;

                std::string path = entry.path().string();
                if (path == repo.gitdir || path.starts_with(gitdir_prefix))
                    continue;

                std::string rel = fs::relative(entry.path(), repo.worktree).string();
                all_files.insert(rel);
            }

            for (const auto &entry : index.entries)
            {
                std::string full_path = (fs::path(repo.worktree) / entry.name).string();

                if (!fs::exists(full_path))
                {
                    std::cout << "  deleted: " << entry.name << "\n";
                }
                else
                {
                    std::string content = utils::read_file_binary(full_path);

                    std::unique_ptr<objects::GitObject> obj = std::make_unique<objects::GitBlob>(content);

                    std::string sha = repository::object_write(obj.get(), nullptr);

                    if (entry.sha != sha)
                        std::cout << "  modified: " << entry.name << "\n";
                }

                all_files.erase(entry.name);
            }

            std::cout << "\n"
                      << "Untracked files:\n";
            for (const auto &file : all_files)
            {
                if (!ignore::check_ignore(ignore, file))
                    std::cout << "  " << file << "\n";
            }
        }

        void rm(const repository::GitRepository &repo, const std::vector<std::string> &paths, bool delete_entry = true, bool skip_missing = false)
        {
            index::GitIndex index = index::index_read(repo);

            std::string worktree = fs::canonical(repo.worktree).string() + fs::path::preferred_separator;

            std::unordered_set<std::string> abspaths;
            for (const auto &path : paths)
            {
                std::string abspath = (fs::absolute(path)).string();
                // std::cout << "abspath: " << abspath << "\n";
                // std::cout << "worktree: " << worktree << "\n";
                if (abspath.starts_with(worktree))
                    abspaths.insert(abspath);
                else
                    throw errors::GitException("Cannot remove paths outside of worktree", errors::ExitCode::FAILURE);
            }

            std::vector<index::GitIndexEntry> kept_entries;
            std::vector<std::string> remove_entries;

            for (const auto &entry : index.entries)
            {
                std::string full_path = (fs::canonical(fs::path(repo.worktree) / entry.name)).string();

                if (abspaths.contains(full_path))
                {
                    remove_entries.push_back(full_path);
                    abspaths.erase(full_path);
                }
                else
                    kept_entries.push_back(entry);
            }

            if (abspaths.size() > 0 && !skip_missing)
                throw errors::GitException("Cannot remove paths not in the index", errors::ExitCode::FAILURE);

            if (delete_entry)
            {
                for (const auto &path : remove_entries)
                    fs::remove(path);
            }

            index.entries = kept_entries;
            index::index_write(repo, index);
        }

        void add(const repository::GitRepository &repo, const std::vector<std::string> &paths, bool delete_entry = true, bool skip_missing = false)
        {
            rm(repo, paths, false, true);
            std::string worktree = fs::canonical(repo.worktree).string() + fs::path::preferred_separator;

            // (absolute, relative_to_worktree)
            std::vector<std::pair<std::string, std::string>> clean_paths;

            for (const auto &path : paths)
            {
                std::string abspath = fs::canonical(fs::absolute(path).parent_path()) / fs::absolute(path).filename();

                if (!abspath.starts_with(worktree) || !fs::is_regular_file(abspath))
                    throw errors::GitException("Not a file, or outside the worktree", errors::ExitCode::FAILURE);
                std::string relpath = fs::relative(abspath, repo.worktree);

                clean_paths.emplace_back(abspath, relpath);
            }

            index::GitIndex index = index::index_read(repo);

            for (const auto &[abspath, relpath] : clean_paths)
            {
                std::string content = utils::read_file_binary(abspath);

                std::unique_ptr<objects::GitObject> obj = std::make_unique<objects::GitBlob>(content);

                std::string sha = repository::object_write(obj.get(), &repo);

                struct stat file_stats;
                stat(abspath.c_str(), &file_stats);

                std::uint32_t ctime_second = file_stats.st_ctim.tv_sec;
                std::uint32_t ctime_nanosecond = file_stats.st_ctim.tv_nsec;
                std::uint32_t mtime_second = file_stats.st_mtim.tv_sec;
                std::uint32_t mtime_nanosecond = file_stats.st_mtim.tv_nsec;

                std::uint32_t dev = file_stats.st_dev;
                std::uint32_t ino = file_stats.st_ino;
                std::uint32_t uid = file_stats.st_uid;
                std::uint32_t gid = file_stats.st_gid;
                std::uint32_t fsize = file_stats.st_size;

                index::GitIndexEntry entry(ctime_second, ctime_nanosecond, mtime_second, mtime_nanosecond, dev, ino, uid, gid, fsize, 0b1000, 0644, false, 0, sha, relpath);

                index.entries.push_back(entry);
            }

            index::index_write(repo, index);
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

    errors::ExitCode cmd_rev_parse(const ParseResult &args)
    {
        std::string format;
        if (args.has("type"))
            format = args.get<std::string>("type");
        std::string name = args.get<std::string>("name");

        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);

        std::optional<std::string> output = repository::object_find(*repo, name, format, true);
        if (!output)
            throw errors::GitException("Object not found " + name, errors::ExitCode::OBJECT_NOT_FOUND);

        std::cout
            << *output
            << "\n";

        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_ls_files(const ParseResult &args)
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);

        index::GitIndex index = index::index_read(*repo);

        bool verbose = args.get<bool>("verbose");

        if (verbose)
            std::cout
                << "Index file format "
                << index.version
                << ", containing "
                << index.entries.size()
                << " entries."
                << "\n";

        std::unordered_map<uint16_t, std::string> file_type;
        file_type[0b1000] = "regular file";
        file_type[0b1010] = "symlink";
        file_type[0b1110] = "git link";

        for (const auto &entry : index.entries)
        {
            std::cout << entry.name << "\n";
            if (verbose)
            {
                const std::string &entry_type = file_type[entry.mode_type];
                std::cout
                    << "  "
                    << entry_type
                    << " with perms: "
                    << std::oct
                    << entry.mode_perms
                    << std::dec
                    << "\n";

                std::cout
                    << "  on blob: "
                    << entry.sha
                    << "\n";

                std::cout
                    << "  created: "
                    << entry.ctime_second
                    << "."
                    << entry.ctime_nanosecond
                    << ", modified: "
                    << entry.mtime_second
                    << "."
                    << entry.mtime_nanosecond
                    << "\n";

                std::cout
                    << "  device: "
                    << entry.dev
                    << ", inode: "
                    << entry.ino
                    << "\n";

                std::cout
                    << "  uid: "
                    << entry.uid
                    << ", gid: "
                    << entry.gid
                    << "\n";

                std::cout
                    << "  flags: stage="
                    << static_cast<int>(entry.flag_stage)
                    << " assume_valid="
                    << entry.flag_assume_valid
                    << "\n";
            }
        }

        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_check_ignore(const ParseResult &args)
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);

        ignore::GitIgnore rules = ignore::gitignore_read(*repo);

        std::vector<std::string> paths = args.get<std::vector<std::string>>("path");
        for (const auto &path : paths)
        {
            if (ignore::check_ignore(rules, fs::path(path).lexically_normal().string()))
                std::cout << path << "\n";
        }

        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_status()
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);

        index::GitIndex index = index::index_read(*repo);

        cmd_status_branch(*repo);
        cmd_status_head_index(*repo, index);
        cmd_status_index_worktree(*repo, index);
        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_rm(const ParseResult &args)
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);

        std::vector<std::string> paths = args.get<std::vector<std::string>>("path");
        rm(*repo, paths);

        return errors::ExitCode::SUCCESS;
    }

    errors::ExitCode cmd_add(const ParseResult &args)
    {
        std::optional<repository::GitRepository> repo = repository::repo_find();
        if (!repo)
            throw errors::GitException("Not a repository", errors::ExitCode::NOT_A_REPO);

        std::vector<std::string> paths = args.get<std::vector<std::string>>("path");
        add(*repo, paths);

        return errors::ExitCode::SUCCESS;
    }
} // namespace twig::commands
