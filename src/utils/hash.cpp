#include "../include/utils/utils.hpp"

#include <openssl/evp.h>
#include <iomanip>
#include <sstream>

namespace twig::utils
{
    namespace
    {
        // Compute SHA-1 via OpenSSL's EVP digest API.
        // EVP is preferred over the deprecated SHA1() direct call.
        // Returns a 40-character lowercase hex string.
        std::string getSHA1_EVP(const std::string &data)
        {
            EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
            const EVP_MD *md = EVP_sha1();

            EVP_DigestInit_ex(mdctx, md, NULL);
            EVP_DigestUpdate(mdctx, data.c_str(), data.size());

            unsigned char md_value[EVP_MAX_MD_SIZE];
            unsigned int md_len;
            EVP_DigestFinal_ex(mdctx, md_value, &md_len);
            EVP_MD_CTX_free(mdctx);

            std::stringstream ss;
            for (unsigned int i = 0; i < md_len; i++)
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)md_value[i];
            return ss.str();
        }
    }

    std::string sha1(const std::string &content)
    {
        return getSHA1_EVP(content);
    }

} // namespace twig::utils
