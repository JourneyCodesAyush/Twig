/**
 * @file object.hpp
 * @brief Git object model and serialization helpers.
 *
 * Git stores all repository data as objects identified by the SHA-1 hash
 * of their serialized contents. Twig models these object types through
 * a small class hierarchy rooted at GitObject.
 *
 * Supported object types:
 * - GitBlob   : raw file contents.
 * - GitTree   : directory structure containing tree entries.
 * - GitCommit : commit metadata and message.
 * - GitTag    : annotated tag metadata.
 *
 * This module also provides parsers and serializers for Git's internal
 * object formats, including key-value list with message (KVLM) records
 * used by commits and tags.
 */

#pragma once

#include <string>
#include <vector>

namespace twig::objects
{
    class GitTreeLeaf;

    /**
     * @brief Parse a KVLM-encoded commit or tag object.
     */
    std::vector<std::pair<std::string, std::string>> kvlm_parse(const std::string &raw);

    /**
     * @brief Serialize KVLM data into Git's commit/tag format.
     */
    std::string kvlm_serialize(const std::vector<std::pair<std::string, std::string>> &kvlm);

    /**
     * @brief Parse a single tree entry.
     */
    GitTreeLeaf tree_parse_one(const std::string &raw, int &update_pos, int start = 0);

    /**
     * @brief Parse a tree object into individual entries.
     */
    std::vector<GitTreeLeaf> tree_parse(const std::string &raw);

    /**
     * @brief Serialize tree entries into Git's tree format.
     */
    std::string tree_serialize(std::vector<GitTreeLeaf> &leaves);

    /**
     * @brief Abstract base class for all Git object types.
     *
     * Derived classes are responsible for converting between their in-memory
     * representation and the binary format stored in the object database.
     */
    class GitObject
    {
    public:
        /**
         * @brief Git object type name ("blob", "tree", "commit", or "tag").
         */
        std::string format;

        GitObject(const std::string &fmt) : format(fmt) {}

        /**
         * @brief Serialize the object into Git's storage format.
         */
        virtual std::string serialize() const = 0;

        /**
         * @brief Populate the object from serialized data.
         */
        virtual void deserialize(const std::string &data) = 0;

        virtual ~GitObject() = default;
    };

    /**
     * @brief Git blob object containing raw file contents.
     */
    class GitBlob : public GitObject
    {
    public:
        std::string blobdata;

        GitBlob(const std::string &data) : GitObject("blob"), blobdata(data) {}

        std::string serialize() const override { return this->blobdata; }

        void deserialize(const std::string &data) override { this->blobdata = data; }
    };

    /**
     * @brief Git commit object.
     *
     * Commits are represented as a sequence of key-value pairs followed by
     * a message body.
     *
     * @note The commit message body is stored using the key "None".
     */
    class GitCommit : public GitObject
    {
    public:
        std::vector<std::pair<std::string, std::string>> kvlm;

        GitCommit() : GitObject("commit") {}
        GitCommit(const std::string &data) : GitObject("commit") { this->kvlm = kvlm_parse(data); }

        std::string serialize() const override { return kvlm_serialize(this->kvlm); }

        void deserialize(const std::string &data) { this->kvlm = kvlm_parse(data); }
    };

    /**
     * @brief Git annotated tag object.
     *
     * @note GitTag reuses the GitCommit KVLM representation because Git
     *       stores annotated tags using the same key-value record format.
     */
    class GitTag : public GitCommit
    {
    public:
        GitTag(const std::string &data) : GitCommit(data) { this->format = "tag"; }
    };

    /**
     * @brief Entry contained within a tree object.
     *
     * Each entry references either a blob or another tree through its SHA-1.
     */
    class GitTreeLeaf
    {
    public:
        std::string mode; ///< Git file mode (e.g. 100644, 100755, 040000).
        std::string path; ///< Entry name relative to the containing tree.
        std::string sha;  ///< Object ID of the referenced blob or tree.
        GitTreeLeaf(const std::string &mode_, const std::string &path_, const std::string &sha_) : mode(mode_), path(path_), sha(sha_) {}
    };

    /**
     * @brief Git tree object representing a directory.
     *
     * A tree contains entries that reference blobs and other trees,
     * forming the repository hierarchy.
     */
    class GitTree : public GitObject
    {
    public:
        std::vector<GitTreeLeaf> leaves;

        GitTree() : GitObject("tree") {}
        GitTree(const std ::string &data) : GitObject("tree") { this->leaves = tree_parse(data); }

        std::string serialize() const override
        {
            std::vector<GitTreeLeaf> copy = leaves;
            return tree_serialize(copy);
        }

        void deserialize(const std::string &data) { this->leaves = tree_parse(data); }
    };

} // namespace twig::objects
