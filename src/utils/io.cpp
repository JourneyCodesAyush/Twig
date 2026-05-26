#include "../include/utils/utils.hpp"

#include <sstream>
#include <fstream>

namespace twig::utils
{
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

} // namespace twig::utils
