#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

struct ParseError : std::runtime_error
{
    explicit ParseError(const std::string &message) : std::runtime_error(message) {}
};

using ArgValue = std::variant<std::string, bool, std::vector<std::string>>;

enum class ArgKind
{
    POSITIONAL,
    FLAG,
    OPTION
};

struct Argument
{
    std::string name;
    char short_flag;
    std::string long_flag;
    ArgKind kind;
    bool required;
    bool variadic;
    std::optional<ArgValue> default_val;
    std::string description;

    explicit Argument(const std::string &name) : name(name),
                                                 short_flag('\0'),
                                                 long_flag(""),
                                                 kind(ArgKind::POSITIONAL),
                                                 required(false),
                                                 variadic(false),
                                                 description("")
    {
    }

    Argument &short_flag_char(char c)
    {
        this->short_flag = c;
        return *this;
    }

    Argument &long_flag_name(const std::string &long_flag)
    {
        this->long_flag = long_flag;
        return *this;
    }

    Argument &as_option()
    {
        this->kind = ArgKind::OPTION;
        return *this;
    }

    Argument &as_flag()
    {
        this->kind = ArgKind::FLAG;
        return *this;
    }

    Argument &make_required()
    {
        this->required = true;
        return *this;
    }

    Argument &make_variadic()
    {
        this->variadic = true;
        return *this;
    }

    Argument &default_value(const ArgValue &value)
    {
        this->default_val = value;
        return *this;
    }

    Argument &describe(const std::string &desc)
    {
        this->description = desc;
        return *this;
    }
};

struct ParseResult
{
    std::unordered_map<std::string, ArgValue> values;

    template <typename T>
    T get(const std::string &name) const
    {
        if (values.count(name) == 0)
        {
            throw ParseError(name + " is invalid");
        }
        try
        {

            return std::get<T>(values.at(name));
        }
        catch (const std::bad_variant_access &)
        {
            throw ParseError(name + " exists but was accessed with wrong type");
        }
    }

    bool has(const std::string &name) const
    {
        return values.count(name) > 0;
    }
};

class SubCommand
{
    std::vector<Argument> arguments;
    std::unordered_map<std::string, int> long_map; // "message" -> index
    std::unordered_map<char, int> short_map;       // 'm' -> index

    std::vector<int> positional_indices;

public:
    std::string name;
    std::string description;

    SubCommand(const std::string &name, const std::string &desc) : name(name), description(desc) {}

    void add_argument(const Argument &argument)
    {
        int index = arguments.size();
        arguments.push_back(argument);

        if (argument.short_flag != '\0')
            short_map[argument.short_flag] = index;

        if (!argument.long_flag.empty())
            long_map[argument.long_flag] = index;

        if (argument.kind == ArgKind::POSITIONAL)
            positional_indices.push_back(index);
    }

    ParseResult parse(const std::vector<std::string> &args)
    {
        for (const auto &token : args)
        {
            if (token == "-h" || token == "--help")
            {
                print_help();
                std::exit(0);
            }
        }

        ParseResult result;
        int positional_index = 0;

        for (int i = 0; i < (int)args.size(); i++)
        {
            std::string token = args[i];

            if (token.substr(0, 2) == "--")
            {
                token = token.substr(2);
                size_t pos = token.find('=');

                if (pos != std::string::npos)
                {
                    std::string flag_name = token.substr(0, pos);
                    std::string value = token.substr(pos + 1);

                    if (long_map.count(flag_name) == 0)
                        throw ParseError("Unknown option: --" + flag_name);

                    int index = long_map.at(flag_name);
                    Argument arg = arguments[index];

                    if (arg.kind != ArgKind::OPTION)
                        throw ParseError("Flags don't take = values");

                    result.values[arg.name] = value;
                }
                else
                {
                    if (long_map.count(token) == 0)
                        throw ParseError("Unknown option: --" + token);

                    int index = long_map.at(token);
                    Argument arg = arguments[index];

                    if (arg.kind == ArgKind::FLAG)
                        result.values[arg.name] = true;

                    if (arg.kind == ArgKind::OPTION)
                    {
                        i++;
                        if (i >= (int)args.size())
                            throw ParseError("Option needs a value");
                        result.values[arg.name] = args[i];
                    }
                }
            }

            else if (token.substr(0, 1) == "-" && token.size() == 2)
            {
                char ch = token[1];

                if (short_map.count(ch) == 0)
                    throw ParseError(std::string("Unknown option: -") + ch);

                int index = short_map.at(ch);    // ← from map
                Argument arg = arguments[index]; // ← use that index

                if (arg.kind == ArgKind::FLAG)
                    result.values[arg.name] = true;

                if (arg.kind == ArgKind::OPTION)
                {
                    i++;
                    if (i >= (int)args.size())
                        throw ParseError("Option needs a value");
                    result.values[arg.name] = args[i];
                }
            }

            else
            {
                if (positional_index >= (int)positional_indices.size())
                    throw ParseError("Too many arguments");

                Argument arg = arguments[positional_indices[positional_index]]; // ← positional_indices here

                if (arg.variadic)
                {
                    std::vector<std::string> collected;
                    while (i < (int)args.size())
                    {
                        if (args[i].substr(0, 1) == "-")
                            break;
                        collected.push_back(args[i++]);
                    }
                    result.values[arg.name] = collected;
                    positional_index++; // mark this slot as filled
                    i--;                // outer loop will i++ so step back once
                }
                else
                {
                    result.values[arg.name] = args[i];
                    positional_index++;
                }
            }
        }

        for (const auto &arg : arguments)
        {
            if (arg.required && result.values.count(arg.name) == 0)
            {
                throw ParseError("Missing argument required: " + arg.name);
            }
            if (!arg.required && result.values.count(arg.name) == 0)
            {
                if (arg.default_val.has_value())
                {
                    result.values[arg.name] = arg.default_val.value();
                }
            }
        }

        return result;
    }

