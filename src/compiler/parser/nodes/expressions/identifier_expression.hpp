#pragma once
#include <string>

#include "expression.hpp"

class IdentifierExpression : public IExpression {
public:
    std::string name_;

    explicit IdentifierExpression(
        std::string name,
        std::int32_t line,
        std::int32_t column) :
            name_(name),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~IdentifierExpression() = default;
};