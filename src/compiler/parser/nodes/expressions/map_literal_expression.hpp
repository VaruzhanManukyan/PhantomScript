#pragma once
#include <memory>
#include <vector>

#include "expression.hpp"

class MapLiteralExpression : public IExpression {
public:
    struct KeyValuePair {
        std::unique_ptr<IExpression> key;
        std::unique_ptr<IExpression> value;
    };

    std::vector<KeyValuePair> elements_;

    explicit MapLiteralExpression(
        std::vector<KeyValuePair> elements,
        std::int32_t line,
        std::int32_t column) :
        elements_(std::move(elements)),
        IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~MapLiteralExpression() = default;
};
