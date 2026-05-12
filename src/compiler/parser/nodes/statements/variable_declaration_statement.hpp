#pragma once
#include <memory>

#include "statement.hpp"
#include "../type/type_node.hpp"
#include "../expressions/expression.hpp"

struct VariableDeclarationStatement : IStatement {
    bool is_mut_;
    std::string name_;
    std::unique_ptr<TypeNode> type_;
    std::unique_ptr<IExpression> initializer_;

    explicit VariableDeclarationStatement(
        bool is_mut,
        std::string name,
        std::unique_ptr<TypeNode> type,
        std::unique_ptr<IExpression> initializer) :
            is_mut_(is_mut),
            name_(std::move(name)),
            type_(std::move(type)),
            initializer_(std::move(initializer)) {}
    virtual ~VariableDeclarationStatement() = default;
};