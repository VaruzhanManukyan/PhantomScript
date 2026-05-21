#pragma once
#include <memory>
#include <string>

#include "expression.hpp"

class UnaryExpression : public IExpression {
public:
    std::string operator_;
    std::unique_ptr<IExpression> right_;

    explicit UnaryExpression(
        std::string operator_,
        std::unique_ptr<IExpression> right,
        std::int32_t line,
        std::int32_t column) :
            operator_(std::move(operator_)),
            right_(std::move(right)),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~UnaryExpression() = default;
};
