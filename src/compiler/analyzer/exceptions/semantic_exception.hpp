#pragma once
#include <stdexcept>
#include <string>
#include <cstdint>

struct SemanticException : std::runtime_error {
    std::int32_t line_;
    std::int32_t column_;

    explicit SemanticException(const std::string& message, std::int32_t line, std::int32_t column)
        : std::runtime_error(message +
            " at line " + std::to_string(line) +
            ", column " + std::to_string(column)),
          line_(line),
          column_(column) {}
};