/**
 * @file utils.hpp
 * @brief Miscellaneous utility functions used throughout Twig.
 *
 * This module contains shared helpers that do not belong to a specific
 * repository subsystem, including:
 *
 * - SHA-1 hashing.
 * - zlib compression and decompression.
 * - Filesystem I/O helpers.
 * - Hexadecimal encoding and decoding.
 * - String manipulation and pattern matching.
 *
 * These functions are intentionally stateless and may be used by any
 * component of the application.
 *
 * @note Filesystem and parsing failures are reported via GitException.
 */

#pragma once

#include <string>

namespace twig::utils
{
    /**
     * @brief Compute the SHA-1 digest of a string.
     *
     * @return 40-character hexadecimal SHA-1 hash.
     */
    std::string sha1(const std::string &content);

    /**
     * @brief Compress data using zlib.
     */
    std::string compress(const std::string &content);

    /**
     * @brief Decompress zlib-compressed data.
     */
    std::string decompress(const std::string &content);

    /**
     * @brief Strips leading and trailing whitespace, tabs, and newlines.
     */
    std::string strip(const std::string &s);

    /**
     * @brief Match a path against a Git-style ignore pattern.
     *
     * Wraps the platform fnmatch implementation and includes Twig-specific
     * handling for certain directory patterns.
     *
     *  @note Intended for Git ignore matching semantics rather than general
     *        shell glob expansion.
     */
    bool fnmatch(std::string path, std::string pattern);

    /**
     * @brief Convert raw bytes to a hexadecimal string.
     */
    std::string bytes_to_hex(const std::string &bytes);
    /**
     * @brief Convert a hexadecimal string to raw bytes.
     */
    std::string hex_to_bytes(const std::string &hex);
    bool is_hex(const std::string &s);

    void write_file(const std::string &path, const std::string &message);
    std::string read_file(const std::string &filename);
    std::string read_file_binary(const std::string &filename);
    void write_file_binary(const std::string &path, const std::string &message);
} // namespace twig::utils
