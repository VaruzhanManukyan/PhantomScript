#pragma once
#include <memory>

#include "expression.hpp"
#include "../types/type_node.hpp"

class CastExpression : public IExpression {
public:
    std::unique_ptr<IExpression> value_;
    std::unique_ptr<TypeNode> type_;

    explicit CastExpression(
        std::unique_ptr<IExpression> value,
        std::unique_ptr<TypeNode> type,
        std::int32_t line,
        std::int32_t column) :
            value_(std::move(value)),
            type_(std::move(type)),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~CastExpression() = default;
};
