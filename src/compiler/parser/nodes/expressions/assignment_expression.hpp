#pragma once
#include "expression.hpp"
#include <memory>
#include <string>

class AssignmentExpression : public IExpression {
public:
    std::unique_ptr<IExpression> target_;
    std::string operator_;
    std::unique_ptr<IExpression> value_;

    explicit AssignmentExpression(
        std::unique_ptr<IExpression> target,
        std::string op,
        std::unique_ptr<IExpression> value,
        std::int32_t line,
        std::int32_t column) :
            target_(std::move(target)),
            operator_(std::move(op)),
            value_(std::move(value)),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~AssignmentExpression() = default;
};