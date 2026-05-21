#pragma once
#include "expression.hpp"

class DoubleLiteralExpression : public IExpression {
public:
    double value_;

    explicit DoubleLiteralExpression(
        double value,
        std::int32_t line,
        std::int32_t column) :
            value_(value),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~DoubleLiteralExpression() = default;
};