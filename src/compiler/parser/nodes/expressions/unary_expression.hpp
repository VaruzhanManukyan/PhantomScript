#pragma once
#include <memory>
#include <string>

#include "expression.hpp"

struct unary_expression : IExpression {
    std::string operator_;
    std::unique_ptr<IExpression> right_;

    unary_expression(std::string operator_, std::unique_ptr<IExpression> right) :
    operator_(std::move(operator_)), right_(std::move(right)) {}
    virtual ~unary_expression() = default;
};
