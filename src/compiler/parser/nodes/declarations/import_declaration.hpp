#pragma once
#include <string>
#include "declaration.hpp"

class ImportDeclaration : public IDeclaration {
public:
    std::string path_;

    ImportDeclaration(std::string path, std::int32_t line, std::int32_t column)
        : IDeclaration(line, column), path_(std::move(path)) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~ImportDeclaration() = default;
};