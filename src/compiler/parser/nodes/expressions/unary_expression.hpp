#pragma once
#include <memory>
#include <string>

#include "expression.hpp"

struct UnaryExpression : IExpression {
    std::string operator_;
    std::unique_ptr<IExpression> right_;

    UnaryExpression(std::string operator_, std::unique_ptr<IExpression> right) :
    operator_(std::move(operator_)), right_(std::move(right)) {}
    virtual ~UnaryExpression() = default;
};
