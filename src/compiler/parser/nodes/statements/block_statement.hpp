#pragma once
#include <memory>
#include <vector>

#include "statement.hpp"

struct BlockStatement : IStatement {
    std::vector<std::unique_ptr<IStatement>> statements_;

    explicit BlockStatement(std::vector<std::unique_ptr<IStatement>> statements) :
        statements_(std::move(statements)) {}
    virtual ~BlockStatement() = default;
};
