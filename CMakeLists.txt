cmake_minimum_required(VERSION 3.22)
project(PhantomScript)

set(CMAKE_CXX_STANDARD 20)

add_executable(TestLexer tests/test_lexer.cpp src/compiler/lexer/lexer.cpp)