#pragma once
#include <memory>
#include <vector>

#include "expression.hpp"

struct ArrayLiteralExpression : IExpression {
    std::vector<std::unique_ptr<IExpression>> elements_;

    explicit ArrayLiteralExpression(std::vector<std::unique_ptr<IExpression>> elements) :
        elements_(std::move(elements)) {}
    virtual ~ArrayLiteralExpression() = default;
};
