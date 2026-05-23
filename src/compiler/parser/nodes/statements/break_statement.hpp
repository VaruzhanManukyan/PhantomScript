#pragma once
#include "statement.hpp"
#include "../ast_node.hpp"

class BreakStatement : public IStatement {
public:
    explicit BreakStatement(std::int32_t line, std::int32_t column) :
        IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }
    
    virtual ~BreakStatement() = default;
};