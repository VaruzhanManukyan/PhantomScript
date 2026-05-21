#pragma once
#include <memory>

#include "statement.hpp"
#include "block_statement.hpp"
#include "../expressions/expression.hpp"

class IfStatement : public IStatement {
public:
    std::unique_ptr<IExpression> expression_;
    std::unique_ptr<BlockStatement> then_branch_;
    std::unique_ptr<IStatement> else_branch_;

    explicit IfStatement(
        std::unique_ptr<IExpression> expression,
        std::unique_ptr<BlockStatement> then_branch,
        std::unique_ptr<IStatement> else_branch,
        std::int32_t line,
        std::int32_t column) :
            expression_(std::move(expression)),
            then_branch_(std::move(then_branch)),
            else_branch_(std::move(else_branch)),
            IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~IfStatement() = default;
};
