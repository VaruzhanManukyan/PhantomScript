#pragma once
#include <memory>

#include "statement.hpp"
#include "../expressions/expression.hpp"

struct ExpressionStatement : IStatement {
    std::unique_ptr<IExpression> expression_;

    explicit ExpressionStatement(std::unique_ptr<IExpression> expression) :
        expression_(std::move(expression)) {}
    virtual ~ExpressionStatement() = default;
};
