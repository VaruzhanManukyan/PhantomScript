#pragma once
#include <cstdint>

#include "expression.hpp"

struct IntLIteralExpression : IExpression {
    std::int32_t value_;

    explicit IntLIteralExpression(std::int32_t value) : value_(value) {}
    virtual ~IntLIteralExpression() = default;
};
