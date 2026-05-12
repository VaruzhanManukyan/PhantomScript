#pragma once
#include <memory>
#include <vector>

#include "expression.hpp"

struct SetLiteralExpression : IExpression {
    std::vector<std::unique_ptr<IExpression>> elements;

    explicit SetLiteralExpression(std::vector<std::unique_ptr<IExpression>> elements) :
        elements(std::move(elements)) {}
    virtual ~SetLiteralExpression() = default;
};
