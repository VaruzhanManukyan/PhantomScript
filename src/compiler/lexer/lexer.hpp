#pragma once
#include <iosfwd>
#include <vector>
#include <unordered_map>

#include "../token/token.hpp"

class Lexer {
private:
    std::istream& input_;
    std::unordered_map<std::string, TokenType> keywords_;
public:
    Lexer(std::istream& input);

    std::vector<Token> tokenize();
};
