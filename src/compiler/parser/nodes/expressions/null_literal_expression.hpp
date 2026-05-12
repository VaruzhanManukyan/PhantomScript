#pragma once
#include "expression.hpp"

struct NullLiteralExpression : IExpression {
    explicit NullLiteralExpression() = default;
    virtual ~NullLiteralExpression() = default;
};
