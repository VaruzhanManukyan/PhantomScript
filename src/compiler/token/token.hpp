#pragma once
#include <cstdint>
#include <string>

#include "enums/token_type.hpp"

struct Token {
    TokenType type;
    std::string lexeme;
    std::int32_t line;
    std::int32_t column;
};
