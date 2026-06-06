/**
 * @file index.hpp
 * @brief Git staging area (index) representation and persistence.
 *
 * The Git index is the staging area between the working directory and
 * the commit history (HEAD). It stores a snapshot of what will be written
 * to the next commit, mapping file paths to blob objects along with
 * filesystem metadata used to efficiently detect changes.
 *
 * The index avoids expensive full file hashing by comparing stored
 * metadata (mtime, size, inode, etc.) against the working directory.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../repository/repo.hpp"

namespace twig::index
{
    /**
     * @brief A single entry in the Git index.
     *
     * Each entry represents a tracked file and stores both:
     * - the object ID (SHA-1) of its content
     * - filesystem metadata for change detection
     *
     * This structure closely mirrors Git's internal index format.
     */
    struct GitIndexEntry
    {
        // Last time a file's metadata changed (seconds since epoch + nanoseconds)
        std::uint32_t ctime_second;
        std::uint32_t ctime_nanosecond;
        // Last time a file's data changed (seconds since epoch + nanoseconds)
        std::uint32_t mtime_second;
        std::uint32_t mtime_nanosecond;

        std::uint32_t dev;
        std::uint32_t ino;
        std::uint32_t uid;
        std::uint32_t gid;
        std::uint32_t fsize;
        std::uint16_t mode_type;
        std::uint16_t mode_perms;
        bool flag_assume_valid;
        std::uint8_t flag_stage;
        std::string sha;
        std::string name;

        GitIndexEntry()
            : ctime_second(0), ctime_nanosecond(0),
              mtime_second(0), mtime_nanosecond(0),
              dev(0), ino(0), uid(0), gid(0), fsize(0),
              mode_type(0), mode_perms(0),
              flag_assume_valid(false), flag_stage(0) {}

        GitIndexEntry(std::uint32_t ctime_second,
                      std::uint32_t ctime_nanosecond,
                      std::uint32_t mtime_second,
                      std::uint32_t mtime_nanosecond,
                      std::uint32_t dev,
                      std::uint32_t ino,
                      std::uint32_t uid,
                      std::uint32_t gid,
                      std::uint32_t fsize,
                      std::uint16_t mode_type,
                      std::uint16_t mode_perms,
                      bool flag_assume_valid,
                      std::uint8_t flag_stage,
                      std::string sha,
                      std::string name)
            : ctime_second(ctime_second), ctime_nanosecond(ctime_nanosecond),
              mtime_second(mtime_second), mtime_nanosecond(mtime_nanosecond),
              dev(dev), ino(ino), uid(uid), gid(gid), fsize(fsize),
              mode_type(mode_type), mode_perms(mode_perms),
              flag_assume_valid(flag_assume_valid), flag_stage(flag_stage),
              sha(std::move(sha)), name(std::move(name)) {}
    };

    /**
     * @brief In-memory representation of the Git index (staging area).
     *
     * The index stores a list of tracked files and their associated metadata.
     * Version refers to the Git index format version (typically 2 in modern Git).
     */
    struct GitIndex
    {
        std::uint32_t version;
        std::vector<GitIndexEntry> entries;

        GitIndex(const std::uint32_t version = 2, const std::vector<GitIndexEntry> entries = {}) : version(version), entries(std::move(entries)) {}
    };

    /**
     * @brief Read and parse the .git/index file.
     *
     * Loads the staging area from disk into memory.
     * May perform validation depending on implementation.
     */
    GitIndex index_read(const repository::GitRepository &repo);

    /**
     * @brief Write the in-memory index back to .git/index.
     *
     * Serializes the staging area to disk. Typically overwrites the existing
     * index file atomically.
     */
    void index_write(const repository::GitRepository &repo, const GitIndex &index);
} // namespace twig::index