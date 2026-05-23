#pragma once
#include <memory>

#include "statement.hpp"
#include "block_statement.hpp"
#include "../expressions/expression.hpp"

class WhileStatement : public IStatement {
public:
    std::unique_ptr<IExpression> condition_;
    std::unique_ptr<BlockStatement> body_;

    explicit WhileStatement(
        std::unique_ptr<IExpression> condition,
        std::unique_ptr<BlockStatement> body,
        std::int32_t line, std::int32_t column) :
            condition_(std::move(condition)),
            body_(std::move(body)),
            IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~WhileStatement() = default;

};