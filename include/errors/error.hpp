#pragma once

#include <stdexcept>

/**
 * @brief Error handling primitives for Twig.
 *
 * All commands signal failure by throwing a GitException. The exit code
 * embedded in the exception is intended to be returned directly from main(),
 * so callers higher up the stack don't need to map exception types to integers.
 */
namespace twig::errors
{
    /**
     * @brief Process exit codes for every failure mode Twig can encounter.
     *
     * The numeric values are stable — external tooling or shell scripts may
     * test `$?` against them. SUCCESS is explicitly 0 so it round-trips
     * correctly through main()'s return value.
     */
    enum class ExitCode
    {
        SUCCESS = 0,             ///< Command completed without error.
        FAILURE = 1,             ///< Generic fallback; prefer a more specific code.
        NOT_A_REPO = 2,          ///< repo_find() walked to filesystem root with no .git found.
        OBJECT_NOT_FOUND = 3,    ///< Requested SHA-1 does not exist in the object store.
        MALFORMED_OBJECT = 4,    ///< Object header or body failed to parse.
        UNKNOWN_OBJECT_TYPE = 5, ///< Type field in object header is not blob/tree/commit/tag.
        INDEX_ERROR = 6,         ///< .git/index could not be read or written.
        IO_ERROR = 7,            ///< Filesystem read/write operation failed.
        INVALID_ARGS = 8,        ///< Command received the wrong number or type of arguments.
    };

    /**
     * @brief Exception thrown by all Twig commands on failure.
     *
     * Wraps a human-readable message (accessible via what()) and an ExitCode
     * that main() returns directly to the shell. Inheriting from
     * std::runtime_error means it's caught by any handler that catches
     * std::exception, giving library consumers a natural fallback.
     *
     * @note Never throw GitException with ExitCode::SUCCESS.
     *
     * @see ExitCode
     */
    struct GitException : public std::runtime_error
    {
        ExitCode code; ///< Exit code to return from main().

        /**
         * @brief Constructs a GitException with a message and exit code.
         *
         * @param message Human-readable description of what went wrong.
         * @param ex_code Specific ExitCode classifying the failure.
         */
        GitException(const std::string &message, ExitCode ex_code)
            : std::runtime_error(message), code(ex_code) {}
    };

} // namespace twig::errors