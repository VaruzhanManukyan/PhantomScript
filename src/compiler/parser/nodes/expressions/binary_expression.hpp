#pragma once
#include <memory>

#include "expression.hpp"

class BinaryExpression : public IExpression {
public:
    std::unique_ptr<IExpression> left_;
    std::string operator_;
    std::unique_ptr<IExpression> right_;

    explicit BinaryExpression(
        std::unique_ptr<IExpression> left,
        std::string operator_,
        std::unique_ptr<IExpression> right,
        std::int32_t line,
        std::int32_t column) :
            left_(std::move(left)),
            operator_(std::move(operator_)),
            right_(std::move(right)),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~BinaryExpression() = default;
};