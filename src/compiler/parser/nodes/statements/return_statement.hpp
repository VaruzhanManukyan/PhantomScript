#pragma once
#include <memory>

#include "statement.hpp"
#include "../expressions/expression.hpp"

class ReturnStatement : public IStatement {
public:
    explicit ReturnStatement(
        std::int32_t line,
        std::int32_t column) :
            IStatement(line, column) {}

    std::unique_ptr<IExpression> expression_;

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~ReturnStatement() = default;
};