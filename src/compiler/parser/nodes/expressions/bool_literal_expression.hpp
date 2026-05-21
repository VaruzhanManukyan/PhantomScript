#pragma once
#include "expression.hpp"

class BoolLiteralExpression : public IExpression {
public:
    bool value_;

    explicit BoolLiteralExpression(
        bool value,
        std::int32_t line,
        std::int32_t column) :
            value_(value),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~BoolLiteralExpression() = default;
};
