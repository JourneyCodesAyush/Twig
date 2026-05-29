#include "../include/utils/utils.hpp"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace twig::utils
{
    std::string strip(const std::string &s)
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
            return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    std::string bytes_to_hex(const std::string &bytes)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        for (const char &ch : bytes)
        {
            unsigned char byte = static_cast<unsigned char>(ch);
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    std::string hex_to_bytes(const std::string &hex)
    {
        std::string bytes;
        for (size_t i = 0; i < hex.length(); i += 2)
        {
            unsigned char byte = static_cast<unsigned char>(std::stoul(hex.substr(i, 2), nullptr, 16));
            bytes += static_cast<char>(byte);
        }
        return bytes;
    }

    bool is_hex(const std::string &s)
    {
        return s.length() >= 4 && s.length() <= 40 && std::all_of(s.begin(), s.end(), ::isxdigit);
    }

    void write_file(const std::string &path, const std::string &message)
    {
        std::ofstream file(path);
        if (file.is_open())
        {
            file << message;
            file.close();
        }
        else
        {
            throw std::runtime_error("Failed to create " + path);
        }
    }

    std::string read_file(const std::string &filename)
    {
        std::stringstream source;
        std::ifstream infile(filename);

        if (!infile.is_open())
        {
            throw std::runtime_error("Could not open file: " + filename);
        }
        source << infile.rdbuf();

        infile.close();

        return source.str();
    }

    std::string read_file_binary(const std::string &filename)
    {
        std::stringstream source;
        std::ifstream infile(filename, std::ios::binary);

        if (!infile.is_open())
        {
            throw std::runtime_error("Could not open file: " + filename);
        }
        source << infile.rdbuf();

        infile.close();

        return source.str();
    }

    void write_file_binary(const std::string &path, const std::string &message)
    {
        std::ofstream file(path, std::ios::binary);
        if (file.is_open())
        {
            file << message;
            file.close();
        }
        else
        {
            throw std::runtime_error("Failed to create " + path);
        }
    }
} // namespace twig::utils
