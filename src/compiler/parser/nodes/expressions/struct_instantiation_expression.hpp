#pragma once
#include <string>
#include <memory>
#include <vector>

#include "expression.hpp"

struct StructInstantiationExpression : IExpression {
    std::string struct_name;
    struct FieldInit {
        std::string name_;
        std::unique_ptr<IExpression> value_;
    };
    std::vector<FieldInit> fields_;

    explicit StructInstantiationExpression(
        std::string struct_name,
        std::vector<FieldInit> fields) :
            struct_name(std::move(struct_name)),
            fields_(std::move(fields)) {}
    virtual ~StructInstantiationExpression() = default;
};
