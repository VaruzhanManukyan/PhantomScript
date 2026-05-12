#pragma once
#include <memory>

#include "statement.hpp"
#include "block_statement.hpp"
#include "../expressions/expression.hpp"

struct IfStatement : IStatement {
    std::unique_ptr<IExpression> expression_;
    std::unique_ptr<BlockStatement> then_branch_;
    std::unique_ptr<IStatement> else_branch_;

    explicit IfStatement(
        std::unique_ptr<IExpression> expression,
        std::unique_ptr<BlockStatement> then_branch,
        std::unique_ptr<IStatement> else_branch) :
            expression_(std::move(expression)),
            then_branch_(std::move(then_branch)),
            else_branch_(std::move(else_branch)) {}
    virtual ~IfStatement() = default;
};
