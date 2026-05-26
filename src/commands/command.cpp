#include <memory>

#include "../include/commands/command.hpp"
#include "../include/repository/objects.hpp"
#include "../include/utils/utils.hpp"
#include "../include/errors/error.hpp"

namespace twig::commands
{
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
} // namespace twig::commands
