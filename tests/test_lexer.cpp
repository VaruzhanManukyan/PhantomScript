#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>

#include "../compiler/lexer/enums/token_type.hpp"
#include "../compiler/lexer/lexer.hpp"

int tests_passed = 0;
int tests_failed = 0;

std::vector<Token> run_lexer(const std::string& code) {
    std::stringstream ss(code);
    Lexer lexer(ss);
    return lexer.tokenize();
}

void assert_token(const Token& t, TokenType expected_type, const std::string& expected_lexeme, int expected_line, int expected_col) {
    if (t.type == expected_type && t.lexeme == expected_lexeme && t.line == expected_line && t.column == expected_col) {
        tests_passed++;
    } else {
        tests_failed++;
        std::cerr << "\n[❌ TEST FAILED]\n"
                  << "  Expected: Type " << (int)expected_type << " | Lexeme '" << expected_lexeme << "' | Pos " << expected_line << ":" << expected_col << "\n"
                  << "  Got:      Type " << (int)t.type << " | Lexeme '" << t.lexeme << "' | Pos " << t.line << ":" << t.column << "\n";
    }
}

void test_operator_lookahead() {
    std::cout << "Running test_operator_lookahead...\n";
    std::vector<Token> tokens = run_lexer("= == => ! != - -= ->");

    assert_token(tokens[0], TokenType::EQ,        "=",  1, 1);
    assert_token(tokens[1], TokenType::EQ_EQ,     "==", 1, 3);
    assert_token(tokens[2], TokenType::FAT_ARROW, "=>", 1, 6);
    assert_token(tokens[3], TokenType::NG,        "!",  1, 9);
    assert_token(tokens[4], TokenType::NG_EQ,     "!=", 1, 11);
    assert_token(tokens[5], TokenType::MINUS,     "-",  1, 14);
    assert_token(tokens[6], TokenType::MINUS_EQ,  "-=", 1, 16);
    assert_token(tokens[7], TokenType::ARROW,     "->", 1, 19);
    assert_token(tokens[8], TokenType::END_OF_FILE, "EOF", 1, 21);
}

void test_glued_tokens() {
    std::cout << "Running test_glued_tokens...\n";
    std::vector<Token> tokens = run_lexer("123var 3.14.15");

    assert_token(tokens[0], TokenType::INT_LITERAL,    "123",  1, 1);
    assert_token(tokens[1], TokenType::IDENTIFIER,     "var",  1, 4);
    assert_token(tokens[2], TokenType::DOUBLE_LITERAL, "3.14", 1, 8);
    assert_token(tokens[3], TokenType::DOT,            ".",    1, 12);
    assert_token(tokens[4], TokenType::INT_LITERAL,    "15",   1, 13);
}

void test_strings_and_errors() {
    std::cout << "Running test_strings_and_errors...\n";

    std::vector<Token> tokens = run_lexer("\"Line1\\nLine2\\\"\"");
    assert_token(tokens[0], TokenType::STRING_LITERAL, "Line1\nLine2\"", 1, 1);

    try {
        run_lexer("mut x = \"Forgot to close");
        std::cerr << "[❌ TEST FAILED] Expected runtime_error for unterminated string!\n";
        tests_failed++;
    } catch (const std::runtime_error& e) {
        std::string err = e.what();
        if (err.find("Unterminated string literal at line 1") != std::string::npos) {
            tests_passed++;
        } else {
            std::cerr << "[❌ TEST FAILED] Wrong error message: " << err << "\n";
            tests_failed++;
        }
    }
}

void test_tricky_comments_and_lines() {
    std::cout << "Running test_tricky_comments_and_lines...\n";
    std::string code =
        "mut a = 1;\n"
        "/* \n"
        "   Line 3 \n"
        "   Line 4 \n"
        "*/ mut b = 2;";

    std::vector<Token> tokens = run_lexer(code);

    assert_token(tokens[0], TokenType::MUT, "mut", 1, 1);
    assert_token(tokens[4], TokenType::SEMICOLON, ";", 1, 10);

    assert_token(tokens[5], TokenType::MUT, "mut", 5, 4);
    assert_token(tokens[6], TokenType::IDENTIFIER, "b", 5, 8);
}

void test_unknown_symbol() {
    std::cout << "Running test_unknown_symbol...\n";
    try {
        run_lexer("let $invalid = 5;");
        std::cerr << "[❌ TEST FAILED] Expected runtime_error for unknown symbol!\n";
        tests_failed++;
    } catch (const std::runtime_error& e) {
        std::string err = e.what();
        if (err.find("Unknown symbol '$'") != std::string::npos) {
            tests_passed++;
        } else {
            std::cerr << "[❌ TEST FAILED] Wrong error message: " << err << "\n";
            tests_failed++;
        }
    }
}

int main() {
    std::cout << "==========================================\n";
    std::cout << "    PHANTOM SCRIPT LEXER UNIT TESTS       \n";
    std::cout << "==========================================\n\n";

    test_operator_lookahead();
    test_glued_tokens();
    test_strings_and_errors();
    test_tricky_comments_and_lines();
    test_unknown_symbol();

    std::cout << "\n==========================================\n";
    std::cout << "Tests Passed: " << tests_passed << "\n";
    if (tests_failed > 0) {
        std::cout << "Tests Failed: " << tests_failed << " ❌\n";
        return 1;
    } else {
        std::cout << "ALL TESTS PASSED! ✅\n";
        std::cout << "Your Lexer is bulletproof.\n";
        return 0;
    }
}