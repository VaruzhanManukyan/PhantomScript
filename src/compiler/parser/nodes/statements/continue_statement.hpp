#pragma once
#include "statement.hpp"
#include "../ast_node.hpp"

class ContinueStatement : public IStatement {
public:
    explicit ContinueStatement(std::int32_t line, std::int32_t column) :
        IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~ContinueStatement() = default;
};