    void print_help() const
    {
        // ── usage line ────────────────────────────────────────
        std::cout << "usage: wyag " << name;

        for (const auto &arg : arguments)
        {
            if (arg.kind == ArgKind::OPTION)
            {
                std::string flag = arg.short_flag != '\0'
                                       ? std::string("-") + arg.short_flag
                                       : "--" + arg.long_flag;

                if (arg.required)
                    std::cout << " " << flag << " <" << arg.name << ">";
                else
                    std::cout << " [" << flag << " <" << arg.name << ">]";
            }
            else if (arg.kind == ArgKind::FLAG)
            {
                std::cout << " [--" << arg.long_flag << "]";
            }
            else if (arg.kind == ArgKind::POSITIONAL)
            {
                if (arg.variadic)
                    std::cout << " <" << arg.name << ">...";
                else if (arg.required)
                    std::cout << " <" << arg.name << ">";
                else
                    std::cout << " [<" << arg.name << ">]";
            }
        }

        std::cout << "\n\n";

        // ── description ───────────────────────────────────────
        std::cout << description << "\n\n";

        // ── arguments table ───────────────────────────────────
        // first pass — find the longest flag string for alignment
        size_t col_width = 0;
        for (const auto &arg : arguments)
        {
            size_t len = 0;
            if (arg.kind == ArgKind::POSITIONAL)
            {
                len = arg.name.size() + 2; // <name>
            }
            else
            {
                if (arg.short_flag != '\0')
                    len += 4; // "-x, "
                if (!arg.long_flag.empty())
                    len += 2 + arg.long_flag.size(); // "--name"
                if (arg.kind == ArgKind::OPTION)
                    len += 3 + arg.name.size(); // " <name>"
            }
            col_width = std::max(col_width, len);
        }

        col_width += 3; // padding between columns

        std::cout << "arguments:\n";

        // second pass — print each row
        for (const auto &arg : arguments)
        {
            std::string left;

            if (arg.kind == ArgKind::POSITIONAL)
            {
                left = "<" + arg.name + ">";
            }
            else
            {
                if (arg.short_flag != '\0')
                    left += std::string("-") + arg.short_flag + ", ";
                else
                    left += "    "; // indent to align with "-x, "

                if (!arg.long_flag.empty())
                    left += "--" + arg.long_flag;

                if (arg.kind == ArgKind::OPTION)
                    left += " <" + arg.name + ">";
            }

            // pad left column to col_width
            if (left.size() < col_width)
                left += std::string(col_width - left.size(), ' ');

            // right column — description hints
            std::string right;

            if (!arg.description.empty())
                right += arg.description;

            if (arg.required)
                right += " (required)";
            else if (arg.default_val.has_value())
            {
                if (std::holds_alternative<std::string>(arg.default_val.value()))
                    right += " (default: " + std::get<std::string>(arg.default_val.value()) + ")";
            }

            std::cout << "  " << left << right << "\n";
        }

        std::cout << "\n";
    }
};

class ArgumentParser
{
    std::string program_name;
    std::string description;
    std::unordered_map<std::string, SubCommand> subcommands;

public:
    ArgumentParser(const std::string &program_name, const std::string &desc) : program_name(program_name), description(desc) {}

    SubCommand &add_subcommand(const std::string &name, const std::string &desc)
    {
        subcommands.emplace(name, SubCommand{name, desc});
        return subcommands.at(name);
    }

    std::pair<std::string, ParseResult> parse(int argc, char **argv)
    {
        std::vector<std::string> args;
        for (int i = 0; i < argc; i++)
        {
            args.emplace_back(std::string(argv[i]));
        }

        if (args.size() < 2)
        {
            print_help();
            std::exit(1);
        }

        if (subcommands.count(args[1]) == 0)
        {
            throw ParseError("Unknown command: " + args[1]);
        }

        auto start = args.begin() + 2;
        auto end = args.end();

        std::vector<std::string> sliced(start, end);
        ParseResult res = subcommands.at(args[1]).parse(sliced);

        return {args[1], res};
    }

    void print_help() const
    {
        // ── usage line ────────────────────────────────────────
        std::cout << "usage: " << program_name << " <command> [<args>]\n\n";

        // ── description ───────────────────────────────────────
        std::cout << description << "\n\n";

        // ── subcommand table ──────────────────────────────────
        // first pass — find longest subcommand name for alignment
        size_t col_width = 0;
        for (const auto &pair : subcommands)
            col_width = std::max(col_width, pair.first.size());

        col_width += 3; // padding between columns

        std::cout << "commands:\n";

        for (const auto &pair : subcommands)
        {
            std::string left = pair.first;
            if (left.size() < col_width)
                left += std::string(col_width - left.size(), ' ');

            std::cout << "  " << left << pair.second.description << "\n";
        }

        std::cout << "\n";
    }
};