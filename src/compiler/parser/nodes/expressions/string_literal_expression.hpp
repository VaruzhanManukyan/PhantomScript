#pragma once
#include <string>

#include "expression.hpp"

struct StringLiteralExpression : IExpression {
    std::string value_;

    explicit StringLiteralExpression(std::string value) : value_(std::move(value)) {};
    virtual ~StringLiteralExpression() = default;
};
