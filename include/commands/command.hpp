#pragma once

#include "../errors/error.hpp"
#include "../argparse/argparse.hpp"

namespace twig::commands
{
    ExitCode cmd_init(const ParseResult &args);
    ExitCode cmd_hash_object(const ParseResult &args);
} // namespace twig::commands
