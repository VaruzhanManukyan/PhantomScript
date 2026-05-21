#pragma once
#include <memory>
#include <vector>

#include "expression.hpp"

class CallExpression : public IExpression {
public:
    std::unique_ptr<IExpression> callee_;
    std::vector<std::unique_ptr<IExpression>> arguments_;

    explicit CallExpression(
        std::unique_ptr<IExpression> callee,
        std::vector<std::unique_ptr<IExpression>> arguments,
        std::int32_t line,
        std::int32_t column) :
            callee_(std::move(callee)),
            arguments_(std::move(arguments)),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~CallExpression() = default;
};
