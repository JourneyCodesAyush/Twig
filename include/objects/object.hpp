#pragma once

#include <string>

namespace twig::objects
{
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
} // namespace twig::objects
