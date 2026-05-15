#pragma once
#include <memory>

#include "statement.hpp"
#include "../expressions/expression.hpp"

struct ReturnStatement : IStatement {
    std::unique_ptr<IExpression> expression_;
    virtual ~ReturnStatement() = default;
};