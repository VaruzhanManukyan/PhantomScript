#pragma once
#include <memory>

#include "statement.hpp"
#include "../expressions/expression.hpp"

class ExpressionStatement : public IStatement {
public:
    std::unique_ptr<IExpression> expression_;

    explicit ExpressionStatement(
        std::unique_ptr<IExpression> expression,
        std::int32_t line,
        std::int32_t column) :
            expression_(std::move(expression)),
            IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~ExpressionStatement() = default;
};
