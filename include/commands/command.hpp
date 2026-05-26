#pragma once

#include "../errors/error.hpp"
#include "../argparse/argparse.hpp"

namespace twig::commands
{
    ExitCode cmd_init(const ParseResult &args);
} // namespace twig::commands
