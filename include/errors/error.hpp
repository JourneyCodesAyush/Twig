#pragma once

enum class ExitCode
{
    SUCCESS = 0,    // explicit 0 so main() can return it directly
    FAILURE = 1,    // generic fallback
    NOT_A_REPO = 2, // repo_find() walked to root and found nothing
    OBJECT_NOT_FOUND = 3,
    MALFORMED_OBJECT = 4,
    UNKNOWN_OBJECT_TYPE = 5,
    INDEX_ERROR = 6,
    IO_ERROR = 7,     // file read/write failures
    INVALID_ARGS = 8, // wrong args passed to a command
};