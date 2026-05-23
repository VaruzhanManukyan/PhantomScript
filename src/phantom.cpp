#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

#include "compiler/lexer/lexer.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/analyzer/analyzer.hpp"
#include "compiler/code_generator/code_generator.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: phantom <file1.ps> <file2.ps> ...\n";
        return 1;
    }

    fs::create_directory("build");

    for (std::size_t i = 1; i < argc; ++i) {
        fs::path input_path(argv[i]);
        std::ifstream file(input_path);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << input_path << "\n";
            continue;
        }

        std::cout << "[Phantom] Compiling " << input_path.filename() << "...\n";

        std::stringstream buffer;
        buffer << file.rdbuf();
        Lexer lexer(buffer);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        std::unique_ptr<ProgramNode> ast = parser.parse();

        SemanticAnalyzer analyzer;
        if (!analyzer.analyze(*ast)) {
            std::cerr << "Semantic errors in " << input_path.filename() << ". Skipping.\n";
            continue;
        }

        CodeGenerator generator;
        std::string stem = input_path.stem().string();
        CompilationArtifacts art = generator.generate(*ast, stem);

        std::ofstream("build/" + stem + ".hpp") << art.hpp_code;
        std::ofstream("build/" + stem + ".cpp") << art.cpp_code;

        if (art.cmake_lists) {
            std::ofstream("build/CMakeLists.txt") << *art.cmake_lists;
        }
        if (art.dockerfile) {
            std::ofstream("build/Dockerfile") << *art.dockerfile;
        }
        if (art.docker_compose) {
            std::ofstream("build/docker-compose.yml") << *art.docker_compose;
        }

        std::cout << "[Success] Generated build/" << stem << ".{hpp,cpp}\n";
    }

    return 0;
}