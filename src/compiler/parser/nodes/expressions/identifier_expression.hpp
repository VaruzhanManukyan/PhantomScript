#pragma once
#include <string>

#include "expression.hpp"

struct IdentifierExpression : IExpression {
    std::string name_;

    explicit IdentifierExpression(std::string name) : name_(name) {}
    virtual ~IdentifierExpression() = default;
};