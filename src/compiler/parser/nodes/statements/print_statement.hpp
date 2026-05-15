#pragma once
#include <memory>

#include "statement.hpp"
#include "../expressions/expression.hpp"
class PrintStatement : public IStatement {
public:
    std::unique_ptr<IExpression> expression_;

    explicit PrintStatement(std::unique_ptr<IExpression> expression)
        : expression_(std::move(expression)) {}
};