#pragma once

#include <string>
#include <vector>

namespace twig::objects
{
    class GitTreeLeaf;

    std::vector<std::pair<std::string, std::string>> kvlm_parse(const std::string &raw);
    std::string kvlm_serialize(const std::vector<std::pair<std::string, std::string>> &kvlm);

    GitTreeLeaf tree_parse_one(const std::string &raw, int &update_pos, int start = 0);
    std::vector<GitTreeLeaf> tree_parse(const std::string &raw);

    std::string tree_serialize(std::vector<GitTreeLeaf> &leaves);

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

    class GitTag : public GitCommit
    {
    public:
        GitTag(const std::string &data) : GitCommit(data) { this->format = "tag"; }
    };

    class GitTreeLeaf
    {
    public:
        std::string mode;
        std::string path;
        std::string sha;
        GitTreeLeaf(const std::string &mode_, const std::string &path_, const std::string &sha_) : mode(mode_), path(path_), sha(sha_) {}
    };

    class GitTree : public GitObject
    {
    public:
        std::vector<GitTreeLeaf> leaves;

        GitTree(const std ::string &data) : GitObject("tree") { this->leaves = tree_parse(data); }

        std::string serialize() const override
        {
            std::vector<GitTreeLeaf> copy = leaves;
            return tree_serialize(copy);
        }

        void deserialize(const std::string &data) { this->leaves = tree_parse(data); }
    };

} // namespace twig::objects
