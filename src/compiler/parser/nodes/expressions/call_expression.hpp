#pragma once
#include <memory>
#include <vector>

#include "expression.hpp"

struct CallExpression : IExpression {
    std::unique_ptr<IExpression> callee_;
    std::vector<std::unique_ptr<IExpression>> arguments_;

    CallExpression(
        std::unique_ptr<IExpression> callee,
        std::vector<std::unique_ptr<IExpression>> arguments) :
            callee_(std::move(callee)),
            arguments_(std::move(arguments)) {}
    virtual ~CallExpression() = default;
};
