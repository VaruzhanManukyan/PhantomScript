#pragma once
#include <memory>

#include "expression.hpp"
#include "../types/type_node.hpp"

struct CastExpression : IExpression {
    std::unique_ptr<IExpression> value_;
    std::unique_ptr<TypeNode> type_;

    explicit CastExpression(
        std::unique_ptr<IExpression> value,
        std::unique_ptr<TypeNode> type) :
            value_(std::move(value)),
            type_(std::move(type)) {}
    virtual ~CastExpression() = default;
};
