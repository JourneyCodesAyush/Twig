#include <iostream>

#include "../include/argparse/argparse.hpp"
#include "../include/commands/command.hpp"
#include "../include/errors/error.hpp"

int main(int argc, char **argv)
{
    ArgumentParser parser("twig", "A git implementation in C++");

    // init [directory]
    // tests: optional positional with default value
    auto &init = parser.add_subcommand("init", "Initialize a new repository");
    init.add_argument(
        Argument("directory")
            .default_value(std::string(".")));

    auto &hash_object = parser.add_subcommand("hash-object", "Compute object ID and optionally create a blob from a file");
    hash_object.add_argument(Argument("w")
                                 .as_flag()
                                 .short_flag_char('w')
                                 .describe("Actually write the object into the object database"));

    hash_object.add_argument(Argument("t")
                                 .as_option()
                                 .short_flag_char('t')
                                 .default_value(std::string("blob"))
                                 .describe("Specify the type of object to create"));

    hash_object.add_argument(Argument("path")
                                 .make_required()
                                 .describe("Path to file to hash"));

    auto &cat_file = parser.add_subcommand("cat-file", "Display object contents");

    cat_file.add_argument(
        Argument("type")
            .make_required());

    cat_file.add_argument(
        Argument("object")
            .make_required());

    try
    {
        auto [subcmd, result] = parser.parse(argc, argv);

        if (subcmd == "init")
        {
            twig::errors::ExitCode code = twig::commands::cmd_init(result);
            return static_cast<int>(code);
        }
        if (subcmd == "hash-object")
        {
            twig::errors::ExitCode code = twig::commands::cmd_hash_object(result);
            return static_cast<int>(code);
        }
        if (subcmd == "cat-file")
        {
            twig::errors::ExitCode code = twig::commands::cmd_cat_file(result);
            return static_cast<int>(code);
        }
    }
    catch (const std::exception &e)
    {
        // Display error
        std::cerr << e.what() << '\n';
        // Exit gracefully
        return static_cast<int>(twig::errors::ExitCode::FAILURE);
    }
    return 0;
}