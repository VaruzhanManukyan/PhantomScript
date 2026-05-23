#pragma once
#include <memory>
#include <string>

#include "statement.hpp"
#include "block_statement.hpp"
#include "../expressions/expression.hpp"

class ForInStatement : public IStatement {
public:
    std::string variable_name_;
    std::unique_ptr<IExpression> iterable_;
    std::unique_ptr<BlockStatement> body_;

    explicit ForInStatement(
        std::string variable_name,
        std::unique_ptr<IExpression> iterable,
        std::unique_ptr<BlockStatement> body,
        std::int32_t line, std::int32_t column) :
            variable_name_(std::move(variable_name)),
            iterable_(std::move(iterable)),
            body_(std::move(body)),
            IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~ForInStatement() = default;
};