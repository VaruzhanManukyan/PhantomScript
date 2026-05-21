#pragma once
#include "expression.hpp"

class NullLiteralExpression : public IExpression {
public:
    explicit NullLiteralExpression(
        std::int32_t line,
        std::int32_t column) :
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~NullLiteralExpression() = default;
};
