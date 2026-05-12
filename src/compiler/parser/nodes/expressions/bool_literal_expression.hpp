#pragma once
#include "expression.hpp"

struct BoolLiteralExpression : IExpression {
    bool value_;

    BoolLiteralExpression(bool value) : value_(value) {}
    virtual ~BoolLiteralExpression() = default;
};
