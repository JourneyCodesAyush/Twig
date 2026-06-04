#include <algorithm>
#include <sstream>

#include "../include/objects/object.hpp"
#include "../include/errors/error.hpp"
#include "../include/utils/utils.hpp"

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

    GitTreeLeaf tree_parse_one(const std::string &raw, int &update_pos, int start)
    {
        // Find the space terminator of the mode
        size_t x = raw.find(' ', start);

        if (x == std::string::npos || ((x - start) != 5 && (x - start) != 6))
            throw errors::GitException("Malformed object found while parsing tree", errors::ExitCode::MALFORMED_OBJECT);

        // Read the mode
        std::string mode = raw.substr(start, x - start);
        if (mode.length() == 5)
            mode = "0" + mode;

        // Find the NULL terminator of   the path
        size_t y = raw.find('\x00', x);
        // Read the path
        std::string path = raw.substr(x + 1, y - x - 1);

        std::string sha_bytes = raw.substr(y + 1, 20);

        std::string sha = utils::bytes_to_hex(sha_bytes);

        update_pos = y + 21;
        return GitTreeLeaf(mode, path, sha);
    }

    std::vector<GitTreeLeaf> tree_parse(const std::string &raw)
    {
        std::vector<GitTreeLeaf> leaves;
        int length = raw.length();

        int pos = 0;
        while (pos < length)
        {
            leaves.push_back(tree_parse_one(raw, pos, pos));
        }
        return leaves;
    }

    std::string tree_serialize(std::vector<GitTreeLeaf> &leaves)
    {
        std::sort(leaves.begin(), leaves.end(), [&](const GitTreeLeaf &leaf1, const GitTreeLeaf &leaf2)
                  {
                    std::string path1 = leaf1.path;
                    std::string path2 = leaf2.path;
            if(leaf1.mode.starts_with("04")){
                path1 += "/";
            }
            if(leaf2.mode.starts_with("04")){
                path2 += "/";
            }
        return path1 < path2; });

        std::string serialized;
        for (const auto &leaf : leaves)
        {
            serialized += leaf.mode;
            serialized += std::string(1, ' ');
            serialized += leaf.path;
            serialized += std::string(1, '\0');
            serialized += utils::hex_to_bytes(leaf.sha);
        }

        return serialized;
    }
} // namespace twig::objects
