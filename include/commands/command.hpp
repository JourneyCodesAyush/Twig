#pragma once

#include "../errors/error.hpp"
#include "../argparse/argparse.hpp"

namespace twig::commands
{
    errors::ExitCode cmd_init(const ParseResult &args);
    errors::ExitCode cmd_hash_object(const ParseResult &args);
    errors::ExitCode cmd_cat_file(const ParseResult &args);
    errors::ExitCode cmd_log(const ParseResult &args);
    errors::ExitCode cmd_ls_tree(const ParseResult &args);
    errors::ExitCode cmd_checkout(const ParseResult &args);
    errors::ExitCode cmd_show_ref(const ParseResult &args);
    errors::ExitCode cmd_tag(const ParseResult &args);
    errors::ExitCode cmd_rev_parse(const ParseResult &args);
    errors::ExitCode cmd_ls_files(const ParseResult &args);
    errors::ExitCode cmd_check_ignore(const ParseResult &args);
    errors::ExitCode cmd_status();
    errors::ExitCode cmd_rm(const ParseResult &args);
    errors::ExitCode cmd_add(const ParseResult &args);
} // namespace twig::commands
