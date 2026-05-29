#include <filesystem>

#include "../include/index/index.hpp"
#include "../include/repository/objects.hpp"
#include "../include/utils/utils.hpp"
#include "../include/errors/error.hpp"

namespace twig::index
{
    namespace
    {
        uint32_t read_u32(const std::string &data, size_t offset)
        {
            return (static_cast<uint32_t>((unsigned char)data[offset]) << 24) |
                   (static_cast<uint32_t>((unsigned char)data[offset + 1]) << 16) |
                   (static_cast<uint32_t>((unsigned char)data[offset + 2]) << 8) |
                   (static_cast<uint32_t>((unsigned char)data[offset + 3]));
        }

        uint16_t read_u16(const std::string &data, size_t offset)
        {
            return (static_cast<uint16_t>((unsigned char)data[offset]) << 8) |
                   (static_cast<uint16_t>((unsigned char)data[offset + 1]));
        }

    } // namespace

    namespace fs = std::filesystem;

    GitIndex index_read(const repository::GitRepository &repo)
    {
        std::optional<std::string> index_file = repository::repo_file(repo, false, {"index"});

        if (!index_file || !fs::exists(*index_file))
            return GitIndex();

        std::string raw = utils::read_file_binary(*index_file);

        if (raw.substr(0, 4) != "DIRC")
            throw errors::GitException("Index error", errors::ExitCode::INDEX_ERROR);

        std::uint32_t version = read_u32(raw, 4);
        if (version != 2)
            throw errors::GitException("Only version 2 is supported", errors::ExitCode::INDEX_ERROR);

        std::uint32_t count = read_u32(raw, 8);

        std::vector<GitIndexEntry> entries;
        std::string content = raw.substr(12);

        int idx = 0;
        for (std::uint32_t i = 0; i < count; i++)
        {
            std::uint32_t ctime_second = read_u32(content, idx);
            idx += 4;

            std::uint32_t ctime_nanosecond = read_u32(content, idx);
            idx += 4;

            std::uint32_t mtime_second = read_u32(content, idx);
            idx += 4;
            std::uint32_t mtime_nanosecond = read_u32(content, idx);
            idx += 4;

            std::uint32_t dev = read_u32(content, idx);
            idx += 4;

            std::uint32_t ino = read_u32(content, idx);
            idx += 4;

            std::uint16_t unused = read_u16(content, idx);

            if (unused != 0)
                throw errors::GitException("Index format error", errors::ExitCode::INDEX_ERROR);
            idx += 2;

            std::uint16_t mode = read_u16(content, idx);
            idx += 2;
            std::uint16_t mode_type = mode >> 12;
            std::uint16_t mode_perms = mode & 0x1FF;

            if (mode_type != 0b1000 && mode_type != 0b1010 && mode_type != 0b1110)
                throw errors::GitException("Invalid mode type", errors::ExitCode::INDEX_ERROR);

            std::uint32_t uid = read_u32(content, idx);
            idx += 4;

            std::uint32_t gid = read_u32(content, idx);
            idx += 4;

            std::uint32_t fsize = read_u32(content, idx);
            idx += 4;

            std::string sha = utils::bytes_to_hex(content.substr(idx, 20));
            idx += 20;

            std::uint16_t flags = read_u16(content, idx);
            idx += 2;

            bool flag_assume_valid = (flags & 0b1000000000000000) != 0;
            bool flag_extended = (flags & 0b0100000000000000) != 0;

            if (flag_extended)
                throw errors::GitException("Flag error", errors::ExitCode::INDEX_ERROR);

            std::uint8_t flag_stage = flags & 0b0011000000000000;

            std::uint32_t name_length = flags & 0b0000111111111111;

            std::string name;
            if (name_length < 0xFFF)
            {
                name = content.substr(idx, name_length);
                idx += name_length + 1; // +1 for null terminator
            }
            else
            {
                size_t null_idx = content.find('\0', idx + 0xFFF);
                name = content.substr(idx, null_idx - idx);
                idx = null_idx + 1;
            }

            // Pad to next multiple of 8
            idx = 8 * ((idx + 7) / 8);

            entries.emplace_back(ctime_second, ctime_nanosecond,
                                 mtime_second, mtime_nanosecond,
                                 dev, ino, uid, gid, fsize,
                                 mode_type, mode_perms,
                                 flag_assume_valid, flag_stage,
                                 sha, name);
        }

        return GitIndex(2, entries);
    }
} // namespace twig::index