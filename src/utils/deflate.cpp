#include "../include/utils/utils.hpp"
#include <zlib.h>
#include <stdexcept>

namespace twig::utils
{
    namespace
    {
        std::string do_compress(const std::string &data)
        {
            uLongf bound = compressBound(data.size());
            std::string out(bound, '\0');

            int rc = compress2(
                reinterpret_cast<Bytef *>(out.data()),
                &bound,
                reinterpret_cast<const Bytef *>(data.data()),
                data.size(),
                Z_BEST_COMPRESSION);

            if (rc != Z_OK)
                throw std::runtime_error("zlib compress failed: " + std::to_string(rc));

            out.resize(bound);
            return out;
        }

        std::string do_decompress(const std::string &data)
        {
            std::string out(data.size() * 4, '\0');

            while (true)
            {
                uLongf out_len = out.size();

                int rc = uncompress(
                    reinterpret_cast<Bytef *>(out.data()),
                    &out_len,
                    reinterpret_cast<const Bytef *>(data.data()),
                    data.size());

                if (rc == Z_OK)
                {
                    out.resize(out_len);
                    return out;
                }
                else if (rc == Z_BUF_ERROR)
                {
                    out.resize(out.size() * 2);
                }
                else
                {
                    throw std::runtime_error("zlib decompress failed: " + std::to_string(rc));
                }
            }
        }
    } // namespace

    std::string compress(const std::string &data)
    {
        return do_compress(data);
    }

    std::string decompress(const std::string &data)
    {
        return do_decompress(data);
    }

} // namespace twig::utils