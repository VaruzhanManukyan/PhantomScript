#pragma once
#include <memory>
#include <vector>

#include "expression.hpp"

struct ListLiteralExpression : IExpression {
    std::vector<std::unique_ptr<IExpression>> elements_;

    explicit ListLiteralExpression(std::vector<std::unique_ptr<IExpression>> elements) :
        elements_(std::move(elements)) {}
    virtual ~ListLiteralExpression() = default;
};
