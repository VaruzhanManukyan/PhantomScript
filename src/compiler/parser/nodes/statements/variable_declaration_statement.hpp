#pragma once
#include <memory>

#include "statement.hpp"
#include "../types/type_node.hpp"
#include "../expressions/expression.hpp"

class VariableDeclarationStatement : public IStatement {
public:
    bool is_mut_;
    std::string name_;
    std::unique_ptr<TypeNode> type_;
    std::unique_ptr<IExpression> initializer_;

    explicit VariableDeclarationStatement(
        bool is_mut,
        std::string name,
        std::unique_ptr<TypeNode> type,
        std::unique_ptr<IExpression> initializer,
        std::int32_t line,
        std::int32_t column) :
            is_mut_(is_mut),
            name_(std::move(name)),
            type_(std::move(type)),
            initializer_(std::move(initializer)),
            IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~VariableDeclarationStatement() = default;
};