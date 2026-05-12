#pragma once
#include <memory>

#include "expression.hpp"
struct BinaryExpression : IExpression {
    std::unique_ptr<IExpression> left_;
    std::string operator_;
    std::unique_ptr<IExpression> right_;

    BinaryExpression(
        std::unique_ptr<IExpression> left,
        std::string operator_,
        std::unique_ptr<IExpression> right) :
            left_(std::move(left)),
            operator_(std::move(operator_)),
            right_(std::move(right)) {}
    virtual ~BinaryExpression() = default;
};