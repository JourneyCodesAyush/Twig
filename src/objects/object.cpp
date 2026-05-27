#include <sstream>

#include "../include/objects/object.hpp"
#include "../include/errors/error.hpp"

namespace twig::objects
{
    std::vector<std::pair<std::string, std::string>> kvlm_parse(const std::string &raw)
    {
        std::vector<std::pair<std::string, std::string>> result;

        size_t pos = 0;

        while (1)
        {
            size_t space = raw.find(' ', pos);
            size_t newline = raw.find('\n', pos);

            if (space == std::string::npos || newline < space)
            {
                // Blank line - rest is message
                if (raw[pos] != '\n')
                {
                    throw errors::GitException("Malformed object", errors::ExitCode::MALFORMED_OBJECT);
                }
                result.push_back({"None", raw.substr(pos + 1)});
                break;
            }

            std::string key = raw.substr(pos, space - pos);

            // Find end of value, accounting for continuation lines
            size_t end = pos;
            while (1)
            {
                end = raw.find('\n', end + 1);
                if (end == std::string::npos)
                    break;
                if (raw[end + 1] != ' ')
                    break;
            }
            std::string value = raw.substr(space + 1, end - space - 1);
            size_t i = 0;
            while ((i = value.find("\n ", i)) != std::string::npos)
                value.replace(i, 2, "\n");

            result.push_back({key, value});
            pos = end + 1;
        }

        return result;
    }

    std::string kvlm_serialize(const std::vector<std::pair<std::string, std::string>> &kvlm)
    {
        std::stringstream ss;

        for (const auto &[key, value] : kvlm)
        {
            if (key == "None")
                continue;

            std::string v = value;
            size_t i = 0;
            while ((i = v.find("\n", i)) != std::string::npos)
            {
                v.replace(i, 1, "\n ");
                i += 2;
            }
            ss << key << ' ' << v << '\n';
        }

        for (auto &[key, value] : kvlm)
        {
            if (key != "None")
                continue;
            ss << '\n'
               << value;
        }
        return ss.str();
    }

} // namespace twig::objects
