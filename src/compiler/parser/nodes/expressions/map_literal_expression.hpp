#pragma once
#include <memory>
#include <vector>

#include "expression.hpp"

struct MapLiteralExpression : IExpression {
    struct KeyValuePair {
        std::unique_ptr<IExpression> key;
        std::unique_ptr<IExpression> value;
    };

    std::vector<KeyValuePair> elements_;

    explicit MapLiteralExpression(std::vector<KeyValuePair> elements) :
        elements_(std::move(elements)) {}
    virtual ~MapLiteralExpression() = default;
};
