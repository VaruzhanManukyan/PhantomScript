#pragma once
#include <memory>
#include <vector>

#include "expression.hpp"

class SetLiteralExpression : public IExpression {
public:
    std::vector<std::unique_ptr<IExpression>> elements;

    explicit SetLiteralExpression(
        std::vector<std::unique_ptr<IExpression>> elements,
        std::int32_t line,
        std::int32_t column) :
            elements(std::move(elements)),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~SetLiteralExpression() = default;
};
