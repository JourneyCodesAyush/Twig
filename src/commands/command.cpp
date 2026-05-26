#include "../include/commands/command.hpp"
#include "../include/repository/objects.hpp"

namespace twig::commands
{
    ExitCode cmd_init(const ParseResult &args)
    {
        auto dir = args.get<std::string>("directory");
        std::cout << "[init] directory: " << dir << "\n";

        repository::GitRepository repo = repository::repo_create(dir);

        return ExitCode::SUCCESS;
    }
} // namespace twig::commands
