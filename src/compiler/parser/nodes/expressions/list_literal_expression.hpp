#pragma once
#include <memory>
#include <vector>

#include "expression.hpp"

class ListLiteralExpression : public IExpression {
public:
    std::vector<std::unique_ptr<IExpression>> elements_;

    explicit ListLiteralExpression(
        std::vector<std::unique_ptr<IExpression>> elements,
        std::int32_t line,
        std::int32_t column) :
        elements_(std::move(elements)),
        IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~ListLiteralExpression() = default;
};
