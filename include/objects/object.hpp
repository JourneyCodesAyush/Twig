#pragma once

#include <string>
#include <vector>

namespace twig::objects
{
    std::vector<std::pair<std::string, std::string>> kvlm_parse(const std::string &raw);
    std::string kvlm_serialize(const std::vector<std::pair<std::string, std::string>> &kvlm);

    class GitObject
    {
    public:
        std::string format;

        GitObject(const std::string &fmt) : format(fmt) {}
        virtual std::string serialize() const = 0;
        virtual void deserialize(const std::string &data) = 0;

        virtual ~GitObject() = default;
    };

    class GitBlob : public GitObject
    {
    public:
        std::string blobdata;

        GitBlob(const std::string &data) : GitObject("blob"), blobdata(data) {}

        std::string serialize() const override { return this->blobdata; }

        void deserialize(const std::string &data) override { this->blobdata = data; }
    };

    class GitCommit : public GitObject
    {
    public:
        std::vector<std::pair<std::string, std::string>> kvlm;

        GitCommit(const std::string &data) : GitObject("commit") { this->kvlm = kvlm_parse(data); }

        std::string serialize() const override { return kvlm_serialize(this->kvlm); }

        void deserialize(const std::string &data) { this->kvlm = kvlm_parse(data); }
    };
} // namespace twig::objects
