#include <istream>

#include "lexer.hpp"
#include "enums/lexer_state.hpp"

Lexer::Lexer(std::istream& input) : input_(input) {
    keywords_ = {
        {"true", TokenType::BOOL_LITERAL}, {"false", TokenType::BOOL_LITERAL},

        {"mut", TokenType::MUT}, {"const", TokenType::CONST},

        {"void", TokenType::VOID}, {"bool", TokenType::BOOL},
        {"int", TokenType::INT}, {"double", TokenType::DOUBLE},
        {"string", TokenType::STRING}, {"option", TokenType::OPTION},
        {"result", TokenType::RESULT}, {"array", TokenType::ARRAY},
        {"list", TokenType::LIST}, {"map", TokenType::MAP},
        {"set", TokenType::SET},

        {"if", TokenType::IF}, {"else", TokenType::ELSE},
        {"return", TokenType::RETURN}, {"match", TokenType::MATCH},

        {"fn", TokenType::FN}, {"struct", TokenType::STRUCT},
        {"enum", TokenType::ENUM}, {"contract", TokenType::CONTRACT},
        {"event", TokenType::EVENT}, {"service", TokenType::SERVICE},
        {"server", TokenType::SERVER}, {"database", TokenType::DATABASE},
        {"query", TokenType::QUERY}, {"group", TokenType::GROUP},
        {"route", TokenType::ROUTE}, {"publish", TokenType::PUBLISH},
        {"consumer", TokenType::CONSUMER}, {"client", TokenType::CLIENT},

        {"GET", TokenType::GET}, {"POST", TokenType::POST},
        {"PUT", TokenType::PUT}, {"DELETE", TokenType::DELETE},
        {"PATCH", TokenType::PATCH},

        {"Ok", TokenType::OK}, {"Err", TokenType::ERR},
        {"Some", TokenType::SOME}, {"null", TokenType::NULL_LITERAL},

        {"body", TokenType::BODY}, {"sql", TokenType::SQL},
        {"print", TokenType::PRINT}, {"as", TokenType::AS}
    };
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    LexerState state = LexerState::START;
    std::string buffer;
    bool has_dot = false;

    std::int32_t line = 1;
    std::int32_t column = 1;

    std::int32_t token_start_line = 1;
    std::int32_t token_start_column = 1;

    auto advance = [&]() -> char {
        char symbol = static_cast<char>(input_.get());
        if (symbol == '\n') {
            column = 1;
            ++line;
        } else {
            ++column;
        }
        return symbol;
    };

    while (true) {
        std::istream::int_type next = input_.peek();

        if (next == std::char_traits<char>::eof()) {
            if (state == LexerState::IN_STRING || state == LexerState::IN_STRING_ESCAPE) {
                throw std::runtime_error("Unterminated string literal at line " + std::to_string(token_start_line));
            }
            if (state == LexerState::IN_COMMENT_MULTI) {
                throw std::runtime_error("Unterminated multi-line comment at line " + std::to_string(token_start_line));
            }

            if (state == LexerState::IN_IDENTIFIER) {
                auto it = keywords_.find(buffer);
                if (it != keywords_.end()) {
                    tokens.push_back({it->second, buffer, token_start_line, token_start_column});
                } else {
                    tokens.push_back({TokenType::IDENTIFIER, buffer, token_start_line, token_start_column});
                }
            } else if (state == LexerState::IN_NUMBER) {
                TokenType type = has_dot ? TokenType::DOUBLE_LITERAL : TokenType::INT_LITERAL;
                tokens.push_back({type, buffer, token_start_line, token_start_column});
            }
            break;
        }

        char symbol = static_cast<char>(next);

        if (state == LexerState::START) {
            token_start_line = line;
            token_start_column = column;

            if (std::isspace(static_cast<unsigned char>(symbol))) {
                advance();
                continue;
            }

            if (std::isalpha(static_cast<unsigned char>(symbol)) || symbol == '_') {
                buffer.clear();
                buffer += advance();
                state = LexerState::IN_IDENTIFIER;
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(symbol))) {
                buffer.clear();
                has_dot = false;
                buffer += advance();
                state = LexerState::IN_NUMBER;
                continue;
            }

            if (symbol == '"') {
                buffer.clear();
                advance();
                state = LexerState::IN_STRING;
                continue;
            }

            advance();

            switch (symbol) {
                case '/': {
                    if (input_.peek() == '/') {
                        advance();
                        state = LexerState::IN_COMMENT_SINGLE;
                    } else if (input_.peek() == '*') {
                        advance();
                        state = LexerState::IN_COMMENT_MULTI;
                    } else if (input_.peek() == '=') {
                        advance();
                        tokens.push_back({TokenType::SLASH_EQ, "/=", token_start_line, token_start_column});
                    } else {
                        tokens.push_back({TokenType::SLASH, "/", token_start_line, token_start_column});
                    }
                    break;
                }
                case '=': {
                    if (input_.peek() == '=') {
                        advance();
                        tokens.push_back({TokenType::EQ_EQ, "==", token_start_line, token_start_column});
                    } else if (input_.peek() == '>') {
                        advance();
                        tokens.push_back({TokenType::FAT_ARROW, "=>", token_start_line, token_start_column});
                    } else {
                        tokens.push_back({TokenType::EQ, "=", token_start_line, token_start_column});
                    }
                    break;
                }
                case '!': {
                    if (input_.peek() == '=') {
                        advance();
                        tokens.push_back({TokenType::NG_EQ, "!=", token_start_line, token_start_column});
                    } else {
                        tokens.push_back({TokenType::NG, "!", token_start_line, token_start_column});
                    }
                    break;
                }
                case '>': {
                    if (input_.peek() == '=') {
                        advance();
                        tokens.push_back({TokenType::GT_EQ, ">=", token_start_line, token_start_column});
                    } else {
                        tokens.push_back({TokenType::GT, ">", token_start_line, token_start_column});
                    }
                    break;
                }
                case '<': {
                    if (input_.peek() == '=') {
                        advance();
                        tokens.push_back({TokenType::LT_EQ, "<=", token_start_line, token_start_column});
                    } else {
                        tokens.push_back({TokenType::LT, "<", token_start_line, token_start_column});
                    }
                    break;
                }
                case '+': {
                    if (input_.peek() == '=') {
                        advance();
                        tokens.push_back({TokenType::PLUS_EQ, "+=", token_start_line, token_start_column});
                    } else {
                        tokens.push_back({TokenType::PLUS, "+", token_start_line, token_start_column});
                    }
                    break;
                }
                case '-': {
                    if (input_.peek() == '=') {
                        advance();
                        tokens.push_back({TokenType::MINUS_EQ, "-=", token_start_line, token_start_column});
                    } else if (input_.peek() == '>') {
                        advance();
                        tokens.push_back({TokenType::ARROW, "->", token_start_line, token_start_column});
                    } else {
                        tokens.push_back({TokenType::MINUS, "-", token_start_line, token_start_column});
                    }
                    break;
                }
                case '*': {
                    if (input_.peek() == '=') {
                        advance();
                        tokens.push_back({TokenType::STAR_EQ, "*=", token_start_line, token_start_column});
                    } else {
                        tokens.push_back({TokenType::STAR, "*", token_start_line, token_start_column});
                    }
                    break;
                }
                case '%': {
                    if (input_.peek() == '=') {
                        advance();
                        tokens.push_back({TokenType::PERCENT_EQ, "%=", token_start_line, token_start_column});
                    } else {
                        tokens.push_back({TokenType::PERCENT, "%", token_start_line, token_start_column});
                    }
                    break;
                }
                case '&': {
                    if (input_.peek() == '&') {
                        advance();
                        tokens.push_back({TokenType::AND_AND, "&&", token_start_line, token_start_column});
                    }
                    break;
                }
                case '|': {
                    if (input_.peek() == '|') {
                        advance();
                        tokens.push_back({TokenType::OR_OR, "||", token_start_line, token_start_column});
                    }
                    break;
                }
                case '.':
                    tokens.push_back({TokenType::DOT, ".", token_start_line, token_start_column});
                    break;
                case '?':
                    tokens.push_back({TokenType::QUESTION, "?", token_start_line, token_start_column});
                    break;
                case ':':
                    tokens.push_back({TokenType::COLON, ":", token_start_line, token_start_column});
                    break;
                case ';':
                    tokens.push_back({TokenType::SEMICOLON, ";", token_start_line, token_start_column});
                    break;
                case '{':
                    tokens.push_back({TokenType::LBRACE, "{", token_start_line, token_start_column});
                    break;
                case '}':
                    tokens.push_back({TokenType::RBRACE, "}", token_start_line, token_start_column});
                    break;
                case '(':
                    tokens.push_back({TokenType::LPAREN, "(", token_start_line, token_start_column});
                    break;
                case ')':
                    tokens.push_back({TokenType::RPAREN, ")", token_start_line, token_start_column});
                    break;
                case '[':
                    tokens.push_back({TokenType::LBRACKET, "[", token_start_line, token_start_column});
                    break;
                case ']':
                    tokens.push_back({TokenType::RBRACKET, "]", token_start_line, token_start_column});
                    break;
                default:
                    throw std::runtime_error(
                        "Syntax Error: Unknown symbol '" + std::string(1, symbol) +
                        "' at line " + std::to_string(token_start_line) +
                        ", column " + std::to_string(token_start_column)
                    );
            }
            continue;
        }

        if (state == LexerState::IN_IDENTIFIER) {
            if (std::isalnum(static_cast<unsigned char>(symbol)) || symbol == '_') {
                buffer += advance();
            } else {
                auto it = keywords_.find(buffer);
                if (it != keywords_.end()) {
                    tokens.push_back({it->second, buffer, token_start_line, token_start_column});
                } else {
                    tokens.push_back({TokenType::IDENTIFIER, buffer, token_start_line, token_start_column});
                }
                state = LexerState::START;
            }
            continue;
        }

        if (state == LexerState::IN_NUMBER) {
            if (std::isdigit(static_cast<unsigned char>(symbol))) {
                buffer += advance();
            } else if (symbol == '.' && !has_dot) {
                has_dot = true;
                buffer += advance();
            } else {
                TokenType type = has_dot ? TokenType::DOUBLE_LITERAL : TokenType::INT_LITERAL;
                tokens.push_back({type, buffer, token_start_line, token_start_column});
                state = LexerState::START;
            }
            continue;
        }

        if (state == LexerState::IN_STRING) {
            if (symbol == '"') {
                advance();
                tokens.push_back({TokenType::STRING_LITERAL, buffer, token_start_line, token_start_column});
                state = LexerState::START;
            } else if (symbol == '\\') {
                advance();
                state = LexerState::IN_STRING_ESCAPE;
            } else {
                buffer += advance();
            }
            continue;
        }

        if (state == LexerState::IN_STRING_ESCAPE) {
            char escaped = advance();
            switch (escaped) {
                case 'n':
                    buffer += '\n';
                    break;
                case 't':
                    buffer += '\t';
                    break;
                case 'r':
                    buffer += '\r';
                    break;
                case '\\':
                    buffer += '\\';
                    break;
                case '"':
                    buffer += '"';
                    break;
                default:
                    buffer += escaped;
                    break;
            }
            state = LexerState::IN_STRING;
            continue;
        }

        if (state == LexerState::IN_COMMENT_SINGLE) {
            if (symbol == '\n') {
                advance();
                state = LexerState::START;
            } else {
                advance();
            }
            continue;
        }

        if (state == LexerState::IN_COMMENT_MULTI) {
            char comment_symbol = advance();
            if (comment_symbol == '*' && input_.peek() == '/') {
                advance();
                state = LexerState::START;
            }
        }
    }

    tokens.push_back({TokenType::END_OF_FILE, "EOF", line, column});
    return tokens;
}
