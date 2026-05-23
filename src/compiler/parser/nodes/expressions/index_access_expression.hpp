#pragma once
#include <memory>
#include "expression.hpp"

class IndexAccessExpression : public IExpression {
public:
    std::unique_ptr<IExpression> object_;
    std::unique_ptr<IExpression> index_;

    IndexAccessExpression(
        std::unique_ptr<IExpression> object,
        std::unique_ptr<IExpression> index,
        std::int32_t line, std::int32_t column) :
            object_(std::move(object)),
            index_(std::move(index)),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~IndexAccessExpression() = default;
};