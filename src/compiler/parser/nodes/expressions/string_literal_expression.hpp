#pragma once
#include <string>

#include "expression.hpp"

class StringLiteralExpression : public IExpression {
public:
    std::string value_;

    explicit StringLiteralExpression(
        std::string value,
        std::int32_t line,
        std::int32_t column) :
            value_(std::move(value)),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~StringLiteralExpression() = default;
};
