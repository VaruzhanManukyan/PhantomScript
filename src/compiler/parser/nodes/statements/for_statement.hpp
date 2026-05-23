#pragma once
#include <memory>

#include "statement.hpp"
#include "block_statement.hpp"
#include "../expressions/expression.hpp"

class ForStatement : public IStatement {
public:
    std::unique_ptr<IStatement> init_;
    std::unique_ptr<IExpression> condition_;
    std::unique_ptr<IExpression> increment_;
    std::unique_ptr<BlockStatement> body_;

    ForStatement(
        std::unique_ptr<IStatement> init,
        std::unique_ptr<IExpression> condition,
        std::unique_ptr<IExpression> increment,
        std::unique_ptr<BlockStatement> body,
        std::int32_t line, std::int32_t column) :
            init_(std::move(init)),
            condition_(std::move(condition)),
            increment_(std::move(increment)),
            body_(std::move(body)),
            IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~ForStatement() = default;
};