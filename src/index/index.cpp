#include <filesystem>
#include <fstream>

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

        void write_u32(std::ofstream &f, uint32_t val)
        {
            f.put((val >> 24) & 0xFF);
            f.put((val >> 16) & 0xFF);
            f.put((val >> 8) & 0xFF);
            f.put(val & 0xFF);
        }

        void write_u16(std::ofstream &f, uint16_t val)
        {
            f.put((val >> 8) & 0xFF);
            f.put(val & 0xFF);
        }
    } // namespace

    namespace fs = std::filesystem;

    GitIndex index_read(const repository::GitRepository &repo)
    {
        std::optional<std::string> index_file = repository::repo_file(repo, false, {"index"});

        if (!index_file || !fs::exists(*index_file))
            return GitIndex();

        std::string raw = utils::read_file_binary(*index_file);

        if (!raw.starts_with("DIRC"))
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

    void index_write(const repository::GitRepository &repo, const GitIndex &index)
    {
        std::optional<std::string> indexFile = repository::repo_file(repo, false, {"index"});
        if (!indexFile)
            throw errors::GitException("Index not found", errors::ExitCode::INDEX_ERROR);

        std::ofstream file(*indexFile, std::ios::binary);

        if (!file.is_open())
            throw errors::GitException("Could not open index file", errors::ExitCode::IO_ERROR);

        file.write("DIRC", 4);
        write_u32(file, index.version);
        write_u32(file, index.entries.size());

        int idx = 0;
        for (const auto &entry : index.entries)
        {
            write_u32(file, entry.ctime_second);
            write_u32(file, entry.ctime_nanosecond);
            write_u32(file, entry.mtime_second);
            write_u32(file, entry.mtime_nanosecond);

            write_u32(file, entry.dev);
            write_u32(file, entry.ino);

            write_u32(file, (entry.mode_type << 12) | entry.mode_perms);

            write_u32(file, entry.uid);
            write_u32(file, entry.gid);

            write_u32(file, entry.fsize);
            // SHA: hex string -> 20 raw bytes
            std::string sha_bytes = utils::hex_to_bytes(entry.sha);
            file.write(sha_bytes.data(), 20);

            // Flags
            uint16_t name_length = entry.name.size() >= 0xFFF ? 0xFFF : entry.name.size();
            uint16_t flag_assume_valid = entry.flag_assume_valid ? 0x8000 : 0;
            uint16_t flags = flag_assume_valid | entry.flag_stage | name_length;
            write_u16(file, flags);

            // Name + null terminator
            file.write(entry.name.data(), entry.name.size());
            file.put(0x00);

            // Padding to next multiple of 8
            idx += 62 + entry.name.size() + 1;
            if (idx % 8 != 0)
            {
                int pad = 8 - (idx % 8);
                for (int i = 0; i < pad; i++)
                    file.put(0x00);
                idx += pad;
            }
        }
    }
} // namespace twig::index