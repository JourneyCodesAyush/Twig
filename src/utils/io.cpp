#include "../include/utils/utils.hpp"

#include <sstream>
#include <fstream>

namespace twig::utils
{
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
