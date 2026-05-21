#pragma once
#include <cstdint>

#include "expression.hpp"

class IntLIteralExpression : public IExpression {
public:
    std::int32_t value_;

    explicit IntLIteralExpression(
        std::int32_t value,
        std::int32_t line,
        std::int32_t column) :
            value_(value),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~IntLIteralExpression() = default;
};
