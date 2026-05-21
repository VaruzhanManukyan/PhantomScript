#pragma once
#include <string>
#include <vector>

#include "declaration.hpp"

class EnumDeclaration : public IDeclaration {
public:
    std::string name_;
    std::vector<std::string> variants_;

    explicit EnumDeclaration(
        std::int32_t line,
        std::int32_t column) :
            IDeclaration(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~EnumDeclaration() = default;
};