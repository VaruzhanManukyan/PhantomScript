#pragma once
#include <memory>
#include <vector>

#include "statement.hpp"

class BlockStatement : public IStatement {
public:
    std::vector<std::unique_ptr<IStatement>> statements_;

    explicit BlockStatement(
        std::vector<std::unique_ptr<IStatement>> statements,
        std::int32_t line,
        std::int32_t column) :
            statements_(std::move(statements)),
            IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~BlockStatement() = default;
};
