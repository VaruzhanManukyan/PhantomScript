#pragma once
#include "expression.hpp"
#include <memory>
#include <string>

struct AssignmentExpression : IExpression {
    std::unique_ptr<IExpression> target_;
    std::string operator_;
    std::unique_ptr<IExpression> value_;

    AssignmentExpression(std::unique_ptr<IExpression> target, std::string op, std::unique_ptr<IExpression> value)
        : target_(std::move(target)), operator_(std::move(op)), value_(std::move(value)) {}
    virtual ~AssignmentExpression() = default;
};