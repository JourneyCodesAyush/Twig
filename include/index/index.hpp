#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../repository/repo.hpp"

namespace twig::index
{
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

    struct GitIndex
    {
        std::uint32_t version;
        std::vector<GitIndexEntry> entries;

        GitIndex(const std::uint32_t version = 2, const std::vector<GitIndexEntry> entries = {}) : version(version), entries(std::move(entries)) {}
    };

    GitIndex index_read(const repository::GitRepository &repo);
} // namespace twig::index