#include <iostream>

#include "../include/argparse/argparse.hpp"
#include "../include/commands/command.hpp"

int main(int argc, char **argv)
{
    ArgumentParser parser("twig", "A git implementation in C++");

    // init [directory]
    // tests: optional positional with default value
    auto &init = parser.add_subcommand("init", "Initialize a new repository");
    init.add_argument(
        Argument("directory")
            .default_value(std::string(".")));

    try
    {
        auto [subcmd, result] = parser.parse(argc, argv);

        if (subcmd == "init")
        {
            ExitCode code = twig::commands::cmd_init(result);
            return static_cast<int>(code);
        }
    }
    catch (const std::exception &e)
    {
        // Display error
        std::cerr << e.what() << '\n';
        // Exit gracefully
        return static_cast<int>(ExitCode::FAILURE);
    }
    return 0;
}