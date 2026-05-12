#pragma once
#include "expression.hpp"

struct DoubleLiteralExpression : IExpression {
    double value_;

    explicit DoubleLiteralExpression(double value) : value_(value) {}
    virtual ~DoubleLiteralExpression() = default;
};